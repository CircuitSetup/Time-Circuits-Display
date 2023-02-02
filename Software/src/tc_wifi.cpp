/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display-A10001986
 *
 * WiFi and Config Portal handling
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tc_global.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef TC_MDNS
#include <ESPmDNS.h>
#endif

#include <WiFiManager.h>

#include "clockdisplay.h"
#include "tc_menus.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"
#include "tc_wifi.h"

// If undefined, use the checkbox/dropdown-hacks.
// If defined, go back to standard text boxes
//#define TC_NOCHECKBOXES

// If defined, show the audio file installer field
// Since we invoke the installer now automatically,
// this is no longer needed.
//#define CP_AUDIO_INSTALLER

Settings settings;

IPSettings ipsettings;

WiFiManager wm;

static char aintCustHTML[768] = "";
static const char aintCustHTML1[] = "<div style='margin:0;padding:0;'><label for='rotate_times'>Time-cycling interval</label><select style='width:auto;margin-left:10px;vertical-align:baseline;' value='";
static const char aintCustHTML2[] = "' name='rotate_times' id='rotate_times' autocomplete='off' title='Selects the interval for automatic time-cycling when idle'><option value='0'";
static const char aintCustHTML3[] = ">Off</option><option value='1'";
static const char aintCustHTML4[] = ">Every 5th minute</option><option value='2'";
static const char aintCustHTML5[] = ">Every 10th minute</option><option value='3'";
static const char aintCustHTML6[] = ">Every 15th minute</option><option value='4'";
static const char aintCustHTML7[] = ">Every 30th minute</option><option value='5'";
static const char aintCustHTML8[] = ">Every 60th minute</option></select></div>";

static char anmCustHTML[768] = "";
static const char anmCustHTML1[] = "<div style='margin:0;padding:0;'><label for='autonmtimes'>Schedule</label><select style='font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;' value='";
static const char anmCustHTML2[] = "' name='autonmtimes' id='autonmtimes' autocomplete='off' title='Select schedule for auto night-mode'><option value='0'";
static const char anmCustHTML3[] = ">&#128337; Daily, set hours below</option><option value='1'";
static const char anmCustHTML4[] = ">&#127968; M-T:17-23/F:13-1/S:9-1/Su:9-23</option><option value='2'";
static const char anmCustHTML5[] = ">&#127970; M-F:9-17</option><option value='3'";
static const char anmCustHTML6[] = ">&#127970; M-T:7-17/F:7-14</option><option value='4'";
static const char anmCustHTML7[] = ">&#128722; M-W:8-20/T-F:8-21/S:8-17</option></select></div>";

#ifdef TC_HAVESPEEDO
static char spTyCustHTML[1024] = "";
static const char spTyCustHTML1[] = "<div style='margin:0;padding:0;'><label for='speedo_type'>Display type</label><select style='width:auto;margin-left:10px;vertical-align:baseline;' value='";
static const char spTyCustHTML2[] = "' name='speedo_type' id='speedo_type' autocomplete='off' title='Selects type of speedo display'>";
static const char spTyCustHTMLE[] = "</select></div>";
static const char spTyOptP1[] = "<option value='";
static const char spTyOptP2[] = "'>";
static const char spTyOptP3[] = "</option>";
static const char *dispTypeNames[SP_NUM_TYPES] = {
  "CircuitSetup.us\0",
  "Adafruit 878 (4x7)\0",
  "Adafruit 878 (4x7;left)\0",
  "Adafruit 1270 (4x7)\0",
  "Adafruit 1270 (4x7;left)\0",
  "Adafruit 1911 (4x14)\0",
  "Adafruit 1911 (4x14;left)\0",
  "Grove 0.54\" 2x14\0",
  "Grove 0.54\" 4x14\0",
  "Grove 0.54\" 4x14 (left)\0"
#ifndef TWPRIVATE
  ,"Ada 1911 (left tube)\0"
  ,"Ada 878 (left tube)\0"
#else
  ,"A10001986 wallclock\0"
  ,"A10001986 speedo replica\0"
#endif
};
#endif

static const char *aco = "autocomplete='off'";

