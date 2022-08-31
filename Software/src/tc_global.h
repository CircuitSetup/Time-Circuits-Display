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

#ifndef _TC_GLOBAL_H
#define _TC_GLOBAL_H

// Uncomment if month is 2 digits (7-seg), as in the original A-Car display.
//#define IS_ACAR_DISPLAY 

//#define TC_DBG              // debug output on Serial

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
// As long as SPIFFS is around, and LittleFS does not support wear leveling,
// we go with SPIFFS.
//#define USE_SPIFFS

//#define TWSOUND         // Use A10001986's sound files
//#define TWPRIVATE     // A10001986's private customizations

// Version strings. 
// These must not contain any characters other than 
// '0'-'9', 'A'-'Z', '(', ')', '.', '_', '-' or space
#ifndef IS_ACAR_DISPLAY
#define TC_VERSION "V2.0.0"           // 13 chars max
#define TC_VERSION_EXTRA "AUG312022"  // 13 chars max
#else   // A-Car
#define TC_VERSION "V2.0.0_A-CAR"     // 12 chars max
#define TC_VERSION_EXTRA "08312022"   // 12 chars max
#endif

#ifdef TWPRIVATE
#undef TC_VERSION
#define TC_VERSION "A10001986P"
#elif defined(TWSOUND)
#undef TC_VERSION
#define TC_VERSION "A10001986"
#endif

// GPIO pins

#define STATUS_LED_PIN     2      // Status LED (on ESP)
#define SECONDS_IN_PIN    15      // SQW Monitor 1Hz from the DS3231
#define ENTER_BUTTON_PIN  16      // enter key
#define WHITE_LED_PIN     17      // white led

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18

#define VOLUME_PIN        32      // analog input pin

// Fake Power On:
// Attach an active-low switch to io13; firmware will start network and 
// sync time, but not enable displays until the switch is activated.
// The white led will flash for 0.5 seconds when the unit is ready to be "fake"
// powered on. De-activating the switch "fake" powers down the device (ie the
// displays are switched off, and no keypad input is accepted)
#define FAKE_POWER_ON             // Wait for switch on io13 before starting displays
#define FAKE_POWER_BUTTON_PIN 13  // GPIO that fake power switch is connected to

// External time travel (input)
// If the defined pin goes low a time travel is triggered (eg when 88mph is reached)
// ATTN: We only simulate re-entry, ie the part of time travel that takes
// place in the new time. So the external time travel should be initiated when
// the present-time part of time travel is over.
#define EXTERNAL_TIMETRAVEL_IN         // Initiate time travel is pin is activated
#define EXTERNAL_TIMETRAVEL_IN_PIN 14  // GPIO that is polled

// External time travel (output)
// TODO - not yet implemented
#define EXTERNAL_TIMETRAVEL_OUT         // Pins activates if tt is triggered from tc
#define EXTERNAL_TIMETRAVEL_OUT_PIN 27  // GPIO that is activated

// EEPROM map
// We use 2(padded to 8) + 10*3 + 4 bytes of EEPROM space at 0x0. 
#define SWVOL_PREF        0x00    // volume save location         (2 bytes, padded to 8)
#define DEST_TIME_PREF    0x08    // destination time prefs       (10 bytes)
#define PRES_TIME_PREF    0x12    // present time prefs           (10 bytes)
#define DEPT_TIME_PREF    0x1c    // departure time prefs         (10 bytes)
#define ALARM_PREF        0x26    // alarm prefs                  (4 bytes; only used if fs unavailable)

#endif
