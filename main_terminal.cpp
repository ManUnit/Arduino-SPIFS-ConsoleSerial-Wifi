#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "terminal.h"
#include "devices.h"
#include <ESP8266WiFi.h>
#include <stdint.h>
 #include <WiFiUdp.h>

const char *configFilePath = "/app.conf";
const char *systemConfigFilePath = "/sysconfig.conf";
const int enablePasswordAddr = 0;
String inputLine;
StaticJsonDocument<512> config;
StaticJsonDocument<512> systemConfig;
bool enableMode = false; //
bool waitingForPassword = false;
unsigned long passwordInputStartTime = 0;

WiFiUDP ntpUDP;


// ============= Body =========== //

void showWifiStatus()
{
    int status = WiFi.status();
    switch (status)
    {
    case WL_CONNECTED:
        Serial.println("WiFi connected.");
        break;
    case WL_NO_SHIELD:
        Serial.println("WiFi shield not present.");
        break;
    case WL_IDLE_STATUS:
        Serial.println("WiFi idle.");
        break;
    case WL_NO_SSID_AVAIL:
        Serial.println("No WiFi networks available.");
        break;
    case WL_SCAN_COMPLETED:
        Serial.println("WiFi scan completed.");
        break;
    case WL_CONNECT_FAILED:
        Serial.println("Failed to connect to WiFi network.");
        break;
    case WL_CONNECTION_LOST:
        Serial.println("WiFi connection lost.");
        break;
    case WL_DISCONNECTED:
        Serial.println("WiFi disconnected.");
        break;
    default:
        Serial.println("Unknown WiFi status.");
        break;
    }
    if (status == WL_CONNECTED)
    {
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        Serial.print("Gateway IP: ");
        Serial.println(WiFi.gatewayIP());

        Serial.print("DNS Server IP: ");
        Serial.println(WiFi.dnsIP());
    }
}

void pingCommand(const char *host)
{
    Serial.print("Pinging ");
    Serial.print(host);
    Serial.println("..");

    int packetsSent = 0;
    int packetsReceived = 0;
    int packetsLost = 0;
    unsigned int minPingTime = UINT32_MAX;
    unsigned int maxPingTime = 0;
    unsigned int totalPingTime = 0;

    for (int i = 0; i < 5; i++)
    {
        unsigned long startTime = millis();
        IPAddress ip;
        if (WiFi.hostByName(host, ip))
        {
            unsigned int pingTime = millis() - startTime;
            packetsSent++;
            packetsReceived++;
            totalPingTime += pingTime;

            if (pingTime < minPingTime)
            {
                minPingTime = pingTime;
            }
            if (pingTime > maxPingTime)
            {
                maxPingTime = pingTime;
            }

            Serial.print("!");
        }
        else
        {
            packetsSent++;
            packetsLost++;
            Serial.print(".");
        }

        delay(1000); // Wait for 1 second before sending the next ping
    }

    Serial.println();
    Serial.print("Packets: Sent = ");
    Serial.print(packetsSent);
    Serial.print(", Received = ");
    Serial.print(packetsReceived);
    Serial.print(", Lost = ");
    Serial.print(packetsLost);
    Serial.print(" (");
    Serial.print((packetsLost * 100) / packetsSent);
    Serial.println("% loss),");

    if (packetsReceived > 0)
    {
        Serial.println("Approximate round trip times in milli-seconds:");
        Serial.print("Minimum = ");
        Serial.print(minPingTime);
        Serial.print("ms, Maximum = ");
        Serial.print(maxPingTime);
        Serial.print("ms, Average = ");
        Serial.print(totalPingTime / packetsReceived);
        Serial.println("ms");
    }
}

void getCreationTime(File file, char *output, size_t bufferSize)
{
    // Function not available for ESP8266, we use custom property "created" in JSON
    String createdStr = config[file.name()]["created"];
    strncpy(output, createdStr.c_str(), bufferSize);
}

void handleSerialInput()
{

    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            // Check for newline or carriage return
            Serial.println(); // Move to the next line after processing the command
            processCommand(); // Call processCommand without arguments
            inputLine = "";   // Clear the input line for the next command
            displayPrompt();  // Display the prompt again
        }
        else
        {
            Serial.print(c); // Echo the character back
            inputLine += c;  // Append the character to the input line
        }
    }
}

