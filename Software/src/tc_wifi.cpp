/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * WiFi and Config Portal handling
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

#include "tc_global.h"

#include <Arduino.h>

#include "src/WiFiManager/WiFiManager.h"

#ifndef WM_MDNS
#define TC_MDNS
#include <ESPmDNS.h>
#endif

#include "tcddisplay.h"
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
unsigned long mqttInitialConnectNow = 0;
#endif

static const char R_updateacdone[] = "/uac";

static const char acul_part1[]  = "</style>";
static const char acul_part3[]  = "</head><body><div id='wrap'><h1 id='h1'>";
static const char acul_part5[]  = "</h1><h3 id='h3'>File upload</h3><div class='msg";
static const char acul_part7[]  = " S' id='lc'><strong>Upload complete.</strong><br>Device rebooting.";
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
    "'>Beep mode",
    "bepm",
    ">Off%s1'",
    ">On%s2'",
    ">Auto (30 secs)%s3'",
    ">Auto (60 secs)%s"
};
static const char *aintCustHTMLSrc[8] = {
    "'>Time-cycling interval",
    "tcint",
    ">Off%s1'",
    ">5 minutes%s2'",
    ">10 minutes%s3'",
    ">15 minutes%s4'",
    ">30 minutes%s5'",
    ">60 minutes%s"
};

static const char *alarmCustHTMLSrc[4] = 
{
    "'>Alarm function",
    "afu",
    ">Simple (Legacy)%s1'",
    ">Extended%s"
};

#define WIFI_ANM_PRESETS (1+AUTONM_NUM_PRESETS)
static const char anmCustHTML1[] = "%s%sanmtim'>Schedule%s%s' name='anmtim' id='anmtim'><option value='10'%s>&#10060; Off%s0'";
static const char *anmCustHTMLSrc[WIFI_ANM_PRESETS] = {
    "%s>&#128337; Daily, set hours below%s1'",
    "%s>&#127968; Mo-Th:17-23 Fr:13-1 Sa:9-1 Su:9-23%s2'",
    "%s>&#127970; Mo-Fr:9-17%s3'",
    "%s>&#127970; Mo-Th:7-17 Fr:7-14%s4'",
    "%s>&#128722; Mo-We:8-20 Th-Fr:8-21 Sa:8-17%s5'",
    "%s>&#127747; Su-Th:20-1 Fr-Sa:20-4%s"
};

static const char spTyCustHTML1[] = "spty'>Speedo display type";
static const char spTyCustHTML2[] = "' name='spty' id='spty'>";
static const char spTyCustHTMLE[] = "</select></div>";
static const char spTyOptP1[] = "<option value='";
static const char spTyOptP2[] = "'>";
static const char spTyOptP3[] = "</option>";
static const struct {
    uint8_t    dispType; 
    const char *dispName;
} dispTypeNames[SP_NUM_TYPES+1] = {
    { SP_NONE,          "None" },
    { SP_CIRCSETUP,     "CircuitSetup" },
//  { SP_CIRCSETUP3,    "CircuitSetup (part 3/right)" },
    { SP_ADAF_7x4,      "Adafruit 878 (4x7)" },
    { SP_ADAF_7x4L,     "Adafruit 878 (4x7;left)" },
    { SP_ADAF_B7x4,     "Adafruit 1270 (4x7)" },
    { SP_ADAF_B7x4L,    "Adafruit 1270 (4x7;left)" },
    { SP_ADAF_14x4,     "Adafruit 1911 (4x14)" },
    { SP_ADAF_14x4L,    "Adafruit 1911 (4x14;left)" },
    { SP_GROVE_2DIG14,  "Grove 0.54\" 2x14" },
    { SP_GROVE_4DIG14,  "Grove 0.54\" 4x14" },
    { SP_GROVE_4DIG14L, "Grove 0.54\" 4x14 (left)" },
#ifndef TWPRIVATE
    { SP_ADAF1911_L,    "Ada 1911 (left tube)" },
    { SP_ADAF878L,      "Ada 878 (left tube)" },
#else
    { SP_ADAF1911_L,    "A10001986 wallclock" },
    { SP_ADAF878L,      "A10001986 speedo replica" },
#endif
    { SP_BTTFN,         "Wireless (BTTFN)" }
};

#ifdef TC_HAVEGPS
static const char *spdRateCustHTMLSrc[6] = 
{
    "'>Update rate",
    "spdrt",
    ">1Hz%s1'",
    ">2Hz%s2'",
    ">4Hz%s3'",
    ">5Hz%s"
};
#endif

#ifdef SERVOSPEEDO
static const char *ttinCustHTMLSrc[6] = {
    "",
    "'TT-IN (IO27)' pin</legend>",
    "tinp",
    "is input for external time travel trigger",
    "controls servo-speedo",
    "controls servo-tacho"
};
static const char *ttoutCustHTMLSrc[6] = {
    "",
    "'TT-OUT (IO14)' pin</legend>",
    "toutp",
    "is output as configured below",
    "controls servo-speedo",
    "controls servo-tacho"
};
#endif

