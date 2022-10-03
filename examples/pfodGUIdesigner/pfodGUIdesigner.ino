/*
   pfodGUIDesigner.cpp
   by Matthew Ford,  2021/12/06
   (c)2021 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.
*/

// for ESP32-C3 should also run on ESP32 (Sparkfun Thing etc)
// Network Settings
#include <IPAddress.h>
const char staticIP[] = "10.1.1.71";
const char ssid[] = "ssid";
const char pw[] = "password";
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 8, 4);

#if defined (ESP32)
#include <WiFi.h>
#elif defined (ESP8266)
#include <ESP8266WiFi.h>
#else
#error "Only ESP32 and ESP8266 supported"
#endif
#include <pfodEEPROM.h>
#include "pfodGUIdisplay.h"
#include "SafeString.h"

const unsigned long BAUD_RATE = 115200ul;
bool connectToWiFi(const char* staticIP, const char* ssid, const char* password);
void start_pfodServer();
void handle_pfodServerConnection();
void sendMainMenu();
void sendMainMenuUpdate();
void closeConnection(Stream *io);

#define DEBUG
Stream *debugPtr = NULL;



void setup() {
  EEPROM.begin(512);  // only use 20bytes for pfodSecurity but reserve 512
  Serial.begin(115200);
  Serial.println();
#ifdef DEBUG
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(250);
  }
  SafeString::setOutput(Serial); // for debug error msgs
  debugPtr = &Serial;
  debugPtr->println();
#endif
  initializeDisplay(&Serial); // for pfodApp generated code goes to pfodApp and to Serial, pass NULL to suppress Serial output

  if (!connectToWiFi(staticIP, ssid, pw)) {
    Serial.println(" Connect to WiFi failed.");
    while (1) {
      delay(10); // wait here but feed WDT
    }
  } else {
    Serial.print("// Connected to "); Serial.print(ssid); Serial.print("  with IP:"); Serial.println(WiFi.localIP());
  }

  start_pfodServer();
}

void loop() {
  handle_pfodServerConnection();
}
// ------------------ trival telnet running on port 4989 server -------------------------
//how many clients should be able to telnet to this
#define MAX_SRV_CLIENTS 1

WiFiServer pfodServer(4989);
WiFiClient pfodServerClients[MAX_SRV_CLIENTS];

void start_pfodServer() {
  pfodServer.begin();
  pfodServer.setNoDelay(true);
  if (debugPtr) {
    debugPtr->print("// pfodServer Ready to Use on ");
    debugPtr->print(WiFi.localIP());
    debugPtr->println(":4989");
  }
}

void handle_pfodServerConnection() {
  //check if there are any new clients
  uint8_t i;
  if (pfodServer.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!pfodServerClients[i] || !pfodServerClients[i].connected()) {
        if (pfodServerClients[i]) {
          pfodServerClients[i].stop();
        }
        pfodServerClients[i] = pfodServer.available();
        if (!pfodServerClients[i]) {
          if (debugPtr) {
            debugPtr->println("// available broken");
          }
        } else {
          if (debugPtr) {
            debugPtr->print("// New client: ");
            debugPtr->print(i); debugPtr->print(' ');
            debugPtr->println(pfodServerClients[i].remoteIP());
          }
          connect_pfodParser(&pfodServerClients[i]);
          break;
        }
      }
    }
    if (i >= MAX_SRV_CLIENTS) {
      //no free/disconnected spot so reject
      pfodServer.available().stop();
    }
  }
  //check clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (pfodServerClients[i] && pfodServerClients[i].connected()) {
      handle_pfodParser();
    }
    else {
      if (pfodServerClients[i]) {
        pfodServerClients[i].stop();
      }
    }
  }
}
// returns false if does not connect
bool connectToWiFi(const char* staticIP, const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  if (staticIP[0] != '\0') {
    IPAddress ip;
    bool validIp = ip.fromString(staticIP);
    if (validIp) {
      IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1
      IPAddress subnet_ip = IPAddress(255, 255, 255, 0);
      WiFi.config(ip, gateway, subnet_ip, dns1, dns2);
    } else {
      if (debugPtr) {
        debugPtr->print("// staticIP is invalid: "); debugPtr->println(staticIP);
      }
      return false;
    }
  }
  if (debugPtr) {
    debugPtr->print("//   Connecting to WiFi ");
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }
  // Wait for connection for 30sec
  unsigned long pulseCounter = 0;
  unsigned long delay_ms = 100;
  unsigned long maxCount = (30 * 1000) / delay_ms; // delay below
  while ((WiFi.status() != WL_CONNECTED) && (pulseCounter < maxCount)) {
    pulseCounter++;
    delay(delay_ms); // often also prevents WDT timing out
    if (debugPtr) {
      if ((pulseCounter % 10) == 0) {
        debugPtr->print(".");
      }
    }
  }
  if (pulseCounter >= maxCount) {
    if (debugPtr) {
      debugPtr->println("// Failed to connect.");
    }
    return false;
  } // else
  if (debugPtr) {
    debugPtr->println("// Connected.");
  }
  return true;
}
