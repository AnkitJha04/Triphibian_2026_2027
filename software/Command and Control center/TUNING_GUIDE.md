# RIFTWALKER PID tuning and sensor bring-up

This guide covers the current safe bring-up firmware. The actuator gates are compile-time disabled, so the PID tab supports parameter read/write, trim offsets, gyro bias calibration and diagnostics without spinning ESCs or moving servos.

## Important distinction

- **Gyro bias calibration** estimates a stationary MPU6050 offset. It is not PID tuning and it does not validate the PCB-to-body axis transform.
- **PID gain editing** changes stored controller parameters. The included values are conservative development placeholders, not gains validated for the user's 2 kg vehicle.
- **Roll/pitch/yaw trims** change command/setpoint offsets. They do not correct sensor wiring, sensor orientation, motor order, or mechanical imbalance.
- **Relative altitude** needs a valid MS5607 reading and ground-pressure reference; it does not provide horizontal position hold.

## First bring-up with propellers removed

1. Keep the LiPo disconnected. Verify PCB output headers against `HARDWARE_PINOUT.md` and continuity-test ground, signal, SDA, SCL, and supply nets before adding ESCs/servos.
2. Upload `firmware/i2c_scanner/I2C_Scanner.ino` and open the serial monitor at 115200 baud. Confirm the MPU6050 ACK at `0x68` (or `0x69` if the AD0 address selection makes it so) and MS5607 ACK at `0x76`/`0x77`.
3. Upload `firmware/flight_control/Actual_Fly_Control.ino`. Close Serial Monitor before opening the RIFTWALKER app. Connect in direct USB serial mode.
4. Inspect the device-reported I²C addresses, IMU health, MS5607 PROM/pressure validity, control-loop duration and overruns. Run `PREFLIGHT` and read the reported reasons. The expected result is BLOCKED while the validation gates remain false.
5. Place the complete vehicle stationary and level. Run `CAL_GYRO` and wait for a successful stillness check. It estimates bias in RAM; reset/power-cycle removes that bias, so recalibrate after restart until persistent calibration is explicitly added and validated.
6. With the vehicle still on the ground and disarmed, use `BARO_ZERO` after the MS5607 pressure reading is stable. This sets a relative reference in RAM for that power session.
7. Select each gain axis and gain field, set the numeric value, and press APPLY GAIN. Use READ ALL GAINS to compare returned `PID,...` lines. SAVE TO ESP32 stores controller gains to NVS. Keep the actuator gates locked during this stage.
8. Trim values are bounded: roll ±4°, pitch ±4°, yaw-rate ±15°/s. Apply trims only while disarmed, then use READ/telemetry to verify the stored command offsets.
9. Inspect loop execution and overrun counts under idle sensor polling. Investigate rising overrun count, stale sensor ages, failed I²C reads, or invalid PROM CRC before considering any later hardware stage.

## Connecting over ESP-NOW

Use the second ESP32-S3 as the ground bridge. Follow `GROUND_BRIDGE_SETUP.md`; the desktop app's serial port must connect to that bridge. Verify the rover MAC, ground MAC, Wi-Fi channel, telemetry freshness, and correlated `@ACK,RX` replies. A `@ACK,SENT` record only means the bridge accepted the local serial request; it is not proof the rover processed the command.

## PID tuning philosophy

There is no reliable way to calculate a safe, “least tuning” flight PID from total mass and centre-of-mass location alone. Tuning also depends on thrust-to-weight ratio, propeller response, total inertia, frame flexibility, servo latency, sign convention, sensor filtering and motor/prop thrust curves. Do not use this firmware's placeholder values as flight gains.

Recommended future process, after the mechanical and electrical checks are complete: validate gyro/body axes on a fixed frame; validate motor/servo directions with props removed; measure actuator response; begin with rate-loop proportional response on a restrained test setup; increase cautiously while monitoring oscillation; then add derivative filtering and integral anti-windup; close attitude loops; validate the thrust allocation; only then evaluate altitude hold. An altitude loop must have hover-throttle feed-forward, adequate vertical control authority, and a carefully tested failsafe.

## Failsafe notes

The command timeout currently disarms to minimum output as a development interlock. **In an airborne vehicle, disarming motors can cause a crash. It is not an autonomous land/return mode.** A controlled descent requires a separately validated failsafe with reliable altitude sensing and enough control authority, which this build does not claim. Maintain a physical battery/motor cutoff and do not fly until that design is validated.