static const char *apChannelCustHTMLSrc[14] = {
    "'>WiFi channel",
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

#ifdef TC_HAVEMQTT
static const char *mqttpCustHTMLSrc[4] = {
    "'>Protocol version",
    "mprot",
    ">3.1.1%s1'",
    ">5.0%s"
};
static const char mqttMsgDisabled[] = "Disabled";
static const char mqttMsgConnecting[] = "Connecting...";
static const char mqttMsgTimeout[] = "Connection time-out";
static const char mqttMsgFailed[] = "Connection failed";
static const char mqttMsgDisconnected[] = "Disconnected";
static const char mqttMsgConnected[] = "Connected";
static const char mqttMsgBadProtocol[] = "Protocol error";
static const char mqttMsgUnavailable[] = "Server unavailable/busy";
static const char mqttMsgBadCred[] = "Login failed";
static const char mqttMsgGenError[] = "Error";
#endif
static const char msgResolvErr[] = "DNS lookup error";    // NTP & MQTT

static const char *wmBuildTzlist(const char *dest, int op);
static const char *wmBuildApChnl(const char *dest, int op);
static const char *wmBuildBestApChnl(const char *dest, int op);

static const char *wmBuildbeepaint(const char *dest, int op);
static const char *wmBuildAlm(const char *dest, int op);
static const char *wmBuildAnmPreset(const char *dest, int op);
static const char *wmBuildNTPLUF(const char *dest, int op);
static const char *wmBuildSpeedoType(const char *dest, int op);
#ifdef TC_HAVEGPS
static const char *wmBuildUpdateRate(const char *dest, int op);
#endif
static const char *wmBuildHaveSD(const char *dest, int op);
#ifdef SERVOSPEEDO
static const char *wmBuildttinp(const char *dest, int op);
static const char *wmBuildttoutp(const char *dest, int op);
#endif

#ifdef TC_HAVEMQTT
static const char *wmBuildMQTTprot(const char *dest, int op);
static const char *wmBuildMQTTstate(const char *dest, int op);
static const char *wmBuildMQTTTM(const char *dest, int op);
#endif

static const char custHTMLHdr1[] = "<div class='cmp0";
static const char custHTMLHdrI[] = " ml20";
static const char custHTMLHdr2[] = "'><label class='mp0' for='";
static const char custHTMLSHdr[] = "</label><select class='sel0' value='";
static const char osde[] = "</option></select></div>";
static const char ooe[]  = "</option><option value='";
static const char custHTMLSel[] = " selected";
static const char custHTMLSelFmt[] = "' name='%s' id='%s' autocomplete='off'><option value='0'";
static const char col_g[] = "609b71";
static const char col_r[] = "dc3630";
static const char col_gr[] = "777";
#ifdef SERVOSPEEDO
static const char rad0[] = "<div class='cmp0'><fieldset class='%s' style='border:none;padding:0;'><legend style='padding:0;margin-bottom:2px'>";
static const char rad1[] = "<input type='radio' id='%s%d' name='%s' value='%d'%s style='margin:5px 5px 5px 10px'><label class='mp0' for='%s%d'>%s</label><br>";
static const char radchk[] = " checked";
static const char rad99[] = "</fieldset></div>";
#endif

static const char *tznp1 = "City/location name [a-z/0-9/-/ ]";

static const char bannerStart[] = "<div class='c' style='background-color:#";
static const char bannerMid[] = ";color:#fff;font-size:80%;border-radius:5px'>";

static const char bestAP[]   = "%s%s%sProposed channel at current location: %d<br>%s(Non-WiFi devices not taken into account)</div>";
static const char badWiFi[]  = "<br><i>Operating in AP mode not recommended</i>";

static const char bannerGen[] = "%s%s%s<i>%s</i></div>";
static const char ntpOFF[] = "NTP is inactive";
static const char ntpUNR[] = "NTP server is unresponsive";
static const char haveNoSD[] = "No SD card present";

#ifdef TC_HAVEMQTT
static const char mqttStatus[] = "%s%s%s%s%s (%d)</div>";
#endif

// WiFi Configuration

#if defined(TC_MDNS) || defined(WM_MDNS)
#define HNTEXT "Hostname<br><span>The Config Portal is accessible at http://<i>hostname</i>.local<br>[Valid characters: a-z/0-9/-]</span>"
#else
#define HNTEXT "Hostname<br><span>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9\\-]+' placeholder='Example: timecircuits'", WFM_LABEL_BEFORE|WFM_SECTS_HEAD);

WiFiManagerParameter custom_sectstart_wifi("WiFi connection: Other settings", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_wifiConRetries("wifiret", "Connection attempts (1-10)", settings.wifiConRetries, 2, "type='number' min='1' max='10'");
WiFiManagerParameter custom_wifiPRe("wifiPRet", "Periodic reconnection attempts ", settings.wifiPRetry, "class='mt5 mb10'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_wifiOffDelay("wifioff", "Power save timer<br><span>(10-99[minutes]; 0=off)</span>", settings.wifiOffDelay, 2, "type='number' min='0' max='99' title='WiFi will be shut down after chosen period'");
// custom_wifihint follows (and is foot; next sect must therefore be SECTS_HEAD)

WiFiManagerParameter custom_sectstart_ap("Access point (AP) mode settings", WFM_SECTS_HEAD|WFM_HL);
WiFiManagerParameter custom_sysID("sysID", "Network name (SSID) appendix<br><span>Will be appended to \"TCD-AP\" [a-z/0-9/-]</span>", settings.systemID, 7, "pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_appw("appw", "Password<br><span>Password to protect TCD-AP. Empty or 8 characters [a-z/0-9/-]</span>", settings.appw, 8, "minlength='8' pattern='[A-Za-z0-9\\-]{8}'");
WiFiManagerParameter custom_apch(wmBuildApChnl);
WiFiManagerParameter custom_bapch(wmBuildBestApChnl);
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "Power save timer<br><span>(10-99[minutes]; 0=off)</span>", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='WiFi-AP will be shut down after chosen period'");

WiFiManagerParameter custom_wifihint("<div style='margin:0;padding:0;font-size:80%'>Hold '7' to re-enable Wifi when in power save mode.</div>", WFM_FOOT);

// Settings

WiFiManagerParameter custom_tzsel(wmBuildTzlist); 
// "<datalist id='tzlist'><option value='PST8PDT,M3.2.0,M11.1.0'>Pacific</option><option value='MST7MDT,M3.2.0,M11.1.0'>Mountain</option><option value='CST6CDT,M3.2.0,M11.1.0'>Central</option><option value='EST5EDT,M3.2.0,M11.1.0'>Eastern</option><option value='GMT0BST,M3.5.0/1,M10.5.0'>Western European</option><option value='CET-1CEST,M3.5.0,M10.5.0/3'>Central European</option><option value='EET-2EEST,M3.5.0/3,M10.5.0/4'>Eastern European</option><option value='MSK-3'>Moscow</option><option value='AWST-8'>Australia Western</option><option value='ACST-9:30'>Australia Central/NT</option><option value='ACST-9:30ACDT,M10.1.0,M4.1.0/3'>Australia Central/SA</option><option value='AEST-10AEDT,M10.1.0,M4.1.0/3'>Australia Eastern VIC/NSW</option><option value='AEST-10'>Australia Eastern QL</option><option value='JST-9'>Japan</option></datalist>");

WiFiManagerParameter custom_playIntro("plIn", "Play intro", settings.playIntro, "style='margin-top:3px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);
WiFiManagerParameter custom_beep_aint(wmBuildbeepaint);  // beep + aint
WiFiManagerParameter custom_sARA("sARA", "Animate time-cycling", settings.autoRotAnim, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_skTTa("skTTa", "Skip time travel display disruption", settings.skipTTAnim, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifndef IS_ACAR_DISPLAY
WiFiManagerParameter custom_p3an("p3an", "Date entry animation like in part 3", settings.p3anim, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif
WiFiManagerParameter custom_playTTSnd("plyTTS", "Play time travel sounds", settings.playTTsnds, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_alarmRTC("artc", "Alarm base is real present time", settings.alarmRTC, "title='If unchecked, alarm base is the displayed \"present\" time'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_mode24("md24", "24-hour clock", settings.mode24, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifndef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travel persistent", settings.timesPers, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif

WiFiManagerParameter custom_timeZone("tzx", "Time zone (in <a href='https://tz.out-a-ti.me' target=_blank>Posix</a> format)", settings.timeZone, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tzlist'", WFM_LABEL_BEFORE|WFM_SECTS);
WiFiManagerParameter custom_ntpServer("ntps", "NTP server", settings.ntpServer, 63, "pattern='[a-zA-Z0-9\\.\\-]+' placeholder='Example: pool.ntp.org'");
WiFiManagerParameter custom_NTPLUF(wmBuildNTPLUF);
#ifdef TC_HAVEGPS
WiFiManagerParameter custom_gpstime("gTm", "Use GPS time", settings.useGPSTime, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif

WiFiManagerParameter custom_sectstart_wc("World Clock mode", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_timeZone1("tz1", "Time zone for Destination Time display", settings.timeZoneDest, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tzlist'");
WiFiManagerParameter custom_timeZone2("tz2", "Time zone for Last Time Departed display", settings.timeZoneDep, 63, "list='tzlist'");
WiFiManagerParameter custom_timeZoneN1("tzn1", tznp1, settings.timeZoneNDest, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO' style='margin-bottom:15px'");
WiFiManagerParameter custom_timeZoneN2("tzn2", tznp1, settings.timeZoneNDep, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+'");
WiFiManagerParameter custom_wcNamePerm("WCNP", "Show names permanently", settings.WCNamePerm, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_alm(wmBuildAlm, WFM_SECTS);
WiFiManagerParameter custom_almadv("<div style='margin:10px 0 5px 0;padding:0;'>Settings for Extended</div>");
WiFiManagerParameter custom_almSnz("aDS", "Snooze", settings.doSnooze, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_almST("aST", "minutes snooze time (1-15)", settings.snoozeTime, 2, "class='ml20' style='margin-right:5px;width:5em' type='number' min='1' max='15'", WFM_LABEL_AFTER|WFM_NO_BR);
WiFiManagerParameter custom_almASnz("aAS", "Auto snooze", settings.autoSnooze, "class='mt5 mb10 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_almLUS("aLUS", "Loop user-provided Alarm sound<br><span>If looped, sound will run for 2 minutes</span>", settings.almLoopUserSnd, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_sectstart_nm("Night mode", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination Time off", settings.dtNmOff, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present Time off", settings.ptNmOff, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last Time Departed off", settings.ltNmOff, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_autoNMTimes(wmBuildAnmPreset);
WiFiManagerParameter custom_autoNMOn("anmon", "Daily night-mode start hour (0-23)", settings.autoNMOn, 2, "type='number' min='0' max='23' title='Hour to switch on night-mode'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Daily night-mode end hour (0-23)", settings.autoNMOff, 2, "type='number' min='0' max='23' title='Hour to switch off night-mode'");
#ifdef TC_HAVELIGHT
WiFiManagerParameter custom_uLS("uLS", "Use light sensor", settings.useLight, "title='If checked, TCD will be put in night-mode if lux level is below or equal threshold.' style='margin-top:14px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_lxLim("lxLim", "Lux threshold (0-50000)", settings.luxLimit, 6, "type='number' min='0' max='50000'");
#endif

WiFiManagerParameter custom_haveSD(wmBuildHaveSD, WFM_SECTS);
WiFiManagerParameter custom_CfgOnSD("CfgSD", "Save secondary settings on SD<br><span>Check this to avoid flash wear</span>", settings.CfgOnSD, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifdef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travel persistent<br><span>Requires SD card</span>", settings.timesPers, "class='mt5 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span>Checking this might help in case of SD card problems</span>", settings.sdFreq, WFM_LABEL_AFTER|WFM_IS_CHKBOX);

#ifdef IS_ACAR_DISPLAY
WiFiManagerParameter custom_swapDL("swapDL", "Swap red and yellow displays like B-Car", settings.swapDL, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS);
WiFiManagerParameter custom_rvapm("rvapm", "Reverse AM/PM like in parts 2/3", settings.revAmPm, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_FOOT);
#else
WiFiManagerParameter custom_rvapm("rvapm", "Reverse AM/PM like in parts 2/3", settings.revAmPm, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS|WFM_FOOT);
#endif

// Peripherals settings

WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch", settings.fakePwrOn, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);

WiFiManagerParameter custom_speedoType(wmBuildSpeedoType, WFM_SECTS);
WiFiManagerParameter custom_speedoBright("speBri", "Speed brightness (0-15)", settings.speedoBright, 2, "type='number' min='0' max='15'");
WiFiManagerParameter custom_spAO("spAO", "Switch speedo off when idle", settings.speedoAO, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_sAF("sAF", "Real-life acceleration figures", settings.speedoAF, "title='If unchecked, movie-like times are used'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_speedoFact("speFac", "Factor for real-life figures (0.5-5.0)", settings.speedoFact, 3, "type='number' min='0.5' max='5.0' step='0.5' title='1.0 means real-world DMC-12 acceleration time.'");
WiFiManagerParameter custom_sL0("sL0", "Speedo display like in part 3", settings.speedoP3, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_s3rd("s3rd", "Display '0' after dot like A-car", settings.speedo3rdD, "title='CircuitSetup speedo only'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifdef TC_HAVEGPS
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed", settings.dispGPSSpeed, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_updrt(wmBuildUpdateRate);
#endif // TC_HAVEGPS
#ifdef TC_HAVETEMP
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature", settings.dispTemp, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_tempBright("temBri", "Temperature brightness (0-15)", settings.tempBright, 2, "type='number' min='0' max='15'");
WiFiManagerParameter custom_tempOffNM("toffNM", "Temperature off in night mode", settings.tempOffNM, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif // TC_HAVETEMP

#ifdef TC_HAVETEMP
WiFiManagerParameter custom_sectstart_te("Temperature/humidity sensor", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_tempUnit("temUnt", "Show temperature in °Celsius", settings.tempUnit, "class='mt5 mb10'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_tempOffs("tOffs", "Temperature offset (-3.0-3.0)", settings.tempOffs, 4, "type='number' min='-3.0' max='3.0' step='0.1'");
#endif // TC_HAVETEMP

#ifdef SERVOSPEEDO
WiFiManagerParameter custom_ttin(wmBuildttinp, WFM_SECTS);
WiFiManagerParameter custom_ttinhl("<div style='margin:10px 0 5px 0;padding:0;'>External time travel trigger</div>");
#else
WiFiManagerParameter custom_sectstart_et("External time travel trigger (TT-IN)", WFM_SECTS|WFM_HL);
#endif
WiFiManagerParameter custom_ettDelay("ettDe", "Delay (ms)", settings.ettDelay, 5, "type='number' min='0' max='60000'");

#ifdef SERVOSPEEDO
WiFiManagerParameter custom_ttout(wmBuildttoutp, WFM_SECTS);
WiFiManagerParameter custom_ttouthl("<div style='margin:10px 0 5px 0;padding:0;'>'TT-OUT' output</div>");
#else
WiFiManagerParameter custom_sectstart_etto("TT-OUT (IO14)", WFM_SECTS|WFM_HL);
#endif
WiFiManagerParameter custom_ETTOcmd("EtCd", "is controlled by commands 990/991", settings.ETTOcmd, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ETTOPUS("EtPU", "Power-up state HIGH", settings.ETTOpus, "class='mt5 mb10 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_useETTO("uEtto", "signals time travel", settings.useETTO, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_noETTOL("uEtNL", "Signal without 5 second lead", settings.noETTOLead, "class='mt5 mb10 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ETTOalm("EtAl", "signals alarm", settings.ETTOalm, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifdef TC_HAVEGPS
#define TEMP_FOOT 0
#else
#define TEMP_FOOT WFM_FOOT
#endif
WiFiManagerParameter custom_TTOAlmDur("taldu", "seconds signal duration (3-99)", settings.ETTOAD, 2, "class='ml20' style='margin-right:5px;width:5em' type='number' min='3' max='99'", WFM_LABEL_AFTER|TEMP_FOOT);

#ifdef TC_HAVEGPS
WiFiManagerParameter custom_sectstart_bt("Wireless communication (BTTF-Network)", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_qGPS("qGPS", "Provide GPS speed to BTTFN clients", settings.provGPS2BTTFN, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_FOOT);
#endif // TC_HAVEGPS

// HA/MQTT Settings

#ifdef TC_HAVEMQTT
WiFiManagerParameter custom_useMQTT("uMQTT", "Home Assistant support (MQTT)", settings.useMQTT, "class='mt5' style='margin-bottom:10px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);
WiFiManagerParameter custom_state(wmBuildMQTTstate);
WiFiManagerParameter custom_mqttServer("MQs", "Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9\\.:\\-]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttVers(wmBuildMQTTprot);
WiFiManagerParameter custom_mqttUser("MQu", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
WiFiManagerParameter custom_mqttTopic("MQt", "Topic to show on Destination Time display", settings.mqttTopic, 63, "placeholder='Optional. Example: home/alarm/status'", WFM_LABEL_BEFORE|WFM_SECTS);
WiFiManagerParameter custom_mqttTopicP("MQtP", "Topic to show on Present Time display", settings.mqttTopicP, 63, "", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_mqttTopicL("MQtL", "Topic to show on Last Time Departed display", settings.mqttTopicL, 63, "", WFM_LABEL_BEFORE);
WiFiManagerParameter custom_pubMQTT("MQpu", "Publish time travel and alarm events", settings.pubMQTT, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS);
WiFiManagerParameter custom_vMQTT("MQpv", "Enhanced Time Travel notification", settings.MQTTvarLead, "class='mt5 ml20' title='Check to send time travel with variable lead.'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_mqttPwr("MQp", "HA controls Fake-Power at startup", settings.mqttPwr, "class='mt5' title='Check to have HA control Fake-Power and take precendence over Fake-Power switch at power-up'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS);
WiFiManagerParameter custom_mqttPwrOn("MQpo", "Wait for POWER_ON at startup", settings.mqttPwrOn, "class='mt5 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_FOOT);
WiFiManagerParameter custom_mqtttm(wmBuildMQTTTM);
#endif // HAVEMQTT

static const int8_t wifiMenu[] = { 
    WM_MENU_WIFI,
    WM_MENU_PARAM,
    WM_MENU_PARAM2,
    #ifdef TC_HAVEMQTT
    WM_MENU_PARAM3,
    #endif
    WM_MENU_SEP_F,
    WM_MENU_UPDATE,
    WM_MENU_SEP,
    WM_MENU_CUSTOM,
    WM_MENU_END
};
#define TC_MENUSIZE (sizeof(wifiMenu) / sizeof(wifiMenu[0]))

#define AA_TITLE "Time Circuits"
#define AA_ICON "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAD1BMVEWOkJHMy8fyJT322DFDtiSDPmFqAAAAKklEQVQI12MQhAIGAQYwYGQQUAIDbAyEGhcwwGQgqzEGA0wGXA2CAXcGAJ79DMHwFaKYAAAAAElFTkSuQmCC"
#define AA_CONTAINER "TCDA"
#define UNI_VERSION TC_VERSION 
#define UNI_VERSION_EXTRA TC_VERSION_EXTRA
#define WEBHOME "tcd"
#define CURRVERSION TC_VERSION_REV
static const char apName[]  = "TCD-AP";

#if defined(IS_ACAR_DISPLAY) && defined(GTE_KEYPAD)
#define _RFW "A-Car + GTE";
#elif defined(IS_ACAR_DISPLAY)
#define _RFW "A-Car";
#elif defined(GTE_KEYPAD)
#define _RFW "GTE";
#else
#define _RFW "Standard";
#endif
static const char rfw[] = _RFW;

static const char myTitle[] = AA_TITLE;
static const char myHead[]  = "<link rel='icon' type='image/png' href='data:image/png;base64," AA_ICON "'><script>window.onload=function(){xxx='" AA_TITLE "';yyy='?';wr=ge('wrap');if(wr){aa=ge('h3');if(aa){yyy=aa.innerHTML;aa.remove();dlel('h1')}zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\" onClick=\"shsp(1);window.location=\\'/\\'\"><div class=\"tpm2\"><img id=\"spi\" src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQMAAACQp+OdAAAABlBMVEUAAABKnW0vhlhrAAAAAXRSTlMAQObYZgAAA'+(zz?'GBJREFUKM990aEVgCAABmF9BiIjsIIbsJYNRmMURiASePwSDPD0vPT12347GRejIfaOOIQwigSrRHDKBK9CCKoEqQF2qQMOSQAzEL9hB9ICNyMv8DPKgjCjLtAD+AV4dQM7O4VX9m1RYAAAAABJRU5ErkJggg==':'HtJREFUKM990bENwyAUBuFnuXDpNh0rZIBIrJUqMBqjMAIlBeIihQIF/fZVX39229PscYG32esCzeyjsXUzNHZsI0ocxJ0kcZIOsoQjnxQJT3FUiUD1NAloga6wQQd+4B/7QBQ4BpLAOZAn3IIy4RfUibCgTTDq+peG6AvsL/jPTu1L9wAAAABJRU5ErkJggg==')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.4em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:5em\"':'')+'>'+yyy+'</div></div>';wr.insertBefore(dd,wr.firstChild);wr.style.position='relative'}var lc=ge('lc');if(lc){lc.style.transform='rotate('+(358+[0,1,3,4,5][Math.floor(Math.random()*4)])+'deg)'}}</script><style>H1{font-family:Bahnschrift,-apple-system,'Segoe UI Semibold',Roboto,'Helvetica Neue',Arial,Verdana,sans-serif;margin:0;text-align:center;}H3{margin:0 0 5px 0;text-align:center;}input{border:thin inset}em > small{display:inline}form{margin-block-end:0;}.tpm{background-color:#fff;cursor:pointer;border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;font-weight:normal;margin-left:2.2em;overflow:clip;}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7.2em;margin-left:0.5em;margin-right:0.5em;border-radius:5px;overflow:hidden;white-space:nowrap}.tpm0{position:relative;width:20em;padding:5px 0px 5px 0px;margin:0 auto 0 auto;}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}.mt5{margin-top:5px!important}.mb10{margin-bottom:10px!important}.mb0{margin-bottom:0px!important}.mb15{margin-bottom:15px!important}.ml20{margin-left:20px}.ss>label span{font-size:80%}</style>";
static const char* myCustMenu = "<a href='https://circuitsetup.us' target=_blank><img style='display:block;margin:10px auto 5px auto;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQsAAAAsCAMAAABFVW1aAAAAQlBMVEUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACO4fbyAAAAFXRSTlMAgMBAd0Twu98QIDGuY1KekNBwiMxE8vI7AAAG9klEQVRo3uSY3Y7cIAyFA5EQkP9IvP+r1sZn7CFMdrLtZa1qMxBjH744QDq0lke2abjaNMLycGdJHAaz8VwnvZX0MtRLhm+2mOPLkrpmFsMNWG6UvLw7g1c2ZcjgToSFTRDqRFipFlztdUVsuyTwBea0y7zDJoHEPEiuobZqavoxiseCVh27cgyLWV42VtdT7npuWHpTogPi+iYmLtCLWUGZKSpbZl+IZW4Rui3RJgFhR3rKAt4WKEw18XsggXDyeHFMdez2I4vjIQvFBlve9W7GYtTwDYscNAg8EMNZ1scsIAaBAHuIBbZLgwZi+gtdpBF+ZFHyYxZwtYa3WMriKLChYbEVmNSF9+wXRVl0Dq2WRXRsth403l4yOuchZuHrtvOEEw9nJLM4OvwlW68sNseWZfonfDN1QcDYKOF4bg/qGt2Ocb7eidSYXyxyVUSBkNyx0fMP7OTmEuAoOifkFlapYSH9rcGbgUcNtMgUZ4FwSmuvjl4QU2MGi+3KAqiFxSEZNFWnRKojBQRExSOVa5XpErQuknyACa9hcjqFbM8BL/v4lAUiI1ASgSRpQ6a9ehzcRyY6wSLcs2DLj1igCx4zTR85WmWjhu9YnJaVuyEeAcdfsdiKFRgEJsmAgbiH+fkXdUq5/sji/C0LTPOWxfmZxdyw0IBWF9NjFpGiNXWxGMy5VsRETf7juZdvSakQ/nsWPpT5X1ns+pREQ1g/sWDfexYzx7iwCJ5subKIsnatGuhkjGBhWblJfY49bYc4SrywODjJVINFEpE6FqZEWWS8h/07kqJVJXY2n1+qOMguAyjv+JFFlPV3+7inuisLdCOQbkFXFpGaRIlxICFLd4St21PtWEb/ehamBPvIVt6X/YS1s94JHOVyvgh2dGBPPV/sysJ2OlhIv2ARJwTCXHoWnjnRL8o52ilqouY9i1TK/JWFnWHsMZ7qxalsiqeGNxZ2HD37ukCIPDxjEbwvoG/Hzp7FRkPnEqk+/KnvtadmvGfB1fudBX43j9G85qQsyKaj3r+wGPJcG7lnEXyUeE/Xzux1hfKcrGMhl9mTsy9HnTzG7tR9u4/wUWX7tnZGj927O4NHH+GqLFAaI1SZjXJe+7B2Tgj4iAVyTQgUpGBtH9GNiUTvPPmNs75lumeRiPHXfQQ7urKYR3g5ZlmysjAYZ8eiCnHqGHQxxib5OxajBFJlryl6dTm4x2FftUz3LCrI7yxW++D1ptcOOS0L7nM9Cxbi4YhaQMCdGumvWEAZcCK1rgUrO0UuIs30I4vlOws74vYshqNdO9nWri4MERzTYbs+wGCOHQvrhXfajIUq28RrBxrq5g68FDZ2+pFFesICpdizkBciKwtPQtLcrRc7DmVg4T2S9idJYxE82269to/YSVeVrdx5MIFgy3+S+ojNmbU/a/lJvxh7FqYELFCKn1hkcQBYWWj1P098NVbAuwWWPJi9xXhJWhYwf2EBm6/fqcPRbHiMCGUDyZqp31OtyM6ehSkBi+ZTCtZ/pzIy2P4ufMgWz1iclxVg+QWLYJWYcGadAgaYq0eg/ZLpnkV+wAJfDD2L5oOAqYsdqWGx2LEILOI8NikDXR+z8LueaKBMTzAB86wpPerDXTLdsxiOrywQe/nIIr8vie5gQVWrsaDRtYLnPPxn9ocdOxYAAABAANafv28EGWwYO43fBgAAWDNWszMpCATr0E0Dih6g3v9VV76eBXWdwx5MvjqMTVI/Y2WAZH49crVd0SECh0oGsugxHChntq62F1etx1PKwXOSytI91Neirh8jUFar+eY01f5I8mN+kQwOillVvIjIA9n/q4FDqIBSjqGjTvLGjgUaeEBAg7LDIIwHgeJriuvHiLWP4e401Z7U+JO/nSQjofryRZQN0oNTsIcuFFvgJBsXLNILlKxr9i7kI+oDJak2qua/erhVYdtyqXenqfak2hXGs2RwGBO2glfhYbvJYxewcxf+cgtXX1+6sJDQF664dbFywcBwmurRQQW4XiSzi4zXIdygzKcumln7vEuO4cxkrMm/3egimhk6l/XahRt5XaPRm5OrZ1Jo2ChnyeCsZBO8iyUYEAWnLn4gPsQNE6WR7dZFRxfpznzvouOhi+nkjJm0MlXms2RwkkSy4k0soSVIUN25/LNH9v2eno2qrA97RFNYv+4RYwIenFw9kwpLi1eJc3yMxHvwKiDssIfzIjIja/6QD2rtLx0WoNy7gPC5C68VSBscw2mqPQmZK+tFMhK6V3u1C2Mzq32S53uEu180HS3YGkKCMFhjmedFdVH82kUKbBYiHMNpqj2pW3C7Sj6cxLgaDS/Cxg/i6z2iowuJn/NDAmnLPC/MudvXLrAYGdxmOl3V1j8qeZN8OGn3zP/BH6Wx/qV3/+q3AAAAAElFTkSuQmCC'></a><div style='font-size:0.75em;line-height:1.2em;font-weight:bold;text-align:center;text-transform:uppercase'>" UNI_VERSION " (" UNI_VERSION_EXTRA ")<br>Powered by <a href='https://out-a-ti.me' target=_blank>A10001986</a> <a href='https://" WEBHOME ".out-a-ti.me' target=_blank>[Home/Updates]</a></div>";
#ifdef CS_EDITION
static const char r_link[]  = "github.com/CircuitSetup/Time-Circuits-Display/releases";
#else
static const char r_link[]  = WEBHOME "r.out-a-ti.me";
#endif

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
const char *menu_tp[]   = { "Flux Capacitor", "SID", "Dash Gauges", "VSR", "AUX", "Remote" };
const char menu_myDiv[] = "<div style='margin-left:auto;margin-right:auto;text-align:center;'>";
const char menu_item[]  = "<a href='http://%s' target=_blank title='%s'><img style='zoom:2;padding:0 5px 0 5px;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQ%s' alt='%s'></a>";

static char newversion[8];
static unsigned long lastUpdateCheck = 0;
static unsigned long lastUpdateLiveCheck = 0;

#define WLA_IP      1
#define WLA_DEL_IP  2
#define WLA_WIFI    4
#define WLA_SET1    8
#define WLA_SET1_B      3
#define WLA_SET2    16
#define WLA_SET2_B      4
#define WLA_SET3    32
#define WLA_SET3_B      5
#define WLA_SET     (WLA_WIFI|WLA_SET1|WLA_SET2|WLA_SET3)
#define WLA_ANY     (WLA_IP|WLA_DEL_IP|WLA_SET)
static uint32_t     wifiLoopSaveAction = 0;

// Did user configure a WiFi network to connect to?
bool wifiHaveSTAConf = false;

bool carMode = false;

static unsigned long lastConnect = 0;
static unsigned long consecutiveAPmodeFB = 0;

static bool ntpLUF = false;

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
bool          blockWiFiSTAPS = false;

static File acFile;
static bool haveACFile = false;
static bool haveAC = false;
static int  numUploads = 0;
static int  *ACULerr = NULL;
static int  *opType = NULL;

bool                 pubMQTT = false;
#ifdef TC_HAVEMQTT
#define MQTT_SHORT_INT (30*1000)
#define MQTT_LONG_INT  (5*60*1000)
static const char    emptyStr[1] = { 0 };
static char          mqnmt[] = "haXt";
static char          mqnmm[] = "haXm";
bool                 useMQTT = false;
bool                 MQTTvarLead = false;
static char          *mqttUser = (char *)emptyStr;
static char          *mqttPass = (char *)emptyStr;
static char          *mqttServer = (char *)emptyStr;
uint8_t              haveMQTTaudio = 0;
const char           *mqttAudioFile[3] = { "/ha-alert.mp3", "/ha-alert-p.mp3", "/ha-alert-l.mp3" };
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

static unsigned int wmLenBuf = 0;

static void wifiOff(bool force);
static void wifiConnect(bool deferConfigPortal = false);
static void wifi_ntp_setup(bool doUseNTP);
static void checkForUpdate();

static void saveParamsCallback(int);
static void saveWiFiCallback(const char *ssid, const char *pass, const char *bssid);
static void preUpdateCallback();
static void postUpdateCallback(bool);
static int  menuOutLenCallback();
static void menuOutCallback(String& page, unsigned int ssize);
static void wifiDelayReplacement(unsigned int mydel);
static void gpCallback(int);
static bool preWiFiScanCallback();

static void updateConfigPortalValues();

static bool isIp(char *str);
static IPAddress stringToIp(char *str);

static void getServerParam(const char *name, char *destBuf, size_t length, int minval, int maxval, int defaultVal);
static bool myisspace(char mychar);
static char *strcpytrim(char* destination, const char* source, bool doFilter = false);
static char *strcpyfilter(char* destination, const char* source);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
static void mystrcpyWiFiDelay(char *sv, WiFiManagerParameter *el);
static void evalCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);

static void setupWebServerCallback();
static void handleUploadDone();
static void handleUploading();
static void handleUploadDone();

#ifdef TC_HAVEMQTT
static void strcpyutf8(char *dst, const char *src, unsigned int len);
static void handleMQTTTopMsg(int idx);
static void mqttPing();
static bool mqttReconnect(bool force = false);
static void mqttLooper();
static void mqttCallback(char *topic, byte *payload, unsigned int length);
static void mqttSubscribe();
#endif

#ifdef TC_HAVEMQTT
static void initMQTTMsg(int idx)
{
    if((mqttMsg[idx] = (char *)malloc(256))) {
        memset(mqttMsg[idx], 0, 256);
        if(check_file_SD(mqttAudioFile[idx])) haveMQTTaudio |= (1 << idx);
    }
}
#endif

/*
 * wifi_setup()
 *
 */
void wifi_setup()
{
    int temp;

    WiFiManagerParameter *wifiParmArray[] = {

      &custom_hostName,

      &custom_sectstart_wifi,
      &custom_wifiConRetries,
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

      NULL
      
    };
    
    WiFiManagerParameter *parmArray[] = {

      &custom_tzsel,

      &custom_playIntro,      
      &custom_beep_aint,
      &custom_sARA,
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

      &custom_timeZone, 
      &custom_ntpServer,
      &custom_NTPLUF,
    #ifdef TC_HAVEGPS
      &custom_gpstime,
    #endif

      &custom_sectstart_wc, 
      &custom_timeZone1, &custom_timeZoneN1,
      &custom_timeZone2, &custom_timeZoneN2,
      &custom_wcNamePerm,

      &custom_alm,
      &custom_almadv,
      &custom_almSnz,
      &custom_almST,
      &custom_almASnz,
      &custom_almLUS,
      
      &custom_sectstart_nm, 
      &custom_dtNmOff, &custom_ptNmOff, &custom_ltNmOff,
      &custom_autoNMTimes, 
      &custom_autoNMOn, &custom_autoNMOff,
    #ifdef TC_HAVELIGHT
      &custom_uLS, &custom_lxLim,
    #endif

      &custom_haveSD,
      &custom_CfgOnSD,
      #ifdef PERSISTENT_SD_ONLY
      &custom_ttrp,
      #endif
      //&custom_sdFrq,

      #ifdef IS_ACAR_DISPLAY
      &custom_swapDL,
      #endif
      &custom_rvapm,
      
      NULL
    };

    WiFiManagerParameter *parm2Array[] = {

      &custom_fakePwrOn,

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

    #ifdef TC_HAVETEMP
      &custom_sectstart_te, 
      &custom_tempUnit, 
      &custom_tempOffs,
    #endif
    
    #ifdef SERVOSPEEDO
      &custom_ttin,
      &custom_ttinhl,
    #else
      &custom_sectstart_et,
    #endif
      &custom_ettDelay,

    #ifdef SERVOSPEEDO
      &custom_ttout,
      &custom_ttouthl,
    #else
      &custom_sectstart_etto,
    #endif
      &custom_ETTOcmd,
      &custom_ETTOPUS,
      &custom_useETTO,
      &custom_noETTOL,
      &custom_ETTOalm,
      &custom_TTOAlmDur,

    #ifdef TC_HAVEGPS
      &custom_sectstart_bt,
      &custom_qGPS,
    #endif
      NULL
    };

    #ifdef TC_HAVEMQTT
    WiFiManagerParameter *parm3Array[] = {

      &custom_useMQTT,
      &custom_state,
      &custom_mqttServer, 
      &custom_mqttVers,
      &custom_mqttUser, 
      &custom_mqttTopic,
      &custom_mqttTopicP,
      &custom_mqttTopicL,
      &custom_pubMQTT,
      &custom_vMQTT,
      &custom_mqttPwr,
      &custom_mqttPwrOn,

      &custom_mqtttm,

      NULL
    };
    #endif

    // Transition from NVS-saved data to own management:
    if(!settings.ssid[0] && settings.ssid[1] == 'X') {
        
        // Read NVS-stored WiFi data
        wm.getStoredCredentials(settings.ssid, sizeof(settings.ssid), settings.pass, sizeof(settings.pass));

        #ifdef TC_DBG_WIFI
        Serial.printf("WiFi Transition: ssid '%s' pass '%s'\n", settings.ssid, settings.pass);
        #endif

        write_settings();
    }

    wm.setHostname(settings.hostName);

    wm.showUploadContainer(haveSD, AA_CONTAINER, rspv, haveAudioFiles);
    wm.setReqFirmwareVersion(rfw);

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
    else if(temp > 11) temp = 11;
    if(!temp) temp = random(1, 11);
    wm.setWiFiAPChannel(temp);

    temp = atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    else if(temp > 10) temp = 10;
    wm.setConnectRetries(temp);

    wm.setMenu(wifiMenu, TC_MENUSIZE, false);

    // WiFi Settings
    wm.allocParms(WM_PARM_WIFI, (sizeof(wifiParmArray) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(wifiParmArray[temp]) {
        wm.addParameter(WM_PARM_WIFI, wifiParmArray[temp]);
        temp++;
    }

    // Settings
    wm.allocParms(WM_PARM_SETTINGS, (sizeof(parmArray) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(parmArray[temp]) {
        wm.addParameter(WM_PARM_SETTINGS, parmArray[temp]);
        temp++;
    }

    // Peripherals
    wm.allocParms(WM_PARM_SETTINGS2, (sizeof(parm2Array) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(parm2Array[temp]) {
        wm.addParameter(WM_PARM_SETTINGS2, parm2Array[temp]);
        temp++;
    }

    // HA/MQTT
    #ifdef TC_HAVEMQTT
    wm.allocParms(WM_PARM_SETTINGS3, (sizeof(parm3Array) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(parm3Array[temp]) {
        wm.addParameter(WM_PARM_SETTINGS3, parm3Array[temp]);
        temp++;
    }
    #endif

    updateConfigPortalValues();

    if(!carMode) {
        wifiHaveSTAConf = (settings.ssid[0] != 0);
        #ifdef TC_DBG_WIFI
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
    #ifdef TC_DBG_WIFI
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
    doAPretry = evalBool(settings.wifiPRetry);

    // Configure static IP
    if(loadIpSettings()) {
        if(checkIPConfig()) {
            IPAddress ip = stringToIp(ipsettings.ip);
            IPAddress gw = stringToIp(ipsettings.gateway);
            IPAddress sn = stringToIp(ipsettings.netmask);
            IPAddress dns = stringToIp(ipsettings.dns);
            wm.setSTAStaticIPConfig(ip, gw, sn, dns);
        }
    }
           
    // Connect
    wifiConnect(deferredCP);

    // MDNS. Needs to be called AFTER mode(STA) or softAP init
    #ifdef TC_MDNS
    if(MDNS.begin(settings.hostName)) {
        MDNS.addService("http", "tcp", 80);
    }
    #endif

    checkForUpdate();

#ifdef TC_HAVEMQTT
    useMQTT = evalBool(settings.useMQTT);
    
    if((!settings.mqttServer[0]) || // No server -> no MQTT
       (wifiInAPMode))              // WiFi in AP mode -> no MQTT
        useMQTT = false;  
    
    if(useMQTT) {

        uint16_t mqttPort = 1883;
        char *t;
        int tt;

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
                /*
                mqttClient.setServer(mqttServer, mqttPort);
                // Disable PING if we can't resolve domain
                mqttDoPing = false;
                */
                useMQTT = false;
                mqttReconnFails = 1; // Abuse for "resolv error"
                Serial.printf("MQTT: Failed to resolve '%s'\n", mqttServer);
            }
        }

        #ifdef TC_DBG_MQTT
        Serial.printf("MQTT: server '%s' port %d\n", mqttServer, mqttPort);
        #endif

    }

    if(useMQTT) {

        char *t;

        pubMQTT = evalBool(settings.pubMQTT);
        MQTTvarLead = pubMQTT ? evalBool(settings.MQTTvarLead) : false;
        //MQTTPwrMaster = evalBool(settings.mqttPwr);
        evalBoolSetClear(settings.mqttPwr, csf, CSF_MQTTPM);
        
        MQTTWaitForOn = (csf & CSF_MQTTPM) ? evalBool(settings.mqttPwrOn) : false;

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
        mqttClient.setVersion(atoi(settings.mqttVers) > 0 ? 5 : 3);
        mqttClient.setClientID(settings.hostName);

        if(*settings.mqttTopic)  initMQTTMsg(0);
        if(*settings.mqttTopicP) initMQTTMsg(1);
        if(*settings.mqttTopicL) initMQTTMsg(2);

        mqttClient.setCallback(mqttCallback);
        mqttClient.setLooper(mqttLooper);

        if(*settings.mqttUser) {
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

        #ifdef TC_DBG_MQTT
        Serial.printf("MQTT: user '%s' pass '%s'\n", mqttUser, mqttPass);
        #endif

        mqttReconnect(true);
        mqttInitialConnectNow = millisNonZero();
        
        // Rest done in loop
            
    } else {

        pubMQTT = MQTTvarLead = false;
        csf &= ~CSF_MQTTPM;
        MQTTWaitForOn = false;
        

        #ifdef TC_DBG_MQTT
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
                    mqttPingNow = mqttRestartPing ? millisNonZero() : 0;
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
                mqttInitialConnectNow = 0;
            }
        }
        mqttClient.loop();

        // Time-out waiting for MQTT connection upon boot in case MQTT 
        // has control over fake-power and we wait for POWER_ON.
        // User can then disable MQTT power control by 996ENTER.
        if(mqttInitialConnectNow && (csf & CSF_MQTTPM) && MQTTWaitForOn) {
            if(millis() - mqttInitialConnectNow > 40*1000) {
                mqttInitialConnectNow = 0;
                mqttFakePowerOn();
            }
        }
    }
#endif

    if(millis() - lastUpdateCheck > 24*60*60*1000) {
        if(!(csf & (CSF_ST|CSF_P0|CSF_P1|CSF_P2|CSF_RE|CSF_AL|CSF_AE))) {
            if(checkAudioDone()) {
                checkForUpdate();
            }
        }
    }

    if(wifiLoopSaveAction & WLA_SET) {

        int temp;

        mp_stop();
        stopAudio();
        ettoPulseEnd();
        send_abort_msg();

        // Save settings and restart esp32

        #ifdef TC_DBG_WIFI
        Serial.println("Config Portal: Saving config");
        #endif

        if(wifiLoopSaveAction & WLA_WIFI) {

            // Parameters on WiFi Config page
            // Note: Parameters that need to be grabbed from the server directly
            // through getServerParam() must be handled in saveWiFiCallback().
            
            // Static IP
            if(wifiLoopSaveAction & WLA_IP) {
                #ifdef TC_DBG_WIFI
                Serial.println("WiFi: Saving IP config");
                #endif
                writeIpSettings();
            } else if(wifiLoopSaveAction & WLA_DEL_IP) {
                #ifdef TC_DBG_WIFI
                Serial.println("WiFi: Deleting IP config");
                #endif
                deleteIpSettings();
            }

            // ssid, pass, bssid copied to settings in saveWiFiCallback()

            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(!*settings.hostName) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            evalCB(settings.wifiPRetry, &custom_wifiPRe);
            mystrcpyWiFiDelay(settings.wifiOffDelay, &custom_wifiOffDelay);
            
            strcpytrim(settings.systemID, custom_sysID.getValue(), true);
            strcpytrim(settings.appw, custom_appw.getValue(), true);
            if((temp = strlen(settings.appw)) > 0) {
                if(temp < 8) {
                    settings.appw[0] = 0;
                }
            }
            mystrcpyWiFiDelay(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);
          
        } else if(wifiLoopSaveAction & WLA_SET1) {

            // Parameters on Settings page
            // Note: Parameters that need to be grabbed from the server directly
            // through getServerParam() must be handled in saveParamsCallback().

            evalCB(settings.playIntro, &custom_playIntro);
            evalCB(settings.autoRotAnim, &custom_sARA);
            evalCB(settings.skipTTAnim, &custom_skTTa);
            #ifndef IS_ACAR_DISPLAY
            evalCB(settings.p3anim, &custom_p3an);
            #endif
            evalCB(settings.playTTsnds, &custom_playTTSnd);
            evalCB(settings.alarmRTC, &custom_alarmRTC);
            evalCB(settings.mode24, &custom_mode24);

            strcpytrim(settings.timeZone, custom_timeZone.getValue());
            strcpytrim(settings.ntpServer, custom_ntpServer.getValue());
            #ifdef TC_HAVEGPS
            evalCB(settings.useGPSTime, &custom_gpstime);
            #endif

            strcpytrim(settings.timeZoneDest, custom_timeZone1.getValue());
            strcpytrim(settings.timeZoneDep, custom_timeZone2.getValue());
            strcpyfilter(settings.timeZoneNDest, custom_timeZoneN1.getValue());
            if(*settings.timeZoneNDest) {
                char *s = settings.timeZoneNDest;
                for ( ; *s; ++s) *s = toupper(*s);
            }
            strcpyfilter(settings.timeZoneNDep, custom_timeZoneN2.getValue());
            if(*settings.timeZoneNDep) {
                char *s = settings.timeZoneNDep;
                for ( ; *s; ++s) *s = toupper(*s);
            }
            evalCB(settings.WCNamePerm, &custom_wcNamePerm);

            evalCB(settings.doSnooze, &custom_almSnz);
            mystrcpy(settings.snoozeTime, &custom_almST);
            evalCB(settings.autoSnooze, &custom_almASnz);
            evalCB(settings.almLoopUserSnd, &custom_almLUS);

            evalCB(settings.dtNmOff, &custom_dtNmOff);
            evalCB(settings.ptNmOff, &custom_ptNmOff);
            evalCB(settings.ltNmOff, &custom_ltNmOff);
            mystrcpy(settings.autoNMOn, &custom_autoNMOn);
            mystrcpy(settings.autoNMOff, &custom_autoNMOff);
            #ifdef TC_HAVELIGHT
            evalCB(settings.useLight, &custom_uLS);
            mystrcpy(settings.luxLimit, &custom_lxLim);
            #endif

            oldCfgOnSD = settings.CfgOnSD[0];
            evalCB(settings.CfgOnSD, &custom_CfgOnSD);
            //evalCB(settings.sdFreq, &custom_sdFrq);
            evalCB(settings.timesPers, &custom_ttrp);

            #ifdef IS_ACAR_DISPLAY
            evalCB(settings.swapDL, &custom_swapDL);
            #endif
            evalCB(settings.revAmPm, &custom_rvapm);

            autoInterval = (uint8_t)atoi(settings.autoRotateTimes);
            beepMode = (uint8_t)atoi(settings.beep);
            saveBeepAutoInterval();

            // Copy SD-saved settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                moveSettings();
            }

        } else if(wifiLoopSaveAction & WLA_SET2) {

            // Parameters on "Peripherals" page
            // Note: Parameters that need to be grabbed from the server directly
            // through getServerParam() must be handled in saveParamsCallback().

            #ifdef TC_HAVETEMP
            evalCB(settings.tempUnit, &custom_tempUnit);
            mystrcpy(settings.tempOffs, &custom_tempOffs);
            #endif

            mystrcpy(settings.speedoBright, &custom_speedoBright);
            evalCB(settings.speedoAO, &custom_spAO);
            evalCB(settings.speedoAF, &custom_sAF);
            mystrcpy(settings.speedoFact, &custom_speedoFact);
            evalCB(settings.speedoP3, &custom_sL0);
            evalCB(settings.speedo3rdD, &custom_s3rd);
            #ifdef TC_HAVEGPS
            evalCB(settings.dispGPSSpeed, &custom_useGPSS);
            #endif
            #ifdef TC_HAVETEMP
            evalCB(settings.dispTemp, &custom_useDpTemp);
            mystrcpy(settings.tempBright, &custom_tempBright);
            evalCB(settings.tempOffNM, &custom_tempOffNM);
            #endif

            evalCB(settings.fakePwrOn, &custom_fakePwrOn);

            mystrcpy(settings.ettDelay, &custom_ettDelay);

            evalCB(settings.ETTOcmd, &custom_ETTOcmd);
            evalCB(settings.useETTO, &custom_useETTO);
            evalCB(settings.ETTOalm, &custom_ETTOalm);
            evalCB(settings.ETTOpus, &custom_ETTOPUS);
            evalCB(settings.noETTOLead, &custom_noETTOL);
            mystrcpy(settings.ETTOAD, &custom_TTOAlmDur);

            #ifdef TC_HAVEGPS
            evalCB(settings.provGPS2BTTFN, &custom_qGPS);
            #endif

        } else if(wifiLoopSaveAction & WLA_SET3) {

            // Parameters on HA/MQTT Settings page
            // Note: Parameters that need to be grabbed from the server directly
            // through getServerParam() must be handled in saveParamsCallback().

            #ifdef TC_HAVEMQTT
            evalCB(settings.useMQTT, &custom_useMQTT);
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            strcpyutf8(settings.mqttTopic, custom_mqttTopic.getValue(), sizeof(settings.mqttTopic));
            strcpyutf8(settings.mqttTopicP, custom_mqttTopicP.getValue(), sizeof(settings.mqttTopicP));
            strcpyutf8(settings.mqttTopicL, custom_mqttTopicL.getValue(), sizeof(settings.mqttTopicL));
            evalCB(settings.pubMQTT, &custom_pubMQTT);
            evalCB(settings.MQTTvarLead, &custom_vMQTT);
            evalCB(settings.mqttPwr, &custom_mqttPwr);
            evalCB(settings.mqttPwrOn, &custom_mqttPwrOn);
            #endif

        }

        write_settings();

        // Reset esp32 to load new settings

        #ifdef TC_DBG_WIFI
        Serial.println("Config Portal: Restarting ESP....");
        #endif
        Serial.flush();

        prepareReboot();
        delay(1000);
        esp_restart();
    }

    wm.process();

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
        if((wifiAPOffDelay > 0) && !bttfnHaveClients) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                if(!WiFi.softAPgetStationNum()) {
                    wifiOff(false);
                    wifiAPIsOff = true;
                    wifiIsOff = false;
                    syncTrigger = false;
                    #ifdef TC_DBG_WIFI
                    Serial.println("WiFi (AP-mode) switched off (power-save)");
                    #endif
                } else {
                    wifiAPModeNow += (wifiAPOffDelay / 10);   // Check again later
                    #ifdef TC_DBG_WIFI
                    Serial.println("WiFi (AP-mode) NOT switched off (power-save) - client connected to AP");
                    #endif
                }
            }
        }
    } else {
        // Disable WiFi in STA mode after a configurable delay (if > 0)
        if(origWiFiOffDelay > 0 && !bttfnHaveClients && !blockWiFiSTAPS) {
            if(!wifiIsOff && (millis() - wifiOnNow >= wifiOffDelay)) {
                wifiOff(false);
                wifiIsOff = true;
                wifiAPIsOff = false;
                syncTrigger = false;
                #ifdef TC_DBG_WIFI
                Serial.println("WiFi (STA-mode) switched off (power-save)");
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
        wm.startAPModeAndPortal(realAPName, settings.appw, settings.ssid, settings.pass, settings.bssid);
        doOnlyAP = true;
    }
    
    // Connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(!doOnlyAP && wm.wifiConnect(settings.ssid, settings.pass, settings.bssid, realAPName, settings.appw)) {
        #ifdef TC_DBG_WIFI
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
        // with MQTT, let alone the Remote.
        WiFi.setSleep(false);

        // Set transmit power to max; we might be connecting as STA after
        // a previous period in AP mode.
        #ifdef TC_DBG_WIFI
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

        wifi_ntp_setup(true);

    } else {

        #ifdef TC_DBG_WIFI
        Serial.println("Config portal running in AP-mode");
        #endif

        {
            #ifdef TC_DBG_WIFI
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

            #ifdef TC_DBG_WIFI
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

        wifi_ntp_setup(false);
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

    // Parm for disableWiFi() is "waitForOFF"
    // which should be true if we stop in AP
    // mode and immediately re-connect, without
    // process()ing for a while after this call.
    // "force" is true if we want to try to
    // reconnect after disableWiFi(), false if 
    // we disconnect upon timer expiration, 
    // so it matches the purpose.
    // "false" also does not cause any delays,
    // while "true" may take up to 2 seconds.
    wm.disableWiFi(force);
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
                // Note: In carMode, wifiHaveSTAConf is false
                if(!wifiHaveSTAConf) {
                    // Best we can do is to restart the timer
                    wifiAPModeNow = Now;
                    return;
                }

                // Make sure we don't cause stutter (beep might be running)
                stopAudio();

                // If ON and User has config's a NW, disable WiFi at this point
                // (in hope of successful connection below)
                wifiOff(true);

            }

        } else {            // We are in STA mode

            // If WiFi is not off, check if caller wanted
            // to start the CP, and do so, if not running
            if(!wifiIsOff && (WiFi.status() == WL_CONNECTED)) {
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

            #ifdef TC_DBG_WIFI
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

                // Make sure we don't cause stutter (beep might be running)
                stopAudio();

                // If ON, disable WiFi at this point
                // (in hope of successful connection below)
                wifiOff(true);

            }

        } else {            // We are in STA mode

            // If WiFi is not off, check if caller wanted
            // to start the CP, and do so, if not running
            if(!wifiIsOff && (WiFi.status() == WL_CONNECTED)) {
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

    // Make sure we don't cause stutter; beep might still be running
    stopAudio();

    // (Re)connect
    wifiConnect(deferCP);

    // Restart timers
    // Note that wifiInAPMode now reflects the
    // result of our above wifiConnect() call

    if(wifiInAPMode) {

        #ifdef TC_DBG_WIFI
        Serial.println("wifiOn: in AP mode after connect");
        #endif
      
        wifiAPModeNow = Now;
        
        #ifdef TC_DBG_WIFI
        if(wifiAPOffDelay > 0) {
            Serial.printf("Restarting WiFi-off timer (AP mode); delay %d\n", wifiAPOffDelay);
        }
        #endif
        
    } else {

        #ifdef TC_DBG_WIFI
        Serial.println("wifiOn: in STA mode after connect");
        #endif

        if(origWiFiOffDelay != 0) {
            desiredDelay = (newDelay > 0) ? newDelay : origWiFiOffDelay;
            if((Now - wifiOnNow >= wifiOffDelay) ||                    // If delay has run out, or
               (wifiOffDelay - (Now - wifiOnNow))  < desiredDelay) {   // new delay exceeds remaining delay:
                wifiOffDelay = desiredDelay;                           // Set new timer delay, and
                wifiOnNow = Now;                                       // restart timer
                #ifdef TC_DBG_WIFI
                Serial.printf("Restarting WiFi-off timer; delay %d\n", wifiOffDelay);
                #endif
            }
        }

        if(millis() - lastUpdateLiveCheck > 6*60*60*1000) {
            checkForUpdate();
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
        if(!wifiIsOff && (WiFi.status() == WL_CONNECTED)) 
            return false;
    }

    return true;
}

void wifiRestartPSTimer()
{
    if(wifiInAPMode) {
        wifiAPModeNow = millis();
    } else {
        wifiOnNow = millis();
    }
}

void wifiStartCP()
{
    if(wifiInAPMode || wifiIsOff)
        return;

    #ifdef TC_DBG_WIFI
    Serial.println("Starting CP");
    #endif

    wm.startWebPortal();
}

static void wifi_ntp_setup(bool doUseNTP)
{
    IPAddress remote_addr;
    bool couldHaveNTP;

    // Re-do lookup on every connect
    ntpLUF = false;

    if(doUseNTP) {
        doUseNTP = (settings.ntpServer[0] != 0);
    }
    if(doUseNTP) {
        if(isIp(settings.ntpServer)) {
            remote_addr = stringToIp(settings.ntpServer);
        } else if(!WiFi.hostByName(settings.ntpServer, remote_addr)) {
            doUseNTP = false;
            ntpLUF = true;
            #ifdef TC_DBG_TIME
            Serial.printf("NTP: Failed to look up %s, NTP disabled\n", settings.ntpServer);
            #endif
        }
    }

    // Do not include ntpLUF here, will be checked on each connect()
    couldHaveNTP = (wifiHaveSTAConf && settings.ntpServer[0]);
        
    ntp_setup(doUseNTP, remote_addr, couldHaveNTP, ntpLUF);
}

static void checkForUpdate()
{
    #ifdef CS_EDITION
    #define _UPDD "tcdv.circuitsetup.us"
    #else
    #define _UPDD "tcdv.out-a-ti.me"
    #endif
    int cver = 0, crev = 0, uver = 0, urev = 0;
    bool haveCVer = false;

    *newversion = 0;

    lastUpdateCheck = millis();

    if(sscanf(CURRVERSION, "V%d.%d", &cver, &crev) != 2) {
        lastUpdateLiveCheck = millis();
        return;
    }
    
    if(WiFi.status() == WL_CONNECTED) {
        IPAddress remote_addr;
        if(WiFi.hostByName(_UPDD, remote_addr)) {
            uver = remote_addr[0]; urev = remote_addr[1];
            if(uver) saveUpdVers(uver, urev);
        }
        lastUpdateLiveCheck = millis();
    } else {
        loadUpdVers(uver, urev);
    }
    #undef _UPDD

    if(uver) {
        haveCVer = true;
        if(((uver << 8) | urev) > ((cver << 8) | crev)) {
            snprintf(newversion, sizeof(newversion), "%d.%d", uver, urev);
        }
    }

    wm.setDownloadLink(r_link, haveCVer, (*newversion) ? newversion : NULL);
}

bool updateAvailable()
{
    return (*newversion != 0);
}

bool checkIPConfig()
{
    return (*ipsettings.ip            &&
            isIp(ipsettings.ip)       &&
            isIp(ipsettings.gateway)  &&
            isIp(ipsettings.netmask)  &&
            isIp(ipsettings.dns));
}

// This is called when the WiFi config is to be saved. 
// SSID, password and BSSID are copied to settings here.
// Static IP and other parameters are taken from WiFiManager's server.
static void saveWiFiCallback(const char *ssid, const char *pass, const char *bssid)
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef TC_DBG_WIFI
    Serial.println("saveWiFiCallback");
    #endif

    // clear as strncpy might leave us unterminated
    memset(ipBuf, 0, 20);
    memset(snBuf, 0, 20);
    memset(gwBuf, 0, 20);
    memset(dnsBuf, 0, 20);

    String temp;
    temp.reserve(24);
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

    #ifdef TC_DBG_WIFI
    if(*ipBuf) {
        Serial.printf("IP:%s / SN:%s / GW:%s / DNS:%s\n", ipBuf, snBuf, gwBuf, dnsBuf);
    } else {
        Serial.println("Static IP unset, using DHCP");
    }
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef TC_DBG_WIFI
        Serial.println("All IPs valid");
        #endif

        wifiLoopSaveAction |= WLA_IP;
          
        memset((void *)&ipsettings, 0, sizeof(ipsettings));
        strcpy(ipsettings.ip, ipBuf);
        strcpy(ipsettings.gateway, gwBuf);
        strcpy(ipsettings.netmask, snBuf);
        strcpy(ipsettings.dns, dnsBuf);

    } else {

        #ifdef TC_DBG_WIFI
        if(*ipBuf) {
            Serial.println("Invalid static IP setup");
        }
        #endif

        wifiLoopSaveAction |= WLA_DEL_IP;

    }
   
    // ssid is the (new?) ssid to connect to, pass the password.
    // (We don't need to compare to the old ones, the settings
    // hash will decide on save/not save.)
    // This is also used to "forget" a saved WiFi network, in
    // which case ssid and pass are empty strings.
    clearWiFiCredentials();
    if(*ssid) {
        strncpy(settings.ssid, ssid, sizeof(settings.ssid) - 1);
        strncpy(settings.pass, pass, sizeof(settings.pass) - 1);
        strncpy(settings.bssid, bssid, sizeof(settings.bssid) - 1);
    }

    #ifdef TC_DBG_WIFI
    Serial.printf("saveWiFiCallback: New ssid '%s'\n", settings.ssid);
    Serial.printf("saveWiFiCallback: New pass '%s'\n", settings.pass);
    Serial.printf("saveWiFiCallback: New bssid '%s'\n", settings.bssid);
    #endif

    // Other parameters on WiFi Config page that
    // need grabbing directly from the server

    getServerParam("apchnl", settings.apChnl, 2, 0, 11, DEF_AP_CHANNEL);
    
    wifiLoopSaveAction |= WLA_WIFI;
}

// This is the callback from the actual Params page. We set
// a flag for the loop to read out and save the new settings.
// paramspage is 1 (settings), 2 (peripherals), or 3 (HA/MQTT settings)
static void saveParamsCallback(int paramspage)
{
    wifiLoopSaveAction |= (1 << (paramspage - 1 + WLA_SET1_B));

    switch(paramspage) {
    case 1:
        getServerParam("bepm", settings.beep, 1, 0, 3, DEF_BEEP);
        getServerParam("tcint", settings.autoRotateTimes, 1, 0, 5, DEF_AUTOROTTIMES);
        getServerParam("afu", settings.alarmType, 1, 0, 1, 0);
        getServerParam("anmtim", settings.autoNMPreset, 2, 0, 10, DEF_AUTONM_PRESET);
        break;
    case 2:
        getServerParam("spty", settings.speedoType, 2, 0, 99, DEF_SPEEDO_TYPE);
        #ifdef TC_HAVEGPS
        getServerParam("spdrt", settings.spdUpdRate, 1, 0, 3, DEF_SPD_UPD_RATE);
        #endif
        #ifdef SERVOSPEEDO
        getServerParam("tinp", settings.ttinpin, 1, 0, 2, 0);
        getServerParam("toutp", settings.ttoutpin, 1, 0, 2, 0);
        #endif
        break;
    case 3:
        #ifdef TC_HAVEMQTT
        getServerParam("mprot", settings.mqttVers, 1, 0, 1, 0);
        for(int i = 0; i < 10; i++) handleMQTTTopMsg(i);
        #endif
        break;
    }
}

// This is called before a firmware updated is initiated.
// Disables WiFi-off-timers and audio, flushes pending save operations
static void preUpdateCallback()
{
    wifiAPOffDelay = 0;
    origWiFiOffDelay = 0;

    mp_stop();
    stopAudio();

    ettoPulseEnd();
    send_abort_msg();

    flushDelayedSave();

    allOff();
    if(sgf & SGF_USpeedoDisp) speedo.off();
    
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

static int menuOutLenCallback()
{
    int numCli = bttfnNumClients();
    uint8_t *ip;
    char *id;
    uint8_t type;
    int mySize = 0;

    if(numCli > 6) numCli = 6;

    if(numCli) {
        bool hdr = false;

        for(int i = 0; i < numCli; i++) {
    
            if(bttfnGetClientInfo(i, &id, &ip, &type)) {
            
                if(type >= BTTFN_TYPE__MIN && type <= BTTFN_TYPE__MAX) {
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

static void menuOutCallback(String& page, unsigned int ssize)
{
    int numCli = bttfnNumClients();
    uint8_t *ip;
    char *id;
    uint8_t type;
    char lbuf[20];
    char *strBuf = (char *)malloc(ssize + 8);

    if(!strBuf) return;

    strBuf[0] = 0;
    
    if(numCli > 6) numCli = 6;

    if(numCli) {

        bool hdr = false;

        for(int i = 0; i < numCli; i++) {
    
            if(bttfnGetClientInfo(i, &id, &ip, &type)) {
            
                if(type >= BTTFN_TYPE__MIN && type <= BTTFN_TYPE__MAX) {
                    if(!hdr) {
                        strcat(strBuf, menu_myDiv);
                        hdr = true;
                    }

                    // id is max 13 chars. If id[12] == '.' it is maxed out, 
                    // hostname is too long, and we use IP instead.
                    // (using strcpy is safe, there are 14 bytes in buffer, 0-term)
                    if(id[0] && (strlen(id) < 13 || id[12] != '.')) {
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

    free(strBuf);
}

static bool preWiFiScanCallback()
{
    // Do not allow a WiFi scan under some circumstances (as
    // it may disrupt sequences and/or leave BTTFN clients
    // out in the dark for too long)

    if((csf & (CSF_NS|CSF_ST|CSF_P0|CSF_P1|CSF_RE|CSF_P2)) || autoIntAnimRunning)
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
        while(millis() - startNow < mydel) {
            audio_loop();
            delay(20);
            audio_loop();
            ntp_short_loop();
            #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
            // speedoUpdate_loop(false) => in sync with loops => No RotEnc, no Remote
            speedoUpdate_loop(false);   // does not call any other loops
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
        case WM_LP_NONE:
        case WM_LP_POSTHTTPSEND:
            audio_loop();
            break;
        }
    }
}

static void updateConfigPortalValues()
{
    // Make sure the settings form has the correct values

    custom_hostName.setValue(settings.hostName);
    custom_wifiConRetries.setValue(settings.wifiConRetries);
    setCBVal(&custom_wifiPRe, settings.wifiPRetry);
    custom_wifiOffDelay.setValue(settings.wifiOffDelay);

    custom_sysID.setValue(settings.systemID);
    custom_appw.setValue(settings.appw);
    // ap channel done on-the-fly
    custom_wifiAPOffDelay.setValue(settings.wifiAPOffDelay);

    setCBVal(&custom_playIntro, settings.playIntro);
    // beep, aint done on-the-fly
    setCBVal(&custom_sARA, settings.autoRotAnim);
    setCBVal(&custom_skTTa, settings.skipTTAnim);
    #ifndef IS_ACAR_DISPLAY
    setCBVal(&custom_p3an, settings.p3anim);
    #endif
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    setCBVal(&custom_alarmRTC, settings.alarmRTC);
    setCBVal(&custom_mode24, settings.mode24);

    custom_timeZone.setValue(settings.timeZone);
    custom_ntpServer.setValue(settings.ntpServer);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_gpstime, settings.useGPSTime);
    #endif

    custom_timeZone1.setValue(settings.timeZoneDest);
    custom_timeZone2.setValue(settings.timeZoneDep);
    custom_timeZoneN1.setValue(settings.timeZoneNDest);
    custom_timeZoneN2.setValue(settings.timeZoneNDep);
    setCBVal(&custom_wcNamePerm, settings.WCNamePerm);

    setCBVal(&custom_almSnz, settings.doSnooze);
    custom_almST.setValue(settings.snoozeTime);
    setCBVal(&custom_almASnz, settings.autoSnooze);
    setCBVal(&custom_almLUS, settings.almLoopUserSnd);

    setCBVal(&custom_dtNmOff, settings.dtNmOff);
    setCBVal(&custom_ptNmOff, settings.ptNmOff);
    setCBVal(&custom_ltNmOff, settings.ltNmOff);
    // AN preset done on-the-fly
    custom_autoNMOn.setValue(settings.autoNMOn);
    custom_autoNMOff.setValue(settings.autoNMOff);
    #ifdef TC_HAVELIGHT
    setCBVal(&custom_uLS, settings.useLight);
    custom_lxLim.setValue(settings.luxLimit);
    #endif

    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);
    setCBVal(&custom_ttrp, settings.timesPers);

    #ifdef IS_ACAR_DISPLAY
    setCBVal(&custom_swapDL, settings.swapDL);
    #endif
    setCBVal(&custom_rvapm, settings.revAmPm);

    setCBVal(&custom_fakePwrOn, settings.fakePwrOn);

    // Speedo type done on-the-fly
    custom_speedoBright.setValue(settings.speedoBright);
    setCBVal(&custom_spAO, settings.speedoAO);
    setCBVal(&custom_sAF, settings.speedoAF);
    custom_speedoFact.setValue(settings.speedoFact);
    setCBVal(&custom_sL0, settings.speedoP3);
    setCBVal(&custom_s3rd, settings.speedo3rdD);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPSS, settings.dispGPSSpeed);
    // Update rate done on-the-fly
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useDpTemp, settings.dispTemp);
    custom_tempBright.setValue(settings.tempBright);
    setCBVal(&custom_tempOffNM, settings.tempOffNM);
    #endif

    #ifdef TC_HAVETEMP
    setCBVal(&custom_tempUnit, settings.tempUnit);
    custom_tempOffs.setValue(settings.tempOffs);
    #endif

    // tt-out, tt-in done on-the-fly
    
    custom_ettDelay.setValue(settings.ettDelay);
    
    #ifdef TC_HAVEGPS
    setCBVal(&custom_qGPS, settings.provGPS2BTTFN);
    #endif   

    setCBVal(&custom_ETTOcmd, settings.ETTOcmd);
    setCBVal(&custom_useETTO, settings.useETTO);
    setCBVal(&custom_ETTOalm, settings.ETTOalm);
    setCBVal(&custom_ETTOPUS, settings.ETTOpus);
    setCBVal(&custom_noETTOL, settings.noETTOLead);
    custom_TTOAlmDur.setValue(settings.ETTOAD);

    #ifdef TC_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    custom_mqttServer.setValue(settings.mqttServer);
    // protocol version done on-the-fly
    custom_mqttUser.setValue(settings.mqttUser);
    custom_mqttTopic.setValue(settings.mqttTopic);
    custom_mqttTopicP.setValue(settings.mqttTopicP);
    custom_mqttTopicL.setValue(settings.mqttTopicL);
    setCBVal(&custom_pubMQTT, settings.pubMQTT);
    setCBVal(&custom_vMQTT, settings.MQTTvarLead);
    setCBVal(&custom_mqttPwr, settings.mqttPwr);
    setCBVal(&custom_mqttPwrOn, settings.mqttPwrOn);
    // MQTT topic/msg done on-the-fly
    #endif
}

static unsigned int calcSelectMenu(const char **theHTML, int cnt, char *setting, bool indent = false)
{
    int sr = atoi(setting);

    unsigned int l = 0;

    l += STRLEN(custHTMLHdr1);
    if(indent) l += STRLEN(custHTMLHdrI);
    l += STRLEN(custHTMLHdr2);
    l += strlen(theHTML[0]);
    l += STRLEN(custHTMLSHdr);
    l += strlen(setting);
    l += (STRLEN(custHTMLSelFmt) - (2*2));
    l += (3*strlen(theHTML[1]));
    for(int i = 0; i < cnt - 2; i++) {
        if(sr == i) l += STRLEN(custHTMLSel);
        l += (strlen(theHTML[i+2]) - 2);
        l += strlen((i == cnt - 3) ? osde : ooe);
    }

    return l + 8;
}

static void buildSelectMenu(char *target, const char **theHTML, int cnt, char *setting, bool indent = false)
{
    int sr = atoi(setting);

    strcpy(target, custHTMLHdr1);
    if(indent) strcat(target, custHTMLHdrI);
    strcat(target, custHTMLHdr2);
    strcat(target, theHTML[1]);
    strcat(target, theHTML[0]);
    strcat(target, custHTMLSHdr);
    strcat(target, setting);
    sprintf(target + strlen(target), custHTMLSelFmt, theHTML[1], theHTML[1]);
    for(int i = 0; i < cnt - 2; i++) {
        if(sr == i) strcat(target, custHTMLSel);
        sprintf(target + strlen(target), 
            theHTML[i+2], (i == cnt - 3) ? osde : ooe);
    }
}

static const char *wmBuildSelect(const char *dest, int op, const char **src, int count, char *setting, bool indent = false)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(src, count, setting, indent);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, src, count, setting, indent);
    
    return str;
}

#ifdef SERVOSPEEDO
static unsigned int lengthRadioButtons(const char **theHTML, int cnt, char *setting)
{
    unsigned int mysize = STRLEN(rad0) + strlen(theHTML[0]) + strlen(theHTML[1]);
    int i, j = strlen(theHTML[2]), sr = atoi(setting);
    
    for(i = 0; i < cnt; i++) {
        mysize += STRLEN(rad1) + (3*j) + (3*2) + ((i==sr) ? STRLEN(radchk) : 0) + strlen(theHTML[3+i]);
    }
    mysize += STRLEN(rad99);

    return mysize;
}

static void buildRadioButtons(char *target, const char **theHTML, int cnt, char *setting)
{
    int i, sr = atoi(setting);
    
    sprintf(target, rad0, theHTML[0]);
    strcat(target, theHTML[1]);
    
    for(i = 0; i < cnt; i++) {
        sprintf(target+strlen(target), rad1, theHTML[2], i, theHTML[2], i, (i==sr) ? radchk : "", theHTML[2], i, theHTML[3+i]);
    }
    strcat(target, rad99);
}

static const char *wmBuildRadioButtons(const char *dest, int op, const char **theHTML, int cnt, char *setting)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = lengthRadioButtons(theHTML, cnt, setting);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildRadioButtons(str, theHTML, cnt, setting);
    
    return str;
}
#endif

static const char *wmBuildTzlist(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

#define TZLISTLEN 844   // Don't waste space calculating this   

    if(op == WM_CP_LEN) {
        wmLenBuf = TZLISTLEN;
        return (const char *)&wmLenBuf;
    }

    char *str = (char *)malloc(TZLISTLEN);

    sprintf(str, "<datalist id='tzlist'><option value='PST8PDT,M3.2.0,M11.1.0'>Pacific%sMST7MDT,M3.2.0,M11.1.0'>Mountain%sCST6CDT,M3.2.0,M11.1.0'>Central%sEST5EDT,M3.2.0,M11.1.0'>Eastern%sGMT0BST,M3.5.0/1,M10.5.0'>Western European%sCET-1CEST,M3.5.0,M10.5.0/3'>Central European%sEET-2EEST,M3.5.0/3,M10.5.0/4'>Eastern European%sMSK-3'>Moscow%sAWST-8'>Australia Western%sACST-9:30'>Australia Central/NT%sACST-9:30ACDT,M10.1.0,M4.1.0/3'>Australia Central/SA%sAEST-10AEDT,M10.1.0,M4.1.0/3'>Australia Eastern VIC/NSW%sAEST-10'>Australia Eastern QL%sJST-9'>Japan</option></datalist>",
        ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe, ooe);

    return str;
}

static const char *wmBuildbeepaint(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(beepCustHTMLSrc, 6, settings.beep);
    l += calcSelectMenu(aintCustHTMLSrc, 8, settings.autoRotateTimes);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, beepCustHTMLSrc, 6, settings.beep);
    buildSelectMenu(str + strlen(str), aintCustHTMLSrc, 8, settings.autoRotateTimes);
    
    return str;
}

static const char *wmBuildAlm(const char *dest, int op)
{
    return wmBuildSelect(dest, op, alarmCustHTMLSrc, 4, settings.alarmType, false);
}

static const char *wmBuildAnmPreset(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = STRLEN(anmCustHTML1) - (6*2) + 
                     STRLEN(custHTMLHdr1) + STRLEN(custHTMLHdr2) + STRLEN(custHTMLSHdr) +
                     2 + STRLEN(custHTMLSel) + STRLEN(ooe) + 4;
    
    for(int i = 0; i < WIFI_ANM_PRESETS; i++) {
        l +=  strlen(anmCustHTMLSrc[i]) - 4 + ((i == (WIFI_ANM_PRESETS-1)) ? STRLEN(osde) : STRLEN(ooe));
    }

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    int tnm = atoi(settings.autoNMPreset);
    char *str = (char *)malloc(l);

    sprintf(str, anmCustHTML1, custHTMLHdr1, custHTMLHdr2, custHTMLSHdr, 
                               settings.autoNMPreset, (tnm == 10) ? custHTMLSel : "", ooe);
    for(int i = 0; i < WIFI_ANM_PRESETS; i++) {
        sprintf(str + strlen(str), anmCustHTMLSrc[i], (tnm == i) ? custHTMLSel : "", (i == (WIFI_ANM_PRESETS-1)) ? osde : ooe);
    }

    return str;
}

static const char *wmBuildSpeedoType(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }
    
    int tt = atoi(settings.speedoType);

    unsigned long l = STRLEN(custHTMLHdr1) + STRLEN(custHTMLHdr2) + 
                      STRLEN(spTyCustHTML1) + STRLEN(custHTMLSHdr) +
                      strlen(settings.speedoType) + STRLEN(spTyCustHTML2);

    l += STRLEN(custHTMLSel) + STRLEN(spTyCustHTMLE);
    for(int i = 0; i < SP_NUM_TYPES+1; i++) {
        l += 2 + STRLEN(spTyOptP1) + 2 + strlen(dispTypeNames[i].dispName) + STRLEN(spTyOptP3);
    }

    l += 8;

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    sprintf(str, "%s%s%s%s%s%s", custHTMLHdr1, custHTMLHdr2, spTyCustHTML1, custHTMLSHdr, settings.speedoType, spTyCustHTML2);

    for(int i = 0; i < SP_NUM_TYPES+1; i++) {
        sprintf(str + strlen(str), "%s%d'%s>%s%s", spTyOptP1, dispTypeNames[i].dispType, 
                                                   (tt == dispTypeNames[i].dispType) ? custHTMLSel : "", dispTypeNames[i].dispName, 
                                                   spTyOptP3);
    }
    strcat(str, spTyCustHTMLE);

    return str;
}

#ifdef TC_HAVEGPS
static const char *wmBuildUpdateRate(const char *dest, int op)
{
    return wmBuildSelect(dest, op, spdRateCustHTMLSrc, 6, settings.spdUpdRate, true);
}
#endif

#ifdef SERVOSPEEDO
static const char *wmBuildttinp(const char *dest, int op)
{
    return wmBuildRadioButtons(dest, op, ttinCustHTMLSrc, 3, settings.ttinpin);
}

static const char *wmBuildttoutp(const char *dest, int op)
{
    return wmBuildRadioButtons(dest, op, ttoutCustHTMLSrc, 3, settings.ttoutpin);
}
#endif

static const char *wmBuildApChnl(const char *dest, int op)
{
    return wmBuildSelect(dest, op, apChannelCustHTMLSrc, 14, settings.apChnl, false);
}

static const char *wmBuildBestApChnl(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    int32_t mychan = 0;
    int qual = 0;

    if(wm.getBestAPChannel(mychan, qual)) {
        unsigned int l = STRLEN(bestAP) - (5*2) + STRLEN(bannerStart) + 6 + STRLEN(bannerMid) + 4 + STRLEN(badWiFi) + 1 + 8;
        if(op == WM_CP_LEN) {
            wmLenBuf = l;
            return (const char *)&wmLenBuf;
        }
        char *str = (char *)malloc(l);
        sprintf(str, bestAP, bannerStart, qual < 0 ? col_r : (qual > 0 ? col_g : col_gr), bannerMid, mychan, qual < 0 ? badWiFi : "");
        return str;
    }

    return NULL;
}

static const char *buildBanner(const char *msg, const char *col, int op) 
{   // "%s%s%s<i>%s</i></div>"
    unsigned int l = STRLEN(bannerStart) + STRLEN(bannerGen) - (2*4) + STRLEN(bannerMid) + strlen(msg) + 6 + 4;

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);
    sprintf(str, bannerGen, bannerStart, col, bannerMid, msg);        

    return str;
}

static const char *wmBuildNTPLUF(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    int r = ntp_status();
    
    if(!ntpLUF && !r)
        return NULL;

    const char *msg;
    const char *col = col_r;
    
    if(ntpLUF)        msg = msgResolvErr;
    else if(r == 1) { msg = ntpOFF; col = col_gr; }
    else              msg = ntpUNR;
        
    return buildBanner(msg, col, op);
}

static const char *wmBuildHaveSD(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }
    
    if(haveSD)
        return NULL;

    return buildBanner(haveNoSD, col_r, op);
}

#ifdef TC_HAVEMQTT
static const char *wmBuildMQTTprot(const char *dest, int op)
{
    return wmBuildSelect(dest, op, mqttpCustHTMLSrc, 4, settings.mqttVers, false);
}

static const char *wmBuildMQTTstate(const char *dest, int op)
{
    // Check original setting, not "useMQTT" which
    // might be overruled.
    if(!evalBool(settings.useMQTT)) {
        return NULL;
    }
    
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    int s = 0;
    const char *msg = NULL;
    const char *cls = col_r;

    if(!useMQTT) {
        if(mqttReconnFails) {
            msg = msgResolvErr;
        } else {
            msg = mqttMsgDisabled;
            cls = col_gr;
        }
    } else {
        s = mqttClient.state();
        switch(s) {
        case MQTT_CONNECTED:
            msg = mqttMsgConnected;
            cls = col_g;
            break;
        case MQTT_CONNECTING:
            msg = mqttMsgConnecting;
            cls = col_gr;
            break;
        case MQTT_CONNECTION_TIMEOUT:
            msg = mqttMsgTimeout;
            break;
        case MQTT_CONNECTION_LOST:
        case MQTT_CONNECT_FAILED:
            msg = mqttMsgFailed;
            break;
        case MQTT_DISCONNECTED:
            msg = mqttMsgDisconnected;
            break;
        case MQTT_CONNECT_BAD_PROTOCOL:
        case MQTT_CONNECT_BAD_CLIENT_ID:
        case 129:
        case 130:
        case 132:
        case 133:
            msg = mqttMsgBadProtocol;
            break;
        case MQTT_CONNECT_UNAVAILABLE:
        case 136:
        case 137:
            msg = mqttMsgUnavailable;
            break;
        case MQTT_CONNECT_BAD_CREDENTIALS:
        case MQTT_CONNECT_UNAUTHORIZED:
        case 134:
        case 135:
        case 138:
        case 140:
        case 156:
        case 157:
            msg = mqttMsgBadCred;
            break;
        default:
            msg = mqttMsgGenError;
            break;
        }
    }

    // "%s%s%s%s%s (%d)</div>"
    unsigned int l = STRLEN(mqttStatus) - (6*2) + STRLEN(bannerStart) + strlen(cls) + 20 + STRLEN(bannerMid) + strlen(msg) + 6;

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }

    char *str = (char *)malloc(l);

    sprintf(str, mqttStatus, bannerStart, cls, ";margin-bottom:10px", bannerMid, msg, s);

    return str;
}

static const char *wmBuildMQTTTM(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }
    const char HTTP_SECT_HEAD[] = "<div class='ss'>";
    const char HTTP_SECT_FOOT[] = "</div>";
    static const char mqtttm1[] = "<label for='%s'>Topic for %d</label><br><input id='%s' name='%s' maxlength='127' value='%s'%s><br><label for='%s'>Message for %d</label><br><input id='%s' name='%s' maxlength='63' value='%s' class='mb15'%s><br>";
    static const char mqtttmp1[] = " placeholder='Example: home/lights/1/'";
    static const char mqtttmp2[] = " placeholder='Example: ON'";

    unsigned int l = 0;

    l += STRLEN(HTTP_SECT_HEAD) + STRLEN(HTTP_SECT_FOOT) + STRLEN(mqtttmp1) + STRLEN(mqtttmp2);
    for(int i = 0; i < 10; i++) {
        l += STRLEN(mqtttm1) - (2*12);
        l += (STRLEN(mqnmt) * 3) + (STRLEN(mqnmm) * 3);
        l += 2 * 3;   // 600-609 1x topic, 1x msg
        if(settings.mqmt[i]) l += strlen(settings.mqmt[i]);
        if(settings.mqmm[i]) l += strlen(settings.mqmm[i]);
    }

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }

    char *str = (char *)malloc(l + 8);

    strcpy(str, HTTP_SECT_HEAD);
    for(int i = 0; i < 10; i++) {
        mqnmt[2] = mqnmm[2] = i + '0';
        sprintf(str + strlen(str), mqtttm1,
               mqnmt, i + 600, mqnmt, mqnmt, settings.mqmt[i] ? settings.mqmt[i] : "", !i ? mqtttmp1 : "",
               mqnmm, i + 600, mqnmm, mqnmm, settings.mqmm[i] ? settings.mqmm[i] : "", !i ? mqtttmp2 : "");
    }
    strcat(str, HTTP_SECT_FOOT);

    return str;
}
#endif

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

              #ifdef TC_DBG_WIFI
              Serial.printf("handleUploading: Too many files, ignoring %s\n", c.c_str());
              #endif

          } else {

              c.toLowerCase();
    
              #ifdef TC_DBG_WIFI
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
    
              #ifdef TC_DBG_WIFI
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

        #ifdef TC_DBG_WIFI
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
    int buflen  = strlen(wm.getHTTPSTART(titStart)) + // includes </title>
                  STRLEN(myTitle)    +   
                  strlen(wm.getHTTPSCRIPT()) +
                  strlen(wm.getHTTPSTYLE()) +
                  STRLEN(acul_part1) +
                  STRLEN(myHead)     +
                  STRLEN(acul_part3) +
                  STRLEN(myTitle)    +
                  STRLEN(acul_part5) +
                  STRLEN(acul_part8) +
                  1;

    #ifdef TC_DBG_WIFI
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
        strcat(buf, acul_part1);
        strcat(buf, myHead);
        strcat(buf, acul_part3);
        strcat(buf, myTitle);
        strcat(buf, acul_part5);

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

static bool isNumString(char *s)
{
    for( ; *s; ++s) {
        if(*s < '0' || *s > '9') return false;
    }
    return true;
}

static void getServerParam(const char *name, char *destBuf, size_t length, int minval, int maxval, int defaultVal)
{
    int i;
    memset(destBuf, 0, length + 1);
    strncpy(destBuf, wm.server->arg(name).c_str(), length);
    if(*destBuf) {
        if(isNumString(destBuf)) i = atoi(destBuf);
        else *destBuf = 0;
    }
    if(!*destBuf || i < minval || i > maxval) {
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

static void mystrcpyWiFiDelay(char *sv, WiFiManagerParameter *el)
{
    int a = atoi(el->getValue());
    if(a > 0 && a < 10) a = 10;
    else if(a > 99)     a = 99;
    else if(a < 0)      a = 0;
    sprintf(sv, "%d", a);
}


static void evalCB(char *sv, WiFiManagerParameter *el)
{
    *sv++ = (*(el->getValue()) == '0') ? '0' : '1';
    *sv = 0;
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{   
    el->setValue((*sv == '0') ? "0" : "1");
}

int16_t filterOutUTF8(char *src, char *dst, int srcLen = 0, int maxChars = 99999)
{
    int i, j, slen = srcLen ? srcLen : strlen(src);
    unsigned char c, e;

    for(i = 0, j = 0; i < slen && j < maxChars; i++) {
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
    int i, slen = strlen(src);
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
    char *dest = dst;
    len--;  // leave room for 0
    while(*src && len--) {
        if(*src != '\'') *dst++ = *src;
        src++;
    }
    *dst = 0;
    truncateUTF8(dest);
}

static void handleMQTTTopMsg(int idx)
{
    int sz;
    mqnmt[2] = mqnmm[2] = idx + '0';
    
    if(settings.mqmt[idx]) {
        free((void *)settings.mqmt[idx]);
        settings.mqmt[idx] = NULL;
    }
    if(settings.mqmm[idx]) {
        free((void *)settings.mqmm[idx]);
        settings.mqmm[idx] = NULL;
    }

    // We don't allow the single quote (') since it messes
    // up our HTML. Maybe escape it.. in the future.

    String tt = wm.server->arg(mqnmt);
    if((sz = tt.length())) {
        sz++;
        if(sz > 128) sz = 128;
        settings.mqmt[idx] = (char *)malloc(sz);
        memset(settings.mqmt[idx], 0, sz);
        strcpyutf8(settings.mqmt[idx], tt.c_str(), sz);
    }

    String tm = wm.server->arg(mqnmm);
    if((sz = tm.length())) {
        sz++;
        if(sz > 64) sz = 64;
        settings.mqmm[idx] = (char *)malloc(sz);
        memset(settings.mqmm[idx], 0, sz);
        strcpyutf8(settings.mqmm[idx], tm.c_str(), sz);
    }
}

static void mqttLooper()
{
    ntp_loop();
    audio_loop();
    #if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
    // We are running in sync with loops => speedoUpdate_loop(false)
    speedoUpdate_loop(false);   // does not call any other loops
    #endif
}

static void receiveMQTTMsg(int idx, uint32_t dmask, char *tempBuf)
{
    if(!mqttMsg[idx]) return;

    int j = filterOutUTF8(tempBuf, mqttMsg[idx]);
    
    mqttIdx[idx] = 0;
    mqttMaxIdx[idx] = (j > DISP_LEN) ? j : -1;
    mqttDisp |= dmask;
    mqttOldDisp &= ~dmask;
    mqttST[idx] = !!(haveMQTTaudio & (1 << idx));
}

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    int i = 0, j, ml = (length <= 255) ? length : 255;
    char tempBuf[256];
    static const char *cmdList[] = {
      "\x0a" "TIMETRAVEL",       // 0
      "\x06" "RETURN",           // 1
      "\xc8" "ALARM_ON",         // 2 also when CSF_OFF or AL
      "\xc9" "ALARM_OFF",        // 3 also when CSF_OFF or AL
      "\x0c" "NIGHTMODE_ON",     // 4
      "\x0d" "NIGHTMODE_OFF",    // 5
      "\x0d" "MP_SHUFFLE_ON",    // 6
      "\x0e" "MP_SHUFFLE_OFF",   // 7
      "\x07" "MP_PLAY",          // 8
      "\x07" "MP_STOP",          // 9
      "\x07" "MP_NEXT",          // 10
      "\x07" "MP_PREV",          // 11
      "\x08" "BEEP_OFF",         // 12
      "\x07" "BEEP_ON",          // 13
      "\x07" "BEEP_30",          // 14
      "\x07" "BEEP_60",          // 15
      "\x08" "PLAYKEY_",         // 16                                PLAYKEY_1..PLAYKEY_9
      "\x07" "STOPKEY",          // 17
      "\x07" "INJECT_",          // 18
      "\x8e" "PLAY_DOOR_OPEN",   // 19 also when CSF_OFF       | PLAY_DOOR_OPEN, PLAY_DOOR_OPEN_L, PLAY_DOOR_OPEN_R
      "\x8f" "PLAY_DOOR_CLOSE",  // 20 also when CSF_OFF       | PLAY_DOOR_CLOSE, PLAY_DOOR_CLOSE_L, PLAY_DOOR_CLOSE_R
      "\xc8" "POWER_ON",         // 21 also when CSF_OFF or AL
      "\xc9" "POWER_OFF",        // 22 also when CSF_OFF or AL
      "\xd0" "POWER_CONTROL_ON", // 23 also when CSF_OFF or AL
      "\xd1" "POWER_CONTROL_OFF",// 24 also when CSF_OFF or AL
      "\xca" "ALARM_STOP",       // 25 also when CSF_OFF or AL
      "\xcc" "ALARM_SNOOZE",     // 26 also when CSF_OFF or AL
      NULL
    };

    if(!length) return;

    if(!strcmp(topic, "bttf/tcd/cmd")) {

        // Not taking commands under these circumstances:
        if(csf & (CSF_MA|CSF_ST|CSF_P0|CSF_P1|CSF_RE))
            return;

        if(ml < 4) return;

        int tempBufLen;
        int k = 0;

        for(tempBufLen = 0; tempBufLen < ml; tempBufLen++) {
            char a = *payload++;
            if(a >= 'a' && a <= 'z') a &= ~0x20;
            tempBuf[tempBufLen] = a;
        }
        tempBuf[tempBufLen] = 0;

        while(cmdList[i]) {
            const char *t = cmdList[i];
            j = k = *t++;
            j &= 0x3f;
            #ifdef ESP32
            if((ml >= j) && (*(uint32_t *)tempBuf == *(uint32_t *)t)) {
                if(!strncmp((const char *)tempBuf+4, t+4, j-4)) {
                    break;
                }
            }
            #else
            if((ml >= j) && !strncmp((const char *)tempBuf, t, j)) {
                break;
            }
            #endif
            i++;
        }

        if(!cmdList[i]) return;

        if((csf & CSF_OFF) && (!(k & 0x80)))
            return;

        if((csf & (CSF_AL|CSF_AE)) && (!(k & 0x40)))
            return;

        switch(i) {
        case 0:
            eef |= (EEF_EttPressed|EEF_EttImmediate);
            break;
        case 1:
            eef |= EEF_EttHeld;
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
            mp_makeShuffle((i == 6));
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
        case 16:
            if(tempBufLen > j && tempBuf[j] >= '1' && tempBuf[j] <= '9') {
                play_key((int)(tempBuf[j] - '0'), 0xffff);
            }
            break;
        case 17:
            stop_key();
            break;
        case 18:
            if(tempBufLen > j) {
                injectInput(&tempBuf[j]);
            }
            break;
        case 19:
        case 20:
            // We don't differ between door 1 and door 2 here;
            // allow panning through door 1.
            doorSnd = (i == 20) ? -1 : 1;
            doorFlags = PA_DOOR;
            if(tempBufLen > j+1 && tempBuf[j] == '_') {
                if(tempBuf[j+1] == 'L')      doorFlags |= PA_DOORL;
                else if(tempBuf[j+1] == 'R') doorFlags |= PA_DOORR;
            }
            doorSndNow = millis();
            doorSndDelay = 0;
            break;
        case 21:
            mqttFakePowerOn();
            break;
        case 22:
            mqttFakePowerOff();
            break;
        case 23:
        case 24:
            mqttFakePowerControl(i == 23);
            break;
        case 25:
            if((!(csf & CSF_AE)) && snoozeRunning()) {
                cancelSnooze();
            }
            // Fall through
        case 26:
            if((!(csf & CSF_AE)) && (csf & CSF_AL)) {
                stopAlarm(true);
                if(i == 25) {
                    cancelSnooze();
                } else {
                    startSnooze();
                }
            }
            break;
        }
            
    } else {

        memcpy(tempBuf, (const char *)payload, ml);
        tempBuf[ml] = 0;

        if(*settings.mqttTopic && (!strcmp(topic, settings.mqttTopic))) {

            receiveMQTTMsg(0, MQ_DISP_D, tempBuf);

        } else if(*settings.mqttTopicP && (!strcmp(topic, settings.mqttTopicP))) {

            receiveMQTTMsg(1, MQ_DISP_P, tempBuf);

        } else if(*settings.mqttTopicL && (!strcmp(topic, settings.mqttTopicL))) {

            receiveMQTTMsg(2, MQ_DISP_L, tempBuf);

        }
    }
}

#ifdef TC_DBG_MQTT
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
                mqttPingNow = millisNonZero();
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
            if(!(mqttReconnectNow = millis() - (mqttReconnectInt - 5000)))
                mqttReconnectNow--;
        } else if(millis() - mqttPingNow > 5000) {
            mqttClient.cancelPing();
            mqttPingNow = millisNonZero();
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

                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Attempting to (re)connect");
                #endif
    
                if(strlen(mqttUser)) {
                    success = mqttClient.connect(mqttUser, strlen(mqttPass) ? mqttPass : NULL);
                } else {
                    success = mqttClient.connect();
                }
    
                mqttReconnectNow = millisNonZero();
                
                if(!success) {
                    mqttRestartPing = true;  // Force PING check before reconnection attempt
                    mqttReconnFails++;
                    if(mqttDoPing) {
                        mqttPingInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    } else {
                        mqttReconnectInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    }
                    #ifdef TC_DBG_MQTT
                    Serial.printf("MQTT: Failed to reconnect (%d)\n", mqttReconnFails);
                    #endif
                } else {
                    mqttReconnFails = 0;
                    mqttReconnectInt = MQTT_SHORT_INT;
                    #ifdef TC_DBG_MQTT
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
        #ifdef TC_DBG_MQTT
        Serial.println("MQTT: Subscribing...");
        #endif
        if(!mqttClient.subscribe("bttf/tcd/cmd", settings.mqttTopic)) {
            if(!mqttClient.subscribe("bttf/tcd/cmd")) {
                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Failed to subscribe to command topic");
                #endif
            } else {
                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Failed to subscribe to user topic");
                #endif
            }
        }
        if(*settings.mqttTopicP || *settings.mqttTopicL) {
            if(!mqttClient.subscribe(settings.mqttTopicP, settings.mqttTopicL)) {
                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Failed to subscribe to 2nd/3rd user topic");
                #endif
            }
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
