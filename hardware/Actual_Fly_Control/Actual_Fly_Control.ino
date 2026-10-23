#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include "RiftwalkerConfig.h"
#include "RiftwalkerProtocol.h"
#if RIFT_ENABLE_ESPNOW_RX
  #include <WiFi.h>
  #include <esp_now.h>
#endif

// RIFTWALKER controller / hardware bring-up firmware.
// All ESC and servo PWM outputs are LOCKED at compile time in this release.
// The vehicle is not flight-ready: PCB net mapping, body axes, vector linkage,
// thrust allocation, altitude hold, and the lost-link flight behaviour are not validated.
static constexpr uint32_t SERIAL_BAUD=115200;
static constexpr uint32_t CONTROL_PERIOD_MS=4;       // 250 Hz control target
static constexpr uint32_t TELEMETRY_PERIOD_MS=100;  // 10 Hz
static constexpr uint32_t COMMAND_TIMEOUT_MS=1200;
static constexpr uint32_t ARM_CONFIRM_MS=8000;
static constexpr float GYRO_LSB_PER_DPS=65.5f;       // MPU6050 ±500 °/s
static constexpr float ACCEL_LSB_PER_G=8192.0f;      // MPU6050 ±4 g
static constexpr float MAX_ATTITUDE_DEG=12.0f;
static constexpr float MAX_RATE_DPS=90.0f;
static constexpr uint8_t MPU_REG_ACCEL_START=0x3B;
static constexpr uint32_t INVALID_AGE_MS=0xFFFFFFFFUL;

enum Mode:uint8_t { MODE_LAND=0, MODE_AIR=1, MODE_WATER=2 };
struct PIDGains { float kp,ki,kd,imax,outMax; };
struct PIDState { float integral=0, previous=0; bool hasPrevious=false; };
struct SensorState {
  bool imuOk=false, baroOk=false;
  uint8_t imuAddr=0,baroAddr=0;
  float rollDeg=NAN,pitchDeg=NAN,gx=NAN,gy=NAN,gz=NAN;
  float altitudeM=NAN,pressurePa=NAN,tempC=NAN,distanceCm=NAN,batteryV=NAN;
  uint16_t waterRaw=UINT16_MAX,batteryRaw=UINT16_MAX;
  uint32_t imuAgeMs=INVALID_AGE_MS,baroAgeMs=INVALID_AGE_MS,rangeAgeMs=INVALID_AGE_MS;
};
struct TelemetryState {
  float rollTarget=0,pitchTarget=0,yawRateTarget=0,throttle=0;
  float rollRateOut=0,pitchRateOut=0,yawRateOut=0;
  float rollTrimDeg=0,pitchTrimDeg=0,yawTrimDps=0;
  uint32_t loopUs=0,loopOverruns=0;
};
struct CommandMsg { char text[RW_CMD_TEXT_CAP]; uint16_t sequence=0; bool viaNow=false; };
static constexpr size_t RIFT_RADIO_PAYLOAD_CAP = sizeof(RwTelemetryPacket) > sizeof(RwTextPacket) ? sizeof(RwTelemetryPacket) : sizeof(RwTextPacket);
struct RadioTxMessage { uint16_t length=0; uint8_t payload[RIFT_RADIO_PAYLOAD_CAP]; };
struct SerialMessage { char text[256]; };

// These are intentionally conservative bench-start values, NOT validated flight gains.
// They must not be treated as tuned for this 2 kg vehicle merely because mass is known.
PIDGains rollRateG{0.035f,0.004f,0.0002f,8.0f,0.08f};
PIDGains pitchRateG{0.035f,0.004f,0.0002f,8.0f,0.08f};
PIDGains yawRateG{0.030f,0.002f,0.0000f,5.0f,0.06f};
PIDGains rollAngleG{3.0f,0.0f,0.0f,5.0f,35.0f};
PIDGains pitchAngleG{3.0f,0.0f,0.0f,5.0f,35.0f};
PIDGains altitudeG{0.12f,0.0f,0.0f,0.0f,0.10f}; // inactive: ALTITUDE_HOLD_VALIDATED=false
PIDState rollRateS,pitchRateS,yawRateS,rollAngleS,pitchAngleS,altitudeS;

Servo esc[4],vectorServo[4];
Preferences prefs;
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t i2cMutex=nullptr, serialMutex=nullptr;
QueueHandle_t commandQueue=nullptr;
QueueHandle_t radioTxQueue=nullptr;
QueueHandle_t serialTxQueue=nullptr;
SensorState sensors;
TelemetryState ctl;
volatile bool armed=false,armPermit=false,stopLatched=true,altHold=false,pidResetPending=false;
volatile Mode currentMode=MODE_LAND;
volatile uint32_t armPermitAt=0,lastCommandAt=0,armedStartedAt=0,armedAccumMs=0;
volatile bool imuCalibrated=false,sensorFault=true,pendingLand=false,pendingWater=false;
volatile bool emergencyStopRequested=false;
volatile bool linkLossHold=false;
volatile uint32_t linkLossStartedAt=0;
volatile bool escAttachOk=false,servoAttachOk=false,rtosTasksReady=false;
volatile uint8_t failsafeState=RW_FS_DISARMED;
float gyroOffX=0,gyroOffY=0,gyroOffZ=0,groundPressurePa=NAN,altitudeTargetM=0;
int escCommandUs[4]={ESC_MIN_US,ESC_MIN_US,ESC_MIN_US,ESC_MIN_US};
int servoCommandDeg[4]={5,0,0,5};
char lastReason[96]="SAFE BOOT: ESC + SERVO OUTPUTS LOCKED";
uint8_t imuAddr=0,baroAddr=0;
uint32_t lastImuSampleMs=0,lastBaroSampleMs=0,lastRangeSampleMs=0;
uint16_t lastProcessedCommandSeq=0;
uint8_t lastProcessedCommandStatus=RW_ACK_NONE;
uint16_t telemetrySequence=0;
volatile bool activeCommandViaNow=false;
volatile uint16_t activeRemoteSequence=0;
#if RIFT_ENABLE_ESPNOW_RX
bool nowPeerReady=false;
volatile bool nowTxTaskReady=false;
volatile uint16_t lastNowRxSeq=0;
volatile bool nowRxSeqSeen=false;
uint16_t lastNowTelemSeq=0;
volatile uint32_t radioTxDrops=0;
#endif

