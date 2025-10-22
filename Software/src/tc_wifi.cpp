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

#include "src/WiFiManager/WiFiManager.h"

#ifndef WM_MDNS
#define TC_MDNS
#include <ESPmDNS.h>
#endif

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

#define STRLEN(x) (sizeof(x)-1)

Settings settings;

IPSettings ipsettings;

WiFiManager wm;

#ifdef TC_HAVEMQTT
WiFiClient mqttWClient;
PubSubClient mqttClient(mqttWClient);
#endif

static const char R_updateacdone[] = "/uac";

static const char acul_part3[]  = "</head><body><div class='wrap'><h1>";
static const char acul_part5[]  = "</h1><h3>";
static const char acul_part6[]  = "</h3><div class='msg";
static const char acul_part7[]  = " S'><strong>Upload successful.</strong><br/>Device rebooting.";
static const char acul_part7a[] = "<br>Installation will proceed after reboot.";
static const char acul_part71[] = " D'><strong>Upload failed.</strong><br>";
static const char acul_part8[]  = "</div></div></body></html>";
static const char *acul_errs[]  = { 
    "Can't open file on SD",
    "No SD card found",
    "Write error",
    "Bad file",
    "Not enough memory",
    "Unrecognized type",
    "Extraneous .bin file"
};

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

static const char anmCustHTML1[] = "<div class='cmp0'><label for='anmtim'>Schedule</label><select class='sel0' value='%s' name='anmtim' id='anmtim' autocomplete='off'><option value='10'%s>&#10060; Off%s0'";
static const char *anmCustHTMLSrc[5] = {
    "%s>&#128337; Daily, set hours below%s1'",
    "%s>&#127968; M-T:17-23/F:13-1/S:9-1/Su:9-23%s2'",
    "%s>&#127970; M-F:9-17%s3'",
    "%s>&#127970; M-T:7-17/F:7-14%s4'",
    "%s>&#128722; M-W:8-20/T-F:8-21/S:8-17%s"
};

#ifdef TC_HAVESPEEDO
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
#endif  //TC_HAVESPEEDO

static const char *apChannelCustHTMLSrc[14] = {
    "<div class='cmp0'><label for='apchnl'>WiFi channel</label><select class='sel0' value='",
    "apchnl",
    ">Random%s1'",
    ">1%s2'",
    ">2%s3'",
    ">3%s4'",
    ">4%s5'",
    ">5%s6'",
    ">6%s7'",
    ">7%s8'",
    ">8%s9'",
    ">9%s10'",
    ">10%s11'",
    ">11%s"
};

static const char *wmBuildbeepaint(const char *dest);
static const char *wmBuildAnmPreset(const char *dest);
#ifdef TC_HAVESPEEDO
static const char *wmBuildSpeedoType(const char *dest);
#ifdef TC_HAVEGPS
static const char *wmBuildUpdateRate(const char *dest);
#endif
#endif
static const char *wmBuildApChnl(const char *dest);
static const char *wmBuildBestApChnl(const char *dest);
static const char *wmBuildHaveSD(const char *dest);

static const char *osde = "</option></select></div>";
static const char *ooe  = "</option><option value='";
static const char custHTMLSel[] = " selected";

static const char *aco = "autocomplete='off'";
static const char *tznp1 = "City/location name [a-z/0-9/-/ ]";

// double-% since this goes through sprintf!
static const char bestAP[]   = "<div class='c' style='background-color:#%s;color:#fff;font-size:80%%;border-radius:5px'>Proposed channel at current location: %d<br>%s(Non-WiFi devices not taken into account)</div>";
static const char badWiFi[]  = "<br><i>Operating in AP mode not recommended</i>";

static const char haveNoSD[] = "<div class='c' style='background-color:#dc3630;color:#fff;font-size:80%;border-radius:5px'><i>No SD card present</i></div>";

// WiFi Configuration

