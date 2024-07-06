// Includes
#include <Arduino.h>
#include <Audio.h>
#include "FS.h"
#include <SPIFFS.h>
#include "mwr_radio.h"

// Initialise Audio library
Audio audio;

/**
 * @brief Initalise Radio
 * 
 * @param DOUT 
 * @param BLCK 
 * @param LRC 
 * @param ASO 
 */
void Radio::init(int DOUT, int BLCK, int LRC, int ASO)
{
  this->ASO = ASO;
  // Set Pins
  audio.setPinout(BLCK, LRC, DOUT);
  // Set volume steps
  audio.setVolumeSteps(64);
  // Minor adjustment for cheap tinny headphones
  audio.setTone(4,-2,-6);
  // Start SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }else{
    Serial.println("SPIFFS started");
  }
  // Discover URLs found
  Serial.println("URLs Read:");
  Serial.println( countLines() );
  this->playlistCount = countLines();
  this->playlistIndex = 0;
  
  // Enable Audio hardware
  digitalWrite(ASO,LOW);
}

/**
 * @brief Set a new URL to play
 * 
 * @param URL 
 */
void Radio::setStation(String URL)
{
  Serial.print("Trying:");
  Serial.println(URL);
  audio.setTone(-10,-5,-1);
  if(audio.isRunning()){
    audio.stopSong();
  }
  audio.connecttohost(URL.c_str());
}
/**
 * @brief Set volume
 * 
 * @param VOL 
 */
void Radio::setVolume(int VOL)
{
  audio.setVolume(VOL);
}

void Radio::setTone(int hi, int mi, int lo)
{
  audio.setTone(hi,mi,lo);
}

/**
 * @brief Run the audio loop
 */
void Radio::play()
{
  audio.loop();
}

/**
 * @brief Play next url
 */
void Radio::next()
{
  if( this->playlistIndex == (this->playlistCount-1) ){
    this->playlistIndex = 0;
  }else{
    this->playlistIndex++;
  }
  String url = this->readLine( this->playlistIndex );
  this->setStation(url);
}

/**
 * @brief Read line from URLs file
 * 
 * @param line 
 * @return String 
 */
String Radio::readLine(int line)
{
  String fileContent = "";
  int lineCount = 0;
  File file = SPIFFS.open("/url.txt");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    if(lineCount == line){
      break;
    }else{
      lineCount++;
    }
  }
  fileContent.replace("\r",""); // remove carriage returns
  return fileContent;
}

/**
 * @brief Count lines in URL file
 * 
 * @return int 
 */
int Radio::countLines()
{
    int lineCount = 0;
    File file = SPIFFS.open("/url.txt");
    if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return 0;
    }
    while(file.available()){
      file.readStringUntil('\n');
      lineCount++;
    }
    return lineCount;
}