static inline float clampf(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
static inline bool finiteV(float v){return isfinite(v);}
static bool zeroMac(const uint8_t mac[6]){uint8_t v=0;for(int i=0;i<6;i++)v|=mac[i];return v==0;}
static void setReason(const char* s){portENTER_CRITICAL(&stateMux);strncpy(lastReason,s?s:"",sizeof(lastReason)-1);lastReason[sizeof(lastReason)-1]='\0';portEXIT_CRITICAL(&stateMux);}
#if RIFT_ENABLE_ESPNOW_RX
static bool queueRadioFrame(const uint8_t* data,size_t length){
  if(!nowPeerReady||!nowTxTaskReady||!radioTxQueue||!data||length==0||length>RIFT_RADIO_PAYLOAD_CAP)return false;
  RadioTxMessage msg={};msg.length=(uint16_t)length;memcpy(msg.payload,data,length);
  if(xQueueSend(radioTxQueue,&msg,0)!=pdTRUE){radioTxDrops++;return false;}
  return true;
}
static void sendRemoteText(const String& s){
  if(!activeCommandViaNow||!nowPeerReady||!nowTxTaskReady||zeroMac(ESPNOW_GROUND_MAC))return;
  RwTextPacket p={};p.sequence=activeRemoteSequence;
  p.length=(uint8_t)min((size_t)s.length(),sizeof(p.text)-1);
  memcpy(p.text,s.c_str(),p.length);p.text[p.length]='\0';rwFinishText(p);
  (void)queueRadioFrame(reinterpret_cast<const uint8_t*>(&p),sizeof(p));
}
#else
static bool queueRadioFrame(const uint8_t*,size_t){return false;}
static void sendRemoteText(const String&){}
#endif
static void printLineNonBlocking(const String& s){
  // No serial mutex waits or UART writes in command/control paths. Text is queued for a
  // low-priority TX task; the same message is queued for ESP-NOW when this was a radio command.
  if(serialTxQueue){SerialMessage msg={};s.toCharArray(msg.text,sizeof(msg.text));
    if(xQueueSend(serialTxQueue,&msg,0)!=pdTRUE&&s.startsWith("DISARMED")) (void)xQueueSendToFront(serialTxQueue,&msg,0);
  }
  sendRemoteText(s);
}
static bool asciiWordEqual(const char* a,const char* b){
  if(!a||!b)return false;
  while(*a&&*b){char x=*a++,y=*b++;if(x>='a'&&x<='z')x=(char)(x-'a'+'A');if(y>='a'&&y<='z')y=(char)(y-'a'+'A');if(x!=y)return false;}
  return *a=='\0'&&*b=='\0';
}
static bool isStopCommandText(const char* text){
  if(!text)return false;
  while(*text==' '||*text=='\t')text++;
  char token[16]={0};size_t n=0;
  while(*text&&*text!=' '&&*text!='\t'&&n<sizeof(token)-1)token[n++]=*text++;
  while(*text==' '||*text=='\t')text++;
  if(*text!='\0')return false;
  return asciiWordEqual(token,"S")||asciiWordEqual(token,"STOP")||asciiWordEqual(token,"DISARM");
}
void report(const String& s){setReason(s.c_str());printLineNonBlocking(s);}

bool i2cWrite(uint8_t addr,uint8_t reg,uint8_t value){if(i2cMutex)xSemaphoreTake(i2cMutex,portMAX_DELAY);Wire.beginTransmission(addr);Wire.write(reg);Wire.write(value);bool ok=Wire.endTransmission()==0;if(i2cMutex)xSemaphoreGive(i2cMutex);return ok;}
bool i2cRead(uint8_t addr,uint8_t reg,uint8_t* data,size_t n){if(i2cMutex)xSemaphoreTake(i2cMutex,portMAX_DELAY);Wire.beginTransmission(addr);Wire.write(reg);if(Wire.endTransmission(false)!=0){if(i2cMutex)xSemaphoreGive(i2cMutex);return false;}size_t got=Wire.requestFrom((int)addr,(int)n,(int)true);if(got!=n){if(i2cMutex)xSemaphoreGive(i2cMutex);return false;}for(size_t i=0;i<n;i++)data[i]=Wire.read();if(i2cMutex)xSemaphoreGive(i2cMutex);return true;}
bool i2cPresent(uint8_t addr){if(i2cMutex)xSemaphoreTake(i2cMutex,portMAX_DELAY);Wire.beginTransmission(addr);bool ok=Wire.endTransmission()==0;if(i2cMutex)xSemaphoreGive(i2cMutex);return ok;}

void setEscOutputs(const int values[4]){
  for(int i=0;i<4;i++){
    escCommandUs[i]=constrain(values[i],ESC_MIN_US,ESC_MAX_US);
#if RIFT_ENABLE_ACTUATOR_OUTPUTS
    if(esc[i].attached())esc[i].writeMicroseconds(escCommandUs[i]);
#else
    escCommandUs[i]=ESC_MIN_US; // command record held safe; no ESC GPIO PWM is attached
#endif
  }
}
void outputsMinimum(){const int v[4]={ESC_MIN_US,ESC_MIN_US,ESC_MIN_US,ESC_MIN_US};setEscOutputs(v);}
bool outputsAtMinimum(){for(int i=0;i<4;i++)if(escCommandUs[i]>ESC_MIN_US)return false;return true;}
void writeServosForMode(Mode m){
  // Water geometry is deliberately unspecified and applyMode rejects MODE_WATER.
  // Keep this fallback at a known land-safe target so the sketch remains buildable
  // without inventing a water-servo configuration.
  const int* target=(m==MODE_AIR)?AIR_SERVO_DEG:LAND_SERVO_DEG;
  for(int i=0;i<4;i++){
    servoCommandDeg[i]=constrain(target[i],0,180);
#if RIFT_ENABLE_SERVO_OUTPUTS
    if(vectorServo[i].attached())vectorServo[i].write(servoCommandDeg[i]);
#endif
  }
}
void disarmNow(const char* why){
  outputsMinimum();if(armed)armedAccumMs+=millis()-armedStartedAt;armed=false;armPermit=false;stopLatched=true;portENTER_CRITICAL(&stateMux);ctl.throttle=0;linkLossHold=false;linkLossStartedAt=0;portEXIT_CRITICAL(&stateMux);altHold=false;
  failsafeState=(why&&strstr(why,"TIMEOUT"))?RW_FS_COMMAND_TIMEOUT:((why&&strstr(why,"FAULT"))?RW_FS_SENSOR_FAULT:RW_FS_OPERATOR_STOP);
  String msg=String("DISARMED: ")+(why?why:"unspecified");setReason(msg.c_str());printLineNonBlocking(msg);
}

// PING is a liveness signal only. It does not change throttle, attitude targets or trims.
// A recovered link resumes normal closed-loop control from the same bounded command state.
void noteControlHeartbeat(){
  const uint32_t now=millis();
  bool recovered=false;
  portENTER_CRITICAL(&stateMux);
  lastCommandAt=now;
  if(linkLossHold){linkLossHold=false;linkLossStartedAt=0;recovered=armed;if(armed)failsafeState=RW_FS_ARMED;}
  portEXIT_CRITICAL(&stateMux);
  if(recovered)setReason("LINK RESTORED: closed-loop attitude control resumed.");
}

// MPU6050 init and read. Preferred address 0x68; try 0x69 only if 0x68 doesn't respond.
bool initMPU(){if(i2cPresent(MPU6050_ADDR_A))imuAddr=MPU6050_ADDR_A;else if(i2cPresent(MPU6050_ADDR_B))imuAddr=MPU6050_ADDR_B;else return false;
  if(!i2cWrite(imuAddr,0x6B,0x00))return false;delay(50);
  if(!i2cWrite(imuAddr,0x1B,0x08))return false; // ±500 deg/s
  if(!i2cWrite(imuAddr,0x1C,0x08))return false; // ±4 g
  if(!i2cWrite(imuAddr,0x1A,0x03))return false; // DLPF
  if(!i2cWrite(imuAddr,0x19,0x03))return false; // 250 Hz
  return true;
}
bool readMPU(int16_t& ax,int16_t& ay,int16_t& az,int16_t& gx,int16_t& gy,int16_t& gz){
  uint8_t b[14];if(!imuAddr||!i2cRead(imuAddr,MPU_REG_ACCEL_START,b,14))return false;
  ax=(int16_t)((uint16_t(b[0])<<8)|b[1]);ay=(int16_t)((uint16_t(b[2])<<8)|b[3]);az=(int16_t)((uint16_t(b[4])<<8)|b[5]);
  gx=(int16_t)((uint16_t(b[8])<<8)|b[9]);gy=(int16_t)((uint16_t(b[10])<<8)|b[11]);gz=(int16_t)((uint16_t(b[12])<<8)|b[13]);return true;
}

// Require stillness: reject if acceleration magnitude or gyro variation indicates motion.
bool calibrateGyro(){
  if(!sensors.imuOk){report("CAL_GYRO FAILED: MPU6050 is not responding.");return false;}
  constexpr int N=250;int good=0;double sx=0,sy=0,sz=0,sxx=0,syy=0,szz=0;bool still=true;
  for(int i=0;i<N;i++){
    int16_t ax,ay,az,gx,gy,gz;
    if(!readMPU(ax,ay,az,gx,gy,gz)){still=false;break;}
    const float gmag=sqrtf((float)ax*ax+(float)ay*ay+(float)az*az)/ACCEL_LSB_PER_G;
    if(gmag<0.80f||gmag>1.20f)still=false;
    sx+=gx;sy+=gy;sz+=gz;sxx+=(double)gx*gx;syy+=(double)gy*gy;szz+=(double)gz*gz;good++;
    vTaskDelay(pdMS_TO_TICKS(4));
  }
  if(good<N){imuCalibrated=false;report("CAL_GYRO FAILED: incomplete sensor samples.");return false;}
  const double mx=sx/N,my=sy/N,mz=sz/N;
  const double vx=sxx/N-mx*mx,vy=syy/N-my*my,vz=szz/N-mz*mz;
  if(!still||vx>180.0||vy>180.0||vz>180.0){imuCalibrated=false;report("CAL_GYRO REJECTED: motion/tilt detected. Place vehicle still and level, then retry.");return false;}
  gyroOffX=(float)mx;gyroOffY=(float)my;gyroOffZ=(float)mz;imuCalibrated=true;sensorFault=false;report("CAL_GYRO OK: stationary bias stored in RAM; not a flight-frame validation.");return true;
}

// MS5607 datasheet compensation; CRC-4 PROM check; 0x76 preferred, then 0x77.
uint16_t prom[8]={0};
bool ms5607PromCrcValid(){uint16_t saved=prom[7];uint16_t expected=saved&0x000F;prom[7]=saved&0xFF00;uint16_t rem=0;for(int cnt=0;cnt<16;cnt++){rem^=(cnt&1)?(prom[cnt>>1]&0x00FF):(prom[cnt>>1]>>8);for(int bit=0;bit<8;bit++)rem=(rem&0x8000)?(uint16_t)((rem<<1)^0x3000):(uint16_t)(rem<<1);}prom[7]=saved;return ((rem>>12)&0x0F)==expected;}
bool msResetAndReadPROM(uint8_t addr){if(i2cMutex)xSemaphoreTake(i2cMutex,portMAX_DELAY);Wire.beginTransmission(addr);Wire.write(0x1E);bool sent=Wire.endTransmission()==0;if(i2cMutex)xSemaphoreGive(i2cMutex);if(!sent)return false;vTaskDelay(pdMS_TO_TICKS(4));for(int i=0;i<8;i++){uint8_t b[2];if(!i2cRead(addr,0xA0+2*i,b,2))return false;prom[i]=(uint16_t(b[0])<<8)|b[1];}if(!(prom[1]&&prom[2]&&prom[3]&&prom[4]&&prom[5]&&prom[6]))return false;return ms5607PromCrcValid();}
uint32_t msReadADC(uint8_t addr){uint8_t b[3];if(!i2cRead(addr,0x00,b,3))return 0;return (uint32_t(b[0])<<16)|(uint32_t(b[1])<<8)|b[2];}
bool msConvert(uint8_t addr,uint8_t cmd,uint32_t& out){if(i2cMutex)xSemaphoreTake(i2cMutex,portMAX_DELAY);Wire.beginTransmission(addr);Wire.write(cmd);bool ok=Wire.endTransmission()==0;if(i2cMutex)xSemaphoreGive(i2cMutex);if(!ok)return false;vTaskDelay(pdMS_TO_TICKS(10));out=msReadADC(addr);return out>0;}
bool initMS5607(){if(i2cPresent(MS5607_ADDR_A)&&msResetAndReadPROM(MS5607_ADDR_A)){baroAddr=MS5607_ADDR_A;return true;}if(i2cPresent(MS5607_ADDR_B)&&msResetAndReadPROM(MS5607_ADDR_B)){baroAddr=MS5607_ADDR_B;return true;}return false;}
bool readMS5607(float& pressurePa,float& tempC){uint32_t D1,D2;if(!baroAddr||!msConvert(baroAddr,0x48,D1)||!msConvert(baroAddr,0x58,D2))return false;
  int64_t dT=(int64_t)D2-((int64_t)prom[5]<<8);int64_t TEMP=2000+(dT*(int64_t)prom[6])/8388608LL;
  int64_t OFF=((int64_t)prom[2]<<17)+((int64_t)prom[4]*dT)/64LL;int64_t SENS=((int64_t)prom[1]<<16)+((int64_t)prom[3]*dT)/128LL;
  int64_t T2=0,OFF2=0,SENS2=0;if(TEMP<2000){T2=(dT*dT)>>31;int64_t t=TEMP-2000;OFF2=61*t*t/16;SENS2=2*t*t;if(TEMP< -1500){int64_t t2=TEMP+1500;OFF2+=15*t2*t2;SENS2+=8*t2*t2;}}
  TEMP-=T2;OFF-=OFF2;SENS-=SENS2;int64_t P=(((int64_t)D1*SENS/2097152LL)-OFF)/32768LL;pressurePa=(float)P;tempC=(float)TEMP/100.0f;return pressurePa>10000&&pressurePa<120000;}

float pidStep(PIDGains& g,PIDState& s,float sp,float measured,float dt){if(!finiteV(dt)||dt<=0||dt>0.05f||!finiteV(sp)||!finiteV(measured))return 0;float e=sp-measured;s.integral=clampf(s.integral+e*dt,-g.imax,g.imax);float d=0;if(s.hasPrevious)d=-(measured-s.previous)/dt;s.previous=measured;s.hasPrevious=true;return clampf(g.kp*e+g.ki*s.integral+g.kd*d,-g.outMax,g.outMax);}
void resetPIDStates(){rollRateS=PIDState{};pitchRateS=PIDState{};yawRateS=PIDState{};rollAngleS=PIDState{};pitchAngleS=PIDState{};altitudeS=PIDState{};}
void requestPIDReset(){portENTER_CRITICAL(&stateMux);pidResetPending=true;portEXIT_CRITICAL(&stateMux);}
bool parseFiniteFloatStrict(const String& text,float& value){String t=text;t.trim();if(!t.length())return false;char* end=nullptr;const char* begin=t.c_str();float parsed=strtof(begin,&end);if(end==begin||!end||*end!='\0'||!finiteV(parsed))return false;value=parsed;return true;}
void updateAttitude(){
  static uint64_t lastUs=0;const uint64_t now=esp_timer_get_time();float dt=lastUs?((now-lastUs)/1e6f):0.004f;lastUs=now;dt=clampf(dt,0.001f,0.02f);
  int16_t axr,ayr,azr,gxr,gyr,gzr;if(!readMPU(axr,ayr,azr,gxr,gyr,gzr)){portENTER_CRITICAL(&stateMux);sensors.imuOk=false;sensors.imuAgeMs=INVALID_AGE_MS;portEXIT_CRITICAL(&stateMux);sensorFault=true;return;}
  const float ax=axr/ACCEL_LSB_PER_G,ay=ayr/ACCEL_LSB_PER_G,az=azr/ACCEL_LSB_PER_G;
  const float gx=(gxr-gyroOffX)/GYRO_LSB_PER_DPS,gy=(gyr-gyroOffY)/GYRO_LSB_PER_DPS,gz=(gzr-gyroOffZ)/GYRO_LSB_PER_DPS;
  const float accRoll=atan2f(ay,az)*180.0f/PI;const float accPitch=atan2f(-ax,sqrtf(ay*ay+az*az))*180.0f/PI;
  float oldRoll=NAN,oldPitch=NAN;portENTER_CRITICAL(&stateMux);oldRoll=sensors.rollDeg;oldPitch=sensors.pitchDeg;portEXIT_CRITICAL(&stateMux);
  const float newRoll=finiteV(oldRoll)?0.985f*(oldRoll+gx*dt)+0.015f*accRoll:accRoll;
  const float newPitch=finiteV(oldPitch)?0.985f*(oldPitch+gy*dt)+0.015f*accPitch:accPitch;
  portENTER_CRITICAL(&stateMux);sensors.rollDeg=newRoll;sensors.pitchDeg=newPitch;sensors.gx=gx;sensors.gy=gy;sensors.gz=gz;sensors.imuOk=true;sensors.imuAgeMs=0;portEXIT_CRITICAL(&stateMux);
  lastImuSampleMs=millis();sensorFault=!imuCalibrated;
}

void sensorTask(void*){TickType_t last=xTaskGetTickCount();for(;;){updateAttitude();const uint32_t now=millis();portENTER_CRITICAL(&stateMux);if(lastImuSampleMs)sensors.imuAgeMs=now-lastImuSampleMs;else sensors.imuAgeMs=INVALID_AGE_MS; // water GPIO42 is not ADC-capable; never analogRead it.
    sensors.waterRaw=UINT16_MAX;
    // Battery measurement remains unavailable: GPIO11 is ADC2 and conflicts with Wi-Fi/ESP-NOW; divider ratio is uncalibrated.
    sensors.batteryRaw=UINT16_MAX;sensors.batteryV=NAN;
    portEXIT_CRITICAL(&stateMux);vTaskDelayUntil(&last,pdMS_TO_TICKS(CONTROL_PERIOD_MS));}}
void baroTask(void*){for(;;){float p,t;if(baroAddr&&readMS5607(p,t)){portENTER_CRITICAL(&stateMux);sensors.pressurePa=p;sensors.tempC=t;sensors.baroOk=true;sensors.baroAgeMs=0;if(!finiteV(groundPressurePa)&&currentMode==MODE_LAND&&!armed)groundPressurePa=p;if(finiteV(groundPressurePa)&&groundPressurePa>10000)sensors.altitudeM=44330.0f*(1.0f-powf(p/groundPressurePa,0.19029495f));else sensors.altitudeM=NAN;portEXIT_CRITICAL(&stateMux);lastBaroSampleMs=millis();}else{portENTER_CRITICAL(&stateMux);sensors.baroOk=false;sensors.pressurePa=NAN;sensors.tempC=NAN;sensors.altitudeM=NAN;portEXIT_CRITICAL(&stateMux);}vTaskDelay(pdMS_TO_TICKS(35));}}
void rangeTask(void*){
#if RIFT_ENABLE_AUTO_TRANSITIONS && HCSR04_LEVEL_SHIFT_VALIDATED
  pinMode(HC_SR04_TRIG_PIN,OUTPUT);pinMode(HC_SR04_ECHO_PIN,INPUT);digitalWrite(HC_SR04_TRIG_PIN,LOW);
  for(;;){digitalWrite(HC_SR04_TRIG_PIN,LOW);ets_delay_us(3);digitalWrite(HC_SR04_TRIG_PIN,HIGH);ets_delay_us(10);digitalWrite(HC_SR04_TRIG_PIN,LOW);const unsigned long us=pulseIn(HC_SR04_ECHO_PIN,HIGH,18000);const float cm=us?us*0.0343f/2.0f:NAN;portENTER_CRITICAL(&stateMux);sensors.distanceCm=cm;portEXIT_CRITICAL(&stateMux);lastRangeSampleMs=millis();vTaskDelay(pdMS_TO_TICKS(100));}
#else
  for(;;){portENTER_CRITICAL(&stateMux);sensors.distanceCm=NAN;sensors.rangeAgeMs=INVALID_AGE_MS;portEXIT_CRITICAL(&stateMux);vTaskDelay(pdMS_TO_TICKS(500));}
#endif
}

bool preflightHardwareReady(){
  return RIFT_ENABLE_ACTUATOR_OUTPUTS && RIFT_ENABLE_SERVO_OUTPUTS && escAttachOk && servoAttachOk && rtosTasksReady &&
         BOARD_NET_MAP_VALIDATED && MOTOR_ORDER_VALIDATED && SERVO_GEOMETRY_VALIDATED && AIR_GEOMETRY_VALIDATED &&
         BODY_AXIS_MAPPING_VALIDATED && FAILSAFE_VALIDATED && ALTITUDE_HOLD_VALIDATED && sensors.imuOk && imuCalibrated &&
         sensors.baroOk && finiteV(groundPressurePa) && HCSR04_LEVEL_SHIFT_VALIDATED && RIFT_ENABLE_AUTO_TRANSITIONS;
}
String preflightReport(){
  SensorState s;portENTER_CRITICAL(&stateMux);s=sensors;portEXIT_CRITICAL(&stateMux);
  String v="PREFLIGHT ";v+=preflightHardwareReady()?"READY":"BLOCKED";
  v+=" | MPU=";v+=s.imuOk?"OK":"MISSING";v+="/gyro=";v+=imuCalibrated?"CAL":"UNCAL";
  v+=" | MS5607=";v+=s.baroOk?"OK":"MISSING";v+="/ref=";v+=finiteV(groundPressurePa)?"SET":"MISSING";
  v+=" | IMU body axes=";v+=BODY_AXIS_MAPPING_VALIDATED?"VALIDATED":"UNVALIDATED";
  v+=" | PCB net map=";v+=BOARD_NET_MAP_VALIDATED?"VALIDATED":"UNVALIDATED (typed pin list differs from schematic labels)";
  v+=" | ESC/servo outputs=";v+=(RIFT_ENABLE_ACTUATOR_OUTPUTS&&RIFT_ENABLE_SERVO_OUTPUTS)?"ENABLED":"COMPILE-TIME LOCKED";
  v+=" | HC-SR04=";v+=HCSR04_LEVEL_SHIFT_VALIDATED?"VALIDATED":"UNAVAILABLE (ECHO circuit unverified)";
  v+=" | water=";v+="UNAVAILABLE (GPIO42 has no ADC)";
  v+=" | battery=";v+="UNAVAILABLE (GPIO11 ADC2 + ESP-NOW; divider not calibrated)";
  v+=" | magnetometer=NOT INSTALLED/UNKNOWN MODEL";
  v+=" | land/water actuator mechanism=NOT VALIDATED";
  v+=" | lost-link auto-land=NOT VALIDATED; never guaranteed crash-free";
  return v;
}

// Inputs are converted to body-axis torque demands only after the body frame and mixer are validated.
// This release never sends these requested values to the physical ESCs because the build gates are false.
void controlTask(void*){TickType_t last=xTaskGetTickCount();uint32_t previousUs=micros();for(;;){const uint32_t start=micros();float dt=(start-previousUs)/1e6f;previousUs=start;if(dt<=0||dt>0.03f)dt=CONTROL_PERIOD_MS/1000.0f;
    SensorState s;TelemetryState c;bool doPidReset=false;portENTER_CRITICAL(&stateMux);s=sensors;c=ctl;doPidReset=pidResetPending;pidResetPending=false;portEXIT_CRITICAL(&stateMux);if(doPidReset)resetPIDStates();
    const bool healthy=s.imuOk&&imuCalibrated&&!sensorFault&&s.imuAgeMs<50;
    bool stopNow=false;
    portENTER_CRITICAL(&stateMux);if(emergencyStopRequested){emergencyStopRequested=false;stopNow=true;}portEXIT_CRITICAL(&stateMux);
    const uint32_t nowMs=millis();
    if(stopNow&&(armed||armPermit))disarmNow("OPERATOR STOP (priority path)");
    if(armed&&!healthy)disarmNow("IMU SENSOR FAULT");
    bool holdingForLink=false;uint32_t holdStartedAt=0;
    portENTER_CRITICAL(&stateMux);holdingForLink=linkLossHold;holdStartedAt=linkLossStartedAt;portEXIT_CRITICAL(&stateMux);
    if(armed&&healthy&&(uint32_t)(nowMs-lastCommandAt)>COMMAND_TIMEOUT_MS){
      if(!holdingForLink){
        portENTER_CRITICAL(&stateMux);linkLossHold=true;linkLossStartedAt=nowMs;failsafeState=RW_FS_LINK_HOLD;portEXIT_CRITICAL(&stateMux);
        setReason("LINK LOSS: bounded attitude-stabilization hold active; awaiting heartbeat.");
        holdingForLink=true;holdStartedAt=nowMs;
      }
      if((uint32_t)(nowMs-holdStartedAt)>LINK_LOSS_ATTITUDE_HOLD_MS)disarmNow("COMMAND TIMEOUT AFTER STABILIZATION HOLD");
    }
    if(!healthy)outputsMinimum();
    else if(armed){
      const float rollRateSp=clampf(pidStep(rollAngleG,rollAngleS,clampf(c.rollTarget+c.rollTrimDeg,-MAX_ATTITUDE_DEG,MAX_ATTITUDE_DEG),s.rollDeg,dt),-MAX_RATE_DPS,MAX_RATE_DPS);
      const float pitchRateSp=clampf(pidStep(pitchAngleG,pitchAngleS,clampf(c.pitchTarget+c.pitchTrimDeg,-MAX_ATTITUDE_DEG,MAX_ATTITUDE_DEG),s.pitchDeg,dt),-MAX_RATE_DPS,MAX_RATE_DPS);
      const float y=pidStep(yawRateG,yawRateS,clampf(c.yawRateTarget+c.yawTrimDps,-MAX_RATE_DPS,MAX_RATE_DPS),s.gz,dt);
      const float r=pidStep(rollRateG,rollRateS,rollRateSp,s.gx,dt),p=pidStep(pitchRateG,pitchRateS,pitchRateSp,s.gy,dt);
      float base=clampf(c.throttle,0,0.6f);
#if RIFT_ENABLE_ALTITUDE_HOLD
      if(altHold&&s.baroOk&&finiteV(s.altitudeM))base=clampf(base+pidStep(altitudeG,altitudeS,altitudeTargetM,s.altitudeM,dt),0,0.6f);
#endif
      // Signs are NOT validated: rotor rotation + variable thrust-vector servo geometry must be measured.
      const float out[4]={base-r*0.5f-p*0.5f-y*0.25f,base-r*0.5f+p*0.5f+y*0.25f,base+r*0.5f-p*0.5f+y*0.25f,base+r*0.5f+p*0.5f-y*0.25f};int pulse[4];
      for(int i=0;i<4;i++)pulse[i]=(int)lroundf(ESC_MIN_US+clampf(out[i],0,0.6f)*(ESC_MAX_US-ESC_MIN_US));
if(RIFT_ENABLE_ACTUATOR_OUTPUTS && RIFT_ENABLE_SERVO_OUTPUTS && BOARD_NET_MAP_VALIDATED && MOTOR_ORDER_VALIDATED && SERVO_GEOMETRY_VALIDATED && AIR_GEOMETRY_VALIDATED && BODY_AXIS_MAPPING_VALIDATED && FAILSAFE_VALIDATED) {
        if(currentMode==MODE_AIR&&healthy)setEscOutputs(pulse);else outputsMinimum();
      } else outputsMinimum();
      portENTER_CRITICAL(&stateMux);ctl.rollRateOut=r;ctl.pitchRateOut=p;ctl.yawRateOut=y;portEXIT_CRITICAL(&stateMux);
    }else{outputsMinimum();resetPIDStates();}
    const uint32_t elapsed=micros()-start;portENTER_CRITICAL(&stateMux);ctl.loopUs=elapsed;if(elapsed>CONTROL_PERIOD_MS*1000UL)ctl.loopOverruns++;portEXIT_CRITICAL(&stateMux);
    vTaskDelayUntil(&last,pdMS_TO_TICKS(CONTROL_PERIOD_MS));
  }}


void setFiniteJson(String& line,const char* key,float val,uint8_t decimals=2){line+="\"";line+=key;line+="\":";line+=finiteV(val)?String(val,static_cast<unsigned int>(decimals)):String("null");line+=",";}
void setAgeJson(String& line,const char* key,uint32_t age){line+="\"";line+=key;line+="\":";line+=age==INVALID_AGE_MS?String("null"):String(age);line+=",";}
void emitTelemetry(){
  SensorState s;TelemetryState c;char reason[96];uint16_t ackSeq;uint8_t ackStatus;portENTER_CRITICAL(&stateMux);s=sensors;c=ctl;ackSeq=lastProcessedCommandSeq;ackStatus=lastProcessedCommandStatus;strncpy(reason,lastReason,sizeof(reason)-1);reason[sizeof(reason)-1]='\0';portEXIT_CRITICAL(&stateMux);
  const bool escGate=RIFT_ENABLE_ACTUATOR_OUTPUTS&&escAttachOk&&rtosTasksReady&&BOARD_NET_MAP_VALIDATED&&MOTOR_ORDER_VALIDATED&&AIR_GEOMETRY_VALIDATED&&BODY_AXIS_MAPPING_VALIDATED&&FAILSAFE_VALIDATED;
  const bool servoGate=RIFT_ENABLE_SERVO_OUTPUTS&&servoAttachOk&&rtosTasksReady&&BOARD_NET_MAP_VALIDATED&&SERVO_GEOMETRY_VALIDATED;
  const bool preflight=preflightHardwareReady();
  String line="@TEL,{\"uptime_ms\":"+String(millis())+",\"armed_runtime_ms\":"+String(armedAccumMs+(armed?millis()-armedStartedAt:0));
  line+=",\"armed\":"+(armed?String("true"):String("false"));line+=",\"mode\":\""+String(currentMode==MODE_AIR?"air":currentMode==MODE_WATER?"water":"land")+"\"";
  line+=",\"pending_mode\":\"none\",\"esc_us\":[";
  for(int i=0;i<4;i++){if(i)line+=",";line+=escGate?String(escCommandUs[i]):String("null");}line+="],\"esc_command_us\":[";
  for(int i=0;i<4;i++){if(i)line+=",";line+=String(escCommandUs[i]);}line+="],\"servo_deg\":[null,null,null,null],\"servo_cmd_deg\":[";
  for(int i=0;i<4;i++){if(i)line+=",";line+=String(servoCommandDeg[i]);}line+="]";
  line+=",\"imu_ok\":"+(s.imuOk?String("true"):String("false"))+",\"imu_calibrated\":"+(imuCalibrated?String("true"):String("false"))+",\"imu_addr\":"+String(imuAddr);
  line+=",\"baro_addr\":"+String(baroAddr)+",\"baro_ok\":"+(s.baroOk?String("true"):String("false"))+",\"altitude_reference_valid\":"+(finiteV(groundPressurePa)?String("true"):String("false"));
  line+=",\"attitude_valid\":"+(BODY_AXIS_MAPPING_VALIDATED?String("true"):String("false"))+",\"body_axes_valid\":"+(BODY_AXIS_MAPPING_VALIDATED?String("true"):String("false"));
  line+=",\"attitude_status\":\""+String(BODY_AXIS_MAPPING_VALIDATED?"BODY_FRAME_VALIDATED":"RAW_ESTIMATE_BODY_AXES_UNVALIDATED")+"\"";
  line+=",\"trims\":{\"roll_deg\":"+String(c.rollTrimDeg,2)+",\"pitch_deg\":"+String(c.pitchTrimDeg,2)+",\"yaw_rate_dps\":"+String(c.yawTrimDps,2)+"}";
  line+=",\"roll_deg\":";line+=finiteV(s.rollDeg)?String(s.rollDeg,2):String("null");line+=",\"pitch_deg\":";line+=finiteV(s.pitchDeg)?String(s.pitchDeg,2):String("null");
  line+=",\"gyro_dps\":[";line+=finiteV(s.gx)?String(s.gx,2):String("null");line+=",";line+=finiteV(s.gy)?String(s.gy,2):String("null");line+=",";line+=finiteV(s.gz)?String(s.gz,2):String("null");line+="]";
  setFiniteJson(line,"alt_m",s.altitudeM,3);setFiniteJson(line,"pressure_pa",s.pressurePa,1);setFiniteJson(line,"temp_c",s.tempC,2);setFiniteJson(line,"distance_cm",(HCSR04_LEVEL_SHIFT_VALIDATED?s.distanceCm:NAN),2);
  line+="\"water_raw\":"+(WATER_SENSOR_CALIBRATED?String(s.waterRaw):String("null"))+",";
  line+="\"water_status\":\"UNAVAILABLE_GPIO42_NOT_ADC\",\"battery_raw\":null,\"battery_v\":null,\"battery_status\":\"UNAVAILABLE_GPIO11_ADC2_WIFI_CONFLICT_OR_DIVIDER_UNCALIBRATED\",";
  setAgeJson(line,"imu_age_ms",s.imuAgeMs);setAgeJson(line,"baro_age_ms",s.baroAgeMs);setAgeJson(line,"range_age_ms",HCSR04_LEVEL_SHIFT_VALIDATED?s.rangeAgeMs:INVALID_AGE_MS);
  line+="\"loop_us\":"+String(c.loopUs)+",\"loop_overruns\":"+String(c.loopOverruns)+",\"outputs_enabled\":"+(escGate?String("true"):String("false"));
  line+=",\"servo_outputs_enabled\":"+(servoGate?String("true"):String("false"))+",\"preflight_passed\":"+(preflight?String("true"):String("false"));
  line+=",\"mag_ok\":false,\"mag_addr\":null,\"mag_status\":\"NOT_INSTALLED_MODEL_REQUIRED\",\"range_status\":\""+String(HCSR04_LEVEL_SHIFT_VALIDATED?"ENABLED":"UNAVAILABLE_ECHO_LEVEL_NOT_VALIDATED")+"\"";
  line+=",\"failsafe_state\":"+String(failsafeState)+",\"last_command_seq\":"+String(lastProcessedCommandSeq)+",\"last_command_status\":"+String(lastProcessedCommandStatus)+",\"last_reason\":\"";
  String safe=reason;safe.replace("\\","\\\\");safe.replace("\"","\\\"");safe.replace("\n"," ");safe.replace("\r"," ");line+=safe+"\"}";
  if(serialMutex&&xSemaphoreTake(serialMutex,0)==pdTRUE){Serial.println(line);xSemaphoreGive(serialMutex);}
#if RIFT_ENABLE_ESPNOW_RX
  if(nowPeerReady){RwTelemetryPacket p={};p.sequence=++telemetrySequence;p.uptimeMs=millis();p.armedRuntimeMs=armedAccumMs+(armed?millis()-armedStartedAt:0);p.armed=armed?1:0;p.mode=(uint8_t)currentMode;
    p.sensorFlags=(s.imuOk?RW_SF_IMU_OK:0)|(imuCalibrated?RW_SF_GYRO_CAL:0)|(s.baroOk?RW_SF_BARO_OK:0)|(finiteV(groundPressurePa)?RW_SF_ALT_REF:0)|(HCSR04_LEVEL_SHIFT_VALIDATED&&finiteV(s.distanceCm)?RW_SF_RANGE_OK:0)|(WATER_SENSOR_CALIBRATED?RW_SF_WATER_OK:0)|(BATTERY_DIVIDER_CALIBRATED?RW_SF_BATTERY_OK:0)|(BODY_AXIS_MAPPING_VALIDATED?RW_SF_BODY_AXES_OK:0);
    p.systemFlags=(escGate?RW_SYS_ESC_OUTPUTS_ENABLED:0)|(servoGate?RW_SYS_SERVO_OUTPUTS_ENABLED:0)|(nowPeerReady?RW_SYS_ESPNOW_PAIRED:0)|(MOTOR_ORDER_VALIDATED?RW_SYS_MOTOR_MAP_VALID:0)|(FAILSAFE_VALIDATED?RW_SYS_FAILSAFE_VALID:0)|(HCSR04_LEVEL_SHIFT_VALIDATED?RW_SYS_HCSR04_VALID:0)|(preflight?RW_SYS_PREFLIGHT_PASSED:0);
    p.imuAddr=imuAddr;p.baroAddr=baroAddr;p.magAddr=0;for(int i=0;i<4;i++){p.escCommandUs[i]=(uint16_t)escCommandUs[i];p.servoCommandDeg[i]=(uint8_t)servoCommandDeg[i];}
    p.rollCdeg=(finiteV(s.rollDeg)&&BODY_AXIS_MAPPING_VALIDATED)?(int16_t)clampf(s.rollDeg*100,-32766,32766):INT16_MAX;p.pitchCdeg=(finiteV(s.pitchDeg)&&BODY_AXIS_MAPPING_VALIDATED)?(int16_t)clampf(s.pitchDeg*100,-32766,32766):INT16_MAX;
    p.rollTrimCdeg=(int16_t)clampf(c.rollTrimDeg*100,-32767,32767);p.pitchTrimCdeg=(int16_t)clampf(c.pitchTrimDeg*100,-32767,32767);p.yawTrimCentiDps=(int16_t)clampf(c.yawTrimDps*100,-32767,32767);
    p.gxCentiDps=finiteV(s.gx)?(int16_t)clampf(s.gx*100,-32766,32766):INT16_MAX;p.gyCentiDps=finiteV(s.gy)?(int16_t)clampf(s.gy*100,-32766,32766):INT16_MAX;p.gzCentiDps=finiteV(s.gz)?(int16_t)clampf(s.gz*100,-32766,32766):INT16_MAX;
    p.altitudeMm=finiteV(s.altitudeM)?(int32_t)clampf(s.altitudeM*1000,-2000000000.0f,2000000000.0f):INT32_MAX;p.pressurePa=finiteV(s.pressurePa)?(int32_t)s.pressurePa:-1;p.temperatureCentiC=finiteV(s.tempC)?(int16_t)clampf(s.tempC*100,-32766,32766):INT16_MAX;
    p.distanceMm=HCSR04_LEVEL_SHIFT_VALIDATED&&finiteV(s.distanceCm)?(int16_t)clampf(s.distanceCm*10,0,30000):INT16_MAX;p.waterRaw=WATER_SENSOR_CALIBRATED?s.waterRaw:UINT16_MAX;p.batteryRaw=BATTERY_DIVIDER_CALIBRATED?s.batteryRaw:UINT16_MAX;
    p.controlLoopUs=c.loopUs;p.loopOverruns=c.loopOverruns;p.imuAgeMs=s.imuAgeMs;p.baroAgeMs=s.baroAgeMs;p.rangeAgeMs=HCSR04_LEVEL_SHIFT_VALIDATED?s.rangeAgeMs:INVALID_AGE_MS;p.lastCommandSequence=ackSeq;p.lastCommandStatus=ackStatus;p.failsafeState=failsafeState;strncpy(p.lastReason,reason,sizeof(p.lastReason)-1);p.lastReason[sizeof(p.lastReason)-1]='\0';rwFinishTelemetry(p);(void)queueRadioFrame(reinterpret_cast<const uint8_t*>(&p),sizeof(p));
  }
#endif
}

void printPID(const char* name,const PIDGains& g){printLineNonBlocking(String("PID,")+name+",KP,"+String(g.kp,6)+",KI,"+String(g.ki,6)+",KD,"+String(g.kd,6)+",IMAX,"+String(g.imax,3)+",OUTMAX,"+String(g.outMax,3));}
PIDGains* getGains(String axis){axis.toUpperCase();if(axis=="ROLL_RATE")return &rollRateG;if(axis=="PITCH_RATE")return &pitchRateG;if(axis=="YAW_RATE")return &yawRateG;if(axis=="ROLL_ANGLE")return &rollAngleG;if(axis=="PITCH_ANGLE")return &pitchAngleG;if(axis=="ALTITUDE")return &altitudeG;return nullptr;}
void printAllPID(){printPID("ROLL_RATE",rollRateG);printPID("PITCH_RATE",pitchRateG);printPID("YAW_RATE",yawRateG);printPID("ROLL_ANGLE",rollAngleG);printPID("PITCH_ANGLE",pitchAngleG);printPID("ALTITUDE",altitudeG);}
void saveGains(){prefs.begin("riftpid",false);auto put=[&](const char* n,PIDGains&g){prefs.putFloat((String(n)+"p").c_str(),g.kp);prefs.putFloat((String(n)+"i").c_str(),g.ki);prefs.putFloat((String(n)+"d").c_str(),g.kd);prefs.putFloat((String(n)+"m").c_str(),g.imax);prefs.putFloat((String(n)+"o").c_str(),g.outMax);};put("rr",rollRateG);put("pr",pitchRateG);put("yr",yawRateG);put("ra",rollAngleG);put("pa",pitchAngleG);put("al",altitudeG);prefs.end();report("PID values saved to NVS. This does not validate flight tuning.");}
void loadGains(){prefs.begin("riftpid",true);auto get=[&](const char*n,PIDGains&g){g.kp=prefs.getFloat((String(n)+"p").c_str(),g.kp);g.ki=prefs.getFloat((String(n)+"i").c_str(),g.ki);g.kd=prefs.getFloat((String(n)+"d").c_str(),g.kd);g.imax=prefs.getFloat((String(n)+"m").c_str(),g.imax);g.outMax=prefs.getFloat((String(n)+"o").c_str(),g.outMax);};get("rr",rollRateG);get("pr",pitchRateG);get("yr",yawRateG);get("ra",rollAngleG);get("pa",pitchAngleG);get("al",altitudeG);prefs.end();}
void applyMode(Mode m){
  if(armed||!outputsAtMinimum()){report("MODE REJECTED: only disarmed with all ESC commands at minimum.");return;}
  if(m==MODE_WATER&&!WATER_SERVO_GEOMETRY_VALID){report("MODE REJECTED: water propulsion/servo mechanism is unspecified; no WATER geometry is sent.");return;}
  armPermit=false;currentMode=m;writeServosForMode(m);
  report(String("MODE STORED ")+(m==MODE_AIR?"AIR":m==MODE_WATER?"WATER":"LAND")+((RIFT_ENABLE_SERVO_OUTPUTS&&SERVO_GEOMETRY_VALIDATED)?" with validated servo output":" logically only; SERVO OUTPUT LOCKED"));
}
void setpoint(String name,float value){name.trim();name.toUpperCase();if(!finiteV(value)){report("SETPOINT rejected: non-finite value.");return;}bool known=true,needsPidReset=false;
  if(name=="ROLL_DEG"){portENTER_CRITICAL(&stateMux);ctl.rollTarget=clampf(value,-MAX_ATTITUDE_DEG,MAX_ATTITUDE_DEG);portEXIT_CRITICAL(&stateMux);}
  else if(name=="PITCH_DEG"){portENTER_CRITICAL(&stateMux);ctl.pitchTarget=clampf(value,-MAX_ATTITUDE_DEG,MAX_ATTITUDE_DEG);portEXIT_CRITICAL(&stateMux);}
  else if(name=="YAW_RATE_DPS"){portENTER_CRITICAL(&stateMux);ctl.yawRateTarget=clampf(value,-MAX_RATE_DPS,MAX_RATE_DPS);portEXIT_CRITICAL(&stateMux);}
  else if(name=="ROLL_TRIM_DEG"||name=="PITCH_TRIM_DEG"||name=="YAW_TRIM_DPS"){
    if(armed){report("TRIM rejected while armed; change trims only while disarmed in this bring-up build.");return;}
    portENTER_CRITICAL(&stateMux);
    if(name=="ROLL_TRIM_DEG")ctl.rollTrimDeg=clampf(value,-4.0f,4.0f);
    else if(name=="PITCH_TRIM_DEG")ctl.pitchTrimDeg=clampf(value,-4.0f,4.0f);
    else ctl.yawTrimDps=clampf(value,-15.0f,15.0f);
    portEXIT_CRITICAL(&stateMux);needsPidReset=true;
  }
  else if(name=="THROTTLE"){portENTER_CRITICAL(&stateMux);ctl.throttle=clampf(value,0,0.6f);portEXIT_CRITICAL(&stateMux);}
  else if(name=="ALT_M"){if(!RIFT_ENABLE_ALTITUDE_HOLD||!ALTITUDE_HOLD_VALIDATED){report("SETPOINT rejected: altitude hold is compile-time gated pending sensor/control validation.");return;}portENTER_CRITICAL(&stateMux);altitudeTargetM=clampf(value,0,5);altHold=true;portEXIT_CRITICAL(&stateMux);}
  else if(name=="ALT_HOLD"){if(value>0.5f&&(!RIFT_ENABLE_ALTITUDE_HOLD||!ALTITUDE_HOLD_VALIDATED)){report("SETPOINT rejected: altitude hold is not validated/enabled.");return;}portENTER_CRITICAL(&stateMux);altHold=(value>0.5f);portEXIT_CRITICAL(&stateMux);if(!altHold)needsPidReset=true;}
  else known=false;
  if(!known){report("SETPOINT rejected: unknown setpoint name.");return;}
  if(needsPidReset)requestPIDReset();noteControlHeartbeat();report("SETPOINT accepted and bounded; output gate status unchanged.");
}
void processCommand(String cmd){cmd.trim();if(!cmd.length())return;String upper=cmd;upper.toUpperCase();
  if(upper=="PING"){noteControlHeartbeat();return;}
  if(upper=="HELP"){report("Commands: PREFLIGHT, STATUS, GETPID, TUNE,AXIS,GAIN,VALUE, SAVE_PID, CAL_GYRO, BARO_ZERO, ARM, START, S/STOP, A/L/W, SETPOINT,NAME,VALUE");return;}
  if(upper=="PREFLIGHT"){report(preflightReport());return;}
  if(upper=="STATUS"){report(String("STATUS: ")+(armed?"ARMED":"DISARMED")+"; ESC outputs="+(RIFT_ENABLE_ACTUATOR_OUTPUTS?"compile-enabled":"LOCKED")+"; servo outputs="+(RIFT_ENABLE_SERVO_OUTPUTS?"compile-enabled":"LOCKED")+"; IMU="+(sensors.imuOk?"OK":"FAULT")+"; gyro="+(imuCalibrated?"CALIBRATED":"REQUIRED")+"; barometer="+(sensors.baroOk?"OK":"FAULT"));return;}
  if(upper=="GETPID"){printAllPID();report("PID snapshot emitted; current gains are bench placeholders until tested.");return;}
  if(upper=="RESET_PID"){if(armed){report("RESET_PID rejected while armed.");return;}portENTER_CRITICAL(&stateMux);ctl.rollTarget=0;ctl.pitchTarget=0;ctl.yawRateTarget=0;ctl.rollTrimDeg=0;ctl.pitchTrimDeg=0;ctl.yawTrimDps=0;ctl.throttle=0;pidResetPending=true;portEXIT_CRITICAL(&stateMux);report("PID states/setpoints/trims reset to zero; outputs remain locked.");return;}
  if(upper=="SAVE_PID"){if(armed){report("SAVE_PID rejected while armed.");return;}saveGains();return;}
  if(upper=="CAL_GYRO"){if(armed){report("CAL_GYRO rejected while armed.");return;}imuCalibrated=false;calibrateGyro();return;}
  if(upper=="BARO_ZERO"){if(armed||currentMode!=MODE_LAND||!outputsAtMinimum()||!sensors.baroOk||!finiteV(sensors.pressurePa)){report("BARO_ZERO rejected: disarm, select LAND, keep vehicle still at ground level and verify MS5607 readings.");return;}groundPressurePa=sensors.pressurePa;altitudeTargetM=0;portENTER_CRITICAL(&stateMux);sensors.altitudeM=0.0f;portEXIT_CRITICAL(&stateMux);report("BARO_ZERO accepted: relative pressure reference stored in RAM until reset.");return;}
  if(upper=="ARM"){
    if(!preflightHardwareReady()){report(String("ARM BLOCKED: preflight not ready. ")+preflightReport());return;}
    if(currentMode!=MODE_AIR){report("ARM BLOCKED: select AIR while disarmed first.");return;}if(armed||!outputsAtMinimum()||!sensors.imuOk||!imuCalibrated||!sensors.baroOk||!finiteV(groundPressurePa)){report("ARM BLOCKED: require disarmed/minimum outputs, calibrated IMU and valid MS5607 ground reference.");return;}
    armPermit=true;armPermitAt=millis();report("ARM request accepted. Send START within 8 s after checks. Motors remain at minimum.");return;
  }
  if(upper=="START"){if(!armPermit||millis()-armPermitAt>ARM_CONFIRM_MS){armPermit=false;report("START rejected: send ARM then START within 8 seconds.");return;}armPermit=false;stopLatched=false;armed=true;failsafeState=RW_FS_ARMED;armedStartedAt=millis();linkLossHold=false;linkLossStartedAt=0;noteControlHeartbeat();report("ARMED at idle; this is not proof of safe hover or tuned PID.");return;}
  if(upper=="S"||upper=="STOP"||upper=="DISARM"){disarmNow("OPERATOR STOP");return;}
  if(upper=="A"||upper=="MODE,AIR"){applyMode(MODE_AIR);return;}if(upper=="L"||upper=="MODE,LAND"){applyMode(MODE_LAND);return;}if(upper=="W"||upper=="MODE,WATER"){applyMode(MODE_WATER);return;}
  if(upper.startsWith("SETPOINT,")){int p1=cmd.indexOf(',');int p2=cmd.indexOf(',',p1+1);float value=0;if(p2<0||!parseFiniteFloatStrict(cmd.substring(p2+1),value)){report("SETPOINT rejected: use SETPOINT,NAME,FINITE_NUMBER.");return;}setpoint(cmd.substring(p1+1,p2),value);return;}
  if(upper.startsWith("TUNE,")){String parts[4];int start=0,n=0;for(int i=0;i<3;i++){int p=cmd.indexOf(',',start);if(p<0)break;parts[n++]=cmd.substring(start,p);start=p+1;}parts[n++]=cmd.substring(start);float val=0;if(n!=4||!parseFiniteFloatStrict(parts[3],val)){report("TUNE rejected: use TUNE,AXIS,KP|KI|KD|IMAX|OUTMAX,FINITE_NUMBER.");return;}PIDGains*g=getGains(parts[1]);String key=parts[2];key.toUpperCase();if(!g||val<0||val>100){report("TUNE rejected: axis/value invalid or outside [0,100].");return;}if(armed){report("TUNE rejected while armed.");return;}portENTER_CRITICAL(&stateMux);if(key=="KP")g->kp=val;else if(key=="KI")g->ki=val;else if(key=="KD")g->kd=val;else if(key=="IMAX")g->imax=val;else if(key=="OUTMAX")g->outMax=val;else{portEXIT_CRITICAL(&stateMux);report("Unknown gain. Use KP, KI, KD, IMAX, OUTMAX.");return;}pidResetPending=true;portEXIT_CRITICAL(&stateMux);printPID(parts[1].c_str(),*g);report("TUNE applied to stored controller gains; actuator output remains locked.");return;}
  // Direction controls are setpoint nudges. No direct open-loop pulse command is implemented.
  float rollSp,pitchSp,baseThrottle;portENTER_CRITICAL(&stateMux);rollSp=ctl.rollTarget;pitchSp=ctl.pitchTarget;baseThrottle=ctl.throttle;portEXIT_CRITICAL(&stateMux);if(upper=="U"){setpoint("PITCH_DEG",pitchSp-3.0f);return;}if(upper=="J"){setpoint("PITCH_DEG",pitchSp+3.0f);return;}if(upper=="H"){setpoint("ROLL_DEG",rollSp-3.0f);return;}if(upper=="K"){setpoint("ROLL_DEG",rollSp+3.0f);return;}
  if(upper=="+"){setpoint("THROTTLE",baseThrottle+0.02f);return;}if(upper=="-"){setpoint("THROTTLE",baseThrottle-0.02f);return;}
  report("Unknown command.");
}

bool enqueueCommand(const char* input,uint16_t seq=0,bool viaNow=false){if(!commandQueue||!input)return false;CommandMsg m={};const size_t n=strnlen(input,RW_CMD_TEXT_CAP-1);memcpy(m.text,input,n);m.text[n]='\0';m.sequence=seq;m.viaNow=viaNow;return xQueueSend(commandQueue,&m,0)==pdTRUE;}
#if RIFT_ENABLE_ESPNOW_RX
void onNowRecv(const esp_now_recv_info_t* info,const uint8_t* data,int len){
  if(!info||!data||len!=(int)sizeof(RwCommandPacket)||zeroMac(ESPNOW_GROUND_MAC)||memcmp(info->src_addr,ESPNOW_GROUND_MAC,6)!=0)return;
  RwCommandPacket p;memcpy(&p,data,sizeof(p));if(!rwValidCommand(p))return;
  if(nowRxSeqSeen&&(int16_t)(p.sequence-lastNowRxSeq)<=0)return; // discard replay/out-of-order frames
  lastNowRxSeq=p.sequence;nowRxSeqSeen=true;
  // Stop is a reception-side priority request: the 250 Hz control task sees it even if
  // the command task is occupied by a lengthy gyro calibration or the normal queue is full.
  if(isStopCommandText(p.text)){
    portENTER_CRITICAL(&stateMux);emergencyStopRequested=true;lastProcessedCommandSeq=p.sequence;lastProcessedCommandStatus=RW_ACK_ACCEPTED;portEXIT_CRITICAL(&stateMux);
  }
  if(!enqueueCommand(p.text,p.sequence,true)&&!isStopCommandText(p.text))setReason("ESP-NOW command dropped: queue full");
}
#endif
void commandTask(void*){
  CommandMsg m;
  for(;;){
    if(commandQueue&&xQueueReceive(commandQueue,&m,portMAX_DELAY)==pdTRUE){
      setReason("COMMAND PROCESSING");
      activeCommandViaNow=m.viaNow;activeRemoteSequence=m.sequence;
      processCommand(String(m.text));
      activeCommandViaNow=false;
      if(m.viaNow){
        char reasonSnapshot[96];
        portENTER_CRITICAL(&stateMux);strncpy(reasonSnapshot,lastReason,sizeof(reasonSnapshot)-1);reasonSnapshot[sizeof(reasonSnapshot)-1]='\0';portEXIT_CRITICAL(&stateMux);
        String reason=String(reasonSnapshot);
        reason.toUpperCase();
        const uint8_t status=(reason.indexOf("BLOCKED")>=0||reason.indexOf("REJECTED")>=0||reason.indexOf("FAILED")>=0||reason.indexOf("ERROR")>=0||reason.indexOf("UNKNOWN")>=0)?RW_ACK_REJECTED:RW_ACK_ACCEPTED;
        portENTER_CRITICAL(&stateMux);lastProcessedCommandSeq=m.sequence;lastProcessedCommandStatus=status;portEXIT_CRITICAL(&stateMux);
      }
    }
  }
}
void modeManagerTask(void*){for(;;){
  // Automatic AIR/LAND/WATER transitions intentionally disabled until range electronics and
  // the land/water propulsion mechanics have been validated. Never change geometry mid-thrust.
  pendingLand=false;pendingWater=false;vTaskDelay(pdMS_TO_TICKS(100));
}}
#if RIFT_ENABLE_ESPNOW_RX
void radioTxTask(void*){
  RadioTxMessage msg;
  for(;;){
    if(xQueueReceive(radioTxQueue,&msg,portMAX_DELAY)!=pdTRUE)continue;
    if(!nowPeerReady||zeroMac(ESPNOW_GROUND_MAC))continue;
    esp_err_t err=ESP_FAIL;
    // Serialise outbound ESP-NOW frames (telemetry and command text) through one task.
    // The spacing avoids issuing a burst of concurrent SDK sends when GETPID emits many lines.
    for(uint8_t attempt=0;attempt<3;attempt++){
      err=esp_now_send(ESPNOW_GROUND_MAC,msg.payload,msg.length);
      if(err==ESP_OK)break;
      vTaskDelay(pdMS_TO_TICKS(3));
    }
    if(err!=ESP_OK)radioTxDrops++;
    vTaskDelay(pdMS_TO_TICKS(4));
  }
}
#endif
void serialTxTask(void*){
  SerialMessage msg;
  for(;;){
    if(xQueueReceive(serialTxQueue,&msg,portMAX_DELAY)!=pdTRUE)continue;
    if(serialMutex&&xSemaphoreTake(serialMutex,pdMS_TO_TICKS(250))==pdTRUE){Serial.println(msg.text);xSemaphoreGive(serialMutex);}
  }
}
void telemetryTask(void*){TickType_t last=xTaskGetTickCount();for(;;){emitTelemetry();vTaskDelayUntil(&last,pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));}}
void rxSerialNonBlocking(){static char buffer[RW_CMD_TEXT_CAP];static size_t used=0;static bool overflow=false;while(Serial.available()>0){const char c=(char)Serial.read();if(c=='\r')continue;if(c=='\n'){if(!overflow){buffer[used]='\0';if(used){if(isStopCommandText(buffer)){portENTER_CRITICAL(&stateMux);emergencyStopRequested=true;portEXIT_CRITICAL(&stateMux);}enqueueCommand(buffer,0,false);}}else printLineNonBlocking("ERROR: serial command too long; discarded.");used=0;overflow=false;}else if(!overflow){if(used<RW_CMD_TEXT_CAP-1)buffer[used++]=c;else{used=0;overflow=true;}}}}

