#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_SSD1306.h>

// ---------------- PIN MAPPING ----------------
#define TRIG 5
#define ECHO 35
#define POT_THR 34
#define BTN_FWD 26
#define BTN_BCK 25
#define BTN_LFT 17
#define BTN_RGT 15
#define BTN_WAT 16

#define STEP_FL 27
#define STEP_BL 14
#define STEP_FR 13
#define STEP_BR 12

#define DIR_L 32
#define DIR_R 33

#define S_FL 18
#define S_FR 19
#define S_BL 23
#define S_BR 4   // (change to 5 if hardware issue)

// ---------------- CONSTANTS ----------------
const int STEPS_PER_REV = 200;
const int MICROSTEPPING = 1;
const int TOTAL_STEPS = STEPS_PER_REV * MICROSTEPPING;

// ---------------- GLOBAL DATA ----------------
volatile float shared_dist = 0;
volatile float shared_pres = 1013.25;
volatile int shared_thr = 0;
volatile bool shared_water = false;
volatile bool shared_isFlying = false;

volatile float accX = 0, accY = 0;

String shared_status = "IDLE";
String stab_action = "LEVEL";

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// ---------------- DEVICES ----------------
Adafruit_MPU6050 mpu;
Adafruit_BMP085 bmp;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Servo servos[4];

