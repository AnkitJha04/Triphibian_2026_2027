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

**Project Guide: Sangeeta Prasanna Ram**  
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

1. To study existing multimodal robotic platforms and identify limitations associated with operating separate UAV, UGV and waterborne systems.
2. To design a lightweight triphibious architecture capable of aerial, terrestrial and surface-water operation while sharing major propulsion, power and control components.
3. To develop the mechanical and electronic prototype, including propulsion, motor-vectoring mechanism, wheels, buoyancy system, sensors, power electronics and onboard control.
4. To implement the control and navigation system for stabilization, mode switching, sensor processing, waypoint navigation and human-in-the-loop operation.
5. To experimentally test and validate flight, ground movement, water movement, stability, transition behaviour, speed, endurance and power consumption.
6. To compare measured results with calculated design values and identify the efficiency penalties introduced by combining three mobility modes.
7. To document the complete design, implementation and experimental results for academic presentation and potential publication.

---

## Scope of the Project

This project will cover the following key areas:

1. Design and development of prototype: Physical construction of the RiftWalker airframe.
2. Hardware implementation: Assembly and integration of the custom mixed-signal PCB, actuators, and propulsion systems.
3. Software/wireless interface: Firmware programming using FreeRTOS for dual-core task allocation (PID loops and sensor fusion) and the implementation of a low-latency ESP-NOW ground control link.
4. Data collection and testing: Real-world validation of transition stability, payload capacity, and buoyancy.
5. Performance analysis: Evaluating power efficiency, battery endurance curves, and thrust-to-weight ratios across different modes of locomotion.

---

## Existing System

Current unmanned robotic systems can broadly be divided into:
| Platform              | Strength                            | Main limitation                                        |
| --------------------- | ----------------------------------- | ------------------------------------------------------ |
| **UAV**               | Fast, obstacle-independent movement | High energy consumption and relatively short endurance |
| **UGV**               | Efficient terrestrial operation     | Terrain and water obstacles restrict mobility          |
| **USV**               | Efficient surface-water operation   | Cannot independently traverse most land obstacles      |
| **Dual-mode robot**   | Operates in two domains             | Third domain remains inaccessible                      |
| **Triphibious robot** | Potential three-domain operation    | Increased mass, control and mechanical complexity      |

Triphibious robotics itself is not unprecedented. Air-land-water prototypes have appeared in published research, including designs using morphing structures, shared propulsion and minimalist wheel arrangements. Recent work continues to focus specifically on reducing the actuator count and improving propulsion efficiency.The key research gap is therefore not simply making a robot that can fly, drive and move on water. The meaningful engineering challenge is doing so while maintaining acceptable:
weight → energy efficiency → mechanical simplicity → controllability → useful endurance.

Limitations of existing systems:
1. High cost and poor scalability: Deploying a fleet of separate air, land, and water robots requires complex logistics, multiple operators, and heavy transport infrastructure.
2. Low endurance: Aerial drones must constantly fight gravity, severely limiting mission time (typically under 20 minutes).
3. Environmental deadlocks: UGVs are stopped by floods or rubble; boats are stranded by sandbars or thick aquatic vegetation.
4. Mechanical bloat: Existing "amphibious" vehicles often use dual drivetrains (e.g., adding heavy wheel motors to a drone chassis or underwater propellers to a rover), which drastically increases weight and ruins the aerodynamic thrust-to-weight ratio.

That is the problem RiftWalker should attempt to investigate.

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

| Sr. No. | Component | Specification | Quantity | Purpose |
| ------- | --------- | ------------- | -------- | ------- |
| 1       |           |               |          |         |
| 2       |           |               |          |         |
| 3       |           |               |          |         |
| 4       |           |               |          |         |

---

## Software Requirements

| Sr. No. | Software / Tool | Version | Purpose |
| ------- | --------------- | ------- | ------- |
| 1       |                 |         |         |
| 2       |                 |         |         |
| 3       |                 |         |         |

---

## Technologies Used

Mention technologies used in the project.

Example:

* Embedded C / Python / JavaScript
* Arduino / STM32 / ESP32 / Raspberry Pi
* ROS / MATLAB / Simulink
* Machine Learning / Computer Vision
* IoT / Cloud / Mobile App
* PCB Design / CAD Design

---

## Methodology

Explain the step-by-step approach.

1. Literature survey
2. Problem identification
3. Requirement analysis
4. System design
5. Hardware/software development
6. Integration
7. Testing and validation
8. Documentation and publication

---

## Project Timeline

| Week / Month | Task Planned          | Status                            |
| ------------ | --------------------- | --------------------------------- |
| Week 1       | Problem finalization  | Pending / In Progress / Completed |
| Week 2       | Literature survey     |                                   |
| Week 3       | Requirement analysis  |                                   |
| Week 4       | System design         |                                   |
| Week 5       | Prototype development |                                   |
| Week 6       | Testing               |                                   |
| Week 7       | Documentation         |                                   |
| Week 8       | Paper writing         |                                   |

---

## Weekly Progress Updates

Students must update this section every week.

| Week   | Date | Work Completed | Work Planned for Next Week | Issues / Challenges | GitHub Commit Link |
| ------ | ---- | -------------- | -------------------------- | ------------------- | ------------------ |
| Week 1 |      |                |                            |                     |                    |
| Week 2 |      |                |                            |                     |                    |
| Week 3 |      |                |                            |                     |                    |
| Week 4 |      |                |                            |                     |                    |
| Week 5 |      |                |                            |                     |                    |
| Week 6 |      |                |                            |                     |                    |
| Week 7 |      |                |                            |                     |                    |
| Week 8 |      |                |                            |                     |                    |

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

1. Start
2. Initialize the system
3. Read input from sensors/user
4. Process the data
5. Generate output/control action
6. Display/store/transmit result
7. Stop

---

## Implementation Details

Explain the actual implementation of the project.

### Hardware Implementation

Write details about connections, components, power supply, sensors, actuators, PCB, enclosure, etc.

### Software Implementation

Write details about code structure, libraries used, algorithms, communication protocols, database, app, cloud, etc.

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

Mention the expected output of the project.

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

Mention real-world applications of the project.

1.
2.
3.
4.

---

## Advantages

1.
2.
3.
4.

---

## Limitations

1.
2.
3.
4.

---

## Future Scope

Mention possible improvements.

1.
2.
3.
4.

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

```text
[1] A. Author, B. Author, "Title of the Paper," Journal/Conference Name, vol. X, no. Y, pp. xx-yy, Year.
[2] Datasheet / Website / Book reference.
```

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
