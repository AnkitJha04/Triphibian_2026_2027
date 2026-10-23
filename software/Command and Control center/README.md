# RIFTWALKER Command Deck — hardware bring-up release

This local-first desktop/web app is designed to inspect and communicate with the ESP32-S3 rover controller through **direct USB serial** or through a **USB-serial ESP-NOW ground bridge**. The Simulation Lab has been removed from this release. The System vehicle view and Use Cases animations are reference/illustrative visuals only and are not treated as measured telemetry.

## Release contents

- `Web/index.html` — responsive product UI with System, Use Cases, and Ground Control. Ground Control has Flight/Drive, PID Tuning, and ESC Calibration & Safety subtabs. It shows device measurements only when fresh and explicitly valid.
- `bridge.py` — local HTTP/serial bridge; it binds to `127.0.0.1`, parses `@TEL,` JSON, detects stale telemetry, allowlists commands, keeps stable event IDs for repeated device responses, and gives STOP priority over a waiting normal command.
- `app.py` — optional PySide6 desktop wrapper around the same local app.
- The companion firmware project is discovered from its standard sibling folder, or from `RIFT_FIRMWARE_ROOT` when it lives elsewhere. Its `Actual_Fly_Control`, `ESP_NOW_Ground_Transmitter`, `Callibration_Code`, and `I2C_Scanner` sketches provide the ESP32-S3 controller, ESP-NOW bridge, guarded ESC calibration, and I²C scanner.
- The paired `RiftwalkerProtocol.h` files in the flight-control and ground-transmitter sketches **must remain identical**; they define the CRC-protected command, telemetry, and text-acknowledgement frames.
- `HARDWARE_PINOUT.md`, `TUNING_GUIDE.md`, `GROUND_BRIDGE_SETUP.md`, `FIRMWARE_BUILD.md`, `SERIAL_PROTOCOL.md`, `VALIDATION_CHECKLIST.md` — wiring warnings, toolchain, bring-up and protocol guidance.

## Start the PC app

### Web UI
1. Install Python 3.10+.
2. Open a terminal in this folder.
3. Run `python -m pip install -r requirements-web.txt`.
4. Run `python bridge.py` (or `run_web.bat` on Windows).
5. Open `http://127.0.0.1:8765`.

### Desktop UI
1. Run `python -m pip install -r requirements.txt`.
2. Run `python app.py` (or `run_desktop.bat` on Windows).

Close Arduino Serial Monitor before connecting from the app; only one process can own a COM port at a time.

## Connection modes

- **Direct USB serial:** select the rover ESP32-S3 COM port. Firmware speaks newline-terminated commands at 115200 baud and emits JSON telemetry lines beginning with `@TEL,`.
- **ESP-NOW bridge:** select the *ground bridge* ESP32-S3 COM port. Configure reciprocal MAC addresses and the same fixed Wi-Fi channel in both `.ino` files before uploading. The PC does not send ESP-NOW directly; the second board translates serial commands to ESP-NOW packets and relays CRC-checked rover telemetry back as `@TEL,` JSON.

See `GROUND_BRIDGE_SETUP.md` for the pairing sequence.

## Current hardware configuration

- Vehicle: ESP32-S3-WROOM-1, MPU6050 at preferred address `0x68` with `0x69` fallback, MS5607 at `0x76` with `0x77` fallback, I²C SDA/SCL GPIO4/GPIO5.
- Requested ESC pins `{15, 6, 18, 9}`; requested servo pins `{8, 17, 7, 16}`.
- ESCs: SimonK 30 A, standard 50 Hz PWM, requested normal pulse range 1000–1800 µs (calibration sketch permits up to 2000 µs only after explicit serial request).
- Range sensor: HC-SR04 trigger GPIO12, echo GPIO13. ECHO level shifting must be electrically verified.
- GPIO42 is not an ADC input on ESP32-S3. The water sensor reading stays unavailable rather than reading the wrong pin. Battery sensing on GPIO11 uses ADC2 and the divider is uncalibrated; it remains unavailable in ESP-NOW mode.
- No magnetometer is installed/identified; heading/magnetometer data is unavailable.

## Deliberate safety restrictions

**This is a hardware bring-up and tuning release, not flight-authorised firmware.** In `RiftwalkerConfig.h`, `RIFT_ENABLE_ACTUATOR_OUTPUTS` and `RIFT_ENABLE_SERVO_OUTPUTS` are both `0`. Thus the ESC/servo output pins are not attached by the flight controller, and the UI blocks arming. Compile-time validation flags also remain false because the PCB header mapping, IMU body axes, motor/servo linkage geometry, thrust allocation, and failsafe behaviour are not yet physically verified.

The app will not manufacture values for current, battery voltage, water input, servo position, motor RPM, or horizontal velocity. A reported servo command angle is not feedback that the linkage reached that angle. PID gains in firmware are initial bench placeholders, not optimal 2 kg flight gains. Do not enable output gates based only on a successful compile or a balanced-looking centre of mass.

No software can guarantee that a vehicle never crashes. On a missed command heartbeat the firmware has only a short, IMU-health-gated attitude-stabilization grace period before it disarms; this must not be described as autonomous hovering or safe landing. Keep a physical power cutoff available and follow the validation checklist before any powered actuator tests.

## Tests performed on this package

Run `python -m py_compile bridge.py app.py` for Python syntax checks. The HTML's inline JavaScript can be checked with `node --check` after extracting the `<script>` content. The local API can be smoke-tested without an ESP32. No physical hardware, Arduino toolchain build, radio range, ESC response, or flight test is claimed by this package.
