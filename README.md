````markdown
# BE Capstone Project

## Project Title

RiftWalker: The Triphibious Bot

---

## Team Details

| Sr. No. | Name of Student | Roll No. | Branch                   | Email ID                      |
|---------|-----------------|----------|--------------------------|-------------------------------|
| 1       | Ankit Jha       | 41       | Automation and Robotics  | 2023.ankitkumar.jha@ves.ac.in |
| 1       | Nidhi Kulkarni  | 15       | Automation and Robotics  | 2023.nidhi.kulkarni@ves.ac.in |
| 1       | Yadnyee Joshi   | 10       | Automation and Robotics  | 2023.yadnyee.joshi@ves.ac.in  |
| 1       | Dnyanam Gala    | 36       | Automation and Robotics  | 2023.dnyanam.gala@ves.ac.in   |

---

## Guide Details

**Project Guide: Mrs. Sangeeta Prasanna Ram**  
**Department:** Automation and Robotics  
**Institute:** VESIT, Mumbai  

---

## Problem Statement

The objective of this project is to create an innovative, small unmanned robotic system that can function in air, land, and water domains through a unified propulsion and control mechanism and thus eliminates the requirement of using different systems such as UAV, UGV, and surface water vehicles in multi-environment missions.

Traditional unmanned systems are usually designed to work in one environment only. While the UAV offers high-speed flying abilities, it lacks endurance; the land robot offers efficiency but is limited by terrain; and the surface vehicle is incapable of crossing land barriers.

Recent studies have shown that integration of these two technologies is possible; however, triphibious robots are far fewer in number compared to dual-environment robots since integration leads to more weight and complexity in design and control, in addition to increased energy consumption.

---

## Abstract

Unmanned robotic systems are being increasingly employed for surveillance purposes, inspection tasks, emergency rescue operations and monitoring of the environment. But since most of these platforms are designed for one environment only, which could be air, land or water, missions in complex environments would either necessitate the use of multiple vehicles or halt the mission as the platform encounters the environment it cannot negotiate. It has been shown through studies on multimodal robots that multimodality is useful in increasing accessibility but it brings with it some difficulties in terms of weight, efficiency, mechanics and control.

The Riftwalker is a prototype of a triphibious robotic platform that aims to study movement in air, land and surface water using the same vehicle architecture. The platform uses four brushless motors as the source of propulsion in aerial environment while servo motors are used to orient the motors for different modes of operation. Passive wheels help the robot move on the ground while elements providing buoyancy help the robot operate in surface water environment.

The current design target has an approximately 2.2 kg maximum take-off mass, with calculated/design-stage performance targets of approximately 58 km/h aerial speed, 78 km/h peak ground speed and 10 km/h surface-water speed. These values will be treated as design targets until experimentally validated.

The expected outcome is a functional prototype demonstrating controlled movement and transition between three operating domains for applications including search and rescue, environmental surveying, inspection and remote reconnaissance.

---

## Objectives

1. To study existing **multimodal and triphibious robotic platforms** and identify the challenges associated with integrating aerial, terrestrial and surface-water mobility into a single robotic system.

2. To develop a **3D CAD model** of the RiftWalker incorporating the structural frame, BLDC motor assemblies, servo-based tilting mechanism, wheels, buoyancy elements and electronics enclosure.

3. To design a **servo-based thrust vectoring mechanism** that changes the orientation of the BLDC propulsion units to enable different modes of locomotion using a unified propulsion system.

4. To perform **engineering calculations and feasibility analysis** including component-wise weight estimation, thrust-to-weight ratio, servo torque, battery and power requirements, land mobility, and water buoyancy.

5. To develop a custom **ESP32-S3 based flight controller PCB** integrating power management, motor and servo interfaces, USB-C programming, battery voltage monitoring and required sensor interfaces.

6. To integrate onboard sensors including the **MPU6050 IMU, magnetometer, barometer, ultrasonic sensor and water sensor** for orientation, altitude, obstacle and environmental-state monitoring.

7. To validate the proposed mechanical and electronic design through **simulation, PCB ERC/DRC verification and prototype testing**, ensuring that the system meets the defined design requirements.