void setup(){
  Serial.begin(SERIAL_BAUD);delay(350);
  serialMutex=xSemaphoreCreateMutex();i2cMutex=xSemaphoreCreateMutex();commandQueue=xQueueCreate(12,sizeof(CommandMsg));radioTxQueue=xQueueCreate(24,sizeof(RadioTxMessage));serialTxQueue=xQueueCreate(24,sizeof(SerialMessage));
  Serial.println("RIFTWALKER SAFE BRING-UP / NO FLIGHT AUTHORISATION");
  Serial.printf("Typed GPIO config: ESC %d,%d,%d,%d | SERVOS %d,%d,%d,%d | I2C SDA/SCL %d/%d\n",ESC_PINS[0],ESC_PINS[1],ESC_PINS[2],ESC_PINS[3],SERVO_PINS[0],SERVO_PINS[1],SERVO_PINS[2],SERVO_PINS[3],I2C_SDA_PIN,I2C_SCL_PIN);
  Serial.printf("ESC pulses: disarmed %d us, stated rotation onset %d us, normal firmware cap %d us. 4S LiPo nominal 14.8 V; fully charged 16.8 V.\n",ESC_MIN_US,ESC_SPINUP_US,ESC_MAX_US);
  Wire.begin(I2C_SDA_PIN,I2C_SCL_PIN,400000);Wire.setTimeOut(3);
  sensors.imuOk=initMPU();sensors.imuAddr=imuAddr;sensors.baroOk=initMS5607();sensors.baroAddr=baroAddr;
  Serial.printf("I2C check: MPU6050 %s at 0x%02X (preferred 0x68); MS5607 %s at 0x%02X (preferred 0x76).\n",sensors.imuOk?"FOUND":"NOT FOUND",imuAddr,sensors.baroOk?"FOUND":"NOT FOUND",baroAddr);
  // Do NOT attach PWM pins during bring-up. Attaching an ESC or servo writes signals to hardware.
#if RIFT_ENABLE_ACTUATOR_OUTPUTS
  ESP32PWM::allocateTimer(0);ESP32PWM::allocateTimer(1);ESP32PWM::allocateTimer(2);ESP32PWM::allocateTimer(3);
  bool escOk=true;
  for(int i=0;i<4;i++){esc[i].setPeriodHertz(ESC_RATE_HZ);if(!esc[i].attach(ESC_PINS[i],ESC_MIN_US,ESC_MAX_US)){escOk=false;report("FATAL: ESC attach failure");}else esc[i].writeMicroseconds(ESC_MIN_US);}
  escAttachOk=escOk;
#endif
#if RIFT_ENABLE_SERVO_OUTPUTS
  // ESP32Servo timer resources are shared; allocate only the timers actually available in this core.
  bool servoOk=true;
  for(int i=0;i<4;i++){vectorServo[i].setPeriodHertz(50);if(!vectorServo[i].attach(SERVO_PINS[i],SERVO_MIN_US,SERVO_MAX_US)){servoOk=false;report("FATAL: servo attach failure");}}
  servoAttachOk=servoOk;
#endif
  outputsMinimum();loadGains();lastCommandAt=millis();
#if RIFT_ENABLE_ESPNOW_RX
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);esp_wifi_set_channel(ESPNOW_ROVER_CHANNEL,WIFI_SECOND_CHAN_NONE);Serial.printf("ROVER STA MAC: %s | ESP-NOW channel %u\n",WiFi.macAddress().c_str(),ESPNOW_ROVER_CHANNEL);
  if(esp_now_init()==ESP_OK){esp_now_register_recv_cb(onNowRecv);if(!zeroMac(ESPNOW_GROUND_MAC)){esp_now_peer_info_t peer={};memcpy(peer.peer_addr,ESPNOW_GROUND_MAC,6);peer.channel=ESPNOW_ROVER_CHANNEL;peer.encrypt=false;if(esp_now_add_peer(&peer)==ESP_OK){nowPeerReady=true;Serial.println("ESP-NOW ground peer configured; verify reciprocal ground bridge MAC and channel.");}else Serial.println("ESP-NOW peer config failed; radio commands not accepted.");}else Serial.println("ESP-NOW is initialized but UNPAIRED: configure ESPNOW_GROUND_MAC before sending commands.");}
  else Serial.println("ESP-NOW init failed; direct USB serial still available.");
