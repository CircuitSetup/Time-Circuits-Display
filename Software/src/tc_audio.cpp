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

/*
AudioGeneratorWAV *wav = new AudioGeneratorWAV();
AudioFileSourceFunction *genFile;
*/
AudioFileSourceSPIFFS *file[2];
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
    // for SD card
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(SPI_SCK, INPUT_PULLUP);
    pinMode(SPI_MISO, INPUT_PULLUP);
    pinMode(SPI_MOSI, INPUT_PULLUP);

    //pinMode(VOLUME, INPUT);

    // set up SD card
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    
    //delay(1000);
    
    /*
    if (!SD.begin(SD_CS)) {
        Serial.println("Error talking to SD card!");
    } else {
        Serial.println("SD card initialized");
    }
    */
   
    SPIFFS.begin();

    audioLogger = &Serial;

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);
    mixer = new AudioOutputMixer(8, out);
    mp3 = new AudioGeneratorMP3();
    beep = new AudioGeneratorMP3();
}

void play_startup() 
{
    play_file("/startup.mp3", getVolume(), 0, true);
}

void play_keypad_sound(char key) 
{
    if(key) {
        beepOn = false;
        switch(key) {
          case '0': play_file("/Dtmf-0.mp3", getVolume(), 0, false); break;
          case '1': play_file("/Dtmf-1.mp3", getVolume(), 0, false); break;
          case '2': play_file("/Dtmf-2.mp3", getVolume(), 0, false); break;
          case '3': play_file("/Dtmf-3.mp3", getVolume(), 0, false); break;
          case '4': play_file("/Dtmf-4.mp3", getVolume(), 0, false); break;
          case '5': play_file("/Dtmf-5.mp3", getVolume(), 0, false); break;
          case '6': play_file("/Dtmf-6.mp3", getVolume(), 0, false); break;
          case '7': play_file("/Dtmf-7.mp3", getVolume(), 0, false); break;
          case '8': play_file("/Dtmf-8.mp3", getVolume(), 0, false); break;
          case '9': play_file("/Dtmf-9.mp3", getVolume(), 0, false); break;
        }
    }
}

/*
 * audio_loop()
 */
void audio_loop() 
{
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            stub[0]->stop();
            stub[0]->flush();
            out->flush();
        }
    }
    if (beep->isRunning()) {
        if (!beep->loop()) {
            beep->stop();
            stub[1]->stop();
            stub[1]->flush();
            out->flush();
        }
    }
    /*
    if (wav->isRunning()) {
        if (!wav->loop()) {
            wav->stop();
            out->flush();
            //delete genFile;
        }
    }
    */
}

void play_file(const char *audio_file, double volume, int channel, bool firstStart) 
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

    if(!firstStart) {
        firstStart = false;
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
    } else {
        mp3 = new AudioGeneratorMP3();
        beep = new AudioGeneratorMP3();
        mySPIFFS0 = new AudioFileSourceSPIFFS();
        mySPIFFS1 = new AudioFileSourceSPIFFS();
        stub[0] = mixer->NewInput();
        stub[1] = mixer->NewInput();
    }

    stub[channel]->SetGain(volume);

    if(channel == 0) {        
        mySPIFFS0->open(audio_file);  
        mp3->begin(mySPIFFS0, stub[0]);
    } else {
        //beep = new AudioGeneratorMP3();
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