8. To fabricate and integrate the mechanical, electronic and control subsystems and evaluate the **air, land and surface-water operating modes** of the RiftWalker prototype.

9. To document the complete design process, calculations, implementation, testing and results, and identify **future improvements and applications** of the proposed multi-terrain robotic platform.


---

## Scope of the Project

This project will cover the following key areas:

1) **3D mechanical design** and development of the servo-based thrust vectoring mechanism.
2) Selection and integration of **BLDC motors, ESCs, servos, wheels and buoyancy elements**.
3) Design of a custom **ESP32-S3 based flight controller PCB** with sensor, motor and servo interfaces.
4) Integration of sensors including **MPU6050, magnetometer, barometer, ultrasonic and water sensors**.
5) **Engineering calculations and simulation** for weight, thrust, torque, power and buoyancy.
6) Prototype fabrication, hardware integration and **testing of air, land and water operation**.
7) Performance evaluation and documentation of the developed system.

---

## Existing System

Existing unmanned robotic systems are generally designed for operation in a specific environment. They can broadly be classified as follows:

| Platform | Strength | Main Limitation |
|---|---|---|
| **UAV**               | Fast aerial movement and access to difficult locations | High energy consumption and limited endurance |
| **UGV**               | Efficient and stable terrestrial operation             | Restricted by water, steep terrain and other obstacles |
| **USV**               | Efficient operation on surface water                   | Limited terrestrial mobility |
| **Dual-Mode Robot**   | Capable of operating in two environments               | Cannot independently access the third environment |
| **Triphibious Robot** | Potential to operate in air, land and water            | Higher mass, mechanical complexity and control requirements |

Research has demonstrated the feasibility of combining multiple modes of locomotion within a single robotic platform. Existing triphibious and multimodal systems have explored approaches such as shared propulsion systems, tilting or morphing mechanisms and integrated mobility structures.

However, combining three different modes of locomotion introduces several engineering challenges, particularly in terms of **weight, energy efficiency, mechanical complexity, controllability and endurance**.

# Limitations of Existing Systems

1. **Multiple System Requirement:** Using separate UAV, UGV and waterborne platforms increases equipment, transportation and operational complexity.

2. **Limited Endurance:** Aerial systems require continuous thrust to remain airborne, resulting in significant power consumption.

3. **Terrain Limitations:** Ground robots may be restricted by water or difficult terrain, while waterborne platforms cannot easily overcome land-based obstacles.

4. **Increased Mechanical Complexity:** Combining separate propulsion and mobility mechanisms for different environments can increase the overall weight, power requirements and mechanical complexity of the system.

These limitations motivate the development of **RiftWalker**, which investigates the feasibility of combining aerial, terrestrial and surface-water mobility within a single robotic platform while sharing major propulsion, power and control components.

---

## Proposed System

RiftWalker proposes one reconfigurable vehicle rather than three independent vehicles.

Main Idea
The same central platform carries the battery, controller, communication system, sensors and propulsion hardware through all operating modes. Components are reused wherever practical rather than adding an independent propulsion system for every environment.

Operating Principle :
Air Mode
Four A2212-class 1400 KV BLDC motors with 9050 propellers operate in a multicopter configuration. The present design calculations indicate approximately 4.72 kg combined maximum static thrust against a 2.20 kg MTOW, corresponding to a calculated thrust-to-weight ratio of approximately 2.15:1.

Land Mode
The vehicle lands onto its wheel system. Servo-controlled propulsion orientation redirects available thrust to generate terrestrial motion rather than requiring a completely separate high-power traction architecture.

Water Mode
Buoyant wheel/body elements keep the electronics and major structure above the waterline while redirected propulsion produces forward motion across the water surface.

Control System : 
An ESP32-S3-class controller is currently proposed for prototype-level control, handling functions such as:

stabilization/PID control,
IMU and navigation sensor processing,
propulsion commands,
servo actuation,
mode management,
telemetry,
waypoint navigation.

For a research prototype this is reasonable, although a dedicated flight controller or higher-reliability compute architecture may eventually be appropriate as autonomy and safety requirements increase.

