/*
 * AudioFileSourceLoop
 * Read SD/SPIFFS/LittleFS file to be used by AudioGenerator
 * Reads file in a loop (for looped playback)
 * 
 * Thomas Winischhofer (A10001986), 2023
 *
 * Based on AudioFileSourceSD by Earle F. Philhower, III
 *
 */

#ifndef _AudioFileSourceLoop_H
#define _AudioFileSourceLoop_H

#include "src/ESP8266Audio/AudioFileSource.h"
#include <SD.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#else
#include <LittleFS.h>
#endif

class AudioFileSourceLoop : public AudioFileSource
{
  public:
    AudioFileSourceLoop() {};
    ~AudioFileSourceLoop();
    
    virtual bool open(const char *filename) = 0;
    uint32_t read(void *data, uint32_t len) override;
    bool seek(int32_t pos, int dir) override;
    bool close() override                 { f.close(); return true; }
    bool isOpen() override                { return f ? true : false; }
    uint32_t getSize() override           { return f ? f.size() : 0; }
    uint32_t getPos() override            { return f ? f.position() : 0; }
    void setStartPos(int32_t newStartPos) { startPos = newStartPos; }
    void setPlayLoop(bool playLoop)       { doPlayLoop = playLoop; }

  protected:
    File    f;
    int32_t startPos = 0;
    bool    doPlayLoop = false;
};

class AudioFileSourceSDLoop : public AudioFileSourceLoop
{
  public:
    AudioFileSourceSDLoop();
    //AudioFileSourceSDLoop(const char *filename);
    
    bool open(const char *filename) override;
};

class AudioFileSourceFSLoop : public AudioFileSourceLoop
{
  public:
    AudioFileSourceFSLoop();
    //AudioFileSourceFSLoop(const char *filename);
    
    bool open(const char *filename) override;
};

#endif
