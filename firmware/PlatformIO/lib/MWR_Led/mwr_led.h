#ifndef mwr_led_h
#define mwr_led_h

// Includes
#include <Arduino.h>

/**
 * @brief LED Library
 */
class Led {
    public:
        Led(int LED_R, int LED_G, int LED_B);
        void setColour(String hexstring);
        void setState(String colour, int time, int repeat);
};

#endif