Expected Benefits :

The expected benefit is mobility flexibility rather than superior performance in every individual domain.

A purpose-built UAV will likely fly more efficiently, a dedicated UGV can achieve better terrestrial endurance, and a purpose-built USV will generally perform better on water. RiftWalker instead investigates whether somewhat reduced domain-specific efficiency can be accepted in exchange for one platform being able to continue a mission when the terrain changes.
That is a much stronger and more realistic technical proposition.

---

## System Architecture

                    ┌─────────────────────┐
                    │   HUMAN OPERATOR    │
                    │ Ground Control / UI │
                    └──────────┬──────────┘
                               │
                     Telemetry / Commands
                               │
                               ▼
                  ┌────────────────────────┐
                  │ ONBOARD CONTROL SYSTEM │
                  │      ESP32-S3          │
                  │                        │
                  │ • Mission Logic        │
                  │ • PID / Stabilization  │
                  │ • Sensor Fusion        │
                  │ • Mode Management      │
                  │ • Navigation           │
                  └───────┬────────┬───────┘
                          │        │
                 ┌────────┘        └───────────┐
                 ▼                             ▼
        ┌─────────────────┐          ┌─────────────────┐
        │ SENSOR SYSTEM   │          │ ACTUATION       │
        │                 │          │ SYSTEM          │
        │ IMU             │          │                 │
        │ GNSS / NavIC    │          │ 4 × ESC         │
        │ Altitude        │          │ 4 × BLDC        │
        │ Camera*         │          │ Vector Servos   │
        └─────────────────┘          └────────┬────────┘
                                             │
                         ┌───────────────────┼──────────────────┐
                         ▼                   ▼                  ▼
                    AIR MODE            LAND MODE          WATER MODE
                    Multirotor          Wheels +           Buoyancy +
                    Flight              Vector Thrust      Vector Thrust
                         │                   │                  │
                         └───────────────────┼──────────────────┘
                                             ▼
                                  MULTI-DOMAIN MOBILITY

                     ┌─────────────────────────────┐
                     │       POWER SYSTEM          │
                     │  4S 6200 mAh LiPo Battery  │
                     │  Power Distribution / BEC  │
                     └─────────────────────────────┘

