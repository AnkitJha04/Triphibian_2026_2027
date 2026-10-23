"""Offline integration tests for the local command bridge; no ESP32 is required."""
from __future__ import annotations

import json
import threading
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

import bridge


class FakeSerialPort:
    """Small test double that injects device replies into SerialController's reader buffer."""
    def __init__(self, owner):
        self.owner = owner
        self.is_open = True
        self.port = "FAKE-COM"

    def write(self, data: bytes):
        command = data.decode("ascii").strip()
        if command == "PREFLIGHT":
            self._line("PREFLIGHT BLOCKED | TEST DOUBLE ONLY")
        elif command == "GETPID":
            self._line("PID,ROLL_RATE,KP,0.035000,KI,0.004000,KD,0.000200,IMAX,8.000,OUTMAX,0.080")
        elif command == "S":
            self._line("DISARMED: OPERATOR STOP")
        elif command == "CAL_GYRO":
            # The real procedure samples the MPU6050 for approximately one second.
            threading.Thread(target=lambda: (time.sleep(1.05), self._line(
                "CAL_GYRO OK: stationary bias stored in RAM; not a flight-frame validation.")),
                daemon=True).start()
        elif command == "STATUS":
            # Intentionally no response: priority STOP should interrupt the waiting HTTP request.
            pass
        else:
            # Other supported command forms use this immediate device-side test acknowledgement.
            self._line(f"ACCEPTED: {command}")
        return len(data)

    def _line(self, line: str):
        with self.owner.lock:
            self.owner.last_rx = time.monotonic()
            self.owner.lines.append(line)
            self.owner.line_counter += 1
            self.owner.lines = self.owner.lines[-120:]
            self.owner.events.append({"id": self.owner.line_counter, "line": line})
            self.owner.events = self.owner.events[-160:]

    def flush(self):
        pass

    def close(self):
        self.is_open = False


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.controller = bridge.SerialController()
        fake = FakeSerialPort(self.controller)
        self.controller.port = fake
        self.controller.running = True
        self.controller.last_telemetry_rx = time.monotonic()

    def tearDown(self):
        self.controller.disconnect()

    def test_read_only_diagnostic_preserves_blocked_status(self):
        result = self.controller.command("PREFLIGHT", "serial")
        self.assertTrue(result["ok"])
        self.assertFalse(result["acknowledged"])
        self.assertIn("PREFLIGHT BLOCKED", result["message"])

    def test_read_pid_response_is_delivered(self):
        result = self.controller.command("GETPID", "serial")
        self.assertTrue(result["ok"])
        self.assertTrue(result["acknowledged"])
        self.assertTrue(result["message"].startswith("PID,ROLL_RATE"))

    def test_gyro_calibration_has_a_longer_realistic_ack_window(self):
        result = self.controller.command("CAL_GYRO", "serial")
        self.assertTrue(result["ok"])
        self.assertTrue(result["acknowledged"])
        self.assertIn("CAL_GYRO OK", result["message"])

    def test_priority_stop_interrupts_pending_command(self):
        result_holder = {}
        worker = threading.Thread(target=lambda: result_holder.setdefault(
            "normal", self.controller.command("STATUS", "serial")), daemon=True)
        worker.start()
        time.sleep(0.10)
        stop = self.controller.command("S", "serial")
        worker.join(timeout=0.5)
        self.assertTrue(stop["ok"])
        self.assertTrue(stop["acknowledged"])
        self.assertFalse(worker.is_alive())
        self.assertIn("SUPERSEDED BY PRIORITY STOP", result_holder["normal"]["message"])

    def test_every_control_command_form_is_routed_by_the_bridge_allowlist(self):
        commands = (
            "PING", "ARM", "START", "S", "STOP", "DISARM", "A", "L", "W",
            "+", "-", "U", "J", "H", "K", "STATUS", "PREFLIGHT", "HELP",
            "GETPID", "SAVE_PID", "CAL_GYRO", "BARO_ZERO", "RESET_PID",
            "TUNE,ROLL_RATE,KP,0.035", "TUNE,PITCH_ANGLE,OUTMAX,35",
            "SETPOINT,ROLL_DEG,-12", "SETPOINT,PITCH_DEG,12",
            "SETPOINT,YAW_RATE_DPS,90", "SETPOINT,ROLL_TRIM_DEG,-4",
            "SETPOINT,PITCH_TRIM_DEG,4", "SETPOINT,YAW_TRIM_DPS,-15",
            "SETPOINT,THROTTLE,0.6", "SETPOINT,ALT_M,5", "SETPOINT,ALT_HOLD,1",
        )
        for command in commands:
            with self.subTest(command=command):
                result = self.controller.command(command, "serial")
                self.assertTrue(result["ok"])
                self.assertTrue(result["sent"])
                # Diagnostics can correctly report a blocked condition or no reply; the
                # bridge must still route them rather than treating that as transport success.
                if command in {"STATUS", "PREFLIGHT"}:
                    self.assertFalse(result["acknowledged"])
                else:
                    self.assertTrue(result["acknowledged"])

        rejected = self.controller.command("SETPOINT,THROTTLE,0.61", "serial")
        self.assertFalse(rejected["ok"])
        self.assertIn("allowlist", rejected["error"])

    def test_product_ui_has_three_main_pages_and_live_tuning_tools(self):
        html = (Path(bridge.__file__).resolve().parent / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn('data-page="system"', html)
        self.assertIn('data-page="missions"', html)
        self.assertIn('data-page="control"', html)
        self.assertNotIn('id="page-simulation"', html)
        self.assertNotIn('id="subpage-simulation"', html)
        self.assertIn('id="applyTrims"', html)
        self.assertIn('id="subpage-calibration"', html)
        self.assertIn('id="tunTelemetryAge"', html)
        self.assertIn('id="tunFailsafe"', html)
        self.assertIn('LINK LOSS · ATTITUDE HOLD', html)

    def test_server_factory_and_removed_simulation_api(self):
        server = bridge.make_server("127.0.0.1", 0)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        base = f"http://127.0.0.1:{server.server_address[1]}"
        try:
            with urllib.request.urlopen(base + "/", timeout=2) as response:
                self.assertEqual(response.status, 200)
                self.assertIn(b"Ground control", response.read())
            with urllib.request.urlopen(base + "/api/health", timeout=2) as response:
                self.assertTrue(json.loads(response.read())["ok"])
            with urllib.request.urlopen(base + "/firmware/flight_control/RiftwalkerConfig.h", timeout=2) as response:
                self.assertEqual(response.status, 200)
                self.assertIn(b"RIFT_ENABLE_ACTUATOR_OUTPUTS 0", response.read())
            with urllib.request.urlopen(base + "/firmware/espnow_ground_transmitter/ESP_NOW_Ground_Transmitter.ino", timeout=2) as response:
                self.assertEqual(response.status, 200)
                self.assertIn(b"esp_now_init", response.read())
            with self.assertRaises(urllib.error.HTTPError) as err:
                urllib.request.urlopen(base + "/api/export-case-package", timeout=2)
            self.assertEqual(err.exception.code, 404)
        finally:
            server.shutdown()
            server.server_close()


if __name__ == "__main__":
    unittest.main(verbosity=2)
