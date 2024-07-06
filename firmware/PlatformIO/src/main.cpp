/**
 * @file main.cpp
 * @brief ESP32 Radio Control with Rotary Encoder and Button
 * @version 2.4
 * @date 2024-07-06
 * 
 * Mini Web Radio (v2) for MK Hospital Radio
 * 
 * For more information, visit the GitHub repository:
 * https://github.com/yourusername/your-repository
 * 
 */

// Includes
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "driver/rtc_io.h"
#include "mwr_radio.h"
#include "mwr_config.h"
#include "mwr_led.h"

// I2S
#define I2S_DOUT 33
#define I2S_BCLK 25
#define I2S_LRC 32
// LED
#define LED_G 26
#define LED_R 27
#define LED_B 14
// Encoder
#define ENC_A 4
#define ENC_B 16
#define BTN 13
// Audio Power
#define ASO 22
// Battery Sense
#define VBAT 34;
// Mode Sense
#define SMOD 18

// Globals
int longPress = 2000;
int menuState;
int mode = 0;
int volume = 8;
volatile int16_t rotationCount = 0;
unsigned long btnPressed = 0;
unsigned long btnReleased = 0;
int timer = 0;
float voltage = 0.0;

// Init MWR Libs
Config config;
Radio radio;
Led led(LED_R, LED_G, LED_B);

/**
 * @brief Set the volume of the radio.
 * 
 * @param level Volume level to set (1-64).
 */
void setVolume(int level){
    Serial.print("Volume:");
    Serial.println(level);
    radio.setVolume(level);
}

/**
 * @brief Interrupt Service Routine for the rotary encoder.
 */
void IRAM_ATTR isr_encoder() {
    static uint32_t old_state = 0x0001;
    uint8_t act_state = 0;
    uint8_t inx;
    static const int8_t enc_states[] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0
    };

    act_state = (digitalRead(ENC_A) << 1) + digitalRead(ENC_B);
    inx = (old_state << 2) + act_state;
    rotationCount += enc_states[inx];
    old_state = act_state;
}

/**
 * @brief Read the state of the button and handle press events.
 */
void read_button(){
  menuState = digitalRead(BTN);

  // Long press - shutdown radio
  if(menuState == LOW && btnPressed == 0) {
      btnPressed = millis(); 
  } else if(menuState == LOW && btnPressed > 0) {
      if((millis() - btnPressed) > longPress) { 
          btnPressed = 0; 
          led.setState("FF0000", 500, 5);
          delay(1000);
          esp_deep_sleep_start(); 
      }
  }
  
  // Short press - change station
  if(menuState == HIGH && btnPressed > 0) { 
      if((millis() - btnPressed) > 250) {
          btnPressed = 0;
          radio.next();
      }
  }

  // Check battery state
  if(timer >= 20000) {
      timer = 0;
      voltage = ((float)analogRead(34) / 4095) * 3.3 * 2 * 1.035;
      char color[6];
      sprintf(color, "33%02u00", map((voltage * 100), 310, 400, 0, 99));
      led.setColour(color);
      // Shutdown to preserve battery lifespan
      if(voltage <= 3.0) {
          led.setState("FF0000", 250, 5);
          Serial.println("Battery low, going to sleep");
          delay(1000);
          Serial.flush();
          esp_deep_sleep_start();
      }
  }
  timer++;
}

/**
 * @brief Handle WiFi disconnect events and attempt reconnection.
 * 
 * @param event WiFi event type.
 * @param info WiFi event information.
 */
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  led.setColour("000055");
  Serial.println("Disconnected from WiFi access point");
  Serial.println("Trying to Reconnect...");
  WiFi.reconnect();
}

void setup() {
  Serial.begin(115200);
  Serial.println("--------------------------------------");
  Serial.println("MWR Version 2");
  Serial.print("MAC Address:");
  Serial.println(WiFi.macAddress());
  Serial.println("--------------------------------------");


  // Setup RTC Wake
  rtc_gpio_pullup_en(GPIO_NUM_13);
  rtc_gpio_pulldown_dis(GPIO_NUM_13);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);

  // Setup GPIO
  pinMode(ASO, OUTPUT); 
  pinMode(SMOD, INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  // Attach Interrupts
  attachInterrupt(ENC_A, isr_encoder, CHANGE);
  attachInterrupt(ENC_B, isr_encoder, CHANGE);

  // Initialise audio
  radio.init(I2S_DOUT, I2S_BCLK, I2S_LRC, ASO);

  // Handle wifi disconnect
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Check and set boot state
  if(digitalRead(SMOD)) {
      config.stMode();
      String url = radio.readLine(0);
      radio.setVolume(volume);
      radio.setStation(url);
      int treble = config.getTreble();
      int mid = config.getMid();
      int bass = config.getBass();
      radio.setTone(bass, mid, treble);
      radio.next();
  } else {
      config.apMode();
  }

  delay(1000);
}

void loop() {

    read_button();

    if (WiFi.status() == WL_CONNECTED) {
      radio.play();
    }else{
      Serial.println("Offline");
      delay(500);
    }
    
    static int16_t lastRotationCount = 0;
    if (rotationCount != lastRotationCount) {
        noInterrupts();
        int16_t rotation = rotationCount;
        rotationCount = 0;
        interrupts();

        if (rotation >= 1) {
            if (volume < 64) { volume++; }
            setVolume(volume);
        } else if (rotation <= -1) {
            if (volume > 1) { volume--; }
            setVolume(volume);
        }
        lastRotationCount = rotation;
    }
}