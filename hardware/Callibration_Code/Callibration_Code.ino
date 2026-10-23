#include <ESP32Servo.h>
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
  #error "Select an ESP32-S3 board (for example ESP32S3 Dev Module) before compiling the ESC calibration sketch."
#endif
// RIFTWALKER ESC CALIBRATION ONLY. Upload separately from flight firmware.
// Propellers removed, battery disconnected before upload, follow ESC-specific manual.
static constexpr int ESC_PINS[4]={15,6,18,9};
static constexpr int ESC_COUNT=4, MIN_US=1000, MAX_US=2000, RATE_HZ=50;
static constexpr uint32_t CONFIRM_WINDOW_MS=10000;
Servo esc[ESC_COUNT];bool maxRequested=false;uint32_t maxAt=0;bool ready=false;
void allEsc(int us){us=constrain(us,MIN_US,MAX_US);for(int i=0;i<ESC_COUNT;i++)if(esc[i].attached())esc[i].writeMicroseconds(us);}
void safeMin(const char* why){allEsc(MIN_US);maxRequested=false;Serial.println(why);Serial.println("All ESC signals requested at 1000 us; calibration storage is not verified.");}
void setup(){Serial.begin(115200);delay(500);Serial.println("RIFTWALKER ESC CALIBRATION - SAFE START");Serial.println("REMOVE PROPELLERS. LiPo disconnected while uploading.");ESP32PWM::allocateTimer(0);ESP32PWM::allocateTimer(1);ESP32PWM::allocateTimer(2);ESP32PWM::allocateTimer(3);ready=true;for(int i=0;i<ESC_COUNT;i++){esc[i].setPeriodHertz(RATE_HZ);esc[i].attach(ESC_PINS[i],MIN_US,MAX_US);if(!esc[i].attached())ready=false;}allEsc(MIN_US);if(!ready){Serial.println("ERROR: ESC attach failure; remain at minimum.");while(true){allEsc(MIN_US);delay(100);}}Serial.println("Ready at minimum. Commands: PREPARE_MAX, CONFIRM_MIN, X.");Serial.println("Only use PREPARE_MAX if your ESC manual requires high-throttle calibration.");}
void loop(){if(Serial.available()){String c=Serial.readStringUntil('\n');c.trim();c.toUpperCase();if(c=="X"||c=="STOP")safeMin("ABORT requested.");else if(c=="PREPARE_MAX"){if(maxRequested)Serial.println("Max already requested; confirm or abort.");else{allEsc(MIN_US);delay(200);allEsc(MAX_US);maxRequested=true;maxAt=millis();Serial.println("2000 us requested. Follow exact ESC manual tones; type CONFIRM_MIN after confirmation.");}}else if(c=="CONFIRM_MIN"){if(!maxRequested)Serial.println("Rejected: PREPARE_MAX required first.");else if(millis()-maxAt>CONFIRM_WINDOW_MS)safeMin("Timeout.");else safeMin("Minimum stage requested.");}else if(c.length())Serial.println("Unknown command.");}if(maxRequested&&millis()-maxAt>CONFIRM_WINDOW_MS)safeMin("SAFETY TIMEOUT: minimum requested.");if(!maxRequested)allEsc(MIN_US);delay(5);}
