/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Global definitions
 */

#ifndef _TC_GLOBAL_H
#define _TC_GLOBAL_H

/*************************************************************************
 ***                     Basic Hardware Definition                     ***
 *************************************************************************/

// Uncomment if month is 2 digits (7-seg), as in the original A-Car display.
//#define IS_ACAR_DISPLAY

/*************************************************************************
 ***                          Version Strings                          ***
 *************************************************************************/

// These must not contain any characters other than
// '0'-'9', 'A'-'Z', '(', ')', '.', '_', '-' or space
#define TC_VERSION "V3.2.000"         // 13 chars max
#ifndef IS_ACAR_DISPLAY
#define TC_VERSION_EXTRA "NOV072024"  // 13 chars max
#else   // A-Car
#define TC_VERSION_EXTRA "11072024"   // 12 chars max
#endif

//#define TC_DBG              // debug output on Serial

/*************************************************************************
 ***                     mDNS (Bonjour) support                        ***
 *************************************************************************/

// Supply mDNS service
// Allows accessing the Config Portal via http://hostname.local
// <hostname> is configurable in the Config Portal
// This needs to be commented if WiFiManager provides mDNS
#define TC_MDNS
// Uncomment this if WiFiManager has mDNS enabled
//#define TC_WM_HAS_MDNS 

/*************************************************************************
 ***                 Configuration for peripherals                     ***
 *************************************************************************/

// Uncomment for support of GPS receiver (MT3333-based) connected via i2c 
// (0x10). Can be used as time source and/or to display actual speed on 
// speedometer display. Also needs to be #defined for using CircuitSetup's
// speedo with integrated GPS receiver.
//#define TC_HAVEGPS

// Uncomment for support of speedo-display connected via i2c (0x70).
// See speeddisplay.h for details
//#define TC_HAVESPEEDO
#define SP_NUM_TYPES    12  // Number of speedo display types supported
#define SP_MIN_TYPE     0
#ifdef TC_HAVESPEEDO
// Uncomment to keep speedo showing "00." when neither temp, nor GPS speed, 
// nor fake speed through rotary encoder are to be displayed instead of 
// switching it off when idle.
//#define SP_ALWAYS_ON
// Uncomment to enable the fake-0 on CircuitSetup's speedo; is not usable
// as a full third digit, just displays "0" when speed to be displayed
//#define SP_CS_0ON
// Uncomment to have the Speedo display "--" when there is no GPS fix.
// When commented, it will display 0.
//#define GPS_DISPLAY_DASHES
#endif

// Uncomment for rotary encoder support
// Currently Adafruit 4991, DFRobot Gravity 360 and DuPPA I2CEncoder 2.1 are
// supported.
// The primary rotary encoder is used to manually select a speed to be
// displayed on a speed display, and/or to be sent to wirelessly connected
// props (BTTFN); also, time travels can be triggered by turning the knob up
// to 88. Turning the knob a few notches below 0 switches the speedo off (or
// allows temperature to be shown, if so configured).
// A secondary rotary encoder is used for audio volume.
//#define TC_HAVE_RE

// Uncomment for Remote control support
// "Remote" is a modified Futaba remote control with CS/A10001986 control board
// and the A10001986 "remote" firmware.
//#define TC_HAVE_REMOTE

// Uncomment for support of a temperature/humidity sensor (MCP9808, BMx280, 
// SI7021, SHT4x, TMP117, AHT20, HTU31D, MS8607) connected via i2c. Will be used 
// for room condition mode and to display ambient temperature on speedometer  
// display when idle (GPS speed has higher priority, ie if "Display GPS speed"
// is checked in the Config Portal, temperature will not be shown on speedo).
// See sensors.cpp for supported i2c slave addresses
//#define TC_HAVETEMP

// Uncomment for support of a light sensor (TSL2561/2591, BH1750, VEML7700/6030
// or LTR303/329) connected via i2c. Used for night-mode-switching. VEML7700  
// and GPS cannot be present at the same time since they share the same 
// i2c slave address. VEML6030 needs to be set to 0x48 if GPS is present.
// See sensors.cpp for supported i2c slave addresses.
//#define TC_HAVELIGHT