#endif
  // Initial relative altitude reference can be set only from a valid MS5607 sample while at rest.
  if(sensors.baroOk){float p,t;if(readMS5607(p,t)){groundPressurePa=p;sensors.pressurePa=p;sensors.tempC=t;sensors.altitudeM=0;lastBaroSampleMs=millis();sensors.baroAgeMs=0;}}
  if(!commandQueue||!i2cMutex||!serialMutex||!radioTxQueue||!serialTxQueue){Serial.println("FATAL: RTOS resource allocation failed; outputs remain unattached/locked.");while(true){delay(1000);}}
#if RIFT_ENABLE_ESPNOW_RX
  BaseType_t h=xTaskCreatePinnedToCore(radioTxTask,"rw_radio_tx",3072,nullptr,2,nullptr,0);
  nowTxTaskReady=(h==pdPASS);
  if(!nowTxTaskReady){nowPeerReady=false;Serial.println("WARNING: ESP-NOW TX task unavailable; radio commands/telemetry disabled.");}
#else
  BaseType_t h=pdPASS;
#endif
  BaseType_t a=xTaskCreatePinnedToCore(commandTask,"rw_cmd",4096,nullptr,3,nullptr,0);
  BaseType_t b=xTaskCreatePinnedToCore(modeManagerTask,"rw_mode",2048,nullptr,1,nullptr,0);
  BaseType_t c=xTaskCreatePinnedToCore(baroTask,"rw_baro",4096,nullptr,2,nullptr,0);
  BaseType_t d=xTaskCreatePinnedToCore(rangeTask,"rw_range",2048,nullptr,1,nullptr,0);
  BaseType_t e=xTaskCreatePinnedToCore(sensorTask,"rw_imu",4096,nullptr,7,nullptr,1);
  BaseType_t f=xTaskCreatePinnedToCore(controlTask,"rw_control250",4096,nullptr,6,nullptr,1);
  BaseType_t i=xTaskCreatePinnedToCore(serialTxTask,"rw_serial_tx",3072,nullptr,2,nullptr,0);
  BaseType_t g=xTaskCreatePinnedToCore(telemetryTask,"rw_telemetry",4096,nullptr,1,nullptr,0);
  if(a!=pdPASS||b!=pdPASS||c!=pdPASS||d!=pdPASS||e!=pdPASS||f!=pdPASS||g!=pdPASS||h!=pdPASS||i!=pdPASS){setReason("FATAL: FreeRTOS task creation failed; outputs remain locked");outputsMinimum();Serial.println(lastReason);}
  else rtosTasksReady=true;
  Serial.println("PREFLIGHT REQUIRED. Outputs are not attached. Run PREFLIGHT, then CAL_GYRO while completely still, then BARO_ZERO while disarmed on the ground.");
  Serial.println("Sensor limitations: water input GPIO42 cannot be analog-read; battery GPIO11/ADC2 is not reported with Wi-Fi and divider uncalibrated; HC-SR04 gated until ECHO level verified; magnetometer not installed/modelled.");
  Serial.println(preflightReport());
}
void loop(){rxSerialNonBlocking();if(armPermit&&millis()-armPermitAt>ARM_CONFIRM_MS){armPermit=false;report("ARM request expired.");}vTaskDelay(pdMS_TO_TICKS(1));}
