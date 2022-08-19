/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * 
 * Code based on Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 *
 * Enhanced/modified/written in 2022 by Thomas Winischhofer (A10001986)
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
 * 
 */

#include "tc_audio.h"

// Use the mixer, or do not use the mixer.
// Since we don't use mixing, it could be turned off, but the sounds and anims
// are synced to the timing with the mixer. 
// Also, it seems the raw i2s output sometimes grables output at the end of
// a sound.
#define TC_USE_MIXER

// Initialize ESP32 Audio Library classes
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *mySPIFFS0;
AudioFileSourceSD *mySD0;

//AudioGeneratorMP3 *beep;
//AudioGeneratorWAV *beep;
//AudioFileSourceSPIFFS *mySPIFFS1;

AudioOutputI2S *out;

#ifdef TC_USE_MIXER
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];
#endif

bool beepOn;

bool audioMute = false;

double curVolFact[2] = { 1.0, 1.0 };
bool   curChkNM[2]   = { true, true };

int sampleCnt = 0;

#define VOL_SMOOTH_SIZE 8
int rawVol[VOL_SMOOTH_SIZE] = {  };
int rawVolIdx = 0;
int anaReadCount = 0;
double prev_vol = 10.0;

/*
 * audio_setup()
 */
void audio_setup() 
{
    audioLogger = &Serial;

    analogSetWidth(9);

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);

    #ifdef TC_USE_MIXER
    mixer = new AudioOutputMixer(8, out); 
    #endif

    mp3  = new AudioGeneratorMP3();
    //beep = new AudioGeneratorMP3();
    //beep = new AudioGeneratorWAV();
    
    mySPIFFS0 = new AudioFileSourceSPIFFS();
    //mySPIFFS1 = new AudioFileSourceSPIFFS();

    if(haveSD) {
        mySD0 = new AudioFileSourceSD();
    }

    #ifdef TC_USE_MIXER
    stub[0] = mixer->NewInput();    
    //stub[1] = mixer->NewInput();
    #endif
}

// Play startup sound
void play_startup() 
{
    play_file("/startup.mp3", 1, true, 0);
}

// Play alarm sound
// always at normal volume, not nm-reduced
void play_alarm() 
{
    play_file("/alarm.mp3", 1, false, 0);    
}

void play_keypad_sound(char key) 
{
    char buf[16] = "/Dtmf-0.mp3\0";
    
    if(key) {
        beepOn = false;
        buf[6] = key;
        play_file(buf, 0.8, true, 0);
    }
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
            //stub[0]->flush();
            //out->flush();
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
            //stub[1]->flush();
            //out->flush();
        }
    }
    */
}

void play_file(const char *audio_file, double volumeFactor, bool checkNightMode, int channel) 
{
    if(audioMute) return;
    
    if(channel != 0) return;  // For now, only 0 is allowed

    #ifdef TC_DBG
    Serial.printf("CH:");
    Serial.print(channel);
    Serial.printf("  Playing <");
    Serial.println(audio_file);    
    #endif

    // If something is currently on, kill it
    if((channel == 0) && mp3->isRunning()) {          
        mp3->stop();
        #ifdef TC_USE_MIXER
        stub[0]->stop();        
        //stub[0]->flush();
        #endif
        //out->flush();          
    }

    /*
    if((channel == 1) && beep->isRunning()) {          
        beep->stop();
        stub[1]->stop();
        //stub[1]->flush();
        //out->flush();
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
        if(haveSD && mySD0->open(audio_file)) {
            #ifdef TC_USE_MIXER           
            mp3->begin(mySD0, stub[0]);
            #else
            mp3->begin(mySD0, out);
            #endif
            Serial.println("Playing from SD");
        } else {
            mySPIFFS0->open(audio_file);  
            #ifdef TC_USE_MIXER
            mp3->begin(mySPIFFS0, stub[0]);
            #else
            mp3->begin(mySPIFFS0, out);
            #endif
            Serial.println("Playing from SPIFFS");
        }
    //} else {        
    //    mySPIFFS1->open(audio_file);
    //    beep->begin(mySPIFFS1, stub[1]);
    //}
}

// Returns value for volume based on the position of the pot
// Since the values vary very much we do some noise reduction
double getRawVolume() 
{
    double vol_val; 
    unsigned long avg = 0;
    
    rawVol[rawVolIdx] = analogRead(VOLUME);

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
    
    vol_val = (double)avg / 511.0; 
    
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
    double vol_val = getRawVolume();

    // If user muted, return 0
    if(vol_val == 0.0) return vol_val;

    vol_val *= curVolFact[channel];

    // Reduce volume in night mode, if requested
    if(curChkNM[channel] && presentTime.getNightMode()) {
        vol_val *= 0.3;
        // Do not totally mute in night mode
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
        //stub[0]->flush();
        #endif 
        //out->flush();                 
    }
    /*
    if(beep->isRunning()) {          
        beep->stop();
        stub[1]->stop();
        //stub[1]->flush();
        //out->flush();
    }
    */
}
