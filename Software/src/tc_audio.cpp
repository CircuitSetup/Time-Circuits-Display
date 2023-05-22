/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Sound handling
 *
 * -------------------------------------------------------------------
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "tc_global.h"


#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#include <AudioFileSourceSPIFFS.h>
#else
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#endif
#include <AudioFileSourceSD.h>
#include <AudioFileSourcePROGMEM.h>

#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>

#include <AudioOutputI2S.h>

#include "tc_settings.h"
#include "tc_keypad.h"
#include "tc_time.h"

#include "tc_audio.h"

static AudioGeneratorMP3 *mp3;
static AudioGeneratorWAV *beep;

#ifdef USE_SPIFFS
static AudioFileSourceSPIFFS *myFS0;
#else
static AudioFileSourceLittleFS *myFS0;
#endif
static AudioFileSourceSD *mySD0;
static AudioFileSourcePROGMEM *myPM;

static AudioOutputI2S *out;

bool audioInitDone = false;
bool audioMute = false;

bool muteBeep = true;

bool haveMusic = false;
bool mpActive = false;
static uint16_t maxMusic = 0;
static uint16_t *playList = NULL;
static int  mpCurrIdx = 0;
static bool mpShuffle = false;
static uint16_t currPlaying = 0;

static const float volTable[20] = {
    0.00, 0.02, 0.04, 0.06,
    0.08, 0.10, 0.13, 0.16,
    0.19, 0.22, 0.26, 0.30,
    0.35, 0.40, 0.50, 0.60,
    0.70, 0.80, 0.90, 1.00
};

uint8_t curVolume = DEFAULT_VOLUME;

static float curVolFact = 1.0;
static bool  curChkNM   = true;
static bool  dynVol     = true;

static int sampleCnt = 0;

#define VOL_SMOOTH_SIZE 4
static int rawVol[VOL_SMOOTH_SIZE];
static int rawVolIdx = 0;
static int anaReadCount = 0;
static long prev_avg, prev_raw, prev_raw2;

// Resolution for pot, 9-12 allowed
#define POT_RESOLUTION 9

static bool mp_checkForFile(int num);
static void mp_nextprev(bool forcePlay, bool next);
static bool mp_play_int(bool force);
static void mp_buildFileName(char *fnbuf, int num);

static int skipID3(char *buf);

static float getRawVolume();
static float getVolume();

#include "tc_beep.h"

/*
 * audio_setup()
 */
void audio_setup()
{
    #ifdef TC_DBG
    audioLogger = &Serial;
    #endif

    // Set resolution for volume pot
    analogReadResolution(POT_RESOLUTION);
    analogSetWidth(POT_RESOLUTION);

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DIN_PIN);

    mp3  = new AudioGeneratorMP3();
    beep = new AudioGeneratorWAV();

    #ifdef USE_SPIFFS
    myFS0 = new AudioFileSourceSPIFFS();
    #else
    myFS0 = new AudioFileSourceLittleFS();
    #endif

    if(haveSD) {
        mySD0 = new AudioFileSourceSD();
    }

    myPM = new AudioFileSourcePROGMEM();

    loadCurVolume();

    loadMusFoldNum();
    mpShuffle = (settings.shuffle[0] != '0');

    // MusicPlayer init
    mp_init();

    audioInitDone = true;
}

void mp_init() 
{
    char fnbuf[20];
    
    haveMusic = false;

    if(playList) {
        free(playList);
        playList = NULL;
    }

    mpCurrIdx = 0;
    
    if(haveSD) {
        int i, j;

        #ifdef TC_DBG
        Serial.println("MusicPlayer: Checking for music files");
        #endif

        mp_buildFileName(fnbuf, 0);
        if(SD.exists(fnbuf)) {
            haveMusic = true;

            for(j = 256, i = 512; j >= 2; j >>= 1) {
                if(mp_checkForFile(i)) {
                    i += j;    
                } else {
                    i -= j;
                }
            }
            if(mp_checkForFile(i)) {
                if(mp_checkForFile(i+1)) i++;
            } else {
                i--;
                if(!mp_checkForFile(i)) i--;
            }
            
            maxMusic = i;
            #ifdef TC_DBG
            Serial.printf("MusicPlayer: last file num %d\n", maxMusic);
            #endif

            playList = (uint16_t *)malloc((maxMusic + 1) * 2);

            if(!playList) {

                haveMusic = false;
                #ifdef TC_DBG
                Serial.println("MusicPlayer: Failed to allocate PlayList");
                #endif

            } else {

                // Init play list
                mp_makeShuffle(mpShuffle);
                
            }

        } else {
            #ifdef TC_DBG
            Serial.printf("MusicPlayer: Failed to open %s\n", fnbuf);
            #endif
        }
    }
}

static bool mp_checkForFile(int num)
{
    char fnbuf[20];

    if(num > 999) return false;

    mp_buildFileName(fnbuf, num);
    if(SD.exists(fnbuf)) {
        return true;
    }
    return false;
}

