/**
 * wm_strings_en.h
 *
 * Based on:
 *
 * WiFiManager, a library for the ESP32/Arduino platform
 *
 * @author Creator tzapu
 * @author tablatronix
 * @version 2.0.15+A10001986
 * @license MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 */

#ifndef _WM_STRINGS_EN_H_
#define _WM_STRINGS_EN_H_

#include "wm_consts_en.h"

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
    "<html lang='en'><head>"
    "<meta name='format-detection' content='telephone=no'>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
    "<title>{v}</title>";

const char HTTP_SCRIPT[]           PROGMEM = "<script>"
    "function wlp(){return window.location.pathname}"
    "function getn(x){return document.getElementsByTagName(x)}"
    "function ge(x){return document.getElementById(x)}"
    "function gecl(x){return document.getElementsByClassName(x)}"
    "function f(){x=ge('p');x.type==='password'?x.type='text':x.type='password';}"
    "</script>";

const char HTTP_HEAD_END[]         PROGMEM = "</head><body><div class='wrap'>";

const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{t}</h1><h3>{v}</h3>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
    "<form action='/wifi' method='get' onsubmit='d=ge(\"wbut\");if(d){d.disabled=true;d.innerHTML=\"Please wait\"}'><button id='wbut'>WiFi Configuration</button></form>\n",
    "",   // unused, otherwise: "<form action='/0wifi' method='get'><button>WiFi Configuration (No scan)</button></form>\n",
    "<form action='/param' method='get'><button>Settings</button></form>\n",
    "",   // unused, otherwise: "<form action='/erase' method='get'><button class='D'>Erase</button></form>\n",
    "<form action='/update' method='get'><button>Update</button></form>\n",
    "<hr>", // sep
    ""      // custom, if _customMenuHTML is NULL
};

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n"; // do not remove newline!
const char HTTP_FORM_END[]         PROGMEM = "<button type='submit'>Save</button></form>";

const char HTTP_FORM_WIFI[]        PROGMEM = "<div class='sects'><div class='headl'>WiFi connection: Network selection</div><label for='s'>Network name (SSID)</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{V}' oninput='h=ge(\"p\");h.disabled=false;if(!this.value.length&&this.placeholder.length){h.placeholder=h.getAttribute(\"data-ph\")||\"********\";}else{h.placeholder=\"\"}'><br/><label for='p'>Password</label><input id='p' name='p' maxlength='64' type='password' placeholder='{p}' data-ph='{p}'><input type='checkbox' onclick='f()'> Show password when typing";
const char HTTP_FORM_WIFI_END[]    PROGMEM = "</div>";
const char HTTP_WIFI_ITEM[]        PROGMEM = "<div><a href='#p' onclick='return c(this)' data-ssid='{V}'>{v}</a><div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i}'></div></div>";
const char HTTP_FORM_SECTION_HEAD[] PROGMEM = "<div class='sects'><div class='headl'>WiFi connection: Static IP settings</div>";
const char HTTP_FORM_SECTION_FOOT[] PROGMEM = "</div>";
const char HTTP_FORM_WIFI_PH[]     PROGMEM = "placeholder='Leave this section empty to use DHCP'";
const char HTTP_MSG_NONETWORKS[]   PROGMEM = "<div class='msg'>No networks found. Click 'WiFi Scan' to re-scan.</div>";
const char HTTP_MSG_SCANFAIL[]     PROGMEM = "<div class='msg D'>Scan failed. Click 'WiFi Scan' to retry.</div>";
const char HTTP_MSG_NOSCAN[]       PROGMEM = "<div class='msg'>Device busy, WiFi scan prohibited.<br/>Click 'WiFi Scan' after animation sequence has finished.</div>";
const char HTTP_SCAN_LINK[]        PROGMEM = "<form action='/wifi?refresh=1' method='POST' onsubmit='if(confirm(\"This will reload the page, changes are not saved. Proceed?\")){d=ge(\"wrefr\");if(d){d.disabled=true;d.innerHTML=\"Please wait\"}return true;}return false;'><button id='wrefr' name='refresh' value='1'>WiFi Scan</button></form>";
const char HTTP_ERASE_BUTTON[]     PROGMEM = "<button form='erform' type='submit'>Forget saved WiFi network</button>";
const char HTTP_ERASE_FORM[]       PROGMEM = "<form id='erform' action='/erase' method='get' onsubmit='return confirm(\"Forget saved WiFi network, including static IP settings?\\n\\nDevice will restart in access point mode.\\nChanges made below will NOT be saved.\");'></form>";

const char HTTP_SAVED[]            PROGMEM = "<div class='msg'>Credentials saved.<br/>";
const char HTTP_SAVED_NORMAL[]     PROGMEM = "Trying to connect to network.<br />In case of error, device boots in AP mode.</div>";
const char HTTP_SAVED_CARMODE[]    PROGMEM = "Device is run in <strong>car mode<strong> and will <strong>not</strong><br/>connect to saved WiFi network after reboot.</div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg S'>Settings saved. Rebooting.<br/></div>";

