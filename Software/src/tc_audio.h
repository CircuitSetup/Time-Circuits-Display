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

#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputMixer.h>

#include "tc_global.h"
#include "tc_keypad.h"
#include "tc_time.h"

// I2S audio pins
#define I2S_BCLK  26
#define I2S_LRCLK 25
#define I2S_DIN   33

// analog input pin
#define VOLUME    32

extern void audio_setup();
extern void play_keypad_sound(char key);
extern void play_startup();
extern void play_alarm();
extern void audio_loop();
extern void play_file(const char *audio_file, double volumeFactor = 1, bool checkNightMode = true, int channel = 0);
extern double getRawVolume();
extern double getVolume(int channel);
extern bool checkAudioDone();
extern void stopAudio();
extern bool beepOn;

extern bool audioMute;

#endif
