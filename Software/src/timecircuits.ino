/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 * This code was written without the use of artificial intelligence.
 *
 * -------------------------------------------------------------------
 * License: Modified MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * Links inside the Software pointing to the original source must not
 * be changed or removed.
 * 
 * In addition, the following restrictions apply:
 *
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Build instructions (for Arduino IDE)
 * 
 * - Install the Arduino IDE
 *   https://www.arduino.cc/en/software
 *    
 * - This firmware requires the "ESP32-Arduino" framework. To install this framework, 
 *   in the Arduino IDE, go to "File" > "Preferences" and add the URL   
 *   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   - or (if the URL above does not work) -
 *   https://espressif.github.io/arduino-esp32/package_esp32_index.json
 *   to "Additional Boards Manager URLs". The list is comma-separated.
 *   
 * - Go to "Tools" > "Board" > "Boards Manager", then search for "esp32", and install 
 *   the latest 2.x version by Espressif Systems. Versions >=3.x are not supported.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model. For
 *   CircuitSetup original boards, select "NodeMCU-32S".
 *   
 * - If you want Arduino IDE to upload the firmware via USB (which is only required for
 *   fresh ESP32 boards; if a previous version of the firmware is installed on your
 *   board, you can update through the Config Portal and don't need an USB connection):
 *   
 *   Connect your ESP32 board using a suitable USB cable.
 *   
 *   Note that ESP32 boards come in two flavors that differ in which serial communications 
 *   chip is used: Either SiLabs CP210x or WCH CH340. CircuitSetup uses the CP210x.
 * 
 *   Mac:
 *   * CP210x: Since ca. 10.15.7, MacOS comes with a driver for the CP210x. For earlier 
 *     versions of MacOS, installing a driver is required:
 *     https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *     The port ("Tools -> "Port" in Arduino IDE) is named 
 *     - /dev/cu.usbserial-xxxx when using the Apple driver, 
 *     - /dev/cu.SLAB_USBtoUART when using the SiLabs driver. 
 *     The maximum upload speed ("Tools" -> "Upload Speed" in Arduino IDE) can be used.
 *     Note: The SiLabs driver has a bug that affects TCD Control Boards V1.4 and later.
 *     Firmware uploads will fail ("No data received"). Use the Apple driver for those 
 *     boards.
 *   * CH340: This chip is supported out-of-the-box since Mojave. 
 *     The port ("Tools -> "Port" in Arduino IDE) is named /dev/cu.usbserial-XXXX, and 
 *     the maximum upload speed is 460800.
 * 
 *   Windows:
 *   * CP210x: Current Windows versions may come with a suitable driver. If the chip 
 *     is not recognized (ie no Port is created), a driver needs to be installed:
 *     https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   * CH340: Windows will install a driver when connecting the board; in case it 
 *     doesn't or it fails doing so, please install this driver:
 *     http://www.wch-ic.com/downloads/CH341SER_ZIP.html
 *     Note that the maximum upload speed for the CH340 is apparently 460800.
 *   After driver installation, connect your ESP32, start the Device Manager, expand 
 *   the "Ports (COM & LPT)" list and look for the port with the ESP32 name. Choose 
 *   this port under "Tools" -> "Port" in Arduino IDE.
 *
 * - Install required libraries. In the Arduino IDE, go to "Tools" -> "Manage Libraries" 
 *   and install the following library:
 *   - ArduinoJSON (>= 6.19): https://arduinojson.org/v6/doc/installation/
 *     (Versions 7 and on of this lib are much bigger, so it might happen that the 
 *     binary does not fit the ESP32's flash memory. Use v6 instead.)
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/Time-Circuits-Display/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "timecircuits-A10001986" folder and 
 *   double-click on "timecircuits-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - For USB connected ESP32 boards: 
 *   Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 * - For OTA updates via the Config Portal:
 *   Go to "Sketch" -> "Export compiled Binary" to compile the firmware; a ".bin" file 
 *   is created in the source directory, which can then be uploaded through the Config
 *   Portal.
 *   
 * - Install the sound-pack: 
 *   - Go to Config Portal, click "Update & Upload" and upload the sound-pack (TCDA.bin,
 *     extracted from install/sound-pack-twXX.zip) through the top (!) file selector.
 *     A FAT32 (not ExFAT!) formatted SD card must be present in the slot during this 
 *     operation.
 *   Alternatively:
 *   - Copy TCDA.bin to the top folder of a FAT32 (not ExFAT!) formatted SD card (max 
 *     32GB) and put this card into the slot while the TCD is powered down. 
 *   - Now power-up. The sound-pack will now be installed. When finished, the TCD will 
 *     reboot.
 */

/*  Changelog
 *          
 *  2026/10/17 (A10001986) [4.00]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    This update is especially relevant for TCDs installed in cars and brings two 
 *    important changes, in GPS speed handling and the future firmware update procedure. 
 *    - GPS speed logic change: "Display GPS speed" and "Provide GPS speed to BTTFN
 *      clients" Config Portal options are gone. User can now switch between [Evaluate
 *      GPS speed] and [Do not evaluate GPS Speed] on-the-fly through keypad command 8.
 *      This toggles between (a) displaying GPS speed on speedo as well as sending it to
 *      BTTFN clients, and (b) using a rotary encoder, displaying "0", temperature, or
 *      switching the speedo off, depending on hardware configuration and other option
 *      settings.
 *    - If GPS does not have a fix, the speedo will display "-" permanently (and no
 *      longer switch to "0" after a while). To avoid this, issue keypad command 8 to
 *      switch away from GPS speed.
 *    - The former GPS speed "Update rate" selector is now labeled "GPS speed" and got
 *      a "disabled" option item which allows categorically disable GPS speed (which
 *      should be done in case of a permanently stationary installation to avoid polling
 *      the GPS receiver unnecessarily frequently).
 *    - GPS: Keypad commands 950/951 disable/enable the GPS fix indicator in the
 *      present time's year field. Defaults to on.
 *    - GPS time sync issue fixed, and speed-up booting with GPS time sync.
 *    - GPS: Keypad menu now has an item that shows if a GPS receiver has been detected
 *      during boot.
 *    - Fix for undetected GPS receivers on very long cables.
 *    - Fixes for speedo-less time travel with GPS speed or RotEnc present.
 *    - Rotary encoder: Some user-experience enhancements.
 *    - Re-arrange WiFi Configuration page for a better user experience
 *    - Remap "show weekday of current present time's date" keypad command to more handy
 *      3 (instead of rather unintuitive 39).
 *    - More beginner-friendly default settings. However, using GPS speed is disabled by
 *      default as there are way more Time Circuits than there are DTMs.
 *    - Keypad command 1971159357, when entered twice, resets all settings to factory
 *      defaults.
 *    - New sound-pack (TW10/CS10)
 *    - Future firmware updates may only consist of one single file instead of "main 
 *      firmware" plus "sound-pack". Firmware updates will require an SD card.
 *      Future sound-pack uploads, if any, are done through the top file selector, and
 *      no longer through the bottom one. The bottom one is for custom mp3 file upload
 *      only.
 *  2026/09/18 (A10001986) [3.27]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - New sound-pack (TW09/CS09)
 *    - Add option to play a default sound-on-the-hour (clock tower bell plus spoken
 *      time; requires fully installed sound-pack; only played if not user-overridden by 
 *      "hour[-xx].mp3" sounds on SD card)
 *    - MQTT: Split "Publish time travel and alarm events" option into two options, 
 *      "Time Travel" and "Alarm".can now go different MQTT/BTTFN routes.
 *    - Keypad command "9" sends "refill" command to Dash Gauges ("009" is also still
 *      supported).
 *    - Add support for forwarding "refill" command from FC's and SID's IR remote control
 *      to the Dash Gauges. (FC 1.107+, SID 1.76+)
 *    - Fix minor issues with ID3v1 and ID3v2.4-UTF8 tags.
 *    - Speed up booting with music folders containing many files
 *    - Music Player: The "TCD_DONE.TXT" file is now obsolete. The firmware instead uses
 *      a cache file in the top-most folder of the SD card ("musicXc") which needs to be
 *      deleted when files are added to the respective folder.
 *    - Protect the TCD from uploading a wrong firmware by accident. The filename of
 *      the firmware binary now must contain the word "circuits". The check is case-
 *      insensitive.
 *    - Add option to configure speedo as briefly shown in part 3 with the left-most
 *      digit covered by gaffer tape. Only supported on upcoming CircuitSetup speedo v2 
 *      (featuring a complete third digit).
 *    - Bonjour/mDNS: Send good-bye packet on controlled reboots
 *    - Minor tweaks and fixes
 *  2026/08/19 (A10001986) [3.26]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - New sound-pack (TW08/CS08)
 *    - WC/RC-hybrid display mode: Show both temperature and humidity on same display
 *    - Door sound playback: Fix regression introduced in 3.25 
 *    - More robust error handling in case of malformed config files
 *    - SD: More compatibility fixes
 *    - Fix typos in Config Portal: TT_OUT is switched by 900/901 (not 990/991).
 *    - "Time Cycling": Correct some times of tt
 *    - Internal BTTFN enhancements
 *    - Code optimizations
 *  2026/07/16 (A10001986) [3.25]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - Bugfix: Year in BTTFN-broadcast time stamp was truncated
 *    - Bugfix: Fix SD initialization in ESP32-Arduino to conform to specs.
 *      Sandisk Ultra 32GB cards (and possibly others that previously were not usable) 
 *      are now recognized.
 *    - Internal BTTFN changes
 *  2026/06/25 (A10001986) [3.24]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - MQTT: Add "VOLUME_UP"/"VOLUME_DOWN"/"VOLUME_SET_xxx" commands
 *    - MQTT: Add publishing Music Player status to bttf/tcd/mpstatus
 *    - Music Player: Use new sorting algorithm as the old one caused a crash on a 
 *      large number of already sorted files (due to excessive recursion).
 *  2026/05/21 (A10001986) [3.23]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - New Sound-Pack (TW07/CS07). Marty will tell you the displayed time whenever
 *      pressing ENTER without entering a command.
 *    - Add keypad commands 30-33 to set relative beep volume level
 *    - Remap keypad command 33 (show weekday) to 39
 *    - Remap keypad commands 000/001/002/003 to 20/21/22/23 to match FC's flux mode
 *      (000-003 still supported)
 *    - Refactor logic when toggling world clock & room condition hybrid mode
 *    - Display TCD-AP BSSID on WiFi Settings page in Config Portal
 *    - Internal BTTFN enhancements
 *  2026/04/19 (A10001986) [3.22]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please see boxed note at version 3.21 below     **
 *    **********************************************************************************
 *    - "Persistent" time travels no longer overwrite stored user-programmed dates
 *      for red and yellow displays. User times can be brought back by 998 at any time.
 *    - Keypad menu: Navigation is now only through 2 (up/+), 5 (select), 8 (down/-),
 *      9 (cancel). Pressing Enter is now equal to "select", no longer used to cycle
 *      through items. Holding the Enter key is no longer part of any menu navigation,
 *      apart from initially entering the keypad menu. When digits are expected to be
 *      entered, ENTER must now be pressed to proceed. Entering the expected number of 
 *      digits no longer automatically proceeds to the next field.
 *    - Alarm: Add "user days" option to select any combination of weekdays.
 *      Pressing 1-7 toggles days, ENTER proceeds, 9 cancels.
 *    - World Clock: Add option to keep location name permanently; only honored if
 *      location name is short enough to fit next to time (9 chars; 8 chars for A-Car)
 *    - Extend MQTT message display feature to all three displays, with separate
 *      topics for each display
 *    - New nightmode schedule preset "Night Owl" (aka "Mancave"). Operating times:
 *      Sun-Thu 8pm-1am, Fri/Sat 8pm-4am.
 *    - Forward Remote's new "REFILL" command from the Remote to the Dash Gauges 
 *      (Remote 1.22)
 *    - Refactor alarm/reminder/timer/SotH logic regarding priorities, concurrency,
 *      locking sequences, etc.
 *    - Fix nightmode weekday selection
 *    - Code optimizations and fixes
 *  2026/03/27 (A10001986) [3.21]
 *    **********************************************************************************
 *    ** If updating from below 3.20, please install 3.20 or 3.20.1 first to have     **
 *    ** your settings converted. If 3.20(.1) is skipped, many of your settings       **
 *    ** will be restored to default values. It suffices to install 3.20 or 3.20.1    **
 *    ** and boot once; you can then immediately update to a later version.           **
 *    ** You can skip the sound-pack when installing 3.20(.1) just for conversion.    **
 *    ** https://github.com/realA10001986/Time-Circuits-Display/releases/tag/V3.20.1  **
 *    **********************************************************************************
 *    - New sound-pack (TW06/CS06)
 *    - New "extended" alarm function: Pressing ENTER optionally snoozes, holding 
 *      ENTER stop the alarm. User-provided sound can be looped. Old legacy mode
 *      still supported.
 *    - 117 toggles new "minimal mode": Present time shows weekday instead of year
 *      when displaying actual present time, red and yellow displays are off.
 *    - 110 restores display mode to default (disables RC, WC, Geo, mini modes)
 *    - 91mmddyyyyhhMM/92mmddyyyyhhMM are short-cuts to enter and save user-defined
 *      dates for the Destination Time/Last Time Departed displays, just like using
 *      the keypad menu. Time-cycling is paused for 30 mins.
 *    - Programming the Destination Time/Last Time Departed displays through the 
 *      keypad menu no longer disables time cycling, it only pauses it for 30 mins.
 *    - MQTT: Add 10 user defined topics and messages for publishing through keypad 
 *      commands 600-609.
 *    - WiFi: Allow defining a BSSID (AP MAC address) to connect to a specific AP
 *      if multiple APs with identical SSID are available.
 *    - A-Car version can now swap red and yellow displays to emulate B-Car as seen
 *      in part 3.
 *    - New Easter Egg
 *    - Music player: Fix off-by-one bug in ID3 decoding
 *    - MQTT: Disable if server can't be resolved
 *    - WiFi: Do not power down AP long as a client is connected
 *    - Brush up Config Portal a bit
 *    - Code optimizations and fixes
 *  2026/03/04 (A10001986) [3.20.1]
 *    - Fix saving display times
 *  2026/02/16 (A10001986) [3.20]
 *    - New file format for secondary and IP settings. This version of the firmware 
 *      converts old to new.
 *    - Add geolocation: Display GPS coordinates in Destination and Last Time Dep
 *      displays. 114(DD notation)/115(DMS)/116(DMD) to enable/disable this mode.
 *    - Display mode (Room condition, World Clock, Geolocation) is now persistent
 *      (SD card required). Current mode is saved 10 seconds after activation (or 
 *      upon fake-power-down or a controlled reboot such as when changing settings 
 *      in Config Portal)
 *    - "Shuffle" setting changes are now saved (SD card required), "Shuffle" option
 *      removed from Config Portal.
 *    - Add option to *not* use GPS time; no one knows what happens after 2034/2038
 *      when the GPS week counter rolls over, so although the firmware tries to
 *      correct wrong times, given I cannot test this in any way, it might fail
 *      and sync to wrong date/time.
 *    - Display MAC address (STA) on WiFi Configuration Page
 *    - Fiddle with timing for smoother Remote speed updates
 *    - World Clock mode: Display location name with time if both fit
 *    - Config files are now only written if actually changed which prolongs
 *      Flash life-span.
 *    - Add NO_MONTH_ANIM compile-time option to skip the date-entry animation.
 *      Might be desirable when using A-car displays: Given the "month" is just
 *      an ordinary 2-digit number (and no back-lit gel) the real thing probably
 *      switched on the entire line at once.
 *    - Code optimizations and fixes
 *  [...]
 *  2022/08/03-06 (A10001986)
 *    - Jun 2022 code base imported from
 *      https://github.com/CircuitSetup/Time-Circuits-Display/tree/587ec1c56fefe8f2a0e08e9a014dd10b810c217b
 */

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>

#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_settings.h"
#include "tc_main.h"
#include "tc_wifi.h"

void setup()
{
    Serial.begin(115200);
    Serial.println();

    // I2C init
    // Make sure our i2c buf is 128 bytes
    Wire.setBufferSize(128);
    // PCF8574 only supports 100kHz, can't go to 400 here.
    // Also, speedo cable is usually quite long, play it safe.
    Wire.begin(-1, -1, 100000);

    main_boot();
    settings_setup();
    wifi_setup();
    audio_setup();
    keypad_setup();
    main_setup();
}

#ifdef TC_PROFILER
#include "zzProfiler.h"
#else
void loop()
{
    keypad_loop();
    audio_loop();
    scanKeypad();
    ntp_loop();
    audio_loop();
    bttfn_loop(BNLP_SK_MC|BNLP_SK_NOTDATA|BNLP_SK_EXPIRE);
    audio_loop();
    main_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    bttfn_loop();
    bttfn_loop_ex();
    audio_loop();
}
#endif

#if defined(TC_DBG_TIME) || defined(TC_DBG_NET) || defined(TC_DBG_GPS) || defined(TC_DBG_GEN)
#warning "Debug output is enabled. Binary not suitable for release."
#endif
