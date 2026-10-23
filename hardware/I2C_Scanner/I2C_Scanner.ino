#include <Arduino.h>
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
  #error "Select an ESP32-S3 board (for example ESP32S3 Dev Module) before compiling the I2C scanner."
#endif
#include <Wire.h>
// Upload to discover actual sensor addresses. Uses RIFTWALKER SDA=GPIO4, SCL=GPIO5.
void setup(){Serial.begin(115200);delay(500);Wire.begin(4,5,100000);Serial.println("RIFTWALKER I2C SCANNER; SDA=GPIO4 SCL=GPIO5");int found=0;for(uint8_t a=1;a<127;a++){Wire.beginTransmission(a);uint8_t e=Wire.endTransmission();if(e==0){Serial.printf("I2C device at 0x%02X",a);if(a==0x68||a==0x69)Serial.print("  <-- MPU6050 candidate");if(a==0x76||a==0x77)Serial.print("  <-- MS5607 candidate");Serial.println();found++;}delay(1);}Serial.printf("Scan complete: %d device(s).\n",found);Serial.println("MPU6050 is normally 0x68 or 0x69 depending on AD0. MS5607 is normally 0x76 or 0x77 depending on CSB/address wiring.");}
void loop(){delay(1000);}
