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

#include "tc_global.h"
#include "AudioFileSourceLoop.h"

AudioFileSourceLoop::AudioFileSourceLoop()
{
}

AudioFileSourceLoop::~AudioFileSourceLoop()
{
    if(f) f.close();
}

uint32_t AudioFileSourceLoop::read(void *data, uint32_t len)
{
    uint32_t glen = f.read(reinterpret_cast<uint8_t*>(data), len);
    if(!doPlayLoop || glen == len) return glen;
    seek(startPos, SEEK_SET);
    return glen + f.read(reinterpret_cast<uint8_t*>(data) + glen, len - glen);
}

bool AudioFileSourceLoop::seek(int32_t pos, int dir)
{
    if(!f) return false;
    if(dir == SEEK_SET)      return f.seek(pos);
    else if(dir == SEEK_CUR) return f.seek(f.position() + pos);
    else if(dir == SEEK_END) return f.seek(f.size() + pos);
    return false;
}

bool AudioFileSourceLoop::close()
{
    f.close();
    return true;
}

bool AudioFileSourceLoop::isOpen()
{
    return f ? true : false;
}

uint32_t AudioFileSourceLoop::getSize()
{
    if(!f) return 0;
    return f.size();
}

uint32_t AudioFileSourceLoop::getPos()
{
    if(!f) return 0;
    return f.position();
}

void AudioFileSourceLoop::setStartPos(int32_t newStartPos)
{
    startPos = newStartPos;
}

void AudioFileSourceLoop::setPlayLoop(bool playLoop)
{
    doPlayLoop = playLoop;
}


// SD -----------------------------------------------

AudioFileSourceSDLoop::AudioFileSourceSDLoop()
{
}

AudioFileSourceSDLoop::AudioFileSourceSDLoop(const char *filename)
{
    open(filename);
}

bool AudioFileSourceSDLoop::open(const char *filename)
{
    f = SD.open(filename, FILE_READ);
    return f;
}

// FlashFS -------------------------------------------

AudioFileSourceFSLoop::AudioFileSourceFSLoop()
{
}

AudioFileSourceFSLoop::AudioFileSourceFSLoop(const char *filename)
{
    open(filename);
}

bool AudioFileSourceFSLoop::open(const char *filename)
{
    #ifdef USE_SPIFFS   // ------------------------------
    f = SPIFFS.open(filename, FILE_READ);
    #else   // ------------------------------------------
    f = LittleFS.open(filename, FILE_READ);
    #endif // -------------------------------------------
    return f;
}
