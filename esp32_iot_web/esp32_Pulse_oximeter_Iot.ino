#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===================== EASY TO REPLACE CONFIG =====================
// WiFi credentials: replace with your own Wi-Fi SSID and password
const char* ssid     = "your Wi-Fi network name (SSID)";
const char* password = "your Wi-Fi password";

// ThingSpeak Write API Key: replace with YOUR channel's Write API Key
const char* THINGSPACE_WRITE_KEY = "your ThingSpeak Write API Key (from Channel > API Keys)";

// ThingSpeak URL (do NOT change this)
const char* THINGSPACE_URL = "https://api.thingspeak.com/update";

// Device metadata (optional, for your own reference)
const char* DEVICE_ID   = "ESP32_OXIMETER_01";   // optional, your device ID name
const char* DEVICE_NAME = "PulseOximeter";       // optional, your device display name

// Cloud send behavior
const unsigned long CLOUD_SEND_INTERVAL_MS = 15000;  // delay between cloud send attempts (ms)
bool ENABLE_CLOUD_UPLOAD = true;                     // set false to disable ThingSpeak uploads
// ==================================================================

PulseOximeter pox;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define REPORTING_PERIOD_MS 1000
#define SAMPLE_COUNT 12

uint32_t lastSampleTime   = 0;
uint32_t lastDisplayTime  = 0;
uint32_t lastCloudSendTime = 0;

float hrSamples[SAMPLE_COUNT];
float spo2Samples[SAMPLE_COUNT];
int   sampleIndex = 0;

float finalBPM  = 0.0;
float finalSpO2 = 0.0;

bool measurementDone = false;
bool fingerDetected  = false;
bool cloudSent       = false;
String cloudStatusMsg = "Not sent";

// ----------------- Helper functions -----------------

float averageArray(float *arr, int n) {
  float sum = 0;
  for (int i = 0; i < n; i++) sum += arr[i];
  return sum / n;
}

void sortArray(float arr[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (arr[j] < arr[i]) {
        float t = arr[i];
        arr[i]  = arr[j];
        arr[j]  = t;
      }
    }
  }
}

float trimmedMean(float arr[], int n, int trimCount) {
  if (n <= 2 * trimCount) return averageArray(arr, n);
  sortArray(arr, n);
  float sum = 0;
  for (int i = trimCount; i < n - trimCount; i++) {
    sum += arr[i];
  }
  return sum / (n - 2 * trimCount);
}

void showLCD(const String &line1, const String &line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ThingSpeak upload function: sends BPM to field1, SpO2 to field2
bool sendToCloudThingSpeak(float bpm, float spo2) {
  if (!ENABLE_CLOUD_UPLOAD) return false;
  if (WiFi.status() != WL_CONNECTED) {
    cloudStatusMsg = "WiFi not connected";
    return false;
  }

  HTTPClient http;

  // Build ThingSpeak URL with API key and fields
  // field1 = Heart Rate (BPM), field2 = SpO2 (%)
  String url = String(THINGSPACE_URL) +
               "?api_key=" + String(THINGSPACE_WRITE_KEY) +
               "&field1=" + String(bpm, 1) +
               "&field2=" + String(spo2, 1);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");  // harmless for GET

  int httpCode   = http.GET();
  String response = http.getString();
  http.end();

  if (httpCode > 0 && httpCode >= 200 && httpCode < 300) {
    cloudStatusMsg = "ThingSpeak OK";
    Serial.println("ThingSpeak upload success");
    Serial.println(response);  // ThingSpeak returns the row ID
    return true;
  } else {
    cloudStatusMsg = "ThingSpeak failed";
    Serial.print("ThingSpeak upload failed: ");
    Serial.println(httpCode);
    Serial.println(response);
    return false;
  }
}

// ----------------- Setup -----------------

void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.init();
  lcd.backlight();
  showLCD("Starting...", "");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  showLCD("Connecting WiFi", "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    lcd.print(".");
  }

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  showLCD("WiFi Connected", WiFi.localIP().toString());
  delay(1500);

  // Initialize MAX30100 pulse oximeter sensor
  if (!pox.begin()) {
    showLCD("Sensor FAIL", "Check wiring");
    while (1) {}
  }

  showLCD("Place finger", "On sensor");
}

// ----------------- Main loop -----------------

void loop() {
  pox.update();

  // ---------- Measurement logic ----------
  if (!measurementDone) {
    float hr   = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    // Check if reading looks valid
    fingerDetected = (hr > 30.0 && hr < 220.0 && spo2 > 50.0 && spo2 <= 100.0);

    if (millis() - lastSampleTime >= REPORTING_PERIOD_MS) {
      lastSampleTime = millis();

      if (fingerDetected) {
        hrSamples[sampleIndex]   = hr;
        spo2Samples[sampleIndex] = spo2;
        sampleIndex++;

        if (sampleIndex >= SAMPLE_COUNT) {
          // Compute trimmed mean to reduce outliers
          finalBPM  = trimmedMean(hrSamples,  SAMPLE_COUNT, 2);
          finalSpO2 = trimmedMean(spo2Samples, SAMPLE_COUNT, 2);
          measurementDone = true;
          cloudSent       = false;

          showLCD("Result Ready", "Stored");
          Serial.print("Final HR: ");
          Serial.println(finalBPM);
          Serial.print("Final SpO2: ");
          Serial.println(finalSpO2);

        } else {
          showLCD("Measuring...", "Hold still");
        }
      } else {
        sampleIndex = 0;
        showLCD("Place finger", "On sensor");
      }
    }
  }

  // ---------- Display final result on LCD ----------
  if (measurementDone && millis() - lastDisplayTime >= 1000) {
    lastDisplayTime = millis();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HR:");
    lcd.print(finalBPM, 1);
    lcd.print(" BPM");

    lcd.setCursor(0, 1);
    lcd.print("SpO2:");
    lcd.print(finalSpO2, 1);
    lcd.print("%");
  }

  // ---------- Send data to ThingSpeak once per measurement ----------
  if (measurementDone && !cloudSent && millis() - lastCloudSendTime >= CLOUD_SEND_INTERVAL_MS) {
    lastCloudSendTime = millis();
    cloudSent = sendToCloudThingSpeak(finalBPM, finalSpO2);

    if (cloudSent) {
      showLCD("ThingSpeak", "Success");
    } else {
      showLCD("ThingSpeak", "Failed");
    }

    // After cloud send, you can reset for next measurement if you want:
    // measurementDone = false;
    // sampleIndex = 0;
  }
}