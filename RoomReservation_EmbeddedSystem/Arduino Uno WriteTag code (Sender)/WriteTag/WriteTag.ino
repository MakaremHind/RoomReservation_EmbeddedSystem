#include <NfcAdapter.h>
#include <PN532/PN532/PN532.h>
#include <SPI.h>
#include <PN532/PN532_SPI/PN532_SPI.h>
#include <SoftwareSerial.h> // Include SoftwareSerial library

// NFC Setup
PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);

// Software Serial for communication with ESP32
SoftwareSerial mySerial(10, 11); // RX, TX (choose free digital pins)

void setup(void) {
    Serial.begin(9600);      // Debugging via Serial Monitor
    mySerial.begin(9600);    // UART communication with ESP32
    Serial.println("NDEF Reader Initialized");
    nfc.begin();             // Initialize NFC reader
}

void loop(void) {
    Serial.println("\nScan an NFC tag...");

    if (nfc.tagPresent()) {          // Check if an NFC tag is present
        NfcTag tag = nfc.read();    // Read NFC tag
        String tagData = tag.getUidString(); // Get UID of the tag as String

        // Debugging output to Serial Monitor
        Serial.print("Tag UID: ");
        Serial.println(tagData);

        // Send tag data to ESP32 via SoftwareSerial
        mySerial.println(tagData); // Use mySerial instead of Serial1
    }
    delay(5000); // Wait 5 seconds before reading again
}
