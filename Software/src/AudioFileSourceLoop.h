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
    AudioFileSourceLoop();
    virtual ~AudioFileSourceLoop() override;
    
    //virtual bool open(const char *filename) override;
    virtual uint32_t read(void *data, uint32_t len) override;
    virtual bool seek(int32_t pos, int dir) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;
    void setStartPos(int32_t newStartPos);
    void setPlayLoop(bool playLoop);

  protected:
    File f;
    int32_t startPos = 0;
    bool doPlayLoop = false;
};

class AudioFileSourceSDLoop : public AudioFileSourceLoop
{
  public:
    AudioFileSourceSDLoop();
    AudioFileSourceSDLoop(const char *filename);
    
    virtual bool open(const char *filename) override;
};

class AudioFileSourceFSLoop : public AudioFileSourceLoop
{
  public:
    AudioFileSourceFSLoop();
    AudioFileSourceFSLoop(const char *filename);
    
    virtual bool open(const char *filename) override;
};

#endif
