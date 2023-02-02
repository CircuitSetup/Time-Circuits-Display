/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * Global definitions
 */

#ifndef _TC_GLOBAL_H
#define _TC_GLOBAL_H

// Version strings.
// These must not contain any characters other than
// '0'-'9', 'A'-'Z', '(', ')', '.', '_', '-' or space
#ifndef IS_ACAR_DISPLAY
#define TC_VERSION "V2.6.0"           // 13 chars max
#define TC_VERSION_EXTRA "JAN282023"  // 13 chars max
#else   // A-Car
#define TC_VERSION "V2.6.0_A-CAR"     // 12 chars max
#define TC_VERSION_EXTRA "01282023"   // 12 chars max
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

// Uncomment for support of gps receiver (MT3333-based) connected via i2c 
// (0x10). Can be used as time source and/or to display actual speed on 
// speedometer display
//#define TC_HAVEGPS

// Uncomment for support of speedo-display connected via i2c (0x70).
// See speeddisplay.h for details
//#define TC_HAVESPEEDO
#define SP_NUM_TYPES    12  // Number of speedo display types supported
#define SP_MIN_TYPE     1   // Change to 0 when CircuitSetup speedo prop exists

// Uncomment for support of a temperature/humidity sensor (MCP9808, BMx280, 
// SI7021, SHT40, TMP117, AHT20, HTU31D) connected via i2c. Will be used for 
// room condition mode and to display ambient temperature on speedometer  
// display when idle (GPS speed has higher priority, ie if GPS speed is 
// enabled in the Config Portal, temperature will not be shown on speedo).
// See sensors.cpp for supported i2c slave addresses
//#define TC_HAVETEMP

// Uncomment for support of a light sensor (TLS2561, BH1750, VEML7700/6030
// or LTR303/329) connected via i2c. Used for night-mode-switching. VEML7700  
// and GPS cannot be present at the same time since they share the same 
// i2c slave address. VEML6030 needs to be set to 0x48 if GPS is present.
// See sensors.cpp for supported i2c slave addresses
//#define TC_HAVELIGHT

// Fake Power Switch:
// Attach an active-low switch to io13; firmware will start network and
// sync time, but not enable displays until the switch is activated.
// The white led will flash for 0.5 seconds when the unit is ready to be 
// "fake" powered on. De-activating the switch "fake" powers down the device
// (ie the displays are switched off, and no keypad input is accepted)
// GPS speed will be shown on speedo display regardless of this switch.
// Uncomment to include support for a Fake Power Switch
#define FAKE_POWER_ON

// External time travel (input) ("ett")
// If the pin goes low (by connecting it to GND using a button), a time
// travel is triggered.
// Uncomment to include support for ett, see below for pin number
#define EXTERNAL_TIMETRAVEL_IN

// External time travel (output) ("etto")
// The defined pin is set HIGH on a time travel, and LOW upon re-entry from 
// a time travel. See tc_time.c for a timing diagram.
// Uncomment to include support for etto, see below for pin number
#define EXTERNAL_TIMETRAVEL_OUT

// --- end of config options

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// Uncomment if month is 2 digits (7-seg), as in the original A-Car display.
//#define IS_ACAR_DISPLAY

// Uncomment if using real GTE/TRW keypad control board
//#define GTE_KEYPAD 

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
// Since I am on esp32-arduino 1.x, I use SPIFFS.
//#define USE_SPIFFS

// Custom stuff -----
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
 ***                             GPIO pins                             ***
 *************************************************************************/

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

#define FAKE_POWER_BUTTON_PIN       13  // Fake "power" switch
#define EXTERNAL_TIMETRAVEL_IN_PIN  27  // Externally triggered TT (input)
#define EXTERNAL_TIMETRAVEL_OUT_PIN 14  // TT trigger output


/*************************************************************************
 ***                             EEPROM map                            ***
 *************************************************************************/

// EEPROM usage is deprecated/phased out.
 
#define SWVOL_PREF        0x00    // volume save location       (2 bytes)
#define DEST_TIME_PREF    0x08    // destination time prefs     (10 bytes)
#define PRES_TIME_PREF    0x12    // present time prefs         (10 bytes)
#define DEPT_TIME_PREF    0x1c    // departure time prefs       (10 bytes)
#define PRES_LY_PREF      0x2A    // present time's "lastYear"  (4 bytes)

/*************************************************************************
 ***             Display IDs (Do not change, used as index)            ***
 *************************************************************************/

#define DISP_DEST     0
#define DISP_PRES     1
#define DISP_LAST     2

/*************************************************************************
 ***                         TimeCircuits Epoch                        ***
 *************************************************************************/

#define SECS1900_1970 2208988800ULL

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

// NTP baseline data: Prolong life time of NTP
// Set this to current year. Stop and leave at 2036.
#define TCEPOCH       2023
// Set to SECS1970_xxxx, xxxx being current year. Stop at 2036.
#define TCEPOCH_SECS  SECS1970_2023

// Epoch for general use; increase yearly, no limit
// Defines the minimum date considered valid
#define TCEPOCH_GEN   2023


#endif
