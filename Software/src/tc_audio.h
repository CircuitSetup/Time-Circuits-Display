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

#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputMixer.h>
//#include <AudioFileSourceFunction.h>
//#include <AudioGeneratorWAV.h>

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "driver/i2s.h"
#include "tc_keypad.h"

// SD Card
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

//I2S audio
#define I2S_BCLK 26
#define I2S_LRCLK 25
#define I2S_DIN 33

#define VOLUME 32

extern void audio_setup();
extern void play_keypad_sound(char key);
extern void play_startup();
extern void audio_loop();
extern void play_file(const char *audio_file, double volume = 0.1, int channel = 0, bool firstStart = false);
//extern void play_DTMF(float hz1, float hz2, double volume = 0.1);
extern double getVolume();
extern bool beepOn;

#endif