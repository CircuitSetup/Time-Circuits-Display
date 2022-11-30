/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tc_global.h"

// Use the mixer, or do not use the mixer.
// Since we don't use mixing, turn it off.
// With the current versions of the audio library,
// turning it on might cause a static after stopping
// sound play back.
//#define TC_USE_MIXER

#include <Arduino.h>
#include <AudioOutputI2S.h>
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

bool audioMute = false;

static const double volTable[16] = {
    0.00, 0.03, 0.06, 0.10,
    0.15, 0.20, 0.25, 0.30,
    0.35, 0.42, 0.50, 0.60,
    0.70, 0.80, 0.90, 1.00
};

uint8_t curVolume = 4;

double  curVolFact[2] = { 1.0, 1.0 };
bool    curChkNM[2]   = { true, true };

static int sampleCnt = 0;

#define VOL_SMOOTH_SIZE 4
static int rawVol[VOL_SMOOTH_SIZE];
static int rawVolIdx = 0;
static int anaReadCount = 0;
static double prev_vol = 10.0;

// Resolution for pot, 9-12 allowed
#define POT_RESOLUTION 9

static double getRawVolume();
static double getVolume(int channel);

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
}

void play_keypad_sound(char key)
{
    char buf[16] = "/Dtmf-0.mp3\0";

    if(key) {
        buf[6] = key;
        play_file(buf, 0.6, true, 0, false);
    }
}

void play_hour_sound(int hour)
{
    char buf[16];

    if(!haveSD) return;
    
    sprintf(buf, "/hour-%02d.mp3", hour);
    if(mySD0->open(buf)) {
        mySD0->close();
        play_file(buf, 1.0, false, 0);
        return;
    }
    
    play_file("/hour.mp3", 1.0, false, 0);
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

void play_file(const char *audio_file, double volumeFactor, bool checkNightMode, int channel, bool allowSD)
{
    if(audioMute) return;

    if(channel != 0) return;  // For now, only 0 is allowed

    pwrNeedFullNow();

    #ifdef TC_DBG
    Serial.print(F("CH:"));
    Serial.print(channel);
    Serial.print(F("  Playing "));
    Serial.println(audio_file);
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
        if(haveSD && allowSD && mySD0->open(audio_file)) {
            #ifdef TC_USE_MIXER
            mp3->begin(mySD0, stub[0]);
            #else
            mp3->begin(mySD0, out);
            #endif
            #ifdef TC_DBG
            Serial.println(F("Playing from SD"));
            #endif
        #ifdef USE_SPIFFS
        } else if(SPIFFS.exists(audio_file) && myFS0->open(audio_file)) {
        #else    
        } else if(myFS0->open(audio_file)) {
        #endif
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
double getRawVolume()
{
    double vol_val;
    unsigned long avg = 0;

    rawVol[rawVolIdx] = analogRead(VOLUME_PIN);

    if(anaReadCount > 1) {
        avg = 0;
        for(int i = rawVolIdx; i > rawVolIdx - anaReadCount; i--) {
            avg += rawVol[i & (VOL_SMOOTH_SIZE-1)];
        }
        avg /= anaReadCount;
        if(anaReadCount < VOL_SMOOTH_SIZE) anaReadCount++;
    } else {
        anaReadCount++;
        avg = rawVol[rawVolIdx];
    }

    rawVolIdx++;
    rawVolIdx &= (VOL_SMOOTH_SIZE-1);

    vol_val = (double)avg / (double)((1<<POT_RESOLUTION)-1);

    if(fabs(vol_val - prev_vol) <= 0.015) {
        vol_val = prev_vol;
    } else {
        prev_vol = vol_val;
    }

    if(vol_val < 0.02) vol_val = 0.0;

    return vol_val;
}

double getVolume(int channel)
{
    double vol_val;

    if(curVolume == 255) {
        vol_val = getRawVolume();
    } else {
        vol_val = (double)volTable[curVolume];
    }

    // If user muted, return 0
    if(vol_val == 0.0) return vol_val;

    vol_val *= curVolFact[channel];
    // Do not totally mute
    if(vol_val < 0.03) vol_val = 0.03;

    // Reduce volume in night mode, if requested
    if(curChkNM[channel] && presentTime.getNightMode()) {
        vol_val *= 0.3;
        // Do not totally mute
        if(vol_val < 0.03) vol_val = 0.03;
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
