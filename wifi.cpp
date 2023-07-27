#include "devices.h"
#include "terminal.h"
#include <ESP8266WiFi.h>
//#include <WiFiUdp.h>
//WiFiUDP udp; 

void readWifiConfig() {
    File configFile = SPIFFS.open(configFilePath, "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return;
    }

    StaticJsonDocument<256> config;
    DeserializationError error = deserializeJson(config, configFile);
    configFile.close();

    if (error) {
        Serial.println("Failed to parse config file");
        return;
    }

    const char* ssid = config["ssid"];
    const char* password = config["password"];

    // Print SSID and password for verification (optional)
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);

    // Use the SSID and password to connect to WiFi
    WiFi.begin(ssid, password);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    uint8_t macAddress[6];
    WiFi.macAddress(macAddress);
    Serial.print("MAC Address: ");
    int wifiConnectloop = 10 ; 
    while (WiFi.status() != WL_CONNECTED) {
      
        Serial.print(".");
        wifiConnectloop--  ; 
        if (wifiConnectloop <= 0){
          Serial.println(" WiFi connection timeout! ");
          break ; 
        }
        delay(1000);
    }

    Serial.println("\nConnected to WiFi");

}
