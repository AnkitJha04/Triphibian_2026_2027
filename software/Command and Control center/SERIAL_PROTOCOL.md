# RIFTWALKER serial command and telemetry protocol

## Link

- UART: 115200 baud, newline-delimited UTF-8/ASCII text.
- Command frame: one ASCII command followed by `\n`.
- Telemetry frame: `@TEL,` followed by one JSON object and newline. The app treats only received, fresh telemetry as current.
- Ordinary diagnostic/output lines are text, not telemetry. `PID,...` lines are parsed for displayed gain values.

## Supported command examples

- `PING` (liveness heartbeat), `STATUS`, `PREFLIGHT`, `HELP`, `GETPID`
- `TUNE,ROLL_RATE,KP,0.035`
- `SETPOINT,ROLL_TRIM_DEG,0.0`, `SETPOINT,PITCH_TRIM_DEG,0.0`, `SETPOINT,YAW_TRIM_DPS,0.0`
- `CAL_GYRO`, `BARO_ZERO`, `RESET_PID`, `SAVE_PID`
- `ARM`, followed by `START` only if the controller explicitly accepts the first stage; `S` / `STOP` / `DISARM`
- `A` / `L`; water mode is rejected because the real water propulsion geometry is not specified.

The bridge allowlists commands and range-checks numeric parameters. The firmware applies its own parsing, state and interlock checks; the app's validation is not a replacement for device-side validation.

## Command-loss behavior

While armed, the desktop bridge sends `PING` only while it has fresh UI activity and receives telemetry. If the controller misses heartbeats for 1.2 seconds but its IMU is still fresh and calibrated, it briefly preserves the existing, bounded attitude setpoints and configured trims for an 800 ms stabilization grace window. A returning `PING` resumes normal closed-loop control without changing those targets. If the link does not recover, or IMU health is lost at any point, the controller disarms.

This is a link-recovery cushion, not altitude hold, position hold, autonomous return-to-home, or an assured landing procedure. It must be tested with outputs disconnected and then in a restrained, technically reviewed setup before actuator gates are ever enabled.

## ACK meanings

- `@ACK,SENT,<seq>,...`: local ground bridge tried to transmit the packet; not application success.
- `@ACK,RX,<seq>,ACCEPTED`: matching rover sequence is reported as accepted by the controller task.
- `@ACK,RX,<seq>,REJECTED`: matching rover sequence was rejected.
- `@ACK,NOPAIR,...`, `@ACK,SENDFAIL,...`, `@ACK,TIMEOUT,...`: bridge-level failure / no correlated rover response.

## Important telemetry field meaning

- `esc_command_us`: controller's requested pulse values. With current compile-time gates locked, these are command state, not electrical PWM output.
- `esc_us`: null unless the firmware explicitly reports an enabled output gate; does not imply measured RPM.
- `servo_cmd_deg`: commanded servo targets, not measured servo position (no position feedback exists).
- `roll_deg`, `pitch_deg`: suppressed by the UI unless `attitude_valid=true`; this firmware keeps axes invalid until orientation is physically validated.
- `alt_m`: a relative MS5607 pressure estimate when the barometer and ground reference are valid. It is not terrain clearance or position.
- `battery_v`, current, RPM, water ADC, range, and horizontal velocity must remain null/dashes unless the exact sensor circuit has been validated and firmware reports it as valid.
- `telemetry_age_s` is calculated on the PC from the last received `@TEL` record. Stale data is withheld from controls.