WiFiManagerParameter custom_headline("<img id='tcgfx' class='tcgfx' src=''>");

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent (0=no, 1=yes)", settings.timesPers, 1, "autocomplete='off' title='If disabled, the displays are reset after reboot'");
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is RTC (1) or displayed \"present\" time (0)", settings.alarmRTC, 1, aco);
WiFiManagerParameter custom_playIntro("plIn", "Play intro (0=off, 1=on)", settings.playIntro, 1, aco);
WiFiManagerParameter custom_mode24("md24", "24-hour clock mode: (0=12hr, 1=24hr)", settings.mode24, 1, aco);
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent", settings.timesPers, 1, "title='If unchecked, the displays are reset after reboot' type='checkbox' style='margin-top:3px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is real present time", settings.alarmRTC, 1, "title='If unchecked, the alarm base is the displayed \"present\" time' type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_playIntro("plIn", "Play intro", settings.playIntro, 1, "type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mode24("md24", "24-hour clock mode", settings.mode24, 1, "type='checkbox' style='margin-bottom:10px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_autoRotateTimes(aintCustHTML);

#if defined(TC_MDNS) || defined(TC_WM_HAS_MDNS)
#define HNTEXT "Hostname<br><span style='font-size:80%'>The Config Portal is accessible at http://<i>hostname</i>.local<br>(Valid characters: a-z/0-9/-)</span>"
#else
#define HNTEXT "Hostname<br><span style='font-size:80%'>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9-]+' placeholder='Example: timecircuits'");
WiFiManagerParameter custom_wifiConRetries("wifiret", "WiFi connection attempts (1-15)", settings.wifiConRetries, 2, "type='number' min='1' max='15' autocomplete='off'", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_wifiConTimeout("wificon", "WiFi connection timeout (7-25[seconds])", settings.wifiConTimeout, 2, "type='number' min='7' max='25'");
WiFiManagerParameter custom_wifiOffDelay("wifioff", "WiFi power save timer<br>(10-99[minutes];0=off)", settings.wifiOffDelay, 2, "type='number' min='0' max='99' title='If in station mode, WiFi will be shut down after chosen number of minutes after power-on. 0 means never.'");
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "WiFi power save timer (AP-mode)<br>(10-99[minutes];0=off)", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='If in AP mode, WiFi will be shut down after chosen number of minutes after power-on. 0 means never.'");
WiFiManagerParameter custom_wifiHint("<div style='margin:0px;padding:0px'>Hold '7' to re-enable Wifi when in power save mode.</div>");

WiFiManagerParameter custom_timeZone("time_zone", "Time zone (in <a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv' target=_blank>Posix</a> format)", settings.timeZone, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0'");
WiFiManagerParameter custom_ntpServer("ntp_server", "NTP Server (empty to disable NTP)", settings.ntpServer, 63, "pattern='[a-zA-Z0-9.-]+' placeholder='Example: pool.ntp.org'");
#ifdef TC_HAVEGPS
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useGPS("uGPS", "Use GPS as time source (0=no, 1=yes)", settings.useGPS, 1, "autocomplete='off' title='Enable to use a GPS receiver as a time source'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useGPS("uGPS", "Use GPS as time source", settings.useGPS, 1, "autocomplete='off' title='Check to use a GPS receiver as a time source' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif

WiFiManagerParameter custom_destTimeBright("dt_bright", "Destination Time display brightness (0-15)", settings.destTimeBright, 2, "type='number' min='0' max='15' autocomplete='off'", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_presTimeBright("pt_bright", "Present Time display brightness (0-15)", settings.presTimeBright, 2, "type='number' min='0' max='15' autocomplete='off'");
WiFiManagerParameter custom_lastTimeBright("lt_bright", "Last Time Dep. display brightness (0-15)", settings.lastTimeBright, 2, "type='number' min='0' max='15' autocomplete='off'");

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time in night mode (0=dimmed, 1=off)", settings.dtNmOff, 1, aco);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time in night mode (0=dimmed, 1=off)", settings.ptNmOff, 1, aco);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. in night mode (0=dimmed, 1=off)", settings.ltNmOff, 1, aco);
WiFiManagerParameter custom_autoNM("anm", "Scheduled night-mode (0=off, 1=on)", settings.autoNM, 1, aco);
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time off in night mode", settings.dtNmOff, 1, "title='If unchecked, the display will be dimmed' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time off in night mode", settings.ptNmOff, 1, "title='If unchecked, the display will be dimmed' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. off in night mode", settings.ltNmOff, 1, "title='If unchecked, the display will be dimmed' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_autoNM("anm", "Scheduled night-mode", settings.autoNM, 1, "title='Check to enable scheduled night-mode' type='checkbox' style='margin-top:14px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_autoNMTimes(anmCustHTML);
WiFiManagerParameter custom_autoNMOn("anmon", "Daily night-mode start hour (0-23)", settings.autoNMOn, 2, "type='number' min='0' max='23' title='Enter hour to switch on night-mode'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Daily night-mode end hour (0-23)", settings.autoNMOff, 2, "type='number' min='0' max='23' autocomplete='off' title='Enter hour to switch off night-mode'");
#ifdef TC_HAVELIGHT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_uLS("uLS", "Use light sensor (0=no, 1=yes)", settings.useLight, 1,  "title='If enabled, device will go into night mode if lux level is below or equal the threshold. Supported sensors: BH1750, TSL2561, LTR3xx, VEML7700/VEML6030' autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_uLS("uLS", "Use light sensor", settings.useLight, 1, "title='If checked, device will go into night mode if lux level is below or equal the threshold. Supported sensors: BH1750, TSL2561, LTR3xx, VEML7700/VEML6030' type='checkbox' style='margin-top:14px'", WFM_LABEL_AFTER);
#endif
WiFiManagerParameter custom_lxLim("lxLim", "<br>Lux threshold (0-50000)", settings.luxLimit, 6, "title='If the lux level is below or equal the threshold, the device will go into night-mode' type='number' min='0' max='50000' autocomplete='off'", WFM_LABEL_BEFORE);
#endif

#ifdef TC_HAVETEMP
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useTemp("uTem", "Use temperature/humidity sensor (0=no, 1=yes)", settings.useTemp, 1, "autocomplete='off' title='Enable to use a temperature/humidity sensor for room condition mode and to display temperature on speedo display while idle. Supported sensors: MCP9808, TMP117, BMx280, SHT4x, SI7012, AHT20/AM2315C, HTU31D'");
WiFiManagerParameter custom_tempUnit("uTem", "Temperture unit (0=°F, 1=°C)", settings.tempUnit, 1, "autocomplete='off' title='Select unit for temperature'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useTemp("uTem", "Use temperature/humidity sensor", settings.useTemp, 1, "title='Check to use a temperature/humidity sensor for room condition mode and to display temperature on speedo display while idle. Supported sensors: MCP9808, TMP117, BMx280, SHT4x, SI7012, AHT20/AM2315C, HTU31D' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_tempUnit("temUnt", "Display in °Celsius", settings.tempUnit, 1, "title='If unchecked, temperature is displayed in Fahrenheit' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_tempOffs("tOffs", "<br>Temperature offset (-3.0-3.0)", settings.tempOffs, 4, "type='number' min='-3.0' max='3.0' step='0.1' title='Correction value to add to temperature' autocomplete='off'");
#endif // TC_HAVETEMP

#ifdef TC_HAVESPEEDO
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useSpeedo("uSpe", "Use speedometer display (0=no, 1=yes)", settings.useSpeedo, 1, "autocomplete='off' title='Enable to use a speedo display'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useSpeedo("uSpe", "Use speedometer display", settings.useSpeedo, 1, "title='Check to use a speedo display' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_speedoType(spTyCustHTML);
WiFiManagerParameter custom_speedoBright("speBri", "<br>Speedo brightness (0-15)", settings.speedoBright, 2, "type='number' min='0' max='15' autocomplete='off'");
WiFiManagerParameter custom_speedoFact("speFac", "Speedo sequence speed factor (0.5-5.0)", settings.speedoFact, 3, "type='number' min='0.5' max='5.0' step='0.5' title='1.0 means the sequence is played in real-world DMC-12 acceleration time. Higher values make the sequence run faster, lower values slower' autocomplete='off'");
#ifdef TC_HAVEGPS
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed (0=no, 1=yes)", settings.useGPSSpeed, 1, "autocomplete='off' title='Enable to use a GPS receiver to display actual speed on speedo display'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed", settings.useGPSSpeed, 1, "autocomplete='off' title='Check to use a GPS receiver to display actual speed on speedo display' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_HAVETEMP
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature (0=no, 1=yes)", settings.useGPSSpeed, 1, "autocomplete='off' title='Enable to display temperature on speedo display when idle (needs temperature sensor)'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature", settings.useGPSSpeed, 1, "autocomplete='off' title='Check to display temperature on speedo display when idle (needs temperature sensor)' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_tempBright("temBri", "<br>Temperature brightness (0-15)", settings.tempBright, 2, "type='number' min='0' max='15' autocomplete='off'");
#endif
#endif // TC_HAVEGPS

#endif // TC_HAVESPEEDO

#ifdef FAKE_POWER_ON
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch (0=no, 1=yes)", settings.fakePwrOn, 1, "autocomplete='off' title='Enable to use a switch to fake-power-up and fake-power-down the device'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch", settings.fakePwrOn, 1, "title='Check to use a switch to fake-power-up and fake-power-down the device' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif

#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettDelay("ettDe", "External time travel button<br>Delay (ms)", settings.ettDelay, 5, "type='number' min='0' max='60000' title='Externally triggered time travel will be delayed by specified number of millisecs'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_ettLong("ettLg", "Time travel sequence (0=short, 1=complete)", settings.ettLong, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_ettLong("ettLg", "Play complete time travel sequence", settings.ettLong, 1, "title='If unchecked, the short \"re-entry\" sequence is played' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif

#ifdef EXTERNAL_TIMETRAVEL_OUT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useETTO("uEtto", "Use compatible external props (0=no, 1=yes)", settings.useETTO, 1, "autocomplete='off' title='Enable to use compatible external props to be part of the time travel sequence, eg. FluxCapacitor, SID, etc.'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useETTO("uEtto", "Use compatible external props", settings.useETTO, 1, "autocomplete='off' title='Check to use compatible external props to be part of the time travel sequence, eg. Flux Capacitor, SID, etc.' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif // EXTERNAL_TIMETRAVEL_OUT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds (0=no, 1=yes)", settings.playTTsnds, 1, "autocomplete='off' title='Enable to have the device play time travel sounds. Disable if other props provide time travel sound.'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds", settings.playTTsnds, 1, "autocomplete='off' title='Check to have the device play time travel sounds. Uncheck if other props provide time travel sound.' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

WiFiManagerParameter custom_musHint("<div style='margin:0px;padding:0px'>MusicPlayer</div>");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle at startup (0=no, 1=yes)", settings.shuffle, 1, "autocomplete='off' title='Enable to shuffle playlist at startup'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle at startup", settings.shuffle, 1, "title='Check to shuffle playlist at startup' type='checkbox' style='margin-top:8px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save alarm/volume on SD (0=no, 1=yes)<br><span style='font-size:80%'>Enable this if you often change alarm or volume settings to avoid flash wear</span>", settings.CfgOnSD, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save alarm/volume settings on SD<br><span style='font-size:80%'>Check this if you often change alarm or volume settings to avoid flash wear</span>", settings.CfgOnSD, 1, "autocomplete='off' type='checkbox' style='margin-top:5px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_sdFrq("sdFrq", "SD clock speed (0=16Mhz, 1=4Mhz)<br><span style='font-size:80%'>Slower access might help in case of problems with SD cards</span>", settings.sdFreq, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span style='font-size:80%'>Checking this might help in case of SD card problems</span>", settings.sdFreq, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef CP_AUDIO_INSTALLER
WiFiManagerParameter custom_copyAudio("cpAu", "Audio file installation: Write COPY here to copy the original audio files from the SD card to the internal storage.", settings.copyAudio, 6, "autocomplete='off'");
WiFiManagerParameter custom_copyHint("<div style='margin:0px;padding:0px;font-size:80%'>If display shows 'ERROR' when finished, write FORMAT instead of COPY on second attempt.</div>");
#endif

WiFiManagerParameter custom_footer("<p></p>");
WiFiManagerParameter custom_sectstart("<div class='sects'>");
WiFiManagerParameter custom_sectend("</div>");

#define TC_MENUSIZE 7
static const char* wifiMenu[TC_MENUSIZE] = {"wifi", "param", "sep", "restart", "update", "sep", "custom" };

static const char* myHead = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAA9QTFRFjpCRzMvH9tgx8iU9Q7YkHP8yywAAAC1JREFUeNpiYEQDDIwMKAAkwIwEiBTAMIMFCRApgGEGExIgUgDDDHQBNAAQYADhYgGBZLgAtAAAAABJRU5ErkJggg=='><script>function getn(x){return document.getElementsByTagName(x)}function ge(x){return document.getElementById(x)}function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;p=l.nextElementSibling.classList.contains('l');ge('p').disabled=!p;if(p){ge('p').placeholder='';ge('p').focus();}}window.onload=function(){document.title='Time Circuits';if(ge('s')&&ge('dns')){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].innerHTML='<img id=\"tcgfx\" class=\"tcgfx\" src=\"\">' + aa[0].innerHTML;}aa=ge('s').parentElement;bb=aa.innerHTML;dd=bb.search('<hr>');ee=bb.search('<button');cc='<div class=\"sects\">'+bb.substring(0,dd)+'</div><div class=\"sects\">'+bb.substring(dd+4,ee)+'</div>'+bb.substring(ee);aa.innerHTML=cc;document.querySelectorAll('a[href=\"#p\"]').forEach((userItem)=>{userItem.onclick=function(){c(this);return false;}});if(aa=ge('s')){aa.oninput=function(){if(this.placeholder.length>0&&this.value.length==0){ge('p').placeholder='********';}}}} if(ge('uploadbin')||window.location.pathname=='/u'||window.location.pathname=='/wifisave'){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].innerHTML='<img id=\"tcgfx\" class=\"tcgfx\" src=\"\">'+aa[0].innerHTML;if((bb=ge('uploadbin'))){aa[0].style.textAlign='center';bb.parentElement.onsubmit=function(){aa=document.getElementById('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}}}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}} if(ge('ebnew')){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm');bb=getn('H3');aa=getn('H1');ff=aa[0].parentNode;ff.style.position='relative';dd.innerHTML='<div class=\"tpm2\"><img src=\"data:image/png;base64,'+(zz?'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAbUlEQVR42tzXwRGAQAwDMdF/09QQQ24MLkDj77oeTiPA1wFGQiHATOgDGAp1AFOhDWAslAHMhS6AQKgCSIQmgEgoAsiEHoBQqAFIhRaAWCgByIVXAMuAdcA6YBlwALAKePzgd71QAByP71uAAQC+xwvdcFg7UwAAAABJRU5ErkJggg==':'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAgElEQVR42tzXQQqDABAEwcr/P50P2BBUdMhee6j7+lw8i4BCD8MiQAjHYRAghAh7ADWMMAcQww5jADHMsAYQwwxrADHMsAYQwwxrADHMsAYQwwxrgLgOPwKeAjgrrACcFkYAzgu3AN4C3AV4D3AP4E3AHcDF+8d/YQB4/Pn+CjAAMaIIJuYVQ04AAAAASUVORK5CYII=')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.2em\"':'')+'>'+aa[0].innerHTML+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:4.5em\"':'')+'>'+bb[0].innerHTML+'</div>';bb[0].remove();aa[0].replaceWith(dd);} if((aa=ge('tcgfx'))){aa.src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAS0AAACDCAMAAADvXL32AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAYBQTFRFNKLchsjq/7oEZrnl/6IKrWVXbaN5U5eQ+/3+9Pr9W3qR/1Qd/5oMKZ3a/5MN/7QFj2tt/3wT/4MRprBK/2EZ/1sbW7TjtN3y/3QVzpMzA4vU4/L6tZNGfMPo6vX7mK1W/04etbU+6Wooq9jx/20X9FMmEpLW5sMV/40P13A0yV5DwuP0mtHu/64Hkc3s/6kI/8UBQajehJBs11k6/8EChKlm0bsn2u747FQr/4gQ3osp6bgV/8oAybototTv6KkaS6zg16cn8cYLu+Dz/2kYCI7UOoCp1uz49ZYU/8gAyeb2DZDV0ur3cb7mnY9azej2w6I441Yyv7k13sEb6Zkdul9PF5XX7cUP+lAiG5fYdnR9+cgF770Q3rUe8KQV3vD58XQf+XMZIJnZzawuUa/h5IEm83sc/ckC9o8V9sgI96wO+bsJ+8AG84kZ+8QEoGZh1b8j/bEI/74D82ch9oMZ97QL4Z4i+aIP7/j8+skEvntFoXpb/2YY98YH978J////jLnAmQAAAIB0Uk5T/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wA4BUtnAAAi20lEQVR42u2di0PT1vfA02VAUpfJhmskIUWslEGzboFBEaVWs1BNGwoikaKuM9Yx9viCsq9G3Hf513/n3ryTG6i4/XSPs7lh6SP95LzuveeeSxXlqlor2qzzr5wulLrW4pRGv2DkSmpFb4o28y+UbFptbltAorXKFkcpfUPKySW9JhZt5l9uKVpOSZu+tAMyvby5sICpmRalNAp0rl7iK22RZf7FFtJijdFLoexMTy8Dtrntg9FWlwNlK0hVuQ7Uev9qG6LlNJXtSySZBl1D1HZ3hTKnFGgjV1c7NdFm/7nQgJZTKi9fOlF2lhe2hwBbfk0DbjR4Nr7G/FNpMTFbPJXbAYoJJnJtkqw2e6L9z6LltKm5S28kOzvLQRhFcTRXrfO6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'}}</script><style type='text/css'>body{font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}div.msg{border:1px solid #ccc;border-left-width:15px;border-radius:20px;background:linear-gradient(320deg,rgb(255,255,255) 0%,rgb(235,234,233) 100%);}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;color:#fff;background-color:#225a98;font-variant-caps:all-small-caps;}button.DD{color:#000;border:4px ridge #999;border-radius:2px;background:#e0c942;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAADBQTFRF////AAAAMyks8+AAuJYi3NHJo5aQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAbP19EwAAAAh0Uk5T/////////wDeg71ZAAAA4ElEQVR42qSTyxLDIAhF7yChS/7/bwtoFLRNF2UmRr0H8IF4/TBsY6JnQFvTJ8D0ncChb0QGlDvA+hkw/yC4xED2Z2L35xwDRSdqLZpFIOU3gM2ox6mA3tnDPa8UZf02v3q6gKRH/Eyg6JZBqRUCRW++yFYIvCjNFIt9OSC4hol/ItH1FkKRQgAbi0ty9f/F7LM6FimQacPbAdG5zZVlWdfvg+oEpl0Y+jzqIJZ++6fLqlmmnq7biZ4o67lgjBhA0kvJyTww/VK0hJr/LHvBru8PR7Dpx9MT0f8e72lvAQYALlAX+Kfw0REAAAAASUVORK5CYII=');background-repeat:no-repeat;background-origin:content-box;background-size:contain;}br{display:block;font-size:1px;content:''}input[type='checkbox']{display:inline-block;margin-top:10px}input{border:thin inset}small{display:none}em > small{display:inline}form{margin-block-end:0;}.tpm{border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;margin-left:2em;}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.sects{background-color:#eee;border-radius:7px;margin-bottom:20px;padding-bottom:7px;padding-top:7px}.tcgfx{display:block;margin:0px auto 10px auto;}</style>";
static const char* myCustMenu = "<form action='/erase' method='get' onsubmit='return confirm(\"This erases the WiFi config and reboots. The clock will restart in access point mode. Are you sure?\");'><button id='ebnew' class='DD'>Erase WiFi Config</button></form><br/><img style='display:block;margin:10px auto 10px auto;' src='data:image/png;base64,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'><div style='font-size:9px;margin-left:auto;margin-right:auto;text-align:center;'>Version " TC_VERSION " (" TC_VERSION_EXTRA ")<br>Powered by A10001986</div>";
// &#x26a0; = warning; &#9762; "radio-active" symbol not rendered properly in many browsers

static int  shouldSaveConfig = 0;
static bool shouldSaveIPConfig = false;
static bool shouldDeleteIPConfig = false;

// WiFi power management in AP mode
bool          wifiInAPMode = false;
bool          wifiAPIsOff = false;
unsigned long wifiAPModeNow;
unsigned long wifiAPOffDelay = 0;     // default: never

// WiFi power management in STA mode
bool          wifiIsOff = false;
unsigned long wifiOnNow = 0;
unsigned long wifiOffDelay     = 0;   // default: never
unsigned long origWiFiOffDelay = 0;

static void wifiConnect(bool deferConfigPortal = false);
static void saveParamsCallback();
static void saveConfigCallback();
static void preUpdateCallback();
static void preSaveConfigCallback();
static void waitConnectCallback();

static void setupStaticIP();
static bool isIp(char *str);
static void ipToString(char *str, IPAddress ip);
static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
#ifndef TC_NOCHECKBOXES
static void strcpyCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);
#endif

/*
 * wifi_setup()
 *
 */
void wifi_setup()
{
    int temp;

    // Explicitly set mode, esp allegedly defaults to STA_AP
    WiFi.mode(WIFI_MODE_STA);

    #ifndef TC_DBG
    wm.setDebugOutput(false);
    #endif

    wm.setParamsPage(true);
    wm.setBreakAfterConfig(true);
    wm.setConfigPortalBlocking(false);
    wm.setPreSaveConfigCallback(preSaveConfigCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setPreOtaUpdateCallback(preUpdateCallback);
    wm.setHostname(settings.hostName);
    wm.setCaptivePortalEnable(false);
    
    // Our style-overrides, the page title
    wm.setCustomHeadElement(myHead);
    wm.setTitle(F("Time Circuits"));
    wm.setDarkMode(false);

    // Hack version number into WiFiManager main page
    wm.setCustomMenuHTML(myCustMenu);

    // Static IP info is not saved by WiFiManager,
    // have to do this "manually". Hence ipsettings.
    wm.setShowStaticFields(true);
    wm.setShowDnsFields(true);

    temp = (int)atoi(settings.wifiConTimeout);
    if(temp < 7) temp = 7;
    if(temp > 25) temp = 25;
    wm.setConnectTimeout(temp);

    temp = (int)atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    if(temp > 15) temp = 15;
    wm.setConnectRetries(temp);

    wm.setCleanConnect(true);
    //wm.setRemoveDuplicateAPs(false);

    wm.setMenu(wifiMenu, TC_MENUSIZE);

    wm.addParameter(&custom_headline);      // 1
    
    wm.addParameter(&custom_sectstart);     // 7
    wm.addParameter(&custom_ttrp);
    wm.addParameter(&custom_alarmRTC);
    wm.addParameter(&custom_playIntro);
    wm.addParameter(&custom_mode24);
    wm.addParameter(&custom_autoRotateTimes);
    wm.addParameter(&custom_sectend);
    
    wm.addParameter(&custom_sectstart);     // 8
    wm.addParameter(&custom_hostName);
    wm.addParameter(&custom_wifiConRetries);
    wm.addParameter(&custom_wifiConTimeout);
    wm.addParameter(&custom_wifiOffDelay);
    wm.addParameter(&custom_wifiAPOffDelay);
    wm.addParameter(&custom_wifiHint);
    wm.addParameter(&custom_sectend);
    
    wm.addParameter(&custom_sectstart);     // 5
    wm.addParameter(&custom_timeZone);
    wm.addParameter(&custom_ntpServer);
    #ifdef TC_HAVEGPS
    wm.addParameter(&custom_useGPS);
    #endif
    wm.addParameter(&custom_sectend);
    
    wm.addParameter(&custom_sectstart);     // 5
    wm.addParameter(&custom_destTimeBright);
    wm.addParameter(&custom_presTimeBright);
    wm.addParameter(&custom_lastTimeBright);
    wm.addParameter(&custom_sectend);
    
    wm.addParameter(&custom_sectstart);     // 11
    wm.addParameter(&custom_dtNmOff);
    wm.addParameter(&custom_ptNmOff);
    wm.addParameter(&custom_ltNmOff);
    wm.addParameter(&custom_autoNM);
    wm.addParameter(&custom_autoNMTimes);
    wm.addParameter(&custom_autoNMOn);
    wm.addParameter(&custom_autoNMOff);
    #ifdef TC_HAVELIGHT
    wm.addParameter(&custom_uLS);
    wm.addParameter(&custom_lxLim);
    #endif
    wm.addParameter(&custom_sectend);

    #ifdef TC_HAVETEMP
    wm.addParameter(&custom_sectstart);     // 5
    wm.addParameter(&custom_useTemp);
    wm.addParameter(&custom_tempUnit);
    wm.addParameter(&custom_tempOffs);
    wm.addParameter(&custom_sectend);
    #endif

    #ifdef TC_HAVESPEEDO
    wm.addParameter(&custom_sectstart);     // 9
    wm.addParameter(&custom_useSpeedo);
    wm.addParameter(&custom_speedoType);
    wm.addParameter(&custom_speedoBright);
    wm.addParameter(&custom_speedoFact);
    #ifdef TC_HAVEGPS
    wm.addParameter(&custom_useGPSS);
    #endif
    #ifdef TC_HAVETEMP
    wm.addParameter(&custom_useDpTemp);
    wm.addParameter(&custom_tempBright);
    #endif
    wm.addParameter(&custom_sectend);
    #endif

    #ifdef FAKE_POWER_ON
    wm.addParameter(&custom_sectstart);     // 3
    wm.addParameter(&custom_fakePwrOn);
    wm.addParameter(&custom_sectend);
    #endif
    
    #ifdef EXTERNAL_TIMETRAVEL_IN
    wm.addParameter(&custom_sectstart);     // 4
    wm.addParameter(&custom_ettDelay);
    wm.addParameter(&custom_ettLong);
    wm.addParameter(&custom_sectend);
    #endif
    
    wm.addParameter(&custom_sectstart);     // 4
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    wm.addParameter(&custom_useETTO);
    #endif
    wm.addParameter(&custom_playTTSnd);
    wm.addParameter(&custom_sectend);
    
    wm.addParameter(&custom_sectstart);     // 4
    wm.addParameter(&custom_musHint);
    wm.addParameter(&custom_shuffle);
    wm.addParameter(&custom_sectend);

    wm.addParameter(&custom_sectstart);     // 4
    wm.addParameter(&custom_CfgOnSD);
    wm.addParameter(&custom_sdFrq);
    wm.addParameter(&custom_sectend);

    #ifdef CP_AUDIO_INSTALLER
    if(check_allow_CPA()) {
        wm.addParameter(&custom_sectstart); // 4
        wm.addParameter(&custom_copyAudio);
        wm.addParameter(&custom_copyHint);
        wm.addParameter(&custom_sectend);
    }
    #endif
    
    wm.addParameter(&custom_footer);        // 1

    updateConfigPortalValues();

    #ifdef TC_MDNS
    if(MDNS.begin(settings.hostName)) {
        MDNS.addService("http", "tcp", 80);
    }
    #endif

    // Read settings for WiFi powersave countdown
    wifiOffDelay = (unsigned long)atoi(settings.wifiOffDelay);
    if(wifiOffDelay > 0 && wifiOffDelay < 10) wifiOffDelay = 10;
    origWiFiOffDelay = wifiOffDelay *= (60 * 1000);
    #ifdef TC_DBG
    Serial.printf("wifiOffDelay is %d\n", wifiOffDelay);
    #endif
    wifiAPOffDelay = (unsigned long)atoi(settings.wifiAPOffDelay);
    if(wifiAPOffDelay > 0 && wifiAPOffDelay < 10) wifiAPOffDelay = 10;
    wifiAPOffDelay *= (60 * 1000);

    // Configure static IP
    if(loadIpSettings()) {
        setupStaticIP();
    }

    // Connect, but defer starting the CP
    wifiConnect(true);
}

/*
 * wifi_loop()
 *
 */
void wifi_loop()
{
    char oldCfgOnSD = 0;
    #ifdef CP_AUDIO_INSTALLER
    bool forceCopyAudio = false;
    #endif
    
    wm.process();

    #ifdef CP_AUDIO_INSTALLER
    if(shouldSaveConfig > 1) {
        if(check_allow_CPA()) {
            if(!strcmp(custom_copyAudio.getValue(), "FORMAT")) {
                pwrNeedFullNow();
                mp_stop();
                stopAudio();
                allOff();
                destinationTime.resetBrightness();
                destinationTime.showTextDirect("FORMATTING");
                destinationTime.on();
                formatFlashFS();      // Format
                allOff();
                rewriteSecondarySettings();
                // all others written below
                forceCopyAudio = true;
            }
        }
    }
    #endif
    
    if(shouldSaveIPConfig) {

        #ifdef TC_DBG
        Serial.println(F("WiFi: Saving IP config"));
        #endif

        writeIpSettings();

        shouldSaveIPConfig = false;

    } else if(shouldDeleteIPConfig) {

        #ifdef TC_DBG
        Serial.println(F("WiFi: Deleting IP config"));
        #endif

        deleteIpSettings();

        shouldDeleteIPConfig = false;

    }

    if(shouldSaveConfig) {

        // Save settings and restart esp32

        #ifdef TC_DBG
        Serial.println(F("Config Portal: Saving config"));
        #endif

        // Only read parms if the user actually clicked SAVE on the params page
        if(shouldSaveConfig > 1) {

            getParam("rotate_times", settings.autoRotateTimes, 1);
            if(strlen(settings.autoRotateTimes) == 0) {
                sprintf(settings.autoRotateTimes, "%d", DEF_AUTOROTTIMES);
            }
            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(strlen(settings.hostName) == 0) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            mystrcpy(settings.wifiConTimeout, &custom_wifiConTimeout);
            mystrcpy(settings.wifiOffDelay, &custom_wifiOffDelay);
            mystrcpy(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);
            strcpytrim(settings.ntpServer, custom_ntpServer.getValue());
            strcpytrim(settings.timeZone, custom_timeZone.getValue());
            
            mystrcpy(settings.destTimeBright, &custom_destTimeBright);
            mystrcpy(settings.presTimeBright, &custom_presTimeBright);
            mystrcpy(settings.lastTimeBright, &custom_lastTimeBright);
            getParam("autonmtimes", settings.autoNMPreset, 1);
            if(strlen(settings.autoNMPreset) == 0) {
                sprintf(settings.autoNMPreset, "%d", DEF_AUTONM_PRESET);
            }
            mystrcpy(settings.autoNMOn, &custom_autoNMOn);
            mystrcpy(settings.autoNMOff, &custom_autoNMOff);
            #ifdef TC_HAVELIGHT
            mystrcpy(settings.luxLimit, &custom_lxLim);
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_IN
            mystrcpy(settings.ettDelay, &custom_ettDelay);
            #endif

            #ifdef TC_HAVETEMP
            mystrcpy(settings.tempOffs, &custom_tempOffs);
            #endif
            
            #ifdef TC_HAVESPEEDO
            getParam("speedo_type", settings.speedoType, 2);
            if(strlen(settings.speedoType) == 0) {
                sprintf(settings.speedoType, "%d", DEF_SPEEDO_TYPE);
            }
            mystrcpy(settings.speedoBright, &custom_speedoBright);
            mystrcpy(settings.speedoFact, &custom_speedoFact);
            #ifdef TC_HAVETEMP
            mystrcpy(settings.tempBright, &custom_tempBright);
            #endif
            #endif
            
            #ifdef TC_NOCHECKBOXES // --------- Plain text boxes:

            mystrcpy(settings.timesPers, &custom_ttrp);
            mystrcpy(settings.alarmRTC, &custom_alarmRTC);
            mystrcpy(settings.playIntro, &custom_playIntro);
            mystrcpy(settings.mode24, &custom_mode24); 
                       
            #ifdef TC_HAVEGPS
            mystrcpy(settings.useGPS, &custom_useGPS);
            #endif
            
            mystrcpy(settings.autoNM, &custom_autoNM);
            mystrcpy(settings.dtNmOff, &custom_dtNmOff);
            mystrcpy(settings.ptNmOff, &custom_ptNmOff);
            mystrcpy(settings.ltNmOff, &custom_ltNmOff);
            #ifdef TC_HAVELIGHT
            mystrcpy(settings.useLight, &custom_uLS);
            #endif
            
            #ifdef TC_HAVETEMP
            mystrcpy(settings.useTemp, &custom_useTemp);
            mystrcpy(settings.tempUnit, &custom_tempUnit);
            #endif
            
            #ifdef TC_HAVESPEEDO
            mystrcpy(settings.useSpeedo, &custom_useSpeedo);
            #ifdef TC_HAVEGPS
            mystrcpy(settings.useGPSSpeed, &custom_useGPSS);
            #endif
            #ifdef TC_HAVETEMP
            mystrcpy(settings.dispTemp, &custom_useDpTemp);
            #endif
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_IN
            mystrcpy(settings.ettLong, &custom_ettLong);
            #endif
            #ifdef FAKE_POWER_ON
            mystrcpy(settings.fakePwrOn, &custom_fakePwrOn);
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            mystrcpy(settings.useETTO, &custom_useETTO);
            #endif
            mystrcpy(settings.playTTsnds, &custom_playTTSnd);

            mystrcpy(settings.shuffle, &custom_shuffle);

            oldCfgOnSD = settings.CfgOnSD[0];
            mystrcpy(settings.CfgOnSD, &custom_CfgOnSD);
            mystrcpy(settings.sdFreq, &custom_sdFrq);

            #else // -------------------------- Checkboxes:

            strcpyCB(settings.timesPers, &custom_ttrp);
            strcpyCB(settings.alarmRTC, &custom_alarmRTC);
            strcpyCB(settings.playIntro, &custom_playIntro);
            strcpyCB(settings.mode24, &custom_mode24);
            
            #ifdef TC_HAVEGPS
            strcpyCB(settings.useGPS, &custom_useGPS);
            #endif
            
            strcpyCB(settings.autoNM, &custom_autoNM);
            strcpyCB(settings.dtNmOff, &custom_dtNmOff);
            strcpyCB(settings.ptNmOff, &custom_ptNmOff);
            strcpyCB(settings.ltNmOff, &custom_ltNmOff);
            #ifdef TC_HAVELIGHT
            strcpyCB(settings.useLight, &custom_uLS);
            #endif
            
            #ifdef TC_HAVETEMP
            strcpyCB(settings.useTemp, &custom_useTemp);
            strcpyCB(settings.tempUnit, &custom_tempUnit);
            #endif
            
            #ifdef TC_HAVESPEEDO
            strcpyCB(settings.useSpeedo, &custom_useSpeedo);
            #ifdef TC_HAVEGPS
            strcpyCB(settings.useGPSSpeed, &custom_useGPSS);
            #endif
            #ifdef TC_HAVETEMP
            strcpyCB(settings.dispTemp, &custom_useDpTemp);
            #endif
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_IN
            strcpyCB(settings.ettLong, &custom_ettLong);
            #endif
            
            #ifdef FAKE_POWER_ON
            strcpyCB(settings.fakePwrOn, &custom_fakePwrOn);
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            strcpyCB(settings.useETTO, &custom_useETTO);
            #endif
            strcpyCB(settings.playTTsnds, &custom_playTTSnd);

            strcpyCB(settings.shuffle, &custom_shuffle);

            oldCfgOnSD = settings.CfgOnSD[0];
            strcpyCB(settings.CfgOnSD, &custom_CfgOnSD);
            strcpyCB(settings.sdFreq, &custom_sdFrq);

            #endif  // -------------------------

            // Copy alarm/volume settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                copySettings();
            }

        }

        // Write settings if requested, or no settings file exists
        if(shouldSaveConfig > 1 || !checkConfigExists()) {
            write_settings();
        }

        #ifdef CP_AUDIO_INSTALLER
        if(shouldSaveConfig > 1) {
            if(check_allow_CPA()) {
                if(forceCopyAudio || (!strcmp(custom_copyAudio.getValue(), "COPY"))) {
                    #ifdef TC_DBG
                    Serial.println(F("Config Portal: Copying audio files...."));
                    #endif
                    pwrNeedFullNow();
                    if(!copy_audio_files()) {
                       delay(3000);
                    }
                    delay(2000);
                }
            }
        }
        #endif
        
        shouldSaveConfig = 0;

        // Reset esp32 to load new settings

        allOff();
        #ifdef TC_HAVESPEEDO
        if(useSpeedo) speedo.off();
        #endif
        destinationTime.resetBrightness();
        destinationTime.showTextDirect("REBOOTING");
        destinationTime.on();

        #ifdef TC_DBG
        Serial.println(F("Config Portal: Restarting ESP...."));
        #endif

        Serial.flush();

        esp_restart();
    }

    // WiFi power management
    // If a delay > 0 is configured, WiFi is powered-down after timer has
    // run out. The timer starts when the device is powered-up/boots.
    // There are separate delays for AP mode and STA mode.
    // WiFi can be re-enabled for the configured time by holding '7'
    // on the keypad.
    // NTP requests will re-enable WiFi (in STA mode) for a short
    // while automatically.
    if(wifiInAPMode) {
        // Disable WiFi in AP mode after a configurable delay (if > 0)
        if(wifiAPOffDelay > 0) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                wifiOff();
                wifiAPIsOff = true;
                wifiIsOff = false;
                #ifdef TC_DBG
                Serial.println(F("WiFi (AP-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    } else {
        // Disable WiFi in STA mode after a configurable delay (if > 0)
        if(origWiFiOffDelay > 0) {
            if(!wifiIsOff && (millis() - wifiOnNow >= wifiOffDelay)) {
                wifiOff();
                wifiIsOff = true;
                wifiAPIsOff = false;
                #ifdef TC_DBG
                Serial.println(F("WiFi (STA-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    }

}

static void wifiConnect(bool deferConfigPortal)
{
    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(wm.autoConnect("TCD-AP")) {
        #ifdef TC_DBG
        Serial.println(F("WiFi connected"));
        #endif

        // Since WM 2.0.13beta, starting the CP invokes an async
        // WiFi scan. This interferes with network access for a 
        // few seconds after connecting. So, during boot, we start
        // the CP later, to allow a quick NTP update.
        if(!deferConfigPortal) {
            wm.startWebPortal();
        }

        // Allow modem sleep:
        // WIFI_PS_MIN_MODEM is the default, and activated when calling this
        // with "true". When this is enabled, received WiFi data can be
        // delayed for as long as the DTIM period.
        // Since it is the default setting, so no need to call it here.
        //WiFi.setSleep(true);

        wifiInAPMode = false;
        wifiIsOff = false;
        wifiOnNow = millis();
        wifiAPIsOff = false;  // Sic! Allows checks like if(wifiAPIsOff || wifiIsOff)

    } else {
        #ifdef TC_DBG
        Serial.println(F("Config portal running in AP-mode"));
        #endif

        wifiInAPMode = true;
        wifiAPIsOff = false;
        wifiAPModeNow = millis();
        wifiIsOff = false;    // Sic!

        {
            #ifdef TC_DBG
            int8_t power;
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power %d\n", power);
            #endif

            // Try to avoid "burning" the ESP when the WiFi mode
            // is "AP" and the vol knob is fully up by reducing
            // the max. transmit power.
            // The choices are:
            // WIFI_POWER_19_5dBm    = 19.5dBm
            // WIFI_POWER_19dBm      = 19dBm
            // WIFI_POWER_18_5dBm    = 18.5dBm
            // WIFI_POWER_17dBm      = 17dBm
            // WIFI_POWER_15dBm      = 15dBm
            // WIFI_POWER_13dBm      = 13dBm
            // WIFI_POWER_11dBm      = 11dBm
            // WIFI_POWER_8_5dBm     = 8.5dBm
            // WIFI_POWER_7dBm       = 7dBm     <-- proven to avoid the issues
            // WIFI_POWER_5dBm       = 5dBm
            // WIFI_POWER_2dBm       = 2dBm
            // WIFI_POWER_MINUS_1dBm = -1dBm
            WiFi.setTxPower(WIFI_POWER_7dBm);

            #ifdef TC_DBG
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power set to %d\n", power);
            #endif
        }
    }
}

void wifiOff()
{
    if( (!wifiInAPMode && wifiIsOff) ||
        (wifiInAPMode && wifiAPIsOff) ) {
        return;
    }

    wm.stopWebPortal();
    wm.disconnect();
    WiFi.mode(WIFI_OFF);
}

void wifiOn(unsigned long newDelay, bool alsoInAPMode, bool deferCP)
{
    unsigned long desiredDelay;
    unsigned long Now = millis();

    if(wifiInAPMode && !alsoInAPMode) return;

    if(wifiInAPMode) {
        if(wifiAPOffDelay == 0) return;   // If no delay set, auto-off is disabled
        wifiAPModeNow = Now;              // Otherwise: Restart timer
        if(!wifiAPIsOff) return;
    } else {
        if(origWiFiOffDelay == 0) return; // If no delay set, auto-off is disabled
        desiredDelay = (newDelay > 0) ? newDelay : origWiFiOffDelay;
        if((Now - wifiOnNow >= wifiOffDelay) ||                    // If delay has run out, or
           (wifiOffDelay - (Now - wifiOnNow))  < desiredDelay) {   // new delay exceeds remaining delay:
            wifiOffDelay = desiredDelay;                           // Set new timer delay, and
            wifiOnNow = Now;                                       // restart timer
            #ifdef TC_DBG
            Serial.printf("Restarting WiFi-off timer; delay %d\n", wifiOffDelay);
            #endif
        }
        if(!wifiIsOff) {
            // If WiFi is not off, check if user wanted
            // to start the CP, and do so, if not running
            if(!deferCP) {
                if(!wm.getWebPortalActive()) {
                    wm.startWebPortal();
                }
            }
            return;
        }
    }

    WiFi.mode(WIFI_MODE_STA);

    wifiConnect(deferCP);
}

// Check if WiFi is on; used to determine if a 
// longer interruption due to a re-connect is to
// be expected.
bool wifiIsOn()
{
    if(wifiInAPMode) {
        if(wifiAPOffDelay == 0) return true;
        if(!wifiAPIsOff) return true;
    } else {
        if(origWiFiOffDelay == 0) return true;
        if(!wifiIsOff) return true;
    }
    return false;
}

void wifiStartCP()
{
    if(wifiInAPMode || wifiIsOff)
        return;

    wm.startWebPortal();
}

// This is called when the WiFi config changes, so it has
// nothing to do with our settings here. Despite that,
// we write out our config file so that when the user initially
// configures WiFi, a default settings file exists upon reboot.
// Also, this triggers a reboot, so if the user entered static
// IP data, it becomes active after this reboot.
static void saveConfigCallback()
{
    shouldSaveConfig = 1;
}

// This is the callback from the actual Params page. In this
// case, we really read out the server parms and save them.
static void saveParamsCallback()
{
    shouldSaveConfig = 2;
}

// This is called before a firmware updated is initiated.
// Disable WiFi-off-timers.
static void preUpdateCallback()
{
    wifiAPOffDelay = 0;
    origWiFiOffDelay = 0;
}

// Grab static IP parameters from WiFiManager's server.
// Since there is no public method for this, we steal
// the html form parameters in this callback.
static void preSaveConfigCallback()
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef TC_DBG
    Serial.println("preSaveConfigCallback");
    #endif

    // clear as strncpy might leave us unterminated
    memset(ipBuf, 0, 20);
    memset(gwBuf, 0, 20);
    memset(snBuf, 0, 20);
    memset(dnsBuf, 0, 20);

    if(wm.server->arg(FPSTR(S_ip)) != "") {
        strncpy(ipBuf, wm.server->arg(FPSTR(S_ip)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_gw)) != "") {
        strncpy(gwBuf, wm.server->arg(FPSTR(S_gw)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_sn)) != "") {
        strncpy(snBuf, wm.server->arg(FPSTR(S_sn)).c_str(), 19);
    } else invalConf |= true;
    if(wm.server->arg(FPSTR(S_dns)) != "") {
        strncpy(dnsBuf, wm.server->arg(FPSTR(S_dns)).c_str(), 19);
    } else invalConf |= true;

    #ifdef TC_DBG
    if(strlen(ipBuf) > 0) {
        Serial.printf("IP:%s / SN:%s / GW:%s / DNS:%s\n", ipBuf, snBuf, gwBuf, dnsBuf);
    } else {
        Serial.println("Static IP unset, using DHCP");
    }
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef TC_DBG
        Serial.println("All IPs valid");
        #endif

        strcpy(ipsettings.ip, ipBuf);
        strcpy(ipsettings.gateway, gwBuf);
        strcpy(ipsettings.netmask, snBuf);
        strcpy(ipsettings.dns, dnsBuf);

        shouldSaveIPConfig = true;

    } else {

        #ifdef TC_DBG
        if(strlen(ipBuf) > 0) {
            Serial.println("Invalid IP");
        }
        #endif

        shouldDeleteIPConfig = true;

    }
}

static void setupStaticIP()
{
    IPAddress ip;
    IPAddress gw;
    IPAddress sn;
    IPAddress dns;

    if(strlen(ipsettings.ip) > 0 &&
        isIp(ipsettings.ip) &&
        isIp(ipsettings.gateway) &&
        isIp(ipsettings.netmask) &&
        isIp(ipsettings.dns)) {

        ip = stringToIp(ipsettings.ip);
        gw = stringToIp(ipsettings.gateway);
        sn = stringToIp(ipsettings.netmask);
        dns = stringToIp(ipsettings.dns);

        wm.setSTAStaticIPConfig(ip, gw, sn, dns);
    }
}

void updateConfigPortalValues()
{
    const char custHTMLSel[] = " selected";
    int t = atoi(settings.autoRotateTimes);
    int tnm = atoi(settings.autoNMPreset);
    #ifdef TC_HAVESPEEDO
    int tt = atoi(settings.speedoType);
    char spTyBuf[8];
    #endif

    // Make sure the settings form has the correct values

    strcpy(aintCustHTML, aintCustHTML1);
    strcat(aintCustHTML, settings.autoRotateTimes);
    strcat(aintCustHTML, aintCustHTML2);
    if(t == 0) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML3);
    if(t == 1) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML4);
    if(t == 2) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML5);
    if(t == 3) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML6);
    if(t == 4) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML7);
    if(t == 5) strcat(aintCustHTML, custHTMLSel);
    strcat(aintCustHTML, aintCustHTML8);

    custom_hostName.setValue(settings.hostName, 31);
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 2);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 2);
    custom_wifiOffDelay.setValue(settings.wifiOffDelay, 2);
    custom_wifiAPOffDelay.setValue(settings.wifiAPOffDelay, 2);
    custom_ntpServer.setValue(settings.ntpServer, 63);
    custom_timeZone.setValue(settings.timeZone, 63);

    custom_destTimeBright.setValue(settings.destTimeBright, 2);
    custom_presTimeBright.setValue(settings.presTimeBright, 2);
    custom_lastTimeBright.setValue(settings.lastTimeBright, 2);

    strcpy(anmCustHTML, anmCustHTML1);
    strcat(anmCustHTML, settings.autoNMPreset);
    strcat(anmCustHTML, anmCustHTML2);
    if(tnm == 0) strcat(anmCustHTML, custHTMLSel);
    strcat(anmCustHTML, anmCustHTML3);
    if(tnm == 1) strcat(anmCustHTML, custHTMLSel);
    strcat(anmCustHTML, anmCustHTML4);
    if(tnm == 2) strcat(anmCustHTML, custHTMLSel);
    strcat(anmCustHTML, anmCustHTML5);
    if(tnm == 3) strcat(anmCustHTML, custHTMLSel);
    strcat(anmCustHTML, anmCustHTML6);
    if(tnm == 4) strcat(anmCustHTML, custHTMLSel);
    strcat(anmCustHTML, anmCustHTML7);

    custom_autoNMOn.setValue(settings.autoNMOn, 2);
    custom_autoNMOff.setValue(settings.autoNMOff, 2);
    #ifdef TC_HAVELIGHT
    custom_lxLim.setValue(settings.luxLimit, 6);
    #endif
    
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettDelay.setValue(settings.ettDelay, 5);
    #endif

    #ifdef TC_HAVETEMP
    custom_tempOffs.setValue(settings.tempOffs, 4);
    #endif

    #ifdef TC_HAVESPEEDO
    strcpy(spTyCustHTML, spTyCustHTML1);
    strcat(spTyCustHTML, settings.speedoType);
    strcat(spTyCustHTML, spTyCustHTML2);
    for (int i = SP_MIN_TYPE; i < SP_NUM_TYPES; i++) {
        strcat(spTyCustHTML, spTyOptP1);
        sprintf(spTyBuf, "%d'", i);
        strcat(spTyCustHTML, spTyBuf);
        if(tt == i) strcat(spTyCustHTML, custHTMLSel);
        strcat(spTyCustHTML, ">");
        strcat(spTyCustHTML, dispTypeNames[i]);
        strcat(spTyCustHTML, spTyOptP3);
    }
    strcat(spTyCustHTML, spTyCustHTMLE);
    custom_speedoBright.setValue(settings.speedoBright, 2);
    custom_speedoFact.setValue(settings.speedoFact, 3);
    #ifdef TC_HAVETEMP
    custom_tempBright.setValue(settings.tempBright, 2);
    #endif
    #endif

    #ifdef TC_NOCHECKBOXES  // Standard text boxes: -------

    custom_ttrp.setValue(settings.timesPers, 1);
    custom_alarmRTC.setValue(settings.alarmRTC, 1);
    custom_playIntro.setValue(settings.playIntro, 1);
    custom_mode24.setValue(settings.mode24, 1);
    #ifdef TC_HAVEGPS
    custom_useGPS.setValue(settings.useGPS, 1);
    #endif
    custom_autoNM.setValue(settings.autoNM, 1);
    custom_dtNmOff.setValue(settings.dtNmOff, 1);
    custom_ptNmOff.setValue(settings.ptNmOff, 1);
    custom_ltNmOff.setValue(settings.ltNmOff, 1);
    #ifdef TC_HAVELIGHT
    custom_uLS.setValue(settings.useLight, 1);
    #endif
    #ifdef TC_HAVETEMP
    custom_useTemp.setValue(settings.useTemp, 1);
    custom_tempUnit.setValue(settings.tempUnit, 1);
    #endif
    #ifdef TC_HAVESPEEDO
    custom_useSpeedo.setValue(settings.useSpeedo, 1);
    #ifdef TC_HAVEGPS
    custom_useGPSS.setValue(settings.useGPSSpeed, 1);
    #endif
    #ifdef TC_HAVETEMP
    custom_useDpTemp.setValue(settings.dispTemp, 1);
    #endif
    #endif
    #ifdef FAKE_POWER_ON
    custom_fakePwrOn.setValue(settings.fakePwrOn, 1);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettLong.setValue(settings.ettLong, 1);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    custom_useETTO.setValue(settings.useETTO, 1);
    #endif
    custom_playTTSnd.setValue(settings.playTTsnds, 1);
    custom_shuffle.setValue(settings.shuffle, 1);
    custom_CfgOnSD.setValue(settings.CfgOnSD, 1);
    custom_sdFrq.setValue(settings.sdFreq, 1);

    #else   // For checkbox hack --------------------------

    setCBVal(&custom_ttrp, settings.timesPers);
    setCBVal(&custom_alarmRTC, settings.alarmRTC);
    setCBVal(&custom_playIntro, settings.playIntro);
    setCBVal(&custom_mode24, settings.mode24);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPS, settings.useGPS);
    #endif
    setCBVal(&custom_autoNM, settings.autoNM);
    setCBVal(&custom_dtNmOff, settings.dtNmOff);
    setCBVal(&custom_ptNmOff, settings.ptNmOff);
    setCBVal(&custom_ltNmOff, settings.ltNmOff);
    #ifdef TC_HAVELIGHT
    setCBVal(&custom_uLS, settings.useLight);
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useTemp, settings.useTemp);
    setCBVal(&custom_tempUnit, settings.tempUnit);
    #endif
    #ifdef TC_HAVESPEEDO
    setCBVal(&custom_useSpeedo, settings.useSpeedo);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPSS, settings.useGPSSpeed);
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useDpTemp, settings.dispTemp);
    #endif
    #endif
    #ifdef FAKE_POWER_ON
    setCBVal(&custom_fakePwrOn, settings.fakePwrOn);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_IN
    setCBVal(&custom_ettLong, settings.ettLong);
    #endif
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    setCBVal(&custom_useETTO, settings.useETTO);
    #endif
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    setCBVal(&custom_shuffle, settings.shuffle);
    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    setCBVal(&custom_sdFrq, settings.sdFreq);

    #endif // ---------------------------------------------    

    #ifdef CP_AUDIO_INSTALLER
    custom_copyAudio.setValue("", 6);   // Always clear
    #endif
}

int wifi_getStatus()
{
    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          return (int)WiFi.status();
      case WIFI_MODE_AP:
          return 0x10000;     // AP MODE
      case WIFI_MODE_NULL:
          return 0x10001;     // OFF
    }

    return 0x10002;           // UNKNOWN
}

bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
{
    IPAddress myip;

    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          myip = WiFi.localIP();
          break;
      case WIFI_MODE_AP:
          myip = WiFi.softAPIP();
          break;
      default:
          a = b = c = d = 0;
          return true;
    }

    a = myip[0];
    b = myip[1];
    c = myip[2];
    d = myip[3];

    return true;
}

