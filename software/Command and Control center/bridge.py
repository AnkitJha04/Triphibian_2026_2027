from __future__ import annotations

import json
import math
import mimetypes
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, unquote

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # The UI can launch without serial support for offline inspection.
    serial = None
    list_ports = None

ROOT = Path(__file__).resolve().parent
# Preserve the on-disk casing so the web UI also launches on case-sensitive hosts.
WEB_DIR = ROOT / "Web"


def _firmware_root() -> Path | None:
    """Find the companion firmware checkout without exposing arbitrary files."""
    configured = os.environ.get("RIFT_FIRMWARE_ROOT")
    candidates = [
        Path(configured) if configured else None,
        ROOT / "firmware",
        # Standard sibling layout used by this project collection.
        ROOT.parents[2] / "Microcontroller codes" / "Arduino and Esp" / "Cpp" / "Major Project",
    ]
    for candidate in candidates:
        if candidate and (candidate / "Actual_Fly_Control" / "Actual_Fly_Control.ino").is_file():
            return candidate.resolve()
    return None


FIRMWARE_ROOT = _firmware_root()
FIRMWARE_ENDPOINTS = {
    "flight_control": "Actual_Fly_Control",
    "espnow_ground_transmitter": "ESP_NOW_Ground_Transmitter",
    "calibration": "Callibration_Code",
    "i2c_scanner": "I2C_Scanner",
}