```markdown
![System Architecture](images/system_architecture.png)
````

The RiftWalker system architecture is divided into four main sections:

Control System: The onboard controller acts as the brain of the robot. It processes sensor data, manages stabilization and navigation, controls propulsion, and switches between air, land, and water modes.
Sensor & Navigation System: Sensors such as the IMU and GNSS/NavIC provide information about orientation, position, motion, and vehicle state. Additional cameras or perception sensors can be integrated for autonomous navigation.
Propulsion & Actuation System: Four BLDC motors with ESCs generate thrust. Servo mechanisms change the direction of the propulsion system depending on the operating mode—vertical thrust for flight and redirected thrust for movement on land or water.
Power & Communication System: A 4S 6200 mAh LiPo battery supplies the motors, servos, controller, and sensors. A wireless telemetry link allows the ground operator to send commands and receive vehicle data.

Overall, the architecture is designed around sharing the same propulsion, power, sensing, and control hardware across all three environments, reducing the need for separate systems for each mode.

---

## Hardware Requirements

| Sr. No. | Component               | Specification                      | Quantity | Purpose                                           |
| ------- | ----------------------- | ---------------------------------- | -------: | ------------------------------------------------- |
| 1       | BLDC Motor              | A2212, 1400 KV                     |        4 | Common propulsion for air, land and water         |
| 2       | Propeller               | 9050, matched to motor             |        4 | Generates aerodynamic thrust                      |
| 3       | ESC                     | 4S-compatible, appropriately rated |        4 | Controls BLDC motor speed                         |
| 4       | LiPo Battery            | 4S, 6200 mAh                       |        1 | Main onboard power source                         |
| 5       | Microcontroller         | ESP32-S3                           |        1 | Main control, sensor processing and telemetry     |
| 6       | IMU                     | 6/9-axis accelerometer + gyroscope |        1 | Orientation, motion and stabilization             |
| 7       | Sensor (Water/Distance) | Relevent to the use case           |        1 | Outdoor positioning and navigation                |
| 8       | Servo Motors            | High-torque, metal-gear            |        4 | Changes propulsion orientation between modes      |
| 9       | Wheels                  | Lightweight passive/free-rolling   |        4 | Supports terrestrial locomotion                   |
| 10      | Buoyancy System         | Lightweight waterproof floats/body |    1 set | Provides stability and flotation on water         |
| 11      | Power Distribution/BEC  | 4S compatible                      |        1 | Distributes and regulates electrical power        |
| 12      | Telemetry System        | Wireless bidirectional link        |        1 | Operator commands and vehicle telemetry           |
| 13      | Camera                 | Lightweight onboard camera         |        1 | Visual feedback/perception                        |
| 14      | Frame & Mounting System | Lightweight rigid structure        |        1 | Supports all mechanical and electronic components |
| 15      | Flight Controller       | Custom                             |        1 | For functioning of everything perfectly           |
---

## Software Requirements

| Sr. No. | Software / Tool          | Version           | Purpose                                          |
| ------- | ------------------------ | ----------------- | ------------------------------------------------ |
| 1       | Arduino IDE / PlatformIO | Current stable    | ESP32-S3 firmware development                    |
| 2       | Embedded C/C++           | Current toolchain | Low-level control and sensor interfacing         |
| 3       | Python                   | 3.x               | Data analysis, testing and algorithm prototyping |
| 4       | MATLAB / Simulink        | Available release | Modelling, PID tuning and system analysis        |
| 5       | Inventor                 | Available release | Mechanical CAD and assembly design               |
| 6       | KiCad                    | Current stable    | PCB and electronic system design                 |
| 7       | Git / GitHub             | Current stable    | Version control and project documentation        |
| 8       | SimScale / OpenFOAM      | Current stable    | Computational Fluid Dynamics (CFD) simulation    |
| 9       | Wokwi                    | Current stable    | Headless hardware testing prior to flashing.     |

---

## Technologies Used

1. Embedded Systems: ESP32-S3 and Embedded C/C++ for real-time control.
2. Custom ESP32-S3 Architecture: Dual-core utilization with FreeRTOS (Core 0: PID Loops, Core 1: Sensor Fusion & Telemetry).
3. Sensor Fusion & Control Systems: Kalman filtering for IMU data and multi-axis Proportional-Integral-Derivative (PID) tuning.
4. ESP-NOW RF Protocol: Low-latency, peer-to-peer 2.4 GHz wireless telemetry bypassing standard Wi-Fi overhead.
5. PCB Design (EDA): Opto-isolated power domains, differential pair routing, and SMD integration.
6. CAD & Mechanical Design: Development of the lightweight frame, wheel mounts, buoyancy structures and motor-vectoring mechanism.
7. Thrust Vectoring: Servo-controlled motor orientation to reuse propulsion across different operating modes.
8. Multirotor Propulsion: BLDC motors, ESCs and propellers for aerial propulsion.
9. Feedback Control: PID-based stabilization and motion control.
10. Wireless Communication: Bidirectional command, telemetry and vehicle-status communication.
11. Semi Autonomous operation: Automatic terrain detection etc.
12. Data Analysis & Simulation: Python and MATLAB/Simulink for evaluation.

---

## Methodology

## Methodology

1. **Literature Survey:** Study existing multimodal and triphibious robotic platforms to understand their propulsion mechanisms, mobility approaches, control strategies and limitations.

2. **Problem Identification and Requirement Analysis:** Identify the major challenges involved in combining air, land and water mobility and establish the mechanical, electrical and performance requirements of RiftWalker.

3. **Concept Development:** Evaluate different multi-terrain mechanisms and finalize the use of a unified BLDC propulsion system with a **servo-based thrust vectoring mechanism**.

4. **Mechanical Design:** Develop the **3D CAD model in Fusion 360**, including the structural frame, motor mounts, servo tilting mechanism, wheels, buoyancy elements and electronics enclosure.

5. **Engineering Analysis:** Perform weight estimation, thrust-to-weight ratio, servo torque, battery and power, land mobility and buoyancy calculations to evaluate the feasibility of the proposed design.

6. **Simulation and Design Validation:** Validate the proposed mechanical concept and operating principle through simulation and analysis, and modify the design based on the obtained results.

7. **Electronic System Development:** Design the custom **ESP32-S3 based flight controller PCB** in KiCad, including power management, motor and servo interfaces, sensor interfaces, USB-C programming and battery monitoring.

8. **PCB Verification and Hardware Integration:** Perform schematic checks, ERC/DRC verification and PCB routing, followed by fabrication and integration of the controller with the motors, servos, sensors and power system.

9. **Firmware and Control Development:** Develop the ESP32-S3 firmware for sensor processing, stabilization, motor and servo control, communication and operating-mode management.

10. **Testing and Validation:** Test the integrated prototype in air, land and surface-water modes and evaluate parameters such as stability, mobility, thrust-vectoring response, power consumption and transition behaviour.

11. **Performance Analysis and Documentation:** Compare experimental results with calculated design values, identify limitations and improvements, and document the complete design, implementation and testing process.

---

## Project Timeline


| Week / Month |                      Task Planned                                  | Status      |
|--------------|--------------------------------------------------------------------|-------------|
| **Week 1**   | Problem Finalization & Project Planning                            | Completed   |
| **Week 2**   | Literature Survey & Research Paper Review                          | Completed   |
| **Week 3**   | Requirement Analysis & Component Selection                         | Completed   |
| **Week 4**   | Engineering Calculations                                           | Completed   |
| **Week 5**   | Mechanical Design & 3D CAD Modelling                               | Completed   |
| **Week 6**   | Simulation & Design Validation                                     | Completed   |
| **Week 7**   | Flight Controller PCB Design & Schematic Development               | Completed   |
| **Week 8**   | PCB Routing, ERC/DRC Verification & Hardware Procurement           | Completed   |
| **Week 9**   | Documentation, Presentation & Hardware Procurement                 | In Progress |
| **Week 10**  | PCB Fabrication & Assembly                                         | Pending     |
| **Week 11**  | Hardware Integration & Sensor Interfacing                          | Pending     |
| **Week 12**  | Firmware Development & System Testing                              | Pending     |
| **Week 13**  | Final Testing, Documentation & Project Demonstration               | Pending     |
| **Week 14**  | Research Paper Writing & Project Documentation                     | Pending     |
| **Week 15**  | Final Presentation & Project Demonstration                         | Pending     |

---

## Weekly Progress Updates

## Weekly Progress Updates

Students must update this section every week.
| Week  | Date        |                  Work Completed                |             Next Week         |         Issues / Challenges       | GitHub Commit |
|---    |---          |---                                             |---                            |---                                |---|
| **1** | 22/06–28/06 | Problem finalization & planning                | Literature survey             | Concept feasibility               | — |
| **2** | 29/06–05/07 | Literature survey & paper review               | Requirement analysis          | Mobility vs. weight               | — |
| **3** | 06/07–12/07 | Requirements & components finalized            | Engineering calculations      | Weight, thrust & cost             | — |
| **4** | 13/07–19/07 | Weight, thrust, torque & buoyancy calculations | Mechanical design             | Maintaining low weight            | — |
| **5** | 20/07–26/07 | 3D CAD model & mechanical design               | Design validation & PCB       | Mechanism integration             | — |
| **6** | 27/07–02/08 | Design validation & simulation                 | PCB schematic                 | Mechanical-electronic integration | — |
| **7** | 03/08–09/08 | ESP32-S3 schematic & component selection       | PCB routing & verification    | GPIO & power management           | — |
| **8** | 10/08–16/08 | PCB routing, ERC/DRC & documentation           | PCB fabrication & procurement | Routing & component placement     | — |


---

## Design Files

Upload and link all design files here.

| File Type       | File Name / Link | Description |
| --------------- | ---------------- | ----------- |
| CAD Model       |                  |             |
| Circuit Diagram |                  |             |
| PCB Design      |                  |             |
| Flowchart       |                  |             |
| Simulation File |                  |             |

---

## Circuit Diagram

Add circuit diagram image here.

```markdown
![Circuit Diagram](images/circuit_diagram.png)
```

---

## Flowchart / Algorithm

Add flowchart image here.

```markdown
![Flowchart](images/flowchart.png)
```

### Algorithm

1. System Initialization – Power up the ESP32-S3 and initialize the IMU, GNSS/NavIC, altitude sensor, ESCs, servos and communication modules.
2. Self-Test & Health Check – Verify sensor communication, battery voltage, telemetry link and actuator status. Prevent arming if a critical fault is detected.
3. Standby / Arm – Keep propulsion disabled until a valid arm command and mission/operator command are received.
4. Sensor Acquisition – Continuously collect orientation, acceleration, altitude, position and other available sensor measurements.
5. Sensor Fusion & State Estimation – Combine measurements to estimate the robot's attitude, position, velocity and motion state.
6. Control Loop – Run the feedback controllers at high frequency to calculate the required thrust, attitude and motion corrections.
7. Mode Decision – Determine whether Air, Land or Water mode is requested/appropriate. For the prototype, operator-confirmed switching is safer than claiming fully autonomous terrain classification.
8. Execute Selected Mode – In Air Mode, operate as a multirotor; in Land Mode, redirect thrust and use the wheels; in Water Mode, maintain flotation and use vectored thrust for surface movement.
9. Navigation – Follow manual commands or mission waypoints while continuously updating the required heading, speed and position.
10. Safety Monitoring – Continuously check battery level, communication link, attitude, actuator/current limits and other available health information.
11. Failsafe Handling – If a critical fault occurs, execute the safest response available for the current mode, such as controlled landing, stopping propulsion, remaining afloat, or returning to a safe location where technically feasible.
12. Telemetry & Logging – Transmit vehicle status to the ground station and record important sensor, control and fault information.
13. Mission Check – If the mission is incomplete, return to the sensing/control loop. If complete, move to shutdown.
14. Safe Shutdown – Stop the mission, disarm propulsion and place actuators in their safe state.

---

## Implementation Details

Explain the actual implementation of the project.

### Hardware Implementation

The hardware architecture is centered around a custom-designed, 2-layer mixed-signal PCB acting as the primary flight controller.

1. Power Domains: To prevent inductive voltage spikes from resetting the logic core, power is strictly isolated into two domains. The "Muscle Domain" draws directly from the 4S 6200mAh LiPo battery through a Matek PDB and a 5A UBEC to drive the 30A ESCs and the high-torque DS3218 vectoring servos at 7V-11V. The "Logic Domain" steps voltage down to a clean 3.3V via an AP2112K LDO regulator to power the ESP32-S3, the MPU6050 IMU, and the VL53L1X altimeter.
2. Structural Assembly: The chassis is fabricated using rigid, lightweight 3d printed components. The four A2212 1400KV BLDC motors are mounted on pivoting brackets connected to the servos, allowing a 90deg dynamic sweep.
3. Multi-Domain Mobility: Along with rubber tires , 190mm Extruded Polystyrene (XPS) closed-cell foam base are attached to the chassis. These provide passive mechanical suspension on pavement and displace enough water (approx. 4.8 Liters) to act as unsinkable buoyancy floats, keeping the central electronics housing safe from water ingress.

### Software Implementation

The firmware is written in Embedded C/C++ using the Arduino IDE and is built upon the FreeRTOS (Real-Time Operating System) framework.

1. Dual-Core Architecture: Task allocation is split across the two cores of the ESP32-S3 to ensure zero latency in critical flight calculations. Core 0 is exclusively dedicated to reading the MPU6050 via I2C, running the Kalman filter, and calculating the high-frequency PID (Proportional-Integral-Derivative) stabilization loops for pitch, roll, and yaw. Core 1 handles non-critical timing tasks, including the transitional state machine (coordinating the servo sweep angles), calculating battery voltage telemetry, and managing the ESP-NOW 2.4GHz peer-to-peer control link.
2. State Machine: A custom dynamic mixing algorithm smoothly blends standard quadcopter multirotor logic into forward-thrust airboat logic as the servos transition from 0degto 90deg.
---

## Code Structure

```text
BE-Capstone-Project/
│
├── README.md
├── docs/
│   ├── literature_survey.md
│   ├── project_report.pdf
│   └── presentation.pptx
│
├── hardware/
│   ├── circuit_diagram.png
│   ├── pcb_design/
│   └── cad_model/
│
├── software/
│   ├── src/
│   ├── include/
│   └── tests/
│
├── images/
│   ├── system_architecture.png
│   ├── prototype_photo.jpg
│   └── results.png
│
└── references/
    └── papers/
