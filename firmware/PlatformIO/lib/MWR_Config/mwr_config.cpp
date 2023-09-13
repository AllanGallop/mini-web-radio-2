// Includes
#include <Arduino.h>
#include "FS.h"
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "mwr_config.h"

// Variables
const char* PARAM_INPUT_1 = "host"; // HTTP Input param for Host
const char* PARAM_INPUT_2 = "ssid"; // HTTP Input param for SSID
const char* PARAM_INPUT_3 = "pass"; // HTTP Input param for Pass
const char* PARAM_INPUT_4 = "url";  // HTTP Input param for URLs

String host;
String ssid;
String pass;
String url;

// SPIFFS file paths
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* hostPath = "/host.txt";
const char* urlPath = "/url.txt";

// Start server on port 80
AsyncWebServer server(80);

/**
 * @brief Construct a new Config:: Config object
 */
Config::Config(){}

/**
 * @brief Access Point Mode
 */
void Config::apMode()
{
  // Define IP addresses
  IPAddress localIP(192,168,1,1);
  IPAddress localGateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);

  Serial.println("Starting WiFi in AP Mode");
  WiFi.disconnect();                              // Disconnect Wifi (just in case)
  WiFi.softAPConfig(localIP,localGateway,subnet); // Configure Wifi
  WiFi.softAP("MWR-WIFI-SETUP", NULL);            // Start Wifi as AP

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Inbound Request");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Serve up static files
  server.serveStatic("/", SPIFFS, "/");

  // Store parameters from request
  server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        if (p->name() == PARAM_INPUT_1) {
          host = p->value().c_str();
          writeFile(SPIFFS, hostPath, host.c_str());
        }
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_2) {
          ssid = p->value().c_str();
          // Write file to save value
          writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_3) {
          pass = p->value().c_str();
          // Write file to save value
          writeFile(SPIFFS, passPath, pass.c_str());
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          url = p->value().c_str();
          // Write file to save value
          writeFile(SPIFFS, urlPath, url.c_str());
        }
      }
    }
    request->send(200, "text/plain", "Done. "+host+" will restart");
    delay(3000);
    ESP.restart();
  });
  server.begin();
}

/**
 * @brief Station Mode (Normal Mode)
 */
void Config::stMode()
{
  Serial.println("Starting WiFi in Station Mode");
  host = readFile(SPIFFS, hostPath);      // Read hostname
  ssid = readFile(SPIFFS, ssidPath);      // Read SSID
  pass = readFile(SPIFFS, passPath);      // Read Password
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host.c_str());         // Set Hostname
  WiFi.begin(ssid.c_str(), pass.c_str()); // Start Wifi
  Serial.print("Connecting to:");
  Serial.println(ssid);
}

/**
 * Returns audio url
 */
String Config::readUrlFromFile()
{
  return readFile(SPIFFS, urlPath);
}

/**
 * Read file from SPIFFS
 */
String Config::readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

/**
 * Write file to SPIFFS
 */
void Config::writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}