void wifi_getMAC(char *buf)
{
    byte myMac[6];
    
    WiFi.macAddress(myMac);
    sprintf(buf, "%02x%02x%02x%02x%02x%02x", myMac[0], myMac[1], myMac[2], myMac[3], myMac[4], myMac[5]); 
}

// Check if String is a valid IP address
static bool isIp(char *str)
{
    int segs = 0;
    int digcnt = 0;
    int num = 0;

    while(*str != '\0') {

        if(*str == '.') {

            if(!digcnt || (++segs == 4))
                return false;

            num = digcnt = 0;
            str++;
            continue;

        } else if((*str < '0') || (*str > '9')) {

            return false;

        }

        if((num = (num * 10) + (*str - '0')) > 255)
            return false;

        digcnt++;
        str++;
    }

    return true;
}

// IPAddress to string
static void ipToString(char *str, IPAddress ip)
{
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

// String to IPAddress
static IPAddress stringToIp(char *str)
{
    int ip1, ip2, ip3, ip4;

    sscanf(str, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

    return IPAddress(ip1, ip2, ip3, ip4);
}

/*
 * Read parameter from server, for customhmtl input
 */
static void getParam(String name, char *destBuf, size_t length)
{
    memset(destBuf, 0, length+1);
    if(wm.server->hasArg(name)) {
        strncpy(destBuf, wm.server->arg(name).c_str(), length);
    }
}

static bool myisspace(char mychar)
{
    return (mychar == ' ' || mychar == '\n' || mychar == '\t' || mychar == '\v' || mychar == '\f' || mychar == '\r');
}

static bool myisgoodchar(char mychar)
{
    return ((mychar >= '0' && mychar <= '9') || (mychar >= 'a' && mychar <= 'z') || (mychar >= 'A' && mychar <= 'Z') || mychar == '-');
}

static char* strcpytrim(char* destination, const char* source, bool doFilter)
{
    char *ret = destination;
    
    do {
        if(!myisspace(*source) && (!doFilter || myisgoodchar(*source))) *destination++ = *source;
        source++;
    } while(*source);
    
    *destination = 0;
    
    return ret;
}

static void mystrcpy(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, el->getValue());
}

#ifndef TC_NOCHECKBOXES
static void strcpyCB(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, ((int)atoi(el->getValue()) > 0) ? "1" : "0");
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{
    const char makeCheck[] = "1' checked a='";
    
    el->setValue(((int)atoi(sv) > 0) ? makeCheck : "1", 14);
}
#endif