void mp_makeShuffle(bool enable)
{
    int numMsx = maxMusic + 1;

    mpShuffle = enable;

    if(!haveMusic) return;
    
    for(int i = 0; i < numMsx; i++) {
        playList[i] = i;
    }
    
    if(enable && numMsx > 2) {
        for(int i = 0; i < numMsx; i++) {
            int ti = esp_random() % numMsx;
            uint16_t t = playList[ti];
            playList[ti] = playList[i];
            playList[i] = t;
        }
        #ifdef TC_DBG
        for(int i = 0; i <= maxMusic; i++) {
            Serial.printf("%d ", playList[i]);
            if((i+1) % 16 == 0 || i == maxMusic) Serial.printf("\n");
        }
        #endif
    }
}

void mp_play(bool forcePlay)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
        mpCurrIdx++;
        if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
    } while(oldIdx != mpCurrIdx);
}

bool mp_stop()
{
    bool ret = mpActive;
    
    if(mpActive) {
        mp3->stop();
        mpActive = false;
    }
    
    return ret;
}

void mp_next(bool forcePlay)
{
    mp_nextprev(forcePlay, true);
}

void mp_prev(bool forcePlay)
{   
    mp_nextprev(forcePlay, false);
}

static void mp_nextprev(bool forcePlay, bool next)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(next) {
            mpCurrIdx++;
            if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
        } else {
            mpCurrIdx--;
            if(mpCurrIdx < 0) mpCurrIdx = maxMusic;
        }
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
    } while(oldIdx != mpCurrIdx);
}

int mp_gotonum(int num, bool forcePlay)
{
    if(!haveMusic) return 0;

    if(num < 0) num = 0;
    else if(num > maxMusic) num = maxMusic;

    if(mpShuffle) {
        for(int i = 0; i <= maxMusic; i++) {
            if(playList[i] == num) {
                mpCurrIdx = i;
                break;
            }
        }
    } else 
        mpCurrIdx = num;

    mp_play(forcePlay);

    return playList[mpCurrIdx];
}

static bool mp_play_int(bool force)
{
    char fnbuf[20];

    mp_buildFileName(fnbuf, playList[mpCurrIdx]);
    if(SD.exists(fnbuf)) {
        if(force) play_file(fnbuf, PA_CHECKNM|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
        currPlaying = playList[mpCurrIdx];
        return true;
    }
    return false;
}

int mp_get_currently_playing()
{
    if(!haveMusic || !mpActive)
        return -1;

    return currPlaying;
}

static void mp_buildFileName(char *fnbuf, int num)
{
    sprintf(fnbuf, "/music%1d/%03d.mp3", musFolderNum, num);
}

bool mp_checkForFolder(int num)
{
    char fnbuf[20];

    if(num < 0 || num > 9) return false;

    sprintf(fnbuf, "/music%1d/000.mp3", num);
    if(SD.exists(fnbuf)) {
        return true;
    }
    return false;
}

void play_keypad_sound(char key)
{
    char buf[16] = "/Dtmf-0.mp3\0";

    if(key) {
        buf[6] = key;
        play_file(buf, PA_CHECKNM, 0.6);
    }
}

void play_hour_sound(int hour)
{
    char buf[16];
    const char *hsnd = "/hour.mp3";

    if(!haveSD || mpActive) return;
    // Not even called in night mode
    
    sprintf(buf, "/hour-%02d.mp3", hour);
    if(SD.exists(buf)) {
        play_file(buf, PA_ALLOWSD);
        return;
    }

    // Check for file so we do not interrupt anything
    // if the file does not exist
    if(SD.exists(hsnd)) {
        play_file(hsnd, PA_ALLOWSD);
    }
}

void play_beep()
{
    if(!FPBUnitIsOn     || 
       mp3->isRunning() || 
       muteBeep         || 
       mpActive         || 
       audioMute        || 
       presentTime.getNightMode()) {
        return;
    }

    pwrNeedFullNow();

    if(beep->isRunning()) {
        beep->stop();
    }

    curVolFact = 0.3;
    curChkNM   = true;
    out->SetGain(getVolume());

    myPM->open(data_beep_wav, data_beep_wav_len);
    beep->begin(myPM, out);
}

/*
 * audio_loop()
 *
 */
void audio_loop()
{   
    float vol;
    
    if(beep->isRunning()) {
        if(!beep->loop()) {
            beep->stop();
        }
    } else if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            if(mpActive) {
                mp_next(true);
            }
        } else {
            sampleCnt++;
            if(sampleCnt > 1) {
                vol = getVolume();
                if(dynVol) out->SetGain(vol);
                sampleCnt = 0;
            }
        }
    } else if(mpActive) {
        pwrNeedFullNow();
        mp_next(true);
    }
}

static int skipID3(char *buf)
{
    if(buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' && 
       (buf[3] == 0x04 || buf[3] == 0x03 || buf[3] == 0x02) && 
       buf[4] == 0) {
        int32_t pos = ((buf[6] << (24-3)) |
                       (buf[7] << (16-2)) |
                       (buf[8] << (8-1))  |
                       (buf[9])) + 10;
        #ifdef TC_DBG
        Serial.printf("Skipping ID3 tags, seeking to %d (0x%x)\n", pos, pos);
        #endif
        return pos;
    }
    return 0;
}

