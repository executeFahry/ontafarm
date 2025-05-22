#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
#include <BH1750.h>  // Library untuk sensor BH1750

// Pin sensor DHT22
#define DHTPIN1 4          // Pin untuk DHT22 pertama
#define DHTPIN2 5          // Pin untuk DHT22 kedua
#define DHTTYPE DHT22

DHT dhtSensors[] = {DHT(DHTPIN1, DHTTYPE), DHT(DHTPIN2, DHTTYPE)};  // Array untuk DHT22 sensor

// Inisialisasi sensor BH1750 untuk Light Intensity
BH1750 lightSensors[] = {BH1750(0x23), BH1750(0x5C)};  // Sensor BH1750 pertama dan kedua (alamat I2C)

// Inisialisasi ADS1115 untuk sensor soil moisture (I2C)
Adafruit_ADS1115 ads;

// Inisialisasi pin untuk soil moisture sensor (kapasitif)
#define SOIL_SENSOR_PIN 35  // Pin analog untuk soil moisture sensor

// Nilai ADC untuk kondisi kering dan basah
const int minValue = 6300;  // Nilai ADC saat tanah basah
const int maxValue = 13200; // Nilai ADC saat tanah kering

const char* ssid = "Bukan Kos Kosan";
const char* password = "MbohGakEroh";

// Base URL endpoint API
const String baseURL = "https://iotmonitor.efrosine.my.id/api/payloads/";

// Device IDs untuk masing-masing sensor
const String deviceIds[] = {
    "DEV001", "DEV002",  // DHT22 pertama
    "DEV003", "DEV004",  // DHT22 kedua
    "DEV009", "DEV010", "DEV011", "DEV012",  // Soil sensors
    "DEV013", "DEV014"   // Light sensors (BH1750)
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Inisialisasi DHT22 sensors
  for (int i = 0; i < 2; i++) {
    dhtSensors[i].begin();
  }

  // Inisialisasi I2C untuk ADS1115 dan BH1750 (SDA, SCL)
  Wire.begin(21, 22);  // Pin SDA dan SCL untuk ESP32

  // Inisialisasi ADS1115
  ads.begin();
  
  // Inisialisasi BH1750 sensors
  for (int i = 0; i < 2; i++) {
    lightSensors[i].begin();
  }

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}

void loop() {
  // Membaca data dari sensor DHT22 pertama dan kedua (temperature & humidity)
  float temperatures[2], humidities[2];
  for (int i = 0; i < 2; i++) {
    temperatures[i] = dhtSensors[i].readTemperature();  // Celsius
    humidities[i] = dhtSensors[i].readHumidity();        // Kelembaban Udara
  }

  // Membaca data dari soil moisture sensor menggunakan ADS1115
  int16_t soilMoisture[4];
  for (int i = 0; i < 4; i++) {
    soilMoisture[i] = ads.readADC_SingleEnded(i);  // Membaca nilai soil moisture dari masing-masing kanal
  }

  // Membaca data dari kedua sensor BH1750 (Light Intensity)
  uint16_t lightLevels[2];
  for (int i = 0; i < 2; i++) {
    lightLevels[i] = lightSensors[i].readLightLevel();  // Nilai light level dalam lux
  }

  // Menghitung persentase kelembapan tanah berdasarkan nilai ADC
  float percentSoil[4];
  for (int i = 0; i < 4; i++) {
    percentSoil[i] = mapToPercentage(soilMoisture[i]);
  }

  // Cek apakah sensor gagal membaca
  for (int i = 0; i < 2; i++) {
    if (isnan(temperatures[i]) || isnan(humidities[i])) {
      Serial.println("Failed to read from DHT sensor!");
      return;  // Menghentikan eksekusi loop jika sensor gagal dibaca
    }
  }

  // Menampilkan hasil baca sensor
  Serial.println("Temperature and Humidity from DHT22:");
  for (int i = 0; i < 2; i++) {
    Serial.print("Temperature ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(temperatures[i]);
    Serial.println("°C");
    Serial.print("Humidity ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(humidities[i]);
    Serial.println("%");
  }

  Serial.println("Soil Moisture Sensor Data:");
  for (int i = 0; i < 4; i++) {
    Serial.print("Soil Moisture ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(soilMoisture[i]);
    Serial.print(" (");
    Serial.print(percentSoil[i]);
    Serial.println("%)");
  }

  Serial.println("Light Intensity (from BH1750):");
  for (int i = 0; i < 2; i++) {
    Serial.print("Light Level ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(lightLevels[i]);
    Serial.println(" lux");
  }

  Serial.println("");

  // Kirim data suhu, kelembaban, dan light level ke masing-masing device
  for (int i = 0; i < 2; i++) {
    sendSensorData(deviceIds[i], "celsius", "temperature", temperatures[i]);
    sendSensorData(deviceIds[i + 2], "percent", "humidity", humidities[i]);
  }

  for (int i = 0; i < 4; i++) {
    sendSensorData(deviceIds[i + 4], "percent", "moisture", percentSoil[i]);
  }

  for (int i = 0; i < 2; i++) {
    sendSensorData(deviceIds[i + 8], "lux", "light_level", lightLevels[i]);
  }

  delay(5000);  // Kirim data setiap 5 detik
}

void sendSensorData(const String& deviceId, const String& unit, const String& key, float value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = baseURL + deviceId;

    http.begin(url);  // Menyiapkan HTTP request ke endpoint tertentu
    http.addHeader("Content-Type", "application/json");

    // Membuat objek JSON untuk data yang akan dikirim
    DynamicJsonDocument doc(200);

    doc["data"]["unit"] = unit;
    doc["data"][key] = value;

    String jsonData;
    serializeJson(doc, jsonData);  // Serialize JSON ke string

    int httpResponseCode = http.PUT(jsonData);
    if (httpResponseCode > 0) {
      // Serial.print("PUT to ");
      // Serial.print(url);
      // Serial.print(" => Code: ");
      // Serial.println(httpResponseCode);
      // String response = http.getString();
      // Serial.println("Response: " + response);
    } else {
      Serial.print("POST failed. Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }
}

// Fungsi untuk memetakan nilai ADC ke persentase kelembapan
float mapToPercentage(int adcValue) {
  // Pemetaan nilai ADC ke dalam rentang 0 - 100%, membalikkan nilai
  return 100.0 - (float)(adcValue - minValue) / (maxValue - minValue) * 100.0;
}