// ---------------- SETUP ----------------
void setup() {
  Wire.begin(21, 22);
  Wire.setClock(400000);

  pinMode(BTN_FWD, INPUT_PULLUP);
  pinMode(BTN_BCK, INPUT_PULLUP);
  pinMode(BTN_LFT, INPUT_PULLUP);
  pinMode(BTN_RGT, INPUT_PULLUP);
  pinMode(BTN_WAT, INPUT_PULLUP);

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

  pinMode(STEP_FL, OUTPUT);
  pinMode(STEP_FR, OUTPUT);
  pinMode(STEP_BL, OUTPUT);
  pinMode(STEP_BR, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  bmp.begin();
  mpu.begin();

  servos[0].attach(S_FL);
  servos[1].attach(S_FR);
  servos[2].attach(S_BL);
  servos[3].attach(S_BR);

  xTaskCreatePinnedToCore(motorTask, "MotorTask", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, NULL, 0);
}

// ---------------- MOTOR TASK ----------------
void motorTask(void * pvParameters) {

  const float MAX_RPM = 1200;
  static uint32_t lastFL = 0, lastFR = 0, lastBL = 0, lastBR = 0;

  for (;;) {

    int thr;
    float localAccX, localAccY;
    bool isFlying, isWater;

    portENTER_CRITICAL(&mux);
    thr = shared_thr;
    localAccX = accX;
    localAccY = accY;
    isFlying = shared_isFlying;
    isWater = shared_water;
    portEXIT_CRITICAL(&mux);

    if (thr > 5) {

      float baseRPM = map(thr, 5, 100, 60, 600);
      float manualBoost = baseRPM * 0.8;
      float Kp = baseRPM*0.06;
      float rpmFL = baseRPM, rpmFR = baseRPM;
      float rpmBL = baseRPM, rpmBR = baseRPM;

      bool fwd = digitalRead(BTN_FWD) == LOW;
      bool bck = digitalRead(BTN_BCK) == LOW;
      bool lft = digitalRead(BTN_LFT) == LOW;
      bool rgt = digitalRead(BTN_RGT) == LOW;
      bool manual = (fwd || bck || lft || rgt);

      // -------- LAND/WATER --------
      if (isWater || !isFlying) {

        digitalWrite(DIR_L, HIGH);
        digitalWrite(DIR_R, HIGH);

        shared_status = isWater ? "SAILING" : "DRIVING";

        if (manual) {
          if (fwd) { pulse(STEP_BL); pulse(STEP_BR); }
          else if (bck) { pulse(STEP_FL); pulse(STEP_FR); }
          else if (lft) { pulse(STEP_FR); pulse(STEP_BR); }
          else if (rgt) { pulse(STEP_FL); pulse(STEP_BL); }
        }

        vTaskDelay(1);
        continue;
      }

      // -------- AIR MODE --------
      digitalWrite(DIR_L, HIGH);
      digitalWrite(DIR_R, LOW);

      if (manual) {
        shared_status = "MANUAL";
        // -------- MANUAL CONTROL (PUSH-PULL MIXING) --------
        if (fwd) {
          // Pitch forward → boost back, reduce front
          rpmBL += manualBoost;
          rpmBR += manualBoost;
          rpmFL -= manualBoost * 0.5;
          rpmFR -= manualBoost * 0.5;
        }
        if (bck) {
          // Pitch backward → boost front, reduce back
          rpmFL += manualBoost;
          rpmFR += manualBoost;
          rpmBL -= manualBoost * 0.5;
          rpmBR -= manualBoost * 0.5;
        }
        if (lft) {
          // Roll left → boost right, reduce left
          rpmFR += manualBoost;
          rpmBR += manualBoost;
          rpmFL -= manualBoost * 0.5;
          rpmBL -= manualBoost * 0.5;
        }
        if (rgt) {
          // Roll right → boost left, reduce right
          rpmFL += manualBoost;
          rpmBL += manualBoost;
          rpmFR -= manualBoost * 0.5;
          rpmBR -= manualBoost * 0.5;
        }

      } else {
        shared_status = "STABLE";

        float pitchErr = localAccX;
        float rollErr  = localAccY;

        if (abs(pitchErr) < 0.2) pitchErr = 0;
        if (abs(rollErr)  < 0.2) rollErr = 0;

        float pitchCorr = pitchErr * Kp;
        float rollCorr  = rollErr  * Kp;

        rpmFL += pitchCorr - rollCorr;
        rpmFR += pitchCorr + rollCorr;
        rpmBL -= pitchCorr - rollCorr;
        rpmBR -= pitchCorr + rollCorr;

        stab_action = (pitchErr == 0 && rollErr == 0) ? "LEVEL" : "CORR";
      }

      // -------- CLAMP --------
      auto clamp = [&](float &r) {
        if (r > MAX_RPM) r = MAX_RPM;
        if (r < 20) r = 20;
      };

      clamp(rpmFL); clamp(rpmFR);
      clamp(rpmBL); clamp(rpmBR);

      // -------- STEP CONTROL (REAL FIX) --------
      uint32_t now = micros();

      int dFL = 60000000 / (rpmFL * TOTAL_STEPS);
      int dFR = 60000000 / (rpmFR * TOTAL_STEPS);
      int dBL = 60000000 / (rpmBL * TOTAL_STEPS);
      int dBR = 60000000 / (rpmBR * TOTAL_STEPS);

      if (dFL < 150 ) dFL = 150 ;
      if (dFR < 150 ) dFR = 150 ;
      if (dBL < 150 ) dBL = 150 ;
      if (dBR < 150 ) dBR = 150 ;

      if (now - lastFL >= dFL) { pulse(STEP_FL); lastFL = now; }
      if (now - lastFR >= dFR) { pulse(STEP_FR); lastFR = now; }
      if (now - lastBL >= dBL) { pulse(STEP_BL); lastBL = now; }
      if (now - lastBR >= dBR) { pulse(STEP_BR); lastBR = now; }

    } else {
      shared_status = "OFF";
      vTaskDelay(10);
    }

    vTaskDelay(1);
  }
}

// ---------------- SENSOR TASK ----------------
void sensorTask(void * pvParameters) {

  float fAccX = 0, fAccY = 0;

  for (;;) {

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    fAccX = 0.8 * fAccX + 0.2 * a.acceleration.x;
    fAccY = 0.8 * fAccY + 0.2 * a.acceleration.y;

    digitalWrite(TRIG, LOW); delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    float dist = pulseIn(ECHO, HIGH, 25000) * 0.034 / 2;
    float pres = bmp.readPressure() / 100.0;
    int thr = map(analogRead(POT_THR), 0, 4095, 0, 100);
    bool water = (digitalRead(BTN_WAT) == LOW);

    bool flying = (pres < 1000.0 && dist > 100.0);

    portENTER_CRITICAL(&mux);
    accX = fAccX;
    accY = fAccY;
    shared_dist = dist;
    shared_pres = pres;
    shared_thr = thr;
    shared_water = water;
    shared_isFlying = flying;
    portEXIT_CRITICAL(&mux);

    // -------- SERVO --------
    float angle = (water || !flying) ? 0 : 90;

    for (int i = 0; i < 2; i++) servos[i].write(180 - angle);
    for (int i = 2; i < 4; i++) servos[i].write(angle);

    // -------- OLED --------
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print(water ? "MODE: WATER" : (flying ? "MODE: AIR" : "MODE: LAND"));

    display.setCursor(0, 10);
    display.print("STAB: ");
    display.println(stab_action);

    display.setCursor(0, 25);
    display.print("ALT: ");
    display.print(dist, 0);
    display.print("cm");

    display.setCursor(0, 35);
    display.print("P: ");
    display.print(pres, 0);

    display.setCursor(0, 45);
    display.print("STAT:");
    display.println(shared_status);

    display.setCursor(0, 55);
    display.print("THR:");
    display.print(thr);
    display.print("%");

    display.display();

    vTaskDelay(100);
  }
}

// ---------------- STEP PULSE ----------------
void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(8);   // FIXED
  digitalWrite(pin, LOW);
}

void loop() {}