void play_file(const char *audio_file, uint16_t flags, float volumeFactor)
{
    char buf[10];
    
    if(audioMute) return;

    if(flags & PA_INTRMUS) {
        mpActive = false;
    } else {
        if(mpActive) return;
    }

    pwrNeedFullNow();

    #ifdef TC_DBG
    Serial.printf("Audio: Playing %s\n", audio_file);
    #endif

    // If something is currently on, kill it
    if(mp3->isRunning()) {
        mp3->stop();
    } 
    if(beep->isRunning()) {
        beep->stop();
    }

    curVolFact = volumeFactor;
    curChkNM   = (flags & PA_CHECKNM) ? true : false;
    dynVol     = (flags & PA_DYNVOL) ? true : false;

    // Reset vol smoothing
    // (user might have turned the pot while no sound was played)
    rawVolIdx = 0;
    anaReadCount = 0;

    out->SetGain(getVolume());

    buf[0] = 0;

    if(haveSD && ((flags & PA_ALLOWSD) || FlashROMode) && mySD0->open(audio_file)) {

        mySD0->read((void *)buf, 10);
        mySD0->seek(skipID3(buf), SEEK_SET);
        mp3->begin(mySD0, out);   
                 
        #ifdef TC_DBG
        Serial.println(F("Playing from SD"));
        #endif
    }
    #ifdef USE_SPIFFS
      else if(SPIFFS.exists(audio_file) && myFS0->open(audio_file))
    #else    
      else if(myFS0->open(audio_file))
    #endif
    {
        myFS0->read((void *)buf, 10);
        myFS0->seek(skipID3(buf), SEEK_SET);
        mp3->begin(myFS0, out);
                  
        #ifdef TC_DBG
        Serial.println(F("Playing from flash FS"));
        #endif
        
    } else {
      
        #ifdef TC_DBG
        Serial.println(F("Audio file not found"));
        #endif
        
    }
}

bool check_file_SD(const char *audio_file)
{
    return (haveSD && SD.exists(audio_file));
}

// Returns value for volume based on the position of the pot
// Since the values vary we do some noise reduction
static float getRawVolume()
{
    float vol_val;
    long avg = 0, avg1 = 0, avg2 = 0;
    long raw;

    raw = analogRead(VOLUME_PIN);

    if(anaReadCount > 1) {
      
        rawVol[rawVolIdx] = raw;

        if(anaReadCount < VOL_SMOOTH_SIZE) {
        
            avg = 0;
            for(int i = rawVolIdx; i > rawVolIdx - anaReadCount; i--) {
                avg += rawVol[i & (VOL_SMOOTH_SIZE-1)];
            }
            avg /= anaReadCount;
            anaReadCount++;

        } else {

            for(int i = rawVolIdx; i > rawVolIdx - anaReadCount; i--) {
                if(i & 1) { 
                    avg1 += rawVol[i & (VOL_SMOOTH_SIZE-1)];
                } else {
                    avg2 += rawVol[i & (VOL_SMOOTH_SIZE-1)];
                }
            }
            avg1 = round((float)avg1 / (float)(VOL_SMOOTH_SIZE/2));
            avg2 = round((float)avg2 / (float)(VOL_SMOOTH_SIZE/2));
            avg = (abs(avg1-prev_avg) < abs(avg2-prev_avg)) ? avg1 : avg2;

            /*
            Serial.printf("%d %d %d %d\n", raw, avg1, avg2, avg);
            */
            
            prev_avg = avg;
        }
        
    } else {
      
        anaReadCount++;
        rawVol[rawVolIdx] = avg = prev_avg = prev_raw = prev_raw2 = raw;
        
    }

    rawVolIdx++;
    rawVolIdx &= (VOL_SMOOTH_SIZE-1);

    vol_val = (float)avg / (float)((1<<POT_RESOLUTION)-1);

    if((raw + prev_raw + prev_raw2 > 0) && vol_val < 0.01) vol_val = 0.01;

    prev_raw2 = prev_raw;
    prev_raw = raw;

    //Serial.println(vol_val);

    return vol_val;
}

static float getVolume()
{
    float vol_val;

    if(curVolume == 255) {
        vol_val = getRawVolume();
    } else {
        vol_val = volTable[curVolume];
    }

    // If user muted, return 0
    if(vol_val == 0.0) return vol_val;

    vol_val *= curVolFact;
    
    // Do not totally mute
    // 0.02 is the lowest audible gain
    if(vol_val < 0.02) vol_val = 0.02;

    // Reduce volume in night mode, if requested
    if(curChkNM && presentTime.getNightMode()) {
        vol_val *= 0.3;
        // Do not totally mute
        if(vol_val < 0.02) vol_val = 0.02;
    }

    return vol_val;
}

bool checkAudioDone()
{
    if( mp3->isRunning() || beep->isRunning() ) return false;
    return true;
}

bool checkMP3Done()
{
    if(mp3->isRunning()) return false;
    return true;
}

void stopAudio()
{
    if(mp3->isRunning()) {
        mp3->stop();
    } else if(beep->isRunning()) {
        beep->stop();
    }
}