// Fake Power Switch (TFC drive switch):
// Attach an active-low switch to io13; firmware will start network and
// sync time, but not enable displays until the switch is activated.
// The white led will flash for 0.5 seconds when the unit is ready to be 
// "fake" powered on. De-activating the switch "fake" powers down the device
// (ie the displays are switched off, and no keypad input is accepted)
// GPS speed will be shown on speedo display regardless of this switch.
// DIY instructions to build a TFC Switch: https://tfc.out-a-ti.me
// Uncomment to include support for a Fake Power Switch.
#define FAKE_POWER_ON

// External time travel (input) ("ETT")
// If the pin goes low (by connecting it to GND using a button), a time
// travel is triggered.
// Uncomment to include support for ETT, see below for pin number
// This is also needed to receive commands via MQTT and BTTFN-wide TTs.
#define EXTERNAL_TIMETRAVEL_IN

// External time travel (output) ("ETTO")
// The defined pin is set HIGH on a time travel, and LOW upon re-entry from 
// a time travel. See tc_time.c for a timing diagram.
// Uncomment to include support for ETTO, see below for pin number
// This is also needed if MQTT or BTTFN is used to trigger other props.
#define EXTERNAL_TIMETRAVEL_OUT

// Uncomment for HomeAssistant MQTT protocol support
#define TC_HAVEMQTT

// Uncomment for bttfn discover (multicast) and notification broadcast.
// This is REQUIRED for operating a Futaba remote control, and very
// useful for other props' quicker speed updates. Supported by the 
// following firmwares: 
// Flux >= 1.60, SID >= 1.40, DG >= 1.10, VSR >= 1.10, Remote >= 0.90
#define TC_BTTFN_MC

// Uncomment to include Exhibition mode
// 99mmddyyyyhhMM sets (and enables) EM with fixed present time as given; 
// 999 toggles between EM and normal operation
//#define HAVE_STALE_PRESENT

// Uncomment to allow "persistent time travels" only if an SD card is
// present and option "Save secondary setting to SD" is checked. 
// Saving clock data to ESP32 Flash memory can cause delays of up to 
// a whopping 1200ms, and thereby audio playback and other issues, 
// doing severe harm to user experience (and causes flash wear...)
#define PERSISTENT_SD_ONLY

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// If this is uncommented, support for line-out audio (CB 1.4.5) is included.
// If enabled (350/351 on keypad), Time travel sounds as well as music from 
// the Music Player will be played over line-out, and not the built-in speaker.
//#define TC_HAVELINEOUT

// If this is commented, the TCD uses the Gregorian calendar all the way,
// ie since year 1. If this is uncommented, the Julian calendar is used
// until either Sep 2, 1752 or Oct 4, 1582, depending on JSWITCH_1582.
#define TC_JULIAN_CAL
// If this is uncommented, the switch from Julian to Gregorian calendar is
// after Oct 4, 1582 (after which point most of Europe, plus the Spanish 
// colonies switched); if commented, the date is Sep 2, 1752 (when USA, UK, 
// Canada switched). Note that in both cases days were skipped; in case of
// 1582, Oct 15 followed Oct 4; in case of 1752, Sep 14 followed Sep 2.
//#define JSWITCH_1582

// Uncomment if using a real GTE/TRW keypad control board
//#define GTE_KEYPAD 

// Uncomment for alternate "animation" when entering a destination time
// (Does not affect other situations where animation is shown, like time
// cycling, or when RC mode is active). Mutually exclusive to a-car mode.
//#define BTTF3_ANIM

// Uncomment if AM and PM should be reversed (like in BTTF2/3-version of TCD)
//#define REV_AMPM

// Uncomment to disable the tt animation
//#define TT_NO_ANIM

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
//#define USE_SPIFFS

// Uncomment for PCF2129 RTC chip support
//#define HAVE_PCF2129

