/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * Code adapted from Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
 * by John Monaco
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

#include "tc_audio.h"

//Initialize ESP32 Audio Library classes
AudioGeneratorMP3 *mp3;
AudioGeneratorMP3 *beep;
/*
AudioGeneratorWAV *wav = new AudioGeneratorWAV();
AudioFileSourceFunction *genFile;
*/
AudioFileSourceSPIFFS *file[2];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];

bool beepOn;

void audio_setup() {
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

void play_startup() {
    play_file("/startup.mp3", getVolume(), 0, true);
}

void play_keypad_sound(char key) {
    if (key) {
        beepOn = false;
        if (key == '0') play_file("/Dtmf-0.mp3", getVolume(), 0, false);
        if (key == '1') play_file("/Dtmf-1.mp3", getVolume(), 0, false);
        if (key == '2') play_file("/Dtmf-2.mp3", getVolume(), 0, false);
        if (key == '3') play_file("/Dtmf-3.mp3", getVolume(), 0, false);
        if (key == '4') play_file("/Dtmf-4.mp3", getVolume(), 0, false);
        if (key == '5') play_file("/Dtmf-5.mp3", getVolume(), 0, false);
        if (key == '6') play_file("/Dtmf-6.mp3", getVolume(), 0, false);
        if (key == '7') play_file("/Dtmf-7.mp3", getVolume(), 0, false);
        if (key == '8') play_file("/Dtmf-8.mp3", getVolume(), 0, false);
        if (key == '9') play_file("/Dtmf-9.mp3", getVolume(), 0, false);
        /*
        if (key == '0') play_DTMF(1336.f, 941.f, getVolume());
        if (key == '1') play_DTMF(1209.f, 697.f, getVolume());
        if (key == '2') play_DTMF(1336.f, 697.f, getVolume());
        if (key == '3') play_DTMF(1477.f, 697.f, getVolume());
        if (key == '4') play_DTMF(1209.f, 770.f, getVolume());
        if (key == '5') play_DTMF(1336.f, 770.f, getVolume());
        if (key == '6') play_DTMF(1477.f, 770.f, getVolume());
        if (key == '7') play_DTMF(1209.f, 852.f, getVolume());
        if (key == '8') play_DTMF(1336.f, 852.f, getVolume());
        if (key == '9') play_DTMF(1477.f, 852.f, getVolume());
        */
    }
}

void audio_loop() {
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

void play_file(const char *audio_file, double volume, int channel, bool firstStart) {
    Serial.printf("CH:");
    Serial.print(channel);
    Serial.printf("  Playing <");
    Serial.print(audio_file);
    Serial.printf("> at volume :");
    Serial.println(volume);

    if (!firstStart) {
        //cant do this if playing the first file after startup
        delete stub[channel];
        delete file[channel];
        //if (channel == 0) delete mp3;
        if (channel == 1) delete beep;
        firstStart = false;
    }
    stub[channel] = mixer->NewInput();
    stub[channel]->SetGain(volume);

    file[channel] = new AudioFileSourceSPIFFS(audio_file);

    mp3 = new AudioGeneratorMP3();

    if (channel == 0) {
        mp3 = new AudioGeneratorMP3();
        mp3->begin(file[0], stub[0]);
    } else {
        beep = new AudioGeneratorMP3();
        beep->begin(file[1], stub[1]);
    }
}

/*
void play_DTMF(float hz1, float hz2, double volume) {
    if (genFile) delete genFile;
    
    genFile = new AudioFileSourceFunction(8., 1, 16000, 16);
    genFile->addAudioGenerators([&](const float time) {
        float v = sin(TWO_PI * hz1 * time);  // generate sine wave
        v *= fmod(time, 1.f);               // change linear
        v *= 0.5;                           // scale
        return v;
    });

    wav = new AudioGeneratorWAV();
    wav->begin(genFile, out);
}
*/

double getVolume() {
    //returns value for volume based on the position of the pot
    double vol_val = analogRead(VOLUME);
    vol_val = vol_val * 1/4095;

    return vol_val;
}