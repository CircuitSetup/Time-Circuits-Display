/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * WiFi and Config Portal handling
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

#include "tc_global.h"

#include <Arduino.h>

#ifdef TC_MDNS
#include <ESPmDNS.h>
#endif

#include "src/WiFiManager/WiFiManager.h"

#include "clockdisplay.h"
#include "tc_menus.h"
#include "tc_time.h"
#include "tc_audio.h"
#include "tc_settings.h"
#include "tc_wifi.h"
#include "tc_keypad.h"
#ifdef TC_HAVEMQTT
#include "mqtt.h"
#endif

// If undefined, use the checkbox/dropdown-hacks.
// If defined, go back to standard text boxes
//#define TC_NOCHECKBOXES

#define STRLEN(x) (sizeof(x)-1)

Settings settings;

IPSettings ipsettings;

WiFiManager wm;

#ifdef TC_HAVEMQTT
WiFiClient mqttWClient;
PubSubClient mqttClient(mqttWClient);
#endif

static const char R_updateacdone[] = "/uac";

static const char acul_part1[]  = "<!DOCTYPE html><html lang='en'><head><meta name='format-detection' content='telephone=no'><meta charset='UTF-8'><meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/><title>";
static const char acul_part2[]  = "</title><style>.c,body{text-align:center;font-family:verdana}div{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}.msg{border-radius:.3rem;width: 100%}.wrap {text-align:left;display:inline-block;min-width:260px;max-width:500px}.msg{padding:20px;margin:20px 0;border:1px solid #eee;border-left-width:5px;border-left-color:#777}.msg h4{margin-top:0;margin-bottom:5px}.msg.P{border-left-color:#1fa3ec}.msg.P h4{color:#1fa3ec}.msg.D{border-left-color:#dc3630}.msg.D h4{color:#dc3630}.msg.S{border-left-color: #5cb85c}.msg.S h4{color: #5cb85c}dt{font-weight:bold}dd{margin:0;padding:0 0 0.5em 0;min-height:12px}td{vertical-align: top;}</style>";
static const char acul_part3[]  = "</head><body class='{c}'>";
static const char acul_part4[]  = "<div class='wrap'><h1>";
static const char acul_part5[]  = "</h1><h3>";
static const char acul_part6[]  = "</h3><div class='msg";
static const char acul_part7[]  = " S'><strong>Upload successful.</strong><br>Installation will proceed...";
static const char acul_part71[] = " D'><strong>Upload failed.</strong><br>";
static const char *acul_errs[]  = { "Can't open file on SD", "No SD card found", "Write error", "Aborted", "Bad file" };
static const char acul_part8[]  = "</div></div></body></html>";

static const char *osde = "</option></select></div>";
static const char *ooe  = "</option><option value='";

static char beepaintCustHTML[820] = "";
static const char *beepCustHTMLSrc[6] = {
    "<div class='cmp0'><label for='beepmode'>Power-up beep mode</label><select class='sel0' value='",
    "beepmode",
    ">Off%s1'",
    ">On%s2'",
    ">Auto (30 secs)%s3'",
    ">Auto (60 secs)%s"
};
static const char *aintCustHTMLSrc[8] = {
    "<div class='cmp0'><label for='rot_int'>Time-cycling interval</label><select class='sel0' value='",
    "rot_int",
    ">Off%s1'",
    ">5 minutes%s2'",     // </option><option value='
    ">10 minutes%s3'",
    ">15 minutes%s4'",
    ">30 minutes%s5'",
    ">60 minutes%s"       // </option></select></div>
};

static char anmCustHTML[512] = "";
static const char anmCustHTML1[] = "<div class='cmp0'><label for='anmtim'>Schedule</label><select class='sel0' value='%s' name='anmtim' id='anmtim' autocomplete='off'><option value='10'%s>&#10060; Off%s0'";
static const char *anmCustHTMLSrc[5] = {
    "%s>&#128337; Daily, set hours below%s1'",
    "%s>&#127968; M-T:17-23/F:13-1/S:9-1/Su:9-23%s2'",
    "%s>&#127970; M-F:9-17%s3'",
    "%s>&#127970; M-T:7-17/F:7-14%s4'",
    "%s>&#128722; M-W:8-20/T-F:8-21/S:8-17%s"
};

#ifdef TC_HAVESPEEDO
static char spTyCustHTML[800] = "";
static const char spTyCustHTML1[] = "<div class='cmp0'><label for='spty'>Speedo display type</label><select class='sel0' value='";
static const char spTyCustHTML2[] = "' name='spty' id='spty' autocomplete='off'>";
static const char spTyCustHTMLE[] = "</select></div>";
static const char spTyOptP1[] = "<option value='";
static const char spTyOptP2[] = "'>";
static const char spTyOptP3[] = "</option>";
static const char *dispTypeNames[SP_NUM_TYPES] = {
  "CircuitSetup.us",
  "Adafruit 878 (4x7)",
  "Adafruit 878 (4x7;left)",
  "Adafruit 1270 (4x7)",
  "Adafruit 1270 (4x7;left)",
  "Adafruit 1911 (4x14)",
  "Adafruit 1911 (4x14;left)",
  "Grove 0.54\" 2x14",
  "Grove 0.54\" 4x14",
  "Grove 0.54\" 4x14 (left)"
#ifndef TWPRIVATE
  ,"Ada 1911 (left tube)"
  ,"Ada 878 (left tube)"
#else
  ,"A10001986 wallclock"
  ,"A10001986 speedo replica"
#endif
};
#ifdef TC_HAVEGPS
static char spdRateCustHTML[300] = "";
static const char *spdRateCustHTMLSrc[6] = 
{
  "<div class='cmp0' style='margin-left:20px'><label for='spdrt'>Update rate</label><select class='sel0' value='",
  "spdrt",
  ">1Hz%s1'",
  ">2Hz%s2'",
  ">4Hz%s3'",
  ">5Hz%s"
};
#endif
#endif

static const char custHTMLSel[] = " selected";
static const char *aco = "autocomplete='off'";
static const char *tznp1 = "City/location name [a-z/0-9/-/ ]";

WiFiManagerParameter custom_aood("<div class='msg P'>Please <a href='/update'>install/update</a> audio data</div>");

WiFiManagerParameter custom_tzsel("<datalist id='tz'><option value='PST8PDT,M3.2.0,M11.1.0'>Pacific</option><option value='MST7MDT,M3.2.0,M11.1.0'>Mountain</option><option value='CST6CDT,M3.2.0,M11.1.0'>Central</option><option value='EST5EDT,M3.2.0,M11.1.0'>Eastern</option><option value='GMT0BST,M3.5.0/1,M10.5.0'>Western European</option><option value='CET-1CEST,M3.5.0,M10.5.0/3'>Central European</option><option value='EET-2EEST,M3.5.0/3,M10.5.0/4'>Eastern European</option><option value='MSK-3'>Moscow</option><option value='AWST-8'>Australia Western</option><option value='ACST-9:30'>Australia Central/NT</option><option value='ACST-9:30ACDT,M10.1.0,M4.1.0/3'>Australia Central/SA</option><option value='AEST-10AEDT,M10.1.0,M4.1.0/3'>Australia Eastern VIC/NSW</option><option value='AEST-10'>Australia Eastern QL</option></datalist>");

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_playIntro("plIn", "Play intro (0=off, 1=on)", settings.playIntro, 1, aco);
WiFiManagerParameter custom_mode24("md24", "Clock mode (0=12hr, 1=24hr)", settings.mode24, 1, aco);
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_playIntro("plIn", "Play intro", settings.playIntro, 1, "type='checkbox' style='margin-top:3px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mode24("md24", "24-hour clock", settings.mode24, 1, "type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_beep_aint(beepaintCustHTML);  // beep + aint
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is RTC (1) or displayed \"present\" time (0)", settings.alarmRTC, 1, aco);
#ifndef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent (0=no, 1=yes)", settings.timesPers, 1, "autocomplete='off'");
#endif
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is real present time", settings.alarmRTC, 1, "title='If unchecked, alarm base is the displayed \"present\" time' type='checkbox'", WFM_LABEL_AFTER);
#ifndef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent", settings.timesPers, 1, "type='checkbox'", WFM_LABEL_AFTER);
#endif
#endif // -------------------------------------------------

