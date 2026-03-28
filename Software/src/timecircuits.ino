/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * -------------------------------------------------------------------
 * License: MIT NON-AI
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
 *   - Go to Config Portal, click "Update" and upload the sound-pack (TCDA.bin, extracted
 *     from install/sound-pack-xxxx.zip) through the bottom file selector.
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
 *    - Add TC_NO_MONTH_ANIM compile-time option to skip the date-entry animation.
 *      Might be desirable when using A-car displays: Given the "month" is just
 *      an ordinary 2-digit number (and no back-lit gel) the real thing probably
 *      switched on the entire line at once.
 *    - Code optimizations and fixes.
 *  2026/01/11 (A10001986) [3.11]
 *    - New sound pack (TW05/CS05)
 *    - Keypad menu: Add navigation using keypad keys 2 (up), 5 (select), 8 (down), 
 *      9 (cancel). Old "ENTER" method still supported.
 *    - Add MQTT commands "PLAY_DOOR_OPEN", "PLAY_DOOR_CLOSE" (with optional
 *      appendices "_L"/"_R" for playing it on one stereo channel only).
 *    - Add way for Dash Gauges to play door open/close sounds through TCD
 *      (Gauges >= 1.29)
 *    - Fix float settings (temp offset, accel figure factor; broken since 3.6)
 *    - Add new notification, which eliminates the need for clients to poll for
 *      data and helps to reduce network traffic.
 *      (For full effect, other props need updates, too: FC >= 1.91, SID >= 1.63, 
 *      Gauges >= 1.30, VSR >= 1.25, Remote >= 1.16)
 *    - Eliminate DNS lookups in loop() (which were a possible reason for sound-
 *      stutter sometimes)
 *    - NTP/GPS time sync logic enhancements
 *    - Various code optimizations (audio, GPS, network, etc)
 *  2025/11/29 (A10001986) [3.10]
 *    - Play user-provided "ttcancel.mp3" if TT is cancelled through brake on
 *      Remote
 *    - Make TCD<->Remote communication more robust
 *    * Requires Remote 1.14 for proper operation in combination with Remote
 *  2025/11/28 (A10001986)
 *    - P0: Fix speed-jumps on Remote when hitting brake (Remote and TCD firmware)
 *  2025/11/27 (A10001986)
 *    - Speedo-less operation with Remote: Set TCD speed to 0 as long as brake 
 *      is on. This prevents a time travel at 88 on the Remote with brake on.
 *  2025/11/25 (A10001986)
 *    - Modify speedo-less time travel sequence if bttf clients are present or
 *      mqtt "publish" (regardless of enhanced TIMETRAVEL message) is enabled,
 *      and the tt is triggered by '0', BTTFN or MQTT (NOT: button): 
 *      Now the sequence takes 5 seconds (as opposed to 1.4 seconds in prior 
 *      versions) to give the props enough time for a proper acceleration sequence, 
 *      and in case there is a ttaccel sound, it is played. 
 *      As before, if the time travel is triggered by external button, it starts
 *      immediately and there is no acceleration-time whatsoever.
 *  2025/11/24 (A10001986)
 *    - If a Futaba Remote control is present, it requires proper acceleration/
 *      deceleration and, in essence, acts like a speedo display; therefore, do
 *      TCD-triggered TT sequence as if speedo is present even if there is none.
 *      If the Remote is fake-off, only do that if it wants to display speed 
 *      while off.
 *  2025/11/23 (A10001986)
 *    - Add "Wireless (BTTFN)" speedo type. Allows using a simple BTTFN-listener 
 *      as speedo.
 *    - Initialize everything speedo-related independent of speedo presence; if
 *      Remote is connected (later), it will require proper accel/deceleration 
 *      sequence. See also changes of 11/24.
 *  2025/11/22 (A10001986)
 *    - TCD-controlled TT while Remote is speedMaster: No TT while brake is on.
 *      If brake is hit while accelerating, TT(P0) is cancelled.
 *  2025/11/21 (A10001986) [3.9]
 *    - WM: Minor HTML tweaks; make page width dynamic for better display on
 *      handheld devices
 *  2025/11/19 (A10001986)
 *    - Add support for MQTT v5.0 (tested with mosquitto only). Has no
 *      advantages over 3.1.1 (but more overhead), only there to use brokers
 *      that lack support for 3.1.1.
 *    - Add connection state info on HA/MQTT Settings page
 *  2025/11/17 (A10001986)
 *    - Clear keypad input buffer when/after holding a key
 *  2025/11/16 (A10001986)
 *    - Delete Remote/KPRemote upon forbidding remote (KP) control 
 *    - WM: Require HTTP_POST for params save pages. Also, check if request 
 *      has parameter, do not overwrite current value with null (protects from 
 *      overwriting settings by errorneous page reloads)
 *  2025/11/15 (A10001986)
 *    - Offer three purposes for TT-OUT (IO14): Time travel (as before), alarm,
 *      and control through keypad command; selectable in Config Portal.
 *  2025/11/13 (A10001986)
 *    - Remove various compilation conditionals (FAKE_POWER_ON, TC_HAVELINEOUT,
 *      EXTERNAL_TIMETRAVEL_IN, EXTERNAL_TIMETRAVEL_OUT, HAVE_STALE_PRESENT,
 *      TC_HAVESPEEDO, TC_BTTFN_MC)
 *  2025/11/11 (A10001986)
 *    - Add "stalled P0" feature for speed notification (used by Remote)
 *    - MQTT: Make "enhanced TT" default to on, it avoids the "5s lead problem".
 *  2025/11/10 (A10001986)
 *    - Add (inter-prop) MQTT-"TIMETRAVEL" command with lead time and time tunnel
 *      duration attached, eg. "TIMETRAVEL_4950_6600". Only on bttf/tcd/pub.
 *    - Various fixes to inter-prop MQTT communication
 *    - Abort a network tt ahead of actions that result in a reboot
 *  2025/11/09 (A10001986)
 *    - Add BTTFN_NOT_BUSY notification and status flag to inform BTTFN clients 
 *      that the TCD is busy (eg keypad menu) and not ready for time travel. This
 *      is also used to transmit Remote(KP)Allowed stati.
 *  2025/11/08 (A10001986)
 *    - Move HA/MQTT settings to separate config portal page
 *    - Allocate MQTT buffer only if MQTT is actually used
 *    - Re-order time_loop(): Handle timers in loop iteration following second
 *      change
 *    - Minor code cleanups
 *    - Put beep in flash now that there is space
 *  2025/11/07 (A10001986)
 *    - MP3/File-Renamer: Ignore non-mp3 files; display number of files-to-do while 
 *      renaming
 *    - Decode ID3 tags and free memory immediately on play-back start
 *    - Remove hack to skip web handling in on mp3-playback start, remove stopping
 *      sound in AP mode on CP access.
 *  2025/11/06 (A10001986)
 *    - Block newly injected MQTT command while previous one is still being
 *      worked on.
 *  2025/11/05 (A10001986)
 *    - Add MQTT command "INJECT_"
 *  2025/11/03 (A10001986)
 *    - Add MQTT commands PLAYKEY_x and STOPKEY
 *    - Add commands 501-509 to play keyX (X=1-9)
 *  2025/11/02 (A10001986) [3.8]
 *    - Add option to disable time-cycling animation
 *    - WM: Generate HTML for checkboxes on-the-fly
 *  2025/11/01 (A10001986)
 *    - Add "POWER_CONTROL_xx" and "POWER_xx" MQTT commands to control Fake-Power 
 *      through HA. POWER_CONTROL_xx enables/disables power control through HA, 
 *      and overrule a TFC switch if enabled. POWER_xx enable/disable fake power 
 *      when HA has control. Command code 996 disables HA power control.
 *      Fake-Power through Futaba remote has priority over HA/MQTT.
 *    - Fix logic error in connection with "Remote fake power controls TCD fake 
 *      power". Remote firmware 1.12+ will not control fake-power with 3.7.
 *  2025/10/31 (A10001986) [3.7]
 *    - New sound-pack: Includes sounds for remote on/off, and new night mode
 *      on/off sounds. (TW04/CS04)
 *    - [Broken in 3.7] Add "Remote fake power controls TCD fake power" feature. 
 *      While Remote is Master, TFC switch changes are tracked but ignored. When 
 *      Remote releases fake power control, TFC switch state becomes immediately
 *       effective. Configuration of this feature is done solely on the Remote.
 *      Requires firmware >= 1.12 on Remote. 
 *    - Save beep mode when changed on-the-fly so it's restored on power-up.
 *    - Fix deleting a bad .bin file after upload
 *  2025/10/26 (A10001986) [3.6.1]
 *    - BTTFN: Fix hostname length issues; code optimizations; minor fix for mc 
 *      notifications. Recommend to update all props' firmwares for similar
 *      fixes.
 *  2025/10/24 (A10001986)
 *    - Restart WiFi power save timers after last BTTFN client has been expired
 *    - WM: Fix AP shutdown; handle mDNS
 *  2025/10/21 (A10001986)
 *    HAPPY BTTF DAY!
 *    - Send "REFILL" (009) to DG only, and via BTTFN, not MQTT.
 *  2025/10/17 (A10001986)
 *    - Wipe flash FS if alien VER found; in case no VER is present, check
 *      available space for audio files, and wipe if not enough.
 *  2025/10/16 (A10001986)
 *    - WM: More event-based waiting instead of delays
 *  2025/10/15 (A10001986) [3.6]
 *    - Speedo: Extend option of displaying leading-0 into "speedo display like in
 *      part 3", where the speedo displays 2 digits all the way, but without the dot.
 *    - Speedo: Add option to enable post-point zero on CircuitSetup speedo; it only
 *      ever displays 0. Ignored if "speedo display like in part 3" is enabled.
 *    - Some more WM changes. Number of scanned networks listed is now restricted in 
 *      order not to run out of memory.
 *    - Internal: new json tags to make json file smaller
 *  2025/10/14 (A10001986) [3.5.4]
 *    - WM: Do not garble UTF8 SSID; skip SSIDs with non-printable characters
 *    - Fix regression in CP ("show password")
 *  2025/10/13 (A10001986) [3.5.3]
 *    - Config Portal: Minor restyling (message boxes)
 *  2025/10/11 (A10001986) [3.5.2]
 *    - More WM changes: Simplify "Forget" using a checkbox; redo signal quality
 *      assessment; remove over-engineered WM debug stuff.
 *    - Slightly increase transmit power in AP mode on CB 1.4+.
 *  2025/10/08-10 (A10001986) [3.5.1]
 *    - Increase max BTTFN clients from 5 to 6
 *    - WM: Set "world safe" country info, limiting choices to 11 channels
 *    - WM: Add "show all", add channel info (when all are shown) and proposed
 *      AP WiFi channel on WiFi Configuration page.
 *    - WM: Use events when connecting, instead of delays
 *  2025/10/07 (A10001986) [3.5]
 *    - Add emergency firmware update via SD (for dev purposes)
 *    - WM fixes (Upload, etc)
 *  2025/10/06 (A10001986)
 *    - WM: Skip setting static IP params in Save
 *    - Add "No SD present" banner in Config Portal if no SD present
 *  2025/10/05 (A10001986)
 *    - CP: Show msg instead of upload file input if no sd card is present
 *  2025/10/05 (A10001986) [3.4.3]
 *    - Let DNS server in AP mode only resolve our domain (hostname)
 *  2025/10/04 (A10001986) [3.4.2]
 *    - Allow 6 clients in AP mode instead of 4. So all props can connect
 *      to the TCD in AP mode, plus one admin computer/hand held.
 *      HOWEVER: Under normal use, do NOT connect a computer or phone to
 *      the TCD in AP mode unless for configuration changes. That computer 
 *      might think it is connected to the internet, resulting in lots of 
 *      network traffic, leading to packet loss, slowdowns or 
 *      out-of-sync animations.
 *  2025/10/03-04 (A10001986) [3.4.1]
 *    - More WiFiManager changes. We no longer use NVS-stored WiFi configs, 
 *      all is managed by our own settings. (No details are known, but it
 *      appears as if the core saves some data to NVS on every reboot, this
 *      is totally not needed for our purposes, nor in the interest of 
 *      flash longevity.)
 *    - Save static IP only if changed
 *  2025/09/22-10/03 (A10001986) [3.4]
 *    - WiFi Manager overhaul; many changes to Config Portal.
 *      WiFi-related settings moved to WiFi Configuration page.
 *      Note: If the TCD is in AP-mode, mp3 playback will be stopped when
 *      accessing Config Portal web pages from now on.
 *      This had lead to sound stutter and incomplete page loads in the past.
 *    - Various code optimizations to minimize code size and used RAM
 *  2025/09/22 (A10001986)
 *    - Config Portal: Re-order settings; remove non-checkbox-code.
 *  2025/09/21 (A10001986)
 *    - Turn former compile-time option "SP_ALWAYS_ON" into run-time option
 *      "Switch off speedo when idle". SP_ALWAYS_ON still exists but now only
 *      defines the default setting.
 *    - Turn former BTTF3_ANIM (for part-3-like date entry animation), REV_AM_PM 
 *      (for reversing AM/PM) and SKIP_TT (for skipping the time-tunnel display
 *      animation) options into run-time options. All default to off.
 *  2025/09/20 (A10001986)
 *    - Config Portal: Display image links to currently connected BTTFN clients 
 *      in top menu
 *    - Config Portal: Add "install sound pack" banner to main menu
 *  2025/09/19 (A10001986) [3.3.1]
 *    - Extend mp3 upload by allowing multiple (max 16) mp3 files to be uploaded
 *      at once. The TCDA.bin file can be uploaded at the same time as well.
 *  2025/09/17-18 (A10001986)
 *    - WiFi Manager: Remove some more unused code to reduce bin size. Reduce 
 *      HTML output by removing "quality icon" styles where not needed. 
 *  2025/09/15 (A10001986) [3.3]
 *    - Refine mp3 upload facility; allow deleting files from SD by prefixing
 *      filename with "delete-".
 *    - WiFi manager: Remove lots of <br> tags; makes Safari display the
 *      pages better.
 *  2025/09/14 (A10001986)
 *    - Allow uploading .mp3 files to SD through config portal. Uses the same
 *      interface as audio container upload. Files are stored in the root
 *      folder of the SD; hence not suitable for music player.
 *  2025/09/13 (A10001986)
 *    - WiFi manager: Remove (ie skip compilation of) unused code
 *    - WiFi manager: Add callback to Erase WiFi settings, before reboot
 *    - WiFi manager: Build param page with fixed size string to avoid memory 
 *      fragmentation; add functions to calculate String size beforehand.
 *  2025/09/12 (A10001986)
 *    - Use "century" bit of RTC in order to restore correct time from RTC if 
 *      TCD is powered down before 1/1/2099 and powered-up after 31/12/2099.
 *    - Make several class methods (implicitly) "inline" to reduce bin size
 *    - Limit speed from GPS receiver to int8 range; minor fix for "-" to "0"
 *      transition in case of lacking fix.
 *  2025/09/09 (A10001986)
 *    - Change version display: TW edition = "A", CS edition = "C"
 *  2025/08/30 (A10001986) [3.2.005]
 *    - Speedo: Add option to display speed with leading 0. Defaults to off.
 *  2025/01/15-17 (A10001986) [3.2.004]
 *    - Optimize play_key; keyX will be stopped instead of (re)started if it is 
 *      currently played when repeatedly triggered.
 *    - Do not reduce volume in night mode for sound played over line-out
 *    - Minor audio file-check optimizations
 *  2025/01/11-12 (A10001986) [3.2.003]
 *    - BTTFN: Add support for requesting currently displayed present, destination, 
 *      departed times
 *    - BTTFN: Add support for remote controlling TCD keypad. Supported by FC 1.70
 *      and SID 1.50, through their IR remote control.
 *  2025/01/07 (A10001986) [3.2.002]
 *    - Sensors: Fix HDC302x detection and humidity
 *  2025/01/01 (A10001986) [3.2.001]
 *    - Set epochs to 2025
 *  2024/12/22 (A10001986)
 *    - Refine change of 2024/11/07. If the fix is lost, the speedo displays "--" for
 *      60 seconds and then switches to "00". If the last GPS speed registered (before
 *      losing the fix) was < 4, it goes to "00" directly. It's nicer to see "--" in a
 *      tunnel than "00", and the exhibition situation (stationary car, indoors) is
 *      still covered.
 *  2024/11/15 (A10001986) [3.2.000]
 *    - Set version number to 3.2.000, in order define a clear limit for Remote
 *      compatibility. No changes.
 *  2024/11/07 (A10001986)
 *    - GPS: Display "00" when there is no fix. Old behavior (--) can be compile-time
 *      configured. Use case: Indoor presentation....
 *  2024/10/27 (A10001986)
 *    - BTTFN: Do regular speed broadcasts every ~3 secs even if speed didn't change.
 *    - BTTFN: Speed from RotEnc only valid while fake power is on
 *  2024/10/26 (A10001986)
 *    - BTTFN: Use multicast for TCD's notifications, instead of sending out
 *      several packets to individual clients. Only used if all connected clients
 *      support that.
 *    - BTTFN: Add "speed broadcast" (only sent if at least one client supports
 *      this); replace specific NOT_REM_SPD packets by this broadcast. (This means
 *      that Futaba "remote" firmwares <0.90 won't work correctly anymore. Get your
 *      update here: https://remote.out-a-ti.me)
 *  2024/10/24 (A10001986)
 *    - Remote: Transmit start of P0 at speed = 0 as well (as opposed to
 *      start transmitting after first increase).
 *  2024/10/23 (A10001986)
 *    - Speedo/acceleration: Implement option to use either real-life figures
 *      (as before), or movie-like figures (matching the Futaba Remote's times).
 *      Accel factor only applies to real-life times.
 *      "Movie-like" is measured/interpolated from mph incrementing times on the 
 *      Remote control's display.
 *      If speedo is not found or disabled, movie-like times are used.
 *  2024/10/15 (A10001986)
 *    - Add support for HDC302X temperature/humidity sensor (yet untested)
 *  2024/10/03 (A10001986)
 *    - Smoothen speedo's catch-up to Remote speed
 *  2024/10/02 (A10001986)
 *    - Speedo: Minor (cosmetic) changes
 *  2024/09/28 (A10001986)
 *    - Revisit UTF8 filtering for MQTT messages and ID3 data.
 *    - Properly truncate UTF8 strings (MQTT user/topics) if beyond buffer
 *      size. (Browsers' 'maxlength' is in characters, buffers are in bytes.
 *      MQTT is generally UTF8, and WiFiManager treats maxlength=buffer size,
 *      something to deal with at some point.)
 *  2024/09/22 (A10001986)
 *    - Sensors: Add logic to update display when sensor wasn't ready at last
 *      sensor read.
 *  2024/09/13 (A10001986)
 *    - Remote: Override user-configured accel factor when the TCD catches up to  
 *      the Remote when brake released (so that doing that at 65 still allows for
 *      the TCD to fully catch up)
 *  2024/09/11 (A10001986)
 *    - Fix C99-compliance
 *  2024/09/10 (A10001986)
 *    - Fake GPS speed for BTTFN clients during P0
 *    - TCD becomes speed master over Remote in P0
 *  2024/09/09 (A10001986)
 *    - New custom sounds: "remoteon.mp3"/"remoteoff.mp3" on SD card, will
 *      be played when Remote is switched on/off. No default sounds.
 *    - Fix logic which speed to tell BTTFN clients
 *  2024/09/08 (A10001986) [A10001986 3.1]
 *    - Fix for line-out switching
 *    - Fix for Remote vs. GPS when no GPS speed is available
 *  2024/09/02 (A10001986)
 *    - Skip white led blink if fake power switch is on pos during boot
 *  2024/08/30-31 (A10001986)
 *    - Many fixes for Remote feature
 *    - Make rotary encoder react quicker after end of time travel (removed
 *      4 second delay)
 *  2024/08/26-29 (A10001986)
 *    - Add preliminary support for CS/A10001986-modified Futaba remote prop
 *  2024/08/23 (A10001986)
 *    - Key3, key6 are now played over line-out (if available and enabled)
 *  2024/08/05 (A10001986)
 *    - Audio: Clear DYNVOL flag if sound is played over line-out
 *    - Intro sound is now played over line-out (if available and enabled)
 *    - New stereo intro sound (sound-pack update required)
 *  2024/07/23 (A10001986)
 *    - Disable ESP32 status led on CB < 1.4.5
 *  2024/06/19 (A10001986)
 *    - Fix: Unmute secondary audio DAC (CB 1.4.5)
 *  2024/06/05 (A10001986)
 *    - Minor fixes for WiFiManager
 *    * Switched to esp32-arduino 2.0.17 for pre-compiled binary.
 *  2024/05/16 (A10001986)
 *    - Re-do DTMF files (added silence to avoid cut-off)
 *      (Another sound-pack update)
 *  2024/05/15 (A10001986)
 *    - Rework line-out setting (350/351 on keypad, instead of option in CP)
 *  2024/05/14 (A10001986)
 *    - Allow CP access if RTC not found
 *    - Switched keypad sounds to wav for more immediate play-back
 *  2024/05/12-13 (A10001986)
 *    - Some preparations for TCD CB 1.4.5
 *  2024/05/09 (A10001986)
 *    - Pre-init ENTER button without pull up
 *  2024/04/12 (A10001986)
 *    - Add temperature unit bit for BTTFN status
 *  2024/04/04 (A10001986)
 *    - Re-phrase "shuffle" Config option
 *  2024/03/27 (A10001986)
 *    - Bug fix in temperature display
 *  2024/03/26 (A10001986)
 *    - BTTFN: Device type VSR (8xxx) added; AUX is now 5xxx.
 *  2024/02/17 (A10001986)
 *    - Minor changes to time travel display disruption in destination time display
 *  2024/02/16 (A10001986)
 *    - Save AutoInterval and brightness in separate config files, remove them from
 *      main config. Both considered "secondary settings", therefore optionally 
 *      saved on SD.
 *  2024/02/14 (A10001986)
 *    - Tweak acceleration sound logic; use with-lead-version of tt sound if time 
 *      is too short for the accel sound, but long enough for the lead.
 *  2024/02/13 (A10001986)
 *    - New user-sound: If SD contains "ttaccel.mp3", this file will be played 
 *      immediately upon initiating a time travel when a speedo is connected, during 
 *      the acceleration phase, until the start of the actual time travel (at which 
 *      point it is interrupted by the usual time travel sound). This sound can
 *      "bridge" the silence while the speedo counts up.
 *    - 998 restores destination and last time departed displays to user-stored values
 *      (ie the ones programmed through the keypad menu, unless overwritten because
 *      "Persistent Time Travels" were enabled at some point after programming those
 *      times).
 *      Command only valid if Persistent Time Travels are off. Pauses time-cycling for
 *      30 mins. Meant as an extension to Exhibition mode for quickly displaying
 *      pre-programmed times.
 *  2024/02/08-12 (A10001986)
 *    - Time: Time stored in the hardware RTC is now UTC (was local time previously). 
 *      This simplifies handling and avoids ambiguities in connection with DST. 
 *      After this update, displayed time will be off unless synced via NTP or GPS.
 *    - User-set dates/times for Destination and Last Time Departed displays are now
 *      considered "secondary settings" and saved to SD (if available and corresponding 
 *      option is set).
 *    - "Persistent Time Travels" now require the option "Save secondary settings to
 *      SD" to be checked, and an SD card to be present. Time travel data is no longer 
 *      saved to esp32 flash memory. Writing to esp32 flash can block for up to 1200ms 
 *      per display, causing undesired disruptions and a bad user experience.
 *    - lastYear and yearOffset are now saved together (in esp32 flash memory).
 *      (presentTime's display specific data is now only the timeDifference.)
 *    - GPS: Don't "hot restart" receiver in boot process any more
 *    - CP: Add header to "Saved" page so user can return to main menu (wm.cpp)
 *    - Time Cycling: Use displayed time, not actual time as interval boundaries
 *      (except when in Exhibition mode)
 *    - Do not "return from time travel" if not on a time travel.
 *    - Exhibition mode: Leave timeDifference alone when time travelling in Exh.
 *      mode. Previously, a time travel was performed for local time and Exh. mode
 *      time simultaneously, which was confusing then disabling Exh. mode.
 *    - Don't call old waitAudioDone[now: Menu] from outside of menu
 *  2024/02/07 (A10001986)
 *    - Config Portal: Propose most used time-zones as datalists for time zone
 *      entry.
 *    - Fix timedifference after year-rollover 9999->1 (bug introduced with Julian
 *      calendar)
 *  2024/02/06 (A10001986)
 *    - Fix reading and parsing of JSON document
 *    - Fixes for using ArduinoJSON 7; not used in bin yet, too immature IMHO.
 *  2024/02/05 (A10001986)
 *    - Tweaks for audio upload
 *  2024/02/04 (A10001986)
 *    - Include fork of WiFiManager (2.0.16rc2 with minor patches) in order to cut 
 *      down bin size
 *    - Audio data (TCDA.bin) can now be uploaded through Config Portal ("UPDATE" page). 
 *      Requires an SD card present.
 *  2024/02/03 (A10001986)
 *    - Check for audio data present also in FlashROMode; better container
 *      validity check; display hint in CP if current audio not installed
 *  2024/01/31 (A10001986)
 *    - Various minor code optimizations (audio, keypad, input, speedo)
 *  2024/01/30 (A10001986)
 *    - Keypad menu: Clear input buffer when quitting the menu (ie discard input
 *      from before entering the menu); Update present time while cycling through 
 *      menu; activate colon for menu items showing time
 *  2024/01/26 (A10001986)
 *    - Add sound-pack versioning; re-installation required with this FW update
 *    - Reformat FlashFS only if audio file installation fails due to a write error
 *    - Some minor bin-size-crunching
 *    - Allow sound installation in Flash-RO-mode
 *  2024/01/22 (A10001986) [released by CS as 3.0.0]
 *    - Save Exh-mode settings when reformatting Flash FS or when copying from
 *      or to SD
 *  2024/01/21 (A10001986)
 *    - BTTFN no longer compile time option, always included
 *    - Major cleanup (code re-ordering, no functional changes)
 *  2024/01/18 (A10001986)
 *    - Fix WiFi menu size
 *  2024/01/16 (A10001986)
 *    - Keypad menu: Propose volume level similar to current knob position when
 *      switching from knob to pre-selected level.
 *  2024/01/15 (A10001986)
 *    - Flush outstanding delayed saves before rebooting and on fake-power-off
 *    - Remove "Restart" menu item from CP, can't let WifiManager reboot behind
 *      our back.
 *  2024/01/14  (A10001986)
 *    - RotEnc: There are primary and secondary RotEncs now, ie you can connect
 *      two rotary encoders, one for Speed (as before), one for Volume.
 *      The primary RotEnc is used for Speed. A secondary RotEnc can be connected
 *      for volume. Secondary RotEncs use different i2c addresses.
 *      Using a RotEnc for volume requires disabling the volume knob by selecting 
 *      a software-controlled level the keypad menu's "Volume" settings. 
 *      RotEnc-changed volume is saved 10 seconds after last change.
 *  2024/01/13  (A10001986)
 *    - New keypad codes:
 *      300-319 sets volume level; 399 activates volume pot; those are short-cuts
 *      to avoid the keypad menu to change the volume selection.
 *  2024/01/06  (A10001986)
 *    - Minor optimizations in time loop; fix timer and reminder suppression
 *      when alarm wouldn't sound because of weekday mismatch
 *  2024/01/03  (A10001986)
 *    - Add "aux" network commands for custom props (8xxx and 8xxxxxx)
 *  2024/01/01  (A10001986)
 *    - Switch epoch definitions to 2024
 *  2023/12/28  (A10001986)
 *    - GPS: Make speedo update rate user-configurable; code simplifications.
 *    - CP: Save space by building select boxes in loops
 *  2023/12/27  (A10001986)
 *    - GPS: Switch to VTG sentence as main speed source for 200ms and 250ms
 *      polling rates; fake 1 and 2mph (GPS data is unreliable at very low 
 *      speeds).
 *  2023/12/26  (A10001986)
 *    - GPS: Increase speed update for speedo-display to 5Hz; more is pointless
 *      as speed gets increasingly jumpy (it already is at 5Hz); better pace-
 *      keeping for GPS polling and displaying speed
 *    - Properly implement millis64()
 *    - Convert some remaining double math into float
 *  2023/12/23  (A10001986)
 *    - GPS: Sync read rate to update rate for smooth speedo display
 *  2023/12/22  (A10001986)
 *    - GPS: Minor changes (use caps for checksum; re-order setDate code)
 *  2023/12/21  (A10001986)
 *    - Some further minor code changes to reduce bin size
 *    - GPS: Read 128 bytes at a time in 4Hz mode; get rid of delay.
 *  2023/12/20  (A10001986)
 *    - Some code size reductions (reduced number of setters in classes)
 *  2023/12/19  (A10001986)
 *    - GPS: Add 4Hz speed update rate compile time option. This is only for when 
 *      displaying speed on the speedo, but puts more stress on overall timing.
 *  2023/12/18  (A10001986)
 *    - Some minor optimizations when calculating local time (hand previously
 *      calculated mins to localToDST)
 *  2023/12/13  (A10001986)
 *    - Keypad input length check logic fix
 *  2023/12/12  (A10001986)
 *    - Display Exhibition Mode status upon enabling/disabling Exh Mode
 *    - Overwrite Exh Mode time on TT / Return from TT only if Exh Mode is
 *      currently on.
 *  2023/12/11  (A10001986)
 *    - Add support for Julian calendar for the period of Jan 1, 1, until either
 *      Sep 2, 1752, or Oct 4, 1582 (defined at compile time, not user configurable, 
 *      due to pre-calculated tables).
 *      Pre-compiled binary uses 1752, the time machine was built in the USA
 *      after all.
 *      If the option "Make time travels persistent" is set while updating,
 *      your present time will be wrong afterwards.
 *    - 33+ENTER shows weekday of currently displayed "present time" date.
 *  2023/12/08  (A10001986)
 *    - Add way to trigger TT from props connected via BTTFN
 *    - Play no sound on TCD on "refill" command
 *    - Overrule RotEnc's "fakeSpeed" already in P0 of TT; this avoids a
 *      mismatch on BTTFN-connected props during their TT sequence.
 *  2023/12/03  (A10001986)
 *    - Sound update (new sound-pack)
 *  2023/11/24  (A10001986)
 *    - Support for DuPPA I2CEncoder 2.1 verified working (no changes)
 *  2023/11/22  (A10001986)
 *    - Speed up date/time transmission to BTTFN clients, and add 24hr flag
 *    - Start beep timer when RotEnc it moved
 *    - Add signal bit for BTTF clients so that they know if "speed" is from 
 *      GPS or RotEnc
 *    - Clean up delay methods
 *  2023/11/19  (A10001986)
 *    - RotEnc: Transmit speedo count-down in P2 to BTTF clients if RotEnc
 *      was in use at the time of the trigger.
 *    - RotEnc logic fixes (reset to "disabled" pos if disabled before 
 *      time travel; don't update during P2; after templock timeout, reset 
 *      enc to "disabled" pos; etc.)
 *  2023/11/18  (A10001986)
 *    - RotEnc: Start from displayed speed when triggering a tt via keypad
 *      or external trigger
 *  2023/11/16  (A10001986)
 *    - Finalize support for MS8607 temp+hum sensor; verified working.
 *  2023/11/15  (A10001986)
 *    - Finalize support for DFRobot Gravity 360 rotary encoder, verified working. 
 *    - Logic fixes for all rotary encoders
 *    * Switched to esp32-arduino 2.0.14 for pre-compiled binary.
 *  2023/11/13  (A10001986)
 *    - Prepare support for DuPPa V2.1 rotary encoder (https://www.duppa.net/shop/i2cencoder-v2-1/)
 *      ("Mini" not supported due to i2c address conflict)
 *    - Prepare support for DFRobot Gravity 360 rotary encoder (https://www.dfrobot.com/product-2575.html)
 *  2023/11/10  (A10001986)
 *    - TSL2591 light sensor verified working
 *  2023/11/09  (A10001986)
 *    - Prepare support for MS8607 temp+hum sensor (yet untested)
 *  2023/11/08  (A10001986)
 *    - Fixes for rot enc (direction; some logic)
 *  2023/11/06-07 (A10001986) [released by CS as 2.9.1]
 *    - Abort audio file installer on first error
 *    - Add TSL2591 light sensor support (yet untested), since 2561 is discontinued.
 *    - Fixes for rotatry encoder
 *    - Fix boot loop when SP_ALWAYS_ON is enabled
 *  2023/11/05 (A10001986)
 *    - Settings: (De)serialize JSON from/to buffer instead of file
 *    - Customize ArduinoJSON compile-time options to reduce bin size
 *  2023/11/04 (A10001986)
 *    - Add premlimiary support for a rotary encoder to select "speed" to be 
 *      displayed on the speedo, and provided to BTTFN clients as (fake) "GPS 
 *      speed". Only hardware type currently supported is the Adafruit 4991.
 *    - Unmount filesystems before reboot
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *    * Switched to LittleFS. Settings will be reset to defaults, and audio files have
 *      to be re-installed after updating from any previous version.
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  2023/11/02 (A10001986)
 *    * WiFiManager: Disable pre-scanning of WiFi networks when starting the CP.
 *      Scanning is now only done when accessing the "Configure WiFi" page.
 *      To do that in your own build-chain, set _preloadwifiscan to false
 *      in WiFiManager.h
 *  2023/10/31 (A10001986)
 *    - Further defer starting the Config Portal in some cases to avoid WiFi scan 
 *      that interferes with BTTFN discover and initial communication
 *  2023/10/30 (A10001986)
 *    - BTTFN: Clients can now discover the TCD's IP address through the TCD's 
 *      hostname. Uses multicast, not DNS.
 *  2023/10/10 (A10001986) [released by CS as 2.9]
 *    - Fix P1 length sent to BTTFN clients
 *  2023/10/05 (A10001986)
 *    - Colons on in night mode
 *    - Send wakeup to network clients after entering a destination date, upon
 *      return-from-time-travel, and when a delayed tt (ETT) is triggered. 
 *      IAW: Whenever the beep timer is(would be) restarted, the wakeup is sent.
 *    - Auto-expire beep-timer when entering night mode (for consistency with 
 *      other props)
 *  2023/10/04 (A10001986)
 *    - Don't use speedo if not detected
 *  2023/10/02 (A10001986)
 *    - Exhibition mode: Honor "Make time travels persistent" option properly
 *  2023/09/30 (A10001986)
 *    - Make remote commands for FC, SID, PG 32bit
 *    - Exhibition mode: Make persistent over reboots/power-downs
 *    * Exhibition mode feature enabled in precompiled binary
 *  2023/09/26 (A10001986)
 *    - Add second payload to BTTFN notifications
 *    - Fix delayed TT-P1; sequence-checks would fail during run of delay.
 *  2023/09/24 (A10001986) [sound-pack-20230924]
 *    - Make timetravel more immediate for some hardware configurations and in
 *      some situations. Enhances inter-operability with third party props.
 *      !!! Requires new sound-pack !!!
 *  2023/09/23 (A10001986)
 *    - Add remote control facility through TCD keypad for Flux Capacitor, SID and 
 *      Plutonium Gauges via BTTFN.
 *      3xxx = Flux Capacitor, 6xxx = SID, 9xxx = Plutonium Gauges. 
 *      See docs of respective prop for more information.
 *    - Add REFILL network command for Plutonium gauges; triggered by
 *      009ENTER.
 *    - Fix display of BTTFN client names
 *  2023/09/16 (A10001986)
 *    - haveFS check before attempting to open audio file from flashFS
 *  2023/09/13 (A10001986)
 *    - Add compile time option to disable tt sequence display flicker
 *      (TT_NO_ANIM)
 *  2023/09/11 (A10001986)
 *    - Guard SPIFFS/LittleFS calls with FS check
 *  2023/09/10 (A10001986)
 *    - If specific config file not found on SD, read from FlashFS - but only
 *      if it is mounted.
 *    - New link to time zones in CP
 *  2023/09/09 (A10001986)
 *    - If SD mount fails at 16Mhz, retry at 25Mhz
 *    - If specific config file not found on SD, read from FlashFS
 *  2023/09/08 (A10001986)
 *    - Add Exhibition mode:
 *      ) 99mmddyyyyhhMM sets a fixed present time
 *      ) 999 toggles between fixed time and normal operation.
 *      [feature disabled in precompiled binary]
 *  2023/09/06 (A10001986)
 *    - Change link in CP
 *  2023/09/04 (A10001986)
 *    - Add option to signal time travel on TT_OUT/IO14 without 5 seconds lead. This
 *      is for signalling a time travel to third party props. For CircuitSetup
 *      original props (if they are connected by wire) this option must NOT be set.
 *      Time travels are still approx 1.4 seconds delayed (time between button press
 *      or TT_IN going active and actual time travel start) because the TCD's time
 *      travel sound starts before the time travel starts.
 *  2023/09/02 (A10001986)
 *    - Make lead time for time travel variable for BTTFN clients. This is especially
 *      important for when GPS is in action. Triggering a TT at high speeds (> 80mph)
 *      was delayed due to the fixed 5000ms ETTO_LEAD. It still is for wired clients
 *      and MQTT, but if only BTTFN is active (ie no wired clients, MQTT off or not
 *      publishing events), the delay is now reduced dynamically.
 *  2023/08/31 (A10001986)
 *    - WiFi connect retry: When no network config'd, set retry to 1
 *  2023/08/28 (A10001986)
 *    - Config Portal: Clicking on header logo jumps to main menu page
 *    - Holding ENTER during boot not only deletes static IP config (as before), but
 *       also temporarily clears AP mode WiFi password (until reboot).
 *  2023/08/27 (A10001986)
 *    - Add "AP name appendix" setting; allows unique AP names when running multiple 
 *      TCDs in AP mode in close range. 7 characters, 0-9/a-z/A-Z/- only, will be 
 *      added to "TCD-AP".
 *    - Add AP password: Allows to configure a WPA2 password for the TCD's AP mode
 *      (empty or 8 characters, 0-9/a-z/A-Z/- only)
 *    - Increase JSON max size for main settings
 *    - Some text changes in Config Portal
 *    - Fix string copy function for 0-length-strings
 *    - Fix HTML pattern attributes for text fields
 *  2023/08/25 (A10001986)
 *    - Restrict WiFi conn retrys to 10 (like WiFiManager)
 *  2023/08/24 (A10001986)
 *    - Add "car mode": Reboots (and stays) in AP-mode. This speeds up boot in a car
 *      without deleting the WiFi configuration; car mode is enabled by typing 991ENTER,
 *      and disabled with 990ENTER. Car mode needs to be enabled if the TCD is acting
 *      as AP for FC/SID and all props are powered up simultaneously.
 *    - Fix fake-power-up for peripherals during boot
 *  2023/08/20 (A10001986)
 *    - MQTT-induced TT is now always immediate, does not honor the delay configured
 *      for the external time travel button
 *  2023/08/19 (A10001986)
 *    - Minor debug code change (no binary change)
 *  2023/08/14 (A10001986)
 *    - (Remove historic and pointless casts for atoi)
 *  2023/08/04 (A10001986)
 *    - Minor cleanup
 *  2023/08/03 (A10001986)
 *    - Fix "animation" and version string on A-Car displays
 *    - Add compile time options (tc_global.h) for
 *      - showing BTTF3-like animation when entering a dest time
 *        (and only then; also not when RC mode is active)
 *      - reversing AM and PM, like seen in some BTTF2/3 scenes
 *  2023/07/31 (A10001986)
 *    - Don't show "REPLACE BATT" on uninitialized clocks
 *    - Remove old unneeded stuff
 *  2023/07/25 (A10001986)
 *    - Loop for adding WiFi parameters
 *  2023/07/24 (A10001986)
 *    - Truncate beep sound data (for bin size)
 *    - Reduce/shrink debug output (for bin size)
 *  2023/07/22 (A10001986)
 *    - BTTFN: Add client device type; let others query IP of given type
 *    - Do some minor binary size crunching
 *  2023/07/20 (A10001986)
 *    - Make "beep" ever so slightly shorter
 *    - Hack to display "0" on CircuitSetup's speedo's fake third digit
 *      (Enabled by compile-time-option, see tc_global.h)
 *  2023/07/17 (A10001986)
 *    - Try to make keypad sounds more immediate
 *  2023/07/11 (A10001986)
 *    - Clean up
 *  2023/07/09 (A10001986)
 *    - BTTFN: Provide some status info to BTTFN clients
 *    - Config Portal: Rename some options for simplification/briefness
 *    - Internal flag-cleanup/fix
 *  2023/07/07 (A10001986)
 *    - Block WiFi power-saving as long as BTTFN clients are present
 *    - GPS speed: Increase update rate to twice per second for smoother speedo display.
 *      Also, add CP option "Provide GPS speed" for peripherals that poll the TCD for 
 *      speed (such as SID). If neither this nor the option "Display GPS speed" is set, 
 *      GPS rate is every 5 seconds; if either is set, twice per second.
 *    - Extend mere "network polling" into "BTTF network" ("BTTFN"): TCD now not only 
 *      answers to polling requests (time, temp, lux, speed), but also sends notifications
 *      to known clients about time timetravel and alarm. Those notifications are only
 *      sent, if MQTT is disabled or "send event notifications" is unchecked.
 *    - BTTFN: Show connected clients in keypad menu (hostname, IP). This helps finding 
 *      out the IP address of other props when they are connected to the TCD's AP.
 *  2023/06/27 (A10001986)
 *    - Fix isIp()
 *  2023/06/26 (A10001986)
 *    - Network polling: Add response marker
 *  2023/06/23 (A10001986)
 *    - Add network data polling facility (time, speed, temperature), in preparation for
 *      other bttf props
 *  2023/06/19 (A10001986)
 *    - Fake power: Turn on keypad LEDs in sync with displays
 *    - Re-do beep sound (better sync'd under ESP32-Arduino 2.0.9)
 *    - Display "updating" and stop audio upon OTA fw update
 *  2023/06/12 (A10001986)
 *    - Don't show audio installer if in FlashROMode
 *  2023/06/06 (A10001986)
 *    - Avoid sound distortion on newly initialized clocks by saving lastYear in setup(),
 *      and skip NTP/GPS update during startup sequence. (Writing to flash is slow enough
 *      to cause sound disturbances)
 *    - Reduce read access to NVM by caching
 *  2023/06/02 (A10001986)
 *    - Display Title & Artist from ID3 tags when typing 55ENTER to display the 
 *      currently played song. (ID3v2 only)
 *  2023/05/30 (A10001986)
 *    - MusicPlayer: 55ENTER shows currently played song (changed this from 88 to avoid
 *      going to song 0 by accident)
 *    - MusicPlayer: Add "auto-renamer". User can now put his mp3-files with original
 *      names (eg "Dire Straits - Money For Nothing.mp3") in the musicX folders on the 
 *      SD and they will be automatically renamed to "000.mp3" and so on. File names are 
 *      sorted alphabetically before renaming (in order to keep some kind of order; 
 *      without sorting, the order would be more or less random).
 *      This also works if files are added to a music folder that already contains some
 *      files with numerical file names.
 *      The process can take up to 10 minutes for 1000 files, perhaps even longer. Mac
 *      users are advised to delete the "._"-files before putting the SD back into the
 *      TCD, as removing these files speeds up the process.
 *      Remember: 128kbps maximum bit rate. Recoding from higher bit rates is still up
 *      to the user. Tools to do this batch-wise are plenty, for Mac and Windows MediaHuman 
 *      Audio Converter (https://www.mediahuman.com/download3.html), for example.
 *    * Switched to esp32-arduino 2.0.9 for pre-compiled binary. If you compile and upload 
 *      using the IDE, settings will be lost and audio files will have to be re-installed.
 *      If upgrading via the Config Portal's "Update" feature, everything stays. 
 *    * Updated WiFiManager to 2.0.16rc2 for pre-compiled binary.
 *    * The source code now includes a subset of the ESP8266Audio library version 1.9.5
 *      with some patches. The reason for this is that ESP8266Audio 1.9.7 is somehow 
 *      broken (delays playback for some reason, spits out i2s errors) and I don't know 
 *      what the future holds for this library. I need a stable basis with predictable 
 *      behavior.
 *  2023/05/23 (A10001986)
 *    - Keypad: 440ENTER deletes timer (4400 still does, too; added for uniformity)
 *  2023/05/22 (A10001986) [released by CS as 2.8]
 *    - 77mmddENTER sets reminder to month/day, leaving time unchanged (unless hr and 
 *      min are zero, in which case it sets the reminder to 9am)  
 *    - MusicPlayer: 88ENTER shows currently played song
 *    - Keypad: 11, 44, 77: If alarm/timer/reminder is unset/off, play regular enter
 *      sound, not the "error" one
 *    - If Music Player is active, show "ERROR" for bad/invalid input (since the player
 *      should not be interrupted) (Exception: Programming a Destination Time)
 *  2023/05/21 (A10001986)
 *    - Minor optimizations
 *  2023/05/20 (A10001986)
 *    - Fix beep modes 2 and 3 with time travel
 *  2023/05/19 (A10001986)
 *    - More internal optimizations (reference DateTime instead of copying it over 
 *      function calls; don't read RTC too often, update dt instead, etc)
 *  2023/05/18 (A10001986)
 *    - Internal optimizations (data entry doesn't show leading 0; blink logic; Audio, 
 *      ClockDisplay: Get rid of bools, use flags; etc)
 *  2023/05/17 (A10001986)
 *    - Add yearly/monthly reminder: Type 77mmddhhMM to set a timer that will play a 
 *      sound yearly on given date, or, if the month is 00, every month on given day. 
 *      77 displays current reminder, 770 deletes it, 777 displays the days/hours/mins 
 *      until the next reminder.
 *      Requires new sound-pack. Reminder always based on real local time.
 *    - Night mode no longer adheres to "Alarm base is real present time" setting. It 
 *      is ALWAYS based on real present time now.
 *    - Display colon in special displays where hours and minutes are used as such.
 *    - Fix ee1-regression
 *  2023/05/15 (A10001986)
 *    - Allow both world clock and room condition mode at the same time.
 *      If both on, only temp is shown, either in red display (if no TZ for red display 
 *      was configured) or in yellow display (regardless of a TZ for that display).
 *      113+ENTER toggles both RC/WC-mode (synchronously).
 *  2023/05/13 (A10001986)
 *    - MQTT: Increase reconnect-attempt-interval over time
 *  2023/05/12 (A10001986)
 *    - Music Player: Fix going to song# when player is off
 *    - MQTT: Add async ping to server before trying to connect. This avoids "frozen" 
 *      displays and audio interruptions but requires that the server properly answers 
 *      to ping (ICMP) requests.
 *  2023/05/11 (A10001986)
 *    - MQTT: Make (re)connection/subscription async on MQTT protocol level
 *    - MQTT: Limit re-connection attempts.
 *  2023/05/10 (A10001986)
 *    - Detect "APSTA" WiFi Mode correctly for Keypad Menu
 *    - Optimize menu code; add "blinking" of editable data
 *    - Minor optimizations for TZ parsing and DST checks
 *  2023/05/05 (A10001986)
 *    - Publish "ALARM" via MQTT
 *    - Fix compilation if GPS is to be left out
 *  2023/05/04 (A10001986)
 *    - Ignore MQTT messages with a length of zero; fix long string handling
 *  2023/05/03 (A10001986)
 *    - Add missing characters to 7-seg font
 *    - JS optimization for CP
 *    - Fix MQTT message scrolling
 *  2023/05/02 (A10001986)
 *    * [Pre-compiled binary: Patch WiFiManager::HTTPSend (avoid duplication of String)]
 *    - HA/MQTT: Publish "REENTRY" for external props; fix error in topic scanning;
 *      subscribe two topics at a time.
 *    - time_loop(): Move less timing critical stuff to when there is no half-
 *      second switch.
 *  2023/05/01 (A10001986)
 *    - HA/MQTT changes: More commands supported; commands can be in lower case; 
 *      no commands while keypad menu or sequences are active;
 *    - Using MQTT now disables WiFi power save
 *    - Disable modem sleep to avoid delays in CP and MQTT
 *  2023/04/29 (A10001986)
 *    - BETA: Add HomeAssitant/MQTT 3.1.1 support. MQTT code by Nicholas O'Leary; adapted,
 *      optimized and minimized by me. Only unencrypted traffic, no TLS/SSL support.
 *      Used in three ways:
 *      1) User can send messages to configurable subscribed topic, which are displayed 
 *      on Destination Time display. Only ASCII text messages supported, no UTF8 or 
 *      binary payload. If the SD card contains "ha-alert.mp3", it will be played upon
 *      reception of a message.
 *      2) User can send commands to TCD (topic "bttf/tcd/cmd"), for example TIMETRAVEL
 *      or RETURN (as in "return from time travel").
 *      3) TCD can trigger time travel via MQTT (topic "bttf/tcd/pub"). This works like
 *      the "external time travel" for wired props.
 *      Broker can be configured using IP or domain, optionally with :xxxx for port
 *      number ("192.168.3.5:1234"); the default port is 1883.  User and password are 
 *      optional; format is "user" (for no password) or "user:pass".
 *      Configuring a WiFi power save timeout disables HA/MQTT.
 *      Broker is strongly recommended to be in same local network; network delays
 *      can cause sound and other timing issues.
 *    - Add "homeassitant" menu item to keypad "network" menu to view connection
 *      status.
 *    - Heavily optimized Config Portal. It grew to big for the available memory, so
 *      some options had to be removed. Also, the nice title graphics had to go, it's 
 *      now the Twin Pine Mall all the way.
 *  2023/04/25 (A10001986)
 *    - Minor optimizations (keypad, gps)
 *  2023/04/23 (A10001986)
 *    - Further simplify keypad code (and fix [unlikely] problem with several keys
 *      pressed at the exact same time)
 *  2023/04/22 (A10001986)
 *    - Simplify keypad handling
 *    - Global license is now MIT (with exception). Possibly GPL-infe[c|s]ted parts
 *      all finally rewritten/removed.
 *  2023/04/21 (A10001986)
 *    - Shrink the header gfx in CP to reduce page data size. WiFiManager builds the
 *      page blob by concatenating Strings, lots of Strings. The big header was one
 *      of the first Strings (hence "header"), which apparently lead to massive heap
 *      fragmentation when repeatedly appending other Strings to it, and as a result, 
 *      sometimes to a blank page in the browser, assumingly caused by a memory issue 
 *      somewhere down the line.
 *  2023/04/20 (A10001986)
 *    - Add "city" (or location name) for World Clock time zones; will be displayed
 *      for 3 seconds every 10 seconds. Keep empty to have the time displayed the
 *      whole time (like before).
 *    - Add tiny delay between BMx820 sensor read-outs
 *  2023/04/18 (A10001986)
 *    - Properly respect beepMode 1
 *    - Better "beep" (less hissing)
 *  2023/04/17 (A10001986)
 *    - Remove deprecated EEPROM stuff
 *  2023/04/16 (A10001986)
 *    - Better beep mode change feedback
 *    - Randomize lamptest() for time travel
 *  2023/04/15 (A10001986)
 *    - Properly restore dest/dep times on quitting the menu
 *  2023/04/13 (A10001986)
 *    - Add beep modes: 000 disables beep, 001 enables beep, 002 and 003 enable beep
 *      for 30/60 seconds after entering a destination time and/or upon initiating a 
 *      time travel. If a time travel is triggered by an external button for which a 
 *      delay is configured, the beep starts when pressing the button and ends 30/60
 *      seconds after the re-entry.
 *  2023/04/12 (A10001986)
 *    - Make "periodic reconnection attempts" a config option; might be undesired in
 *      car setups.
 *    - Don't reconnect WiFi for NTP if mp3 is played back or user has used the keypad
 *      within the last 2 minutes.
 *  2023/04/11 (A10001986)
 *    - Add World Clock mode. User can now configure a separate time zone for each the 
 *      red and yellow displays. 112+ENTER toggles World Clock (WC) mode. Red/yellow 
 *      displays will show the time in the configured time zones. Entering a destination 
 *      time, time travel and some other actions disable WC mode.
 *  2023/04/09 (A10001986)
 *    - Revisit WiFi reconnection logic: Support case where WiFi network was inaccessible
 *      during power-up. See comments tc_time.cpp for details.
 *    - Add keypad menu item "TIME SYNC", shows when last time sync (NTP/GPS) was done.
 *  2023/04/06 (A10001986) [would have been CS 2.7 Release]
 *    - Audio: Re-do beep; remove all traces of (obsolete) MIXER; Short fx are now 
 *      played without re-scanning the analog input during play-back. Reason: Pot 
 *      tolerance led to audible "distortions" with very short sounds.
 *  2023/04/05 (A10001986)
 *    - WiFi: Reconnect for NTP also if previously in AP-mode, if a WiFi network 
 *      is configured to connect to. This helps for unstable WiFi networks (or ones 
 *      which are disabled in a schedule), if the power-saving timers are non-zero. 
 *      The TCD might be unable to (re)connect to a previously configured network when 
 *      attempting to sync time via NTP and end up in AP mode. However, after the power-
 *      saving timers expire, an new connection attempt might succeed.
 *      So, if your WiFi network is somewhat unstable, set the WiFi power save timers
 *      (both for station as well as AP mode) so non-zero; this causes re-connects for
 *      NTP allowing time synchonization even if some of those reconnecting attempts
 *      fail (because the WiFi network is not found at some attempts).
 *    - Reset max tx power when connecting to WiFi network (might have been reduced
 *      when falling-back to AP mode earlier)
 *    Note that some WiFi activity (key exchange? Re-scan?) might cause distortions 
 *    in mp3 play back. I experienced this when connecting the TCD to a "personal 
 *    hotspot" on an iPhone; every 2 or 3 minutes exactly, there is a small distortion.
 *    Does not happen when the TCD is connected to my home network.
 *  2023/04/04 (A10001986)
 *    - NTP: Do not overwrite previous packet age in case of a bad(outdated) new packet
 *  2023/04/01 (A10001986)
 *    - Fix beep: Off in nightmode and when unit is fake-powered off
 *    - Fix CB1.3 LED logic: On only if unit is NOT fake powered off, and NOT
 *      in night mode.
 *    - Add beep option to setup page
 *  2023/03/28 (A10001986)
 *    - Add option to keep temperature display on (and dimmed) in night mode
 *  2023/03/23 (A10001986)
 *    - Add (annoying) beep sound. Enabled/disabled by 000+ENTER. Disabled by default.
 *      Requires new sound-pack.
 *  2023/03/16 (A10001986)
 *    - Fixes for CircuitSetup's speedo display
 *  2023/03/05 (A10001986)
 *    - External time travel button: When held for 3 seconds, a "return from time
 *      travel" is triggered. This brings a change in external time travel triggering:
 *      Up until now, a time travel was issued when the button was pressed for 200ms, 
 *      regardless of when it was released. Now, the tt will be triggered upon release 
 *      of the button, as long it is released in under 3000ms. If a delay is 
 *      configured, it, too, will start running upon release.
 *  2023/02/21 (A10001986)
 *    - Prepare for TCD CB 1.3 with switchable LEDs. LEDs are off when fake power
 *      is off, and in night mode.
 *  2023/01/28 (A10001986) [released by CS as 2.6]
 *    - PCF2129 RTC: Fix obvious copy/paste error; add OTP refresh
 *  2023/01/26 (A10001986)
 *    - GPS: Code optimizations; quicker time-sync if GPS has valid time
 *  2023/01/25 (A10001986)
 *    - GPS speed: Fix tt sequence/sound timing when counting up from GPS speed
 *    - GPS speed: Soften count-down from 88 back to current GPS speed
 *    - Turn speedo off when rebooting
 *  2023/01/23 (A10001986)
 *    - GPS speed: Reduce delay between reading speed and displaying it
 *    - GPS time fix: Fractions are of variable size, not always 3 digits
 *  2023/01/22 (A10001986)
 *    - Minor fix (reset flags after audio file installation; better question mark)
 *  2023/01/21 (A10001986)
 *    - Audio installer change: The installer is now invoked automatically during
 *      boot. If the user cancels, the installer is still available as the first menu 
 *      item in the keypad menu. It was, however, removed from the Config Portal.
 *      The installer now removes the ID file after successfully copying the files
 *      so that it is not shown again on reboot.
 *    - Show hint to install audio files during boot if files not present on flash
 *      FS
 *    - Return from tt: Don't stop music player if we were on actual present time
 *      already.
 *  2023/01/20 (A10001986)
 *    - Add count-down timer function: To start, type 44xxENTER, xx being the number
 *      of minutes (2 digits). Type 44ENTER to see the time remaining.
 *    - Update for sound-pack: Add "timer.mp3" sound, changed "alarm.mp3" sound.
 *  2023/01/19 (A10001986)
 *    - CP: All sub-pages are now uniform (redo logo on Update-page)
 *  2023/01/18 (A10001986)
 *    - Invalidate EEPROM contents after migrating to flash FS
 *    - Patch the living daylights out of the CP's WiFi Config Page (new JS with 
 *      corrected logic for Password-placeholder, uniform look, etc)
 *    - Fix regression on WiFi Config page (clicking on SSID-Link)
 *  2023/01/16-17 (A10001986)
 *    - Use files for clock states instead of the EEPROM API
 *    - Some more CP eye-candy, plus an easter egg
 *  2023/01/15 (A10001986)
 *    - More style for the Config Portal
 *  2023/01/14 (A10001986)
 *    - Another NM logic fix (when forced if min == 0)
 *  2023/01/13 (A10001986)
 *    - Minor fixes and enhancement (settings, alarm, etc)
 *  2023/01/12 (A10001986)
 *    - Disable NM and stop music player before copying audio files
 *    - More NM logic fixes
 *  2023/01/11 (A10001986)
 *    - Fix minor logic error in loadCurVolume()
 *    - Really disable night-mode when entering keypad menu
 *    - Fix LR303/329 light sensor lux calculation; sensor now officially supported
 *  2023/01/10 (A10001986)
 *    - Simplify debug and other output (printf)
 *    - GPS: Check NMEA length before other tests
 *  2023/01/09 (A10001986)
 *    - Settings: Add some more checks for validity, code simplifications
 *    - Clockdisplay: Make loadLastYear() return impossible values on error to force 
 *      comparisons to fail
 *    - [Prepare Flash-RO (read-only) mode]
 *  2023/01/07 (A10001986)
 *    - Change "Time-rotation" to "Time-cycling" in CP
 *    - Re-order keypad menu (SET RTC before other displays)
 *    - Replace double by float almost everywhere, precision not needed
 *  2023/01/06 (A10001986)
 *    - Flash wear awareness week continues: Add option to save alarm and volume
 *      settings on SD card. Folks who often change their alarm or volume settings
 *      are advised to check/enable this.
 *      Music folder number was removed from Config Portal, and is now always saved 
 *      on the SD card.
 *    - Code optimizations (mainly settings and rtc)
 *  2023/01/05 (A10001986)
 *    - Keypad menu: Save settings only if they were changed to reduce flash wear.
 *    - Fix "8" being written to typing buffer despite being held
 *    - Fix restoring volume in case of timeout in keypad menu
 *  2023/01/04 (A10001986)
 *    - Minor enhancements for Keypad menu (Music folder number: Show info if /musicX 
 *      or /musicX/000.mp3 is not found on SD; Alarm: Blink on/off to indicate which
 *      field is to be edited; etc); code simplifications.
 *  2023/01/02 (A10001986)
 *    - Night-mode: Treat daily schedule like other schedules with regard to hourly
 *      checks; this fixes returning to nm after manual override.
 *    - Add 11+ENTER short-cut to quickly show alarm settings
 *    - Alarm times are ALWAYS shown in 24-hour mode, regardless of Config Portal
 *      24hr setting.
 *  2023/01/01 (A10001986)
 *    - Have a go at wobbling volume, again: New "noise reduction" from apparently
 *      totally unstable analog input; knob now always mutes when turned fully off; 
 *      new fixed-level table with better granularity at lower volumes, also it now
 *      has 20 levels.
 *    - Audio: Add mp3 music player. Music files must reside on the SD card in /musicX 
 *      (X being 0-9) folder(s), be named 000.mp3 up to 999.mp3 (starting at 000.mp3 
 *      in each folder), and be of max 128kpbs. Up to ten folders (music0-music9) are 
 *      supported; the folder number is selected in the Config Portal or the keypad 
 *      menu (default: 0, hence /music0).
 *      Keypad layout changed as follows:
 *      Holding 1 now toggles the alarm on/off (formerly: Switch alarm on)
 *      Holding 4 now toggles night-mode on/off (formerly: Switch nm on)
 *      Holding 2 goes to previous song (formerly: Switch alarm off)
 *      Holding 5 starts/stops the mp3 player (formerly: Switch nm off)
 *      Holding 8 goes to next song
 *      222+ENTER    disables shuffle (played in order 000->999)
 *      555+ENTER    enables shuffle (played in random order)
 *      888+ENTER    goes to song 000
 *      888xxx+ENTER goes to song xxx
 *    - Audio: Skip ID3 tags before starting playback. This fixes a freeze when
 *      attempting to play mp3 files with large tags, eg cover art.
 *    - Increase SD/SPI frequency to 16Mhz (with option to reduce back to 4Mhz in
 *      Config Portal)
 *    - Light sensor: Allowed lux level range is now 0-50000
 *  2022/12/26 (A10001986)
 *    - Fix reading BME280 sensor (burst read all data registers for consistency)
 *    - Disable WiFi-power-save when user initiates firmware update
 *    - Display "wait" if Flash-FS is being formatted upon boot
 *    - Add support for LTR3xx light sensor [untested]
 *    - [All supported temperature sensors verified working]
 *  2022/12/19 (A10001986)
 *    - Changed read logic for Si7021 and SHT4x; fix typo in TMP117 code path
 *    - Run MCP9808 in higher resolution mode, scrap sensor shut-down
 *    - Restrict allowed chars in NTP server and hostname fields in Config Portal
 *    * Updated WiFiManager to 2.0.15-rc1 in pre-compiled binary
 *  2022/12/18 (A10001986)
 *    - Audio files installer in keypad menu: If copy fails, re-format flash FS,
 *      re-write settings, and retry copy. (Same can be done by writing "FORMAT"
 *      instead of "COPY" in Config Portal)
 *  2022/12/17 (A10001986)
 *    - "Animate" room condition mode display (imitate delayed month)
 *    - Sensors keypad menu: Read sensors only every 3rd second to avoid self-
 *      heating
 *    - Add mDNS support: Access the Config Portal via http://timecircuits.local (the
 *      hostname is configurable in the Config Portal)
 *    - Disable WiFiManager's "capitive portal" in order to avoid server-redirects to 
 *      IP address.
 *  2022/12/16 (A10001986)
 *    - Add support for SI7021, SHT40, TMP117, AHT20/AM2315C, HTU31D temperature sensors 
 *    - Add reading humidity from BME280, SI7021, SHT40, AHT20, HTU31D sensors 
 *    - Add "room condition" mode, where destination and departed time are replaced
 *      by temperature and humidity, respectively. Toggle normal and rc mode by entering
 *      "111" and pressing ENTER. Sensor values are updated every 30 seconds in this
 *      mode (otherwise every 2 minutes).
 *    - Add humidity display to SENSORS keypad menu
 *    - Add MAC address display to "Network" keypad menu. Only the "station mode" MAC
 *      is shown, ie the MAC address of the ESP32 when it is connected to a WiFi
 *      network. For some reason there are up to four MAC addresses, used in different
 *      network modes (AP, STA, etc). The digit "6" as part of the MAC is shown using
 *      the "modern"/common segment pattern here to distinguish it from "b".
 *    - Fix formatting bug in tc_font.h leading to font missing one character
 *  2022/12/02 (A10001986) [released by CS as 2.5]
 *    - Add support for BMx280 sensor (temperature only).
 *    - Modify former "light sensor" keypad menu to not only show measured lux level
 *      from a light sensor, but also current ambient temperature as measured by
 *      a connected temperature sensor. Rename menu to "Sensors" accordingly.
 *    - Add temperature offset value. User can program an offset that will be added
 *      to the measured temperature for compensating sensor inaccuracies or suboptimal
 *      sensor placement. In order to calibrate the offset, use the keypad menu 
 *      "SENSORS" since the temperature shown there is not rounded (unlike what is
 *      shown on a speedo display if it has less than three digits).
 *  2022/11/22 (A10001986) [released by CS as 2.4]
 *    - Audio: SPIFFS does not adhere to POSIX standards and returns a file object
 *      even if a file does not exist. Fix by work-around (SPIFFS only).
 *    - clockdisplay: lampTest(), as part of the display disruption sequence, might 
 *      be the reason for some red displays to go dark after time travel; reduce 
 *      the number of segments lit.
 *    - Rename "tempSensor" to "sensors" and add light sensor support. Three models
 *      are supported: TSL2561, BH1750, VEML7700/VEML6030 (VEML7700 only if no GPS 
 *      receiver is connected due to an i2c address conflict; VEML6030 must be configured
 *      for address 0x48, ie ADDR must be high, if GPS is connected at the same time). 
 *      The light sensor is solely used for switching the device into night mode. The 
 *      keypad menu as a new option to show the currently measured lux in order to 
 *      determine a suitable threshold to be configured in the Config Portal.
 *    - Display "--" on speedo instead of "NAN" if temperature cannot be read
 *    - Check more properly for GPS sensor (do a read test).
 *    - Main config file now (de)serialized using Dynamic JSON object due to its size
 *  2022/11/11 (A10001986)
 *    - Extended Sound on the Hour: User can now put "hour-xx.mp3" files for each
 *      hour on the SD card (hour-00.mp3, hour-01.mp3, ..., hour-23.mp3). If one of
 *      the files is missing, "hour.mp3" will be played (if it exists on the SD card)
 *    - Support comma on ADA-1270 7-segment display
 *  2022/11/10 (A10001986)
 *    - Minor optimizations (wifi)
 *    - Soft-reset the clock by entering 64738 and ENTER
 *  2022/11/08 (A10001986) [released by CS as 2.3]
 *    - Allow time travel to (non-existing) year 0, so users can simulate the movie
 *      error (Dec 25, 0000).
 *    - RTC can no longer be set to a date below TCEPOCH_GEN (which is 2022 currently)
 *    - Adapt temperature sensor code to allow quickly adding other sensor types
 *    - Fix time travel time difference in case of a 9999->1 roll-over.
 *  2022/11/06 (A10001986)
 *    - Add short-cut to set alarm by typing 11hhmm+ENTER (weekday selection must
 *      still be done through keypad menu)
 *    - Change default time zone to UTC; folks without WiFi should not be bothered
 *      by unexpected DST changes.
 *    - Change default "time travel persistence" to off to avoid flash wear.
 *  2022/11/05 (A10001986)
 *    - Re-order keypad menu; alarm and volume are probably used more often than
 *      programming times for the displays or setting the RTC...
 *    - Add weekday selection for alarm
 *    - Add option to disable time travel sounds (in order to have other props play
 *      theirs. This is only useful in connection with compatible props, triggered by 
 *      IO14.)
 *    - Rename "tc_input.*" files to "input.*" for consistency reasons
 *  2022/11/03 (A10001986)
 *    - Reboot after audio file installation from keypad menu
 *    - Ignore held keys while in keypad menu
 *    - Optimize tc_input
 *  2022/11/02 (A10001986)
 *    - Re-order Config Portal options
 *    - Disable colon in night mode
 *    - Unify keypad and keypad_i2c into keypad_i2c; remove unused stuff, and further
 *      optimize code and improve key responsiveness and debouncing. The dependency 
 *      on Keypad is now history.
 *    - Include minimized version of OneButton library (renamed "TCButton"), thereby
 *      remove dependency on this library.
 *    - "tc_keypadi2c.*" renamed to tc_input.* to better reflect actual contents.
 *  2022/10/31 (A10001986)
 *    - Strengthen logic regarding when to try to resync time with NTP or GPS; if 
 *      there is no autoritative time, WiFi power saving timeout is set long enough 
 *      to avoid consecutive reconnects (and thereby frozen displays). This 
 *      essentially defeats the WiFi power saving feature, but accurate time is 
 *      more important than power saving. This is a clock after all.
 *    - Fix NTP update when WiFi is on power-save mode (ie off); WiFiManager's annoying 
 *      async WiFi scan when re-connecting and starting the Config Portal prohibited an 
 *      NTP update within the available time frame. Fixed by immediately re-triggering 
 *      an NTP request when an old packet timed out.
 *    - Do not start Config Portal when WiFi is reconnecting because of an NTP update.
 *      (This also fixes the issue above, but both measures are reasonable)
 *    - NTP: Give request packets a unique id in order to filter out outdated 
 *      responses; response packet timeout changed from 3 to 10 seconds.
 *  2022/10/29 (A10001986)
 *    - Added auto night-mode presets. There are currently four presets, for
 *      (hopefully) typical home, office and store setups. The times in the description
 *      define when the clock is in use, which might appear somewhat counter-intuitive 
 *      at first, but is easier to describe:
 *      Home: Mon-Thu 5pm-11pm, Fri 1pm-1am, Sat 9am-1am, Sun 9am-11pm
 *      Office (1): Mon-Fri 9am-5pm
 *      Office (2): Mon-Thu 7am-5pm, Fri 7am-2pm
 *      Shop: Mon-Wed 8am-8pm, Thu-Fri 8am-9pm, Sat 8am-5pm
 *      The old daily "start hour" and "end hour" setting still present, of course.
 *    - time_loop: Do not adjust time if DST flag is -1
 *  2022/10/27 (A10001986)
 *    - Minor fixes/enhancements (in time setup, etc)
 *  2022/10/26 (A10001986)
 *    - Enhancements to DST logic
 *    - Fine-tune GPS polling and RTC updating
 *    - Remove unused stuff
 *  2022/10/24 (A10001986) [released by CS as 2.2]
 *    - Defer starting the Config Portal during boot: Starting with 2.0.13beta,
 *      WiFiManager triggers an async WiFi Scan when the CP is started, which 
 *      interferes with our NTP traffic during the boot process. Start CP after NTP 
 *      is done.
 *  2022/10/14-23 (A10001986)
 *    - Fix time travel with speedo: Added forgotten code after re-write.
 *    - Added support for MTK3333 based GPS receivers connected through i2c. These
 *      can act as a source of authoritative time (just like NTP), as well as for
 *      calculating current speed, to be displayed on a speedo display (which only
 *      really makes sense in a car/boat/whatevermoves).
 *    - Due to the system's time library's inability to handle some time zones
 *      correctly (eg Lord Howe and Iran) and its unawareness of the well-known facts  
 *      that NTP will roll-over in 2036 and unix time in 2038 (for which the system 
 *      as it stands is not prepared and will fail), all ties with time library were 
 *      cut, and a completely native time system was implemented, handling NTP/GPS, 
 *      time zones and DST calculation all by itself.
 *      This allows using this firmware with correct clocking until the year 9999 
 *      (at which point it rolls over to 1), with NTP support until around
 *      2150. Also, DST is now automatically switched in stand-alone mode
 *      (ie without NTP or GPS).
 *      For all this to work, it is essential that the user configures his time
 *      zone in the Config Portal, and, for GPS and stand-alone, adjusts the TC's
 *      RTC (real time clock) to current local time using the keypad menu.
 *    - General cleanup (declarations, header files, etc)
 *  2022/10/13 (A10001986)
 *    - Implement own DST management (TZ string parsing, DST time adjustments)
 *      This system is only active, if no authoritative time source is available.
 *      As long as NTP or GPS deliver time, we rely on their assessments.
 *    - Clean up declarations & definitions all over
 *  2022/10/11 (A10001986)  [released by CS as 2.1]
 *    - IMPORTANT BUGFIX: Due to some (IMHO) compiler idiocy and my sloppyness, in 
 *      this case presenting itself in trusting Serial output instead of checking 
 *      the actual result of a function, the entire leap-year-detection was de-funct. 
 *      IOW: The clock didn't support leap years for time travels and some other
 *      functions.
 *    - New and now centralized logic to keep RTC within its supported time span 
 *      when we and our descendants cross over to 2100 or fun loving folks set their 
 *      RTC to years <1900 or >2099.
 *    - Throw out more unused code from DateTime class
 *    - Use Sakamoto's method for day-of-week determination
 *    - Fix tcRTC.getTemperature() (only used in debug mode)
 *    - Clarification: The clock only supports the Gregorian Calendar, of which it
 *      pretends to have been used since year 1. The Julian Calendar is not taken
 *      into account. As a result, some years that, in the Julian Calendar, were leap 
 *      years between years 1 and 1582 in most of today's Europe, 1700 in DK/NO/NL
 *      (except Holland and Zeeland), 1752 in the British Empire, 1753 in Sweden, 
 *      1760 in Lorraine, 1872 in Japan, 1912 in China, 1915 in Bulgaria, 1917 in 
 *      the Ottoman Empire, 1918 in Russia and Serbia and 1923 in Greece, are 
 *      normal years in the Gregorian one. As a result, dates do not match in those 
 *      two calender systems, the Julian calendar is currently 13 days behind. 
 *      I wonder if Doc's TC took all this into account. Then again, he wanted to
 *      see Christ's birth on Dec 25, 0. Luckily, he didn't actually try to travel
 *      to that date. Assuming a negative roll-over, he might have ended up in
 *      eternity.
 *  2022/10/08 (A10001986)
 *    - Integrate cut-down version of RTCLib's DateTime to reduce bloat
 *    - Remove RTCLib dependency; add native RTC support for DS3231 and PCF2129 RTCs
 *  2022/10/06 (A10001986)
 *    - Add unit selector for temperature in Config Portal
 *  2022/10/05 (A10001986)
 *    - Important: The external time travel trigger button is now no longer
 *      on IO14, but IO27. This will require some soldering on existing TC
 *      boards, see https://github.com/realA10001986/Time-Circuits-Display
 *      Also, externally triggered time travels will now include the speedo
 *      sequence as part of the time travel sequence, if a speedo is
 *      connected and activated in the Config Portal. If, in the Config Portal,
 *      "Play complete time travel sequence" is unchecked, only the re-entry
 *      part will be played (as before).
 *    - Add trigger signal for future external props (IO14). If you don't
 *      have any compatible external props connected (which is likely since 
 *      those do not yet exist), please leave this disabled in the Config 
 *      Portal as it might delay the time travel sequence unnecessarily.
 *    - Activate support code for temperature sensor. This is for home setups
 *      with a speedo display connected; the speedo will show the ambient
 *      temperature as read from this sensor while idle.
 *    - [Add support for GPS for speed and time. This is yet inactive as it
 *      is partly untested.]
 *  2022/09/23 (A10001986)
 *    - Minor fixes
 *  2022/09/20 (A10001986)
 *    - Minor bug fixes (speedo)
 *    - [temperature sensor enhanced and fixed; inactive]
 *  2022/09/12 (A10001986)
 *    - Fix brightness logic if changed in menu, and night mode activated
 *      afterwards.
 *    - No longer call .save() on display when changing brightness in menu
 *    - [A10001986 wallclock customization: temperature sensor; inactive]
 *  2022/09/08-10 (A10001986)
 *    - Keypadi2c: Custom delay function to support uninterrupted audio during
 *      key scanning; i2c is now read three times in order to properly
 *      debounce.
 *    - Keypad/menu: Properly debounce enter key in menu (where we cannot
 *      use the keypad lib).
 *    - Menu: Volume setter now plays demo sound with delay (1s)
 *    - Added WiFi-off timers for power saving. After a configurable number of
 *      minutes, WiFi is powered-down. There are separate options for WiFi
 *      in station mode (ie when connected to a WiFi network), and in AP mode
 *      (ie when the device acts as an Access Point).
 *      The timer starts at power-up. To re-connect/re-enable the AP, hold
 *      '7' on the keypad for 2 seconds. WiFi will be re-enabled for the
 *      configured amount of minutes, then powered-down again.
 *      For NTP re-syncs, the firmware will automatically reconnect for a
 *      short period of time.
 *      Reduces power consumption by 0.2W.
 *    - Added CPU power save: The device reduces the CPU speed to 80MHz if
 *      idle and while WiFi is powered-down. (Reducing the CPU speed while
 *      WiFi is active leads to WiFi errors.)
 *      Reduces power consumption by 0.1W.
 *    - Added option to re-format the flash FS in case of FS corruption.
 *      In the Config Portal, write "FORMAT" into the audio file installer
 *      section instead of "COPY". This forces the flash FS to be formatted,
 *      all settings files to be rewritten and the audio files (re-)copied
 *      from SD. Prerequisite: A properly set-up SD card is in the SD card
 *      slot (ie one that contains all original files from the "data" folder
 *      of this repository)
 *  2022/09/07 (A10001986)
 *    - Speedo module re-written to make speedo type configurable at run-time
 *    - WiFi Menu: Params over Info
 *    - time: end autopause regardless of autoInt setting (avoid overflow)
 *  2022/09/05-06 (A10001986)  [released by CS as 2.0]
 *    - Fix TC settings corruption when changing WiFi settings
 *    - Format flash file system if mounting fails
 *    - Reduce WiFi transmit power in AP mode (to avoid power issues with volume
 *      pot if not at minimum)
 *    - Nightmode: Displays can be individually configured to be dimmed or
 *      switched off in night mode
 *    - Fix logic screw-up in autoTimes, changed intervals to 5, 10, 15, 30, 60.
 *    - More Config Portal beauty enhancements
 *    - Clockdisplay: Remove dependency on settings.h
 *    - Fix static ip parameter handling (make sure strings are 0-terminated)
 *    - [I2C-Speedo integration; still inactive]
 *  2022/08/31 (A10001986)
 *    - Add some tool tips to Config Portal
 *  2022/08/30 (A10001986)
 *    - Added delay for externally triggered time-travel, configurable in Config Portal
 *    - Added option to make ext. triggered time-travel long (or short as before)
 *    - Added some easter eggs
 *    - Fix compilation for LittleFS
 *    - Added a little gfx to Config Portal
 *  2022/08/29 (A10001986)
 *    - Auto-Night-Mode added
 *  2022/08/28 (A10001986)
 *    - Cancel enter-animation correctly when other anim is initiated
 *    - Shutdown sound for fake power off
 *    - Minor tweaks to long time travel display "disruption"
 *    - Timetravel un-interruptible by enter key
 *    - Re-do hack to show version on Config Portal
 *    - Add GPL-compliant (C) info
 *  2022/08/26 (A10001986)
 *    - Attempt to beautify the Config Portal by using checkboxes instead of
 *      text input for boolean options
 *  2022/08/25 (A10001986)
 *    - Add default sound file installer. This installer is for initial installation
 *      or upgrade of the firmware. Put the contents of the data folder on a
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
 *      All four IP addresses must be valid. IP settings can be reset to DHCP
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
 *    - Added menu item to show firmware version
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
 *    - Fixed auto time rotation pause logic at menu return
 *    - [Fixed crash when saving settings before WiFi was connected (John)]
 *  2022/08/17 (A10001986)
 *    - Silence compiler warnings
 *    - Fixed missing return value in loadAlarm
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
 *    - Fixed fake power off if time rotation interval is non-zero
 *    - Corrected some inconsistency in my assumptions on A-car display
 *      handling
 *  2022/08/13 (A10001986)
 *    - Changed "fake power" logic : This is no longer a "button" to
 *      only power on, but a switch. The unit can now be "fake" powered
 *      on and "fake" powered off.
 *    - External time travel trigger: Connect active-low button to
 *      io14 (see tc_global.h). Upon activation (press for 200ms), a time
 *      travel is triggered. Note that the device only simulates the
 *      re-entry part of a time travel so the trigger should be timed
 *      accordingly.
 *    - Fixed millis() roll-over errors
 *    - All new sounds. The volume of the various sound effects has been
 *      normalized, and the sound quality has been digitally enhanced.
 *    - Made keypad more responsive
 *    - Fixed garbled keypad sounds in menu
 *    - Fixed timeout logic errors in menu
 *    - Made RTC usable for eternity (by means of yearOffs)
 *  2022/08/12 (A10001986)
 *    - A-Car display support enhanced (untested)
 *    - Added SD support. Audio files will be played from SD, if
 *      an SD is found. Files need to reside in the root folder of
 *      a FAT-formatted SD.
 *      Mp3 files with 128kpbs or below recommended.
 *  2022/08/11 (A10001986)
 *    - Integrated a modified Keypad_I2C into the project in order
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
 *    - Fixed autoInterval array size
 *    - Minor cleanups
 *  2022/08/09 (A10001986)
 *    - Fixed "animation" (ie. month displayed a tad delayed)
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
 *    - Jun 2022 code base imported from
 *      https://github.com/CircuitSetup/Time-Circuits-Display/tree/587ec1c56fefe8f2a0e08e9a014dd10b810c217b
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

#include "tc_global.h"

#include <Arduino.h>
#include <Wire.h>

#include "tc_audio.h"
#include "tc_keypad.h"
#include "tc_menus.h"
#include "tc_settings.h"
#include "tc_time.h"
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

    time_boot();
    settings_setup();
    wifi_setup();
    audio_setup();
    keypad_setup();
    time_setup();
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
    time_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    bttfn_loop();
    bttfn_loop_ex();
    audio_loop();
}
#endif

#if defined(TC_DBG_TIME) || defined(TC_DBG_NET) || defined(TC_DBG_GPS)
#warning "Debug output is enabled. Binary not suitable for release."
#endif