class SerialController:
    def __init__(self) -> None:
        self.port = None
        self.reader: threading.Thread | None = None
        self.running = False
        self.lock = threading.RLock()
        self.write_lock = threading.Lock()
        self.lines: list[str] = []
        # Keep stable event IDs so repeated identical responses (e.g. GETPID) are not hidden by the UI.
        self.events: list[dict] = []
        self.line_counter = 0
        self.command_lock = threading.Lock()
        self.interrupt_epoch = 0
        self.telemetry: dict | None = None
        self.last_rx = 0.0
        self.last_telemetry_rx = 0.0
        self.last_ping = 0.0
        self.last_ui_heartbeat = 0.0
        self.last_error: str | None = None

    def ports(self) -> list[dict]:
        if list_ports is None:
            return []
        return [{"device": p.device, "description": p.description or "Serial device"}
                for p in list_ports.comports()]

    def connect(self, device: str, baud: int = 115200) -> tuple[bool, str]:
        if serial is None:
            return False, "pyserial is not installed. Run: python -m pip install -r requirements.txt"
        if not device:
            return False, "Select a serial port."
        self.disconnect()
        try:
            port = serial.Serial(device, baudrate=baud, timeout=0.15, write_timeout=0.5)
            time.sleep(1.8)  # ESP32 boards often reset when the serial port opens.
            with self.lock:
                self.port = port
                self.lines = []
                self.events = []
                self.line_counter = 0
                self.telemetry = None
                self.last_telemetry_rx = 0.0
                self.last_error = None
                self.last_rx = time.monotonic()
                self.last_ping = 0.0
                self.last_ui_heartbeat = time.monotonic()
                self.running = True
            self.reader = threading.Thread(target=self._read_loop, daemon=True, name="serial-reader")
            self.reader.start()
            return True, f"Connected to {device} at {baud} baud. Waiting for telemetry."
        except Exception as exc:
            with self.lock:
                self.port = None
                self.running = False
                self.last_error = str(exc)
            return False, str(exc)

    def disconnect(self) -> None:
        with self.lock:
            self.running = False
            port = self.port
            self.port = None
        if port is not None:
            try:
                port.close()
            except Exception:
                pass

    def _read_loop(self) -> None:
        while True:
            with self.lock:
                running, port = self.running, self.port
                telemetry = dict(self.telemetry or {})
            if not running or port is None:
                return
            try:
                raw = port.readline()
                now = time.monotonic()
                if raw:
                    line = raw.decode("utf-8", errors="replace").strip()
                    if line:
                        with self.lock:
                            self.last_rx = now
                            if line.startswith("@TEL,"):
                                try:
                                    parsed = json.loads(line[5:])
                                    if isinstance(parsed, dict):
                                        self.telemetry = parsed
                                        self.last_telemetry_rx = now
                                    else:
                                        message = "Malformed telemetry: expected JSON object"
                                        self.lines.append(message)
                                        self.line_counter += 1
                                        self.events.append({"id": self.line_counter, "line": message})
                                        self.lines = self.lines[-120:]
                                        self.events = self.events[-160:]
                                except json.JSONDecodeError:
                                    message = "Malformed telemetry line received"
                                    self.lines.append(message)
                                    self.line_counter += 1
                                    self.events.append({"id": self.line_counter, "line": message})
                                    self.lines = self.lines[-120:]
                                    self.events = self.events[-160:]
                            else:
                                self.lines.append(line)
                                self.line_counter += 1
                                self.lines = self.lines[-120:]
                                self.events.append({"id": self.line_counter, "line": line})
                                self.events = self.events[-160:]
                # The firmware fails safe if the UI/bridge heartbeat disappears.
                if telemetry.get("armed") and now - self.last_ping >= 0.5 and now - self.last_ui_heartbeat <= 1.2:
                    with self.write_lock:
                        with self.lock:
                            current = self.port if self.running else None
                        if current is not None and current.is_open:
                            current.write(b"PING\n")
                    self.last_ping = now
            except Exception as exc:
                with self.lock:
                    self.last_error = str(exc)
                    self.running = False
                return

    def touch_ui(self) -> None:
        with self.lock:
            self.last_ui_heartbeat = time.monotonic()

    def status(self) -> dict:
        with self.lock:
            connected = bool(self.running and self.port is not None and getattr(self.port, "is_open", False))
            now = time.monotonic()
            return {
                "connected": connected,
                "port": self.port.port if connected else None,
                "telemetry": dict(self.telemetry) if self.telemetry else None,
                "lines": list(self.lines[-40:]),
                "events": list(self.events[-40:]),
                "last_rx_age_s": round(now - self.last_rx, 3) if self.last_rx else None,
                "telemetry_age_s": round(now - self.last_telemetry_rx, 3) if self.last_telemetry_rx else None,
                "error": self.last_error,
            }

    def command(self, command: str, transport: str = "serial") -> dict:
        command = command.strip().upper()
        is_stop = command in {"S", "STOP", "DISARM"}
        if is_stop:
            # STOP intentionally bypasses the normal transaction lock. Cancel the older
            # request's waiting phase, then write STOP as soon as the UART write lock allows.
            with self.lock:
                self.interrupt_epoch += 1
            return self._command_locked(command, transport, epoch=None, priority_stop=True)
        # Normal command/ACK transactions are serialized to avoid ACK misattribution.
        with self.command_lock:
            with self.lock:
                epoch = self.interrupt_epoch
            return self._command_locked(command, transport, epoch=epoch, priority_stop=False)

    def _command_locked(self, command: str, transport: str = "serial", epoch: int | None = None,
                        priority_stop: bool = False) -> dict:
        command = command.strip().upper()
        transport = transport.strip().lower()
        if transport not in {"serial", "espnow"}:
            return {"ok": False, "error": "Unknown transport profile."}
        allowed = {"ARM", "START", "S", "STOP", "DISARM", "L", "A", "W", "+", "-", "U", "J", "H", "K", "STATUS", "PREFLIGHT", "HELP", "GETPID", "SAVE_PID", "CAL_GYRO", "BARO_ZERO", "RESET_PID", "PING"}
        if command not in allowed:
            parts = command.split(",")
            safe = False
            try:
                if len(parts) == 4 and parts[0] == "TUNE":
                    gain = float(parts[3])
                    safe = (parts[1] in {"ROLL_RATE", "PITCH_RATE", "YAW_RATE", "ROLL_ANGLE", "PITCH_ANGLE", "ALTITUDE"}
                            and parts[2] in {"KP", "KI", "KD", "IMAX", "OUTMAX"}
                            and math.isfinite(gain) and 0 <= gain <= 100)
                elif len(parts) == 3 and parts[0] == "SETPOINT":
                    bounds = {"ROLL_DEG": (-12, 12), "PITCH_DEG": (-12, 12), "YAW_RATE_DPS": (-90, 90),
                              "ROLL_TRIM_DEG": (-4, 4), "PITCH_TRIM_DEG": (-4, 4), "YAW_TRIM_DPS": (-15, 15),
                              "THROTTLE": (0, 0.6), "ALT_M": (0, 5), "ALT_HOLD": (0, 1)}
                    key, val = parts[1], float(parts[2])
                    safe = key in bounds and math.isfinite(val) and bounds[key][0] <= val <= bounds[key][1]
            except (ValueError, OverflowError):
                safe = False
            if not safe:
                return {"ok": False, "error": "Command rejected by bridge allowlist or numeric limits."}
        with self.lock:
            port = self.port
            connected = self.running and port is not None and getattr(port, "is_open", False)
            before = self.line_counter
        if not connected:
            return {"ok": False, "error": "No serial controller/ground bridge is connected."}
        try:
            # If a priority STOP arrived while this non-stop request waited to write, discard
            # it before transmission rather than racing a command behind the STOP.
            if not priority_stop:
                with self.lock:
                    if epoch != self.interrupt_epoch:
                        return {"ok": True, "sent": False, "acknowledged": False, "transport": transport,
                                "message": "SUPERSEDED BY PRIORITY STOP; command not transmitted."}
            with self.write_lock:
                with self.lock:
                    if not self.running or self.port is not port or not port.is_open:
                        return {"ok": False, "error": "Serial link closed before command write."}
                port.write((command + "\n").encode("ascii"))
                port.flush()
            # Direct rover serial responds with text; the bridge first emits @ACK,SENT,<seq>,
            # then a packet-correlated @ACK,RX,<seq>,ACCEPTED|REJECTED. Do not treat SENT as ACK.
            # Gyro calibration intentionally samples for ~1 s; leave time for its explicit
            # result and (over ESP-NOW) the next 10 Hz telemetry ACK.
            if command == "CAL_GYRO":
                wait_s = 2.2 if transport == "espnow" else 1.7
            else:
                wait_s = 1.35 if transport == "espnow" else 0.85
            deadline = time.monotonic() + wait_s
            cursor = before
            sent_sequence = None
            response = None
            accepted = False
            negative = False
            collected: list[str] = []
            while time.monotonic() < deadline:
                if not priority_stop:
                    with self.lock:
                        if epoch != self.interrupt_epoch:
                            return {"ok": True, "sent": True, "acknowledged": False, "transport": transport,
                                    "sequence": sent_sequence, "message": "SUPERSEDED BY PRIORITY STOP; waiting stopped.",
                                    "responses": collected[-8:]}
                with self.lock:
                    offset = max(0, self.line_counter - len(self.lines))
                    start_index = max(0, cursor - offset)
                    new_lines = list(self.lines[start_index:])
                    current_counter = self.line_counter
                for line in new_lines:
                    collected.append(line)
                    if line.startswith("@ACK,SENT,"):
                        fields = line.split(",", 3)
                        if len(fields) >= 3:
                            try: sent_sequence = int(fields[2])
                            except ValueError: sent_sequence = None
                        continue
                    if line.startswith("@ACK,RX,"):
                        fields = line.split(",", 3)
                        if len(fields) >= 4 and sent_sequence is not None:
                            try: seq = int(fields[2])
                            except ValueError: seq = -1
                            if seq == sent_sequence:
                                response = line
                                accepted = fields[3].strip().upper() == "ACCEPTED"
                                negative = not accepted
                                break
                        continue
                    if line.startswith("@ACK,NOPAIR,") or line.startswith("@ACK,SENDFAIL,") or line.startswith("@ACK,REJECTED,") or line.startswith("@ACK,TIMEOUT,"):
                        response = line
                        negative = True
                        break
                    # Any ordinary line is an application-level response on direct serial.
                    if transport == "serial" and response is None:
                        response = line
                        upper = line.upper()
                        negative = any(token in upper for token in ("BLOCKED", "REJECTED", "ERROR", "UNKNOWN COMMAND", "NOT VALIDATED"))
                        accepted = not negative and any(token in upper for token in ("ACCEPTED", "ARMED", "DISARMED", "STATUS:", "PREFLIGHT", "PID,", "SAVED", "CAL_GYRO OK", "CALIBRATION", "RESET", "MODE STORED", "BARO_ZERO"))
                        break
                    # In bridge mode, forwarded application response lines arrive as plain text.
                    # Keep collecting until the matching binary telemetry acknowledgement appears.
                    if transport == "espnow" and response is None and not line.startswith("@ACK,"):
                        response = line
                        upper = line.upper()
                        negative = any(token in upper for token in ("BLOCKED", "REJECTED", "ERROR", "UNKNOWN COMMAND", "NOT VALIDATED"))
                if response and transport == "serial":
                    break
                # For ESP-NOW, continue until the exact application ACK is correlated.
                if response and transport == "espnow" and isinstance(response, str) and response.startswith("@ACK,RX,"):
                    break
                cursor = current_counter
                time.sleep(0.012)
            # Prefer the exact correlated ACK when available; otherwise report the best response
            # honestly without upgrading a transport send into an application acknowledgement.
            if transport == "espnow" and sent_sequence is not None and response and response.startswith("@ACK,RX,"):
                message = response
            elif response:
                message = response
            elif sent_sequence is not None:
                message = f"ESP-NOW packet sequence {sent_sequence} sent; rover acknowledgement not received before timeout."
            else:
                message = "Command bytes sent; no application response received before timeout."
            acknowledged = accepted and not negative
            return {"ok": True, "sent": True, "acknowledged": acknowledged, "transport": transport,
                    "sequence": sent_sequence, "message": message, "responses": collected[-8:]}
        except Exception as exc:
            return {"ok": False, "error": str(exc)}


