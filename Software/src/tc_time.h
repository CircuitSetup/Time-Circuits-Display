/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Time Circuits Display
 * (C) 2021-2022 John deGlavina https://circuitsetup.us
 * (C) 2022-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Time-Circuits-Display
 * https://tcd.out-a-ti.me
 *
 * Time and Main Controller
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

#ifndef _TC_TIME_H
#define _TC_TIME_H

#include "rtc.h"
#include "tcddisplay.h"
#ifdef TC_HAVEGPS
#include "gps.h"
#endif
#include "speeddisplay.h"
#if defined(TC_HAVELIGHT) || defined(TC_HAVETEMP)
#include "sensors.h"
#endif

void      time_boot();
void      time_setup();
void      time_loop();

int       timeTravelProbe(bool doComplete, bool& withSpeedo, bool forceNoLead = false);
int       timeTravel(bool doComplete, bool withSpeedo, bool forceNoLead = false);

void      send_refill_msg();
void      send_wakeup_msg();

void      send_abort_msg();

void      setTTOUTpin(uint8_t val);
void      ettoPulseEnd();

void      bttfnSendFluxCmd(uint32_t payload);
void      bttfnSendSIDCmd(uint32_t payload);
void      bttfnSendPCGCmd(uint32_t payload);
void      bttfnSendVSRCmd(uint32_t payload);
void      bttfnSendAUXCmd(uint32_t payload);
void      bttfnSendRemCmd(uint32_t payload);

void      resetPresentTime();

bool      currentlyOnTimeTravel();

void      backupDestTime();
void      restoreDestTime();
void      backupLastTime();
void      restoreLastTime();

void      updatePresentTime(uint8_t wdplus1 = 0);
void      updateStalePresent(int index);

void      pauseAuto();
bool      checkIfAutoPaused();
void      endPauseAuto(void);

#ifdef TC_HAVEMQTT
void      mqttFakePowerControl(bool);
void      mqttFakePowerOn();
void      mqttFakePowerOff();
#endif
void      fpbKeyPressed();
void      fpbKeyLongPressStop();

void      myCustomDelay_KP(int iter, unsigned long mydel);

void      pwrNeedFullNow(bool force = false);

void      myrtcnow(DateTime& dt);

uint64_t  millis64();
unsigned long millisNonZero();

bool      startSnooze();
void      cancelSnooze();
bool      snoozeRunning();

void      enableWcMode(bool onOff);
bool      toggleWcMode();
bool      isWcMode();
void      setDatesTimesWC(DateTime& dt);

void      enableMiniMode(bool onOff);
bool      toggleMiniMode();
bool      isMiniMode();

void      enableRcMode(bool onOff);
bool      toggleRcMode();
bool      isRcMode();

#ifdef TC_HAVETEMP
bool      tempInCelsius();
#endif

#ifdef TC_HAVE_RE
void      re_vol_reset();
#endif

void      flushDelayedSave();

void      animate(bool withLEDs = false);
void      allShowPattern();
void      allOff();
void      allOn();
void      allresetBrightness();
void      loadUserDLTimes();

#ifdef TC_HAVEGPS
bool      gpsHaveFix();
bool      gpsMakePos(char *lat, char *lon);
bool      haveNavMode();
void      enableNavMode(bool onOff);
bool      toggleNavMode();
int       gpsGetDM();
void      setNavDisplayMode(int dm);
#endif
bool      isNavMode();

#if defined(TC_HAVEGPS) || defined(TC_HAVE_RE) || defined(TC_HAVE_REMOTE)
void      speedoUpdate_loop(bool async);
#endif

void      mydelay(unsigned long mydel);

