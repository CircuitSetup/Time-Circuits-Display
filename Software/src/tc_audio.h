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

#ifndef _TC_AUDIO_H
#define _TC_AUDIO_H

extern bool audioMute;

extern uint8_t curVolume;

void audio_setup();
void play_keypad_sound(char key);
void play_hour_sound(int hour);
void audio_loop();
void play_file(const char *audio_file, double volumeFactor = 1.0, bool checkNightMode = true, int channel = 0, bool allowSD = true);
bool checkAudioDone();
void stopAudio();

#endif
