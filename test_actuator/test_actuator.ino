#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin relay 4 channel (pastikan sesuai dengan wiring fisik)
const int relayPins[] = {21, 22, 4, 5};  // Urutan: Fan, Mist Maker, Water Pump, Grow Light
const String devices[] = {"DEV011", "DEV012", "DEV013", "DEV014"}; // Sesuaikan dengan deviceId di payload

const char* ssid = "Tselhome-7A14";
const char* password = "81190678";
const String baseURL = "https://ghouse.efrosine.my.id/api/actuators/payloads/";

// Timeout untuk HTTP request (ms)
const unsigned long HTTP_TIMEOUT = 10000;
const unsigned long UPDATE_INTERVAL = 2000;

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi pin relay
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // Matikan semua relay saat start
  }

  connectToWiFi();
}

void loop() {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      String payload = getPayload();
      if (!payload.isEmpty()) {
        processPayload(payload);
        printRelayStates();
      }
    } else {
      Serial.println("WiFi disconnected! Reconnecting...");
      connectToWiFi();
    }
    lastUpdate = millis();
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

String getPayload() {
  HTTPClient http;
  String payload = "";

  http.begin(baseURL);
  int httpCode = http.GET();

  // Handle HTTP response
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    Serial.println("Received payload:");
    Serial.println(payload);
  } else {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return payload;
}

void processPayload(const String &payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    Serial.println("Raw payload:");
    Serial.println(payload);
    return;
  }

  JsonArray devicesArray = doc.as<JsonArray>();
  
  Serial.println("Processing devices:");
  for (JsonObject device : devicesArray) {
    String deviceId = device["deviceId"].as<String>();
    String status = device["status"].as<String>(); // Diubah ke struktur JSON baru
    
    Serial.print("Found device: ");
    Serial.print(deviceId);
    Serial.print(" with status: ");
    Serial.println(status);

    // Cari index device yang sesuai
    for (int i = 0; i < 4; i++) {
      if (deviceId == devices[i]) {
        Serial.print("Matched device index: ");
        Serial.println(i);
        controlRelay(i, status);
        break;
      }
    }
  }
}

void controlRelay(int index, const String &status) {
  String lowerStatus = status;
  lowerStatus.toLowerCase(); // Handle case insensitive
  int state = (lowerStatus == "on") ? LOW : HIGH;
  digitalWrite(relayPins[index], state);
  
  Serial.printf("Device %s (%s): Relay pin %d set to %s\n",
                devices[index].c_str(),
                lowerStatus.c_str(),
                relayPins[index],
                state == LOW ? "ON" : "OFF");
}

// Fungsi baru untuk menampilkan status semua relay
void printRelayStates() {
  Serial.println("Current Relay States:");
  for (int i = 0; i < 4; i++) {
    Serial.printf("Relay %d (%s): %s\n", 
                 relayPins[i], 
                 devices[i].c_str(), 
                 digitalRead(relayPins[i]) == LOW ? "ON" : "OFF");
  }
  Serial.println("=====================");
}