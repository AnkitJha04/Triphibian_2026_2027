# RIFTWALKER / TRIPHIBIOUS COMMAND DECK — FINAL

Canonical interface file: **`Index.html`**

Desktop app: **`Application/app.py`** (loads that HTML). See the root `README.md` for run commands.

## What this build is based on
This build was rebuilt from the supplied project material rather than from the earlier generic dashboard alone:

- `Project Overview.md` — primary project architecture and operating principle.
- `Algorithm.png` — system initialization, health check, standby, sensor acquisition, fusion, control loop, mode decision, safety monitoring, failsafe and mission-completion flow.
- `Circuit Diagram Schematic Accurate.png` — custom flight-controller architecture and component/power/signal relationships.
- `Circuit Diagram.png` and `System Architecture.png` — system-level arrangement and multi-domain actuation/sensing relationships.
- `Major Project Simulation.zip` — Wokwi/ESP32 prototype firmware and wiring. This is treated as a prototype reference, not silently substituted for the final PCB.
- `Related Defence Projects.pdf` — supplied application/use-case material.
- Previous RiftWalker HTML files — used for visual/tactical presentation language only, not as the technical authority.

## Important source reconciliation
The project overview describes the final propulsion architecture as four A2212 BLDC motors, four ESCs and four 1045 propellers with four servo-driven motor pivots. The supplied Wokwi simulation is a bench prototype using an ESP32 DevKit, MPU6050, BMP180, SSD1306, HC-SR04, four A4988 drivers/stepper motors and four servos. The dashboard deliberately keeps these two layers separate so a prototype component is not presented as a final hardware component.

## Included functionality

### 1. Digital twin
- Four propulsion pods and four vector pivots.
- A2212 + 1045 propulsion representation.
- Four foam wheels for land contact and water buoyancy.
- Air / Land / Water state-dependent animation.
- Animated propeller motion tied to throttle.
- Vector angle moves at 120 degrees/second in the local model, matching the documented prototype design intent.
- Camera presets and mouse orbit/zoom on the flight view.

### 2. Telemetry
Local simulation is always functional and deterministic. It reports:
- mode
- throttle
- speed
- altitude/distance
- pressure
- battery
- roll
- pitch
- current
- power
- individual vector-servo state (front/rear mirror values follow the uploaded prototype's `180-angle` / `angle` mapping)

The UI does **not** call local simulation telemetry “hardware telemetry”.

Environment pressure scales thrust, speed, and current once per simulation step. It does not compound every animation frame.

### 3. Real ESP32 link
The Ground Control tab uses this explicit contract:

- `GET /health`
- `GET /telemetry`
- `POST /command` with JSON

Example telemetry response:

```json
{
  "ok": true,
  "mode": "air",
  "throttle": 42,
  "vector": 90,
  "altitude": 1.25,
  "pressure": 998.4,
  "speed": 18.2,
  "roll": -1.1,
  "pitch": 2.4,
  "battery": 91,
  "current": 18.6,
  "power": 275
}
```

Example command response:

```json
{
  "ok": true,
  "applied": true,
  "command": "SET_THROTTLE"
}
```

The interface only displays `ACK`/`APPLIED` after the device actually answers. Network failure is shown as `FAIL`.

### 4. Hardware / PCB
The interactive PCB map follows the supplied custom schematic's logical placement:
- ESP32-S3 central controller
- MPU6050 / magnetometer sensor area
- AP2112K 3.3 V regulation area
- ESC outputs
- vector-servo outputs
- propulsion/wheel blocks
- USB-C service
- battery ADC
- prototype sonar/water sensing

The exact supplied schematic is also available inside the Hardware tab.

### 5. Use cases
The mission library is custom to the supplied triphibious architecture:
- Flood Rescue / Access
- Coastal Survey
- Hazardous Area Inspection
- Industrial / Dam Inspection
- Disaster Recon
- Wetland / Environment

Each scenario has its own environment sequence, route, phase list and animation rather than a generic route reused for every case.

## Prototype pin map retained from the uploaded Wokwi code
The uploaded prototype uses:

| Function | GPIO |
|---|---:|
| FWD / BCK | 26 / 25 |
| LEFT / RIGHT | 17 / 15 |
| WATER | 16 |
| Throttle ADC | 34 |
| Ultrasonic | TRIG 5 / ECHO 35 |
| Vector servos | 18 / 19 / 23 / 4 |
| Step outputs | 27 / 14 / 13 / 12 |
| DIR | 32 / 33 |
| I2C SDA / SCL | 21 / 22 |

The firmware also uses FreeRTOS tasks for motor and sensor processing.

## Why the previous telemetry problem is not hidden
The uploaded Wokwi sketch does not implement a Wi-Fi HTTP telemetry server. Therefore a browser cannot truthfully fetch live hardware telemetry directly from that exact sketch. This dashboard therefore:

1. runs a complete local simulation without pretending it is hardware;
2. provides the hardware API adapter separately;
3. requires the ESP32 firmware to expose `/health`, `/telemetry` and `/command` before it will show real device ACK/data.

This is intentional: a green “ESP32 LIVE” indicator is never produced merely because the UI changed a local variable.

## Run
**Browser:** open `Index.html`.

**Desktop:** `python Application/app.py` after `python -m pip install -r Application/requirements.txt`.

No CDN or framework is required for the simulation interface.

For hardware mode, the client must reach the ESP32 IP and the firmware must implement the API contract above. CORS must be allowed when the page is not served from the ESP32 origin.

## Recommended hardware-side next step
Add the API adapter to the actual ESP32-S3 firmware. Do not retrofit the Wokwi stepper/A4988 test architecture into the final PCB merely to make the dashboard appear to work. The final project documentation identifies A2212/ESC/1045 propulsion and servo-based motor orientation; the Wokwi file is the prototype test bench.

## Safety
This interface is a visualization/control-deck prototype. It must not be used as a safety-certified flight controller. Real motor/servo testing should be performed with propellers removed or in an appropriate restrained test setup and with an independent physical emergency-disarm path.
