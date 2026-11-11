#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// ========== CONFIGURATION ==========
const char* ssid = "iothealth";
const char* password = "12345678";

#define SDA_PIN 21
#define SCL_PIN 22
#define GSR_PIN 34
#define REPORTING_PERIOD_MS 1000
#define STRESS_MAX_RANGE 500.0  

// ========== GLOBAL VARIABLES ==========
PulseOximeter pox;
WebServer server(80);

float heartRate = 0;
float spo2 = 0;
float stress_percent = 0;
float filtered_adc = 0;
float baseline_adc = 0;
const float EMA_ALPHA = 0.1;
bool sensorOK = false;
uint32_t tsLastReport = 0;

// ========== CALLBACKS & HANDLERS ==========
void onBeatDetected() { Serial.println("💓 Beat!"); }

void handleData() {
  String json = "{\"hr\":" + String(heartRate) + 
                ",\"spo2\":" + String(spo2) + 
                ",\"stress\":" + String(stress_percent) + "}";
  server.send(200, "application/json", json);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(SDA_PIN, SCL_PIN);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // Silent GSR Calibration
  delay(2000); 
  long sum = 0;
  for(int i = 0; i < 100; i++) {
    sum += analogRead(GSR_PIN);
    delay(30);
  }
  baseline_adc = sum / 100;
  filtered_adc = baseline_adc;

  if (pox.begin()) {
    sensorOK = true;
    pox.setOnBeatDetectedCallback(onBeatDetected);
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  }

  server.on("/data", handleData);
  server.begin();
  Serial.println(WiFi.localIP());
}

// ========== LOOP ==========
void loop() {
  
  if (sensorOK) pox.update();
  server.handleClient();

  // GSR Logic to Calculate Stress Percentage
  static uint32_t lastGSR = 0;
  if (millis() - lastGSR >= 50) {
    int raw_val = analogRead(GSR_PIN);
    filtered_adc = (EMA_ALPHA * raw_val) + ((1.0 - EMA_ALPHA) * filtered_adc);
    float delta = filtered_adc - baseline_adc;
    stress_percent = (delta / STRESS_MAX_RANGE) * 100.0;
    if (stress_percent > 100) stress_percent = 100;
    if (stress_percent < -100) stress_percent = -100;
    lastGSR = millis();
  }

  // Update biometric data indicators locally
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    heartRate = pox.getHeartRate();
    spo2 = pox.getSpO2();
    tsLastReport = millis();
  }
}