#include "terminal.h"
#include "devices.h"
#include <TimeLib.h>

const int ledPin = D0;
unsigned long previousMillis = 0;
const long Led_DO_interval = 500; // LED blink interval in milliseconds

void setup() {
    pinMode(ledPin, OUTPUT); // Set D0 as OUTPUT for the LED
    digitalWrite(ledPin, LOW); // Turn off the LED initially
    Serial.begin(115200);
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }
    loadConfigFromFile(configFilePath, config);
    loadConfigFromFile(systemConfigFilePath, systemConfig);

    // Set serial speed from systemConfig, defaulting to 115200 if not present
    String serialSpeedStr = systemConfig["serial_speed"].as<String>();
    long serialSpeed = serialSpeedStr.toInt();
    if (serialSpeed == 0) {
        serialSpeed = 115200; // Default to 115200 if not a valid number
    }
    Serial.begin(serialSpeed);

    displayPrompt();
    readWifiConfig();
    connectToWiFi();


}

void loop(){
    if (Serial.available() > 0) {
        handleSerialInput();
    }
    
    // Call the blinkLED() function to blink the LED
    blinkLED();
    
}

void blinkLED() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= Led_DO_interval ) {
        // Save the last time the LED was updated
        previousMillis = currentMillis;
        //  Serial.println(  currentMillis ); 
        // Toggle the LED state
        
        if (digitalRead(ledPin) == LOW) {
            digitalWrite(ledPin, HIGH);
            
        } else {
            digitalWrite(ledPin, LOW);
        }
    }
}
