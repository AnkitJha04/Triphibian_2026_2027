# RIFTWALKER preflight / bring-up validation checklist

This checklist is deliberately ordered so software and bench verification happen before the flight-control output gates could ever be changed. All propellers remain removed during the initial checks.

## A. PCB/net review

- [ ] Reconcile typed ESC/servo pin assignments with KiCad net names and connector labels.
- [ ] Continuity-test each output header signal to the intended GPIO; verify no shorts to 3V3/5V/GND.
- [ ] Verify common signal ground from ESP32-S3 to all ESCs and servos.
- [ ] Verify servo supply current capability, voltage and ground return are appropriate; do not power servos from a weak 3V3 rail.
- [ ] Correct or verify the HC-SR04 ECHO divider so the ESP32-S3 input sees a safe voltage and a valid logic-high.
- [ ] Water-sensor GPIO42 and battery GPIO11 are not used for validated readings; revise hardware or add a suitable external ADC as documented.
- [ ] Verify I²C pull-ups, bus voltage, SDA/SCL continuity and no address conflict.

## B. Sensor bring-up

- [ ] I²C scanner detects MPU6050 and MS5607 at their actual addresses.
- [ ] MPU6050 stays responsive for at least several minutes; sensor age does not expire.
- [ ] Confirm PCB components-side-down orientation and USB-C/front reference in the body frame. Physically tilt each axis and verify sign and scale before marking axes valid.
- [ ] Run gyro calibration on a stationary, level chassis and verify the stillness test passes.
- [ ] MS5607 PROM CRC passes; pressure and temperature are plausible and continuous.
- [ ] Set a ground-pressure reference; verify relative altitude returns near zero while stationary and changes in the correct direction when raised/lowered by hand.
- [ ] Range ECHO voltage checked with a meter/scope; compare HC-SR04 reading to a ruler only after electrical level is safe.
- [ ] Do not enable water input until its valid ADC/interface and wet/dry threshold have been implemented.
- [ ] No magnetometer input is assumed until the exact part, address, orientation and driver are selected.

## C. Communication / RTOS

- [ ] Direct USB serial emits `@TEL` JSON at the expected cadence and all numeric fields remain honest/null when unavailable.
- [ ] Desktop/serial disconnect makes telemetry stale, clears live values and blocks state-dependent commands.
- [ ] Check task creation succeeds; loop duration and overrun counters remain stable for an extended idle run.
- [ ] ESP-NOW peer MACs and channel match; commands produce sequence-correlated `@ACK,RX` application results.
- [ ] Test packet loss, repeated packets, wrong MAC, wrong CRC, malformed commands, out-of-range gains, queue full and serial line overflow.
- [ ] Validate link-loss behaviour with actuator outputs physically disconnected. A software timeout is not proof of a safe airborne descent.

## D. Actuator bench validation (requires a separately reviewed stage)

- [ ] Confirm motor positions and rotation directions without propellers, using a safe ESC test method.
- [ ] Check servo direction and actual linkage movement over small pulse/angle increments; never assume command degrees equal linkage angle.
- [ ] Check vector pivots do not bind and mechanical endpoints have clearance in all modes.
- [ ] Identify thrust axes at the actual land/air poses and measure motor/propeller thrust, current and response; verify thrust reserve for 2 kg all-up mass.
- [ ] Validate the control allocator and feedback sign using recorded sensor/controller logs before applying motor outputs.
- [ ] Test a deliberate sensor fault and command-loss while tethered/restraint-approved, with a physical cutoff ready.
- [ ] Review and validate the failsafe state for every operational phase. An immediate motor disarm in the air can cause a fall.
- [ ] Only after signed technical review should compile-time gates be considered for modification. Compile-time flags alone are not validation.

## Gate status in this release

This release leaves ESC output, servo output, auto transitions and altitude hold disabled. The board net map, body axes, mixer/rotation, servo geometry, range interface and failsafe are not validated. Arming is expected to remain blocked. This is intentional.