uint8_t   dayOfWeek(int d, int m, int y);
int       daysInMonth(int month, int year);
bool      isLeapYear(int year);
uint32_t  getHrs1KYrs(int index);
#ifdef TC_JULIAN_CAL
void      correctNonExistingDate(int year, int month, int& day);
#endif
uint8_t*  e(uint8_t *, uint32_t, int);
void      correctYr4RTC(uint16_t& year, int16_t& offs);
int       mins2Date(int year, int month, int day, int hour, int mins);
bool      parseTZ(int index, int currYear, bool doparseDST = true);
int       timeIsDST(int index, int year, int month, int day, int hour, int mins, int& currTimeMins);
void      UTCtoLocal(DateTime &dtu, DateTime& dtl, int index);
void      LocalToUTC(int& ny, int& nm, int& nd, int& nh, int& nmm, int index);

void      ntp_setup(bool doUseNTP, IPAddress& ntpServer, bool couldHaveNTP, bool ntpLUF);
void      ntp_loop();
void      ntp_short_loop();
int       ntp_status();

int       bttfnNumClients();
bool      bttfnGetClientInfo(int c, char **id, uint8_t **ip, uint8_t *type);
bool      bttfn_loop(uint32_t taskMask = 0);
bool      bttfn_loop_ex();
void      bttfn_notify_info();

#ifdef TC_HAVE_REMOTE
void      removeRemote();
void      removeKPRemote();
#endif

#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Dash gauges
#define BTTFN_TYPE_VSR     4    // VSR
#define BTTFN_TYPE_AUX     5    // Aux (user custom device)
#define BTTFN_TYPE_REMOTE  6    // Futaba remote control
#define BTTFN_TYPE__MIN    1
#define BTTFN_TYPE__MAX    BTTFN_TYPE_REMOTE

#define AUTONM_NUM_PRESETS 5

#define BEEPM2_SECS 30
#define BEEPM3_SECS 60

#define REM_BRAKE 1900

extern bool showUpdAvail;

extern uint16_t lastYear;

extern DateTime gdtu, gdtl;
extern bool couldDST[3];

extern uint32_t wcf;
#define WCF_HaveWCM     0x0001
#define WCF_HaveTZ1     0x0002
#define WCF_HaveTZ2     0x0004
#define WCF_Trigger     0x0008
#define WCF_showName1   0x0040
#define WCF_showName2   0x0080
#define WCF_HaveRCM     0x8000
extern int destShowAlt, depShowAlt;

extern bool syncTrigger;
extern unsigned long syncTriggerNow;
extern bool doAPretry;
extern bool deferredCP;

extern uint64_t lastAuthTime64;

extern tcdDisplay destinationTime;
extern tcdDisplay presentTime;
extern tcdDisplay departedTime;

extern uint32_t ttinpin;
extern uint32_t ttoutpin;

extern int           doorSnd;
extern uint32_t      doorFlags;
extern unsigned long doorSndDelay;
extern unsigned long doorSndNow;
extern int           door2Snd;
extern uint32_t      door2Flags;
extern unsigned long door2SndDelay;
extern unsigned long door2SndNow;

extern bool playTTsounds;

extern uint32_t sgf;
#define SGF_USpeedo       0x0001    // useSpeedo
#define SGF_USpeedoDisp   0x0002    // useSpeedoDisplay
#define SGF_UTemp         0x0004    // useTemp
#define SGF_DispTemp      0x0008    // dispTemp
#define SGF_URotEnc       0x0010    // useRotEnc
#define SGF_DispRotEnc    0x0020    // dispRotEnc
#define SGF_SpAlwsOn      0x0040    // speedoAlwaysOn
#define SGF_UGPS          0x0080    // useGPS
#define SGF_UGPSTime      0x0100    // useGPSTime
#define SGF_DispGPSSpd    0x0200    // dispGPSSpeed
#define SGF_GPS2BTTFN     0x0400    // provGPS2BTTFN
#define SGF_TempCelsius   0x2000    // tempUnit
#define SGF_ULightSens    0x4000    // useLight
#define SGF_URotEncVol    0x8000    // useRotEncVol

extern speedDisplay speedo;
#ifdef TC_HAVETEMP
extern tempSensor tempSens;
#endif
#ifdef TC_HAVELIGHT
extern lightSensor lightSens;
#endif
#ifdef TC_HAVE_REMOTE
extern bool remoteAllowed;
extern bool remoteKPAllowed;
#endif

