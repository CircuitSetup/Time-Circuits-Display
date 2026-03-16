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
unsigned long mqttInitialConnectNow = 0;
#endif

static const char R_updateacdone[] = "/uac";

static const char acul_part3[]  = "</head><body><div class='wrap'><h1>";
static const char acul_part5[]  = "</h1><h3>";
static const char acul_part6[]  = "</h3><div class='msg";
static const char acul_part7[]  = " S' id='lc'><strong>Upload successful.</strong><br>Device rebooting.";
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

static const char anmCustHTML1[] = "%s%sanmtim'>Schedule%s%s' name='anmtim' id='anmtim'><option value='10'%s>&#10060; Off%s0'";
static const char *anmCustHTMLSrc[5] = {
    "%s>&#128337; Daily, set hours below%s1'",
    "%s>&#127968; M-T:17-23/F:13-1/S:9-1/Su:9-23%s2'",
    "%s>&#127970; M-F:9-17%s3'",
    "%s>&#127970; M-T:7-17/F:7-14%s4'",
    "%s>&#128722; M-W:8-20/T-F:8-21/S:8-17%s"
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

static const char *wmBuildApChnl(const char *dest, int op);
static const char *wmBuildBestApChnl(const char *dest, int op);

static const char *wmBuildbeepaint(const char *dest, int op);
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
#endif

static const char custHTMLHdr1[] = "<div class='cmp0'";
static const char custHTMLHdr2[] = "><label class='mp0' for='";
static const char custHTMLHdrI[] = " style='margin-left:20px'";
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

static const char bannerGen[] = "%s%s%s%s</div>";
static const char ntpLUFS[] = "<i>Failed to resolve address</i>";
static const char ntpOFF[] = "<i>NTP is inactive</i>";
static const char ntpUNR[] = "<i>NTP server is unresponsive</i>";
static const char haveNoSD[] = "<i>No SD card present</i>";

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
WiFiManagerParameter custom_wifiConTimeout("wificon", "Connection timeout (7-25[seconds])", settings.wifiConTimeout, 2, "type='number' min='7' max='25'");
WiFiManagerParameter custom_wifiPRe("wifiPRet", "Periodic reconnection attempts ", settings.wifiPRetry, "class='mt5 mb10'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_wifiOffDelay("wifioff", "Power save timer<br><span>(10-99[minutes]; 0=off)</span>", settings.wifiOffDelay, 2, "type='number' min='0' max='99' title='WiFi will be shut down after chosen period'");
// custom_wifihint follows (and is foot; next sect must therefore be SECTS_HEAD)

WiFiManagerParameter custom_sectstart_ap("Access point (AP) mode settings", WFM_SECTS_HEAD|WFM_HL);
WiFiManagerParameter custom_sysID("sysID", "Network name (SSID) appendix<br><span>Will be appended to \"TCD-AP\" [a-z/0-9/-]</span>", settings.systemID, 7, "pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_appw("appw", "Password<br><span>Password to protect TCD-AP. Empty or 8 characters [a-z/0-9/-]</span>", settings.appw, 8, "minlength='8' pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_apch(wmBuildApChnl);
WiFiManagerParameter custom_bapch(wmBuildBestApChnl);
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "Power save timer<br><span>(10-99[minutes]; 0=off)</span>", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='WiFi-AP will be shut down after chosen period'");

WiFiManagerParameter custom_wifihint("<div style='margin:0;padding:0;font-size:80%'>Hold '7' to re-enable Wifi when in power save mode.</div>", WFM_FOOT);

// Settings

WiFiManagerParameter custom_tzsel("<datalist id='tzlist'><option value='PST8PDT,M3.2.0,M11.1.0'>Pacific</option><option value='MST7MDT,M3.2.0,M11.1.0'>Mountain</option><option value='CST6CDT,M3.2.0,M11.1.0'>Central</option><option value='EST5EDT,M3.2.0,M11.1.0'>Eastern</option><option value='GMT0BST,M3.5.0/1,M10.5.0'>Western European</option><option value='CET-1CEST,M3.5.0,M10.5.0/3'>Central European</option><option value='EET-2EEST,M3.5.0/3,M10.5.0/4'>Eastern European</option><option value='MSK-3'>Moscow</option><option value='AWST-8'>Australia Western</option><option value='ACST-9:30'>Australia Central/NT</option><option value='ACST-9:30ACDT,M10.1.0,M4.1.0/3'>Australia Central/SA</option><option value='AEST-10AEDT,M10.1.0,M4.1.0/3'>Australia Eastern VIC/NSW</option><option value='AEST-10'>Australia Eastern QL</option></datalist>");

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
WiFiManagerParameter custom_timeZone2("tz2", "Time zone for Last Time Dep. display", settings.timeZoneDep, 63, "placeholder='Example: CST6CDT,M3.2.0,M11.1.0' list='tzlist'");
WiFiManagerParameter custom_timeZoneN1("tzn1", tznp1, settings.timeZoneNDest, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO' style='margin-bottom:15px'");
WiFiManagerParameter custom_timeZoneN2("tzn2", tznp1, settings.timeZoneNDep, DISP_LEN, "pattern='[a-zA-Z0-9 \\-]+' placeholder='Optional. Example: CHICAGO'");

WiFiManagerParameter custom_sectstart_nm("Night mode", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_dtNmOff("dTnMOff", "Destination time off", settings.dtNmOff, "title='Dimmed if unchecked' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ptNmOff("pTnMOff", "Present time off", settings.ptNmOff, "title='Dimmed if unchecked' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ltNmOff("lTnMOff", "Last time dep. off", settings.ltNmOff, "title='Dimmed if unchecked' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_autoNMTimes(wmBuildAnmPreset);
WiFiManagerParameter custom_autoNMOn("anmon", "Daily night-mode start hour (0-23)", settings.autoNMOn, 2, "type='number' min='0' max='23' title='Hour to switch on night-mode'");
WiFiManagerParameter custom_autoNMOff("anmoff", "Daily night-mode end hour (0-23)", settings.autoNMOff, 2, "type='number' min='0' max='23' title='Hour to switch off night-mode'");
#ifdef TC_HAVELIGHT
WiFiManagerParameter custom_uLS("uLS", "Use light sensor", settings.useLight, "title='If checked, TCD will be put in night-mode if lux level is below or equal threshold.' style='margin-top:14px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_lxLim("lxLim", "Lux threshold (0-50000)", settings.luxLimit, 6, "type='number' min='0' max='50000'");
#endif

#ifdef TC_HAVETEMP
WiFiManagerParameter custom_sectstart_te("Temperature/humidity sensor", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_tempUnit("temUnt", "Show temperature in Celsius", settings.tempUnit, "title='Fahrenheit if unchecked' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_tempOffs("tOffs", "Temperature offset (-3.0-3.0)", settings.tempOffs, 4, "type='number' min='-3.0' max='3.0' step='0.1'");
#endif // TC_HAVETEMP

WiFiManagerParameter custom_speedoType(wmBuildSpeedoType, WFM_SECTS);
WiFiManagerParameter custom_speedoBright("speBri", "Speed brightness (0-15)", settings.speedoBright, 2, "type='number' min='0' max='15'");
WiFiManagerParameter custom_spAO("spAO", "Switch speedo off when idle", settings.speedoAO, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_sAF("sAF", "Real-life acceleration figures", settings.speedoAF, "title='If unchecked, movie-like times are used'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_speedoFact("speFac", "Factor for real-life figures (0.5-5.0)", settings.speedoFact, 3, "type='number' min='0.5' max='5.0' step='0.5' title='1.0 means real-world DMC-12 acceleration time.'");
WiFiManagerParameter custom_sL0("sL0", "Speedo display like in part 3", settings.speedoP3, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_s3rd("s3rd", "Display '0' after dot like A-car", settings.speedo3rdD, "title='CircuitSetup speedo only.'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifdef TC_HAVEGPS
WiFiManagerParameter custom_useGPSS("uGPSS", "Display GPS speed", settings.dispGPSSpeed, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_updrt(wmBuildUpdateRate);  // speed update rate
#endif // TC_HAVEGPS
#ifdef TC_HAVETEMP
WiFiManagerParameter custom_useDpTemp("dpTemp", "Display temperature", settings.dispTemp, "title='Check to display temperature on speedo when idle'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_tempBright("temBri", "Temperature brightness (0-15)", settings.tempBright, 2, "type='number' min='0' max='15'");
WiFiManagerParameter custom_tempOffNM("toffNM", "Temperature off in night mode", settings.tempOffNM, "title='Dimmed if unchecked' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif // TC_HAVETEMP

WiFiManagerParameter custom_fakePwrOn("fpo", "Use fake power switch", settings.fakePwrOn, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS);

#ifdef SERVOSPEEDO
WiFiManagerParameter custom_ttin(wmBuildttinp, WFM_SECTS);
WiFiManagerParameter custom_ttinhl("<div style='margin:10px 0 5px 0;padding:0;'>External time travel trigger</div>");
#else
WiFiManagerParameter custom_sectstart_et("External time travel trigger (TT-IN)", WFM_SECTS|WFM_HL);
#endif
WiFiManagerParameter custom_ettDelay("ettDe", "Delay (ms)", settings.ettDelay, 5, "type='number' min='0' max='60000'");
//WiFiManagerParameter custom_ettLong("ettLg", "Play complete time travel sequence", settings.ettLong, "title='If unchecked, the short \"re-entry\" sequence is played' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

#ifdef SERVOSPEEDO
WiFiManagerParameter custom_ttout(wmBuildttoutp, WFM_SECTS);
WiFiManagerParameter custom_ttouthl("<div style='margin:10px 0 5px 0;padding:0;'>'TT-OUT' output</div>");
#else
WiFiManagerParameter custom_sectstart_etto("TT-OUT (IO14)", WFM_SECTS|WFM_HL);
#endif
WiFiManagerParameter custom_ETTOcmd("EtCd", "is controlled by commands 990/991", settings.ETTOcmd, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ETTOPUS("EtPU", "Power-up state HIGH", settings.ETTOpus, "class='mt5 mb10' style='margin-left:20px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_useETTO("uEtto", "signals time travel", settings.useETTO, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_noETTOL("uEtNL", "Signal without 5 second lead", settings.noETTOLead, "class='mt5 mb10' style='margin-left:20px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ETTOalm("EtAl", "signals alarm", settings.ETTOalm, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_TTOAlmDur("taldu", "seconds signal duration (3-99)", settings.ETTOAD, 3, "style='margin-left:20px;margin-right:5px;width:5em' type='number' min='3' max='99'", WFM_LABEL_AFTER);

#ifdef TC_HAVEGPS
WiFiManagerParameter custom_sectstart_bt("Wireless communication (BTTF-Network)", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_qGPS("qGPS", "Provide GPS speed to BTTFN clients", settings.provGPS2BTTFN, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif // TC_HAVEGPS

WiFiManagerParameter custom_haveSD(wmBuildHaveSD, WFM_SECTS);
WiFiManagerParameter custom_CfgOnSD("CfgSD", "Save secondary settings on SD<br><span>Check this to avoid flash wear</span>", settings.CfgOnSD, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#ifdef PERSISTENT_SD_ONLY
WiFiManagerParameter custom_ttrp("ttrp", "Make time travel persistent<br><span>Requires SD card</span>", settings.timesPers, "class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
#endif
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span>Checking this might help in case of SD card problems</span>", settings.sdFreq, WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_rvapm("rvapm", "Reverse AM/PM like in parts 2/3", settings.revAmPm, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS|WFM_FOOT);

#ifdef TC_HAVEMQTT
WiFiManagerParameter custom_useMQTT("uMQTT", "Home Assistant support (MQTT)", settings.useMQTT, "class='mt5' style='margin-bottom:10px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);
WiFiManagerParameter custom_state(wmBuildMQTTstate);
WiFiManagerParameter custom_mqttServer("MQs", "Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9\\.:\\-]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttVers(wmBuildMQTTprot);
WiFiManagerParameter custom_mqttUser("MQu", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
WiFiManagerParameter custom_mqttTopic("MQt", "Topic to display", settings.mqttTopic, 127, "placeholder='Optional. Example: home/alarm/status'");
WiFiManagerParameter custom_pubMQTT("MQpu", "Publish time travel and alarm events", settings.pubMQTT, "class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_vMQTT("MQpv", "Enhanced Time Travel notification", settings.MQTTvarLead, "class='mt5' style='margin-left:20px' title='Check to send time travel with variable lead.'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_mqttPwr("MQp", "HA controls Fake-Power at startup", settings.mqttPwr, "class='mt5' title='Check to have HA control Fake-Power and take precendence over Fake-Power switch at power-up'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_mqttPwrOn("MQpo", "Wait for POWER_ON at startup", settings.mqttPwrOn, "class='mt5' style='margin-left:20px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_FOOT);
#endif // HAVEMQTT

#ifdef TC_HAVEMQTT
#define TC_MENUSIZE 8
#else
#define TC_MENUSIZE 7
#endif
static const int8_t wifiMenu[TC_MENUSIZE] = { 
    WM_MENU_WIFI,
    WM_MENU_PARAM,
    #ifdef TC_HAVEMQTT
    WM_MENU_PARAM2,
    #endif
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
#define PARM2TITLE WM_PARAM2_TITLE
#define CURRVERSION TC_VERSION_REV

static const char myTitle[] = AA_TITLE;
static const char apName[]  = "TCD-AP";
static const char myHead[]  = "<link rel='shortcut icon' type='image/png' href='data:image/png;base64," AA_ICON "'><script>window.onload=function(){xx=false;document.title=xxx='" AA_TITLE "';id=-1;ar=['/u','/uac','/wifisave','/paramsave','/param2save'];ti=['Firmware upload','','WiFi Configuration','Settings','" PARM2TITLE "'];if(ge('s')&&ge('dns')){xx=true;yyy=ti[2]}if(ge('uploadbin')||(id=ar.indexOf(wlp()))>=0){xx=true;if(id>=2){yyy=ti[id]}else{yyy=ti[0]};aa=gecl('wrap');if(aa.length>0){if(ge('uploadbin')){aa[0].style.textAlign='center';}aa=getn('H3');if(aa.length>0){aa[0].remove()}aa=getn('H1');if(aa.length>0){aa[0].remove()}}}if(ge('ttrp')||wlp()=='/param'||wlp()=='/param2'){xx=true;yyy=ti[3];}if(ge('ebnew')){xx=true;bb=getn('H3');aa=getn('H1');yyy=bb[0].innerHTML;ff=aa[0].parentNode;ff.style.position='relative';}if(xx){zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\" onClick=\"window.location=\\'/\\'\"><div class=\"tpm2\"><img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQMAAACQp+OdAAAABlBMVEUAAABKnW0vhlhrAAAAAXRSTlMAQObYZgAAA'+(zz?'GBJREFUKM990aEVgCAABmF9BiIjsIIbsJYNRmMURiASePwSDPD0vPT12347GRejIfaOOIQwigSrRHDKBK9CCKoEqQF2qQMOSQAzEL9hB9ICNyMv8DPKgjCjLtAD+AV4dQM7O4VX9m1RYAAAAABJRU5ErkJggg==':'HtJREFUKM990bENwyAUBuFnuXDpNh0rZIBIrJUqMBqjMAIlBeIihQIF/fZVX39229PscYG32esCzeyjsXUzNHZsI0ocxJ0kcZIOsoQjnxQJT3FUiUD1NAloga6wQQd+4B/7QBQ4BpLAOZAn3IIy4RfUibCgTTDq+peG6AvsL/jPTu1L9wAAAABJRU5ErkJggg==')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.4em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:5em\"':'')+'>'+yyy+'</div></div>';}if(ge('ebnew')){bb[0].remove();aa[0].replaceWith(dd);}else if(xx){aa=gecl('wrap');if(aa.length>0){aa[0].insertBefore(dd,aa[0].firstChild);aa[0].style.position='relative';}}var lc=ge('lc');if(lc){lc.style.transform='rotate('+(358+[0,1,3,4,5][Math.floor(Math.random()*4)])+'deg)'}}</script><style type='text/css'>H1,H2{margin-top:0px;margin-bottom:0px;text-align:center;}H3{margin-top:0px;margin-bottom:5px;text-align:center;}button{transition-delay:250ms;margin-top:10px;margin-bottom:10px;font-variant-caps:all-small-caps;border-bottom:0.2em solid #225a98}input{border:thin inset}em > small{display:inline}form{margin-block-end:0;}.tpm{cursor:pointer;border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;font-weight:normal;margin-left:2.2em;overflow:clip;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI Semibold',Roboto,'Helvetica Neue',Verdana,Helvetica}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7.2em;margin-left:0.5em;margin-right:0.5em;border-radius:5px}.tpm0{position:relative;width:20em;padding:5px 0px 5px 0px;margin:0 auto 0 auto;}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}.mt5{margin-top:5px!important}.mb10{margin-bottom:10px!important}.mb0{margin-bottom:0px!important}.mb15{margin-bottom:15px!important}.ss>label span{font-size:80%}</style>";
static const char* myCustMenu = "<img id='ebnew' style='display:block;margin:10px auto 5px auto;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAR8AAAAyCAMAAABSzC8jAAAAUVBMVEUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABcqRVCAAAAGnRSTlMAv4BAd0QgzxCuMPBgoJBmUODdcBHumYgzVQmpHc8AAAf3SURBVGje7Jjhzp0gDIYFE0BQA/Ef93+hg7b4wvQ7R5Nl2Y812fzgrW15APE4eUW2rxOZNJfDcRu2q2Zjv9ygfe+1xSY7bXNWHH3lm13NJ01P/5PcrqyIeepfcLeCraOfpN7nPoSuLWjxHCSVa7aQs909Zxcf8mDBTNOcxWwlgmbw02gqNxv7z+5t8FIM2IdO1OUPzzmUNPl/K4F0vbIiNnMCf7pnmO79kBq57sviAiq3GKT3QFyqbG2NFUC4SDSDeckn68FLkWpPEXVFCbKUJDIQ84XP/pgPvO/LWlCHC60zjnzMKczkC4p9c3vLJ8GLYmMiBIGnGeHS2VdJ6/jCJ73ik10fIrhB8yefA/4jn/1syGLXWlER3DzmuNS4Vz4z2YWPnWfNqcVrTTKLtkaP0Q4IdhlQcdpkIPbCR3K1yn3jUzvr5JWLoa6j+SkuJNAkiESp1qYdiXPMALrUOyT7RpG8CL4Iin01jQRopWkufNCCyVbakbO0jCxUGjqugYgoLAzdJtpc+HQJ4Hj2aHBEgVRIFG/s5f3UPJUFPjxGE8+YyOiqMIPPWnmDDzI/5BORE70clHFjR1kaMEGLjc/xhY99yofCbpC4ENGmkQ/2yIWP5b/Ax1PYP8tHomB1bZSYFwSnIp9E3R/5ZPOIj6jLUz7Z3/EJlG/kM9467W/311aubuTDnQYD4SG6nEv/QkRFssXtE58l5+PN+tGP+Cw1sx/4YKjKf+STbp/PutqVT9I60e3sJVF30CIWK19c0XR11uCzF3XkI7kqXNbtT3w28gOflVMJHwc+eDYN55d25zTXSCuFJWHkk5gPZdzTh/P9ygcvmEJx645cyYLCYqk/Ffoab4k+5+X2fJ+FRl1g93zgp2iiqEwjfJiWbtqWr4dQESKGwSW5xIJH5XwEju+H7/gEP11exEY+7Dzr8q8IVVxkjHVy3Cc+R87HAz5iWqSDT/vYa9sEPiagcvAp5kUwHR97rh/Ae7V+wtp7be6OTyiXvbAo/7zCQKa6wT7xMTnbx3w0pMtr6z6BTwG08Mof+JCgWLh7/oDz/fvh3fPZrYmXteorHvkc3FF3QK2+dq2NT91g6ub90DUatlR0z+cQP6Q2I5/YazP4cGGJXPB+KMtCfpv5Cx/KqPgwen5+CWehGBtfiYPTZCnONtsplizdmwQ9/ez1/AKNg/Rv55edD54I8Alr07gs8GFzlqNh9fbCcfJx5brIrXwGvOAj16V5WeaC+jVg0FEyF+fOh98nPvHxpD8430Mh0R1t0UGrZQXwEYv3fOTRLnzGo49hveejmtdBfHGdGoy1LRPilMHCf+EzpYd8NtoVkKBxX/ydj/+Jzzzw2fgeuVU2hqNfgVc+hrb8wMf0fIzw9XJ1IefEOQVDyOQPFukLn/0ZH/nBdc/Hj+eXoyHsFz4ibB0fV8MF3MrbmMULHyQHn7iQK3thg4Xa68zSdr7rPkaMfPYvfPwjPpwyQRq1NA4yrG6ig2Ud+ehUOtYwfP8Z0RocbuDTbB75wFbhg421Q/TsLXw2xgEWceTTDDOb7vnATxgsnOvKR8qJ+H1x+/0nd0MN7IvvSOP3jVd88CFq3FhiSxeljezo10r4wmd/yGflDXblg7JkkAEvRSMfRB0/OIMPb7CXfGK3C5NssIgfH2Ttw9tKgXo+2xc+/gkf2cLpjg/K4kH6jNoGPnM/p9Kwm5nARx63b/ioGgB89nZyeSKyuW7kqqU1PZ/4hc+UnvGRDXblg7JkkPMWam3ajdPchKSnv2PeTP+qmdn8JPy7Rf+3X+zBgQAAAAAAkP9rI6iqqirNme2qpDAMhhtIWvxVKP2w7/1f6DapVmdnzsDCCucFx7QmaXx0ouB/kOfGfprM52Rkf4xZtb9E5BERsxnM0TlhGZvK/PXImI5sEj9sf9kzu3q9ltBt2hKK7bKmP2rRFZxlkcttWI3Zu2floeqGBzhnCVqQjmGq94hyfK3dzUiOwWNTmT9rJDmCiWXYcrNdDmqXi3mHqh0RZLnMIUHPPiGzJo2zkuXmghnZPavQZAMNI5fykQ9zA/wV0LBJr00LD8yhHnyIh4ynNz6RGYlZjI9ah+0qCvOWbhWAJVJ3hMrMceYKqK4plh1kK3hgYy5xuXWELo3cw1L+KONnC/yRzxpexyxsR9LYXau3zYSCzfi449f4zPHcF+wWtgRYHWsVBk/Xjs1Gx7apl7+7Wdjz8lq2YL/zYRH5zKeh8L7qOwxGFRG7cyrknU8QkX2xelVAiH4tmi8+dt022BVYNSy3DjSdel4bosupuTufWz/hiuAu5QSA8t98VKyn5Et456OiH/hIAdDORWX+vxL6ZFOSu/NZbnoUSLt7XKztt6X8wqcy8+rPW34JiLVgu/hc/UfUf9jxjU8honbxeVXmDeBjUT9Zlz4zC+obH3PT1C2huKcV7fSRiBLoQ/8RBn146o24eufDq5nklL70H4/0sQi6NZYqyWwPYvS5QkVctV1kgw6e1HmamPrYn4OWtl41umjhZWw6LfGNj4v41p+TLujZLbG3i/TSePukmEDIcybaKwHvy82zOezuWd24/PT8EiQ15GyniQqaNmqUst5/Eg3tRz//xqcDSLc3hgwEArqjsR+arMlul2ak50ywsLrcGgolBPddz/OxIV98YgDQsvoXIJ33j0mmv3zj43oCCuer+9h4PRTO51fJxpJPPrkCIFlusun4V375878k4T+G/QFTIGsvrRmuEwAAAABJRU5ErkJggg=='><div style='font-size:0.9em;line-height:0.9em;font-weight:bold;margin:5px auto 0px auto;text-align:center;font-variant:all-small-caps'>" UNI_VERSION " (" UNI_VERSION_EXTRA ")<br>Powered by A10001986 <a href='https://" WEBHOME ".out-a-ti.me' style='text-decoration:underline' target=_blank>[Home/Updates]</a></div>";

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
const char menu_isp[]   = "Please <a href='/update'>install</a> sound pack (";
const char menu_uai[]   = "Update available (";
const char menu_end[]   = ")</div><hr>";
const char menu_item[]  = "<a href='http://%s' target=_blank title='%s'><img style='zoom:2;padding:0 5px 0 5px;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQ%s' alt='%s'></a>";

static char newversion[8];

static int  shouldSaveConfig = 0;
static bool shouldSaveIPConfig = false;
static bool shouldDeleteIPConfig = false;

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
bool                 useMQTT = false;
bool                 MQTTvarLead = false;
static char          *mqttUser = (char *)emptyStr;
static char          *mqttPass = (char *)emptyStr;
static char          *mqttServer = (char *)emptyStr;
bool                 haveMQTTaudio = false;
const char           *mqttAudioFile = "/ha-alert.mp3";
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
static void saveWiFiCallback(const char *ssid, const char *pass);
static void preUpdateCallback();
static void postUpdateCallback(bool);
static void preSaveWiFiCallback();
static int  menuOutLenCallback();
static void menuOutCallback(String& page, unsigned int ssize);
static void wifiDelayReplacement(unsigned int mydel);
static void gpCallback(int);
static bool preWiFiScanCallback();

static void updateConfigPortalValues();

static void setupStaticIP();
static bool isIp(char *str);
static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length, int defVal);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static char* strcpyfilter(char* destination, const char* source);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
static void evalCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);

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

      &custom_fakePwrOn,
    
      #ifdef SERVOSPEEDO
      &custom_ttin,
      &custom_ttinhl,
      #else
      &custom_sectstart_et,
      #endif
      &custom_ettDelay,  
      //&custom_ettLong,

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

      &custom_haveSD,
      &custom_CfgOnSD,
      #ifdef PERSISTENT_SD_ONLY
      &custom_ttrp,
      #endif
      //&custom_sdFrq,

      &custom_rvapm,
      
      NULL
    };

    #ifdef TC_HAVEMQTT
    WiFiManagerParameter *parm2Array[] = {

      &custom_useMQTT,
      &custom_state,
      &custom_mqttServer, 
      &custom_mqttVers,
      &custom_mqttUser, 
      &custom_mqttTopic,
      &custom_pubMQTT,
      &custom_vMQTT,
      &custom_mqttPwr,
      &custom_mqttPwrOn,

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

    temp = 0;
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

    #ifdef TC_HAVEMQTT
    wm.allocParms2((sizeof(parm2Array) / sizeof(WiFiManagerParameter *)) - 1);

    temp = 0;
    while(parm2Array[temp]) {
        wm.addParameter2(parm2Array[temp]);
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
        setupStaticIP();
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

        pubMQTT = evalBool(settings.pubMQTT);
        MQTTvarLead = pubMQTT ? evalBool(settings.MQTTvarLead) : false;
        MQTTPwrMaster = evalBool(settings.mqttPwr);
        MQTTWaitForOn = MQTTPwrMaster ? evalBool(settings.mqttPwrOn) : false;

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
        mqttClient.setVersion(atoi(settings.mqttVers) > 0 ? 5 : 3);
        mqttClient.setClientID(settings.hostName);

        mqttMsg = (char *)malloc(256);
        memset(mqttMsg, 0, 256);

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

        #ifdef TC_DBG_MQTT
        Serial.printf("MQTT: server '%s' port %d user '%s' pass '%s'\n", mqttServer, mqttPort, mqttUser, mqttPass);
        #endif

        haveMQTTaudio = check_file_SD(mqttAudioFile);
            
        mqttReconnect(true);
        mqttInitialConnectNow = millis();
        
        // Rest done in loop
            
    } else {

        pubMQTT = MQTTvarLead = false;
        MQTTPwrMaster = MQTTWaitForOn = false;

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
    bool didstopstuff = false;

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
                mqttInitialConnectNow = 0;
            }
        }
        mqttClient.loop();

        // Time-out waiting for MQTT connection upon boot in case MQTT 
        // has control over fake-power and we wait for POWER_ON.
        // User can then disable MQTT power control by 996ENTER.
        if(mqttInitialConnectNow && MQTTPwrMaster && MQTTWaitForOn) {
            if(millis() - mqttInitialConnectNow > 40*1000) {
                mqttInitialConnectNow = 0;
                mqttFakePowerOn();
            }
        }
    }
#endif
    
    if(shouldSaveIPConfig) {

        #ifdef TC_DBG_WIFI
        Serial.println("WiFi: Saving IP config");
        #endif

        mp_stop();
        stopAudio();

        ettoPulseEnd();
        send_abort_msg();

        writeIpSettings();

        shouldSaveIPConfig = false;
        didstopstuff = true;

    } else if(shouldDeleteIPConfig) {

        #ifdef TC_DBG_WIFI
        Serial.println("WiFi: Deleting IP config");
        #endif

        mp_stop();
        stopAudio();

        ettoPulseEnd();
        send_abort_msg();

        deleteIpSettings();

        shouldDeleteIPConfig = false;
        didstopstuff = true;

    }

    if(shouldSaveConfig) {

        int temp;

        // Save settings and restart esp32

        if(!didstopstuff) {
            mp_stop();
            stopAudio();
            ettoPulseEnd();
            send_abort_msg();
        }

        #ifdef TC_DBG_WIFI
        Serial.println("Config Portal: Saving config");
        #endif

        // Only read parms if the user actually clicked SAVE on the wifi config or params pages
        if(shouldSaveConfig == 1) {

            // Parameters on WiFi Config page

            // Note: Parameters that need to grabbed from the server directly
            // through getParam() must be handled in preSaveWiFiCallback().

            // ssid, pass copied to settings in saveWiFiCallback()

            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(!*settings.hostName) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            mystrcpy(settings.wifiConTimeout, &custom_wifiConTimeout);
            evalCB(settings.wifiPRetry, &custom_wifiPRe);
            mystrcpy(settings.wifiOffDelay, &custom_wifiOffDelay);
            
            strcpytrim(settings.systemID, custom_sysID.getValue(), true);
            strcpytrim(settings.appw, custom_appw.getValue(), true);
            if((temp = strlen(settings.appw)) > 0) {
                if(temp < 8) {
                    settings.appw[0] = 0;
                }
            }
            mystrcpy(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);
          
        } else if(shouldSaveConfig == 2) {

            // Parameters on Settings page

            evalCB(settings.playIntro, &custom_playIntro);
            getParam("bepm", settings.beep, 1, DEF_BEEP);
            getParam("tcint", settings.autoRotateTimes, 1, DEF_AUTOROTTIMES);
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

            evalCB(settings.dtNmOff, &custom_dtNmOff);
            evalCB(settings.ptNmOff, &custom_ptNmOff);
            evalCB(settings.ltNmOff, &custom_ltNmOff);
            getParam("anmtim", settings.autoNMPreset, 2, DEF_AUTONM_PRESET);
            mystrcpy(settings.autoNMOn, &custom_autoNMOn);
            mystrcpy(settings.autoNMOff, &custom_autoNMOff);
            #ifdef TC_HAVELIGHT
            evalCB(settings.useLight, &custom_uLS);
            mystrcpy(settings.luxLimit, &custom_lxLim);
            #endif

            #ifdef TC_HAVETEMP
            evalCB(settings.tempUnit, &custom_tempUnit);
            mystrcpy(settings.tempOffs, &custom_tempOffs);
            #endif

            getParam("spty", settings.speedoType, 2, DEF_SPEEDO_TYPE);
            mystrcpy(settings.speedoBright, &custom_speedoBright);
            evalCB(settings.speedoAO, &custom_spAO);
            evalCB(settings.speedoAF, &custom_sAF);
            mystrcpy(settings.speedoFact, &custom_speedoFact);
            evalCB(settings.speedoP3, &custom_sL0);
            evalCB(settings.speedo3rdD, &custom_s3rd);
            #ifdef TC_HAVEGPS
            evalCB(settings.dispGPSSpeed, &custom_useGPSS);
            getParam("spdrt", settings.spdUpdRate, 1, DEF_SPD_UPD_RATE);
            #endif
            #ifdef TC_HAVETEMP
            evalCB(settings.dispTemp, &custom_useDpTemp);
            mystrcpy(settings.tempBright, &custom_tempBright);
            evalCB(settings.tempOffNM, &custom_tempOffNM);
            #endif

            evalCB(settings.fakePwrOn, &custom_fakePwrOn);

            mystrcpy(settings.ettDelay, &custom_ettDelay);
            //evalCB(settings.ettLong, &custom_ettLong);
            
            #ifdef TC_HAVEGPS
            evalCB(settings.provGPS2BTTFN, &custom_qGPS);
            #endif

            evalCB(settings.ETTOcmd, &custom_ETTOcmd);
            evalCB(settings.useETTO, &custom_useETTO);
            evalCB(settings.ETTOalm, &custom_ETTOalm);
            evalCB(settings.ETTOpus, &custom_ETTOPUS);
            evalCB(settings.noETTOLead, &custom_noETTOL);
            mystrcpy(settings.ETTOAD, &custom_TTOAlmDur);

            oldCfgOnSD = settings.CfgOnSD[0];
            evalCB(settings.CfgOnSD, &custom_CfgOnSD);
            //evalCB(settings.sdFreq, &custom_sdFrq);
            evalCB(settings.timesPers, &custom_ttrp);

            #ifdef SERVOSPEEDO
            getParam("tinp", settings.ttinpin, 1, 0);
            getParam("toutp", settings.ttoutpin, 1, 0);
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

        } else {

            // Parameters on HA/MQTT Settings page

            #ifdef TC_HAVEMQTT
            evalCB(settings.useMQTT, &custom_useMQTT);
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            getParam("mprot", settings.mqttVers, 1, 0);
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            strcpyutf8(settings.mqttTopic, custom_mqttTopic.getValue(), sizeof(settings.mqttTopic));
            evalCB(settings.pubMQTT, &custom_pubMQTT);
            evalCB(settings.MQTTvarLead, &custom_vMQTT);
            evalCB(settings.mqttPwr, &custom_mqttPwr);
            evalCB(settings.mqttPwrOn, &custom_mqttPwrOn);
            #endif

        }

        // Write settings if requested, or no settings file exists
        if(shouldSaveConfig >= 1 || !checkConfigExists()) {
            write_settings();
        }
        
        shouldSaveConfig = 0;

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
        if(wifiAPOffDelay > 0 && !bttfnHaveClients) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                wifiOff(false);
                wifiAPIsOff = true;
                wifiIsOff = false;
                syncTrigger = false;
                #ifdef TC_DBG_WIFI
                Serial.println("WiFi (AP-mode) switched off (power-save)");
                #endif
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
        wm.startConfigPortal(realAPName, settings.appw, settings.ssid, settings.pass);
        doOnlyAP = true;
    }
    
    // Automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(!doOnlyAP && wm.autoConnect(settings.ssid, settings.pass, realAPName, settings.appw)) {
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
        // with MQTT.
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
    *newversion = 0;
    if(WiFi.status() == WL_CONNECTED) {
        IPAddress remote_addr;
        if(WiFi.hostByName(WEBHOME "v.out-a-ti.me", remote_addr)) {
            int curr = 0, revision = 0;
            if(sscanf(CURRVERSION, "V%d.%d", &curr, &revision) == 2) {
                if(((remote_addr[0] << 8) | remote_addr[1]) > ((curr << 8) | revision)) {
                    snprintf(newversion, sizeof(newversion), "%d.%d", remote_addr[0], remote_addr[1]);
                }
            }
        }
    }
}

bool updateAvailable()
{
    return (*newversion != 0);
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

    #ifdef TC_DBG_WIFI
    Serial.printf("saveWiFiCallback: New ssid '%s'\n", settings.ssid);
    Serial.printf("saveWiFiCallback: New pass '%s'\n", settings.pass);
    #endif
    
    shouldSaveConfig = 1;
}

// This is the callback from the actual Params page. We set
// a flag for the loop to read out and save the new settings.
// paramspage is 1 or 2
static void saveParamsCallback(int paramspage)
{
    shouldSaveConfig = paramspage + 1;
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
    if(useSpeedoDisplay) speedo.off();
    
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

    #ifdef TC_DBG_WIFI
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

    #ifdef TC_DBG_WIFI
    if(strlen(ipBuf) > 0) {
        Serial.printf("IP:%s / SN:%s / GW:%s / DNS:%s\n", ipBuf, snBuf, gwBuf, dnsBuf);
    } else {
        Serial.println("Static IP unset, using DHCP");
    }
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef TC_DBG_WIFI
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
            #ifdef TC_DBG_WIFI
            Serial.println("New static IP config");
            #endif
        } else {
            #ifdef TC_DBG_WIFI
            Serial.println("Static IP config unchanged");
            #endif
        }

    } else {

        #ifdef TC_DBG_WIFI
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

bool checkIPConfig()
{
    return (strlen(ipsettings.ip) > 0 &&
            isIp(ipsettings.ip)       &&
            isIp(ipsettings.gateway)  &&
            isIp(ipsettings.netmask)  &&
            isIp(ipsettings.dns));
}

static void setupStaticIP()
{
    IPAddress ip;
    IPAddress gw;
    IPAddress sn;
    IPAddress dns;

    if(checkIPConfig()) {

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

    if(!haveAudioFiles || *newversion) {
        mySize += 4;  // <hr>
        mySize += STRLEN(menu_myDiv) + STRLEN(menu_end);
        mySize += !haveAudioFiles ? (STRLEN(menu_isp) + 4) : (STRLEN(menu_uai) + sizeof(newversion));
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
    
    if(!haveAudioFiles || *newversion) {
        strcpy(strBuf, "<hr>");
        strcat(strBuf, menu_myDiv);
        if(!haveAudioFiles) {
            strcat(strBuf, menu_isp);
            strcat(strBuf, rspv);
        } else {
            strcat(strBuf, menu_uai);
            strcat(strBuf, newversion);
        }
        strcat(strBuf, menu_end);
    }

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

    setCBVal(&custom_sARA, settings.autoRotAnim);
    setCBVal(&custom_skTTa, settings.skipTTAnim);
    #ifndef IS_ACAR_DISPLAY
    setCBVal(&custom_p3an, settings.p3anim);
    #endif
    setCBVal(&custom_playTTSnd, settings.playTTsnds);
    setCBVal(&custom_alarmRTC, settings.alarmRTC);
    setCBVal(&custom_mode24, settings.mode24);

    custom_timeZone.setValue(settings.timeZone, 63);
    custom_ntpServer.setValue(settings.ntpServer, 63);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_gpstime, settings.useGPSTime);
    #endif

    custom_timeZone1.setValue(settings.timeZoneDest, 63);
    custom_timeZone2.setValue(settings.timeZoneDep, 63);
    custom_timeZoneN1.setValue(settings.timeZoneNDest, DISP_LEN);
    custom_timeZoneN2.setValue(settings.timeZoneNDep, DISP_LEN);

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

    // Speedo type done on-the-fly
    custom_speedoBright.setValue(settings.speedoBright, 2);
    setCBVal(&custom_spAO, settings.speedoAO);
    setCBVal(&custom_sAF, settings.speedoAF);
    custom_speedoFact.setValue(settings.speedoFact, 3);
    setCBVal(&custom_sL0, settings.speedoP3);
    setCBVal(&custom_s3rd, settings.speedo3rdD);
    #ifdef TC_HAVEGPS
    setCBVal(&custom_useGPSS, settings.dispGPSSpeed);
    // Update rate done on-the-fly
    #endif
    #ifdef TC_HAVETEMP
    setCBVal(&custom_useDpTemp, settings.dispTemp);
    custom_tempBright.setValue(settings.tempBright, 2);
    setCBVal(&custom_tempOffNM, settings.tempOffNM);
    #endif

    setCBVal(&custom_fakePwrOn, settings.fakePwrOn);
    
    custom_ettDelay.setValue(settings.ettDelay, 5);
    //setCBVal(&custom_ettLong, settings.ettLong);
    
    #ifdef TC_HAVEGPS
    setCBVal(&custom_qGPS, settings.provGPS2BTTFN);
    #endif   

    #ifdef TC_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    custom_mqttServer.setValue(settings.mqttServer, 79);
    // protocol version done on-the-fly
    custom_mqttUser.setValue(settings.mqttUser, 63);
    custom_mqttTopic.setValue(settings.mqttTopic, 63);
    setCBVal(&custom_pubMQTT, settings.pubMQTT);
    setCBVal(&custom_vMQTT, settings.MQTTvarLead);
    setCBVal(&custom_mqttPwr, settings.mqttPwr);
    setCBVal(&custom_mqttPwrOn, settings.mqttPwrOn);
    #endif

    setCBVal(&custom_ETTOcmd, settings.ETTOcmd);
    setCBVal(&custom_useETTO, settings.useETTO);
    setCBVal(&custom_ETTOalm, settings.ETTOalm);
    setCBVal(&custom_ETTOPUS, settings.ETTOpus);
    setCBVal(&custom_noETTOL, settings.noETTOLead);
    custom_TTOAlmDur.setValue(settings.ETTOAD, 3);

    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);
    setCBVal(&custom_ttrp, settings.timesPers);

    // tt-out, tt-in done on-the-fly
    
    setCBVal(&custom_rvapm, settings.revAmPm);
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
#endif

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

static const char *wmBuildAnmPreset(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = STRLEN(anmCustHTML1) - (6*2) + 
                     STRLEN(custHTMLHdr1) + STRLEN(custHTMLHdr2) + STRLEN(custHTMLSHdr) +
                     2 + STRLEN(custHTMLSel) + STRLEN(ooe) + 4;
    
    for(int i = 0; i < 5; i++) {
        l +=  strlen(anmCustHTMLSrc[i]) - 4 + ((i == 4) ? STRLEN(osde) : STRLEN(ooe));
    }

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    int tnm = atoi(settings.autoNMPreset);
    char *str = (char *)malloc(l);    // actual length 473

    sprintf(str, anmCustHTML1, custHTMLHdr1, custHTMLHdr2, custHTMLSHdr, 
                               settings.autoNMPreset, (tnm == 10) ? custHTMLSel : "", ooe);
    for(int i = 0; i < 5; i++) {
        sprintf(str + strlen(str), anmCustHTMLSrc[i], (tnm == i) ? custHTMLSel : "", (i == 4) ? osde : ooe);
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
                      STRLEN(spTyCustHTML1) + + STRLEN(custHTMLSHdr) +
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
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(spdRateCustHTMLSrc, 6, settings.spdUpdRate, true);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);
    
    buildSelectMenu(str, spdRateCustHTMLSrc, 6, settings.spdUpdRate, true);
    
    return str;
}
#endif

#ifdef SERVOSPEEDO
static const char *wmBuildttinp(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int t = lengthRadioButtons(ttinCustHTMLSrc, 3, settings.ttinpin);

    if(op == WM_CP_LEN) {
        wmLenBuf = t;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(t);
    buildRadioButtons(str, ttinCustHTMLSrc, 3, settings.ttinpin);
        
    return str;
}

static const char *wmBuildttoutp(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int t = lengthRadioButtons(ttoutCustHTMLSrc, 3, settings.ttoutpin);
    
    if(op == WM_CP_LEN) {
        wmLenBuf = t;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(t);
    buildRadioButtons(str, ttoutCustHTMLSrc, 3, settings.ttoutpin);
        
    return str;
}
#endif

static const char *wmBuildApChnl(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(apChannelCustHTMLSrc, 14, settings.apChnl);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, apChannelCustHTMLSrc, 14, settings.apChnl);
    
    return str;
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
{   // "%s%s%s%s</div>";
    unsigned int l = STRLEN(bannerStart) + 7 + STRLEN(bannerMid) + strlen(msg) + 6 + 4;

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
    
    if(ntpLUF)        msg = ntpLUFS;
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
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(mqttpCustHTMLSrc, 4, settings.mqttVers);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, mqttpCustHTMLSrc, 4, settings.mqttVers);
    
    return str;
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
        msg = mqttMsgDisabled;
        cls = col_gr;
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

static void evalCB(char *sv, WiFiManagerParameter *el)
{
    *sv++ = (*(el->getValue()) == '0') ? '0' : '1';
    *sv = 0;
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{   
    el->setValue((*sv == '0') ? "0" : "1", 1);
}

int16_t filterOutUTF8(char *src, char *dst, int srcLen = 0, int maxChars = 99999)
{
    int i, j, slen = srcLen ? srcLen : strlen(src);
    unsigned char c, e;

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
    strncpy(dst, src, len - 1);
    dst[len - 1] = 0;
    truncateUTF8(dst);
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

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    int i = 0, j, ml = (length <= 255) ? length : 255;
    char tempBuf[256];
    static const char *cmdList[] = {
      "\x0a" "TIMETRAVEL",       // 0
      "\x06" "RETURN",           // 1
      "\x08" "ALARM_ON",         // 2
      "\x09" "ALARM_OFF",        // 3
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
      "\x08" "PLAYKEY_",         // 16  PLAYKEY_1..PLAYKEY_9
      "\x07" "STOPKEY",          // 17
      "\x07" "INJECT_",          // 18
      "\x0e" "PLAY_DOOR_OPEN",   // 19 also when !FPBUnitIsOn | PLAY_DOOR_OPEN, PLAY_DOOR_OPEN_L, PLAY_DOOR_OPEN_R
      "\x0f" "PLAY_DOOR_CLOSE",  // 20 also when !FPBUnitIsOn | PLAY_DOOR_CLOSE, PLAY_DOOR_CLOSE_L, PLAY_DOOR_CLOSE_L
      "\x08" "POWER_ON",         // 21 also when !FPBUnitIsOn
      "\x09" "POWER_OFF",        // 22 also when !FPBUnitIsOn
      "\x10" "POWER_CONTROL_ON", // 23 also when !FPBUnitIsOn
      "\x11" "POWER_CONTROL_OFF",// 24 also when !FPBUnitIsOn
      NULL
    };

    if(!length) return;

    if(!strcmp(topic, "bttf/tcd/cmd")) {

        // Not taking commands under these circumstances:
        if(csf & (CSF_MA|CSF_ST|CSF_P0|CSF_P1|CSF_RE))
            return;

        if(ml < 4) return;

        int tempBufLen;

        for(tempBufLen = 0; tempBufLen < ml; tempBufLen++) {
            char a = *payload++;
            if(a >= 'a' && a <= 'z') a &= ~0x20;
            tempBuf[tempBufLen] = a;
        }
        tempBuf[tempBufLen] = 0;

        while(cmdList[i]) {
            const char *t = cmdList[i];
            j = *t++;
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

        // Only command accepted while off are PLAY_DOOR_xx and POWER_xx
        if(!FPBUnitIsOn && (i < 19))
            return;

        switch(i) {
        case 0:
            isEttKeyPressed = isEttKeyImmediate = true;
            break;
        case 1:
            isEttKeyHeld = true;
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
    
        #ifdef TC_DBG_MQTT
        Serial.printf("MQTT: Message about [%s]: %s\n", topic, mqttMsg);
        #endif
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

                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Attempting to (re)connect");
                #endif
    
                if(strlen(mqttUser)) {
                    success = mqttClient.connect(mqttUser, strlen(mqttPass) ? mqttPass : NULL);
                } else {
                    success = mqttClient.connect();
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
        if(!mqttClient.subscribe("bttf/tcd/cmd", settings.mqttTopic)) {
            if(!mqttClient.subscribe("bttf/tcd/cmd")) {
                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Failed to subscribe to all topics");
                #endif
            } else {
                #ifdef TC_DBG_MQTT
                Serial.println("MQTT: Failed to subscribe to user topic");
                #endif
            }
        } else {
            #ifdef TC_DBG_MQTT
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
