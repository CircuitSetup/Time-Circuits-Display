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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tc_global.h"

// Use the mixer, or do not use the mixer.
// Since we don't use mixing, turn it off.
// With the current versions of the audio library,
// turning it on might cause a static after stopping
// sound play back.
//#define TC_USE_MIXER

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
#include <AudioGeneratorMP3.h>
//#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#ifdef TC_USE_MIXER
#include <AudioOutputMixer.h>
#endif

#include "tc_settings.h"
#include "tc_keypad.h"
#include "tc_time.h"

#include "tc_audio.h"

// Initialize ESP32 Audio Library classes

static AudioGeneratorMP3 *mp3;
//static AudioGeneratorMP3 *beep;
//static AudioGeneratorWAV *beep;

#ifdef USE_SPIFFS
static AudioFileSourceSPIFFS *myFS0;
//static AudioFileSourceSPIFFS *myFS1;
#else
static AudioFileSourceLittleFS *myFS0;
//static AudioFileSourceLittleFS *myFS1;
#endif

static AudioFileSourceSD *mySD0;

static AudioOutputI2S *out;

#ifdef TC_USE_MIXER
static AudioOutputMixer *mixer;
static AudioOutputMixerStub *stub[2];
#endif

bool audioInitDone = false;
bool audioMute = false;

bool muteBeep = true;

bool haveMusic = false;
bool mpActive = false;
static uint16_t maxMusic = 0;
static uint16_t *playList = NULL;
static int  mpCurrIdx = 0;
static bool mpShuffle = false;

static const float volTable[20] = {
    0.00, 0.02, 0.04, 0.06,
    0.08, 0.10, 0.13, 0.16,
    0.19, 0.22, 0.26, 0.30,
    0.35, 0.40, 0.50, 0.60,
    0.70, 0.80, 0.90, 1.00
};

uint8_t curVolume = DEFAULT_VOLUME;

static float curVolFact[2] = { 1.0, 1.0 };
static bool  curChkNM[2]   = { true, true };

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
static float getVolume(int channel);

/*
 * audio_setup()
 */
void audio_setup()
{
    audioLogger = &Serial;

    // Set resolution for volume pot
    analogReadResolution(POT_RESOLUTION);
    analogSetWidth(POT_RESOLUTION);

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DIN_PIN);

    #ifdef TC_USE_MIXER
    mixer = new AudioOutputMixer(8, out);
    #endif

    mp3  = new AudioGeneratorMP3();
    //beep = new AudioGeneratorMP3();
    //beep = new AudioGeneratorWAV();

    #ifdef USE_SPIFFS
    myFS0 = new AudioFileSourceSPIFFS();
    //myFS1 = new AudioFileSourceSPIFFS();
    #else
    myFS0 = new AudioFileSourceLittleFS();
    //myFS1 = new AudioFileSourceLittleFS();
    #endif

    if(haveSD) {
        mySD0 = new AudioFileSourceSD();
    }

    #ifdef TC_USE_MIXER
    stub[0] = mixer->NewInput();
    //stub[1] = mixer->NewInput();
    #endif

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
            mpActive = true;
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
        #ifdef TC_USE_MIXER
        stub[0]->stop();
        #endif
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
        if(force) play_file(fnbuf, 1.0, true, true);
        return true;
    }
    return false;
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
        play_file(buf, 0.6, true, false, false);
    }
}

void play_hour_sound(int hour)
{
    char buf[16];

    if(!haveSD || mpActive) return;
    
    sprintf(buf, "/hour-%02d.mp3", hour);
    if(SD.exists(buf)) {
        play_file(buf, 1.0, false, false);
        return;
    }
    
    play_file("/hour.mp3", 1.0, false, false);
}

void play_beep()
{
    if(muteBeep || mpActive || mp3->isRunning())
        return;

    play_file("/beep.mp3", 0.3, true, false, false);
}

/*
 * audio_loop()
 *
 */
