<div align="center">
  <h1 style="font-family: 'Helvetica Neue', Arial, sans-serif; font-weight: 300; letter-spacing: 3px; color: #1a202c; margin-bottom: 0;">
    LITERATURE SURVEY
  </h1>
  <p style="font-size: 1.3em; color: #3182ce; font-weight: bold; margin-top: 5px; letter-spacing: 1.5px;">
    RiftWalker Triphibious Robotic Platform
  </p>
  <hr style="width: 120px; border: 2px solid #3182ce; border-radius: 2px; margin-top: 15px; margin-bottom: 35px;">
</div>

<blockquote>
  <strong>Overview:</strong> Recent advancements in autonomous systems have catalyzed the development of multi-domain robotic platforms capable of traversing land, air, and aquatic environments. Researchers are actively exploring unified propulsion architectures, dynamic mechanical reconfiguration, and advanced control logic to optimize cross-domain mobility. The following literature review analyzes the current state-of-the-art in triphibious systems, identifying critical mechanical strengths and structural limitations to establish the engineering foundation for the <strong>RiftWalker</strong> mechatronic architecture.
</blockquote>

<br>

---

## Reviewed Literature

### 1. Kinematic Reconfiguration via Tilting-Rotor Structures (2020)
*   **Paper:** *"Design and Performance Analysis of A Triphibious Robot with Tilting-Rotor Structure"*
*   **Core Mechanism:** This study investigates the application of a tilt-rotor mechanism to dynamically alter propeller thrust vectors, enabling seamless mechanical transitions between distinct modes of locomotion without requiring redundant drive systems.
*   **RiftWalker Inspiration:** This research successfully validated `dynamic thrust vectoring` as a mathematically and mechanically superior method for multi-terrain mobility. It serves as the direct foundational inspiration for RiftWalker's 90-degree, servo-actuated vectoring hubs, allowing us to actively shift the propulsion plane.

---

### 2. Actuator Multiplexing and Propulsive Unification (2026)
*   **Paper:** *"TriphiBot: A Triphibious Robot"*
*   **Core Mechanism:** The authors propose a highly unified propulsion architecture that completely eliminates secondary drivetrains. Instead of integrating independent mechanisms for flying, driving, and swimming, the system intelligently reuses a single set of motors across all three environmental domains.
*   **RiftWalker Inspiration:** This approach decisively solves the issue of `parasitic dead-weight`. By adopting this unified philosophy, RiftWalker uses the exact same BLDC flight motors to generate aerial lift and provide differential thrust for terrestrial and marine steering, drastically reducing overall system complexity and gross takeoff weight.

---

### 3. Aerodynamic Transitions in Multimodal Vehicles
*   **Paper:** *"Design and Experiment of a Multimodal Aerial-Aquatic Vehicle with Morphing Wing and Tilting Rotors"*
*   **Core Mechanism:** The paper introduces a reconfigurable morphing structure paired with tilting rotors, specifically optimized to handle the extreme fluid dynamic differences between aerial downwash and aquatic displacement.
*   **RiftWalker Inspiration:** The study confirmed that dynamically changing rotor geometry is critical for operational flexibility and efficiency. This heavily influenced our structural design—specifically our `Z-shaped carbon-fiber arms`—which elevate the propeller arc to ensure the chassis does not block downward airflow, locking in a high thrust-to-weight ratio.

---

### 4. Pragmatic Deployment of Multi-Terrain Quadcopters
*   **Paper:** *"Design and Fabrication of a Multi-Terrain Triphibian Quadcopter"*
*   **Core Focus:** This research shifts focus toward real-world deployment, evaluating multi-domain platforms in the context of precision agriculture, hazardous infrastructure inspection, and rapid disaster management.
*   **RiftWalker Inspiration:** The study explicitly highlighted how unifying three vehicular roles into a single platform drastically reduces operational overhead and boosts mission response times. This strategic analysis directly shaped the target use-cases for RiftWalker, emphasizing `terrain independence` for tactical and search-and-rescue operations.

---

### 5. Extreme Hardware Optimization (2023)
*   **Paper:** *"CapsuleBot: Hybrid Aerial-Ground Robot"*
*   **Core Mechanism:** This paper demonstrates a hybrid model that shares the exact same actuators for both airborne flight and terrestrial rolling, pushing the boundaries of mechanical minimization.
*   **RiftWalker Inspiration:** The emphasis on hyper-efficient hardware utilization inspired RiftWalker’s elimination of complex, heavy steering racks. Instead of dedicated steering hardware, RiftWalker relies entirely on `differential thrust` and the dual-purpose nature of our XPS foam wheels (acting as both ground clearance and marine pontoons).

<br>

---

## Research Gap & The RiftWalker Solution

While existing literature demonstrates the theoretical viability of multi-terrain robotics, the current generation of triphibious prototypes consistently suffers from three critical engineering flaws:

*   **High Mechanical Complexity:** Reliance on heavy steering racks, independent drive axles, and redundant marine thrusters.
*   **Terrain-Locking & Parasitic Mass:** When operating in one domain (e.g., flight), the hardware dedicated to the other two domains becomes dead-weight, severely degrading battery life and maneuverability.
*   **Bloated Avionics:** Dependence on off-the-shelf, single-domain flight controllers that lack the multi-threaded processing required for instantaneous shape-shifting and cross-domain stabilization.

<br>

### Our Approach: The RiftWalker Architecture
RiftWalker is engineered to bridge these gaps by delivering a highly scalable, maximum-efficiency architecture that completely eliminates parasitic drivetrain weight. We achieve this through three core pillars:

#### 1. Unified Mechatronic Mechanism
By deploying high-torque digital servos, the entire BLDC propulsion system is dynamically vectored. This allows RiftWalker to steer across land and water using pure differential thrust, aided by underslung closed-cell XPS foam wheels that provide mechanical clearance on land and precise buoyancy control on water.

#### 2. Custom Multi-Domain Avionics
Bypassing pre-existing, off-the-shelf software, RiftWalker is powered by a custom-engineered `ESP32-S3 PCB`. This hardware features strict power and sensor isolation to protect sensitive IMU and barometric data from high-current servo spikes.

#### 3. Real-Time Dual-Core Processing
Utilizing a custom FreeRTOS firmware written from scratch, RiftWalker dedicates one processing core exclusively to `environmental sensor fusion` (detecting the active domain) and the second core strictly to the `PID stabilization` and motor timing loops, ensuring seamless, interruption-free transitions across air, land, and water.
