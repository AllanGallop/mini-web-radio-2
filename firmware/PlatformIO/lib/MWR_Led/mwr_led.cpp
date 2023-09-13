// Includes
#include <Arduino.h>
#include "mwr_led.h"

/**
 * @brief Construct a new Led:: Led object
 * 
 * @param LED_R 
 * @param LED_G 
 * @param LED_B 
 */
Led::Led(int LED_R, int LED_G, int LED_B)
{
    // Setup pins and default to RED
    pinMode(LED_R,OUTPUT); digitalWrite(LED_R,HIGH);
    pinMode(LED_G,OUTPUT); digitalWrite(LED_G,HIGH);
    pinMode(LED_B,OUTPUT); digitalWrite(LED_B,HIGH);
    // Setup PWM
    ledcSetup(1, 12000, 8);
    ledcSetup(2, 12000, 8);
    ledcSetup(3, 12000, 8);
    // Attach PWN to LEDs
    ledcAttachPin(LED_R, 1);
    ledcAttachPin(LED_G, 2);
    ledcAttachPin(LED_B, 3);
};

/**
 * @brief Set LED to Colour
 * 
 * @param hexstring 
 */
void Led::setColour(String hexstring)
{
    long number = (long) strtol( &hexstring[0], NULL, 16);
    int R = number >> 16;
    int G = number >> 8 & 0xFF;
    int B = number & 0xFF;
    ledcWrite(1, (255-R));
    ledcWrite(2, (255-G));
    ledcWrite(3, (255-B));
};

/**
 * @brief Set LED state
 * 
 * @param colour 
 * @param time 
 * @param repeat 
 */
void Led::setState(String colour, int time, int repeat){
    for(int i = repeat; i > 0; i--)
    {
      this->setColour(colour);
      delay(time);
      this->setColour("000000");
    }
}