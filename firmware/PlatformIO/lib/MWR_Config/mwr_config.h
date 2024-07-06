#ifndef MWR_CONFIG_H
#define MWR_CONFIG_H

#include <Arduino.h>

class Config {
public:
    Config();
    void apMode();
    void stMode();
    String readUrlFromFile();
    int getTreble();
    int getMid();
    int getBass();

private:
    String readFile(fs::FS &fs, const char * path);
    void writeFile(fs::FS &fs, const char * path, const char * message);
    void writeTone(int treble, int mid, int bass);
    String host;
    String ssid;
    String pass;
    String url;
    int treble;
    int mid;
    int bass;
};

#endif