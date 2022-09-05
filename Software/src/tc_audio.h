/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us 
 * (C) 2022 Thomas Winischhofer (A10001986)
 * 
 * Clockdisplay and keypad menu code based on code by John Monaco
 * Marmoset Electronics 
 * https://www.marmosetelectronics.com/time-circuits-clock
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

#include "tc_global.h"

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
#include <AudioGeneratorWAV.h>
#include <AudioOutputMixer.h>

#include "tc_keypad.h"
#include "tc_time.h"

extern void audio_setup();
extern void play_keypad_sound(char key);
extern void audio_loop();
extern void play_file(const char *audio_file, double volumeFactor = 1.0, bool checkNightMode = true, int channel = 0, bool allowSD = true);
extern double getRawVolume();
extern double getVolume(int channel);
extern bool checkAudioDone();
extern void stopAudio();

extern bool audioMute;

extern uint8_t curVolume;

#endif