#if defined(TC_MDNS) || defined(TC_WM_HAS_MDNS)
#define HNTEXT "Hostname<br><span style='font-size:80%'>The Config Portal is accessible at http://<i>hostname</i>.local<br>[Valid characters: a-z/0-9/-]</span>"
#else
#define HNTEXT "Hostname<br><span style='font-size:80%'>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9\\-]+' placeholder='Example: timecircuits'");
WiFiManagerParameter custom_sysID("sysID", "AP Mode: Network name appendix<br><span style='font-size:80%'>Will be appended to \"TCD-AP\" [a-z/0-9/-]</span>", settings.systemID, 7, "pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_appw("appw", "AP Mode: WiFi password<br><span style='font-size:80%'>Password to protect TCD-AP. Empty or 8 characters [a-z/0-9/-]<br><b>Write this down, you might lock yourself out!</b></span>", settings.appw, 8, "minlength='8' pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_wifiConRetries("wifiret", "WiFi connection attempts (1-10)", settings.wifiConRetries, 2, "type='number' min='1' max='10' autocomplete='off'", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_wifiConTimeout("wificon", "WiFi connection timeout (7-25[seconds])", settings.wifiConTimeout, 2, "type='number' min='7' max='25'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_wifiPRe("wifiPRet", "Periodic reconnection attempts (0=no, 1=yes)", settings.wifiPRetry, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_wifiPRe("wifiPRet", "Periodic reconnection attempts ", settings.wifiPRetry, 1, "autocomplete='off' type='checkbox' style='margin:5px 0 10px 0'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_wifiOffDelay("wifioff", "<br>WiFi power save timer<br>(10-99[minutes];0=off)", settings.wifiOffDelay, 2, "type='number' min='0' max='99' title='WiFi will be shut down after chosen number of minutes after power-on. 0 means never.'");
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "WiFi power save timer for AP-mode<br>(10-99[minutes];0=off)", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='WiFi-AP will be shut down after chosen number of minutes after power-on. 0 means never.'");

WiFiManagerParameter custom_timeZone("time_zone", "Time zone (in <a href='https://tz.out-a-ti.me' target=_blank>Posix</a> format)", settings.timeZone, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_ntpServer("ntp_server", "NTP server", settings.ntpServer, 63, "pattern='[a-zA-Z0-9\\.\\-]+' placeholder='Example: pool.ntp.org'");

WiFiManagerParameter custom_timeZone1("time_zone1", "Time zone for Destination Time display", settings.timeZoneDest, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_timeZone2("time_zone2", "Time zone for Last Time Dep. display", settings.timeZoneDep, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_timeZoneN1("time_zonen1", tznp1, settings.timeZoneNDest, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO' style='margin-bottom:15px'");
WiFiManagerParameter custom_timeZoneN2("time_zonen2", tznp1, settings.timeZoneNDep, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO'");

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time (0=dimmed, 1=off)", settings.dtNmOff, 1, aco);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time (0=dimmed, 1=off)", settings.ptNmOff, 1, aco);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. (0=dimmed, 1=off)", settings.ltNmOff, 1, aco);
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time off", settings.dtNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time off", settings.ptNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. off", settings.ltNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_autoNMTimes(anmCustHTML);
WiFiManagerParameter custom_autoNMOn("anmon", "Daily night-mode start hour (0-23)", settings.autoNMOn, 2, "type='number' min='0' max='23' title='Hour to switch on night-mode'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Daily night-mode end hour (0-23)", settings.autoNMOff, 2, "type='number' min='0' max='23' autocomplete='off' title='Hour to switch off night-mode'");
#ifdef TC_HAVELIGHT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_uLS("uLS", "Use light sensor (0=no, 1=yes)", settings.useLight, 1,  "title='If enabled, TCD will be put in night-mode if lux level is below or equal threshold.' autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_uLS("uLS", "Use light sensor", settings.useLight, 1, "title='If checked, TCD will be put in night-mode if lux level is below or equal threshold.' type='checkbox' style='margin-top:14px'", WFM_LABEL_AFTER);
#endif
WiFiManagerParameter custom_lxLim("lxLim", "<br>Lux threshold (0-50000)", settings.luxLimit, 6, "type='number' min='0' max='50000' autocomplete='off'", WFM_LABEL_BEFORE);
#endif

#ifdef TC_HAVETEMP
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_tempUnit("uTem", "Temperature unit (0=°F, 1=°C)", settings.tempUnit, 1, "autocomplete='off' title='Select unit for temperature'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_tempUnit("temUnt", "Show temperature in Celsius", settings.tempUnit, 1, "title='Fahrenheit if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_tempOffs("tOffs", "<br>Temperature offset (-3.0-3.0)", settings.tempOffs, 4, "type='number' min='-3.0' max='3.0' step='0.1' title='Correction value to add to temperature' autocomplete='off'");
#endif // TC_HAVETEMP

#ifdef TC_HAVESPEEDO
WiFiManagerParameter custom_speedoType(spTyCustHTML);
WiFiManagerParameter custom_speedoBright("speBri", "<br>Speed brightness (0-15)", settings.speedoBright, 2, "type='number' min='0' max='15' autocomplete='off'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_sAF("sAF", "Acceleration times (0=real, 1=movie)", settings.speedoAF, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_sAF("sAF", "Real-life acceleration figures", settings.speedoAF, 1, "autocomplete='off' title='If unchecked, movie-like times are used' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_speedoFact("speFac", "<br>Factor for real-life figures (0.5-5.0)", settings.speedoFact, 3, "type='number' min='0.5' max='5.0' step='0.5' title='1.0 means real-world DMC-12 acceleration time.' autocomplete='off'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_sL0("sL0", "Display speed with leading 0 (0=no, 1=yes)", settings.speedoL0Spd, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_sL0("sL0", "Display speed with leading 0", settings.speedoL0Spd, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_HAVEGPS
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed (0=no, 1=yes)", settings.useGPSSpeed, 1, "autocomplete='off' title='Enable to display actual GPS speed on speedo'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed", settings.useGPSSpeed, 1, "autocomplete='off' title='Check to display actual GPS speed on speedo' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_updrt(spdRateCustHTML);  // speed update rate
#endif // TC_HAVEGPS
#ifdef TC_HAVETEMP
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature (0=no, 1=yes)", settings.dispTemp, 1, "autocomplete='off' title='Enable to display temperature on speedo when idle'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature", settings.dispTemp, 1, "autocomplete='off' title='Check to display temperature on speedo when idle' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_tempBright("temBri", "<br>Temperature brightness (0-15)", settings.tempBright, 2, "type='number' min='0' max='15' autocomplete='off'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_tempOffNM("toffNM", "Temperature in night mode (0=dimmed, 1=off)", settings.tempOffNM, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_tempOffNM("toffNM", "Temperature off in night mode", settings.tempOffNM, 1, "autocomplete='off' title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif
#endif // TC_HAVESPEEDO

#ifdef FAKE_POWER_ON
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch (0=no, 1=yes)", settings.fakePwrOn, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch", settings.fakePwrOn, 1, "type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif

#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettDelay("ettDe", "Delay (ms)", settings.ettDelay, 5, "type='number' min='0' max='60000' title='Externally triggered time travel is delayed by specified number of millisecs'");
//#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
//WiFiManagerParameter custom_ettLong("ettLg", "Time travel sequence (0=short, 1=complete)", settings.ettLong, 1, "autocomplete='off'");
//#else // -------------------- Checkbox hack: --------------
//WiFiManagerParameter custom_ettLong("ettLg", "Play complete time travel sequence", settings.ettLong, 1, "title='If unchecked, the short \"re-entry\" sequence is played' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
//#endif // -------------------------------------------------
#endif

#ifdef EXTERNAL_TIMETRAVEL_OUT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useETTO("uEtto", "Control props connected by wire (0=no, 1=yes)", settings.useETTO, 1, "autocomplete='off' title='Enable to notify props of a time travel'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useETTO("uEtto", "Control props connected by wire", settings.useETTO, 1, "autocomplete='off' title='Check to notify props of a time travel' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_noETTOL("uEtNL", "Signal Time Travel without 5s lead (0=no, 1=yes)", settings.noETTOLead, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_noETTOL("uEtNL", "Signal Time Travel without 5s lead", settings.noETTOLead, 1, "autocomplete='off' type='checkbox' class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif // EXTERNAL_TIMETRAVEL_OUT
#ifdef TC_HAVEGPS
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_qGPS("qGPS", "Provide GPS speed for wireless props (0=no, 1=yes)", settings.quickGPS, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_qGPS("qGPS", "Provide GPS speed for wireless props", settings.quickGPS, 1, "autocomplete='off' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif // TC_HAVEGPS
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds (0=no, 1=yes)", settings.playTTsnds, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds", settings.playTTsnds, 1, "autocomplete='off' type='checkbox'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_HAVEMQTT
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_useMQTT("uMQTT", "Use HA/MQTT 3.1.1 (0=no, 1=yes)", settings.useMQTT, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_useMQTT("uMQTT", "Use Home Assistant (MQTT 3.1.1)", settings.useMQTT, 1, "type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
WiFiManagerParameter custom_mqttServer("ha_server", "<br>Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9\\.:\\-]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttUser("ha_usr", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
WiFiManagerParameter custom_mqttTopic("ha_topic", "Topic to display", settings.mqttTopic, 127, "placeholder='Optional. Example: home/alarm/status'");
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_pubMQTT("pMQTT", "Send event notifications (0=no, 1=yes)", settings.pubMQTT, 1, "autocomplete='off' title='Enable to send out time travel/alarm notifications'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_pubMQTT("pMQTT", "Send event notifications", settings.pubMQTT, 1, "type='checkbox' class='mt5' title='Check to send out time travel/alarm notifications'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------
#endif // HAVEMQTT

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle mode enabled at startup (0=no, 1=yes)", settings.shuffle, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_shuffle("musShu", "Shuffle mode enabled at startup", settings.shuffle, 1, "type='checkbox' style='margin-top:8px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save secondary settings on SD (0=no, 1=yes)<br><span style='font-size:80%'>Enable this to avoid flash wear</span>", settings.CfgOnSD, 1, "autocomplete='off'");
#ifndef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent (0=no, 1=yes)<br><span style='font-size:80%'>Requires SD card and above option to be enabled</span>", settings.timesPers, 1, "autocomplete='off'");
#endif
#else // -------------------- Checkbox hack: --------------
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save secondary settings on SD <span style='font-size:80%'>(Avoids flash wear)</span>", settings.CfgOnSD, 1, "autocomplete='off' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#ifdef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent <span style='font-size:80%'>(Requires SD card)</span>", settings.timesPers, 1, "type='checkbox' class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER);
#endif
#endif // -------------------------------------------------
#ifdef TC_NOCHECKBOXES  // --- Standard text boxes: -------
//WiFiManagerParameter custom_sdFrq("sdFrq", "SD clock speed (0=16Mhz, 1=4Mhz)<br><span style='font-size:80%'>Slower access might help in case of problems with SD cards</span>", settings.sdFreq, 1, "autocomplete='off'");
#else // -------------------- Checkbox hack: --------------
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span style='font-size:80%'>Checking this might help in case of SD card problems</span>", settings.sdFreq, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
#endif // -------------------------------------------------

WiFiManagerParameter custom_sectstart_head("<div class='sects'>");
WiFiManagerParameter custom_sectstart("</div><div class='sects'>");
WiFiManagerParameter custom_sectend("</div>");

WiFiManagerParameter custom_sectstart_wi("<div style='margin:0;padding:0'>Hold '7' to re-enable Wifi when in power save mode.</div></div><div class='sects'>");
WiFiManagerParameter custom_sectstart_wc("</div><div class='sects'><div class='headl'>World Clock mode</div>");
WiFiManagerParameter custom_sectstart_nm("</div><div class='sects'><div class='headl'>Night mode</div>");
#ifdef TC_HAVETEMP
WiFiManagerParameter custom_sectstart_te("</div><div class='sects'><div class='headl'>Temperature/humidity sensor</div>");
#endif
WiFiManagerParameter custom_sectstart_et("</div><div class='sects'><div class='headl'>External time travel button</div>");
WiFiManagerParameter custom_sectstart_mp("</div><div class='sects'><div class='headl'>MusicPlayer</div>");

WiFiManagerParameter custom_sectend_foot("</div><p></p>");

#define TC_MENUSIZE 6
static const char* wifiMenu[TC_MENUSIZE] = {"wifi", "param", "sep", "update", "sep", "custom" };

static const char apName[]  = "TCD-AP";
static const char myTitle[] = "Time Circuits";
static const char myHead[]  = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAA9QTFRFjpCRzMvH9tgx8iU9Q7YkHP8yywAAAC1JREFUeNpiYEQDDIwMKAAkwIwEiBTAMIMFCRApgGEGExIgUgDDDHQBNAAQYADhYgGBZLgAtAAAAABJRU5ErkJggg=='><script>function wlp(){return window.location.pathname;}function getn(x){return document.getElementsByTagName(x)}function ge(x){return document.getElementById(x)}function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;p=l.nextElementSibling.classList.contains('l');ge('p').disabled=!p;if(p){ge('p').placeholder='';ge('p').focus();}}uacs1=\"(function(el){document.getElementById('uacb').style.display = el.value=='' ? 'none' : 'initial';})(this)\";uacstr=\"Upload audio data (TCDA.bin)<br><form method='POST' action='uac' enctype='multipart/form-data' onchange=\\\"\"+uacs1+\"\\\"><input type='file' name='upac' accept='.bin,application/octet-stream'><button id='uacb' type='submit' class='h D'>Upload</button></form>\";window.onload=function(){xx=false;document.title='Time Circuits';if(ge('s')&&ge('dns')){xx=true;xxx=document.title;yyy='Configure WiFi';aa=ge('s').parentElement;bb=aa.innerHTML;dd=bb.search('<hr>');ee=bb.search('<button');cc='<div class=\"sects\">'+bb.substring(0,dd)+'</div><div class=\"sects\">'+bb.substring(dd+4,ee)+'</div>'+bb.substring(ee);aa.innerHTML=cc;document.querySelectorAll('a[href=\"#p\"]').forEach((userItem)=>{userItem.onclick=function(){c(this);return false;}});if(aa=ge('s')){aa.oninput=function(){if(this.placeholder.length>0&&this.value.length==0){ge('p').placeholder='********';}}}}if(ge('uploadbin')){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].insertAdjacentHTML('beforeend',uacstr);}}if(ge('uploadbin')||wlp()=='/u'||wlp()=='/uac'||wlp()=='/wifisave'||wlp()=='/paramsave'){xx=true;xxx=document.title;yyy=(wlp()=='/wifisave')?'Configure WiFi':(wlp()=='/paramsave'?'Setup':'Firmware update');aa=document.getElementsByClassName('wrap');if(aa.length>0){if((bb=ge('uploadbin'))){aa[0].style.textAlign='center';bb.parentElement.onsubmit=function(){aa=ge('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uacb');if(aa){aa.disabled=true}};if((bb=ge('uacb'))){aa[0].style.textAlign='center';bb.parentElement.onsubmit=function(){aa=ge('uacb');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uploadbin');if(aa){aa.disabled=true}}}}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}}if(ge('ttrp')||wlp()=='/param'){xx=true;xxx=document.title;yyy='Setup';}if(ge('ebnew')){xx=true;bb=getn('H3');aa=getn('H1');xxx=aa[0].innerHTML;yyy=bb[0].innerHTML;ff=aa[0].parentNode;ff.style.position='relative';}if(xx){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\" onClick=\"window.location=\\'/\\'\"><div class=\"tpm2\"><img src=\"data:image/png;base64,'+(zz?'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAbUlEQVR42tzXwRGAQAwDMdF/09QQQ24MLkDj77oeTiPA1wFGQiHATOgDGAp1AFOhDWAslAHMhS6AQKgCSIQmgEgoAsiEHoBQqAFIhRaAWCgByIVXAMuAdcA6YBlwALAKePzgd71QAByP71uAAQC+xwvdcFg7UwAAAABJRU5ErkJggg==':'iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAAZQTFRFSp1tAAAA635cugAAAAJ0Uk5T/wDltzBKAAAAgElEQVR42tzXQQqDABAEwcr/P50P2BBUdMhee6j7+lw8i4BCD8MiQAjHYRAghAh7ADWMMAcQww5jADHMsAYQwwxrADHMsAYQwwxrADHMsAYQwwxrgLgOPwKeAjgrrACcFkYAzgu3AN4C3AV4D3AP4E3AHcDF+8d/YQB4/Pn+CjAAMaIIJuYVQ04AAAAASUVORK5CYII=')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.2em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:4.5em\"':'')+'>'+yyy+'</div></div>';}if(ge('ebnew')){bb[0].remove();aa[0].replaceWith(dd);}if((ge('s')&&ge('dns'))||ge('uploadbin')||wlp()=='/u'||wlp()=='/uac'||wlp()=='/wifisave'||wlp()=='/paramsave'||ge('ttrp')||wlp()=='/param'){aa=document.getElementsByClassName('wrap');if(aa.length>0){aa[0].insertBefore(dd,aa[0].firstChild);aa[0].style.position='relative';}}}</script><style type='text/css'>body{font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}div.msg{border:1px solid #ccc;border-left-width:15px;border-radius:20px;background:linear-gradient(320deg,rgb(255,255,255) 0%,rgb(235,234,233) 100%);}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;color:#fff;background-color:#225a98;font-variant-caps:all-small-caps;}button.DD{color:#000;border:4px ridge #999;border-radius:2px;background:#e0c942;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAADBQTFRF////AAAAMyks8+AAuJYi3NHJo5aQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAbP19EwAAAAh0Uk5T/////////wDeg71ZAAAA4ElEQVR42qSTyxLDIAhF7yChS/7/bwtoFLRNF2UmRr0H8IF4/TBsY6JnQFvTJ8D0ncChb0QGlDvA+hkw/yC4xED2Z2L35xwDRSdqLZpFIOU3gM2ox6mA3tnDPa8UZf02v3q6gKRH/Eyg6JZBqRUCRW++yFYIvCjNFIt9OSC4hol/ItH1FkKRQgAbi0ty9f/F7LM6FimQacPbAdG5zZVlWdfvg+oEpl0Y+jzqIJZ++6fLqlmmnq7biZ4o67lgjBhA0kvJyTww/VK0hJr/LHvBru8PR7Dpx9MT0f8e72lvAQYALlAX+Kfw0REAAAAASUVORK5CYII=');background-repeat:no-repeat;background-origin:content-box;background-size:contain;}br{display:block;font-size:1px;content:''}input[type='checkbox']{display:inline-block;margin-top:10px}input{border:thin inset}small{display:none}em > small{display:inline}form{margin-block-end:0;}.tpm{cursor:pointer;border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;font-weight:normal;margin-left:2em;}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.sects{background-color:#eee;border-radius:7px;margin-bottom:20px;padding-bottom:7px;padding-top:7px}.tpm0{position:relative;width:20em;margin:0 auto 0 auto;}.headl{margin:0 0 5px 0;padding:0}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}.mt5{margin-top:5px!important}</style>";
static const char* myCustMenu = "<form action='/erase' method='get' onsubmit='return confirm(\"This erases the WiFi config and reboots. The TCD will restart in access point mode. Are you sure?\");'><button id='ebnew' class='DD'>Erase WiFi Config</button></form><br/><img style='display:block;margin:10px auto 10px auto;' src='data:image/png;base64,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'><div style='font-size:10px;margin-left:auto;margin-right:auto;text-align:center;'>Version " TC_VERSION " (" TC_VERSION_EXTRA ")<br>Powered by <a href='https://tcd.out-a-ti.me' target=_blank>A10001986 [Documentation]</a></div>";

static int  shouldSaveConfig = 0;
static bool shouldSaveIPConfig = false;
static bool shouldDeleteIPConfig = false;

// Did user configure a WiFi network to connect to?
bool wifiHaveSTAConf = false;

bool carMode = false;

static unsigned long lastConnect = 0;
static unsigned long consecutiveAPmodeFB = 0;

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

static File acFile;
static bool haveACFile = false;
static int ACULerr = 0;

#ifdef TC_HAVEMQTT
#define       MQTT_SHORT_INT  (30*1000)
#define       MQTT_LONG_INT   (5*60*1000)
bool          useMQTT = false;
char          mqttUser[64] = { 0 };
char          mqttPass[64] = { 0 };
char          mqttServer[80] = { 0 };
uint16_t      mqttPort = 1883;
bool          haveMQTTaudio = false;
const char    *mqttAudioFile = "/ha-alert.mp3";
bool          pubMQTT = false;
static unsigned long mqttReconnectNow = 0;
static unsigned long mqttReconnectInt = MQTT_SHORT_INT;
static uint16_t      mqttReconnFails = 0;
static bool          mqttSubAttempted = false;
static bool          mqttOldState = true;
static bool          mqttDoPing = true;
static bool          mqttRestartPing = false;
static bool          mqttPingDone = false;
static unsigned long mqttPingNow = 0;
static unsigned long mqttPingInt = MQTT_SHORT_INT;
static uint16_t      mqttPingsExpired = 0;
#endif

static void wifiOff(bool force);
static void wifiConnect(bool deferConfigPortal = false);
static void saveParamsCallback();
static void saveConfigCallback();
static void preUpdateCallback();
static void preSaveConfigCallback();
static void waitConnectCallback();

static void setupStaticIP();
static bool isIp(char *str);
static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length, int defVal);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static char* strcpyfilter(char* destination, const char* source);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
#ifndef TC_NOCHECKBOXES
static void strcpyCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);
#endif
static void buildSelectMenu(char *target, const char **theHTML, int cnt, char *setting);

static void setupWebServerCallback();
static void handleUploadDone();
static void handleUploading();
static void handleUploadDone();

#ifdef TC_HAVEMQTT
static void strcpyutf8(char *dst, const char *src, unsigned int len);
static void mqttPing();
static bool mqttReconnect(bool force = false);
static void mqttLooper();
static void mqttCallback(char *topic, byte *payload, unsigned int length);
static void mqttSubscribe();
#endif


/*
 * wifi_setup()
 *
 */
void wifi_setup()
{
    int temp;
    
    WiFiManagerParameter *parmArray[] = {

      &custom_aood,

      &custom_tzsel,

      &custom_sectstart_head, 
      &custom_playIntro, 
      &custom_mode24,
      &custom_beep_aint,
      &custom_alarmRTC,
      #ifndef PERSISTENT_SD_ONLY
      &custom_ttrp,
      #endif

      &custom_sectstart, 
      &custom_hostName,
      &custom_sysID,
      &custom_appw,
      &custom_wifiConRetries, 
      &custom_wifiConTimeout, 
      &custom_wifiPRe,
      &custom_wifiOffDelay, 
      &custom_wifiAPOffDelay,

      &custom_sectstart_wi,   // (ss_wi contains wifiHint from section above)
      &custom_timeZone, 
      &custom_ntpServer,   
      &custom_sectstart_wc, 
      &custom_timeZone1, &custom_timeZoneN1,
      &custom_timeZone2, &custom_timeZoneN2,
    
      &custom_sectstart_nm, 
      &custom_dtNmOff, &custom_ptNmOff, &custom_ltNmOff,
      &custom_autoNMTimes, 
      &custom_autoNMOn, &custom_autoNMOff,
    #ifdef TC_HAVELIGHT
      &custom_uLS, &custom_lxLim,
    #endif

    #ifdef TC_HAVETEMP
      &custom_sectstart_te, 
      &custom_tempUnit, 
      &custom_tempOffs,
    #endif

    #ifdef TC_HAVESPEEDO
      &custom_sectstart, 
      &custom_speedoType, 
      &custom_speedoBright,
      &custom_sAF,
      &custom_speedoFact,
      &custom_sL0,
    #ifdef TC_HAVEGPS
      &custom_useGPSS,
      &custom_updrt,
    #endif
    #ifdef TC_HAVETEMP
      &custom_useDpTemp, 
      &custom_tempBright, 
      &custom_tempOffNM,
    #endif
    #endif

    #ifdef FAKE_POWER_ON
      &custom_sectstart, 
      &custom_fakePwrOn,
    #endif
    
    #ifdef EXTERNAL_TIMETRAVEL_IN
      &custom_sectstart_et, 
      &custom_ettDelay,  
      //&custom_ettLong,
    #endif
    
      &custom_sectstart,
    #ifdef EXTERNAL_TIMETRAVEL_OUT
      &custom_useETTO,
      &custom_noETTOL,
    #endif
    #ifdef TC_HAVEGPS
      &custom_qGPS,
    #endif
      &custom_playTTSnd,

    #ifdef TC_HAVEMQTT
      &custom_sectstart, 
      &custom_useMQTT, 
      &custom_mqttServer, 
      &custom_mqttUser, 
      &custom_mqttTopic,
    #ifdef EXTERNAL_TIMETRAVEL_OUT
      &custom_pubMQTT,
    #endif
    #endif
    
      &custom_sectstart_mp, 
      &custom_shuffle,

      &custom_sectstart, 
      &custom_CfgOnSD,
      #ifdef PERSISTENT_SD_ONLY
      &custom_ttrp,
      #endif
      //&custom_sdFrq, 
      &custom_sectend_foot,
      
      NULL
    };
      
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
    wm.setWebServerCallback(setupWebServerCallback);
    wm.setHostname(settings.hostName);
    wm.setCaptivePortalEnable(false);
    
    // Our style-overrides, the page title
    wm.setCustomHeadElement(myHead);
    wm.setTitle(myTitle);
    wm.setDarkMode(false);

    // Hack some stuff into WiFiManager main page
    wm.setCustomMenuHTML(myCustMenu);

    // Static IP info is not saved by WiFiManager,
    // have to do this "manually". Hence ipsettings.
    wm.setShowStaticFields(true);
    wm.setShowDnsFields(true);

    temp = atoi(settings.wifiConTimeout);
    if(temp < 7) temp = 7;
    if(temp > 25) temp = 25;
    wm.setConnectTimeout(temp);

    temp = atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    if(temp > 10) temp = 10;
    wm.setConnectRetries(temp);

    wm.setCleanConnect(true);
    //wm.setRemoveDuplicateAPs(false);

    #ifdef WIFIMANAGER_2_0_17
    wm._preloadwifiscan = false;
    wm._asyncScan = true;
    #endif

    wm.setMenu(wifiMenu, TC_MENUSIZE);

    temp = haveAudioFiles ? 1 : 0;
    while(parmArray[temp]) {
        wm.addParameter(parmArray[temp]);
        temp++;
    }

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

    // Disable WiFI PS in AP mode for car mode?
    // No, user might have only TCD, no FC or SID
    // Power saving makes sense then.
    //if(carMode) {
    //    wifiAPOffDelay = 0;
    //}

    // Read setting for "periodic retries"
    // This determines if, after a fall-back to AP mode,
    // the device should periodically retry to connect
    // to a configured WiFi network; see time_loop().
    doAPretry = (atoi(settings.wifiPRetry) > 0);

    // Configure static IP
    if(loadIpSettings()) {
        setupStaticIP();
    }
           
    // Find out if we have a configured WiFi network to connect to,
    // or if we are condemned to AP mode for good
    if(!carMode) {
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        wifiHaveSTAConf = (conf.sta.ssid[0] != 0);
        #ifdef TC_DBG
        Serial.printf("WiFi network configured: %s (%s)\n", wifiHaveSTAConf ? "YES" : "NO", 
                    wifiHaveSTAConf ? (const char *)conf.sta.ssid : "n/a");
        #endif
        // No point in retry when we have no WiFi config'd
        if(!wifiHaveSTAConf) {
            wm.setConnectRetries(1);
        }
    }

    // Connect, but defer starting the CP
    wifiConnect(true);
    
#ifdef TC_HAVEMQTT
    useMQTT = (atoi(settings.useMQTT) > 0);
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    pubMQTT = (atoi(settings.pubMQTT) > 0);
    #endif
    
    if((!settings.mqttServer[0]) || // No server -> no MQTT
       (wifiInAPMode))              // WiFi in AP mode -> no MQTT
        useMQTT = false;  
    
    if(useMQTT) {

        bool mqttRes = false;
        char *t;
        int tt;

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        if((t = strchr(settings.mqttServer, ':'))) {
            strncpy(mqttServer, settings.mqttServer, t - settings.mqttServer);
            mqttServer[t - settings.mqttServer + 1] = 0;
            tt = atoi(t+1);
            if(tt > 0 && tt <= 65535) {
                mqttPort = tt;
            }
        } else {
            strcpy(mqttServer, settings.mqttServer);
        }

        if(isIp(mqttServer)) {
            mqttClient.setServer(stringToIp(mqttServer), mqttPort);
        } else {
            IPAddress remote_addr;
            if(WiFi.hostByName(mqttServer, remote_addr)) {
                mqttClient.setServer(remote_addr, mqttPort);
            } else {
                mqttClient.setServer(mqttServer, mqttPort);
                // Disable PING if we can't resolve domain
                mqttDoPing = false;
                Serial.printf("MQTT: Failed to resolve '%s'\n", mqttServer);
            }
        }
        
        mqttClient.setCallback(mqttCallback);
        mqttClient.setLooper(mqttLooper);

        if(settings.mqttUser[0] != 0) {
            if((t = strchr(settings.mqttUser, ':'))) {
                strncpy(mqttUser, settings.mqttUser, t - settings.mqttUser);
                mqttUser[t - settings.mqttUser + 1] = 0;
                strcpy(mqttPass, t + 1);
            } else {
                strcpy(mqttUser, settings.mqttUser);
            }
        }

        #ifdef TC_DBG
        Serial.printf("MQTT: server '%s' port %d user '%s' pass '%s'\n", mqttServer, mqttPort, mqttUser, mqttPass);
        #endif

        haveMQTTaudio = check_file_SD(mqttAudioFile);
            
        mqttReconnect(true);
        // Rest done in loop
            
    } else {

        #ifdef EXTERNAL_TIMETRAVEL_OUT
        pubMQTT = false;
        #endif

        #ifdef TC_DBG
        Serial.println("MQTT: Disabled");
        #endif

    }
#endif
}

/*
 * wifi_loop()
 *
 */
void wifi_loop()
{
    char oldCfgOnSD = 0;

#ifdef TC_HAVEMQTT
    if(useMQTT) {
        if(mqttClient.state() != MQTT_CONNECTING) {
            if(!mqttClient.connected()) {
                if(mqttOldState || mqttRestartPing) {
                    // Disconnection first detected:
                    mqttPingDone = mqttDoPing ? false : true;
                    mqttPingNow = mqttRestartPing ? millis() : 0;
                    mqttOldState = false;
                    mqttRestartPing = false;
                    mqttSubAttempted = false;
                }
                if(mqttDoPing && !mqttPingDone) {
                    audio_loop();
                    mqttPing();
                    audio_loop();
                }
                if(mqttPingDone) {
                    audio_loop();
                    mqttReconnect();
                    audio_loop();
                }
            } else {
                // Only call Subscribe() if connected
                mqttSubscribe();
                mqttOldState = true;
            }
        }
        mqttClient.loop();
    }
#endif
    
    wm.process();
    
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

            int temp;

            getParam("beepmode", settings.beep, 1, DEF_BEEP);
            getParam("rot_int", settings.autoRotateTimes, 1, DEF_AUTOROTTIMES);
            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(strlen(settings.hostName) == 0) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            strcpytrim(settings.systemID, custom_sysID.getValue(), true);
            strcpytrim(settings.appw, custom_appw.getValue(), true);
            if((temp = strlen(settings.appw)) > 0) {
                if(temp < 8) {
                    settings.appw[0] = 0;
                }
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            mystrcpy(settings.wifiConTimeout, &custom_wifiConTimeout);
            mystrcpy(settings.wifiOffDelay, &custom_wifiOffDelay);
            mystrcpy(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);
            strcpytrim(settings.ntpServer, custom_ntpServer.getValue());
            strcpytrim(settings.timeZone, custom_timeZone.getValue());

            strcpytrim(settings.timeZoneDest, custom_timeZone1.getValue());
            strcpytrim(settings.timeZoneDep, custom_timeZone2.getValue());
            strcpyfilter(settings.timeZoneNDest, custom_timeZoneN1.getValue());
            if(strlen(settings.timeZoneNDest) != 0) {
                char *s = settings.timeZoneNDest;
                for ( ; *s; ++s) *s = toupper(*s);
            }
            strcpyfilter(settings.timeZoneNDep, custom_timeZoneN2.getValue());
            if(strlen(settings.timeZoneNDep) != 0) {
                char *s = settings.timeZoneNDep;
                for ( ; *s; ++s) *s = toupper(*s);
            }
            
            getParam("anmtim", settings.autoNMPreset, 2, DEF_AUTONM_PRESET);
            
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
            getParam("spty", settings.speedoType, 2, DEF_SPEEDO_TYPE);
            mystrcpy(settings.speedoBright, &custom_speedoBright);
            mystrcpy(settings.speedoFact, &custom_speedoFact);
            #ifdef TC_HAVEGPS
            getParam("spdrt", settings.spdUpdRate, 1, DEF_SPD_UPD_RATE);
            #endif
            #ifdef TC_HAVETEMP
            mystrcpy(settings.tempBright, &custom_tempBright);
            #endif
            #endif

            #ifdef TC_HAVEMQTT
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            strcpyutf8(settings.mqttTopic, custom_mqttTopic.getValue(), sizeof(settings.mqttTopic));
            #endif
            
            #ifdef TC_NOCHECKBOXES // --------- Plain text boxes:

            mystrcpy(settings.timesPers, &custom_ttrp);
            mystrcpy(settings.alarmRTC, &custom_alarmRTC);
            mystrcpy(settings.playIntro, &custom_playIntro);
            mystrcpy(settings.mode24, &custom_mode24);

            mystrcpy(settings.wifiPRetry, &custom_wifiPRe);
            
            mystrcpy(settings.dtNmOff, &custom_dtNmOff);
            mystrcpy(settings.ptNmOff, &custom_ptNmOff);
            mystrcpy(settings.ltNmOff, &custom_ltNmOff);
            #ifdef TC_HAVELIGHT
            mystrcpy(settings.useLight, &custom_uLS);
            #endif
            
            #ifdef TC_HAVETEMP
            mystrcpy(settings.tempUnit, &custom_tempUnit);
            #endif
            
            #ifdef TC_HAVESPEEDO
            mystrcpy(settings.speedoAF, &custom_sAF);
            mystrcpy(settings.speedoL0Spd, &custom_sL0);
            #ifdef TC_HAVEGPS
            mystrcpy(settings.useGPSSpeed, &custom_useGPSS);
            #endif
            #ifdef TC_HAVETEMP
            mystrcpy(settings.dispTemp, &custom_useDpTemp);
            mystrcpy(settings.tempOffNM, &custom_tempOffNM);
            #endif
            #endif
            
            //#ifdef EXTERNAL_TIMETRAVEL_IN
            //mystrcpy(settings.ettLong, &custom_ettLong);
            //#endif

            #ifdef FAKE_POWER_ON
            mystrcpy(settings.fakePwrOn, &custom_fakePwrOn);
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            mystrcpy(settings.useETTO, &custom_useETTO);
            mystrcpy(settings.noETTOLead, &custom_noETTOL);
            #endif
            #ifdef TC_HAVEGPS
            mystrcpy(settings.quickGPS, &custom_qGPS);
            #endif
            mystrcpy(settings.playTTsnds, &custom_playTTSnd);

            #ifdef TC_HAVEMQTT
            mystrcpy(settings.useMQTT, &custom_useMQTT);
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            mystrcpy(settings.pubMQTT, &custom_pubMQTT);
            #endif
            #endif

            mystrcpy(settings.shuffle, &custom_shuffle);

            oldCfgOnSD = settings.CfgOnSD[0];
            mystrcpy(settings.CfgOnSD, &custom_CfgOnSD);
            //mystrcpy(settings.sdFreq, &custom_sdFrq);

            #else // -------------------------- Checkboxes:

            strcpyCB(settings.timesPers, &custom_ttrp);
            strcpyCB(settings.alarmRTC, &custom_alarmRTC);
            strcpyCB(settings.playIntro, &custom_playIntro);
            strcpyCB(settings.mode24, &custom_mode24);

            strcpyCB(settings.wifiPRetry, &custom_wifiPRe);

            strcpyCB(settings.dtNmOff, &custom_dtNmOff);
            strcpyCB(settings.ptNmOff, &custom_ptNmOff);
            strcpyCB(settings.ltNmOff, &custom_ltNmOff);
            #ifdef TC_HAVELIGHT
            strcpyCB(settings.useLight, &custom_uLS);
            #endif
            
            #ifdef TC_HAVETEMP
            strcpyCB(settings.tempUnit, &custom_tempUnit);
            #endif
            
            #ifdef TC_HAVESPEEDO
            strcpyCB(settings.speedoAF, &custom_sAF);
            strcpyCB(settings.speedoL0Spd, &custom_sL0);
            #ifdef TC_HAVEGPS
            strcpyCB(settings.useGPSSpeed, &custom_useGPSS);
            #endif
            #ifdef TC_HAVETEMP
            strcpyCB(settings.dispTemp, &custom_useDpTemp);
            strcpyCB(settings.tempOffNM, &custom_tempOffNM);
            #endif
            #endif
            
            //#ifdef EXTERNAL_TIMETRAVEL_IN
            //strcpyCB(settings.ettLong, &custom_ettLong);
            //#endif
            
            #ifdef FAKE_POWER_ON
            strcpyCB(settings.fakePwrOn, &custom_fakePwrOn);
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            strcpyCB(settings.useETTO, &custom_useETTO);
            strcpyCB(settings.noETTOLead, &custom_noETTOL);
            #endif
            #ifdef TC_HAVEGPS
            strcpyCB(settings.quickGPS, &custom_qGPS);
            #endif
            strcpyCB(settings.playTTsnds, &custom_playTTSnd);

            #ifdef TC_HAVEMQTT
            strcpyCB(settings.useMQTT, &custom_useMQTT);
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            strcpyCB(settings.pubMQTT, &custom_pubMQTT);
            #endif
            #endif

            strcpyCB(settings.shuffle, &custom_shuffle);

            oldCfgOnSD = settings.CfgOnSD[0];
            strcpyCB(settings.CfgOnSD, &custom_CfgOnSD);
            //strcpyCB(settings.sdFreq, &custom_sdFrq);

            #endif  // -------------------------

            autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
            saveAutoInterval();

            // Copy secondary settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                copySettings();
            }

        }

        stopAudio();

        // Write settings if requested, or no settings file exists
        if(shouldSaveConfig > 1 || !checkConfigExists()) {
            write_settings();
        }
        
        shouldSaveConfig = 0;

        // Reset esp32 to load new settings

        #ifdef TC_DBG
        Serial.println(F("Config Portal: Restarting ESP...."));
        #endif
        Serial.flush();

        prepareReboot();
        delay(500);
        esp_restart();
    }

    // WiFi power management
    // If a delay > 0 is configured, WiFi is powered-down after timer has
    // run out. The timer starts when the device is powered-up/boots.
    // There are separate delays for AP mode and STA mode.
    // WiFi can be re-enabled for the configured time by holding '7'
    // on the keypad.
    // NTP requests will - under some conditions - re-enable WiFi for a 
    // short while automatically if the user configured a WiFi network 
    // to connect to.
    
    if(wifiInAPMode) {
        // Disable WiFi in AP mode after a configurable delay (if > 0)
        if(wifiAPOffDelay > 0 && !bttfnHaveClients) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                wifiOff(false);
                wifiAPIsOff = true;
                wifiIsOff = false;
                syncTrigger = false;
                #ifdef TC_DBG
                Serial.println(F("WiFi (AP-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    } else {
        // Disable WiFi in STA mode after a configurable delay (if > 0)
        if(origWiFiOffDelay > 0 && !bttfnHaveClients) {
            if(!wifiIsOff && (millis() - wifiOnNow >= wifiOffDelay)) {
                wifiOff(false);
                wifiIsOff = true;
                wifiAPIsOff = false;
                syncTrigger = false;
                #ifdef TC_DBG
                Serial.println(F("WiFi (STA-mode) is off. Hold '7' to re-enable."));
                #endif
            }
        }
    }

}

static void wifiConnect(bool deferConfigPortal)
{     
    bool doOnlyAP = false;
    char realAPName[16];

    strcpy(realAPName, apName);
    if(settings.systemID[0]) {
        strcat(realAPName, settings.systemID);
    }
    
    if(carMode) {
        wm.startConfigPortal(realAPName, settings.appw);
        doOnlyAP = true;
    }
    
    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(!doOnlyAP && wm.autoConnect(realAPName, settings.appw)) {
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

        // Disable modem sleep, don't want delays accessing the CP or
        // with MQTT.
        WiFi.setSleep(false);

        // Set transmit power to max; we might be connecting as STA after
        // a previous period in AP mode.
        #ifdef TC_DBG
        {
            wifi_power_t power = WiFi.getTxPower();
            Serial.printf("WiFi: Max TX power in STA mode %d\n", power);
        }
        #endif
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        wifiInAPMode = false;
        wifiIsOff = false;
        wifiOnNow = millis();
        wifiAPIsOff = false;  // Sic! Allows checks like if(wifiAPIsOff || wifiIsOff)

        consecutiveAPmodeFB = 0;  // Reset counter of consecutive AP-mode fall-backs

    } else {

        #ifdef TC_DBG
        Serial.println(F("Config portal running in AP-mode"));
        #endif

        {
            #ifdef TC_DBG
            int8_t power;
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power in AP mode %d\n", power);
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

        wifiInAPMode = true;
        wifiAPIsOff = false;
        wifiAPModeNow = millis();
        wifiIsOff = false;    // Sic!

        if(wifiHaveSTAConf)       // increase counter of consecutive AP-mode fall-backs
            consecutiveAPmodeFB++;    
    }

    lastConnect = millis();
}

// This must not be called if no power-saving
// timers are configured.
static void wifiOff(bool force)
{
    if(!force) {
        if( (!wifiInAPMode && wifiIsOff) ||
            (wifiInAPMode && wifiAPIsOff) ) {
            return;
        }
    }

    wm.stopWebPortal();
    wm.disconnect();
    WiFi.mode(WIFI_OFF);
}

void wifiOn(unsigned long newDelay, bool alsoInAPMode, bool deferCP)
{
    unsigned long desiredDelay;
    unsigned long Now = millis();
    
    // wifiON() is called when the user pressed (and held) "7" (with alsoInAPMode
    // TRUE) and when a time sync via NTP is issued (with alsoInAPMode FALSE).
    //
    // Holding "7" serves two purposes: To re-enable WiFi if in power save mode,
    // and to re-connect to a configured WiFi network if we failed to connect to 
    // that network at the last connection attempt. In both cases, the Config Portal
    // is started.
    //
    // The NTP-triggered call should only re-connect if we are in power-save mode
    // after being connected to a user-configured network, or if we are in AP mode
    // but the user had config'd a network. Should only be called when frozen 
    // displays are feasible (eg night hours).
    //    
    // "wifiInAPMode" only tells us our latest mode; if the configured WiFi
    // network was - for whatever reason - was not available when we
    // tried to (re)connect, "wifiInAPMode" is true.

    // At this point, wifiInAPMode reflects the state after
    // the last connection attempt.

    if(alsoInAPMode) {    // User held "7"
        
        if(wifiInAPMode) {  // We are in AP mode

            if(!wifiAPIsOff) {

                // If ON but no user-config'd WiFi network -> bail
                if(!wifiHaveSTAConf) {
                    // Best we can do is to restart the timer
                    wifiAPModeNow = Now;
                    return;
                }

                // If ON and User has config's a NW, disable WiFi at this point
                // (in hope of successful connection below)
                wifiOff(true);

            }

        } else {            // We are in STA mode

            // If WiFi is not off, check if caller wanted
            // to start the CP, and do so, if not running
            if(!wifiIsOff) {
                if(!deferCP) {
                    if(!wm.getWebPortalActive()) {
                        wm.startWebPortal();
                    }
                }
                // Restart timer
                wifiOnNow = Now;
                return;
            }

        }

    } else {      // NTP-triggered

        // If no user-config'd network - no point, bail
        if(!wifiHaveSTAConf) return;

        if(wifiInAPMode) {  // We are in AP mode (because connection failed)

            #ifdef TC_DBG
            Serial.printf("wifiOn: consecutiveAPmodeFB %d\n", consecutiveAPmodeFB);
            #endif

            // Reset counter of consecutive AP-mode fallbacks
            // after a couple of days
            if(Now - lastConnect > 4*24*60*60*1000)
                consecutiveAPmodeFB = 0;

            // Give up after so many attempts
            if(consecutiveAPmodeFB > 5)
                return;

            // Do not try to switch from AP- to STA-mode
            // if last fall-back to AP-mode was less than
            // 15 (for the first 2 attempts, then 90) minutes ago
            if(Now - lastConnect < ((consecutiveAPmodeFB <= 2) ? 15*60*1000 : 90*60*1000))
                return;

            if(!wifiAPIsOff) {

                // If ON, disable WiFi at this point
                // (in hope of successful connection below)
                wifiOff(true);

            }

        } else {            // We are in STA mode

            // If WiFi is not off, check if caller wanted
            // to start the CP, and do so, if not running
            if(!wifiIsOff) {
                if(!deferCP) {
                    if(!wm.getWebPortalActive()) {
                        wm.startWebPortal();
                    }
                }
                // Add 60 seconds to timer in case the NTP
                // request might fall off the edge
                if(origWiFiOffDelay > 0) {
                    if((Now - wifiOnNow >= wifiOffDelay) ||
                       ((wifiOffDelay - (Now - wifiOnNow)) < (60*1000))) {
                        wifiOnNow += (60*1000);
                    }
                }
                return;
            }

        }

    }

    // (Re)connect
    WiFi.mode(WIFI_MODE_STA);
    wifiConnect(deferCP);

    // Restart timers
    // Note that wifiInAPMode now reflects the
    // result of our above wifiConnect() call

    if(wifiInAPMode) {

        #ifdef TC_DBG
        Serial.println("wifiOn: in AP mode after connect");
        #endif
      
        wifiAPModeNow = Now;
        
        #ifdef TC_DBG
        if(wifiAPOffDelay > 0) {
            Serial.printf("Restarting WiFi-off timer (AP mode); delay %d\n", wifiAPOffDelay);
        }
        #endif
        
    } else {

        #ifdef TC_DBG
        Serial.println("wifiOn: in STA mode after connect");
        #endif

        if(origWiFiOffDelay != 0) {
            desiredDelay = (newDelay > 0) ? newDelay : origWiFiOffDelay;
            if((Now - wifiOnNow >= wifiOffDelay) ||                    // If delay has run out, or
               (wifiOffDelay - (Now - wifiOnNow))  < desiredDelay) {   // new delay exceeds remaining delay:
                wifiOffDelay = desiredDelay;                           // Set new timer delay, and
                wifiOnNow = Now;                                       // restart timer
                #ifdef TC_DBG
                Serial.printf("Restarting WiFi-off timer; delay %d\n", wifiOffDelay);
                #endif
            }
        }

    }
}

// Check if a longer interruption due to a re-connect is to
// be expected when calling wifiOn(true, xxx).
bool wifiOnWillBlock()
{
    if(wifiInAPMode) {  // We are in AP mode
        if(!wifiAPIsOff) {
            if(!wifiHaveSTAConf) {
                return false;
            }
        }
    } else {            // We are in STA mode
        if(!wifiIsOff) return false;
    }

    return true;
}

void wifiStartCP()
{
    if(wifiInAPMode || wifiIsOff)
        return;

    #ifdef TC_DBG
    Serial.println("Starting CP");
    #endif

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
// Disable WiFi-off-timers and audio.
static void preUpdateCallback()
{
    wifiAPOffDelay = 0;
    origWiFiOffDelay = 0;

    mp_stop();
    stopAudio();

    flushDelayedSave();

    allOff();
    #ifdef TC_HAVESPEEDO
    if(useSpeedo) speedo.off();
    #endif
    destinationTime.resetBrightness();
    destinationTime.showTextDirect("UPDATING");
    destinationTime.on();
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
    int tnm = atoi(settings.autoNMPreset);
    #ifdef TC_HAVESPEEDO
    int tt = atoi(settings.speedoType);
    char spTyBuf[8];
    #endif

    // Make sure the settings form has the correct values

    beepaintCustHTML[0] = 0;
    buildSelectMenu(beepaintCustHTML, beepCustHTMLSrc, 6, settings.beep);
    buildSelectMenu(beepaintCustHTML, aintCustHTMLSrc, 8, settings.autoRotateTimes);

    custom_hostName.setValue(settings.hostName, 31);
    custom_sysID.setValue(settings.systemID, 7);
    custom_appw.setValue(settings.appw, 8);
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 2);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 2);
    custom_wifiOffDelay.setValue(settings.wifiOffDelay, 2);
    custom_wifiAPOffDelay.setValue(settings.wifiAPOffDelay, 2);
    custom_ntpServer.setValue(settings.ntpServer, 63);
    custom_timeZone.setValue(settings.timeZone, 63);

    custom_timeZone1.setValue(settings.timeZoneDest, 63);
    custom_timeZone2.setValue(settings.timeZoneDep, 63);
    custom_timeZoneN1.setValue(settings.timeZoneNDest, DISP_LEN);
    custom_timeZoneN2.setValue(settings.timeZoneNDep, DISP_LEN);

    sprintf(anmCustHTML, anmCustHTML1, settings.autoNMPreset, (tnm == 10) ? custHTMLSel : "", ooe);
    for(int i = 0; i < 5; i++) {
        sprintf(anmCustHTML + strlen(anmCustHTML), anmCustHTMLSrc[i], (tnm == i) ? custHTMLSel : "", (i == 4) ? osde : ooe);
    }

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
    sprintf(spTyCustHTML, "%s%s%s%s%d'%s>%s%s", spTyCustHTML1, settings.speedoType, spTyCustHTML2, spTyOptP1, 99, (tt == 99) ? custHTMLSel : "", "None", spTyOptP3);
    for (int i = SP_MIN_TYPE; i < SP_NUM_TYPES; i++) {
        sprintf(spTyCustHTML + strlen(spTyCustHTML), "%s%d'%s>%s%s", spTyOptP1, i, (tt == i) ? custHTMLSel : "", dispTypeNames[i], spTyOptP3);
    }
    strcat(spTyCustHTML, spTyCustHTMLE);
    custom_speedoBright.setValue(settings.speedoBright, 2);
    custom_speedoFact.setValue(settings.speedoFact, 3);
    #ifdef TC_HAVEGPS
    spdRateCustHTML[0] = 0;
    buildSelectMenu(spdRateCustHTML, spdRateCustHTMLSrc, 6, settings.spdUpdRate);
    #endif
    #ifdef TC_HAVETEMP
    custom_tempBright.setValue(settings.tempBright, 2);
    #endif
    #endif

    #ifdef TC_HAVEMQTT
    custom_mqttServer.setValue(settings.mqttServer, 79);
    custom_mqttUser.setValue(settings.mqttUser, 63);
    custom_mqttTopic.setValue(settings.mqttTopic, 63);
    #endif

    #ifdef TC_NOCHECKBOXES  // Standard text boxes: -------

    custom_ttrp.setValue(settings.timesPers, 1);
    custom_alarmRTC.setValue(settings.alarmRTC, 1);
    custom_playIntro.setValue(settings.playIntro, 1);
    custom_mode24.setValue(settings.mode24, 1);
    custom_wifiPRe.setValue(settings.wifiPRetry, 1);
    custom_dtNmOff.setValue(settings.dtNmOff, 1);
    custom_ptNmOff.setValue(settings.ptNmOff, 1);
    custom_ltNmOff.setValue(settings.ltNmOff, 1);
    #ifdef TC_HAVELIGHT
    custom_uLS.setValue(settings.useLight, 1);
    #endif
    #ifdef TC_HAVETEMP
    custom_tempUnit.setValue(settings.tempUnit, 1);
    #endif
    #ifdef TC_HAVESPEEDO
    custom_sAF.setValue(settings.speedoAF, 1);
    custom_sL0.setValue(settings.speedoL0Spd, 1);
    #ifdef TC_HAVEGPS
    custom_useGPSS.setValue(settings.useGPSSpeed, 1);
    #endif
    #ifdef TC_HAVETEMP
    custom_useDpTemp.setValue(settings.dispTemp, 1);
    custom_tempOffNM.setValue(settings.tempOffNM, 1);
    #endif
    #endif
    #ifdef FAKE_POWER_ON
    custom_fakePwrOn.setValue(settings.fakePwrOn, 1);
    #endif
    //#ifdef EXTERNAL_TIMETRAVEL_IN
    //custom_ettLong.setValue(settings.ettLong, 1);
    //#endif
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    custom_useETTO.setValue(settings.useETTO, 1);
    custom_noETTOL.setValue(settings.noETTOLead, 1);
    #endif
    #ifdef TC_HAVEGPS
    custom_qGPS.setValue(settings.quickGPS, 1);
    #endif
    custom_playTTSnd.setValue(settings.playTTsnds, 1);
    #ifdef TC_HAVEMQTT
    custom_useMQTT.setValue(settings.useMQTT, 1);
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    custom_pubMQTT.setValue(settings.pubMQTT, 1);
    #endif
    #endif
    custom_shuffle.setValue(settings.shuffle, 1);
    custom_CfgOnSD.setValue(settings.CfgOnSD, 1);
    //custom_sdFrq.setValue(settings.sdFreq, 1);

    #else   // For checkbox hack --------------------------

    setCBVal(&custom_ttrp, settings.timesPers);
    setCBVal(&custom_alarmRTC, settings.alarmRTC);
    setCBVal(&custom_playIntro, settings.playIntro);
    setCBVal(&custom_mode24, settings.mode24);
    setCBVal(&custom_wifiPRe, settings.wifiPRetry);
    setCBVal(&custom_dtNmOff, settings.dtNmOff);
    setCBVal(&custom_ptNmOff, settings.ptNmOff);
    setCBVal(&custom_ltNmOff, settings.ltNmOff);
    #ifdef TC_HAVELIGHT
    setCBVal(&custom_uLS, settings.useLight);
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_tempUnit, settings.tempUnit);
    #endif
    #ifdef TC_HAVESPEEDO
    setCBVal(&custom_sAF, settings.speedoAF);
    setCBVal(&custom_sL0, settings.speedoL0Spd);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPSS, settings.useGPSSpeed);
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useDpTemp, settings.dispTemp);
    setCBVal(&custom_tempOffNM, settings.tempOffNM);
    #endif
    #endif
    #ifdef FAKE_POWER_ON
    setCBVal(&custom_fakePwrOn, settings.fakePwrOn);
    #endif
    //#ifdef EXTERNAL_TIMETRAVEL_IN
    //setCBVal(&custom_ettLong, settings.ettLong);
    //#endif
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    setCBVal(&custom_useETTO, settings.useETTO);
    setCBVal(&custom_noETTOL, settings.noETTOLead);
    #endif
    #ifdef TC_HAVEGPS
    setCBVal(&custom_qGPS, settings.quickGPS);
    #endif
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    #ifdef TC_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    setCBVal(&custom_pubMQTT, settings.pubMQTT);
    #endif
    #endif
    setCBVal(&custom_shuffle, settings.shuffle);
    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);

    #endif // --------------------------------------------- 
}

static void buildSelectMenu(char *target, const char **theHTML, int cnt, char *setting)
{
    int sr = atoi(setting);
    
    strcat(target, theHTML[0]);
    strcat(target, setting);
    sprintf(target + strlen(target), "' name='%s' id='%s' autocomplete='off'><option value='0'", theHTML[1], theHTML[1]);
    for(int i = 0; i < cnt - 2; i++) {
        if(sr == i) strcat(target, custHTMLSel);
        sprintf(target + strlen(target), 
            theHTML[i+2], (i == cnt - 3) ? osde : ooe);
    }
}

/*
 * Audio data uploader
 */
static void setupWebServerCallback()
{
    wm.server->on(WM_G(R_updateacdone), HTTP_POST, &handleUploadDone, &handleUploading);
}

static void doCloseACFile(bool doRemove)
{
    if(haveACFile) {
        closeACFile(acFile);
        haveACFile = false;
    }
    if(doRemove) removeACFile();
}

static void handleUploading()
{
    HTTPUpload& upload = wm.server->upload();

    if(upload.status == UPLOAD_FILE_START) {

          preUpdateCallback();
          
          haveACFile = openACFile(acFile); 
          ACULerr = haveACFile ? 0 : (haveSD ? 1 : 2);
          
    } else if(upload.status == UPLOAD_FILE_WRITE) {

          if(haveACFile) {
              if(writeACFile(acFile, upload.buf, upload.currentSize) != upload.currentSize) {
                  doCloseACFile(true);
                  ACULerr = 3;
              }
          }

    } else if(upload.status == UPLOAD_FILE_END) {

        doCloseACFile(false);
      
    } else if(upload.status == UPLOAD_FILE_ABORTED) {

        doCloseACFile(true);
        ACULerr = 4;

    }

    delay(0);
}

static void handleUploadDone()
{
    const char *ebuf = "ERROR";
    const char *dbuf = "DONE";
    char *buf = NULL;
    //bool ownbuf = false;
    int buflen  = STRLEN(acul_part1) +
                  STRLEN(myTitle)    +
                  STRLEN(acul_part2) +
                  STRLEN(myHead)     +
                  STRLEN(acul_part3) +
                  STRLEN(acul_part4) +
                  STRLEN(myTitle)    +
                  STRLEN(acul_part5) +
                  STRLEN(apName)     +
                  STRLEN(acul_part6) +
                  STRLEN(acul_part8) +
                  1;

    if(!ACULerr) {
        if(!check_if_default_audio_present()) {
            ACULerr = 5;
            removeACFile();
        }
    }

    buflen += ACULerr ? (STRLEN(acul_part71) + strlen(acul_errs[ACULerr-1])) : STRLEN(acul_part7);

    if(!(buf = (char *)malloc(buflen))) {
        buf = (char *)(ACULerr ? ebuf : dbuf);
        //ownbuf = true;
    } else {
        strcpy(buf, acul_part1);
        strcat(buf, myTitle);
        strcat(buf, acul_part2);
        strcat(buf, myHead);
        strcat(buf, acul_part3);
        strcat(buf, acul_part4);
        strcat(buf, myTitle);
        strcat(buf, acul_part5);
        strcat(buf, apName);
        strcat(buf, acul_part6);
        if(!ACULerr) {
            strcat(buf, acul_part7);
        } else {
            strcat(buf, acul_part71);
            strcat(buf, acul_errs[ACULerr-1]);
        }
        strcat(buf, acul_part8);
    }

    String str(buf);
    wm.server->send(200, F("text/html"), str);
    
    delay(1000);
    ESP.restart();

    // preUpdateCallback() destroyed too much for us to resume, need to reboot
    //if(!ownbuf) free(buf);
}

/*
 * Helpers
 */
int wifi_getStatus()
{
    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          return (int)WiFi.status();
      case WIFI_MODE_AP:
          return 0x10000;     // AP MODE
      case WIFI_MODE_APSTA:
          return 0x10003;     // AP/STA MIXED
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
      case WIFI_MODE_APSTA:
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

    if(segs == 3)
        return true;

    return false;
}

// IPAddress to string
void ipToString(char *str, IPAddress ip)
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
static void getParam(String name, char *destBuf, size_t length, int defaultVal)
{
    memset(destBuf, 0, length+1);
    if(wm.server->hasArg(name)) {
        strncpy(destBuf, wm.server->arg(name).c_str(), length);
    }
    if(strlen(destBuf) == 0) {
        sprintf(destBuf, "%d", defaultVal);
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

static bool myisgoodchar2(char mychar)
{
    return ((mychar == ' '));
}

static char* strcpytrim(char* destination, const char* source, bool doFilter)
{
    char *ret = destination;
    
    while(*source) {
        if(!myisspace(*source) && (!doFilter || myisgoodchar(*source))) *destination++ = *source;
        source++;
    }
    
    *destination = 0;
    
    return ret;
}

static char* strcpyfilter(char *destination, const char *source)
{
    char *ret = destination;
    
    while(*source) {
        if(myisgoodchar(*source) || myisgoodchar2(*source)) *destination++ = *source;
        source++;
    }
    
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
    strcpy(sv, (atoi(el->getValue()) > 0) ? "1" : "0");
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{
    const char makeCheck[] = "1' checked a='";
    
    el->setValue((atoi(sv) > 0) ? makeCheck : "1", 14);
}
#endif

int16_t filterOutUTF8(char *src, char *dst, int srcLen = 0, int maxChars = 99999)
{
    int i, j, k, slen = srcLen ? srcLen : strlen(src);
    unsigned char c, d, e;

    for(i = 0, j = 0; i < slen && j <= maxChars; i++) {
        c = (unsigned char)src[i];
        if(c >= 32 && c <= 126) {     // skip 127 = DEL
            if(c >= 'a' && c <= 'z') c &= ~0x20;
            else if(c == 126) c = '-';   // 126 = ~ but displayed as °, so make '-'
            dst[j++] = c; 
        } else {
            e = 0;
            if     (c >= 192 && c < 224)  e = 1;
            else if(c >= 224 && c < 240)  e = 2;
            else if(c >= 240 && c < 245)  e = 3;    // yes, 245 (otherwise bad UTF8)
            if(e) {
                if((i + e) < slen) {
                    /*
                    for(k = i + 1, d = 0; k <= i + 1 + e; k++) {
                        d |= (unsigned char)src[k];
                    }
                    if(d > 127 && d < 192) i += e;
                    */
                    i += e;   // Why check? Just skip.
                } else {
                    break;
                }
            }
        }
    }
    dst[j] = 0;

    return j;
}

#ifdef TC_HAVEMQTT
static void truncateUTF8(char *src)
{
    int i, j, slen = strlen(src);
    unsigned char c, e;

    for(i = 0; i < slen; i++) {
        c = (unsigned char)src[i];
        e = 0;
        if     (c >= 192 && c < 224)  e = 1;
        else if(c >= 224 && c < 240)  e = 2;
        else if(c >= 240 && c < 248)  e = 3;  // Invalid UTF8 >= 245, but consider 4-byte char anyway
        if(e) {
            if((i + e) < slen) {
                i += e;
            } else {
                src[i] = 0;
                return;
            }
        }
    }
}

static void strcpyutf8(char *dst, const char *src, unsigned int len)
{
    strncpy(dst, src, len - 1);
    dst[len - 1] = 0;
    truncateUTF8(dst);
}

static void mqttLooper()
{
    ntp_loop();
    audio_loop();
    #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
    // No RotEnc since this is called while
    // time_loop() is active, too. No parallel
    // polling of RotEnc!!
    gps_loop(false);   // does not call any other loops
    #endif
}

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    int i = 0, j, ml = (length <= 255) ? length : 255;
    char tempBuf[256];
    static const char *cmdList[] = {
      "TIMETRAVEL",       // 0
      "RETURN",           // 1
      "ALARM_ON",         // 2
      "ALARM_OFF",        // 3
      "NIGHTMODE_ON",     // 4
      "NIGHTMODE_OFF",    // 5
      "MP_SHUFFLE_ON",    // 6
      "MP_SHUFFLE_OFF",   // 7
      "MP_PLAY",          // 8
      "MP_STOP",          // 9
      "MP_NEXT",          // 10
      "MP_PREV",          // 11
      "BEEP_OFF",         // 12
      "BEEP_ON",          // 13
      "BEEP_30",          // 14
      "BEEP_60",          // 15
      NULL
    };

    if(!length) return;

    if(!strcmp(topic, "bttf/tcd/cmd")) {

        // Not taking commands under these circumstances:
        if(!FPBUnitIsOn || menuActive   || startup || 
           timeTravelP0 || timeTravelP1 || timeTravelRE)
            return;

        memcpy(tempBuf, (const char *)payload, ml);
        tempBuf[ml] = 0;

        for(j = 0; j < ml; j++) {
            if(tempBuf[j] >= 'a' && tempBuf[j] <= 'z') tempBuf[j] &= ~0x20;
        }

        while(cmdList[i]) {
            j = strlen(cmdList[i]);
            if((ml >= j) && !strncmp((const char *)tempBuf, cmdList[i], j)) {
                break;
            }
            i++;          
        }

        if(!cmdList[i]) return;

        switch(i) {
        case 0:
            #ifdef EXTERNAL_TIMETRAVEL_IN
            isEttKeyPressed = isEttKeyImmediate = true;
            #endif
            break;
        case 1:
            #ifdef EXTERNAL_TIMETRAVEL_IN
            isEttKeyHeld = true;
            #endif
            break;
        case 2:
            alarmOn();
            break;
        case 3:
            alarmOff();
            break;
        case 4:
            nightModeOn();
            manualNightMode = 1;
            manualNMNow = millis();
            break;
        case 5:
            nightModeOff();
            manualNightMode = 0;
            manualNMNow = millis();
            break;
        case 6:
        case 7:
            if(haveMusic) mp_makeShuffle((i == 6));
            break;
        case 8:    
            if(haveMusic) mp_play();
            break;
        case 9:
            if(haveMusic) mp_stop();
            break;
        case 10:
            if(haveMusic) mp_next(mpActive);
            break;
        case 11:
            if(haveMusic) mp_prev(mpActive);
            break;
        case 12:
        case 13:
        case 14:
        case 15:
            setBeepMode(i-12);
            break;
        }
            
    } else if(!strcmp(topic, settings.mqttTopic)) {

        memcpy(tempBuf, (const char *)payload, ml);
        tempBuf[ml] = 0;

        j = filterOutUTF8(tempBuf, mqttMsg);
        
        mqttIdx = 0;
        mqttMaxIdx = (j > DISP_LEN) ? j : -1;
        mqttDisp = 1;
        mqttOldDisp = 0;
        mqttST = haveMQTTaudio;
    
        #ifdef TC_DBG
        Serial.printf("MQTT: Message about [%s]: %s\n", topic, mqttMsg);
        #endif
    }
}

#ifdef TC_DBG
#define MQTT_FAILCOUNT 6
#else
#define MQTT_FAILCOUNT 120
#endif

static void mqttPing()
{
    switch(mqttClient.pstate()) {
    case PING_IDLE:
        if(WiFi.status() == WL_CONNECTED) {
            if(!mqttPingNow || (millis() - mqttPingNow > mqttPingInt)) {
                mqttPingNow = millis();
                if(!mqttClient.sendPing()) {
                    // Mostly fails for internal reasons;
                    // skip ping test in that case
                    mqttDoPing = false;
                    mqttPingDone = true;  // allow mqtt-connect attempt
                }
            }
        }
        break;
    case PING_PINGING:
        if(mqttClient.pollPing()) {
            mqttPingDone = true;          // allow mqtt-connect attempt
            mqttPingNow = 0;
            mqttPingsExpired = 0;
            mqttPingInt = MQTT_SHORT_INT; // Overwritten on fail in reconnect
            // Delay re-connection for 5 seconds after first ping echo
            mqttReconnectNow = millis() - (mqttReconnectInt - 5000);
        } else if(millis() - mqttPingNow > 5000) {
            mqttClient.cancelPing();
            mqttPingNow = millis();
            mqttPingsExpired++;
            mqttPingInt = MQTT_SHORT_INT * (1 << (mqttPingsExpired / MQTT_FAILCOUNT));
            mqttReconnFails = 0;
        }
        break;
    } 
}

static bool mqttReconnect(bool force)
{
    bool success = false;

    if(useMQTT && (WiFi.status() == WL_CONNECTED)) {

        if(!mqttClient.connected()) {
    
            if(force || !mqttReconnectNow || (millis() - mqttReconnectNow > mqttReconnectInt)) {

                #ifdef TC_DBG
                Serial.println("MQTT: Attempting to (re)connect");
                #endif
    
                if(strlen(mqttUser)) {
                    success = mqttClient.connect(settings.hostName, mqttUser, strlen(mqttPass) ? mqttPass : NULL);
                } else {
                    success = mqttClient.connect(settings.hostName);
                }
    
                mqttReconnectNow = millis();
                
                if(!success) {
                    mqttRestartPing = true;  // Force PING check before reconnection attempt
                    mqttReconnFails++;
                    if(mqttDoPing) {
                        mqttPingInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    } else {
                        mqttReconnectInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    }
                    #ifdef TC_DBG
                    Serial.printf("MQTT: Failed to reconnect (%d)\n", mqttReconnFails);
                    #endif
                } else {
                    mqttReconnFails = 0;
                    mqttReconnectInt = MQTT_SHORT_INT;
                    #ifdef TC_DBG
                    Serial.println("MQTT: Connected to broker, waiting for CONNACK");
                    #endif
                }
    
                return success;
            } 
        }
    }
      
    return true;
}

static void mqttSubscribe()
{
    // Meant only to be called when connected!
    if(!mqttSubAttempted) {
        if(!mqttClient.subscribe("bttf/tcd/cmd", settings.mqttTopic)) {
            if(!mqttClient.subscribe("bttf/tcd/cmd")) {
                #ifdef TC_DBG
                Serial.println("MQTT: Failed to subscribe to all topics");
                #endif
            } else {
                #ifdef TC_DBG
                Serial.println("MQTT: Failed to subscribe to user topic");
                #endif
            }
        } else {
            #ifdef TC_DBG
            Serial.println("MQTT: Subscribed to all topics");
            #endif
        }
        mqttSubAttempted = true;
    }
}

bool mqttState()
{
    return (useMQTT && mqttClient.connected());
}

void mqttPublish(const char *topic, const char *pl, unsigned int len)
{
    if(useMQTT) {
        mqttClient.publish(topic, (uint8_t *)pl, len, false);
    }
}

#endif