```

---

## How to Run the Project

### Step 1: Clone the Repository

```bash
git clone https://github.com/username/project-name.git
```

### Step 2: Install Dependencies

```bash
pip install -r requirements.txt
```

or mention specific software/library installation steps.

### Step 3: Upload / Run the Code

```bash
python main.py
```

or

```bash
arduino-cli upload -p COMx --fqbn board_name
```

### Step 4: Observe the Output

When powered, the RiftWalker initializes its IMU and establishes a secure ESP-NOW link with the ground station, displaying battery voltage and current vector angles. Upon command, the craft can spool its motors to take off vertically like a standard drone, demonstrating stable hover capabilities. When the operator engages "Surface Mode," the servos smoothly rotate the motor arms $90^\circ$ forward while adjusting thrust to prevent altitude loss. On the ground or water, the vehicle utilizes differential aero-thrust to steer and propel itself forward at high speeds, gliding on its XPS wheels with a drastically reduced current draw compared to hovering.

---

## Testing and Results

| Test No. | Test Description | Expected Result | Actual Result | Status      |
| -------- | ---------------- | --------------- | ------------- | ----------- |
| 1        |                  |                 |               | Pass / Fail |
| 2        |                  |                 |               | Pass / Fail |
| 3        |                  |                 |               | Pass / Fail |

---

## Result Images / Videos

Add images or videos of the working prototype.

```markdown
![Prototype](images/prototype_photo.jpg)
```

Video Link:

```markdown
[Project Demo Video](https://drive.google.com/your-video-link)
```

---

## Applications

1. Post-Disaster Search & Rescue (SAR): Navigating through flooded urban environments, blocked roads, and debris dams where neither standard boats nor ground rovers can pass.
2. Hazardous Environmental Monitoring: Surveying toxic lakes, unstable wetlands, or swamp ecosystems without the risk of underwater propellers getting tangled in aquatic vegetation.
3. Tactical Reconnaissance & Border Patrol: Serving as a covert, multi-terrain sentinel capable of flying to a location rapidly and transitioning to a low-power, silent ground/water crawl for long-term observation.
4. Amphibious Ship-to-Shore Scouting: Launching from a naval vessel, flying to the coastline, and seamlessly driving onto the beach to conduct physical surveys.
ETC.

---

## Advantages

1. Elimination of Drivetrain Bloat: Achieves an exceptional 2.15:1 thrust-to-weight ratio by using a single set of aero-propellers for all terrains, eliminating the dead-weight of separate wheel motors and heavy gearboxes.
2. Exponential Endurance Gains: By dropping to the ground or water and operating as an aero-propelled rover, mission endurance is extended from a maximum of ~9.5 minutes in the air up to ~54.5 minutes on the surface.
3. Passive Buoyancy & Suspension: The lightweight XPS foam wheels serve a dual purpose, acting as excellent shock absorbers for rough terrain and fail-safe, unsinkable flotation hulls for water traversal.
4. Cost-Effective & Scalable: By eliminating redundant drive hardware and integrating a custom ESP32-based flight controller, the unit cost is drastically reduced compared to commercial military-grade amphibious systems.

---

## Limitations

1. Transition Vulnerability: The physical transition phase takes  seconds, during which the craft is highly susceptible to sudden crosswinds or aerodynamic stalling if not perfectly timed.
2. Ground Clearance Ceilings: Terrestrial obstacle clearance is strictly limited by the radius of the XPS wheels; the vehicle cannot drive over boulders or deep trenches without reverting to flight mode.
3. Acoustic Signature: Using aero-propulsion to drive on land generates significantly more noise (propeller wash) than a traditional electric wheeled rover, reducing its stealth capabilities in close-proximity scenarios.
4. Thermal Constraints: When planing on water at high throttle for extended periods, the enclosed central electronics box lacks active airflow cooling, potentially leading to thermal throttling of the ESCs.
   
---

## Future Scope

1. Autonomous AI Integration: Upgrading the processing core to incorporate a lightweight AI vision module (e.g., using the ESP32-S3's vector instructions or an external neural accelerator) for autonomous optical obstacle avoidance.
2. Triple-Redundant Aerospace Sensing: Developing a "V2" PCB that utilizes triple-redundant IMUs and Barometers connected via an SPI bus to create a fault-tolerant voting architecture for military-grade reliability.
3. BVLOS Telemetry Upgrade: Replacing the 2.4GHz ESP-NOW link with a secure, AES-encrypted Long Range (LoRa) 915MHz telemetry module for Beyond Visual Line of Sight (BVLOS) operations.
4. Aquatic Self-Righting Mechanism: Engineering an automated algorithmic response or physical mass-shifting mechanism to ensure the craft can self-right and recover if flipped upside down by heavy waves in water mode.
   
---

## Research Paper / Publication

| Item                      | Details                                                   |
| ------------------------- | --------------------------------------------------------- |
| Paper Title               |                                                           |
| Conference / Journal Name |                                                           |
| Paper Status              | Not Started / Drafting / Submitted / Accepted / Published |
| Submission Date           |                                                           |
| Paper Link                |                                                           |

---

## References

Add references in IEEE format.

Example:

## References

[1] G. Zhong, J. Cao, X. Chai, and Y. Bai, "Design and Performance Analysis of a Triphibious Robot with Tilting-Rotor Structure," *IEEE Access*, vol. 9, pp. 10871–10879, 2021, doi: 10.1109/ACCESS.2021.3050182.

[2] X. Li, M. Lai, M. Zhang, J. Lin, T. Lai, J. Zhi, C. Xu, F. Gao, Y. Cao, "TriphiBot: A Triphibious Robot Combining FOC-based Propulsion with Eccentric Design," arXiv preprint arXiv:2602.01385, 2026.

[3] M. Xia, B. Wang, and Z. Luo, "Design and Experiment of a Multimodal Aerial Aquatic Vehicle with Morphing Wing and Tilting Rotors," *Advances in Mechanical Engineering*, vol. 17, 2025, doi: 10.1177/16878132251319142.

[4] S. L. Pradeep Kumar, B. Deeban, D. Santhosh Kumar, and M. Sasi Kumar, "Design and Fabrication of a Multi-Terrain Triphibian Quadcopter for Airborne, Terrestrial, and Aquatic Mobility," *Discover Mechanical Engineering*, vol. 5, art. no. 20, 2026, doi: 10.1007/s44245-025-00167-7.

[5] Z. Zheng, Q. Cai, J. Wang, X. Xu, M. Cao, H. Yu, J. Li, J. Meng, and G. Lu, "CapsuleBot: A Novel Hybrid Aerial-Ground Bi-Copter Robot With Two Actuated-Wheel-Rotors," *IEEE Robotics and Automation Letters*, vol. 10, no. 1, pp. 120–127, 2025, doi: 10.1109/LRA.2024.3504232.
---

## Repository Update Guidelines

Each student team must update the GitHub repository regularly.

Minimum expected updates:

* Update README every week.
* Push code changes regularly.
* Upload circuit diagrams, CAD files, PCB files, reports and presentations.
* Add weekly progress in the progress table.
* Maintain proper folder structure.
* Do not upload unnecessary temporary files.
* Each major update should have a meaningful commit message.

Example commit messages:

```text
Added problem statement and objectives
Updated system architecture diagram
Added sensor interfacing code
Updated weekly progress for Week 3
Added testing results and prototype images
```

---

## Declaration

We declare that this project work is carried out by our team as part of the BE Capstone Project. The work will be regularly updated on GitHub and all references used will be properly cited.

---

## License

This project is for academic use only.

Optional:

```text
MIT License / Creative Commons / Institute Use Only
```

```
```
