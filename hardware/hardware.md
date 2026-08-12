<div align="center">
  <h1 style="font-family: 'Helvetica Neue', Arial, sans-serif; font-weight: 300; letter-spacing: 3px; color: #1a202c; margin-bottom: 0;">
    HARDWARE ARCHITECTURE
  </h1>
  <p style="font-size: 1.3em; color: #3182ce; font-weight: bold; margin-top: 5px; letter-spacing: 1.5px;">
    RiftWalker: Unified Triphibious Robotics
  </p>
  <hr style="width: 120px; border: 2px solid #3182ce; border-radius: 2px; margin-top: 15px; margin-bottom: 35px;">
</div>

<blockquote>
  <strong>Welcome to the RiftWalker Hardware Repository.</strong> This document outlines the physical engineering and custom avionics required to build the RiftWalker platform. By completely abandoning off-the-shelf drone frames and standard flight controllers, this hardware is purpose-built to eliminate parasitic dead-weight and enable unrestricted multi-domain mobility across Air, Land, and Water.
</blockquote>

<br>

---

##  Repository Structure

```text
├── /CAD            # 3D models and structural files
    Required Cad Files
├── /PCB            # KiCad project files and Schematics
    Required PCB Files
```

<br>

---

## Mechanical Design (Frame & Actuation)

The physical chassis is explicitly engineered to solve two major triphibious challenges: aerodynamic blockage and terrain-locking.

*   **Z-Shaped Carbon-Fiber Arms:** The motor arms step upwards in a distinct Z-shape. This elevates the propeller arc, completely clearing the main chassis from the downward airflow, thereby preventing parasitic thrust-blockage.
*   **Dynamic Vectoring Hubs:** Instead of adding extra drivetrains, high-torque digital servos physically sweep the BLDC propulsion system **90 degrees forward**. This allows RiftWalker to steer using differential thrust on both the ground and water.
*   **Multi-Domain XPS Wheels:** The underslung wheels are constructed from closed-cell XPS foam. On land, they provide mechanical ground clearance; on water, they act as precisely calculated buoyant pontoons, forcing the waterline strictly below the electronics bay.

<br>

---

## Custom Avionics (ESP32-S3 PCB)

Off-the-shelf flight controllers are bloated and strictly designed for single-domain use. We engineered a custom **ESP32-S3-WROOM-1** PCB from scratch to gain total hardware access for our dual-core FreeRTOS architecture.

### Key PCB Design Decisions:

#### 1. Strict Power Isolation
*   **Logic (3.3V):** Powered by an AP2112K LDO regulator, providing clean, stable power to the ESP32 and $I^2C$ sensors.
*   **Actuators (5V):** Servo power is completely decoupled from the logic circuit. A large bulk polarized capacitor (`C10`) is placed on the 5V rail to absorb the massive current spikes caused by the vectoring servos, preventing mid-air brownouts.

#### 2. Sensor Thermal & RF Shielding
The **MPU-6050** (6-axis IMU) and **MS5607** (Barometer) are intentionally mounted on the **bottom layer** of the PCB. This physically isolates them from the ESP32’s thermal output and RF noise, ensuring perfectly clean environmental data.

#### 3. Antenna Keep-Out Zone
A strict copper keep-out zone is enforced around the ESP32 antenna to maximize telemetry and control range across all terrains.

#### 4. Safe Logic-Level Shifting
The HC-SR04 ultrasonic sensor outputs a 5V echo pulse. A precise $2\text{k}\Omega / 1\text{k}\Omega$ hardware voltage divider safely steps this down to 3.3V, protecting the microcontroller's GPIO pins.

<br>

---

## Bill of Materials (Core Components)

| Component | Description | Qty |
| :--- | :--- | :---: |
| **ESP32-S3-WROOM-1** | Dual-core MCU for custom FreeRTOS flight logic | 1 |
| **MPU-6050** | 6-Axis IMU (Pitch/Roll/Yaw stabilization) | 1 |
| **MS5607-02BA** | High-precision barometer for altitude detection | 1 |
| **HC-SR04** | Ultrasonic distance sensor | 1 |
| **BLDC Motors & ESCs** | Primary unified propulsion | 4 |
| **High-Torque Servos** | Actuates the 90-degree vectoring sweep | 4 |
| **AP2112K-3.3** | 3.3V Low-Dropout Voltage Regulator | 1 |

<br>

---
*For software implementation, firmware flashing instructions, and FreeRTOS details, please refer to `SOFTWARE.md`.*
