# RiftWalker — Tactical Control Station

Single-file dashboard (`index.html`) for the triphibious VTOL **RiftWalker**: kinematics you can fly, interactive hardware, operational usages, a firmware lab you can edit and simulate, and a Wi‑Fi command page that tracks that firmware.

**Engineering:** Ankit Jha, Nidhi Kulkarni, Yadnyee Joshi, Dnyanam Gala

---

## Run

Open `index.html` in a modern browser, or:

```bash
python -m http.server 8765
```

Then visit `http://127.0.0.1:8765/`. Tailwind CDN and fonts need network on first load.

---

## Pilot controls (all pages)

| Input | Action |
| --- | --- |
| **↑ / W-style ArrowUp** | Forward (air: translate; land/water: aero-drive) |
| **↓** | Reverse / back |
| **← →** | Yaw (differential on surface) |
| **Camera** | **TOP / ISO / SIDE / FRONT / REAR** presets. Drag the canvas to orbit, **Q/E** to yaw the camera, wheel to zoom, **AUTO** to spin. X (red) / Y+ forward (green) / Z up (blue) gizmo. Servo 0° = disk in XY (lift); 90° = disk facing aft (aero-drive). |
| **Throttle slider** | 0–100% collective / ESC duty |

Arrows are ignored while a text field or the firmware editor is focused.

### Physics used in the twin

- Mass \(m = 2.2\,\mathrm{kg}\), max thrust \(T_\max = 4.72\,\mathrm{kgf}\), hover fraction \(T/W^{-1} = 2.2/4.72 = 0.466\) (**46.6%**).
- Instantaneous T/W \(= (\mathrm{thr}\cdot T_\max)/m\). Air current \(I \approx 2.2 + (31.2-2.2)\,(\mathrm{thr}/0.466)^{1.55}\,\mathrm{A}\). Land / water use quadratic load at 5.4 A and 14.9 A full throttle (design points).
- Caps: 58 / 78 / 10 km/h (air / land / water). DS3218 vector slews at **120°/s** (0° lift disk, 90° aero-drive).
- Muscle bus slider (7–12.6 V) rescales predicted current as \(I \cdot 11.1 / V_\mathrm{bus}\).

---

## Five views

1. **Kinematics** — Top-down canvas. Props are large silver disks with amber tips (no overlay covering them). Domain buttons, throttle, pad, live I / heading / T/W.
2. **Hardware** — Click cards for inspector text. Bus voltage and **manual vector** sliders couple into the same twin.
3. **Usages** — Defence perimeter, recon, SAR, border intercept, ecology, port security, wildfire, humanitarian, infrastructure, hazmat *standoff sensing*. Click map = waypoint; arrows drive the asset.
4. **Firmware** — Full editable FreeRTOS + HTTP C2 sketch. **COMPILE**, **SIMULATE TICK** / **RUN 2s** (injects MS5607 hPa and ToF mm into your `pressure < … && distance > …` law), **FLASH ESP32-S3** (stores image in `localStorage` for Command), **PARSE @CMD**.
5. **Command** — Palette is rebuilt from firmware `// @CMD …` lines (and `server.on("…")` as fallback). **USE TWIN** applies routes to this dashboard. **PING /api/telemetry** tries `http://<ip>:<port>` on a real ESP32 (firmware must send `Access-Control-Allow-Origin: *`). **ESTOP** zeroes throttle.

Add a new radio command by inserting a line in firmware:

```text
// @CMD id=MYCMD method=POST path=/api/foo args=pct label=My command
```

then PARSE or FLASH. The Command page updates without a rebuild.

---

## Hardware C2 (optional)

1. Copy the Firmware editor into Arduino-ESP32 / ESP-IDF, flash the S3.
2. Join AP `RiftWalker` (see sketch) or the board’s STA IP.
3. Set IPv4 on Command (default `192.168.4.1`), click PING.
4. Mixed-content: serve this dashboard over **http://** not https://, or the browser will block `http://192.168.4.1`.

If fetch fails (CORS, wrong LAN), the twin still executes the same command so you can debug UX offline.

---

## Original RTOS law (still in the sketch)

```c
if (pressure < 1000 && distance > 100) {
  activeDomain = DOMAIN_AIR;
  vectorServos(0); // VTOL Lift
} else {
  activeDomain = DOMAIN_SURFACE;
  vectorServos(90); // Aero-Drive
}
```

Change the numbers in the editor and SIMULATE — the kinematics domain follows the parsed thresholds.
