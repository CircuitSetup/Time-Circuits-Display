/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022 Thomas Winischhofer (A10001986)
 *
 * Global definitions
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

#ifndef _TC_GLOBAL_H
#define _TC_GLOBAL_H

// Version strings.
// These must not contain any characters other than
// '0'-'9', 'A'-'Z', '(', ')', '.', '_', '-' or space
#ifndef IS_ACAR_DISPLAY
#define TC_VERSION "V2.1.0"           // 13 chars max
#define TC_VERSION_EXTRA "OCT102022"  // 13 chars max
#else   // A-Car
#define TC_VERSION "V2.1.0_A-CAR"     // 12 chars max
#define TC_VERSION_EXTRA "10102022"   // 12 chars max
#endif

//#define TC_DBG            // debug output on Serial

// Uncomment if month is 2 digits (7-seg), as in the original A-Car display.
//#define IS_ACAR_DISPLAY

// Uncomment for support of speedo-display connected via i2c (0x70)
//#define TC_HAVESPEEDO
#define SP_NUM_TYPES    10  // Number of speedo display types supported
#define SP_MIN_TYPE     1   // Change to 0 when CircuitSetup speedo prop exists

// Uncomment for support of gps receiver (MT3333-based) connected via i2c (0x10)
// Will be used to display actual speed on speedometer display
#define TC_HAVEGPS

// Uncomment for support of temperature sensor (MCP9808-based) connected via i2c (0x18)
// Will be used to display ambient temperature on speedometer display when idle.
// GPS has higher priority, ie if GPS is used, temp will not be shown.
#define TC_HAVETEMP

// Fake Power Switch:
// Attach an active-low switch to io13; firmware will start network and
// sync time, but not enable displays until the switch is activated.
// The white led will flash for 0.5 seconds when the unit is ready to be "fake"
// powered on. De-activating the switch "fake" powers down the device (ie the
// displays are switched off, and no keypad input is accepted)
// GPS speed will be shown on speedo display regardless of this switch.
// Uncomment to include support for Fake Power Switch
#define FAKE_POWER_ON
#define FAKE_POWER_BUTTON_PIN 13  // GPIO that fake power switch is connected to

// External time travel (input) ("ett")
// If the defined pin goes low (by connecting it to GND using a button), a time
// travel is triggered.
// Uncomment to include support for ett
#define EXTERNAL_TIMETRAVEL_IN
#define EXTERNAL_TIMETRAVEL_IN_PIN 27  // GPIO that is polled

// External time travel (output) ("etto")
// The defined pin is set HIGH on a time travel, and LOW upon re-entry from a
// time travel. See tc_time.c for a timing diagram.
// Uncomment to include support for etto
#define EXTERNAL_TIMETRAVEL_OUT
#define EXTERNAL_TIMETRAVEL_OUT_PIN 14  // GPIO that is activated

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
// As long as SPIFFS is around, and LittleFS does not support wear leveling,
// we go with SPIFFS.
//#define USE_SPIFFS

//#define TWSOUND         // Use A10001986's sound files
//#define TWPRIVATE     // A10001986's private customizations

#ifdef TWPRIVATE
#undef TC_VERSION
#define TC_VERSION "A10001986P"
#undef SP_NUM_TYPES
#define SP_NUM_TYPES 12
#elif defined(TWSOUND)
#undef TC_VERSION
#define TC_VERSION "A10001986"
#endif

// No GPS or temperature without speedo
#ifndef TC_HAVESPEEDO
#undef TC_HAVEGPS
#undef TC_HAVETEMP
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

// EEPROM map
// We use 2(padded to 8) + 10*3 + 4 bytes of EEPROM space at 0x0.
#define SWVOL_PREF        0x00    // volume save location         (2 bytes, padded to 8)
#define DEST_TIME_PREF    0x08    // destination time prefs       (10 bytes)
#define PRES_TIME_PREF    0x12    // present time prefs           (10 bytes)
#define DEPT_TIME_PREF    0x1c    // departure time prefs         (10 bytes)
#define ALARM_PREF        0x26    // alarm prefs                  (4 bytes; only used if fs unavailable)

#endif
