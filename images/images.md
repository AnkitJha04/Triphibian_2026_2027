# RiftWalker Visual Repository

Welcome to the RiftWalker Visual Repository. This document serves as a visual index and explanation for the core engineering diagrams driving our platform. It breaks down the high-level mechatronic architecture, the custom electrical routing, and the dual-core software logic required for seamless triphibious operation.

---

## 1. System Architecture
*Reference: `system_architecture.jpeg`*

This diagram illustrates the top-down flow of commands and the physical unification of our hardware.

* **The Command Flow:** Human operator telemetry is received by the **ESP32-S3 Onboard Control System**. This central brain handles mission logic, PID stabilization, and sensor fusion.
* **The Hardware Split:** The controller pushes data to two distinct branches:
  * **Sensor System:** The IMU (Pitch/Roll), Barometer (Altitude), and Ultrasonic sensors.
  * **Actuation System:** The 4× BLDC flight motors and the high-torque vectoring servos.
* **The Triphibious Convergence:** Unlike traditional diagrams that show separate motors for different terrains, our architecture diagram shows a single actuation system converging into **Air Mode** (multirotor), **Land Mode** (wheels + vector thrust), and **Water Mode** (buoyancy + vector thrust). It visually proves our absolute elimination of parasitic dead-weight.

---

## 2. Circuit Diagram 
*Reference: `circuit_diagram.jpeg`*

The circuit diagrams map out our core strategy for isolating high-current electrical noise from sensitive flight logic.

* **Dual Power Routing (Schematic):** The schematic visually separates the **5V Actuator Bus** (driving the servos) from the **3.3V Logic Bus** (regulated by the AP2112K). A large bulk capacitor (`C10`) is placed directly on the servo power terminal to act as a physical buffer against voltage drops.
* **Safe Logic Shifting:** The schematic details the exact 2KΩ / 1KΩ resistor network used to safely step down the 5V ultrasonic echo pulse before it reaches the 3.3V GPIO pins of the ESP32.
* **Physical Board Layout (3D Render):** The PCB layout visually demonstrates our interference protection strategies. The IMU and Barometer are placed strictly on the **bottom layer** to shield them from the ESP32’s heat. Concurrently, the top layer features a massive, visually distinct **Copper Keep-Out Zone** around the antenna to ensure maximum RF range.

---

## 3. Software Flowchart (State Machine)
*Reference: `flowchart.jpeg*

This flowchart maps the real-time execution of our custom FreeRTOS firmware and outlines how the robot autonomously decides its physical shape.

* **Initialization & Core Split:** The program flow starts by dividing tasks. Core 0 handles the asynchronous `SensorTask` loop, while Core 1 handles the high-speed `MotorTask` loop. Hardware Mutex Locks are visualized as bridges allowing safe data transfer between the two processing loops.
* **Domain Detection Logic:**
  * **Decision Node 1 (Water):** If the water sensor triggers `LOW` ➔ Vector 0° ➔ **Water Mode** (Differential Thrust).
  * **Decision Node 2 (Air):** If Pressure < 1000 hPa AND Distance > 100cm ➔ Vector 90° ➔ **Air Mode** (PID Stabilization).
  * **Default State:** If neither condition is met ➔ Vector 0° ➔ **Land Mode** (Differential Thrust).
* **Execution Loop:** The flowchart maps how Core 1 continuously clamps RPM values and calculates strict microsecond step-delays based on the active domain state, proving that environmental checks never bottleneck the motor response pulses.
