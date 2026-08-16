/*
  Smart Home Controller - Embedded Systems Project
  Board: Arduino UNO
  Simulation-safe outputs: LEDs represent room light and fan.
  Sensors: PIR + LDR + DHT11
  Display: 16x2 I2C LCD
*/

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- PIN CONFIGURATION ----------------
#define PIR_PIN           2
#define DHT_PIN           3
#define SECURITY_SW_PIN   4
#define MANUAL_LIGHT_PIN  5
#define MANUAL_FAN_PIN    6

#define LIGHT_PIN         8
#define FAN_PIN           9
#define BUZZER_PIN        10
#define RED_LED_PIN       11
#define GREEN_LED_PIN     12

#define LDR_PIN           A0

#define DHT_TYPE DHT11

// ---------------- CONFIGURATION ----------------
const int DARK_THRESHOLD = 500;
const float FAN_ON_TEMP = 30.0;
const float FAN_OFF_TEMP = 28.0;
const unsigned long NO_MOTION_TIMEOUT = 10000UL; // 10 seconds for demo

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- STATE VARIABLES ----------------
bool securityMode = false;
bool manualMode = false;

bool motionDetected = false;
bool lightOn = false;
bool fanOn = false;
bool alarmOn = false;

int ldrValue = 0;
float temperature = 0.0;
unsigned long lastMotionTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;

// ---------------- SENSOR FUNCTIONS ----------------
void readSensors() {
  motionDetected = digitalRead(PIR_PIN) == HIGH;
  ldrValue = analogRead(LDR_PIN);

  float t = dht.readTemperature();
  if (!isnan(t)) {
    temperature = t;
  }

  if (motionDetected) {
    lastMotionTime = millis();
  }

  securityMode = digitalRead(SECURITY_SW_PIN) == LOW;
  manualMode = digitalRead(MANUAL_LIGHT_PIN) == LOW ||
               digitalRead(MANUAL_FAN_PIN) == LOW;
}

bool isDark() {
  return ldrValue < DARK_THRESHOLD;
}

// ---------------- AUTOMATION ----------------
void controlLight() {
  if (manualMode) {
    // Manual mode has priority over automatic appliance logic.
    lightOn = digitalRead(MANUAL_LIGHT_PIN) == LOW;
    return;
  }

  if (motionDetected && isDark()) {
    lightOn = true;
  } else if (millis() - lastMotionTime > NO_MOTION_TIMEOUT) {
    lightOn = false;
  }
}

void controlFan() {
  if (manualMode) {
    fanOn = digitalRead(MANUAL_FAN_PIN) == LOW;
    return;
  }

  // Hysteresis prevents rapid ON/OFF switching near the threshold.
  if (!fanOn && temperature >= FAN_ON_TEMP) {
    fanOn = true;
  } else if (fanOn && temperature <= FAN_OFF_TEMP) {
    fanOn = false;
  }
}

void controlSecurity() {
  alarmOn = securityMode && motionDetected;
}

// ---------------- OUTPUTS ----------------
void updateOutputs() {
  digitalWrite(LIGHT_PIN, lightOn ? HIGH : LOW);
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);

  digitalWrite(BUZZER_PIN, alarmOn ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, alarmOn ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, alarmOn ? LOW : HIGH);
}

// ---------------- DISPLAY ----------------
void updateDisplay() {
  if (millis() - lastDisplayUpdate < 1000) return;
  lastDisplayUpdate = millis();

  lcd.clear();

  if (alarmOn) {
    lcd.setCursor(0, 0);
    lcd.print("INTRUDER ALERT");
    lcd.setCursor(0, 1);
    lcd.print("SECURITY ACTIVE");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C L:");
  lcd.print(lightOn ? "ON " : "OFF");

  lcd.setCursor(0, 1);
  lcd.print("F:");
  lcd.print(fanOn ? "ON " : "OFF");
  lcd.print(" M:");
  lcd.print(motionDetected ? "YES" : "NO ");
}

// ---------------- SERIAL MONITOR ----------------
void printStatus() {
  if (millis() - lastSerialUpdate < 2000) return;
  lastSerialUpdate = millis();

  Serial.println("------------- SMART HOME -------------");
  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("LDR: ");
  Serial.print(ldrValue);
  Serial.print(" -> Room: ");
  Serial.println(isDark() ? "DARK" : "BRIGHT");

  Serial.print("Motion: ");
  Serial.println(motionDetected ? "DETECTED" : "NONE");

  Serial.print("Light: ");
  Serial.println(lightOn ? "ON" : "OFF");

  Serial.print("Fan: ");
  Serial.println(fanOn ? "ON" : "OFF");

  Serial.print("Security: ");
  Serial.println(securityMode ? "ARMED" : "SAFE");

  Serial.print("Alarm: ");
  Serial.println(alarmOn ? "ON" : "OFF");

  Serial.print("Mode: ");
  Serial.println(manualMode ? "MANUAL" : "AUTOMATIC");
  Serial.println("--------------------------------------");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(SECURITY_SW_PIN, INPUT_PULLUP);
  pinMode(MANUAL_LIGHT_PIN, INPUT_PULLUP);
  pinMode(MANUAL_FAN_PIN, INPUT_PULLUP);

  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home");
  lcd.setCursor(0, 1);
  lcd.print("Controller Ready");

  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(1500);
  lcd.clear();
}

// ---------------- MAIN LOOP ----------------
void loop() {
  readSensors();

  controlLight();
  controlFan();
  controlSecurity();

  updateOutputs();
  updateDisplay();
  printStatus();

  delay(50);
}