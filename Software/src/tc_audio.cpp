/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
 * Enhanced/modified in 2022 by Thomas Winischhofer (A10001986)
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

// Initialize ESP32 Audio Library classes
AudioGeneratorMP3 *mp3;
AudioGeneratorMP3 *beep;
AudioFileSourceSPIFFS *mySPIFFS0;
AudioFileSourceSPIFFS *mySPIFFS1;

AudioFileSourceSD *mySD0;

AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];

bool beepOn;

bool audioMute = false;

/*
 * audio_setup()
 */
void audio_setup() 
{
    audioLogger = &Serial;

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);
    
    mixer = new AudioOutputMixer(8, out);

    mp3  = new AudioGeneratorMP3();
    beep = new AudioGeneratorMP3();
    
    mySPIFFS0 = new AudioFileSourceSPIFFS();
    mySPIFFS1 = new AudioFileSourceSPIFFS();

    if(haveSD) {
        mySD0 = new AudioFileSourceSD();
    }
    
    stub[0] = mixer->NewInput();
    stub[1] = mixer->NewInput();
   
}

void play_startup(bool nm) 
{
    play_file("/startup.mp3", getVolumeNM(nm), 0);
}

// Play alarm sound
// always at normal volume, not nm-reduced
void play_alarm() 
{
    play_file("/alarm.mp3", getVolume(), 0);    
}

void play_keypad_sound(char key, bool nm) 
{
    char buf[16] = "/Dtmf-0.mp3\0";
    
    if(key) {
        beepOn = false;
        buf[6] = key;
        play_file(buf, getVolumeNM(nm) * 0.8, 0);
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
            stub[0]->stop();
            stub[0]->flush();
            out->flush();
        }
    }
    if(beep->isRunning()) {
        if(!beep->loop()) {
            beep->stop();
            stub[1]->stop();
            stub[1]->flush();
            out->flush();
        }
    }
}

void play_file(char *audio_file, double volume, int channel) 
{
    if(audioMute) return;

    #ifdef TC_DBG
    Serial.printf("CH:");
    Serial.print(channel);
    Serial.printf("  Playing <");
    Serial.print(audio_file);
    Serial.printf("> at volume :");
    Serial.println(volume);
    #endif

    // If something is currently on, kill it
    if(mp3->isRunning()) {          
        mp3->stop();
        stub[0]->stop();
        stub[0]->flush();
        out->flush();          
    }
    if(beep->isRunning()) {          
        beep->stop();
        stub[1]->stop();
        stub[1]->flush();
        out->flush();
    }

    stub[channel]->SetGain(volume);

    if(channel == 0) { 
        if(haveSD && mySD0->open(audio_file)) {           
            mp3->begin(mySD0, stub[0]);
            Serial.println("Playing from SD");
        } else {
            mySPIFFS0->open(audio_file);  
            mp3->begin(mySPIFFS0, stub[0]);
            Serial.println("Playing from SPIFFS");
        }
    } else {        
        mySPIFFS1->open(audio_file);
        beep->begin(mySPIFFS1, stub[1]);
    }
}

// returns value for volume based on the position of the pot
double getVolume() 
{
    double vol_val = analogRead(VOLUME);
    
    vol_val = vol_val * 1/4095;

    return vol_val;
}

double getVolumeNM(bool nm) 
{
    double vol_val = getVolume();

    // If user muted, return
    if(vol_val == 0.0) return vol_val;

    // Reduce volume in night mode
    if(nm) {
        vol_val = vol_val * 0.3;
        // Do not totally mute in night mode
        if(vol_val < 0.03) vol_val = 0.03;
    }

    return vol_val;
}

bool checkAudioDone()
{
    if( (mp3->isRunning()) || (beep->isRunning()) ) return false;
    return true;
}

void stopAudio()
{
    if(mp3->isRunning()) {          
        mp3->stop();
        stub[0]->stop();
        stub[0]->flush();
        out->flush();          
    }
    if(beep->isRunning()) {          
        beep->stop();
        stub[1]->stop();
        stub[1]->flush();
        out->flush();
    }
}
