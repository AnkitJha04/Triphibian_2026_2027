# Release notes — RIFTWALKER hardware bring-up v1

## Included

- Removed the standalone Simulation Lab window and associated runtime module from the deliverable.
- Preserved the System view, illustrative Use Cases page, and Ground Control product interface.
- Added/retained connection mode selection for direct rover USB serial or serial-to-ESP-NOW ground bridge.
- Ground Control includes PID gain editing, roll/pitch/yaw-rate trim offsets, gyro calibration, barometer ground-reference command, ESC calibration overview, live sensor/RTOS telemetry and command/event logs.
- Stale telemetry is expired from the tuning dashboard; null/non-finite values display as unavailable instead of zero.
- Device application ACKs are separated from ground transport `SENT` messages; ESP-NOW bridge correlates acknowledgements to command sequence and has bounded same-sequence retries.
- Shared `RiftwalkerProtocol.h` files were kept identical; protocol includes CRC and packet version checks.
- Added runtime guards for ESC/servo attach success and RTOS task creation in addition to compile-time validation flags. Serial output from non-telemetry reports is queued through a low-priority FreeRTOS task; ESP-NOW transmit frames are serialized by a single TX task.
- Emergency STOP can bypass the bridge’s normal command transaction and cancels a waiting request; firmware latches STOP from the serial parser/radio receive path for processing by the controller task. It requests minimum output/disarm, not autonomous landing.
- Browser telemetry polling now uses a sequential 100 ms timeout loop so slow status requests cannot overlap; live values are expired after the stale threshold.

## Still locked / not claimed

- ESC/servo output gates are 0; hardware outputs are not attached by the flight-controller sketch.
- Air/land/water mechanics, body-axis transform, motor/servo net map, thrust mixing, range-sensor ECHO level, water sensing and battery sensing are not validated.
- No real motor current, RPM, servo position, water reading, battery voltage, horizontal velocity, magnetometer or position-hold telemetry is fabricated.
- PID numbers remain bench placeholders. No Arduino-core compile, board-level test, ESP-NOW RF test, ESC test or physical flight test is claimed in this release. Five offline bridge transaction tests and the MS5607 reference-vector test pass; these do not validate embedded firmware execution.

## Known hardware notes

- The typed ESC/servo GPIO table appears to differ from connector/net labels in the uploaded KiCad design; verify continuity on the fabricated PCB before connecting actuators.
- GPIO42 cannot perform analog input on the ESP32-S3.
- GPIO11 is ADC2 and its reading is not used during ESP-NOW operation; voltage divider calibration is not yet set.
- The reported HC-SR04 divider values must be checked, as they may not provide a valid ECHO high threshold.
- Magnetometer model and address are unknown and no driver is enabled.
