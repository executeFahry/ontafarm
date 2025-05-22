#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin untuk LED dan Buzzer
#define RELAY_PINS {18, 19, 21, 14}  // Pin untuk LED dan Buzzer
int actuatorPins[] = RELAY_PINS;  // Array untuk pin

const char* ssid = "Bukan Kos Kosan";
const char* password = "MbohGakEroh";

// Base URL endpoint API
const String baseURL = "https://iotmonitor.efrosine.my.id/api/payloads/";

// Device IDs untuk masing-masing dummy actuator
const String devices[] = {"DEV005", "DEV006", "DEV007", "DEV008"};  // LED, Fan, Water Pump, Mist Maker

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Inisialisasi pin untuk LED dan Buzzer
  for (int i = 0; i < 4; i++) {
    pinMode(actuatorPins[i], OUTPUT);
    digitalWrite(actuatorPins[i], LOW);  // Matikan semua aktuator pada awalnya
  }

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}

void loop() {
// Mengambil data status dari API untuk semua aktuator
  for (int i = 0; i < 4; i++) {
    getActuatorStatus(devices[i], actuatorPins[i]);
  }

  delay(5000);  // Mengecek status setiap 5 detik
}

void getActuatorStatus(const String& deviceId, int actuatorPin) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = baseURL + deviceId;

    http.begin(url);  // Menyiapkan HTTP request ke endpoint tertentu
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {  // Pastikan response berhasil (HTTP 200)
      String response = http.getString();
      // Serial.println("Response: " + response);  // Debug: Tampilkan response API

      // Parse JSON response
      DynamicJsonDocument doc(200);
      DeserializationError error = deserializeJson(doc, response);

      if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.f_str());
        return;
      }

      // Mengakses data dari response JSON
      JsonObject data = doc[0]["data"];

      // Tampilkan data yang diterima dari API
      if (data.containsKey("status")) {
        String status = data["status"];
        controlActuator(status, actuatorPin);  // Kontrol Aktuator berdasarkan status
      }
    } else {
      Serial.print("GET failed. Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }
}

void controlActuator(const String& status, int actuatorPin) {
  if (status == "on") {
    if (actuatorPin == 14) {
      tone(actuatorPin, 500);  // Nyalakan buzzer dengan tone 500Hz
      Serial.println("Mist Maker turned ON (Buzzer)");
    } else {
      digitalWrite(actuatorPin, HIGH);  // Nyalakan LED
      Serial.println("Actuator turned ON (LED)");
    }
  } else if (status == "off") {
    if (actuatorPin == 14) {
      noTone(actuatorPin);  // Matikan buzzer
      Serial.println("Mist Maker turned OFF (Buzzer)");
      Serial.println("");
    } else {
      digitalWrite(actuatorPin, LOW);   // Matikan LED
      Serial.println("Actuator turned OFF (LED)");
    }
  } else {
    Serial.println("Invalid status");
  }
}