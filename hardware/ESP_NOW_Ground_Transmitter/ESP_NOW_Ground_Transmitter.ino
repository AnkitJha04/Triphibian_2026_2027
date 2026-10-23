#include <Arduino.h>
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
  #error "Select an ESP32-S3 board (for example ESP32S3 Dev Module) before compiling the ground bridge."
#endif
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>
#include "RiftwalkerProtocol.h"

// Upload to a SECOND ESP32-S3. The PC app talks to this board over USB serial;
// it wraps allowed ASCII commands in a CRC-protected packet and sends ESP-NOW frames.
// Replace ROVER_MAC with the vehicle MAC printed by Actual_Fly_Control.ino.
// Configure ESPNOW_GROUND_MAC in the vehicle sketch with this board's MAC.
// Both boards must use the same fixed channel. Zero MAC means paired mode is disabled.
static constexpr uint32_t SERIAL_BAUD=115200;
static constexpr uint8_t ESPNOW_CHANNEL=1;
static constexpr uint8_t ROVER_MAC[6]={0x00,0x00,0x00,0x00,0x00,0x00};
static constexpr uint32_t INVALID_AGE_MS=0xFFFFFFFFUL;
QueueHandle_t telemetryQueue=nullptr;
QueueHandle_t textQueue=nullptr;
uint16_t txSequence=0,lastTelemetrySequence=0;
bool paired=false,telemetrySequenceSeen=false;
// Single in-flight transaction: retries reuse the same sequence so the rover's replay filter
// prevents duplicate execution, while a lost packet can still be delivered on a later attempt.
RwCommandPacket pendingPacket={};
bool pendingCommandValid=false;
uint8_t pendingRetries=0;
uint32_t pendingSentAt=0;
static constexpr uint32_t COMMAND_RETRY_MS=160;
static constexpr uint8_t MAX_COMMAND_RETRIES=3;

