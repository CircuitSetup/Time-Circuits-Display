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

/*
 * 
 * Needs ESP32 Arduino framework: https://github.com/espressif/arduino-esp32
 *
 * Library dependencies:
 * - OneButton: https://github.com/mathertel/OneButton
 *   (Tested with 2.0.4)
 * - ESP8266Audio: https://github.com/earlephilhower/ESP8266Audio
 *   (1.9.7 and later for esp-arduino 2.x; 1.9.5 for 1.x)
 * - RTClib (Adafruit): https://github.com/adafruit/RTClib
 *   (Tested with 2.1.1)
 * - WifiManager (tablatronix, tzapu; v0.16 and later) https://github.com/tzapu/WiFiManager
 *   (Tested with 2.1.12beta)
 * - Keypad ("by Community; Mark Stanley, Alexander Brevig): https://github.com/Chris--A/Keypad
 *   (Tested with 3.1.1)
 * 
 * 
 * Detailed installation and compilation instructions are here:
 * https://github.com/CircuitSetup/Time-Circuits-Display/wiki/Programming-the-ESP32-Module
 */

/* Changelog 
 *  
 *  2022/08/26 (A10001986)
 *    - Attempt to beautify the Config Portal by using checkboxes instead of
 *      text input for boolean options
 *  2022/08/25 (A10001986)
 *    - Add default sound file installer. This installer is for initial installation  
 *      or upgrade of the software. Put the contents of the data folder on a
 *      FAT formatted SD card, put this card in the slot, reboot the clock,
 *      and either go to the "INSTALL AUDIO FILES" menu item in the keypad menu,  
 *      or to the "Setup" page on the Config Portal (see bottom). 
 *      Note that this only installs the original default files. It is not meant 
 *      for custom audio files substituting the default files. Custom audio files 
 *      reside on the SD card and will be played back from there. 
 *  2022/08/24 (A10001986)
 *    - Intro beefed up with sound
 *    - Do not interrupt time travel by key presses
 *    - Clean up code for default settings
 *    - Prepare for LittleFS; switch not done yet. I prefer SPIFFS as long as it's
 *      around, and LittleFS doesn't support wear leveling.
 *    - AutoTimes sync'd with movies
 *  2022/08/23 (A10001986)
 *    - Allow a static IP (plus gateway, subnet mask, dns) to be configured.
 *      All four IP address must be valid. IP settings can be reset to DHCP
 *      by holding down ENTER during power-up until the white LED goes on.
 *    - F-ified most constant texts (pointless on ESP32, but standard)
 *  2022/08/22 (A10001986)
 *    - New long time travel sequence (only for keypad-timetravel, not for 
 *      externally triggered timetravel)
 *    - Hourly sound now respects the "RTC vs presentTime" setting for the alarm
 *    - Fix bug introduced in last update (crash when setting alarm)
 *    - Audio: Less logging; fix pot resolution for esp32 2.x; reduce "noise
 *      reduction" to 4 values to make knob react faster
 *    - Network info now functional in AP mode
 *    - Proper check for day validity in clockdisplay
 *  2022/08/21 (A10001986)
 *    - Added software volume: Volume can now be set by the volume knob, or by
 *      setting a value in the new keymap Volume menu.
 *    - Value check for settings entered on WiFi setup page
 *  2022/08/20 (A10001986)
 *    - Added a little intro display upon power on; not played at "fake" power on.
 *    - Added menu item to show software version
 *    - Fixed copy/paste error in WiFi menu display; add remaining WiFi stati.
 *    - Fixed compilation for A-Car display
 *    - Displays off during boot 
 *  2022/08/19 (A10001986)
 *    - Network keypad menu: Add WiFi status information
 *    - audio: disable mixer, might cause static after stopping sound playback
 *    - audio cleanup
 *    - clean up sound/animation delay definitions
 *    - audio: vol knob delivers inconsistent values, do some "noise reduction"
 *    - clean up clockdisplay, add generic text routine, scrap unused stuff
 *  2022/08/18 (A10001986)
 *    - Destination time/date can now be entered in mmddyyyy, mmddyyyyhhmm or hhmm
 *      format.
 *    - Sound file "hour.mp3" is played hourly on the hour, if the file exists on 
 *      the SD card; disabled in night mode
 *    - Holding "3" or "6" plays sound files "key3.mp3"/"key6.mp3" if these files
 *      exist on the SD card
 *    - Since audio mixing is a no-go for the time being, remove all unneccessary 
 *      code dealing with this.
 *    - Volume knob is now polled during play back, allowing changes while sound
 *      is playing
 *    - Fix auto time rotation pause logic at menu return
 *    - [Fix crash when saving settings before WiFi was connected (John)]
 *  2022/08/17 (A10001986)
 *    - Silence compiler warnings
 *    - Fix missing return value in loadAlarm
 *  2022/08/16 (A10001986)
 *    - Show "BATT" during booting if RTC battery is depleted and needs to be 
 *      changed
 *    - Pause autoInterval-cycling when user entered a valid destination time
 *      and/or initiated a time travel
 *  2022/08/15 (A10001986)
 *    - Time logic re-written. RTC now always keeps real actual present
 *      time, all fake times are calculated off the RTC time. 
 *      This makes the device independent of NTP; the RTC can be manually 
 *      set through the keypad menu ("RTC" is now displayed to remind the 
 *      user that he is actually setting the *real* time clock).
 *    - Alarm base can now be selected between RTC (ie actual present
 *      time, what is stored in the RTC), or "present time" (ie fake
 *      present time).
 *    - Fix fake power off if time rotation interval is non-zero
 *    - Correct some inconsistency in my assumptions on A-car display
 *      handling
 *  2022/08/13 (A10001986)
 *    - Changed "fake power" logic : This is no longer a "button" to  
 *      only power on, but a switch. The unit can now be "fake" powered 
 *      and "fake" powered off. 
 *    - External time travel trigger: Connect active-low button to
 *      io14 (see tc_global.h). Upon activation (press for 200ms), a time
 *       travel is triggered. Note that the device only simulates the 
 *      re-entry part of a time travel so the trigger should be timed 
 *      accordingly.
 *    - Fix millis() roll-over errors
 *    - All new sounds. The volume of the various sound effects has been
 *      normalized, and the sound quality has been digitally enhanced.
 *    - Make keypad more responsive
 *    - Fix garbled keypad sounds in menu
 *    - Fix timeout logic errors in menu
 *    - Make RTC usable for eternity (by means of yearOffs)
 *  2022/08/12 (A10001986)
 *    - A-Car display support enhanced (untested)
 *    - Added SD support. Audio files will be played from SD, if
 *      an SD is found. Files need to reside in the root folder of
 *      a FAT-formatted SD and be named 
 *      - "startup.mp3": The startup sound
 *      - "enter.mp3": The sound played when a date was entered
 *      - "baddate.mp3": If a bad date was entered
 *      - "timetravel.mp3": A time travel was triggered
 *      - "alarm.mp3": The alarm sound
 *      - "nmon.mp3": Night mode is enabled
 *      - "nmoff.mp3": Night mode is disabled*      
 *      - "alarmon.mp3": The alarm was enabled
 *      - "alarmoff.mp3": The alarm was disabled      
 *      Mp3 files with 128kpbs or below recommended. 
 *  2022/08/11 (A10001986)
 *    - Integrate a modified Keypad_I2C into the project in order 
 *      to fix the "ghost" key presses issue by reducing i2c traffic 
 *      and validating the port status data by reading the value
 *      twice.
 *  2022/08/10 (A10001986)
 *    - Added "fake power on" facility. Device will boot, setup 
 *      WiFi, sync time with NTP, but not start displays until
 *      an active-low button is pressed (connected to io13, see 
 *      tc_global.h)
 *  2022/08/10 (A10001986)
 *    - Nightmode now also reduced volume of sound (except alarm)
 *    - Fix autoInterval array size
 *    - Minor cleanups
 *  2022/08/09 (A10001986)
 *    - Fix "animation" (ie. month displayed a tad delayed)
 *    - Added night mode; enabled by holding "4", disabled by holding "5"
 *    - Fix for flakey i2c connection to RTC (check data and retry)
 *      Sometimes the RTC chip loses sync and no longer responds, this
 *      happens rarely and is still under investigation.
 *    - Quick alarm enable/disable: Hold "1" to enable, "2" to disable
 *    - If alarm is enabled, the dot in present time's minute field is lit
 *    - Selectable "persistent" time travel mode (WiFi Setup page):
 *        If enabled, time travel is persistent, which means all times
 *        changed during a time travel are saved to EEPROM, overwriting 
 *        user programmed times. In persistent mode, the fake present time  
 *        also continues to run during power loss, and is NOT reset to 
 *        actual present time upon restart.
 *        If disabled, user programmed times are never overwritten, and
 *        time travels are not persistent. Present time will be reset
 *        to actual present time after power loss.
 *    - Alarm data is now saved to file system, no longer to EEPROM
 *      (reduces wear on flash memory)
 *  2022/08/03-06 (A10001986)
 *    - Alarm function added
 *    - 24-hour mode added for non-Americans (though not authentic at all)
 *    - Keypad menu item to show IP address added
 *    - Configurable WiFi connection timeouts and retries
 *    - Keypad menu re-done
 *    - "Present time" is a clock (not stale) after time travel
 *    - Return from Time Travel (hold "9" for 2 seconds)
 *    - Support for time zones and automatic DST
 *    - More stable sound playback
 *    - various bug fixes
 */