void processCommand()
{
    inputLine.trim(); // Trim the input line to remove leading/trailing spaces

    if (inputLine.length() == 0)
    {
        // If the input line is empty, just display the prompt and return
        return;
    }

    if (enableMode)
    {
        processEnableCommand(inputLine);
    }
    else
    {
        if (inputLine == "enable")
        {
            enterEnableMode();
        }
        if (inputLine == "show ntp")
        {
            showNTPServer();
        }
        else if (inputLine == "show unixtime")
        {
            showUnixTime();
        }
        else if (inputLine == "show date")
        {
            showDate();
        }
        else if (inputLine == "show wifi")
        {
            showWifiStatus(); // Handle the "show wifi" command
        }
        else if (inputLine == "show disk")
        {
            showDiskSpace();
        }
        else if (inputLine.startsWith("ping "))
        {
            pingCommand(inputLine.substring(5).c_str());
        }
        else if (inputLine == "ls")
        {
            listFiles();
        }
        else if (inputLine == "?")
        {
            printNormalHelp();
        }
        else
        {
            Serial.println("Invalid command in normal mode.");
        }
    }
}

void showDiskSpace()
{
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.print("SPIFFS Total Bytes: ");
    Serial.println(fs_info.totalBytes);
    Serial.print("SPIFFS Used Bytes: ");
    Serial.println(fs_info.usedBytes);
    Serial.print("SPIFFS Free Bytes: ");
    Serial.println(fs_info.totalBytes - fs_info.usedBytes);
}

void processEnableCommand(const String &command)
{
    String cmd = command; // Copy the value of the const reference into a non-const String
    cmd.trim();           // Now you can call trim() on the non-const String

    if (cmd == "exit")
    {
        enableMode = false;
    }
    else if (cmd == "show config")
    {
        displayConfig();
    }
    else if (cmd.startsWith("set "))
    {
        // Parse and update configuration parameters in enable mode
        // Example: set ssid-name="Anan AP"
        updateConfig(cmd);
    }
    else if (inputLine.startsWith("enable password="))
    {
        setEnablePassword(cmd);
    }
    else if (cmd.startsWith("no "))
    {
        // Remove configuration parameters in enable mode
        // Example: no ssid-name
        removeConfig(cmd);
    }
    else if (cmd == "download")
    {
        downloadConfigFile();
    }
    else if (cmd.startsWith("serial speed="))
    {
        setSerialSpeed(cmd);
    }
    else if (cmd == "write mem")
    {
        saveConfigToFile();
        Serial.println("Configuration saved to file.");
    }
    else if (cmd == "?")
    {
        // Print help for enable mode commands
        printEnableHelp();
    }
    else
    {
        Serial.print("Invalid input command: ");
        Serial.println(cmd);
        Serial.println("Invalid command in enable mode.");
    }
}

// List enable commands  ======================
void setSerialSpeed(const String &command)
{
    long speed = command.substring(command.indexOf('=') + 1).toInt();
    systemConfig["serial_speed"] = speed;
    Serial.println("Serial speed set to " + String(speed) + " bps.");
    Serial.begin(speed); // Update serial speed immediately
}

void setEnablePassword(const String &command)
{
    String newPassword = command.substring(command.indexOf('=') + 1);
    newPassword.trim();
    systemConfig["enable_password"] = newPassword; // Save the password to systemConfig
    Serial.println("New enable password set.");
}

void downloadConfigFile()
{
    // Open the config file in SPIFFS
    File configFile = SPIFFS.open(configFilePath, "r");
    if (!configFile)
    {
        Serial.println("Failed to open config file");
        return;
    }

    // Read and send the data from the config file to Serial Monitor
    while (configFile.available())
    {
        char data = configFile.read();
        Serial.write(data); // Send the data to Serial Monitor
    }

    // Close the file
    configFile.close();

    Serial.println("File download complete");
}

void processNormalCommand(const String &command)
{
    if (command == "enable")
    {
        // Enter enable mode if the correct password is provided
        enterEnableMode();
    }
    else if (command == "?")
    {
        // Print help for normal mode commands
        printNormalHelp();
    }
    else
    {
        Serial.println("Invalid command in normal mode.");
    }
}

void updateConfig(const String &command)
{
    // Parse and update the configuration parameters
    // Example: command = "set ssid-name="Anan AP""
    int equalsPos = command.indexOf('=');
    if (equalsPos != -1)
    {
        String paramName = command.substring(4, equalsPos);
        String paramValue = command.substring(equalsPos + 1);
        config[paramName] = paramValue;
        Serial.println("Parameter updated.");
    }
    else
    {
        Serial.println("Invalid set command.");
    }
}

void enterEnableMode()
{
    String enteredPassword;
    Serial.print("Enable password: ");

    while (Serial.available() == 0)
    {
        // Wait for user input
    }

    enteredPassword = Serial.readStringUntil('\r');
    enteredPassword.trim();

    File systemConfigFile = SPIFFS.open(systemConfigFilePath, "r");
    if (systemConfigFile)
    {
        StaticJsonDocument<512> systemConfig;
        deserializeJson(systemConfig, systemConfigFile);
        systemConfigFile.close();

        String storedPassword = systemConfig["enable_password"].as<String>(); // Read the password from config

        if (enteredPassword == storedPassword)
        {
            enableMode = true;
            Serial.println("");
            return;
        }
        else
        {
            Serial.println("Invalid password.");
        }
    }
    else
    {
        Serial.println("\n\nFailed to open sysconfig.conf file.");
    }
}