controller = SerialController()


class Handler(BaseHTTPRequestHandler):
    server_version = "RiftwalkerBridge/1.0"

    def log_message(self, fmt, *args):
        return

    def _json(self, status: int, obj: dict):
        payload = json.dumps(obj, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def _body(self) -> dict:
        try:
            size = min(int(self.headers.get("Content-Length", "0")), 16384)
            return json.loads(self.rfile.read(size).decode("utf-8")) if size else {}
        except Exception:
            return {}

    def do_GET(self):
        path = unquote(urlparse(self.path).path)
        if path == "/api/ports":
            return self._json(200, {"ok": True, "ports": controller.ports(),
                                    "serial_available": serial is not None})
        if path in ("/api/status", "/api/health"):
            st = controller.status()
            return self._json(200, {"ok": True, **st})
        if path == "/":
            path = "/index.html"
        if path.startswith("/firmware/"):
            parts = Path(path).parts
            endpoint = parts[2] if len(parts) > 2 else ""
            relative = Path(*parts[3:]) if len(parts) > 3 else Path()
            source_dir = FIRMWARE_ENDPOINTS.get(endpoint)
            if FIRMWARE_ROOT is None or source_dir is None:
                return self._json(404, {"ok": False, "error": "Firmware source is unavailable."})
            allowed_root = (FIRMWARE_ROOT / source_dir).resolve()
            target = (allowed_root / relative).resolve()
        else:
            allowed_root = WEB_DIR
            target = (WEB_DIR / path.lstrip("/")).resolve()
        if allowed_root not in target.parents or not target.is_file():
            return self._json(404, {"ok": False, "error": "File not found."})
        content = target.read_bytes()
        content_type = mimetypes.guess_type(target.name)[0] or "application/octet-stream"
        if target.suffix == ".ino":
            content_type = "text/plain; charset=utf-8"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def do_POST(self):
        path = urlparse(self.path).path
        body = self._body()
        if path == "/api/heartbeat":
            controller.touch_ui()
            return self._json(200, {"ok": True})
        if path == "/api/connect":
            ok, message = controller.connect(str(body.get("port", "")), int(body.get("baud", 115200)))
            return self._json(200 if ok else 400, {"ok": ok, "message": message, "error": None if ok else message})
        if path == "/api/disconnect":
            controller.disconnect()
            return self._json(200, {"ok": True, "message": "Serial port disconnected."})
        if path == "/api/command":
            result = controller.command(str(body.get("command", "")), str(body.get("transport", "serial")))
            return self._json(200 if result.get("ok") else 400, result)
        return self._json(404, {"ok": False, "error": "Unknown API endpoint."})


class RiftwalkerHTTPServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def make_server(host: str = "127.0.0.1", port: int = 8765) -> RiftwalkerHTTPServer:
    """Create the shared local-only server used by web and desktop launchers."""
    return RiftwalkerHTTPServer((host, port), Handler)


def main() -> int:
    server = make_server("127.0.0.1", 8765)
    print("Riftwalker web app: http://127.0.0.1:8765")
    print("Offline interface is available without hardware; live control requires a connected ESP32.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        controller.disconnect()
        server.shutdown()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
