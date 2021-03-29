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
AudioGeneratorMP3 *mp3 = new AudioGeneratorMP3();
AudioFileSourceSPIFFS *file = NULL;
AudioOutputI2S *out = new AudioOutputI2S(0, 0, 32, 0);

void audio_setup() {
    // for SD card
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(SPI_SCK, INPUT_PULLUP);
    pinMode(SPI_MISO, INPUT_PULLUP);
    pinMode(SPI_MOSI, INPUT_PULLUP);

    // set up SD card
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    delay(1000);
    if (!SD.begin(SD_CS)) {
        Serial.println("Error talking to SD card!");
        // return; //while(true);  // end program
    } else {
        Serial.println("SD card initialized");
    }

    SPIFFS.begin();
    audioLogger = &Serial;

    //file = new AudioFileSourceSPIFFS("/startup.mp3");
    out->SetGain(0.06);  //Set the volume
    out->SetOutputModeMono(true);
    out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);
    //mp3->begin(file, out);  //Start playing the track loaded

}

void play_keypad_sound(char key) {
    if (key) {
        if (key == '0') file = new AudioFileSourceSPIFFS("/Dtmf-0.mp3");
        if (key == '1') file = new AudioFileSourceSPIFFS("/Dtmf-1.mp3");
        if (key == '2') file = new AudioFileSourceSPIFFS("/Dtmf-2.mp3");
        if (key == '3') file = new AudioFileSourceSPIFFS("/Dtmf-3.mp3");
        if (key == '4') file = new AudioFileSourceSPIFFS("/Dtmf-4.mp3");
        if (key == '5') file = new AudioFileSourceSPIFFS("/Dtmf-5.mp3");
        if (key == '6') file = new AudioFileSourceSPIFFS("/Dtmf-6.mp3");
        if (key == '7') file = new AudioFileSourceSPIFFS("/Dtmf-7.mp3");
        if (key == '8') file = new AudioFileSourceSPIFFS("/Dtmf-8.mp3");
        if (key == '9') file = new AudioFileSourceSPIFFS("/Dtmf-9.mp3");

        /*out = new AudioOutputI2S(0, 0, 8, 0);
        mp3 = new AudioGeneratorMP3();
        out->SetGain(0.06);  //Set the volume
        */
        mp3->begin(file, out);  //Start playing the track loaded
        out->flush(); 
    }
}

void audio_loop() {
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            out->stop();
            delete file;
            //delete out;
            //delete mp3;
        }
    }
}

void play_file(const char *audio_file) {
    file = new AudioFileSourceSPIFFS(audio_file);
    //out->SetGain(0.06);  //Set the volume
    //out->SetOutputModeMono(true);
    //out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);
    mp3->begin(file, out);  //Start playing the track loaded
    out->flush(); 
}