static bool zeroMac(const uint8_t mac[6]){uint8_t x=0;for(int i=0;i<6;i++)x|=mac[i];return x==0;}
static bool parseFiniteFloatStrict(const String& text,float& value){
  String t=text;t.trim();if(!t.length())return false;
  char* end=nullptr;const char* begin=t.c_str();const float parsed=strtof(begin,&end);
  if(end==begin||!end||*end!='\0'||!isfinite(parsed))return false;value=parsed;return true;
}
static bool validCommandText(const char* c){
  if(!c||!c[0])return false;
  const String s=String(c);String u=s;u.toUpperCase();
  if(u=="PING"||u=="STATUS"||u=="PREFLIGHT"||u=="HELP"||u=="GETPID"||u=="SAVE_PID"||u=="CAL_GYRO"||u=="BARO_ZERO"||u=="RESET_PID"||u=="ARM"||u=="START"||u=="S"||u=="STOP"||u=="DISARM"||u=="A"||u=="L"||u=="W"||u=="+"||u=="-"||u=="U"||u=="J"||u=="H"||u=="K")return true;
  const int p1=u.indexOf(',');const int p2=u.indexOf(',',p1+1);
  if(u.startsWith("TUNE,")&&p1>0&&p2>p1){const int p3=u.indexOf(',',p2+1);if(p3<0)return false;String axis=u.substring(p1+1,p2),gain=u.substring(p2+1,p3);float v=0;if(!parseFiniteFloatStrict(u.substring(p3+1),v))return false;return (axis=="ROLL_RATE"||axis=="PITCH_RATE"||axis=="YAW_RATE"||axis=="ROLL_ANGLE"||axis=="PITCH_ANGLE"||axis=="ALTITUDE")&&(gain=="KP"||gain=="KI"||gain=="KD"||gain=="IMAX"||gain=="OUTMAX")&&v>=0&&v<=100;}
  if(u.startsWith("SETPOINT,")&&p2>p1){String name=u.substring(p1+1,p2);float v=0;if(!parseFiniteFloatStrict(u.substring(p2+1),v))return false;if(name=="ROLL_DEG"||name=="PITCH_DEG")return v>=-12&&v<=12;if(name=="ROLL_TRIM_DEG"||name=="PITCH_TRIM_DEG")return v>=-4&&v<=4;if(name=="YAW_TRIM_DPS")return v>=-15&&v<=15;if(name=="YAW_RATE_DPS")return v>=-90&&v<=90;if(name=="THROTTLE")return v>=0&&v<=0.6;if(name=="ALT_M")return v>=0&&v<=5;if(name=="ALT_HOLD")return v>=0&&v<=1;}
  return false;
}
static void ackLine(const char* kind,uint16_t seq,const char* msg){Serial.printf("@ACK,%s,%u,%s\n",kind,(unsigned)seq,msg?msg:"");}
void onRecv(const esp_now_recv_info_t* info,const uint8_t* data,int len){
  if(!info||!data||zeroMac(ROVER_MAC)||memcmp(info->src_addr,ROVER_MAC,6)!=0)return;
  if(len==(int)sizeof(RwTelemetryPacket)&&telemetryQueue){
    RwTelemetryPacket p;memcpy(&p,data,sizeof(p));if(!rwValidTelemetry(p))return;
    if(telemetrySequenceSeen&&(int16_t)(p.sequence-lastTelemetrySequence)<=0)return;
    lastTelemetrySequence=p.sequence;telemetrySequenceSeen=true;(void)xQueueSend(telemetryQueue,&p,0);return;
  }
  if(len==(int)sizeof(RwTextPacket)&&textQueue){
    RwTextPacket p;memcpy(&p,data,sizeof(p));if(!rwValidText(p))return;
    (void)xQueueSend(textQueue,&p,0);return;
  }
}
static void addBool(String& j,const char* name,bool v){j+="\"";j+=name;j+="\":";j+=v?"true":"false";j+=",";}
static void addNumOrNull(String& j,const char* name,int64_t v,bool valid){j+="\"";j+=name;j+="\":";j+=valid?String((long long)v):String("null");j+=",";}
static void addFloatOrNull(String& j,const char* name,float v,bool valid,uint8_t dec=2){j+="\"";j+=name;j+="\":";j+=valid?String(v,static_cast<unsigned int>(dec)):String("null");j+=",";}
static void addAge(String& j,const char* name,uint32_t age){j+="\"";j+=name;j+="\":";j+=age==INVALID_AGE_MS?String("null"):String(age);j+=",";}
static void printTelemetry(const RwTelemetryPacket& p){
  const uint16_t sf=p.sensorFlags,sy=p.systemFlags;
  String j="@TEL,{\"transport\":\"ESP-NOW\",\"bridge_paired\":"+(paired?String("true"):String("false"))+",\"uptime_ms\":"+String(p.uptimeMs)+",\"armed_runtime_ms\":"+String(p.armedRuntimeMs)+",";
  addBool(j,"armed",p.armed!=0);j+="\"mode\":\"";j+=p.mode==1?"air":p.mode==2?"water":"land";j+="\",";
  j+="\"esc_us\":[";for(int i=0;i<4;i++){if(i)j+=",";j+=(sy& RW_SYS_ESC_OUTPUTS_ENABLED)?String(p.escCommandUs[i]):String("null");}j+="],\"esc_command_us\":[";for(int i=0;i<4;i++){if(i)j+=",";j+=String(p.escCommandUs[i]);}j+="],\"servo_deg\":[null,null,null,null],\"servo_cmd_deg\":[";for(int i=0;i<4;i++){if(i)j+=",";j+=String(p.servoCommandDeg[i]);}j+="],";
  addBool(j,"imu_ok",sf& RW_SF_IMU_OK);addBool(j,"imu_calibrated",sf& RW_SF_GYRO_CAL);addNumOrNull(j,"imu_addr",p.imuAddr,p.imuAddr!=0);addBool(j,"baro_ok",sf& RW_SF_BARO_OK);addNumOrNull(j,"baro_addr",p.baroAddr,p.baroAddr!=0);addBool(j,"altitude_reference_valid",sf& RW_SF_ALT_REF);
  const bool axisValid=sf& RW_SF_BODY_AXES_OK;addBool(j,"attitude_valid",axisValid);addBool(j,"body_axes_valid",axisValid);
  addFloatOrNull(j,"roll_deg",p.rollCdeg/100.0f,p.rollCdeg!=INT16_MAX&&axisValid);addFloatOrNull(j,"pitch_deg",p.pitchCdeg/100.0f,p.pitchCdeg!=INT16_MAX&&axisValid);
  j+="\"gyro_dps\":[";j+=p.gxCentiDps==INT16_MAX?String("null"):String(p.gxCentiDps/100.0f,2);j+=",";j+=p.gyCentiDps==INT16_MAX?String("null"):String(p.gyCentiDps/100.0f,2);j+=",";j+=p.gzCentiDps==INT16_MAX?String("null"):String(p.gzCentiDps/100.0f,2);j+="],";
  addFloatOrNull(j,"alt_m",p.altitudeMm/1000.0f,p.altitudeMm!=INT32_MAX,3);addNumOrNull(j,"pressure_pa",p.pressurePa,p.pressurePa>=0);addFloatOrNull(j,"temp_c",p.temperatureCentiC/100.0f,p.temperatureCentiC!=INT16_MAX);addFloatOrNull(j,"distance_cm",p.distanceMm/10.0f,p.distanceMm!=INT16_MAX);
  addNumOrNull(j,"water_raw",p.waterRaw,p.waterRaw!=UINT16_MAX&&(sf& RW_SF_WATER_OK));addNumOrNull(j,"battery_raw",p.batteryRaw,p.batteryRaw!=UINT16_MAX&&(sf& RW_SF_BATTERY_OK));addNumOrNull(j,"battery_v",0,false);
  j+="\"water_status\":\"UNAVAILABLE_GPIO42_NOT_ADC\",\"battery_status\":\"UNAVAILABLE_GPIO11_ADC2_WIFI_CONFLICT_OR_DIVIDER_UNCALIBRATED\",\"range_status\":\"";j+=(sy& RW_SYS_HCSR04_VALID)?"ENABLED":"UNAVAILABLE_ECHO_LEVEL_NOT_VALIDATED";j+="\",";
  addAge(j,"imu_age_ms",p.imuAgeMs);addAge(j,"baro_age_ms",p.baroAgeMs);addAge(j,"range_age_ms",p.rangeAgeMs);
  addNumOrNull(j,"loop_us",p.controlLoopUs,true);addNumOrNull(j,"loop_overruns",p.loopOverruns,true);addBool(j,"outputs_enabled",sy& RW_SYS_ESC_OUTPUTS_ENABLED);addBool(j,"servo_outputs_enabled",sy& RW_SYS_SERVO_OUTPUTS_ENABLED);addBool(j,"preflight_passed",sy& RW_SYS_PREFLIGHT_PASSED);
  j+="\"trims\":{\"roll_deg\":"+String(p.rollTrimCdeg/100.0f,2)+",\"pitch_deg\":"+String(p.pitchTrimCdeg/100.0f,2)+",\"yaw_rate_dps\":"+String(p.yawTrimCentiDps/100.0f,2)+"},";
  j+="\"mag_ok\":false,\"mag_addr\":null,\"mag_status\":\"NOT_INSTALLED_MODEL_REQUIRED\",";
  addNumOrNull(j,"failsafe_state",p.failsafeState,true);addNumOrNull(j,"last_command_seq",p.lastCommandSequence,true);addNumOrNull(j,"last_command_status",p.lastCommandStatus,true);
  j+="\"last_reason\":\"";String reason=p.lastReason;reason.replace("\\","\\\\");reason.replace("\"","\\\"");reason.replace("\n"," ");reason.replace("\r"," ");j+=reason; j+="\"}";
  Serial.println(j);
  if(pendingCommandValid&&p.lastCommandSequence==pendingPacket.sequence&&p.lastCommandStatus!=RW_ACK_NONE){
    ackLine("RX",p.lastCommandSequence,p.lastCommandStatus==RW_ACK_ACCEPTED?"ACCEPTED":"REJECTED");
    pendingCommandValid=false;pendingRetries=0;
  }
}
static bool isStopCommand(const char* c){String u=String(c?c:"");u.trim();u.toUpperCase();return u=="S"||u=="STOP"||u=="DISARM";}
static bool transmitPending(){
  if(!pendingCommandValid||!paired)return false;
  const esp_err_t e=esp_now_send(ROVER_MAC,reinterpret_cast<const uint8_t*>(&pendingPacket),sizeof(pendingPacket));
  if(e!=ESP_OK){ackLine("SENDFAIL",pendingPacket.sequence,esp_err_to_name(e));return false;}
  pendingSentAt=millis();return true;
}
static bool sendLineCommand(const char* c){
  if(!validCommandText(c)){ackLine("REJECTED",txSequence,"COMMAND_NOT_IN_ALLOWLIST");return false;}
  if(!paired||zeroMac(ROVER_MAC)){ackLine("NOPAIR",txSequence,"CONFIGURE_ROVER_MAC_AND_REBUILD");return false;}
  // Operator STOP supersedes a pending routine transaction. Other overlapping transactions
  // are rejected so acknowledgement correlation remains deterministic.
  if(pendingCommandValid&&!isStopCommand(c)){ackLine("REJECTED",pendingPacket.sequence,"BUSY_PREVIOUS_COMMAND_AWAITING_ROVER_ACK");return false;}
  if(isStopCommand(c)){pendingCommandValid=false;pendingRetries=0;}
  RwCommandPacket p={};p.sequence=++txSequence;p.length=(uint8_t)min((size_t)strlen(c),(size_t)(RW_CMD_TEXT_CAP-1));memcpy(p.text,c,p.length);p.text[p.length]='\0';rwFinishCommand(p);
  pendingPacket=p;pendingCommandValid=true;pendingRetries=0;
  if(!transmitPending()){pendingCommandValid=false;return false;}
  ackLine("SENT",p.sequence,"WAITING_FOR_ROVER_ACK");return true;
}

