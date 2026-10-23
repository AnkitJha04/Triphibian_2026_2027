# RIFTWALKER embedded firmware build & upload guide

This guide applies to the current hardware bring-up build. **Actuator outputs are compile-time locked. This is not flight-authorised firmware.** Build/upload only with propellers removed and the LiPo disconnected during flashing/wiring changes.

## Required toolchain

1. Install Arduino IDE 2.x.
2. Install the **Espressif ESP32 Arduino board package 3.x** through Boards Manager. The ESP-NOW receive callback uses the `esp_now_recv_info_t` callback signature used by Arduino-ESP32 3.x / current ESP-NOW APIs; do not select an older 2.x package without porting and reviewing the callback layer.
3. Install the **ESP32Servo** library from Arduino Library Manager.
4. Select the ESP32-S3 board entry matching your carrier/module. `ESP32S3 Dev Module` is a common starting selection for ESP32-S3-WROOM-1 designs, but flash size, PSRAM, USB mode and partition options must match the exact module/PCB build. Do **not** use `DOIT ESP32 DEVKIT V1` / `esp32:esp32:esp32doit-devkit-v1`: that targets the original ESP32 and the sketches now reject it at compile time. Don't guess a PSRAM/flash configuration from the module family name alone.

Reference documentation:
- Espressif Arduino-ESP32 core compatibility: https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/core_compatibility.html
- ESP-NOW Arduino API: https://github.com/espressif/arduino-esp32/blob/master/docs/en/api/espnow.rst

## The sketches are separate firmware images

Open/upload **one sketch at a time**. Keep each sketch's companion headers in the same sketch directory.

| Purpose | Sketch | Required board |
|---|---|---|
| Rover sensing, telemetry, RTOS and gated controller | `firmware/flight_control/Actual_Fly_Control.ino` | Rover ESP32-S3 |
| ESC calibration only | `firmware/esc_calibration/Callibration_Code.ino` | Rover ESP32-S3; never use as flight firmware |
| PC serial ↔ ESP-NOW bridge | `firmware/espnow_ground_transmitter/ESP_NOW_Ground_Transmitter.ino` | Second/ground ESP32-S3 |
| I²C address discovery | `firmware/i2c_scanner/I2C_Scanner.ino` | Rover ESP32-S3 |

## First upload sequence

1. Keep the propellers removed and LiPo unplugged.
2. Upload `I2C_Scanner.ino`; open Serial Monitor at 115200 baud. Look for the MPU6050 at `0x68` (fall back to `0x69` only when it responds) and MS5607 at `0x76` (fall back to `0x77`). Record actual addresses; an ACK is not proof of sensor calibration.
3. Upload `Actual_Fly_Control.ino`. In the serial boot output, verify the printed GPIO arrays agree with the fabricated PCB traces and connectors. Expect preflight to report `BLOCKED` and both output gates to report locked.
4. Run `CAL_GYRO` with the vehicle stationary and level, then `BARO_ZERO` while grounded. These are sensor reference operations only; body axes still require a physical tilt/rotation test.
5. Connect the desktop app to the rover's USB serial port. Check live telemetry ages, measured/estimated/unavailable statuses, `GETPID`, `PREFLIGHT`, and the event log.
6. For ESP-NOW, upload the ground sketch to a **second** ESP32-S3, configure reciprocal MAC addresses and the same channel in both sketches, then re-upload both boards. Connect the desktop app to the **ground bridge's COM port**, not the rover's port.

## Do not unlock outputs yet

`RIFT_ENABLE_ACTUATOR_OUTPUTS`, `RIFT_ENABLE_SERVO_OUTPUTS`, `RIFT_ENABLE_AUTO_TRANSITIONS`, and `RIFT_ENABLE_ALTITUDE_HOLD` are zero deliberately. The typed GPIO list and current KiCad connector/net labels need reconciliation; the sensor body-axis transform and thrust allocation have not been physically validated; water sensing on GPIO42 is unavailable; the battery ADC/divider configuration is unvalidated; and the HC-SR04 ECHO logic-high path must be measured. The firmware's compile-time validation flags intentionally prevent casual activation.

Do not edit those flags just to make the ARM button clickable. First complete `VALIDATION_CHECKLIST.md`, verify signals without props, bench-test each actuator independently, validate motor/servo signs and allocation with a restrained setup, and validate the bounded link-loss attitude-hold then disarm behavior in a restrained setup. It is not a safe auto-land in flight and cannot guarantee the vehicle will not crash.
