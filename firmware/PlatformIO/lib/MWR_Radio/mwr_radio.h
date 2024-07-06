#ifndef mwr_radio_h
#define mwr_radio_h

// Includes
#include <Audio.h>

/**
 * @brief Radio Library
 */
class Radio
{
  public:
    void init(int DOUT, int BLCK, int LRC, int ASO);
    String readLine(int line);
    int countLines();
    void setStation(String URL);
    void setVolume(int VOL);
    void setTone(int hi, int mi, int lo);
    void play();
    void next();
  private:
    int playlistCount;
    int playlistIndex = 0;
    int ASO;
;
};

#endif
