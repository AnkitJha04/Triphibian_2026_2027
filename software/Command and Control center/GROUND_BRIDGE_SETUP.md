# ESP-NOW ground bridge setup

The desktop app speaks ordinary newline-terminated commands over USB serial. The ground ESP32-S3 wraps each allowed command in a sequence-numbered CRC-checked binary packet and transmits it to the rover with ESP-NOW. The rover returns a CRC-checked binary telemetry packet; the ground board converts it to `@TEL,{...}` JSON for the app and forwards text status lines. Application acknowledgements are reported as `@ACK,RX,<sequence>,ACCEPTED|REJECTED`.

## Pair the boards

1. Upload `firmware/flight_control/Actual_Fly_Control.ino` to the rover with the output gates still disabled.
2. Open rover USB serial at 115200 baud and record the line `ROVER STA MAC: xx:xx:xx:xx:xx:xx | ESP-NOW channel 1`.
3. Upload `firmware/espnow_ground_transmitter/ESP_NOW_Ground_Transmitter.ino` to the second ESP32-S3.
4. Open ground USB serial at 115200 baud and record `RIFTWALKER GROUND BRIDGE MAC: xx:xx:xx:xx:xx:xx | channel 1`.
5. In `Actual_Fly_Control.ino`, set `ESPNOW_GROUND_MAC` to the ground board's six MAC octets.
6. In `ESP_NOW_Ground_Transmitter.ino`, set `ROVER_MAC` to the rover board's six MAC octets.
7. Re-upload both sketches. Both use channel 1 by default; they must be on the same channel. MAC addresses are not inferred or auto-paired.
8. Close both serial monitors. Start RIFTWALKER, select **ESP-NOW bridge**, select the ground board's COM port and connect. Wait for fresh `@TEL` packets.
9. Send `STATUS`, `PREFLIGHT`, and `GETPID`. Check the event log for the correlated application acknowledgement, not just `@ACK,SENT`.

## Command/telemetry mechanics

- Rover radio receive callback validates packet length, source MAC, CRC, protocol version and monotonically advancing sequence; it enqueues the command without doing the control operation in the Wi-Fi callback.
- The rover command task processes the command and publishes its result via telemetry and an optional CRC-checked text packet.
- The ground bridge allows one outstanding command transaction at a time, retries the same sequence a bounded number of times, and waits for the rover's application status. The rover's replay filter prevents repeated packets from executing the same command twice.
- `@ACK,SENT` means only local transmission was requested; `@ACK,RX` means the rover's application result was reflected in telemetry. Lack of `@ACK,RX` is a communication warning/timeout, not success.
- Direct serial uses the same text commands but does not use ESP-NOW framing.

## Diagnostics

- No `ROVER STA MAC`: check firmware boot output / selected USB serial port.
- `UNPAIRED SAFE STATE`: one of the reciprocal MAC addresses was not configured; no radio command path is paired.
- `NOPAIR`: check `ROVER_MAC`, `ESPNOW_GROUND_MAC`, ESP-NOW init and equal Wi-Fi channel.
- Telemetry stale: check radio peers and serial port, close other programs that use the port, and check for queue/radio errors.
- `@ACK,SENT` with no `@ACK,RX`: the rover may not have received/processsed the command, telemetry may be lost, or the command may have been rejected before an app acknowledgement. Check rover diagnostics and do not assume success.

This link is not safety-certified and is not a substitute for a physical kill switch. Both output gates remain disabled in this release.