extern tcRTC rtc;

extern int8_t        manualNightMode;
extern unsigned long manualNMNow;
extern bool          forceReEvalANM;

extern uint8_t remMonth;
extern uint8_t remDay;
extern uint8_t remHour;
extern uint8_t remMin;

extern unsigned long ctDown;
extern unsigned long ctDownNow;

extern bool          alarmOnOff;
extern uint8_t       alarmHour;
extern uint8_t       alarmMinute;
extern uint8_t       alarmWeekday;
extern unsigned long alarmPlaying;
extern uint32_t      alf;
#define ALF_RTC      0x0001   // alarmRTC: Alarm/HourlySound based on RTC
#define ALF_ADV      0x0002   // alarmAdv
#define ALF_SNOOZE   0x0004   // doSnooze
#define ALF_ASNOOZE  0x0008   // autoSnooze
#define ALF_HS       0x0010   // alarmHardStop
#define ALF_USRLOOP  0x0020   // userAlarmLoop
#define ALF_RANOUT   0x0040   // alarmRanOut

extern bool          ETTOcommands;

extern uint8_t          autoInterval;
extern const uint8_t    autoTimeIntervals[6];
extern int              autoIntAnimRunning;
#define NUM_AUTOTIMES 11
extern const dateStruct destinationTimes[NUM_AUTOTIMES];
extern const dateStruct departedTimes[NUM_AUTOTIMES];

extern bool       stalePresent;
extern dateStruct stalePresentTime[2];

extern int  specDisp;

extern uint32_t csf;
#define CSF_P0        0x00000001    // Time Travel sequence stages
#define CSF_P1        0x00000002
#define CSF_RE        0x00000004
#define CSF_P2        0x00000008
#define CSF_ST        0x00000010    // "startup" sequence running
#define CSF_OFF       0x00000020    // We are fake-off (if bit set)
#define CSF_MA        0x00000040    // Menu active = busy
#define CSF_NS        0x00000080    // No scan: Suppress WiFi Scan (so we don't have to abuse any other flag)
#define CSF_NM        0x00000100    // Night mode
#define CSF_AL        0x00000200    // Alarm (Extended mode only)
#define CSF_AE        0x00000400    // Enter pressed during Alarm, extends AL period (Extended mode only)
#define CSF_HAVEREM   0x00000800    // Remote present
#define CSF_RSM       0x00001000    // Remote is speed-master
#define CSF_RPM       0x00002000    // Remote is power-master
#define CSF_MQTTPM    0x00004000    // MQTT is power-master (MQTTPwrMaster)
#define CSF_RESTOREFP 0x00008000    // restore fake-power
#define CSF_PWRLOW    0x40000000    // CPU in power-safe
#define CSF_BOOTSTRAP 0x80000000    // We are in boot-stap

// bttfn_loop() taskMask
#define BNLP_SK_MC      1   // skip MC socket poll
#define BNLP_SK_SP      2   // skip socket poll
#define BNLP_SK_NOTDATA 4   // skip sending out NOT_DATA
#define BNLP_SK_EXPIRE  8   // skip client expiry

extern uint32_t  mqttDisp;
#ifdef TC_HAVEMQTT
#define MQ_DISP_D 1
#define MQ_DISP_P 2
#define MQ_DISP_L 4
extern uint32_t mqttOldDisp;
extern char     *mqttMsg[];
extern uint16_t mqttIdx[];
extern int16_t  mqttMaxIdx[];
extern bool     mqttST[];
#endif

extern bool bttfnHaveClients;

// Time Travel difference to RTC
extern uint64_t timeDifference;
extern bool     timeDiffUp;

extern bool timetravelPersistent;

extern bool MQTTWaitForOn;

extern uint8_t beepMode;
extern bool beepTimer;
extern unsigned long beepTimeout;
extern unsigned long beepTimerNow;

#endif
