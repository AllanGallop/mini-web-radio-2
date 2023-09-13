// Includes
#include <Arduino.h>
#include <SPIFFS.h>
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
// Rotary Encoder
#define sv DRAM_ATTR static volatile

// Globals
int longPress = 2000;
int menuState;
int mode = 0;
int volume = 8;
sv int16_t rotationcount = 0;
unsigned long btnPressed = 0;
unsigned long btnReleased = 0;
int timer = 0;
float voltage = 0.0;

// Init MWR Libs
Config config;
Radio radio;
Led led(LED_R, LED_G, LED_B);

/**
 * @brief Set the Volume
 * 
 * @param level int
 */
void setVolume(int level){
    Serial.print("Volume:");
    Serial.println(level);
    radio.setVolume(level);
}

/**
 * @brief Encoder Interrupt
 */
void IRAM_ATTR isr_encoder()
{
  sv uint32_t     old_state = 0x0001 ;    // Previous state
  sv int16_t      rotationCounter = 0 ;   // rotation count
  uint8_t         act_state = 0 ;         // The current state of the 2 PINs
  uint8_t         inx ;                   // state table Index
  sv const int8_t enc_states [] =         // Table must be in DRAM (iram safe)
  { 0,                    // 00 -> 00
    -1,                   // 00 -> 01     // dt goes HIGH
    1,                    // 00 -> 10
    0,                    // 00 -> 11
    1,                    // 01 -> 00     // dt goes LOW
    0,                    // 01 -> 01
    0,                    // 01 -> 10
    -1,                   // 01 -> 11     // clk goes HIGH
    -1,                   // 10 -> 00     // clk goes LOW
    0,                    // 10 -> 01
    0,                    // 10 -> 10
    1,                    // 10 -> 11     // dt goes HIGH
    0,                    // 11 -> 00
    1,                    // 11 -> 01     // clk goes LOW
    -1,                   // 11 -> 10     // dt goes HIGH
    0                     // 11 -> 11
  } ;

  // Read current state of CLK, DT pin. Result is a 2 bit binary number: 00, 01, 10 or 11.
  act_state = ( digitalRead ( ENC_A ) << 1 ) + digitalRead ( ENC_B);
  // Index of table
  inx = ( old_state << 2 ) + act_state;
  // get the delta
  rotationCounter += enc_states[inx];
  // actions depending on rotation count
  if ( rotationCounter == 4 )
  {
    if(volume < 64){volume++; setVolume(volume);}
    rotationCounter = 0 ;
  }
  else if ( rotationCounter == -4 )
  {
    if(volume > 1){volume--; setVolume(volume);};
    rotationCounter = 0;
  }
  // record state
  old_state = act_state;
}

/**
 * @brief Standard Arduino Setup
 */
void setup() {

  Serial.begin(115200);

  // Wakeup
  rtc_gpio_pullup_en(GPIO_NUM_13);
  rtc_gpio_pulldown_dis(GPIO_NUM_13);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13,0);

  // Pin Setup
  pinMode(ASO, OUTPUT); 
  pinMode(SMOD,INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);

  // Encoder Pins
  pinMode(ENC_A,INPUT_PULLUP);
  pinMode(ENC_B,INPUT_PULLUP);
  // Encoder Interrupts
  attachInterrupt ( ENC_A, isr_encoder, CHANGE ) ;
  attachInterrupt ( ENC_B, isr_encoder, CHANGE ) ;

  // Initalise radio
  radio.init(I2S_DOUT, I2S_BCLK, I2S_LRC, ASO);

  // Read jumper and initialise appropiate mode
  if( digitalRead(SMOD)){
    config.stMode();
  }else{
    config.apMode();
  }

  // Wait for wifi
  while (WiFi.status() != WL_CONNECTED){
      led.setColour( "000055" );
      delay(500);
  }

  // Not really needed but gives debugger a breather
  delay(500);

  // Get and Set first station URL
  String url = radio.readLine(0);
  Serial.print("Playing:");
  Serial.println(url);

  // Initalise Audio
  radio.setVolume(volume);
  radio.setStation(url);
  led.setColour("005500");
}

/**
 * @brief Read encoder button
 */
void read_button(){
  // Get menu state
  menuState = digitalRead(BTN);

  // Long Press
  if(menuState == LOW && btnPressed == 0) {
    btnPressed = millis(); 
  } else if(menuState == LOW && btnPressed > 0) {
      if( (millis() - btnPressed) > longPress){ 
        btnPressed = 0; 
        led.setState("FF0000",500,5);
        delay(1000);
        esp_deep_sleep_start(); 
      }
  }
  // Short Press
  if(menuState == HIGH && btnPressed > 0){ 
    if( (millis() - btnPressed) > 250){
      btnPressed = 0;
      radio.next();
    }
  }

  // Check every 2000 ticks
  if(timer >= 2000){
    // Read voltage
    voltage = ((float)analogRead(34) / 4095) * 3.3 * 2 * 1.035;
    Serial.print(voltage);
    Serial.println("v");
    char color[6];
    sprintf(color,"33%02u00", map((voltage*100),310,400,0,99));
    led.setColour(color);
    if(voltage <= 3.0){
        led.setState("FF0000",250,5);// Danger, sleep ESP to protect battery
        Serial.println("Battery low, going to sleep");
        delay(1000);
        Serial.flush();
        esp_deep_sleep_start();
      }
  timer=0;
  }
  timer++;
}

/**
 * @brief Standard Arduino Loop
 */
void loop() {
  radio.play();
  read_button();
}

/**
 * @brief Handle Wifi Disconnect Event
 * Tries to reconnect and sets LED to reflect this
 * @param event 
 * @param info 
 */
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.println("Disconnected from WiFi access point");
    Serial.println("Trying to Reconnect...");
    WiFi.reconnect();
      while (WiFi.status() != WL_CONNECTED){
      Serial.println("DISCONNECTED");
      led.setColour( "000055" );
    }
  }
}