/*************************************************************************
 ***                           Customization                           ***
 *************************************************************************/

//#define TWSOUND         // Use A10001986's sound files
//#define TWPRIVATE     // A10001986's private customizations

#ifdef TWPRIVATE
#undef TC_VERSION
#define TC_VERSION "A10001986P"
#elif defined(TWSOUND)
#undef TC_VERSION
#define TC_VERSION "A10001986"
#endif

/*************************************************************************
 ***                             Sanitation                            ***
 *************************************************************************/

#ifndef TC_HAVESPEEDO
#undef SP_ALWAYS_ON
#endif
#ifdef IS_ACAR_DISPLAY
#undef BTTF3_ANIM
#endif

/*************************************************************************
 ***                  esp32-arduino version detection                  ***
 *************************************************************************/

#if defined __has_include && __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#ifdef ESP_ARDUINO_VERSION_MAJOR
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2,0,8)
    #define HAVE_GETNEXTFILENAME
    #endif
#endif
#endif

/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

#define STATUS_LED_PIN      2      // Status LED (on ESP) (unused on TCD CB <V1.4)
#define SECONDS_IN_PIN     15      // SQW Monitor 1Hz from the DS3231
#define ENTER_BUTTON_PIN   16      // enter key
#define WHITE_LED_PIN      17      // white led
#define LEDS_PIN           12      // Red/amber/green LEDs (TCD CB >=V1.3)

// I2S audio pins
#define I2S_BCLK_PIN       26
#define I2S_LRCLK_PIN      25
#define I2S_DIN_PIN        33

// SD Card pins
#define SD_CS_PIN           5
#define SPI_MOSI_PIN       23
#define SPI_MISO_PIN       19
#define SPI_SCK_PIN        18

#define VOLUME_PIN         32      // analog input pin (TCD CB <V1.4.5)
#define VOLUME_PIN_NEW     34      // analog input pin (TCD CB >=V1.4.5)

#define MUTE_LINEOUT_PIN    2      // TCD CB >=1.4.5
#define SWITCH_LINEOUT_PIN 32      // TCD CB >=1.4.5
#define MLO_MIRROR         35      // TCD CB >=1.4.5

#define FAKE_POWER_BUTTON_PIN       13  // Fake "power" switch
#define EXTERNAL_TIMETRAVEL_IN_PIN  27  // Externally triggered TT (input)
#define EXTERNAL_TIMETRAVEL_OUT_PIN 14  // TT trigger output

/*************************************************************************
 ***             Display IDs (Do not change, used as index)            ***
 *************************************************************************/

#define DISP_DEST     0
#define DISP_PRES     1
#define DISP_LAST     2

// Num of characters on display
#ifdef IS_ACAR_DISPLAY
#define DISP_LEN      12
#else
#define DISP_LEN      13
#endif

/*************************************************************************
 ***                         TimeCircuits Epoch                        ***
 *************************************************************************/

#define SECS1970_2022 1640995200ULL
#define SECS1970_2023 1672531200ULL
#define SECS1970_2024 1704067200ULL
#define SECS1970_2025 1735689600ULL
#define SECS1970_2026 1767225600ULL
#define SECS1970_2027 1798761600ULL
#define SECS1970_2028 1830297600ULL
#define SECS1970_2029 1861920000ULL
#define SECS1970_2030 1893456000ULL
#define SECS1970_2031 1924992000ULL
#define SECS1970_2032 1956528000ULL
#define SECS1970_2033 1988150400ULL
#define SECS1970_2034 2019686400ULL
#define SECS1970_2035 2051222400ULL
#define SECS1970_2036 2082758400ULL

// NTP baseline data: Prolong life time of NTP and GPS
// Set this to current year. Stop at 2036.
#define TCEPOCH       2024
// Set to SECS1970_xxxx, xxxx being current year. Stop at 2036.
#define TCEPOCH_SECS  SECS1970_2024

// Epoch for general use; increase yearly, no limit
// Defines the minimum date considered valid
#define TCEPOCH_GEN   2024

#endif