void setup(){
  Serial.begin(SERIAL_BAUD);delay(400);telemetryQueue=xQueueCreate(6,sizeof(RwTelemetryPacket));textQueue=xQueueCreate(12,sizeof(RwTextPacket));if(!telemetryQueue||!textQueue){Serial.println("FATAL: RX queue allocation failed; ESP-NOW telemetry/ACK relay disabled.");while(true)delay(1000);}
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);esp_wifi_set_channel(ESPNOW_CHANNEL,WIFI_SECOND_CHAN_NONE);
  Serial.printf("RIFTWALKER GROUND BRIDGE MAC: %s | channel %u\n",WiFi.macAddress().c_str(),ESPNOW_CHANNEL);
  if(esp_now_init()!=ESP_OK){Serial.println("ERROR: esp_now_init failed; no wireless commands possible.");return;}
  esp_now_register_recv_cb(onRecv);
  if(zeroMac(ROVER_MAC)){Serial.println("UNPAIRED SAFE STATE: fill ROVER_MAC with rover MAC and configure reciprocal ESPNOW_GROUND_MAC in the rover sketch.");}
  else{esp_now_peer_info_t peer={};memcpy(peer.peer_addr,ROVER_MAC,6);peer.channel=ESPNOW_CHANNEL;peer.encrypt=false;if(esp_now_add_peer(&peer)==ESP_OK){paired=true;Serial.println("Peer added. This is transport setup only, not proof of flight-control readiness.");}else Serial.println("ERROR: could not add rover peer; wireless commands disabled.");}
  Serial.println("USB serial protocol 115200. Commands are newline-framed and allowlisted. Telemetry uses CRC-checked binary ESP-NOW packets converted to @TEL JSON.");
}
void loop(){
  static char buffer[RW_CMD_TEXT_CAP];static size_t used=0;static bool overflow=false;
  while(Serial.available()>0){const char c=(char)Serial.read();if(c=='\r')continue;if(c=='\n'){if(!overflow&&used){buffer[used]='\0';sendLineCommand(buffer);}else if(overflow)ackLine("REJECTED",txSequence,"SERIAL_COMMAND_TOO_LONG");used=0;overflow=false;}else if(!overflow){if(used<RW_CMD_TEXT_CAP-1)buffer[used++]=c;else{used=0;overflow=true;}}}
  if(pendingCommandValid&&(uint32_t)(millis()-pendingSentAt)>=COMMAND_RETRY_MS){
    if(pendingRetries<MAX_COMMAND_RETRIES){pendingRetries++;if(!transmitPending()){pendingCommandValid=false;} }
    else {ackLine("TIMEOUT",pendingPacket.sequence,"NO_ROVER_APPLICATION_ACK_AFTER_RETRIES");pendingCommandValid=false;pendingRetries=0;}
  }
  RwTelemetryPacket p;if(telemetryQueue&&xQueueReceive(telemetryQueue,&p,0)==pdTRUE)printTelemetry(p);
  RwTextPacket text;for(uint8_t n=0;n<4&&textQueue&&xQueueReceive(textQueue,&text,0)==pdTRUE;n++){if(rwValidText(text))Serial.println(text.text);}
  delay(1);
}