#if defined(TC_MDNS) || defined(TC_WM_HAS_MDNS)
#define HNTEXT "Hostname<br><span style='font-size:80%'>The Config Portal is accessible at http://<i>hostname</i>.local<br>[Valid characters: a-z/0-9/-]</span>"
#else
#define HNTEXT "Hostname<br><span style='font-size:80%'>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9\\-]+' placeholder='Example: timecircuits'");
WiFiManagerParameter custom_wifiConRetries("wifiret", "Connection attempts (1-10)", settings.wifiConRetries, 2, "type='number' min='1' max='10' autocomplete='off'", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_wifiConTimeout("wificon", "Connection timeout (7-25[seconds])", settings.wifiConTimeout, 2, "type='number' min='7' max='25'");
WiFiManagerParameter custom_wifiPRe("wifiPRet", "Periodic reconnection attempts ", settings.wifiPRetry, 1, "autocomplete='off' type='checkbox' class='mt5 mb10'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_wifiOffDelay("wifioff", "Power save timer<br><span style='font-size:80%'>(10-99[minutes]; 0=off)</span>", settings.wifiOffDelay, 2, "type='number' min='0' max='99' title='WiFi will be shut down after chosen number of minutes after power-on. 0 means never.'");

WiFiManagerParameter custom_sysID("sysID", "Network name (SSID) appendix<br><span style='font-size:80%'>Will be appended to \"TCD-AP\" [a-z/0-9/-]</span>", settings.systemID, 7, "pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_appw("appw", "Password<br><span style='font-size:80%'>Password to protect TCD-AP. Empty or 8 characters [a-z/0-9/-]<br><b>Write this down, you might lock yourself out!</b></span>", settings.appw, 8, "minlength='8' pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_apch(wmBuildApChnl);
WiFiManagerParameter custom_bapch(wmBuildBestApChnl);
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "Power save timer<br><span style='font-size:80%'>(10-99[minutes]; 0=off)</span>", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='WiFi-AP will be shut down after chosen number of minutes after power-on. 0 means never.'");

// Settings

WiFiManagerParameter custom_aood("<div class='msg P'>Please <a href='/update'>install/update</a> sound pack</div>");

WiFiManagerParameter custom_tzsel("<datalist id='tz'><option value='PST8PDT,M3.2.0,M11.1.0'>Pacific</option><option value='MST7MDT,M3.2.0,M11.1.0'>Mountain</option><option value='CST6CDT,M3.2.0,M11.1.0'>Central</option><option value='EST5EDT,M3.2.0,M11.1.0'>Eastern</option><option value='GMT0BST,M3.5.0/1,M10.5.0'>Western European</option><option value='CET-1CEST,M3.5.0,M10.5.0/3'>Central European</option><option value='EET-2EEST,M3.5.0/3,M10.5.0/4'>Eastern European</option><option value='MSK-3'>Moscow</option><option value='AWST-8'>Australia Western</option><option value='ACST-9:30'>Australia Central/NT</option><option value='ACST-9:30ACDT,M10.1.0,M4.1.0/3'>Australia Central/SA</option><option value='AEST-10AEDT,M10.1.0,M4.1.0/3'>Australia Eastern VIC/NSW</option><option value='AEST-10'>Australia Eastern QL</option></datalist>");

WiFiManagerParameter custom_playIntro("plIn", "Play intro", settings.playIntro, 1, "type='checkbox' style='margin-top:3px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_beep_aint(wmBuildbeepaint);  // beep + aint 
WiFiManagerParameter custom_skTTa("skTTa", "Skip display disruption animation", settings.skipTTAnim, 1, "autocomplete='off' type='checkbox'", WFM_LABEL_AFTER);
#ifndef IS_ACAR_DISPLAY
WiFiManagerParameter custom_p3an("p3an", "Date entry animation like in part 3", settings.p3anim, 1, "autocomplete='off' type='checkbox'", WFM_LABEL_AFTER);
#endif
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds", settings.playTTsnds, 1, "autocomplete='off' type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is real present time", settings.alarmRTC, 1, "title='If unchecked, alarm base is the displayed \"present\" time' type='checkbox'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mode24("md24", "24-hour clock", settings.mode24, 1, "type='checkbox'", WFM_LABEL_AFTER);
#ifndef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent", settings.timesPers, 1, "type='checkbox'", WFM_LABEL_AFTER);
#endif

WiFiManagerParameter custom_timeZone("time_zone", "Time zone (in <a href='https://tz.out-a-ti.me' target=_blank>Posix</a> format)", settings.timeZone, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_ntpServer("ntp_server", "NTP server", settings.ntpServer, 63, "pattern='[a-zA-Z0-9\\.\\-]+' placeholder='Example: pool.ntp.org'");

WiFiManagerParameter custom_timeZone1("time_zone1", "Time zone for Destination Time display", settings.timeZoneDest, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_timeZone2("time_zone2", "Time zone for Last Time Dep. display", settings.timeZoneDep, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tz'");
WiFiManagerParameter custom_timeZoneN1("time_zonen1", tznp1, settings.timeZoneNDest, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO' style='margin-bottom:15px'");
WiFiManagerParameter custom_timeZoneN2("time_zonen2", tznp1, settings.timeZoneNDep, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO'");

WiFiManagerParameter custom_shuffle("musShu", "Shuffle mode enabled at startup", settings.shuffle, 1, "type='checkbox' class='mt5'", WFM_LABEL_AFTER);

WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time off", settings.dtNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time off", settings.ptNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. off", settings.ltNmOff, 1, "title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_autoNMTimes(wmBuildAnmPreset);
WiFiManagerParameter custom_autoNMOn("anmon", "Daily night-mode start hour (0-23)", settings.autoNMOn, 2, "type='number' min='0' max='23' title='Hour to switch on night-mode'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Daily night-mode end hour (0-23)", settings.autoNMOff, 2, "type='number' min='0' max='23' autocomplete='off' title='Hour to switch off night-mode'");
#ifdef TC_HAVELIGHT
WiFiManagerParameter custom_uLS("uLS", "Use light sensor", settings.useLight, 1, "title='If checked, TCD will be put in night-mode if lux level is below or equal threshold.' type='checkbox' style='margin-top:14px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_lxLim("lxLim", "Lux threshold (0-50000)", settings.luxLimit, 6, "type='number' min='0' max='50000' autocomplete='off'", WFM_LABEL_BEFORE);
#endif

#ifdef TC_HAVETEMP
WiFiManagerParameter custom_tempUnit("temUnt", "Show temperature in Celsius", settings.tempUnit, 1, "title='Fahrenheit if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_tempOffs("tOffs", "Temperature offset (-3.0-3.0)", settings.tempOffs, 4, "type='number' min='-3.0' max='3.0' step='0.1' title='Correction value to add to temperature' autocomplete='off'");
#endif // TC_HAVETEMP

#ifdef TC_HAVESPEEDO
WiFiManagerParameter custom_speedoType(wmBuildSpeedoType);
WiFiManagerParameter custom_speedoBright("speBri", "Speed brightness (0-15)", settings.speedoBright, 2, "type='number' min='0' max='15' autocomplete='off'");
WiFiManagerParameter custom_spAO("spAO", "Switch speedo off when idle", settings.speedoAO, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_sAF("sAF", "Real-life acceleration figures", settings.speedoAF, 1, "autocomplete='off' title='If unchecked, movie-like times are used' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_speedoFact("speFac", "Factor for real-life figures (0.5-5.0)", settings.speedoFact, 3, "type='number' min='0.5' max='5.0' step='0.5' title='1.0 means real-world DMC-12 acceleration time.' autocomplete='off'");
WiFiManagerParameter custom_sL0("sL0", "Speedo display like in part 3", settings.speedoP3, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_s3rd("s3rd", "Display post-point 0 like A-car", settings.speedo3rdD, 1, "autocomplete='off' type='checkbox' style='margin-top:12px' title='CircuitSetup speedo only. The digit is always 0.'", WFM_LABEL_AFTER);
#ifdef TC_HAVEGPS
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed", settings.useGPSSpeed, 1, "autocomplete='off' title='Check to display actual GPS speed on speedo' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_updrt(wmBuildUpdateRate);  // speed update rate
#endif // TC_HAVEGPS
#ifdef TC_HAVETEMP
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature", settings.dispTemp, 1, "autocomplete='off' title='Check to display temperature on speedo when idle' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_tempBright("temBri", "Temperature brightness (0-15)", settings.tempBright, 2, "type='number' min='0' max='15' autocomplete='off'");
WiFiManagerParameter custom_tempOffNM("toffNM", "Temperature off in night mode", settings.tempOffNM, 1, "autocomplete='off' title='Dimmed if unchecked' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // TC_HAVETEMP
#endif // TC_HAVESPEEDO

#ifdef FAKE_POWER_ON
WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch", settings.fakePwrOn, 1, "type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif

#ifdef EXTERNAL_TIMETRAVEL_IN
WiFiManagerParameter custom_ettDelay("ettDe", "Delay (ms)", settings.ettDelay, 5, "type='number' min='0' max='60000' title='Externally triggered time travel is delayed by specified number of millisecs'");
//WiFiManagerParameter custom_ettLong("ettLg", "Play complete time travel sequence", settings.ettLong, 1, "title='If unchecked, the short \"re-entry\" sequence is played' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif

#ifdef TC_HAVEGPS
WiFiManagerParameter custom_qGPS("qGPS", "Provide GPS speed for BTTFN clients", settings.quickGPS, 1, "autocomplete='off' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#endif // TC_HAVEGPS

#ifdef TC_HAVEMQTT
WiFiManagerParameter custom_useMQTT("uMQTT", "Use Home Assistant (MQTT 3.1.1)", settings.useMQTT, 1, "type='checkbox' class='mt5' style='margin-bottom:10px'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mqttServer("ha_server", "Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9\\.:\\-]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttUser("ha_usr", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
WiFiManagerParameter custom_mqttTopic("ha_topic", "Topic to display", settings.mqttTopic, 127, "placeholder='Optional. Example: home/alarm/status'");
WiFiManagerParameter custom_pubMQTT("pMQTT", "Send event notifications", settings.pubMQTT, 1, "type='checkbox' class='mt5' title='Check to send out time travel/alarm notifications'", WFM_LABEL_AFTER);
#endif // HAVEMQTT

#ifdef EXTERNAL_TIMETRAVEL_OUT
WiFiManagerParameter custom_useETTO("uEtto", "Control props connected by wire", settings.useETTO, 1, "autocomplete='off' title='Check to notify props of a time travel' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
WiFiManagerParameter custom_noETTOL("uEtNL", "Signal Time Travel without 5s lead", settings.noETTOLead, 1, "autocomplete='off' type='checkbox' class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER);
#endif // EXTERNAL_TIMETRAVEL_OUT

WiFiManagerParameter custom_haveSD(wmBuildHaveSD);
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save secondary settings on SD <span style='font-size:80%'>(Avoids flash wear)</span>", settings.CfgOnSD, 1, "autocomplete='off' type='checkbox' class='mt5'", WFM_LABEL_AFTER);
#ifdef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travels persistent <span style='font-size:80%'>(Requires SD card)</span>", settings.timesPers, 1, "type='checkbox' class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER);
#endif
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span style='font-size:80%'>Checking this might help in case of SD card problems</span>", settings.sdFreq, 1, "autocomplete='off' type='checkbox' style='margin-top:12px'", WFM_LABEL_AFTER);

WiFiManagerParameter custom_rvapm("rvapm", "Reverse AM/PM like in parts 2/3", settings.revAmPm, 1, "autocomplete='off' type='checkbox' class='mt5'", WFM_LABEL_AFTER);

WiFiManagerParameter custom_sectstart_head("<div class='sects'>");
WiFiManagerParameter custom_sectstart("</div><div class='sects'>");
WiFiManagerParameter custom_sectend("</div>");

WiFiManagerParameter custom_wifihint("<div style='margin:0;padding:0'>Hold '7' to re-enable Wifi when in power save mode.</div>");

WiFiManagerParameter custom_sectstart_wifi("</div><div class='sects'><div class='headl'>WiFi connection: Other settings</div>");

WiFiManagerParameter custom_sectstart_wc("</div><div class='sects'><div class='headl'>World Clock mode</div>");
WiFiManagerParameter custom_sectstart_mp("</div><div class='sects'><div class='headl'>MusicPlayer</div>");
WiFiManagerParameter custom_sectstart_nm("</div><div class='sects'><div class='headl'>Night mode</div>");
#ifdef TC_HAVETEMP
WiFiManagerParameter custom_sectstart_te("</div><div class='sects'><div class='headl'>Temperature/humidity sensor</div>");
#endif
WiFiManagerParameter custom_sectstart_et("</div><div class='sects'><div class='headl'>External time travel button</div>");
WiFiManagerParameter custom_sectstart_ap("</div><div class='sects'><div class='headl'>Access point (AP) mode settings</div>");
WiFiManagerParameter custom_sectstart_bt("</div><div class='sects'><div class='headl'>Wireless communication (BTTF-Network)</div>");

WiFiManagerParameter custom_sectend_foot("</div><p></p>");

#define TC_MENUSIZE 7
static const int8_t wifiMenu[TC_MENUSIZE] = { 
    WM_MENU_WIFI,
    WM_MENU_PARAM,
    WM_MENU_SEP,
    WM_MENU_UPDATE,
    WM_MENU_SEP,
    WM_MENU_CUSTOM,
    WM_MENU_END
};

#define AA_TITLE "Time Circuits"
#define AA_ICON "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAD1BMVEWOkJHMy8fyJT322DFDtiSDPmFqAAAAKklEQVQI12MQhAIGAQYwYGQQUAIDbAyEGhcwwGQgqzEGA0wGXA2CAXcGAJ79DMHwFaKYAAAAAElFTkSuQmCC"
#define AA_CONTAINER "TCDA"
#define UNI_VERSION TC_VERSION 
#define UNI_VERSION_EXTRA TC_VERSION_EXTRA
#define WEBHOME "tcd"

static const char myTitle[] = AA_TITLE;
static const char apName[]  = "TCD-AP";
static const char myHead[]  = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64," AA_ICON "'><script>window.onload=function(){xx=false;document.title=xxx='" AA_TITLE "';id=-1;ar=['/u','/uac','/wifisave','/paramsave'];ti=['Firmware upload','','WiFi Configuration','Settings'];if(ge('s')&&ge('dns')){xx=true;yyy=ti[2]}if(ge('uploadbin')||(id=ar.indexOf(wlp()))>=0){xx=true;if(id>=2){yyy=ti[id]}else{yyy=ti[0]};aa=gecl('wrap');if(aa.length>0){if(ge('uploadbin')){aa[0].style.textAlign='center';}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}}if(ge('ttrp')||wlp()=='/param'){xx=true;yyy=ti[3];}if(ge('ebnew')){xx=true;bb=getn('H3');aa=getn('H1');yyy=bb[0].innerHTML;ff=aa[0].parentNode;ff.style.position='relative';}if(xx){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\" onClick=\"window.location=\\'/\\'\"><div class=\"tpm2\"><img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQMAAACQp+OdAAAABlBMVEUAAABKnW0vhlhrAAAAAXRSTlMAQObYZgAAA'+(zz?'GBJREFUKM990aEVgCAABmF9BiIjsIIbsJYNRmMURiASePwSDPD0vPT12347GRejIfaOOIQwigSrRHDKBK9CCKoEqQF2qQMOSQAzEL9hB9ICNyMv8DPKgjCjLtAD+AV4dQM7O4VX9m1RYAAAAABJRU5ErkJggg==':'HtJREFUKM990bENwyAUBuFnuXDpNh0rZIBIrJUqMBqjMAIlBeIihQIF/fZVX39229PscYG32esCzeyjsXUzNHZsI0ocxJ0kcZIOsoQjnxQJT3FUiUD1NAloga6wQQd+4B/7QBQ4BpLAOZAn3IIy4RfUibCgTTDq+peG6AvsL/jPTu1L9wAAAABJRU5ErkJggg==')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.4em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:5em\"':'')+'>'+yyy+'</div></div>';}if(ge('ebnew')){bb[0].remove();aa[0].replaceWith(dd);}else if(xx){aa=gecl('wrap');if(aa.length>0){aa[0].insertBefore(dd,aa[0].firstChild);aa[0].style.position='relative';}}}</script><style type='text/css'>H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;font-variant-caps:all-small-caps;border-bottom:0.2em solid #225a98}input{border:thin inset}em > small{display:inline}form{margin-block-end:0;}.tpm{cursor:pointer;border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;font-weight:normal;margin-left:2.2em;overflow:clip;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI Semibold',Roboto,'Helvetica Neue',Verdana,Helvetica}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7.2em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.tpm0{position:relative;width:20em;padding:5px 0px 5px 0px;margin:0 auto 0 auto;}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}.mt5{margin-top:5px!important}.mb10{margin-bottom:10px!important}.mb0{margin-bottom:0px!important}.mb15{margin-bottom:15px!important}</style>";
static const char* myCustMenu = "<img id='ebnew' style='display:block;margin:10px auto 5px auto;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAR8AAAAyCAMAAABSzC8jAAAAUVBMVEUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABcqRVCAAAAGnRSTlMAv4BAd0QgzxCuMPBgoJBmUODdcBHumYgzVQmpHc8AAAf3SURBVGje7Jjhzp0gDIYFE0BQA/Ef93+hg7b4wvQ7R5Nl2Y812fzgrW15APE4eUW2rxOZNJfDcRu2q2Zjv9ygfe+1xSY7bXNWHH3lm13NJ01P/5PcrqyIeepfcLeCraOfpN7nPoSuLWjxHCSVa7aQs909Zxcf8mDBTNOcxWwlgmbw02gqNxv7z+5t8FIM2IdO1OUPzzmUNPl/K4F0vbIiNnMCf7pnmO79kBq57sviAiq3GKT3QFyqbG2NFUC4SDSDeckn68FLkWpPEXVFCbKUJDIQ84XP/pgPvO/LWlCHC60zjnzMKczkC4p9c3vLJ8GLYmMiBIGnGeHS2VdJ6/jCJ73ik10fIrhB8yefA/4jn/1syGLXWlER3DzmuNS4Vz4z2YWPnWfNqcVrTTKLtkaP0Q4IdhlQcdpkIPbCR3K1yn3jUzvr5JWLoa6j+SkuJNAkiESp1qYdiXPMALrUOyT7RpG8CL4Iin01jQRopWkufNCCyVbakbO0jCxUGjqugYgoLAzdJtpc+HQJ4Hj2aHBEgVRIFG/s5f3UPJUFPjxGE8+YyOiqMIPPWnmDDzI/5BORE70clHFjR1kaMEGLjc/xhY99yofCbpC4ENGmkQ/2yIWP5b/Ax1PYP8tHomB1bZSYFwSnIp9E3R/5ZPOIj6jLUz7Z3/EJlG/kM9467W/311aubuTDnQYD4SG6nEv/QkRFssXtE58l5+PN+tGP+Cw1sx/4YKjKf+STbp/PutqVT9I60e3sJVF30CIWK19c0XR11uCzF3XkI7kqXNbtT3w28gOflVMJHwc+eDYN55d25zTXSCuFJWHkk5gPZdzTh/P9ygcvmEJx645cyYLCYqk/Ffoab4k+5+X2fJ+FRl1g93zgp2iiqEwjfJiWbtqWr4dQESKGwSW5xIJH5XwEju+H7/gEP11exEY+7Dzr8q8IVVxkjHVy3Cc+R87HAz5iWqSDT/vYa9sEPiagcvAp5kUwHR97rh/Ae7V+wtp7be6OTyiXvbAo/7zCQKa6wT7xMTnbx3w0pMtr6z6BTwG08Mof+JCgWLh7/oDz/fvh3fPZrYmXteorHvkc3FF3QK2+dq2NT91g6ub90DUatlR0z+cQP6Q2I5/YazP4cGGJXPB+KMtCfpv5Cx/KqPgwen5+CWehGBtfiYPTZCnONtsplizdmwQ9/ez1/AKNg/Rv55edD54I8Alr07gs8GFzlqNh9fbCcfJx5brIrXwGvOAj16V5WeaC+jVg0FEyF+fOh98nPvHxpD8430Mh0R1t0UGrZQXwEYv3fOTRLnzGo49hveejmtdBfHGdGoy1LRPilMHCf+EzpYd8NtoVkKBxX/ydj/+Jzzzw2fgeuVU2hqNfgVc+hrb8wMf0fIzw9XJ1IefEOQVDyOQPFukLn/0ZH/nBdc/Hj+eXoyHsFz4ibB0fV8MF3MrbmMULHyQHn7iQK3thg4Xa68zSdr7rPkaMfPYvfPwjPpwyQRq1NA4yrG6ig2Ud+ehUOtYwfP8Z0RocbuDTbB75wFbhg421Q/TsLXw2xgEWceTTDDOb7vnATxgsnOvKR8qJ+H1x+/0nd0MN7IvvSOP3jVd88CFq3FhiSxeljezo10r4wmd/yGflDXblg7JkkAEvRSMfRB0/OIMPb7CXfGK3C5NssIgfH2Ttw9tKgXo+2xc+/gkf2cLpjg/K4kH6jNoGPnM/p9Kwm5nARx63b/ioGgB89nZyeSKyuW7kqqU1PZ/4hc+UnvGRDXblg7JkkPMWam3ajdPchKSnv2PeTP+qmdn8JPy7Rf+3X+zBgQAAAAAAkP9rI6iqqirNme2qpDAMhhtIWvxVKP2w7/1f6DapVmdnzsDCCucFx7QmaXx0ouB/kOfGfprM52Rkf4xZtb9E5BERsxnM0TlhGZvK/PXImI5sEj9sf9kzu3q9ltBt2hKK7bKmP2rRFZxlkcttWI3Zu2floeqGBzhnCVqQjmGq94hyfK3dzUiOwWNTmT9rJDmCiWXYcrNdDmqXi3mHqh0RZLnMIUHPPiGzJo2zkuXmghnZPavQZAMNI5fykQ9zA/wV0LBJr00LD8yhHnyIh4ynNz6RGYlZjI9ah+0qCvOWbhWAJVJ3hMrMceYKqK4plh1kK3hgYy5xuXWELo3cw1L+KONnC/yRzxpexyxsR9LYXau3zYSCzfi449f4zPHcF+wWtgRYHWsVBk/Xjs1Gx7apl7+7Wdjz8lq2YL/zYRH5zKeh8L7qOwxGFRG7cyrknU8QkX2xelVAiH4tmi8+dt022BVYNSy3DjSdel4bosupuTufWz/hiuAu5QSA8t98VKyn5Et456OiH/hIAdDORWX+vxL6ZFOSu/NZbnoUSLt7XKztt6X8wqcy8+rPW34JiLVgu/hc/UfUf9jxjU8honbxeVXmDeBjUT9Zlz4zC+obH3PT1C2huKcV7fSRiBLoQ/8RBn146o24eufDq5nklL70H4/0sQi6NZYqyWwPYvS5QkVctV1kgw6e1HmamPrYn4OWtl41umjhZWw6LfGNj4v41p+TLujZLbG3i/TSePukmEDIcybaKwHvy82zOezuWd24/PT8EiQ15GyniQqaNmqUst5/Eg3tRz//xqcDSLc3hgwEArqjsR+arMlul2ak50ywsLrcGgolBPddz/OxIV98YgDQsvoXIJ33j0mmv3zj43oCCuer+9h4PRTO51fJxpJPPrkCIFlusun4V375878k4T+G/QFTIGsvrRmuEwAAAABJRU5ErkJggg=='><div style='font-size:0.9em;line-height:0.9em;font-weight:bold;margin:5px auto 0px auto;text-align:center;font-variant:all-small-caps'>" UNI_VERSION " (" UNI_VERSION_EXTRA ")<br>Powered by A10001986 <a href='https://" WEBHOME ".out-a-ti.me' style='text-decoration:underline' target=_blank>[Home]</a></div>";

static const char* cliImages[] = {
    /* fc */
    "AgMAAABinRfyAAAADFBMVEVJSkrOzMP88bOTj3X+RyUkAAAAL0lEQVQI12MIBQIGBwYGRihxgJmRwXkC20EGhxRJIFf6CZDgMYDJMjWgEwi9YKMA/v8ME3vY03UAAAAASUVORK5CYII=",
    /* sid */
    "BAMAAADt3eJSAAAAD1BMVEWOkJHMy8dDtiT22DHyJT118zqFAAAALUlEQVQI12MQhAIGAQYwYAQzXGAMY0wGswGQYYzBAJIQhhKTAhARxYBbCncGAAUkB7Vkqvl1AAAAAElFTkSuQmCC",
    /* dg */
    "BAMAAADt3eJSAAAAD1BMVEUAAACOkJHMy8f////jAAAaAkeAAAAANUlEQVQI12NQggIGRUEwEEJiMIABkMFsbOJsbIDCYDY2hjAMYAxjZhjDAMiAa8fDQNgFdwYAnxgL08XYZH4AAAAASUVORK5CYII=",
    /* vsr */
    "BAMAAADt3eJSAAAAFVBMVEVJSkrMy8e2v73a5eP20zTyJT0AAAC7naKPAAAALklEQVQI12MQhAIGAQYGZgMGBkZkBpOSkgKEoRoEZaglKcCk0BlO5IiAASPcGQArMAeen41yugAAAABJRU5ErkJggg==",
    /* aux */
    "AQMAAAAlPW0iAAAABlBMVEVJSkrMy8feYSxoAAAAKklEQVQI12P4/5+hgZGh+SBD+0OGvkIQavjI0PgQJNLcCJZqBCIIG6gYAMbFElHyhQBTAAAAAElFTkSuQmCC",
    /* remote */
    "BAMAAADt3eJSAAAAFVBMVEVJSkrMy8e2v70AAADa5ePyJT320zT0Sr0YAAAAQElEQVQI12MQhAIGAQYwYAQzHFAZTEoKUEYalMFsDAIghhIQKDMyiAaDGSARZSMlYxAjGMZASIEZCO1wS+HOAADWkAscxWroAQAAAABJRU5ErkJggg=="
};
const char *menu_tp[] = { "Flux Capacitor", "SID", "Dash Gauges", "VSR", "AUX", "Remote" };
const char menu_myDiv[] = "<div style='margin-left:auto;margin-right:auto;text-align:center;'>";
const char menu_isp[] = "Please <a href='/update'>install</a> sound pack</div><hr>";
const char menu_item[] = "<a href='http://%s' target=_blank title='%s'><img style='zoom:2;padding:0 5px 0 5px;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQ%s' alt='%s'></a>";

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
static bool haveAC = false;
static int  numUploads = 0;
static int  *ACULerr = NULL;
static int  *opType = NULL;

#ifdef TC_HAVEMQTT
#define       MQTT_SHORT_INT  (30*1000)
#define       MQTT_LONG_INT   (5*60*1000)
static const char emptyStr[1] = { 0 };
bool          useMQTT = false;
char          *mqttUser = (char *)emptyStr;
char          *mqttPass = (char *)emptyStr;
char          *mqttServer = (char *)emptyStr;
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
static void saveWiFiCallback(const char *ssid, const char *pass);
static void preUpdateCallback();
static void postUpdateCallback(bool);
static void preSaveWiFiCallback();
static int  menuOutLenCallback();
static void menuOutCallback(String& page);
static void wifiDelayReplacement(unsigned int mydel);
static void gpCallback(int);
static bool preWiFiScanCallback();

static void setupStaticIP();
static bool isIp(char *str);
static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length, int defVal);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static char* strcpyfilter(char* destination, const char* source);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
static void strcpyCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);
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

    WiFiManagerParameter *wifiParmArray[] = {

      &custom_sectstart_head, 
      &custom_hostName,

      &custom_sectstart_wifi,
      &custom_wifiConRetries, 
      &custom_wifiConTimeout, 
      &custom_wifiPRe,
      &custom_wifiOffDelay,
      &custom_wifihint,

      &custom_sectstart_ap,
      &custom_sysID,
      &custom_appw,
      &custom_apch,
      &custom_bapch,
      &custom_wifiAPOffDelay,
      &custom_wifihint,

      &custom_sectend_foot,

      NULL
      
    };
    
    WiFiManagerParameter *parmArray[] = {

      &custom_aood,

      &custom_tzsel,

      &custom_sectstart_head, 
      &custom_playIntro,      
      &custom_beep_aint,
      &custom_skTTa,
    #ifndef IS_ACAR_DISPLAY
      &custom_p3an,
    #endif
      &custom_playTTSnd,
      &custom_alarmRTC,
      &custom_mode24,
    #ifndef PERSISTENT_SD_ONLY
      &custom_ttrp,
    #endif

      &custom_sectstart,
      &custom_timeZone, 
      &custom_ntpServer, 
        
      &custom_sectstart_wc, 
      &custom_timeZone1, &custom_timeZoneN1,
      &custom_timeZone2, &custom_timeZoneN2,

      &custom_sectstart_mp, 
      &custom_shuffle,
      
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
      &custom_spAO,
      &custom_sAF,
      &custom_speedoFact,
      &custom_sL0,
      &custom_s3rd,
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
    
    #ifdef TC_HAVEGPS
      &custom_sectstart_bt,
      &custom_qGPS,
    #endif
     
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
    
    #ifdef EXTERNAL_TIMETRAVEL_OUT
      &custom_sectstart,
      &custom_useETTO,
      &custom_noETTOL,
    #endif

      &custom_sectstart,
      &custom_haveSD,
      &custom_CfgOnSD,
      #ifdef PERSISTENT_SD_ONLY
      &custom_ttrp,
      #endif
      //&custom_sdFrq,

      &custom_sectstart,
      &custom_rvapm,
      
      &custom_sectend_foot,
      
      NULL
    };

    // Transition from NVS-saved data to own management:
    if(!settings.ssid[0] && settings.ssid[1] == 'X') {
        
        // Read NVS-stored WiFi data
        wm.getStoredCredentials(settings.ssid, sizeof(settings.ssid), settings.pass, sizeof(settings.pass));

        #ifdef TC_DBG
        Serial.printf("WiFi Transition: ssid '%s' pass '%s'\n", settings.ssid, settings.pass);
        #endif

        write_settings();
    }

    wm.setHostname(settings.hostName);

    wm.showUploadContainer(haveSD, AA_CONTAINER, true);

    wm.setPreSaveWiFiCallback(preSaveWiFiCallback);
    wm.setSaveWiFiCallback(saveWiFiCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setPreOtaUpdateCallback(preUpdateCallback);
    wm.setPostOtaUpdateCallback(postUpdateCallback);
    wm.setWebServerCallback(setupWebServerCallback);
    wm.setMenuOutLenCallback(menuOutLenCallback);
    wm.setMenuOutCallback(menuOutCallback);
    wm.setDelayReplacement(wifiDelayReplacement);
    wm.setGPCallback(gpCallback);
    wm.setPreWiFiScanCallback(preWiFiScanCallback);
    
    // Our style-overrides, the page title
    wm.setCustomHeadElement(myHead);
    wm.setTitle(myTitle);

    // Hack some stuff into WiFiManager main page
    wm.setCustomMenuHTML(myCustMenu);

    wm.setCarMode(carMode);
    wm.setWiFiAPMaxClients(6);

    temp = atoi(settings.apChnl);
    if(temp < 0) temp = 0;
    if(temp > 11) temp = 11;
    if(!temp) temp = random(1, 11);
    wm.setWiFiAPChannel(temp);

    temp = atoi(settings.wifiConTimeout);
    if(temp < 7) temp = 7;
    if(temp > 25) temp = 25;
    wm.setConnectTimeout(temp);

    temp = atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    if(temp > 10) temp = 10;
    wm.setConnectRetries(temp);

    wm.setMenu(wifiMenu, TC_MENUSIZE, false);

    wm.allocParms((sizeof(parmArray) / sizeof(WiFiManagerParameter *)) - 1);

    temp = haveAudioFiles ? 1 : 0;
    while(parmArray[temp]) {
        wm.addParameter(parmArray[temp]);
        temp++;
    }

    wm.allocWiFiParms((sizeof(wifiParmArray) / sizeof(WiFiManagerParameter *)) - 1);

    temp = 0;
    while(wifiParmArray[temp]) {
        wm.addWiFiParameter(wifiParmArray[temp]);
        temp++;
    }

    updateConfigPortalValues();

    if(!carMode) {
        wifiHaveSTAConf = (settings.ssid[0] != 0);
        #ifdef TC_DBG
        Serial.printf("WiFi network configured: %s (%s)\n", wifiHaveSTAConf ? "YES" : "NO", 
                    wifiHaveSTAConf ? (const char *)settings.ssid : "n/a");
        #endif
        // No point in retry when we have no WiFi config'd
        if(!wifiHaveSTAConf) {
            wm.setConnectRetries(1);
        }
    }

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
           
    // Connect, but defer starting the CP
    wifiConnect(true);

    // MDNS. Needs to be called AFTER mode(STA) or softAP init
    #ifdef TC_MDNS
    if(MDNS.begin(settings.hostName)) {
        MDNS.addService("http", "tcp", 80);
    }
    #endif
    
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

        mqttMsg = (char *)malloc(256);
        memset(mqttMsg, 0, 256);

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        if((t = strchr(settings.mqttServer, ':'))) {
            size_t ts = (t - settings.mqttServer) + 1;
            mqttServer = (char *)malloc(ts);
            memset(mqttServer, 0, ts);
            strncpy(mqttServer, settings.mqttServer, t - settings.mqttServer);
            tt = atoi(t + 1);
            if(tt > 0 && tt <= 65535) {
                mqttPort = tt;
            }
        } else {
            mqttServer = settings.mqttServer;
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
                size_t ts = strlen(settings.mqttUser) + 1;
                mqttUser = (char *)malloc(ts);
                strcpy(mqttUser, settings.mqttUser);
                mqttUser[t - settings.mqttUser] = 0;
                mqttPass = mqttUser + (t - settings.mqttUser + 1);
            } else {
                mqttUser = settings.mqttUser;
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
    
    if(shouldSaveIPConfig) {

        #ifdef TC_DBG
        Serial.println("WiFi: Saving IP config");
        #endif

        mp_stop();
        stopAudio();

        writeIpSettings();

        shouldSaveIPConfig = false;

    } else if(shouldDeleteIPConfig) {

        #ifdef TC_DBG
        Serial.println("WiFi: Deleting IP config");
        #endif

        mp_stop();
        stopAudio();

        deleteIpSettings();

        shouldDeleteIPConfig = false;

    }

    if(shouldSaveConfig) {

        int temp;

        // Save settings and restart esp32

        mp_stop();
        stopAudio();

        #ifdef TC_DBG
        Serial.println("Config Portal: Saving config");
        #endif

        // Only read parms if the user actually clicked SAVE on the wifi config or params pages
        if(shouldSaveConfig == 1) {

            // Parameters on WiFi Config page

            // Note: Parameters that need to grabbed from the server directly
            // through getParam() must be handled in preSaveWiFiCallback().

            // ssid, pass copied to settings in saveWiFiCallback()

            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(strlen(settings.hostName) == 0) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            mystrcpy(settings.wifiConTimeout, &custom_wifiConTimeout);
            strcpyCB(settings.wifiPRetry, &custom_wifiPRe);
            mystrcpy(settings.wifiOffDelay, &custom_wifiOffDelay);
            
            strcpytrim(settings.systemID, custom_sysID.getValue(), true);
            strcpytrim(settings.appw, custom_appw.getValue(), true);
            if((temp = strlen(settings.appw)) > 0) {
                if(temp < 8) {
                    settings.appw[0] = 0;
                }
            }
            
            mystrcpy(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);
          
        } else { 

            // Parameters on Settings page

            strcpyCB(settings.playIntro, &custom_playIntro);
            getParam("beepmode", settings.beep, 1, DEF_BEEP);
            getParam("rot_int", settings.autoRotateTimes, 1, DEF_AUTOROTTIMES);
            strcpyCB(settings.skipTTAnim, &custom_skTTa);
            #ifndef IS_ACAR_DISPLAY
            strcpyCB(settings.p3anim, &custom_p3an);
            #endif
            strcpyCB(settings.playTTsnds, &custom_playTTSnd);
            strcpyCB(settings.alarmRTC, &custom_alarmRTC);
            strcpyCB(settings.mode24, &custom_mode24);

            strcpytrim(settings.timeZone, custom_timeZone.getValue());
            strcpytrim(settings.ntpServer, custom_ntpServer.getValue());

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

            strcpyCB(settings.shuffle, &custom_shuffle);

            strcpyCB(settings.dtNmOff, &custom_dtNmOff);
            strcpyCB(settings.ptNmOff, &custom_ptNmOff);
            strcpyCB(settings.ltNmOff, &custom_ltNmOff);
            getParam("anmtim", settings.autoNMPreset, 2, DEF_AUTONM_PRESET);
            mystrcpy(settings.autoNMOn, &custom_autoNMOn);
            mystrcpy(settings.autoNMOff, &custom_autoNMOff);
            #ifdef TC_HAVELIGHT
            strcpyCB(settings.useLight, &custom_uLS);
            mystrcpy(settings.luxLimit, &custom_lxLim);
            #endif

            #ifdef TC_HAVETEMP
            strcpyCB(settings.tempUnit, &custom_tempUnit);
            mystrcpy(settings.tempOffs, &custom_tempOffs);
            #endif

            #ifdef TC_HAVESPEEDO
            getParam("spty", settings.speedoType, 2, DEF_SPEEDO_TYPE);
            mystrcpy(settings.speedoBright, &custom_speedoBright);
            strcpyCB(settings.speedoAO, &custom_spAO);
            strcpyCB(settings.speedoAF, &custom_sAF);
            mystrcpy(settings.speedoFact, &custom_speedoFact);
            strcpyCB(settings.speedoP3, &custom_sL0);
            strcpyCB(settings.speedo3rdD, &custom_s3rd);
            #ifdef TC_HAVEGPS
            strcpyCB(settings.useGPSSpeed, &custom_useGPSS);
            getParam("spdrt", settings.spdUpdRate, 1, DEF_SPD_UPD_RATE);
            #endif
            #ifdef TC_HAVETEMP
            strcpyCB(settings.dispTemp, &custom_useDpTemp);
            mystrcpy(settings.tempBright, &custom_tempBright);
            strcpyCB(settings.tempOffNM, &custom_tempOffNM);
            #endif
            #endif // HAVESPEEDO

            #ifdef FAKE_POWER_ON
            strcpyCB(settings.fakePwrOn, &custom_fakePwrOn);
            #endif

            #ifdef EXTERNAL_TIMETRAVEL_IN
            mystrcpy(settings.ettDelay, &custom_ettDelay);
            //strcpyCB(settings.ettLong, &custom_ettLong);
            #endif
            
            #ifdef TC_HAVEGPS
            strcpyCB(settings.quickGPS, &custom_qGPS);
            #endif

            #ifdef TC_HAVEMQTT
            strcpyCB(settings.useMQTT, &custom_useMQTT);
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            strcpyutf8(settings.mqttTopic, custom_mqttTopic.getValue(), sizeof(settings.mqttTopic));
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            strcpyCB(settings.pubMQTT, &custom_pubMQTT);
            #endif
            #endif
            
            #ifdef EXTERNAL_TIMETRAVEL_OUT
            strcpyCB(settings.useETTO, &custom_useETTO);
            strcpyCB(settings.noETTOLead, &custom_noETTOL);
            #endif
          
            oldCfgOnSD = settings.CfgOnSD[0];
            strcpyCB(settings.CfgOnSD, &custom_CfgOnSD);
            //strcpyCB(settings.sdFreq, &custom_sdFrq);
            strcpyCB(settings.timesPers, &custom_ttrp);

            strcpyCB(settings.revAmPm, &custom_rvapm);

            autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
            saveAutoInterval();

            // Copy secondary settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                copySettings();
            }

        }

        // Write settings if requested, or no settings file exists
        if(shouldSaveConfig >= 1 || !checkConfigExists()) {
            write_settings();
        }
        
        shouldSaveConfig = 0;

        // Reset esp32 to load new settings

        #ifdef TC_DBG
        Serial.println("Config Portal: Restarting ESP....");
        #endif
        Serial.flush();

        prepareReboot();
        delay(1000);
        esp_restart();
    }

    // For some reason (probably memory allocation doing some
    // garbage collection), the first HTTPSend (triggered in
    // wm.process()) after initiating mp3 playback takes
    // unusally long, in bad cases 100s of ms. It's worse 
    // closer to playback start.
    // This hack skips Webserver-handling inside wm.process() 
    // if an mp3 playback started within the last 3 seconds.
    wm.process(!checkAudioStarted());

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
                Serial.println("WiFi (AP-mode) is off. Hold '7' to re-enable.");
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
                Serial.println("WiFi (STA-mode) is off. Hold '7' to re-enable.");
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
    
    if(carMode || !settings.ssid[0]) {
        wm.startConfigPortal(realAPName, settings.appw, settings.ssid, settings.pass);
        doOnlyAP = true;
    }
    
    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(!doOnlyAP && wm.autoConnect(settings.ssid, settings.pass, realAPName, settings.appw)) {
        #ifdef TC_DBG
        Serial.println("WiFi connected");
        #endif

        // During boot, we start the CP later, to allow a quick NTP update.
        if(!deferConfigPortal) {
            wm.startWebPortal();
        }

        // Allow modem sleep:
        // WIFI_PS_MIN_MODEM is the default, and activated when calling this
        // with "true". When this is enabled, received WiFi data can be
        // delayed for as long as the DTIM period.
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
        Serial.println("Config portal running in AP-mode");
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
            // This has been fixed with CB 1.4 but much more is
            // simply not needed. The props are close together,
            // and we don't want to burn power needlessly.
            // The choices are:
            // WIFI_POWER_19_5dBm    = 19.5dBm
            // WIFI_POWER_19dBm      = 19dBm
            // WIFI_POWER_18_5dBm    = 18.5dBm
            // WIFI_POWER_17dBm      = 17dBm
            // WIFI_POWER_15dBm      = 15dBm
            // WIFI_POWER_13dBm      = 13dBm
            // WIFI_POWER_11dBm      = 11dBm
            // WIFI_POWER_8_5dBm     = 8.5dBm
            // WIFI_POWER_7dBm       = 7dBm     <-- proven to avoid the issues on CB < 1.4
            // WIFI_POWER_5dBm       = 5dBm
            // WIFI_POWER_2dBm       = 2dBm
            // WIFI_POWER_MINUS_1dBm = -1dBm

            WiFi.setTxPower(haveLineOut ? WIFI_POWER_8_5dBm : WIFI_POWER_7dBm);

            #ifdef TC_DBG
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power set to %d\n", power);
            #endif
        }

        wifiInAPMode = true;
        wifiAPIsOff = false;
        wifiAPModeNow = millis();
        wifiIsOff = false;    // Sic!

        if(wifiHaveSTAConf) {    
            consecutiveAPmodeFB++;  // increase counter of consecutive AP-mode fall-backs
        }
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
                // Note: In carMode, wifiHaveSTAConf = false
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

// This is called when the WiFi config is to be saved. We set
// a flag for the loop to read out and save the new WiFi config.
// SSID and password are copied to settings here.
static void saveWiFiCallback(const char *ssid, const char *pass)
{
    // ssid is the (new?) ssid to connect to, pass the password.
    // (We don't need to compare to the old ones since the
    // settings are saved in any case)
    // This is also used to "forget" a saved WiFi network, in
    // which case ssid and pass are empty strings.
    memset(settings.ssid, 0, sizeof(settings.ssid));
    memset(settings.pass, 0, sizeof(settings.pass));
    if(*ssid) {
        strncpy(settings.ssid, ssid, sizeof(settings.ssid) - 1);
        strncpy(settings.pass, pass, sizeof(settings.pass) - 1);
    }

    #ifdef TC_DBG
    Serial.printf("saveWiFiCallback: New ssid '%s'\n", settings.ssid);
    Serial.printf("saveWiFiCallback: New pass '%s'\n", settings.pass);
    #endif
    
    shouldSaveConfig = 1;
}

// This is the callback from the actual Params page. We set
// a flag for the loop to read out and save the new settings.
static void saveParamsCallback()
{
    shouldSaveConfig = 2;
}

// This is called before a firmware updated is initiated.
// Disables WiFi-off-timers and audio, flushes pending save operations
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

static void preUpdateCallback_int()
{
    preUpdateCallback();
    destinationTime.showTextDirect("UPLOADING");
}

// This is called after a firmware updated has finished.
// parm = true of ok, false if error. WM reboots only 
// if the update worked, ie when res is true.
static void postUpdateCallback(bool res)
{
    Serial.flush();
    prepareReboot();

    // WM does not reboot on OTA update errors.
    // However, our preparation destroyed too 
    // much, so we reboot anyway.
    if(!res) {
        delay(1000);
        esp_restart();
    }
}

// Grab static IP and other parameters from WiFiManager's server.
// Since there is no public method for this, we steal the HTML
// form parameters in this callback.
static void preSaveWiFiCallback()
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef TC_DBG
    Serial.println("preSaveWiFiCallback");
    #endif

    // clear as strncpy might leave us unterminated
    memset(ipBuf, 0, 20);
    memset(snBuf, 0, 20);
    memset(gwBuf, 0, 20);
    memset(dnsBuf, 0, 20);

    String temp;
    temp.reserve(16);
    if((temp = wm.server->arg(FPSTR(WMS_ip))) != "") {
        strncpy(ipBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_sn))) != "") {
        strncpy(snBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_gw))) != "") {
        strncpy(gwBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_dns))) != "") {
        strncpy(dnsBuf, temp.c_str(), 19);
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

        shouldSaveIPConfig = (strcmp(ipsettings.ip, ipBuf)      ||
                              strcmp(ipsettings.gateway, gwBuf) ||
                              strcmp(ipsettings.netmask, snBuf) ||
                              strcmp(ipsettings.dns, dnsBuf));
          
        if(shouldSaveIPConfig) {
            strcpy(ipsettings.ip, ipBuf);
            strcpy(ipsettings.gateway, gwBuf);
            strcpy(ipsettings.netmask, snBuf);
            strcpy(ipsettings.dns, dnsBuf);
            #ifdef TC_DBG
            Serial.println("New static IP config");
            #endif
        } else {
            #ifdef TC_DBG
            Serial.println("Static IP config unchanged");
            #endif
        }

    } else {

        #ifdef TC_DBG
        if(strlen(ipBuf) > 0) {
            Serial.println("Invalid IP");
        }
        #endif

        shouldDeleteIPConfig = true;

    }

    // Other parameters on WiFi Config page that
    // need grabbing directly from the server

    getParam("apchnl", settings.apChnl, 2, DEF_AP_CHANNEL);
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

static int menuOutLenCallback()
{
    int numCli = bttfnNumClients();
    uint8_t *ip;
    char *id;
    uint8_t type;
    int mySize = 0;

    if(!haveAudioFiles) {
      
        mySize += 4;  // <hr>
        mySize += STRLEN(menu_myDiv);
        mySize += STRLEN(menu_isp);

    }

    if(numCli > 6) numCli = 6;

    if(numCli) {
        bool hdr = false;

        for(int i = 0; i < numCli; i++) {
    
            if(bttfnGetClientInfo(i, &id, &ip, &type)) {
            
                if(type >= 1 && type <= 6) {
                    if(!hdr) {
                        mySize += STRLEN(menu_myDiv);
                        hdr = true;
                    }

                    mySize += STRLEN(menu_item) + 32;   // IP or hostname
                    mySize += (2 * strlen(menu_tp[type - 1]));
                    mySize += strlen(cliImages[type - 1]);
                }
            }
        }

        if(hdr) {
            mySize += 6; // </div>
        }

    }

    return mySize;
}

static void menuOutCallback(String& page)
{
    int numCli = bttfnNumClients();
    uint8_t *ip;
    char *id;
    uint8_t type;
    char lbuf[20];
    char strBuf[2600];    // ca 2500 with all shown

    strBuf[0] = 0;
    
    if(!haveAudioFiles) {
        strcpy(strBuf, "<hr>");
        strcat(strBuf, menu_myDiv);
        strcat(strBuf, menu_isp);
    }

    if(numCli > 6) numCli = 6;

    if(numCli) {

        bool hdr = false;

        for(int i = 0; i < numCli; i++) {
    
            if(bttfnGetClientInfo(i, &id, &ip, &type)) {
            
                if(type >= 1 && type <= 6) {
                    if(!hdr) {
                        strcat(strBuf, menu_myDiv);
                        hdr = true;
                    }

                    // id is max 12 chars. If maxed out, we assume hostname 
                    // is too long, and we use IP instead
                    if(id[0] && strlen(id) < 12) {
                        strcpy(lbuf, id);
                        strcat(lbuf, ".local");
                    } else {
                        sprintf(lbuf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                    }
                    
                    sprintf(&strBuf[strlen(strBuf)], menu_item, 
                          lbuf,
                          menu_tp[type - 1], 
                          cliImages[type - 1],
                          menu_tp[type - 1]);
                }
            }
        }

        if(hdr) {
            strcat(strBuf, "</div>");
        }

    }

    if(strBuf[0]) {
        page += strBuf;
    }
}

static bool preWiFiScanCallback()
{
    // Do not allow a WiFi scan under some circumstances (as
    // it may disrupt sequences and/or leave BTTFN clients
    // out in the dark for too long)
    
    if(playingIntro || startup || timeTravelP0 || timeTravelP1 || timeTravelRE)
        return false;

    if(timeTravelP2 && bttfnNumClients() > 0)
        return false;

    return true;
}

static void wifiDelayReplacement(unsigned int mydel)
{
    // Called when WM needs to delay to wait for
    // stuff. Must call delay() inside to yield.
    // MUST NOT call wifi_loop() !!!
    if((mydel > 30) && audioInitDone) {
        unsigned long startNow = millis();
        #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
        unsigned long gpsnow = startNow;
        #endif
        while(millis() - startNow < mydel) {
            audio_loop();
            delay(20);
            audio_loop();
            ntp_short_loop();
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE)
            if(millis() - gpsnow > 200) {
                // false = No RotEnc
                gps_loop(false);   // does not call any other loops
                gpsnow = millis();
            }
            #endif
        }
    } else {
        delay(mydel);
    }
}

void gpCallback(int reason)
{
    // Called when WM does stuff that might
    // take some time, like before and after
    // HTTPSend().
    // MUST NOT call wifi_loop() !!!
    
    if(audioInitDone) {
        switch(reason) {
        case WM_LP_PREHTTPSEND:
            if(wifiInAPMode) {
                if(checkMP3Running()) {
                    mp_stop();
                    stopAudio();
                    return;
                }
            } // fall through
        case WM_LP_NONE:
        case WM_LP_POSTHTTPSEND:
            audio_loop();
            yield();
            break;
        }
    }
}

void updateConfigPortalValues()
{
    int tnm = atoi(settings.autoNMPreset);

    // Make sure the settings form has the correct values

    custom_hostName.setValue(settings.hostName, 31);
    custom_wifiConTimeout.setValue(settings.wifiConTimeout, 2);
    custom_wifiConRetries.setValue(settings.wifiConRetries, 2);
    setCBVal(&custom_wifiPRe, settings.wifiPRetry);
    custom_wifiOffDelay.setValue(settings.wifiOffDelay, 2);

    custom_sysID.setValue(settings.systemID, 7);
    custom_appw.setValue(settings.appw, 8);
    // ap channel done on-the-fly
    custom_wifiAPOffDelay.setValue(settings.wifiAPOffDelay, 2);

    setCBVal(&custom_playIntro, settings.playIntro);

    // beep, aint done on-the-fly

    setCBVal(&custom_skTTa, settings.skipTTAnim);
    #ifndef IS_ACAR_DISPLAY
    setCBVal(&custom_p3an, settings.p3anim);
    #endif
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    setCBVal(&custom_alarmRTC, settings.alarmRTC);
    setCBVal(&custom_mode24, settings.mode24);

    custom_timeZone.setValue(settings.timeZone, 63);
    custom_ntpServer.setValue(settings.ntpServer, 63);

    custom_timeZone1.setValue(settings.timeZoneDest, 63);
    custom_timeZone2.setValue(settings.timeZoneDep, 63);
    custom_timeZoneN1.setValue(settings.timeZoneNDest, DISP_LEN);
    custom_timeZoneN2.setValue(settings.timeZoneNDep, DISP_LEN);

    setCBVal(&custom_shuffle, settings.shuffle);

    setCBVal(&custom_dtNmOff, settings.dtNmOff);
    setCBVal(&custom_ptNmOff, settings.ptNmOff);
    setCBVal(&custom_ltNmOff, settings.ltNmOff);
    // AN preset done on-the-fly
    custom_autoNMOn.setValue(settings.autoNMOn, 2);
    custom_autoNMOff.setValue(settings.autoNMOff, 2);
    #ifdef TC_HAVELIGHT
    setCBVal(&custom_uLS, settings.useLight);
    custom_lxLim.setValue(settings.luxLimit, 6);
    #endif

    #ifdef TC_HAVETEMP
    setCBVal(&custom_tempUnit, settings.tempUnit);
    custom_tempOffs.setValue(settings.tempOffs, 4);
    #endif

    #ifdef TC_HAVESPEEDO
    // Speedo type done on-the-fly
    custom_speedoBright.setValue(settings.speedoBright, 2);
    setCBVal(&custom_spAO, settings.speedoAO);
    setCBVal(&custom_sAF, settings.speedoAF);
    custom_speedoFact.setValue(settings.speedoFact, 3);
    setCBVal(&custom_sL0, settings.speedoP3);
    setCBVal(&custom_s3rd, settings.speedo3rdD);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPSS, settings.useGPSSpeed);
    // Update rate done on-the-fly
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useDpTemp, settings.dispTemp);
    custom_tempBright.setValue(settings.tempBright, 2);
    setCBVal(&custom_tempOffNM, settings.tempOffNM);
    #endif
    #endif // HAVESPEEDO

    #ifdef FAKE_POWER_ON
    setCBVal(&custom_fakePwrOn, settings.fakePwrOn);
    #endif
    
    #ifdef EXTERNAL_TIMETRAVEL_IN
    custom_ettDelay.setValue(settings.ettDelay, 5);
    //setCBVal(&custom_ettLong, settings.ettLong);
    #endif
    
    #ifdef TC_HAVEGPS
    setCBVal(&custom_qGPS, settings.quickGPS);
    #endif   

    #ifdef TC_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    custom_mqttServer.setValue(settings.mqttServer, 79);
    custom_mqttUser.setValue(settings.mqttUser, 63);
    custom_mqttTopic.setValue(settings.mqttTopic, 63);
    #ifdef EXTERNAL_TIMETRAVEL_OUT
    setCBVal(&custom_pubMQTT, settings.pubMQTT);
    #endif
    #endif

    #ifdef EXTERNAL_TIMETRAVEL_OUT
    setCBVal(&custom_useETTO, settings.useETTO);
    setCBVal(&custom_noETTOL, settings.noETTOLead);
    #endif

    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);
    setCBVal(&custom_ttrp, settings.timesPers);
    
    setCBVal(&custom_rvapm, settings.revAmPm);
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

static const char *wmBuildbeepaint(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }
    
    char *str = (char *)malloc(720);    // actual length 696

    str[0] = 0;
    buildSelectMenu(str, beepCustHTMLSrc, 6, settings.beep);
    buildSelectMenu(str, aintCustHTMLSrc, 8, settings.autoRotateTimes);
    
    return str;
}

static const char *wmBuildAnmPreset(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }
    
    int tnm = atoi(settings.autoNMPreset);
    char *str = (char *)malloc(500);    // actual length 480

    sprintf(str, anmCustHTML1, settings.autoNMPreset, (tnm == 10) ? custHTMLSel : "", ooe);
    for(int i = 0; i < 5; i++) {
        sprintf(str + strlen(str), anmCustHTMLSrc[i], (tnm == i) ? custHTMLSel : "", (i == 4) ? osde : ooe);
    }

    return str;
}

#ifdef TC_HAVESPEEDO
static const char *wmBuildSpeedoType(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }
    
    int tt = atoi(settings.speedoType);
    char *str = (char *)malloc(780);    // actual length 761

    sprintf(str, "%s%s%s%s%d'%s>%s%s", spTyCustHTML1, settings.speedoType, spTyCustHTML2, spTyOptP1, 99, (tt == 99) ? custHTMLSel : "", "None", spTyOptP3);
    for (int i = SP_MIN_TYPE; i < SP_NUM_TYPES; i++) {
        sprintf(str + strlen(str), "%s%d'%s>%s%s", spTyOptP1, i, (tt == i) ? custHTMLSel : "", dispTypeNames[i], spTyOptP3);
    }
    strcat(str, spTyCustHTMLE);

    return str;
}

#ifdef TC_HAVEGPS
static const char *wmBuildUpdateRate(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }
    
    char *str = (char *)malloc(320);    // actual length 300
    
    str[0] = 0;
    buildSelectMenu(str, spdRateCustHTMLSrc, 6, settings.spdUpdRate);
    
    return str;
}
#endif
#endif

static const char *wmBuildApChnl(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }
    
    char *str = (char *)malloc(600);    // actual length 563

    str[0] = 0;
    buildSelectMenu(str, apChannelCustHTMLSrc, 14, settings.apChnl);
    
    return str;
}

static const char *wmBuildBestApChnl(const char *dest)
{
    if(dest) {
      free((void *)dest);
      return NULL;
    }

    int32_t mychan = 0;
    int qual = 0;

    if(wm.getBestAPChannel(mychan, qual)) {
        char *str = (char *)malloc(STRLEN(bestAP) + 4 + 7 + STRLEN(badWiFi) + 1);
        sprintf(str, bestAP, qual < 0 ? "dc3630" : (qual > 0 ? "609b71" : "777"), mychan, qual < 0 ? badWiFi : "");
        return str;
    }

    return NULL;
}

static const char *wmBuildHaveSD(const char *dest)
{
    if(dest || haveSD)
        return NULL;

    return haveNoSD;
}

/*
 * Audio data uploader
 */

static void doReboot()
{
    delay(1000);
    prepareReboot();
    delay(500);
    esp_restart();
}

static void allocUplArrays()
{
    if(opType) free((void *)opType);
    opType = (int *)malloc(MAX_SIM_UPLOADS * sizeof(int));
    if(ACULerr) free((void *)ACULerr);
    ACULerr = (int *)malloc(MAX_SIM_UPLOADS * sizeof(int));;
    memset(opType, 0, MAX_SIM_UPLOADS * sizeof(int));
    memset(ACULerr, 0, MAX_SIM_UPLOADS * sizeof(int));
}

static void setupWebServerCallback()
{
    wm.server->on(R_updateacdone, HTTP_POST, &handleUploadDone, &handleUploading);
}

static void doCloseACFile(int idx, bool doRemove)
{
    if(haveACFile) {
        closeACFile(acFile);
        haveACFile = false;
    }
    if(doRemove) removeACFile(idx);  
}

static void handleUploading()
{
    HTTPUpload& upload = wm.server->upload();

    if(upload.status == UPLOAD_FILE_START) {

          String c = upload.filename;
          const char *illChrs = "|~><:*?\" ";
          int temp;
          char tempc;

          if(numUploads >= MAX_SIM_UPLOADS) {
            
              haveACFile = false;

              #ifdef TC_DBG
              Serial.printf("handleUploading: Too many files, ignoring %s\n", c.c_str());
              #endif

          } else {

              c.toLowerCase();
    
              #ifdef TC_DBG
              Serial.printf("handleUploading: Filenames: %s %s\n", upload.filename.c_str(), c.c_str());
              #endif
    
              // Remove path and some illegal characters
              tempc = '/';
              for(int i = 0; i < 2; i++) {
                  if((temp = c.lastIndexOf(tempc)) >= 0) {
                      if(c.length() - 1 > temp) {
                          c = c.substring(temp);
                      } else {
                          c = "";
                      }
                      break;
                  }
                  tempc = '\\';
              }
              for(int i = 0; i < strlen(illChrs); i++) {
                  c.replace(illChrs[i], '_');
              }
              if(!c.indexOf("..")) {
                  c.replace("..", "");
              }
    
              #ifdef TC_DBG
              Serial.printf("handleUploading: Filename after sanitation: %s\n", c.c_str());
              #endif
    
              if(!numUploads) {
                  allocUplArrays();
                  preUpdateCallback_int();
              }
    
              haveACFile = openUploadFile(c, acFile, numUploads, haveAC, opType[numUploads], ACULerr[numUploads]);

              if(haveACFile && opType[numUploads] == 1) {
                  haveAC = true;
              }

          }

    } else if(upload.status == UPLOAD_FILE_WRITE) {

          if(haveACFile) {
              if(writeACFile(acFile, upload.buf, upload.currentSize) != upload.currentSize) {
                  doCloseACFile(numUploads, true);
                  ACULerr[numUploads] = UPL_WRERR;
              }
          }

    } else if(upload.status == UPLOAD_FILE_END) {

        if(numUploads < MAX_SIM_UPLOADS) {

            doCloseACFile(numUploads, false);
    
            if(opType[numUploads] >= 0) {
                renameUploadFile(numUploads);
            }
    
            numUploads++;

        }
      
    } else if(upload.status == UPLOAD_FILE_ABORTED) {

        if(numUploads < MAX_SIM_UPLOADS) {
            doCloseACFile(numUploads, true);
        }

        #ifdef TC_DBG
        Serial.printf("handleUploading: Aborted - rebooting\n");
        #endif

        doReboot();

    }

    delay(0);
}

static void handleUploadDone()
{
    const char *ebuf = "ERROR";
    const char *dbuf = "DONE";
    char *buf = NULL;
    bool haveErrs = false;
    bool haveAC = false;
    int titStart = -1;
    int buflen  = strlen(wm.getHTTPSTART(titStart)) +
                  STRLEN(myTitle)    +
                  strlen(wm.getHTTPSCRIPT()) +
                  strlen(wm.getHTTPSTYLE()) +
                  STRLEN(myHead)     +
                  STRLEN(acul_part3) +
                  STRLEN(myTitle)    +
                  STRLEN(acul_part5) +
                  STRLEN(apName)     +
                  STRLEN(acul_part6) +
                  STRLEN(acul_part8) +
                  1;

    #ifdef TC_DBG
    Serial.printf("handleUploadDone: %d files uploaded\n", numUploads);
    #endif

    for(int i = 0; i < numUploads; i++) {
        if(opType[i] > 0) {
            haveAC = true;
            if(!ACULerr[i]) {
                if(!check_if_default_audio_present()) {
                    haveAC = false;
                    ACULerr[i] = UPL_BADERR;
                    removeACFile(i);
                }
            }
            break;
        }
    }    

    if(!haveSD && numUploads) {
      
        buflen += (STRLEN(acul_part71) + strlen(acul_errs[1]));
        
    } else {

        for(int i = 0; i < numUploads; i++) {
            if(ACULerr[i]) haveErrs = true;
        }
        if(haveErrs) {
            buflen += STRLEN(acul_part71);
            for(int i = 0; i < numUploads; i++) {
                if(ACULerr[i]) {
                    buflen += getUploadFileNameLen(i);
                    buflen += 2; // :_
                    buflen += strlen(acul_errs[ACULerr[i]-1]);
                    buflen += 4; // <br>
                }
            }
        } else {
            buflen += strlen(wm.getHTTPSTYLEOK());
            buflen += STRLEN(acul_part7);
        }
        if(haveAC) {
            buflen += STRLEN(acul_part7a);
        }
    }

    buflen += 8;

    if(!(buf = (char *)malloc(buflen))) {
        buf = (char *)(haveErrs ? ebuf : dbuf);
    } else {
        strcpy(buf, wm.getHTTPSTART(titStart));
        if(titStart >= 0) {
            strcpy(buf + titStart, myTitle);
            strcat(buf, "</title>");
        }
        strcat(buf, wm.getHTTPSCRIPT());
        strcat(buf, wm.getHTTPSTYLE());
        if(!haveErrs) {
            strcat(buf, wm.getHTTPSTYLEOK());
        }
        strcat(buf, myHead);
        strcat(buf, acul_part3);
        strcat(buf, myTitle);
        strcat(buf, acul_part5);
        strcat(buf, apName);
        strcat(buf, acul_part6);

        if(!haveSD && numUploads) {

            strcat(buf, acul_part71);
            strcat(buf, acul_errs[1]);
            
        } else {
            
            if(haveErrs) {
                strcat(buf, acul_part71);
                for(int i = 0; i < numUploads; i++) {
                    if(ACULerr[i]) {
                        char *t = getUploadFileName(i);
                        if(t) {
                            strcat(buf, t);
                        }
                        strcat(buf, ": ");
                        strcat(buf, acul_errs[ACULerr[i]-1]);
                        strcat(buf, "<br>");
                    }
                }
            } else {
                strcat(buf, acul_part7);
            }
            if(haveAC) {
                strcat(buf, acul_part7a);
            }
        }

        strcat(buf, acul_part8);
    }

    freeUploadFileNames();
    
    String str(buf);
    wm.server->send(200, "text/html", str);

    // Reboot required even for mp3 upload, because for most files, we check
    // during boot if they exist (to avoid repeatedly failing open() calls)

    doReboot();
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

    while(*str) {

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
    if(!*destBuf) {
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

static void strcpyCB(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, (atoi(el->getValue()) > 0) ? "1" : "0");
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{
    const char makeCheck[] = "1' checked a='";
    
    el->setValue((atoi(sv) > 0) ? makeCheck : "1", 14);
}

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

        if(!mqttMsg) return;
        
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