const char HTTP_UPDATE[]           PROGMEM = "Upload new firmware<br/><form method='POST' action='u' enctype='multipart/form-data' onsubmit=\"var aa=ge('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uacb');if(aa){aa.disabled=true}\" onchange=\"(function(el){ge('uploadbin').style.display=el.value==''?'none':'initial';})(this)\"><input type='file' name='update' accept='.bin,application/octet-stream'><button id='uploadbin' type='submit' class='h D'>Update</button></form>";
const char HTTP_UPLOADSND1[]       PROGMEM = "Upload sound pack (";
const char HTTP_UPLOADSND2[]       PROGMEM = ".bin)<br>and/or .mp3 file(s)<br><form method='POST' action='uac' enctype='multipart/form-data' onsubmit=\"var aa=ge('uacb');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uploadbin');if(aa){aa.disabled=true}\" onchange=\"(function(el){ge('uacb').style.display=el.value==''?'none':'initial';})(this)\"><input type='file' name='upac' multiple accept='.bin,application/octet-stream,.mp3,audio/mpeg'><button id='uacb' type='submit' class='h'>Upload</button></form>";
const char HTTP_UPDATE_FAIL1[]     PROGMEM = "<div class='msg D'><strong>Update failed.</strong><br/>";
const char HTTP_UPDATE_FAIL2[]     PROGMEM = "</div>";
const char HTTP_UPDATE_SUCCESS[]   PROGMEM = "<div class='msg S'><strong>Update successful.</strong><br/>Device rebooting.</div>";
const char HTTP_UPLOAD_SDMSG[]     PROGMEM = "<div class='msg' style='text-align:left'>In order to upload the sound-pack,<br/>please insert an SD card.</div>";

const char HTTP_ERASED[]           PROGMEM = "<div class='msg S'>The WiFi network has been forgotten.<br />Restarting in AP mode.<br/></div>";

const char HTTP_BACKBTN[]          PROGMEM = "<hr><form action='/' method='get'><button>Back</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg S'><strong>Connected</strong> to {v}<br/><em><small>with IP {i}</small></em></div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg {c}'><strong>Not connected</strong> to {v}{r}<br/><em><small>Currently operating in {V}</small></em></div>";
const char HTTP_STATUS_APMODE[]    PROGMEM = "AP-mode";
const char HTTP_STATUS_CARMODE[]   PROGMEM = "car mode";
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/>Network not found";    // WL_NO_SSID_AVAIL
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/>Could not connect";    // WL_CONNECT_FAILED
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='msg'>No WiFi connection configured</div>";
const char HTTP_BR[]               PROGMEM = "<br/>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
    ".c,body{text-align:center;font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}"
    "div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
    "input,button,select,.msg{border-radius:.3rem;width: 100%}"
    "input[type=radio],input[type=checkbox]{width:auto}"
    "button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background-color:#225a98;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
    "input[type='file']{border:1px solid #225a98}"
    ".h{display:none}"
    ".wrap{text-align:left;display:inline-block;min-width:260px;max-width:500px}"
    ".headl{margin:0 0 7px 0;padding:0}"
    ".sects{background-color:#eee;border-radius:7px;margin-bottom:20px;padding-bottom:7px;padding-top:7px}"
    // links
    "a{color:#000;font-weight:bold;text-decoration:none}"
    "a:hover{color:#225a98;text-decoration:underline}"
    // msg callouts
    ".msg{padding:20px;margin:20px 0;border:1px solid #ccc;border-radius:20px;border-left-width:15px;border-left-color:#777;background:linear-gradient(320deg,rgb(255,255,255) 0%,rgb(235,234,233) 100%)}"
    ".msg.P{border-left-color:#225a98}"
    ".msg.D{border-left-color:#dc3630}"
    ".msg.S{border-left-color:#609b71}"
    // buttons
    "button{transition:0s opacity;transition-delay:3s;transition-duration:0s;cursor:pointer}"
    "button.D{background-color:#dc3630;border-color:#dc3630}"
    "button:active{opacity:50% !important;cursor:wait;transition-delay:0s}"
    ":disabled {opacity:0.5;}"
    "</style>";

// quality icons plus some specific JS
const char HTTP_STYLE_QI[]         PROGMEM = "<style>"
    ".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}"
    ".q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}"
    ".q.q-2:after{background-position-x:-32px}"
    ".q.q-3:after{background-position-x:-48px}"
    ".q.q-4:after{background-position-x:-64px}"
    ".q.l:before{background-position-x:-80px;padding-right:5px}"
    ".ql .q{float:left}"
    ".q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position: 16px 0;"
    "background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');}"
    // icons @2x media query (32px rescaled)
    "@media (-webkit-min-device-pixel-ratio:2),(min-resolution:192dpi){.q:before,.q:after{"
    "background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');"
    "background-size: 95px 16px;}}"
    "</style>"
    "<script>function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
    "p=l.nextElementSibling.classList.contains('l');pp=ge('p');"
    "pp.disabled=!p;pp.placeholder='';if(p){pp.focus()}return false;}"
    "</script>";

const char A_paramsave[]          PROGMEM = "paramsave";
const char A_wifisave[]           PROGMEM = "wifisave";

const char S_titlewifi[]          PROGMEM = "WiFi Configuration";
const char S_titleparam[]         PROGMEM = "Settings";
const char S_titleupd[]           PROGMEM = "Upload";

const char S_passph[]             PROGMEM = "********";
const char S_staticip[]           PROGMEM = "Static IP";
const char S_staticgw[]           PROGMEM = "Static gateway";
const char S_staticdns[]          PROGMEM = "Static DNS";
const char S_subnet[]             PROGMEM = "Subnet mask";

const char S_brand[]              PROGMEM = "WiFiManager";

const char S_GET[]                PROGMEM = "GET";
const char S_POST[]               PROGMEM = "POST";
const char S_NA[]                 PROGMEM = "Unknown";

const char S_notfound[]           PROGMEM = "404 File not found\n\n";
const char S_debugPrefix[]        PROGMEM = "*wm:";

// debug strings
#ifdef WM_DEBUG_LEVEL
const char D_HR[]                 PROGMEM = "--------------------";
#endif

#endif  // _WM_STRINGS_EN_H_
