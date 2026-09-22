# Riftwalker Triphibious Command Deck (desktop)

Native Python window for the Command Deck. It hosts the exact same interface as `TRIPHIBIOUS_Command_Deck_FINAL_FIXED.html` (copied here as `index.html`).

## Requirements

- Windows 10/11
- Python 3.10 or newer
- Packages in `requirements.txt` (PySide6 + Qt WebEngine via PySide6-Addons)

## Install and run

```powershell
cd Application
python -m pip install -r requirements.txt
python app.py
```

From the project root:

```powershell
python -m pip install -r Application\requirements.txt
python Application\app.py
```

On Windows you can also double-click `run.bat` in this folder. If Python packages are missing, that script installs `requirements.txt` and starts the app.

If Qt WebEngine is missing:

```powershell
python -m pip install --upgrade PySide6 PySide6-Addons PySide6-Essentials
```

## What it loads

`app.py` serves and displays, in this order if present:

1. `../TRIPHIBIOUS_Command_Deck_FINAL_FIXED.html`
2. `TRIPHIBIOUS_Command_Deck_FINAL_FIXED.html` in this folder
3. `index.html` in this folder

`index.html` is kept identical to the canonical HTML so this folder remains runnable by itself.

## Features (same as the HTML deck)

- Dark Riftwalker command-deck UI
- SYSTEM, DIGITAL TWIN, HARDWARE / PCB, USE CASES, GROUND CONTROL, REFERENCE GALLERY
- Digital twin with chassis, battery bay, four motor pods, vector pivots, propellers, tyres, and foam floats
- Drag to orbit, wheel to zoom, ISO / TOP / FRONT / SIDE cameras
- Air / Land / Water modes; vector tilts propellers only
- Environment pressure and Ground Control UP / DOWN move the model inside the same canvas
- Throttle, vector, and environment pressure
- Local telemetry; ESP32 feed only after a real device ACK
- Local telemetry; ESP32 feed only after a real device ACK
- Scrollable reference gallery with the supplied project images

## Hardware adapter

Unchanged API:

- `GET /health`
- `GET /telemetry`
- `POST /command`

Default device URL in Ground Control: `http://192.168.4.1`

The desktop app remains fully usable in local simulation if no ESP32 is connected.
