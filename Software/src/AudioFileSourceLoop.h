/*
 * AudioFileSourceLoop
 * Read SD/SPIFFS/LittleFS file to be used by AudioGenerator
 * Reads file in a loop (for looped playback)
 * 
 * Thomas Winischhofer (A10001986), 2023/2026
 *
 * Based on AudioFileSourceSD by Earle F. Philhower, III
 *
 */

#ifndef _AudioFileSourceLoop_H
#define _AudioFileSourceLoop_H

#include "src/ESP8266Audio/AudioFileSource.h"
#include "src/SD/SD.h"
#include <LittleFS.h>

class AudioFileSourceLoop : public AudioFileSource
{
  public:
    AudioFileSourceLoop() { maxSegs = 5; };
    ~AudioFileSourceLoop();
    
    virtual bool open(const char *filename) = 0;
    bool open_c(const char *filename, const int16_t *segs);
    uint32_t read(void *data, uint32_t len) override;
    bool seek(int32_t pos, int dir) override;
    bool close() override                    { if(toc) { free(toc); toc = NULL; } f.close(); return true; }
    bool isOpen() override                   { return f ? true : false; }
    uint32_t getSize() override              { return f ? f.size() : 0; }
    uint32_t getPos() override               { return f ? ((ftype == 2) ? (csegOLen - csegLen) : f.position()) : 0; }
    void setStartPos(int32_t newStartPos)    { startPos = newStartPos; }
    void setPlayLoop(bool playLoop)          { doPlayLoop = playLoop; }
    void setEndPos(int32_t newEndPos)        { endPos = newEndPos; }
    uint32_t (*c)(uint8_t *, uint32_t, uint32_t) = NULL;

  protected:
    File    f;
    int32_t startPos = 0, endPos = 0;
    bool    doPlayLoop = false;
    int     ftype = 0;
    
  private:
    bool     seekNext();
    uint32_t c_read(uint8_t *buf, uint32_t len);

    int32_t        *toc = NULL;
    int            segIdx = 0;
    uint32_t       csegLen = 0, csegOLen = 0;
    uint32_t       maxSegs = 0;
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