#include <Arduino.h>
#include "clockdisplay.h"
#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_time.h"
#include "tc_wifi.h"
#include "tc_settings.h"

void setup() 
{
    Serial.begin(115200);    

    Wire.begin();
    // scan();
    Serial.println();
    
    time_boot();
    settings_setup();
    wifi_setup();
    audio_setup();

    menu_setup();
    keypad_setup();
    time_setup();
}


void loop() 
{
    keypad_loop();
    get_key();
    time_loop();
    wifi_loop();
    audio_loop();
}

// For testing I2C connections and addresses
/*
void scan() 
{
    Serial.println(" Scanning I2C Addresses");
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < 128; i++) {
        Wire.beginTransmission(i);
        uint8_t ec = Wire.endTransmission(true);
        if (ec == 0) {
            if (i < 16) Serial.print('0');
            Serial.print(i, HEX);
            cnt++;
        } else
            Serial.print("..");
        Serial.print(' ');
        if ((i & 0x0f) == 0x0f) Serial.println();
    }
    Serial.print("Scan Completed, ");
    Serial.print(cnt);
    Serial.println(" I2C Devices found.");
}

bool i2cReady(uint8_t adr) 
{
    uint32_t timeout = millis();
    bool ready = false;
    while ((millis() - timeout < 100) && (!ready)) {
        Wire.beginTransmission(adr);
        ready = (Wire.endTransmission() == 0);
    }
    return ready;
}
*/