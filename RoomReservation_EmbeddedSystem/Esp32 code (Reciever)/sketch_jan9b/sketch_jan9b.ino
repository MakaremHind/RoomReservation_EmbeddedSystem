#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid = "Galaxy A34 5G 95F7";                  // Wi-Fi SSID (name)
const char* password = "1234567891";                      // Wi-Fi password

// Server details for the API
const char* serverName = "https://roomreservation.cleverapps.io/access";

// Pin definitions
#define RXp2 16                                           // RX pin for Serial2 communication
#define TXp2 17                                           // TX pin for Serial2 communication
#define BUTTON_ROOM 4                                     // Button to toggle room selection
#define ACCESS_LED 18                                     // LED to indicate access granted
#define DENIED_LED 19                                     // LED to indicate access denied

long roomId = 1;                                          // Initial room ID (default to Room 1)

void setup() {
  Serial.begin(115200);                                   // Initialize Serial monitor
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);           // Initialize Serial2 for RFID reader communication

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {                // Wait until Wi-Fi is connected
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  // Configure pins
  pinMode(BUTTON_ROOM, INPUT_PULLDOWN);                   // Configure button as input with pulldown resistor
  pinMode(ACCESS_LED, OUTPUT);                            // Configure access LED as output
  pinMode(DENIED_LED, OUTPUT);                            // Configure denied LED as output
}

void loop() {
  // Check if the button is pressed to toggle the room ID
  if (digitalRead(BUTTON_ROOM)) {
    roomId = (roomId == 1) ? 2 : 1;                       // Toggle between Room 1 and Room 2
    delay(200);                                           // Debounce delay
    Serial.println(roomId == 1 ? "Room 1 Selected" : "Room 2 Selected");
  }

  // Check if data is available from Serial2 (RFID reader)
  if (Serial2.available()) {
    String receivedData = Serial2.readString();           // Read the incoming data
    receivedData.trim();                                  // Remove any leading or trailing whitespace

    Serial.print("Received Raw Data: ");
    Serial.println(receivedData);

    // Extract the Tag UID from the received data
    int startIndex = receivedData.indexOf("Tag UID:");
    if (startIndex == -1) {                               // Check alternative label
      startIndex = receivedData.indexOf("TagUID:");
    }

    if (startIndex == -1) {                               // No valid Tag UID found
      Serial.println("No valid Tag UID found. Skipping...");
      return;
    }

    String tagUID = receivedData.substring(startIndex + 8); // Extract the UID
    tagUID.trim();                                        // Remove extra spaces
    tagUID.replace(" ", "");                             // Remove spaces within the UID

    if (tagUID.length() < 8) {                            // Validate UID length
      Serial.println("Invalid UID length. Skipping...");
      return;
    }

    Serial.print("Extracted and Formatted Tag UID: ");
    Serial.println(tagUID);

    sendToAPI(tagUID, roomId);                            // Send UID and room ID to the API
  }

  delay(2000);                                            // Short delay before next loop iteration
}

void sendToAPI(String tagUID, long roomId) {
  if (WiFi.status() == WL_CONNECTED) {                   // Ensure Wi-Fi is connected
    HTTPClient http;

    // Create JSON payload
    String jsonPayload = "{\"cardNumber\":\"" + tagUID + "\",\"roomId\":" + String(roomId) + "}";
    Serial.print("Payload: ");
    Serial.println(jsonPayload);

    // Send POST request to the server
    http.begin(serverName);                              // Initialize connection to server
    http.addHeader("Content-Type", "application/json");  // Set request header
    int httpResponseCode = http.POST(jsonPayload);       // Send JSON payload

    if (httpResponseCode > 0) {                          // Check server response
      String response = http.getString();                // Read server response
      Serial.print("Server Response: ");
      Serial.println(response);

      // Parse JSON response
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);

      if (error) {                                       // Handle JSON parsing errors
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
        return;
      }

      // Process access control response
      bool accessGranted = doc["accessGranted"];
      if (accessGranted) {
        Serial.println("Access granted!");
        digitalWrite(ACCESS_LED, HIGH);                  // Turn on access LED
        delay(3000);                                     // Keep LED on for 3 seconds
        digitalWrite(ACCESS_LED, LOW);                   // Turn off access LED
      } else {
        Serial.println("Access denied!");
        digitalWrite(DENIED_LED, HIGH);                  // Turn on denied LED
        delay(3000);                                     // Keep LED on for 3 seconds
        digitalWrite(DENIED_LED, LOW);                   // Turn off denied LED
      }

    } else {
      Serial.print("HTTP Error Code: ");                 // Handle HTTP errors
      Serial.println(httpResponseCode);
    }

    http.end();                                          // End HTTP connection
  } else {
    Serial.println("Wi-Fi not connected!");              // Handle Wi-Fi disconnection
  }
}
