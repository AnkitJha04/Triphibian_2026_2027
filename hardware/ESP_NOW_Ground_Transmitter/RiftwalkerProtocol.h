#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Shared binary protocol: keep byte-for-byte identical in the flight and bridge sketches.
static constexpr uint32_t RW_CMD_MAGIC  = 0x5257434DUL; // RWCM
static constexpr uint32_t RW_TELEM_MAGIC= 0x52575450UL; // RWTP
static constexpr uint32_t RW_TEXT_MAGIC = 0x52575458UL; // RWTX
static constexpr uint8_t  RW_PROTO_VERSION = 1;
static constexpr size_t   RW_CMD_TEXT_CAP = 80;

enum RwSensorFlags : uint16_t {
  RW_SF_IMU_OK       = 1u << 0,
  RW_SF_GYRO_CAL     = 1u << 1,
  RW_SF_BARO_OK      = 1u << 2,
  RW_SF_ALT_REF      = 1u << 3,
  RW_SF_RANGE_OK     = 1u << 4,
  RW_SF_WATER_OK     = 1u << 5,
  RW_SF_BATTERY_OK   = 1u << 6,
  RW_SF_BODY_AXES_OK = 1u << 7
};
enum RwSystemFlags : uint16_t {
  RW_SYS_ESC_OUTPUTS_ENABLED   = 1u << 0,
  RW_SYS_SERVO_OUTPUTS_ENABLED = 1u << 1,
  RW_SYS_ESPNOW_PAIRED         = 1u << 2,
  RW_SYS_MOTOR_MAP_VALID       = 1u << 3,
  RW_SYS_FAILSAFE_VALID        = 1u << 4,
  RW_SYS_HCSR04_VALID          = 1u << 5,
  RW_SYS_MAG_OK                = 1u << 6,
  RW_SYS_PREFLIGHT_PASSED      = 1u << 7
};
enum RwCommandStatus : uint8_t { RW_ACK_NONE=0, RW_ACK_ACCEPTED=1, RW_ACK_REJECTED=2 };
enum RwFailsafeState : uint8_t { RW_FS_DISARMED=0, RW_FS_ARMED=1, RW_FS_COMMAND_TIMEOUT=2, RW_FS_SENSOR_FAULT=3, RW_FS_LOOP_OVERRUN=4, RW_FS_OPERATOR_STOP=5, RW_FS_LINK_HOLD=6 };

#pragma pack(push, 1)
struct RwCommandPacket {
  uint32_t magic;
  uint8_t version;
  uint16_t sequence;
  uint8_t length;
  char text[RW_CMD_TEXT_CAP];
  uint16_t crc;
};
struct RwTelemetryPacket {
  uint32_t magic;
  uint8_t version;
  uint16_t sequence;
  uint32_t uptimeMs;
  uint32_t armedRuntimeMs;
  uint8_t armed;
  uint8_t mode; // 0 land, 1 air, 2 water (water is currently inhibited)
  uint16_t sensorFlags;
  uint16_t systemFlags;
  uint8_t imuAddr;
  uint8_t baroAddr;
  uint8_t magAddr; // zero means not configured/detected
  uint16_t escCommandUs[4];
  uint8_t servoCommandDeg[4];
  int16_t rollCdeg;       // INT16_MAX => unavailable / invalid body axes
  int16_t pitchCdeg;      // INT16_MAX => unavailable / invalid body axes
  int16_t rollTrimCdeg;   // requested trim, signed centidegrees
  int16_t pitchTrimCdeg;  // requested trim, signed centidegrees
  int16_t yawTrimCentiDps;// requested yaw-rate trim, signed centidegrees/s
  int16_t gxCentiDps;     // INT16_MAX => unavailable
  int16_t gyCentiDps;
  int16_t gzCentiDps;
  int32_t altitudeMm;     // INT32_MAX => no valid relative altitude
  int32_t pressurePa;     // -1 => unavailable
  int16_t temperatureCentiC; // INT16_MAX => unavailable
  int16_t distanceMm;     // INT16_MAX => unavailable/unvalidated wiring
  uint16_t waterRaw;      // UINT16_MAX => unavailable (GPIO42 is not ADC-capable)
  uint16_t batteryRaw;    // UINT16_MAX => unavailable/unvalidated
  uint32_t controlLoopUs;
  uint32_t loopOverruns;
  uint32_t imuAgeMs;
  uint32_t baroAgeMs;
  uint32_t rangeAgeMs;
  uint16_t lastCommandSequence;
  uint8_t lastCommandStatus;
  uint8_t failsafeState;
  char lastReason[64];
  uint16_t crc;
};
struct RwTextPacket {
  uint32_t magic;
  uint8_t version;
  uint16_t sequence;
  uint8_t length;
  char text[96];
  uint16_t crc;
};
#pragma pack(pop)

static inline uint16_t rwCrc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t b = 0; b < 8; ++b)
      crc = (crc & 0x8000u) ? static_cast<uint16_t>((crc << 1) ^ 0x1021u) : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}
static inline void rwFinishCommand(RwCommandPacket& p) {
  p.magic=RW_CMD_MAGIC; p.version=RW_PROTO_VERSION;
  if (p.length >= RW_CMD_TEXT_CAP) p.length = RW_CMD_TEXT_CAP - 1;
  p.text[p.length]='\0';
  p.crc=rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwCommandPacket, crc));
}
static inline bool rwValidCommand(const RwCommandPacket& p) {
  return p.magic==RW_CMD_MAGIC && p.version==RW_PROTO_VERSION && p.length>0 &&
         p.length<RW_CMD_TEXT_CAP && p.text[p.length]=='\0' &&
         rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwCommandPacket, crc))==p.crc;
}
static inline void rwFinishTelemetry(RwTelemetryPacket& p) {
  p.magic=RW_TELEM_MAGIC; p.version=RW_PROTO_VERSION;
  p.crc=rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwTelemetryPacket, crc));
}
static inline bool rwValidTelemetry(const RwTelemetryPacket& p) {
  return p.magic==RW_TELEM_MAGIC && p.version==RW_PROTO_VERSION &&
         rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwTelemetryPacket, crc))==p.crc;
}
static inline void rwFinishText(RwTextPacket& p) {
  p.magic=RW_TEXT_MAGIC; p.version=RW_PROTO_VERSION;
  if (p.length >= sizeof(p.text)) p.length = sizeof(p.text)-1;
  p.text[p.length]='\0';
  p.crc=rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwTextPacket, crc));
}
static inline bool rwValidText(const RwTextPacket& p) {
  return p.magic==RW_TEXT_MAGIC && p.version==RW_PROTO_VERSION &&
         p.length>0 && p.length<sizeof(p.text) && p.text[p.length]=='\0' &&
         rwCrc16(reinterpret_cast<const uint8_t*>(&p), offsetof(RwTextPacket, crc))==p.crc;
}
