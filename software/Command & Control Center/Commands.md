# RIFTWALKER // Triphibious Command Deck

Source-locked command deck for the triphibious vehicle (air, land, water). Local simulation always works. Live ESP32 telemetry is shown only after the device answers the API contract below.

## Files

| Path | Role |
|---|---|
| `Index.html` | Canonical Command Deck. Open this in a browser, or let the desktop app load it. |
| `Application/index.html` | Exact copy of the canonical HTML, so the `Application` folder can run on its own. |
| `Application/app.py` | Native desktop window (PySide6 + Qt WebEngine) that hosts the same HTML. |
| `Application/requirements.txt` | Python packages for the desktop app. |
| `READMED.md` | Architecture notes, pin map, and hardware contract. |

## Run in a browser

Double-click `Index.html`, or from this folder:

```powershell
Start-Process ".\Index.html"
```

No CDN, build step, or internet connection is required for simulation.

Optional local server (useful if a browser blocks `file://` features):

```powershell
python -m http.server 8765
```

Then open `http://127.0.0.1:8765/Index.html.html`.

## Run the desktop app

Requires **Python 3.10+**.

```powershell
cd Application
python -m pip install -r requirements.txt
python app.py
```

Or from this folder:

```powershell
python -m pip install -r Application\requirements.txt
python Application\app.py
```

The window loads the same Command Deck HTML (System, Digital Twin, Hardware / PCB, Use Cases, Ground Control, Reference Gallery). Drag to orbit the model; mouse wheel zooms.

## What works without hardware

- Air / Land / Water modes and vector animation (120°/s)
- Vector changes **propeller direction only**; the model does not drop when you vector
- Environment pressure and Ground Control **UP / DOWN** move the vehicle up or down **inside the same view** (window size stays put)
- Throttle, vector, and environment-pressure controls
- Ground Control pad: UP, DOWN, FWD, BACK, LEFT, RIGHT, STOP
- Live local telemetry (mode, altitude, speed, pressure, battery, roll, pitch, current, power, per-servo angles)
- Interactive PCB map, mission cases, and the supplied reference gallery
- Ground-control pad, ESTOP, and command log in **local simulation** mode

## ESP32 hardware link

Ground Control uses:

- `GET /health`
- `GET /telemetry`
- `POST /command` with JSON

Until those endpoints succeed, the UI stays on **SIMULATION**. It never labels local variables as “ESP32 LIVE”.

Firmware must allow CORS if the page origin is not the ESP32 itself.

Example telemetry:

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

## Safety

This is a visualization and operator-deck prototype, not a certified flight controller. Test motors with propellers removed or restrained, and keep a physical emergency-disarm path.
