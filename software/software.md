# RiftWalker Software Repository

Off-the-shelf flight controllers (like Betaflight or ArduPilot) are strictly optimized for single-domain operation. To control a shape-shifting triphibian robot, the RiftWalker project bypasses pre-existing software entirely. This repository contains our custom C++ firmware, heavily utilizing FreeRTOS on the ESP32-S3 to achieve true, uninterrupted multi-domain mobility.

---

## Dual-Core RTOS Architecture

To guarantee absolute system stability, the firmware leverages the ESP32-S3’s dual-core processor. We strictly isolated environmental sensing operations from the high-speed motor control loops.

### Core 0: Sensor Fusion & Telemetry (`SensorTask`)
Core 0 is completely dedicated to reading environmental data and driving the OLED user interface.

* **Domain Detection:** The system continuously polls barometric pressure and ultrasonic distance. If the pressure drops below 1000 hPa and the distance exceeds 100 cm, the system autonomously registers its state as `AIR MODE`.
* **Dynamic Reconfiguration:** Upon detecting a terrain shift, Core 0 instantly commands four onboard servos to sweep 90 degrees. This action physically shape-shifts the robot from a driving/sailing configuration into a multirotor flight configuration.

### Core 1: Flight Control & Motor Timing (`MotorTask`)
Core 1 is isolated purely for high-speed mathematical corrections and Electronic Speed Controller (ESC) driving.

* **Forward-Only PID Stabilization:** When operating in Air Mode, this core runs a custom PID algorithm utilizing pitch and roll data from the IMU. To maintain aerodynamic stability without mechanical stuttering, the motor logic is clamped to strictly apply *forward* RPM corrections—the motors never reverse direction to compensate for tilt.
* **Differential Thrust Mapping:** In Land or Water mode, the standard PID loop is bypassed. Core 1 instead implements differential thrust mapping (push-pull mixing). This allows the system to steer the robot entirely by varying the left and right motor RPMs, eliminating the need for a mechanical steering rack.

---

## Concurrency & Safety (Mutex Locks)

Because Core 0 (Sensors) and Core 1 (Motors) operate asynchronously, data collisions could cause a catastrophic mid-air failure. To prevent this, the architecture implements **Hardware Mutex Locks** (`portENTER_CRITICAL` / `portEXIT_CRITICAL`). 

When Core 0 updates telemetry data (Pitch, Roll, and Altitude), it briefly locks the shared memory space. Core 1 reads this shared data securely and unlocks it within microseconds. This mechanism guarantees that any unexpected I2C sensor delay will *ever* interrupt critical motor timing pulses.

---

## Wokwi Simulation Environment

Before flashing the custom printed circuit board (PCB), the system logic was fully verified within a simulated environment using the following implementation strategies:

* **Actuator Simulation:** Because Wokwi does not natively support high-speed brushless DC (BLDC) ESCs, the flight motors are simulated using four A4988 stepper drivers. By calculating strict microsecond delays through a custom `pulse()` function, the simulation successfully mimics precise RPM control and thrust mixing.
* **UI Verification:** An SSD1306 OLED displays real-time system states (`SAILING`, `DRIVING`, `STABLE`), proving that dual-core memory sharing operates flawlessly under maximum processing load.

---

*For mechanical dimensions, PCB schematics, and component isolation strategies, please refer to [`HARDWARE.md`](HARDWARE.md).*