void saveConfigToFile()
{
    // Save the configuration in the config variable to "/app.conf" file
    File configFile = SPIFFS.open(configFilePath, "w");
    if (configFile)
    {
        serializeJson(config, configFile);
        configFile.close();
        Serial.println("Main configuration saved to /app.conf.");
    }
    else
    {
        Serial.println("Failed to open config file for writing.");
    }

    // Save the system configuration in the systemConfig variable to "/sysconfig.conf" file
    File systemConfigFile = SPIFFS.open(systemConfigFilePath, "w");
    if (systemConfigFile)
    {
        serializeJson(systemConfig, systemConfigFile);
        systemConfigFile.close();
        Serial.println("System configuration saved to /sysconfig.conf.");
    }
    else
    {
        Serial.println("Failed to open system config file for writing.");
    }
}

void loadConfigFromFile(const char *filePath, StaticJsonDocument<512> &jsonDocument)
{
    File configFile = SPIFFS.open(filePath, "r");
    if (configFile)
    {
        deserializeJson(jsonDocument, configFile);
        configFile.close();
        Serial.println("Configuration loaded from " + String(filePath));
    }
    else
    {
        Serial.println("Failed to open config file: " + String(filePath));
    }
}

void displayConfig()
{
    // Display the current configuration from "/app.conf"
    Serial.println("Application configuration  /app.conf:");
    serializeJsonPretty(config, Serial);
    Serial.println();

    // Display the current configuration from "/sysconfig.conf"
    File systemConfigFile = SPIFFS.open(systemConfigFilePath, "r");
    if (systemConfigFile)
    {
        Serial.println("System configuration /sysconfig.conf:");
        StaticJsonDocument<512> systemConfig;
        deserializeJson(systemConfig, systemConfigFile);
        serializeJsonPretty(systemConfig, Serial);
        Serial.println();
        systemConfigFile.close();
    }
    else
    {
        Serial.println("Failed to open /sysconfig.conf file.");
    }
}

void displayPrompt()
{
    Serial.print("Suntrack");
    if (enableMode)
    {
        Serial.print("(enable)");
    }
    if (enableMode == true)
    {
        Serial.print("# ");
    }
    else
    {
        Serial.print("> ");
    }
}

void displayEnablePrompt()
{
    Serial.print("Suntrack(enable)# ");
}

void printNormalHelp()
{
    // Print help for normal mode commands
    // You can customize this based on your commands
    Serial.println("Available commands in normal mode:");
    Serial.println("show disk     : Show system free space");
    Serial.println("show wifi     : Show Wifi status ip address ");
    Serial.println("ls            : List files system");
    Serial.println("enable        : Enter enable mode");
    Serial.println("?             : Print this help message");
}

void printEnableHelp()
{
    // Print help for enable mode commands
    // You can customize this based on your commands
    Serial.println("Available commands in enable mode:");
    Serial.println("show config              : Display current configuration");
    Serial.println("set <param>=<value>      : Set configuration parameter");

    Serial.println("serial speed=<number>    : Set com port speed ");
    Serial.println("enable password=<value>  : Set update enable password");
    Serial.println("no <param>               : Remove configuration parameter");
    Serial.println("write mem                : Save configuration to file");
    Serial.println("exit                     : Exit enable mode");
    Serial.println("?                        : Print this help message");
}

void removeConfig(const String &command)
{
    // Remove configuration parameter
    // Example: command = "no ssid-name"
    String paramName = command.substring(3); // Remove "no " from the command

    // Check if the parameter exists and remove it from the config
    if (config.containsKey(paramName))
    {
        config.remove(paramName);
        Serial.println("Parameter removed.");
    }
    else
    {
        Serial.println("Parameter not found.");
    }
}

// Normal mode functions
void listFiles()
{
    Serial.println("Listing files in the root directory:");
    Serial.println("===================================");

    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
        File entry = dir.openFile("r");
        String fileName = dir.fileName();
        size_t fileSize = entry.size();

        bool isDir = fileName.endsWith("/");
        char fileTime[20];
        getCreationTime(entry, fileTime, sizeof(fileTime));

        Serial.print(fileSize);
        Serial.print(" ");
        Serial.print(fileTime);
        Serial.print(" ");
        Serial.println(fileName);

        entry.close();
    }

    Serial.println("===================================");
  
}

 void showNTPServer(){
       // 
 }

    void showUnixTime(){
       // 
    }
    void  showDate(){
      //
    }