void audio_loop()
{   
    if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            #ifdef TC_USE_MIXER
            stub[0]->stop();
            #endif
            if(mpActive) {
                mp_next(true);
            }
        } else {
            sampleCnt++;
            if(sampleCnt > 1) {
                #ifdef TC_USE_MIXER
                stub[0]->SetGain(getVolume(0));
                #else
                out->SetGain(getVolume(0));
                #endif
                sampleCnt = 0;
            }
        }
    } else if(mpActive) {
        pwrNeedFullNow();
        mp_next(true);
    }
    /*
    if(beep->isRunning()) {
        if(!beep->loop()) {
            beep->stop();
            stub[1]->stop();
        }
    }
    */
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

void play_file(const char *audio_file, float volumeFactor, bool checkNightMode, bool interruptMusic, bool allowSD, int channel)
{
    char buf[10];
    
    if(audioMute) return;

    if(channel != 0) return;  // For now, only 0 is allowed

    if(!interruptMusic) {
        if(mpActive) return;
    } else {
        mpActive = false;
    }

    pwrNeedFullNow();

    #ifdef TC_DBG
    Serial.printf("Audio: Playing %s (channel %d)\n", audio_file, channel);
    #endif

    // If something is currently on, kill it
    if((channel == 0) && mp3->isRunning()) {
        mp3->stop();
        #ifdef TC_USE_MIXER
        stub[0]->stop();
        #endif
    }

    /*
    if((channel == 1) && beep->isRunning()) {
        beep->stop();
        stub[1]->stop();
    }
    */

    curVolFact[channel] = volumeFactor;
    curChkNM[channel] = checkNightMode;

    #ifdef TC_USE_MIXER
    stub[channel]->SetGain(getVolume(channel));
    #else
    out->SetGain(getVolume(channel));
    #endif

    //if(channel == 0) {
        if(haveSD && (allowSD || FlashROMode) && mySD0->open(audio_file)) {

            buf[0] = 0;
            mySD0->read((void *)buf, 10);
            mySD0->seek(skipID3(buf), SEEK_SET);
          
            #ifdef TC_USE_MIXER
            mp3->begin(mySD0, stub[0]);
            #else
            mp3->begin(mySD0, out);
            #endif
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
            buf[0] = 0;
            myFS0->read((void *)buf, 10);
            myFS0->seek(skipID3(buf), SEEK_SET);
            
            #ifdef TC_USE_MIXER
            mp3->begin(myFS0, stub[0]);
            #else
            mp3->begin(myFS0, out);
            #endif
            #ifdef TC_DBG
            Serial.println(F("Playing from flash FS"));
            #endif
        } else {
            #ifdef TC_DBG
            Serial.println(F("Audio file not found"));
            #endif
        }
    //} else {
    //    myFS1->open(audio_file);
    //    beep->begin(myFS1, stub[1]);
    //}
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

static float getVolume(int channel)
{
    float vol_val;

    if(curVolume == 255) {
        vol_val = getRawVolume();
    } else {
        vol_val = volTable[curVolume];
    }

    // If user muted, return 0
    if(vol_val == 0.0) return vol_val;

    vol_val *= curVolFact[channel];
    
    // Do not totally mute
    // 0.02 is the lowest audible gain
    if(vol_val < 0.02) vol_val = 0.02;

    // Reduce volume in night mode, if requested
    if(curChkNM[channel] && presentTime.getNightMode()) {
        vol_val *= 0.3;
        // Do not totally mute
        if(vol_val < 0.02) vol_val = 0.02;
    }

    return vol_val;
}

bool checkAudioDone()
{
    if( (mp3->isRunning()) /*|| (beep->isRunning())*/ ) return false;
    return true;
}

void stopAudio()
{
    if(mp3->isRunning()) {
        mp3->stop();
        #ifdef TC_USE_MIXER
        stub[0]->stop();
        #endif
    }
    /*
    if(beep->isRunning()) {
        beep->stop();
        stub[1]->stop();
    }
    */
}
