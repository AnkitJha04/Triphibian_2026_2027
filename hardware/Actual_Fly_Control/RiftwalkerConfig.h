#pragma once
// RIFTWALKER ESP32-S3-WROOM-1: configuration from user's typed GPIO table.
// IMPORTANT: uploaded KiCad schematic header/net labels appear to differ from this pin table.
// PCB is fabricated but NOT assembled. Keep both output gates at 0 until nets are continuity-tested.
#include <Arduino.h>

// The configured pin map and peripherals are for the ESP32-S3-WROOM-1 only.
// Refuse a generic/original ESP32 build rather than producing a plausible but unsafe binary.
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
  #error "Select an ESP32-S3 board (for example ESP32S3 Dev Module) before compiling Riftwalker firmware."
#endif

static constexpr int ESC_PINS[4]   = {15, 6, 18, 9}; // M1 FR-CCW, M2 RR-CW, M3 FL-CW, M4 RL-CCW (user-provided)
static constexpr int SERVO_PINS[4] = {8, 17, 7, 16}; // S1..S4 each moves corresponding motor pod (user-provided)
static constexpr int I2C_SDA_PIN = 4;
static constexpr int I2C_SCL_PIN = 5;
static constexpr int HC_SR04_TRIG_PIN = 12;
static constexpr int HC_SR04_ECHO_PIN = 13; // Circuit level must be validated: schematic's apparent divider may be too low for a logic HIGH
static constexpr int WATER_SENSOR_ADC_PIN = 42; // User's fabricated PCB pin. GPIO42 is NOT ADC-capable on ESP32-S3; do not analogRead it.
static constexpr int BATTERY_ADC_PIN = 11; // User's PCB pin (ADC2); do not claim valid data with ESP-NOW/Wi-Fi active.
static constexpr int MAG_DRDY_PIN = 14; // Reserved for a future, specifically identified magnetometer; no magnetometer installed/configured.
static constexpr uint8_t MPU6050_ADDR_A = 0x68;
static constexpr uint8_t MPU6050_ADDR_B = 0x69; // fallback only if 0x68 does not respond
static constexpr uint8_t MS5607_ADDR_A = 0x76;
static constexpr uint8_t MS5607_ADDR_B = 0x77; // fallback only if 0x76 does not respond
static constexpr int ESC_MIN_US = 1000;
static constexpr int ESC_SPINUP_US = 1080; // reported measured rotation onset; not used as disarmed pulse
static constexpr int ESC_MAX_US = 1800;   // normal firmware cap; calibration sketch separately permits 2000 us after explicit confirmation
static constexpr int ESC_RATE_HZ = 50;    // verify SimonK build accepts standard PWM at this rate
static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2500;
// When an otherwise healthy airborne controller loses its command heartbeat, retain the
// last bounded attitude targets and trims only long enough for a transient link recovery.
// This is deliberately NOT an autonomous hover or landing mode; after the window the
// controller disarms. Validate the vehicle-specific lost-link response before enabling PWM.
static constexpr uint32_t LINK_LOSS_ATTITUDE_HOLD_MS = 800;
// Latest user-provided linkage targets, command targets only (no position feedback installed).
static constexpr int LAND_SERVO_DEG[4] = {5, 0, 0, 5};
static constexpr int AIR_SERVO_DEG[4]  = {100, 90, 90, 100};
// No water linkage/propulsion geometry was supplied. No placeholder angle is ever written.
static constexpr bool WATER_SERVO_GEOMETRY_VALID = false;
static constexpr float BATTERY_DIVIDER_RATIO = 0.0f; // Uncalibrated; battery voltage must remain null
static constexpr int WATER_ADC_THRESHOLD = 0; // Uncalibrated; no automatic wet detection
static constexpr bool WATER_SENSOR_CALIBRATED = false;
static constexpr bool WATER_WET_ABOVE_THRESHOLD = true;
static constexpr bool BATTERY_DIVIDER_CALIBRATED = false;
static constexpr bool HCSR04_LEVEL_SHIFT_VALIDATED = false;
static constexpr bool BODY_AXIS_MAPPING_VALIDATED = false; // PCB component-side down; axes must be bench-verified
static constexpr bool BOARD_NET_MAP_VALIDATED = false;      // KiCad net/header map vs typed arrays not yet reconciled
static constexpr bool MOTOR_ORDER_VALIDATED = false;
static constexpr bool SERVO_GEOMETRY_VALIDATED = false;
static constexpr bool AIR_GEOMETRY_VALIDATED = false;
static constexpr bool FAILSAFE_VALIDATED = false;
static constexpr bool ALTITUDE_HOLD_VALIDATED = false;
static constexpr bool LAND_WATER_MECHANISM_VALIDATED = false;

// SAFETY GATES. Do not set these to 1 until the physical validation checklist is signed off.
#define RIFT_ENABLE_ACTUATOR_OUTPUTS 0
#define RIFT_ENABLE_SERVO_OUTPUTS 0
#define RIFT_ENABLE_ESPNOW_RX 1 // protocol support enabled; paired radio remains disabled until MACs are set
#define RIFT_ENABLE_AUTO_TRANSITIONS 0
#define RIFT_ENABLE_ALTITUDE_HOLD 0

// Fill the reciprocal MACs after reading them from each board over USB serial.
// ESP-NOW channel is fixed at 1 on BOTH boards. All-zero MAC means unpaired and safe.
static constexpr uint8_t ESPNOW_GROUND_MAC[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
static constexpr uint8_t ESPNOW_ROVER_CHANNEL = 1;

#if RIFT_ENABLE_ACTUATOR_OUTPUTS
static_assert(BOARD_NET_MAP_VALIDATED, "ESC outputs blocked: verify PCB nets against GPIO table.");
static_assert(MOTOR_ORDER_VALIDATED, "ESC outputs blocked: validate motor order/rotation.");
static_assert(SERVO_GEOMETRY_VALIDATED, "ESC outputs blocked: validate thrust-vector linkage.");
static_assert(AIR_GEOMETRY_VALIDATED, "ESC outputs blocked: validate air geometry/allocator.");
static_assert(BODY_AXIS_MAPPING_VALIDATED, "ESC outputs blocked: validate IMU body-axis transform.");
static_assert(FAILSAFE_VALIDATED, "ESC outputs blocked: validate lost-link and sensor-fault response.");
#endif
#if RIFT_ENABLE_SERVO_OUTPUTS
static_assert(BOARD_NET_MAP_VALIDATED, "Servo outputs blocked: verify PCB nets against GPIO table.");
static_assert(SERVO_GEOMETRY_VALIDATED, "Servo outputs blocked: validate linkage endpoints.");
#endif
#if RIFT_ENABLE_ALTITUDE_HOLD
static_assert(ALTITUDE_HOLD_VALIDATED, "Altitude hold blocked: validate pressure reference/control response.");
static_assert(BODY_AXIS_MAPPING_VALIDATED, "Altitude hold blocked: validate IMU body frame.");
#endif
