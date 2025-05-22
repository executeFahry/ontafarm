#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
#include <BH1750.h>

// Pin sensor DHT22
#define DHTPIN1 4
#define DHTPIN2 5
#define DHTTYPE DHT22

DHT dhtSensors[] = {DHT(DHTPIN1, DHTTYPE), DHT(DHTPIN2, DHTTYPE)};

// Sensor BH1750
BH1750 lightSensors[] = {BH1750(0x23), BH1750(0x5C)};

// ADS1115
Adafruit_ADS1115 ads;

const int minValue = 6300;
const int maxValue = 13200;

const char* ssid = "Tselhome-7A14";
const char* password = "81190678";

const String baseURL = "https://ghouse.efrosine.my.id/api/payloads/";

// Perbaikan urutan Device IDs
const String deviceIds[] = {
    "DEV001", "DEV003",  // DHT22 pertama: Temp1(DEV001), Hum1(DEV003)
    "DEV002", "DEV004",  // DHT22 kedua: Temp2(DEV002), Hum2(DEV004)
    "DEV005", "DEV006", "DEV007", "DEV008",  // Soil sensors
    "DEV009", "DEV010"   // Light sensors
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  for (int i = 0; i < 2; i++) {
    dhtSensors[i].begin();
  }

  Wire.begin(21, 22);
  ads.begin();
  
  for (int i = 0; i < 2; i++) {
    lightSensors[i].begin();
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  float temperatures[2], humidities[2];
  for (int i = 0; i < 2; i++) {
    temperatures[i] = dhtSensors[i].readTemperature();
    humidities[i] = dhtSensors[i].readHumidity();
  }

  int16_t soilMoisture[4];
  for (int i = 0; i < 4; i++) {
    soilMoisture[i] = ads.readADC_SingleEnded(i);
  }

  float percentSoil[4];
  for (int i = 0; i < 4; i++) {
    percentSoil[i] = mapToPercentage(soilMoisture[i]);
  }

  uint16_t lightLevels[2];
  for (int i = 0; i < 2; i++) {
    lightLevels[i] = lightSensors[i].readLightLevel();
  }

  // Perbaikan pengiriman data sesuai payload
  // DHT Sensors
  sendSensorData("DEV001", "temperature", "celsius", temperatures[0]);
  sendSensorData("DEV003", "humidity", "percent", humidities[0]);
  sendSensorData("DEV002", "temperature", "celsius", temperatures[1]);
  sendSensorData("DEV004", "humidity", "percent", humidities[1]);

  // Soil Sensors
  for (int i = 0; i < 4; i++) {
    sendSensorData(deviceIds[i + 4], "soil_moisture", "percent", percentSoil[i]);
  }

  // Light Sensors
  for (int i = 0; i < 2; i++) {
    sendSensorData(deviceIds[i + 8], "light_level", "lux", lightLevels[i]);
  }

  delay(5000);
}

void sendSensorData(const String& deviceId, const String& type, const String& unit, float value) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(baseURL + deviceId);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(256);
  doc["deviceId"] = deviceId;  // Tambahkan deviceId di root object
  JsonObject data = doc.createNestedObject("data");
  data["type"] = type;
  data["unit"] = unit;
  data["value"] = value;

  String jsonData;
  serializeJson(doc, jsonData);

  // Debug: Tampilkan JSON yang dikirim
  Serial.print("Mengirim ke ");
  Serial.print(deviceId);
  Serial.print(": ");
  Serial.println(jsonData);

  int httpCode = http.PUT(jsonData);
  
  if (httpCode <= 0) {
    Serial.print("Error ");
    Serial.println(httpCode);
  }
  
  http.end();
}

float mapToPercentage(int adcValue) {
  adcValue = constrain(adcValue, minValue, maxValue);
  return 100.0 - ((float)(adcValue - minValue) / (maxValue - minValue)) * 100.0;
}