#ifndef TERMINAL_H
#define TERMINAL_H

#include <FS.h>
#include <ArduinoJson.h>
#include <ctime>

extern const char* configFilePath;
extern const char* systemConfigFilePath;
extern bool waitingForPassword;
extern unsigned long passwordInputStartTime;
extern const int enablePasswordAddr;
extern String inputLine;
extern StaticJsonDocument<512> config;
extern StaticJsonDocument<512> systemConfig;
extern bool enableMode;
const unsigned long PASSWORD_TIMEOUT = 10000; // 10 seconds

void setupTerminal();
void processTerminalInput();
void listFiles();
void getCreationTime(File file, char* output, size_t bufferSize);
void handleSerialInput();
void processCommand();
void showDiskSpace();
void processEnableCommand(const String& command);
void setSerialSpeed(const String& command);
void setEnablePassword(const String& command);
void downloadConfigFile();
void enterEnableMode();
void saveConfigToFile();
void loadConfigFromFile(const char* filePath, StaticJsonDocument<512>& jsonDocument);
void displayConfig();
void displayPrompt();
void displayEnablePrompt();
void printNormalHelp();
void printEnableHelp();
void removeConfig(const String& command);
void updateConfig(const String& command) ; 

void showNTPServer();
void showUnixTime();
void showDate();

#endif
