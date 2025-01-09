#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Galaxy A34 5G 95F7";                  
const char* password = "1234567891";       

const char* serverName = "https://roomreservation.cleverapps.io/access";

#define RXp2 16
#define TXp2 17
#define BUTTON_ROOM 4
#define ACCESS_LED 18
#define DENIED_LED 19

long roomId = 1;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  pinMode(BUTTON_ROOM, INPUT_PULLDOWN);
  pinMode(ACCESS_LED, OUTPUT);
  pinMode(DENIED_LED, OUTPUT);
}

void loop() {
  if (digitalRead(BUTTON_ROOM)) {
    roomId = (roomId == 1) ? 2 : 1;
    delay(200);
    Serial.println(roomId == 1 ? "Room 1 Selected" : "Room 2 Selected");
  }

  if (Serial2.available()) {
    String receivedData = Serial2.readString();
    receivedData.trim();

    Serial.print("Received Raw Data: ");
    Serial.println(receivedData);

    int startIndex = receivedData.indexOf("Tag UID:");
    if (startIndex == -1) {
      startIndex = receivedData.indexOf("TagUID:");
    }

    if (startIndex == -1) {
      Serial.println("No valid Tag UID found. Skipping...");
      return;
    }

    String tagUID = receivedData.substring(startIndex + 8);
    tagUID.trim();
    tagUID.replace(" ", "");

    if (tagUID.length() < 8) {
      Serial.println("Invalid UID length. Skipping...");
      return;
    }

    Serial.print("Extracted and Formatted Tag UID: ");
    Serial.println(tagUID);

    sendToAPI(tagUID, roomId);
  }

  delay(2000);
}

void sendToAPI(String tagUID, long roomId) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String jsonPayload = "{\"cardNumber\":\"" + tagUID + "\",\"roomId\":" + String(roomId) + "}";
    Serial.print("Payload: ");
    Serial.println(jsonPayload);

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Server Response: ");
      Serial.println(response);

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);

      if (error) {
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
        return;
      }

      bool accessGranted = doc["accessGranted"];
      if (accessGranted) {
        Serial.println("Access granted!");
        digitalWrite(ACCESS_LED, HIGH);
        delay(3000);
        digitalWrite(ACCESS_LED, LOW);
      } else {
        Serial.println("Access denied!");
        digitalWrite(DENIED_LED, HIGH);
        delay(3000);
        digitalWrite(DENIED_LED, LOW);
      }

    } else {
      Serial.print("HTTP Error Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected!");
  }
}
