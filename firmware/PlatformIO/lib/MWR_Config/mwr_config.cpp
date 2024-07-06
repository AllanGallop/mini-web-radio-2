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
const char* PARAM_INPUT_5 = "treb"; // HTTP Input param for URLs
const char* PARAM_INPUT_6 = "mid";  // HTTP Input param for URLs
const char* PARAM_INPUT_7 = "bass"; // HTTP Input param for URLs

IPAddress primaryDNS(8, 8, 8, 8);      // Primary DNS server (Google DNS)
IPAddress secondaryDNS(8, 8, 4, 4);    // Secondary DNS server (Google DNS)

String host;
String ssid;
String pass;
String url;
int treble = 0;
int mid    = 0;
int bass   = 0;

// SPIFFS file paths
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* hostPath = "/host.txt";
const char* urlPath = "/url.txt";
const char* tonePath = "/tone.txt";

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
        if (p->name() == PARAM_INPUT_5) {
          treble = p->value().toInt();
        }
        if (p->name() == PARAM_INPUT_6) {
          mid = p->value().toInt();
        }
        if (p->name() == PARAM_INPUT_7) {
          bass = p->value().toInt();
        }
        writeTone(treble, mid, bass);
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
  String toneValues = readFile(SPIFFS, tonePath); // Read tone values from CSV

  // Parse CSV
  if (!toneValues.isEmpty()) {
    int comma1 = toneValues.indexOf(',');
    int comma2 = toneValues.indexOf(',', comma1 + 1);
    treble = toneValues.substring(0, comma1).toInt();
    mid = toneValues.substring(comma1 + 1, comma2).toInt();
    bass = toneValues.substring(comma2 + 1).toInt();
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host.c_str());         // Set Hostname
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, primaryDNS, secondaryDNS);
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

/**
 * Write treble, mid, bass as CSV to tone.txt
 */
void Config::writeTone(int treble, int mid, int bass) {
  String toneData = String(treble) + "," + String(mid) + "," + String(bass);
  writeFile(SPIFFS, tonePath, toneData.c_str());
}

// Getter methods for treble, mid, and bass
int Config::getTreble() {
  return treble;
}

int Config::getMid() {
  return mid;
}

int Config::getBass() {
  return bass;
}
