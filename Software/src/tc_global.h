/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
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

// Uncomment if using A-Car displays (numeric month)
// See TC_NO_MONTH_ANIM for more "A-carness".
//#define IS_ACAR_DISPLAY

// Uncomment if using a GTE keypad control board
//#define GTE_KEYPAD

/*************************************************************************
 ***                          Version Strings                          ***
 *************************************************************************/

// These must not contain any characters other than
// '0'-'9', 'A'-'Z', '(', ')', '.', '_', '-' or space
#define TC_VERSION_REV   "V3.21"      // 7 chars max. Do NOT change format.
#ifndef IS_ACAR_DISPLAY
#define TC_VERSION_EXTRA "MAR272026"  // 13 chars max
#else   // A-Car
#define TC_VERSION_EXTRA "03272026"   // 12 chars max
#endif

/*************************************************************************
 ***                 Configuration for peripherals                     ***
 *************************************************************************/

// Uncomment for support of GPS receiver (MT3333-based) connected via i2c 
// (0x10). Can be used as time source and/or to display actual speed on 
// speedometer display and/or to display geolocation. Needs to be #defined
// for using CircuitSetup's speedo with integrated GPS receiver.
//#define TC_HAVEGPS

// Uncomment for rotary encoder support
// Currently Adafruit 4991/5880, DFRobot Gravity 360 and DuPPA I2CEncoder 2.1
// are supported.
// The primary rotary encoder is used to manually select a speed to be
// displayed on a speed display, and/or to be sent to wirelessly connected
// props (BTTFN); also, time travels can be triggered by turning the knob up
// to 88. Turning the knob a few notches below 0 switches the speedo off (or
// allows temperature to be shown, if so configured).
// A secondary rotary encoder is used for audio volume.
//#define TC_HAVE_RE

// Uncomment for Remote control support
// "Remote" is a modified Futaba remote control. See https://remote.out-a-ti.me
//#define TC_HAVE_REMOTE

// Uncomment to trigger a time travel when reaching a (real) GPS speed of 88. 
// Use at own risk.
//#define NOT_MY_RESPONSIBILITY

// Uncomment for support of a temperature/humidity sensor (MCP9808, BMx280, 
// SI7021, SHT4x, TMP117, AHT20, HTU31D, MS8607, HDC302x) connected via i2c. 
// Will be used for room condition mode and to display ambient temperature on 
// speedometer display when idle (GPS speed has higher priority, ie if "Display 
// GPS speed" is checked in the Config Portal, temperature will not be shown on 
// speedo). See sensors.cpp for supported i2c slave addresses
//#define TC_HAVETEMP

// Uncomment for support of a light sensor (TSL2561/2591, BH1750, VEML7700/6030
// or LTR303/329) connected via i2c. Used for night-mode-switching. VEML7700  
// and GPS cannot be present at the same time since they share the same 
// i2c slave address. VEML6030 needs to be set to 0x48 if GPS is present.
// See sensors.cpp for supported i2c slave addresses.
//#define TC_HAVELIGHT

// (Un)comment for RTC chip selection. At least one MUST be defined.
#define HAVE_DS3231
//#define HAVE_PCF2129

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// Uncomment for HomeAssistant MQTT protocol support
#define TC_HAVEMQTT

// Uncomment to allow "persistent time travels" only if an SD card is
// present and option "Save secondary setting to SD" is checked. 
// Saving clock data to ESP32 Flash memory can cause delays of up to 
// a whopping 1200ms, and thereby audio playback and other issues, 
// doing severe harm to user experience (and causes flash wear...)
#define PERSISTENT_SD_ONLY

// If this is commented, the TCD uses the Gregorian calendar all the way,
// ie since year 1. If this is uncommented, the Julian calendar is used
// until either Sep 2, 1752 or Oct 4, 1582, depending on JSWITCH_1582.
#define TC_JULIAN_CAL
// If this is uncommented, the switch from Julian to Gregorian calendar is
// after Oct 4, 1582 (at which point most of Europe, plus the Spanish 
// colonies switched); if commented, the date is Sep 2, 1752 (when USA, UK, 
// Canada switched). Note that in both cases days were skipped; in case of
// 1582, Oct 15 followed Oct 4; in case of 1752, Sep 14 followed Sep 2.
//#define JSWITCH_1582

// Uncomment to skip the date-entry animation (month showing up delayed).
// Might be desirable when using A-car displays: Given the "month" is
// just an ordinary 2-digit number (and no back-lit gel) the real thing
// probably switched on the entire line at once.
#ifdef IS_ACAR_DISPLAY
#define TC_NO_MONTH_ANIM
#endif

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino 2.x)
//#define USE_SPIFFS

/*************************************************************************
 ***                           Customization                           ***
 *************************************************************************/

//#define V_A10001986     // Define A10001986 edition (sound-pack)

//#define TWPRIVATE       // A10001986's private customizations

#ifdef TWPRIVATE
#define SERVOSPEEDO
//#define TC_PROFILER
#endif

/*************************************************************************
 ***                               Debug                               ***
 *************************************************************************/

// Debug output selection
//#define TCD_DBG_NONE

#if !defined(TCD_DBG_NONE)
//#define TC_DBG_BOOT           // Boot strap & settings
//#define TC_DBG_WIFI           // WiFi-related
//#define TC_DBG_MQTT           // MQTT-related
//#define TC_DBG_AUDIO          // Audio-related
//#define TC_DBG_TIME           // Time handling
//#define TC_DBG_NET            // Prop network
//#define TC_DBG_TT             // Time travel
//#define TC_DBG_GPS            // GPS-related
//#define TC_DBG_GEN            // Generic
#endif

/*************************************************************************
 ***                             Sanitation                            ***
 *************************************************************************/

#ifdef IS_ACAR_DISPLAY
#define V_ACAR "A"
#else
#define V_ACAR ""
#endif 
#ifdef GTE_KEYPAD
#define V_GTE "G"
#else
#define V_GTE ""
#endif
#ifdef SERVOSPEEDO
#define V_SS "S"
#else
#define V_SS ""
#endif
#ifdef TWPRIVATE
#define TC_VERSION TC_VERSION_REV " P" V_ACAR V_GTE V_SS
#elif defined(V_A10001986)
#define TC_VERSION TC_VERSION_REV " A" V_ACAR V_GTE V_SS
#else
#define TC_VERSION TC_VERSION_REV " C" V_ACAR V_GTE V_SS
#endif

#if !defined(HAVE_PCF2129) && !defined(HAVE_DS3231)
#define HAVE_DS3231
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
#define TCEPOCH       2026
// Set to SECS1970_xxxx, xxxx being current year. Stop at 2036.
#define TCEPOCH_SECS  SECS1970_2026

// Epoch for general use; increase yearly, no limit
// Defines the minimum date considered valid
#define TCEPOCH_GEN   2026

#endif
