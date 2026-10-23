# RIFTWALKER hardware configuration and unresolved checks

Configuration is taken from the user's latest typed pin list and the uploaded KiCad schematic/PCB. The board is fabricated but not assembled. **Firmware output gates remain disabled.** The typed output GPIO array and apparent net/header groupings in the uploaded schematic do not fully agree; resolve that before soldering/continuity verification.

## Typed GPIO plan

| Function | GPIO | Notes |
|---|---:|---|
| M1 ESC | 15 | Front-right; CCW (user-provided) |
| M2 ESC | 6 | Rear-right; CW |
| M3 ESC | 18 | Front-left; CW |
| M4 ESC | 9 | Rear-left; CCW |
| S1 | 8 | Front-right pod; 5° land → 100° air |
| S2 | 17 | Rear-right pod; 0° land → 90° air |
| S3 | 7 | Front-left pod; 0° land → 90° air |
| S4 | 16 | Rear-left pod; 5° land → 100° air |
| I²C SDA | 4 | MPU6050 + MS5607 shared bus |
| I²C SCL | 5 | MPU6050 + MS5607 shared bus |
| HC-SR04 TRIG | 12 | Disabled until hardware test |
| HC-SR04 ECHO | 13 | Verify divider/level shifter; currently marked unavailable |
| Analog water sensor | 42 | **Not ADC capable on ESP32-S3; no reading is taken** |
| Battery divider | 11 | ADC2; divider ratio uncalibrated and conflicts with Wi-Fi/ESP-NOW ADC usage; no reading taken |
| Future magnetometer DRDY | 14 | Reserved only; model is unknown/not installed |

## Components / declared mechanics

- 2 kg all-up vehicle mass, stated by user.
- 389.5 × 280 mm body envelope; 365 mm left-to-right and 242 mm front-to-back motor-centre rectangle.
- A2212 1400 KV BLDC motors, SimonK 30 A ESCs, 9050 propellers, DS3218 180° servos rated at 20 kg·cm stall torque per user's description.
- 4S 6200 mAh 35C battery. 14.8 V nominal, 16.8 V fully charged. Firmware doesn't report battery voltage because the divider has not been calibrated and current pin is incompatible with the intended live radio sampling.
- PCB mounted component-side down, USB-C facing the vehicle front, MPU6050 near centre.

## Sensors

- MPU6050 discovery checks `0x68` first, then `0x69` if the first address does not respond. A successful I²C ACK alone doesn't validate the body's axis signs or sensor orientation.
- MS5607 is probed at `0x76`, then `0x77`; PROM coefficients are checked using the device CRC before pressure is used.
- Relative barometric height only exists after a valid ground-pressure reference. It is not horizontal position and is not equivalent to a range sensor reading.
- HC-SR04 distance is intentionally suppressed until the ECHO logic-level circuit is verified. If R10=2 kΩ and R9=1 kΩ form the pictured divider, the resulting 1.67 V from a 5 V ECHO high may not meet the ESP32-S3 input-high threshold. Re-check actual nets/values.
- No magnetometer model has been selected. Do not assume every magnetometer has the same address or register map. Configure the part-specific driver when the exact part is known.
- Water sensor was previously stated to be analog on GPIO42, but GPIO42 is not an ESP32-S3 ADC pin. Firmware reports this as unavailable; it must not silently read a replacement pin without a board change.

## Output gates

`RIFT_ENABLE_ACTUATOR_OUTPUTS=0`, `RIFT_ENABLE_SERVO_OUTPUTS=0`, `RIFT_ENABLE_AUTO_TRANSITIONS=0`, and `RIFT_ENABLE_ALTITUDE_HOLD=0` are intentional defaults. Compile-time validation constants such as `BOARD_NET_MAP_VALIDATED`, `MOTOR_ORDER_VALIDATED`, `BODY_AXIS_MAPPING_VALIDATED`, `SERVO_GEOMETRY_VALIDATED`, `AIR_GEOMETRY_VALIDATED`, and `FAILSAFE_VALIDATED` also remain false.

Do not change these to true until the physical nets are reconciled, motor rotation and thrust axes are verified without propellers, sensor axes are confirmed, servo travel is measured at the linkage, the control allocator is validated, and the failsafe has a reviewed safe response.
