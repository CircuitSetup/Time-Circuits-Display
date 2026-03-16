/**
 * wm_strings_en.h
 *
 * Based on:
 * WiFiManager, a library for the ESP32/Arduino platform
 * Creator tzapu (tablatronix)
 * Version 2.0.15
 * License MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 */

#ifndef _WM_STRINGS_EN_H_
#define _WM_STRINGS_EN_H_

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
    "<html lang='en'><head>"
    "<meta name='format-detection' content='telephone=no'>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>"
    "<title>{v}</title>";

const char HTTP_SCRIPT[]           PROGMEM = "<script>"
    "function wlp(){return window.location.pathname}"
    "function getn(x){return document.getElementsByTagName(x)}"
    "function ge(x){return document.getElementById(x)}"
    "function gecl(x){return document.getElementsByClassName(x)}"
    "function f(){x=ge('p');x.type==='password'?x.type='text':x.type='password'}"
    "</script>";

const char HTTP_HEAD_END[]         PROGMEM = "</head><body><div class='wrap'>";

const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{t}</h1><h3>{v}</h3>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
    "<form action='/wifi' method='get' onsubmit='d=ge(\"wbut\");if(d){d.disabled=true;d.innerHTML=\"Please wait\"}'><button id='wbut'>WiFi Configuration</button></form>\n",
    "<form action='/param' method='get'><button>Settings</button></form>\n",
    "<form action='/param2' method='get'><button>" WM_PARAM2_CAPTION "</button></form>\n",
    "<form action='/update' method='get'><button>Update</button></form>\n",
    "<hr>", // sep
    ""      // custom, if _customMenuHTML is NULL
};

const char HTTP_DIV_END[]          PROGMEM = "</div>";

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<input id='{i}' name='{n}' {l} value='{v}' {c} {f}>";
const char HTTP_FORM_END[]         PROGMEM = "<button type='submit'>Save</button></form>";

const char HTTP_FORM_WIFI[]        PROGMEM = "<div class='ss'><div class='hl'>WiFi connection: Network selection</div><label for='s'>Network name (SSID)</label><br><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{V}' oninput='var x=ge(\"fg\");var y=ge(\"p\");y.disabled=false;if(!this.value.length&&this.placeholder.length){if(x&&!y.value.length){x.style.display=\"\"}y.placeholder=y.getAttribute(\"data-ph\")||\"********\";}else{if(x){x.style.display=\"none\"}y.placeholder=\"\"}'><br><label for='p'>Password</label><br><input id='p' name='p' maxlength='64' type='password' placeholder='{p}' data-ph='{p}' oninput='var x=ge(\"fg\");if(x){var y=ge(\"s\");if(!y.value.length&&y.placeholder.length){if(this.value.length){x.style.display=\"none\"}else{x.style.display=\"\"}}}'><br><label><input type='checkbox' onclick='f()' style='margin:0px 5px 10px 0px'>Show password when typing</label><br>";
//const char HTTP_FORM_WIFI_END[]  PROGMEM = "</div>";
#define HTTP_FORM_WIFI_END         HTTP_DIV_END
const char HTTP_WIFI_ITEM[]        PROGMEM = "<div><a href='#p' onclick='return {t}(this)' data-ssid='{V}'>{v}</a>{c}<div role='img' aria-label='{r}dBm' title='{r}dBm' class='q q-{q} {i}'></div></div>";
const char HTTP_FORM_SECT_HEAD[]   PROGMEM = "<div class='ss'><div class='hl'>WiFi connection: Static IP settings</div>";
//const char HTTP_FORM_SECT_FOOT[] PROGMEM = "</div>";
#define HTTP_FORM_SECT_FOOT        HTTP_DIV_END
const char HTTP_FORM_WIFI_PH[]     PROGMEM = "placeholder='Leave this section empty to use DHCP'";
const char HTTP_MSG_NONETWORKS[]   PROGMEM = "<div class='msg'>No networks found. Click 'WiFi Scan' to re-scan.</div>";
const char HTTP_MSG_SCANFAIL[]     PROGMEM = "<div class='msg D'>Scan failed. Click 'WiFi Scan' to retry.</div>";
const char HTTP_MSG_NOSCAN[]       PROGMEM = "<div class='msg'>Device busy, WiFi scan prohibited.<br>Click 'WiFi Scan' after animation sequence has finished.</div>";
const char HTTP_SCAN_LINK[]        PROGMEM = "<form action='/wifi?refresh=1' method='POST' onsubmit='if(confirm(\"This will reload the page, changes are not saved. Proceed?\")){var d=ge(\"wrefr\");if(d){d.disabled=true;d.innerHTML=\"Please wait\"}return true;}return false;'><button id='wrefr' name='refresh' value='1'>WiFi Scan</button></form>";
const char HTTP_ERASE_BUTTON[]     PROGMEM = "<div id='fg' class='c' style='border:2px solid #dc3630;border-radius:7px'><label><input id='fgn' name='fgn' type='checkbox' style='margin-top:0'>Forget saved WiFi network</label></div>";
const char HTTP_SHOWALL[]          PROGMEM = "<div class='c'><button class='s' form='saform' type='submit'>Show all</button></div>";
const char HTTP_SHOWALL_FORM[]     PROGMEM = "<form id='saform' action='/wifi?showall=1' method='POST'></form>";

const char HTTP_PARAMSAVED[]       PROGMEM = "<div id='lc' class='msg S'>Settings saved. Rebooting.<br>";
const char HTTP_SAVED_NORMAL[]     PROGMEM = "Trying to connect to network.<br>In case of error, device boots in AP mode.";
const char HTTP_SAVED_CARMODE[]    PROGMEM = "<br>Device is run in <strong>car mode</strong> and will <em>not</em><br>connect to WiFi network after reboot.";
const char HTTP_SAVED_ERASED[]     PROGMEM = "WiFi network credentials deleted.<br>Restarting in AP mode.<br>";
//const char HTTP_PARAMSAVED_END[] PROGMEM = "</div>";
#define HTTP_PARAMSAVED_END        HTTP_DIV_END

const char HTTP_UPDATE[]           PROGMEM = "<form method='POST' action='u' enctype='multipart/form-data' onsubmit=\"var aa=ge('uploadbin');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uacb');if(aa){aa.disabled=true}\" onchange=\"(function(el){ge('uploadbin').style.display=el.value==''?'none':'initial';})(this)\"><div class='ss'>Upload new firmware<br><input type='file' name='update' accept='.bin,application/octet-stream'><br><button id='uploadbin' type='submit' class='h D'>Update</button></div></form>";
const char HTTP_UPLOADSND1[]       PROGMEM = "<form method='POST' action='uac' enctype='multipart/form-data' onsubmit=\"var aa=ge('uacb');if(aa){aa.disabled=true;aa.innerHTML='Please wait'}aa=ge('uploadbin');if(aa){aa.disabled=true}\" onchange=\"(function(el){ge('uacb').style.display=el.value==''?'none':'initial';})(this)\"><div class='ss'>Upload sound pack (";
const char HTTP_UPLOADSND2[]       PROGMEM = ".bin)<br>and/or .mp3 file(s)<br><input type='file' name='upac' multiple accept='.bin,application/octet-stream,.mp3,audio/mpeg'><br><button id='uacb' type='submit' class='h'>Upload</button></div></form>";
const char HTTP_UPDATE_FAIL1[]     PROGMEM = "<div class='msg D'><strong>Update failed.</strong><br>";
//const char HTTP_UPDATE_FAIL2[]   PROGMEM = "</div>";
#define HTTP_UPDATE_FAIL2          HTTP_DIV_END
const char HTTP_UPDATE_SUCCESS[]   PROGMEM = "<div id='lc' class='msg S'><strong>Update successful.</strong><br>Device rebooting.</div>";
const char HTTP_UPLOAD_SDMSG[]     PROGMEM = "<div class='msg'>In order to upload sound files,<br>please insert an SD card.</div>";

const char HTTP_STATUS_HEAD[]      PROGMEM = "<div class='sta'><span class='{c}'>&#x25CF;</span> ";
//const char HTTP_STATUS_TAIL[]    PROGMEM = "</div>";
#define HTTP_STATUS_TAIL           HTTP_DIV_END
const char HTTP_STATUS_ON[]        PROGMEM = "{v}<br>{i}";
const char HTTP_STATUS_OFF[]       PROGMEM = "{v}<br>{r}Operating in {V}mode";
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "Network not found<br>";             // WL_NO_SSID_AVAIL
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "Failed to connect<br>";             // WL_CONNECT_FAILED
const char HTTP_STATUS_DISCONN[]   PROGMEM = "Disconnected. Wrong Password?<br>"; // WL_DISCONNECTED
const char HTTP_STATUS_APMODE[]    PROGMEM = "AP-";
const char HTTP_STATUS_CARMODE[]   PROGMEM = "car ";
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='sta'>No WiFi connection configured";

const char HTTP_BR[]               PROGMEM = "<br>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTML_CHKBOX[]           PROGMEM = "type='checkbox' autocomplete='off'";  // ac=off to fix FF idiocy
const char HTTP_SECT_HEAD[]        PROGMEM = "<div class='ss'>";
//const char HTTP_SECT_START[]     PROGMEM = "</div>"; // + HTTP_SECT_HEAD
#define HTTP_SECT_START            HTTP_DIV_END        // + HTTP_SECT_HEAD
//const char HTTP_SECT_FOOT[]      PROGMEM = "</div>";
#define HTTP_SECT_FOOT             HTTP_DIV_END
const char HTTP_HL_S[]             PROGMEM = "<div class='hl'>";
#define HTTP_HL_E                  HTTP_DIV_END
//const char HTTP_HL_E[]           PROGMEM = "</div>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
    ".c,body{text-align:center;font-family:-apple-system,BlinkMacSystemFont,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}"
    "div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
    "input,button,select,.msg{border-radius:.3rem;width: 100%}"
    "input[type=checkbox],input[type=radio]{display:inline-block;margin-top:10px;margin-right:5px;width:auto;}"
    "button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background-color:#225a98;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
    "input[type='file']{border:1px solid #225a98}"
    "label{display:inline-block;vertical-align:text-top;margin:0 10px 0 0;padding:0 10px 0 0;white-space:normal}"
    ".h{display:none}"
    ".wrap{text-align:left;display:inline-block;min-width:260px;max-width:500px}"
    ".hl{margin:0 0 7px 0;padding:0}"
    ".ss{background-color:#eee;border-radius:7px;margin-bottom:20px;padding:7px 10px 7px 10px;white-space:nowrap}"
    "label.mp0{margin:0;padding:0}"
    // links
    "a{color:#000;font-weight:bold;text-decoration:none}"
    "a:hover{color:#225a98;text-decoration:underline}"
    // status
    ".sta{font-variant:small-caps;color:#888;text-align:center;line-height:1.1em}"
    ".sta>span.n{color:#888}"
    ".sta>span.g{color:#609b71}"
    ".sta>span.r{color:#dc3630}"
    // msg boxes (overruled on some pages)
    ".msg{padding:20px;margin:20px 0;border-top:1px solid #000;border-bottom:1px solid #000;border-radius:0;text-align:center;}"
    ".msg.P{}"
    ".msg.D{border-color:#dc3630;border-width:3px}"
    ".msg.S{}"
    // buttons
    "button{transition:0s opacity;transition-delay:3s;transition-duration:0s;cursor:pointer}"
    "button.D{background-color:#dc3630;border-color:#dc3630}"
    "button:active{opacity:50% !important;cursor:wait;transition-delay:0s}"
    ":disabled{opacity:0.5}"
    "</style>";

const char HTTP_STYLE_MSG[]        PROGMEM = "<style>"
    ".msg.S{padding:60px 20px 20px 20px;margin:20px 0;border:1px inset #000;border-bottom:1px solid #000;border-radius:0;text-align:center;box-shadow:2px 2px 5px 4px inset rgba(0,0,0,0.1);"
    "background: no-repeat url('data:image/png;base64,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') 5px 5px"
    "}"
    "</style>";

// quality icons plus some specific JS
const char HTTP_STYLE_QI[]         PROGMEM = "<style>"
    "button.s{width:initial;line-height:1.3em;margin:0}"
    ".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}"
    ".q.q-0::after{background-position-x:0}"
    ".q.q-1::after{background-position-x:0}"
    ".q.q-2::after{background-position-x:-21px}"
    ".q.q-3::after{background-position-x:-42px}"
    ".q.q-4::after{background-position-x:-63px}"
    ".q.l::before{background-position-x:-84px}"
    ".ql .q{float:left}"
    ".q::after,.q::before{content:'';width:21px;height:16px;display:inline-block;background-repeat:no-repeat;background-position:21px 0;"
    "background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGIAAAAQCAMAAADakVr2AAAA81BMVEUAAADHycnHyckAAADHyckAAAAAAADHycnHyckAAAAAAADHyckAAAAAAAAAAAAAAADHycnHyckAAADHyckAAAAAAADHyckAAAAAAADHyckAAADHyckAAADHycnHyckAAADHyckAAADHyckAAADHyckAAAAAAAAAAAAAAAAAAADHyckAAADHycnHyckAAADHycnHyckAAADHyckAAAAAAADHycnHycnHyckAAADHycnHyckAAAAAAADHyckAAAAAAADHyckAAAAAAAAAAADHyckAAADHycnHyckAAAAAAADHycnHycnHycnHyckAAAAAAADHyclrQTMdAAAAT3RSTlMAKG7W0JlsZrukGaP7zciQRr+8OiSxn0ApHh0WDOvft6melZSRYyET9eKtqJuOXU1LLhrr0cfCi1g/IwkG99qsp4+HcWBFBdR8ODO0WVdVJL4uIQAAAbdJREFUOMullOlWwjAQRqctXSi0IItUQNmFulRZ3EBFRdx18v5P47TRUimLwP2RfLmnJ8lMzwl47IixyO7NbiQm7nCxmtSVodFtdXvDUR5mky6zCQfpVWU7ixNKbZhBlT7fq6vJdFI93qN89LRMXgWljIh2whm/jp17m7IM07xFGNs68ZenGmMRQfivfLMkRCPny5yJeGsBxxpmvZq2GFNpEsSLo2NRoPRILSjPkndcWsp9JaFYXL6nEBsUM7504lgCj2aPjtPdbbQk3YgOIvgtr7ROYa7MGcjp5VwpZIzxX0mLnypS+NpGk+fCNu1zVr2on9Ec6yyQgygimrWHhEnzYZHL6z9yAL+4f+UBeabNYoKX+hTvpqUwkTJiVPeSHqW4QBJYA3hBnvvnIt36U/woAKjnQkDqzsjJB2ReUqjHl8plE6AhZeZLooIt225h9nddqDOXagcCFGvoIjdnyeIy2UUOgN+W5/IzY2UIUEGUDiWqfS0poUfXfxc0kUZV60MA3VRobBj7a8lb9GgF1KA9gDD71+tKO3SEnsUShNhAjlIeLxPzFcd4BqbIbyDDFBMoh+1mkvMNs1qA66I7EMYAAAAASUVORK5CYII=');}"
    // icons @2x media query (32px rescaled)
    "@media (-webkit-min-device-pixel-ratio:2),(min-resolution:192dpi){.q::before,.q::after{"
    "background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMQAAAAgCAMAAAB6rSfNAAABJlBMVEUAAAAAAAAAAAAAAADHyckAAADHyckAAADHycnHyckAAADHyckAAADHycnHyckAAAAAAAAAAAAAAADHyckAAADHyckAAAAAAAAAAADHycnHyckAAADHyckAAADHycnHycnHyckAAAAAAADHyckAAAAAAAAAAAAAAADHyckAAAAAAAAAAAAAAADHycnHyckAAADHyckAAADHyckAAADHyckAAADHyckAAADHyckAAAAAAADHyckAAADHycnHycnHycnHyckAAADHycnHycnHycnHyckAAADHycnHycnHycnHycnHycnHyckAAADHycnHycnHycnHyckAAADHyckAAAAAAAAAAADHyckAAADHyckAAAAAAAAAAADHycmXmZmQkpIAAADHycms9KC6AAAAYHRSTlMA3u8J/LxKI+rfBHFFJg359Oa1sHFXOTInHdvTy5eWd1BIE4J3Uywd1rGhTT/3ko+JWDYvF+rSyq2siW1nYUAK49m3tKNoX0Y7Lyry7sysppwhDwalkoJdxnx7beK+jWod6B+WAAADSklEQVRYw9WV21raQBCANyAECFTOQSByEEREUVREEEVEOSlqFbGe2tn3f4nGsJQsSQhNP/jof7PJzn8zu7MziOKpfZr3Re02mz3qW+fOa1Rw7qY5+P6cLTHFIlPKCv4dMzJALXB/gyl2716+Lco0F/puoHCXm4/o7/i+votVWHlwLcJkhVVQYVUIodk5useanKzN28z0QZNyBs3IxviUtnzrG7HYxrrveAWP2J+vORiffCQrDFKpgZCNjG+mMlslhfGQ26u4rF6/WR9sJHDhmp/J1mGIaTsnewPmxHaRBD5mqKkAHnIcV8asURI8m5dZgCGRtNJM9EiwiXR4xRL2c/XwwRa5/vmYbyDB7KibwbpGSbEVi0XgESGPJU5lV+7qHHRlJXA2NK7+xXwM8XwoqTB/PoPEu7bZHBp9RFFlpAKskt9rLBIeNYs1rhHelR6j/TI22jySDi42s8nRZsbvZValZ8t4UxnK/FWVriEzzUxGQCRF5wAi8m0rxpeT1TpZ0ScYBwyauR5QRNKUmQDw6pllgAJdS2JU4NmKuPCju2gM13YYK7ggVX0XM2YGGVDw4ZSbVa++WU4higpAmawCHbnSGEtPYsyo2dcYYA6DJkG8MlaqNABGvr9mJ4PpkrN2XK5Oi/ORwWQT+7kxMzM6XG8qcciymwl/lowwT8iYSRDtJPkwyXs3mUzckaz3nJK5FECGzASZYamkrPe8e0AibcCUJWFSSyJO+vYEG2QoGTJz6h1+QMaXAVPnJtDBCr45UOpd8Yhb4mrIDAK4g0pzswjgNGISeMtXW5YQP3pywWrvSutTfL8RjTby8drwBYRfiKAwHbk9b6nk3UubtcwcE1IzM/WmIZNgAQqT0uBuMeHmFU3FbwKC+82xOFM/CZedav0dpAlLtXXP4aJM/SQCeIJrpEEBJqguyNRPooW/COc5Lr+lbJnKplh/9vv3ItJnYVGmfjl1d7GNtJj2BcZtpMmmG4pO0lo8AMHFmfoPu/X5Z4bVPgNoCk5LEhHMlvQCTWUS/yOzJeHYcZon95bILANFT1ViPWJ6k/1tiUyeoS6CV5V+gEgE0SyhOY1NkNhZdnMqIZDgl92czjaIZJffnIpDcK9um5ffHPMbdfGaiuWnXuoAAAAASUVORK5CYII=');"
    "background-size:98px 16px}}"
    "</style>"
    "<script>function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
    "var p=l.nextElementSibling.classList.contains('l');var pp=ge('p');"
    "pp.disabled=!p;pp.placeholder='';var x=ge('fg');if(x){x.style.display='none'}if(p){pp.focus()}else{pp.value=''}return false}"
    "function d(l){return false}"
    "</script>";

const char A_paramsave[]          PROGMEM = "paramsave";
const char A_param2save[]         PROGMEM = "param2save";
const char A_wifisave[]           PROGMEM = "wifisave";

const char S_titlewifi[]          PROGMEM = "WiFi Configuration";
const char S_titleparam[]         PROGMEM = "Settings";
#ifdef WM_PARAM2
const char S_titleparam2[]        PROGMEM = WM_PARAM2_TITLE;
#endif
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

const char S_nonprintable[]       PROGMEM = "[Non-printable SSID]";

const char S_notfound[]           PROGMEM = "404 File not found\n\n";

// Routes
const char R_root[]               PROGMEM = "/";
const char R_wifi[]               PROGMEM = "/wifi";
const char R_wifisave[]           PROGMEM = "/wifisave";
const char R_param[]              PROGMEM = "/param";
const char R_paramsave[]          PROGMEM = "/paramsave";
#ifdef WM_PARAM2
const char R_param2[]             PROGMEM = "/param2";
const char R_param2save[]         PROGMEM = "/param2save";
#endif
const char R_update[]             PROGMEM = "/update";
const char R_updatedone[]         PROGMEM = "/u";

// Strings
const char S_ip[]                 PROGMEM = WMS_ip;
const char S_gw[]                 PROGMEM = WMS_gw;
const char S_sn[]                 PROGMEM = WMS_sn;
const char S_dns[]                PROGMEM = WMS_dns;

// Tokens
const char T_v[]                  PROGMEM = "{v}"; // @token v
const char T_V[]                  PROGMEM = "{V}"; // @token v
const char T_I[]                  PROGMEM = "{I}"; // @token I
const char T_i[]                  PROGMEM = "{i}"; // @token i
const char T_n[]                  PROGMEM = "{n}"; // @token n
const char T_p[]                  PROGMEM = "{p}"; // @token p
const char T_t[]                  PROGMEM = "{t}"; // @token t
const char T_l[]                  PROGMEM = "{l}"; // @token l
const char T_c[]                  PROGMEM = "{c}"; // @token c
const char T_e[]                  PROGMEM = "{e}"; // @token e
const char T_q[]                  PROGMEM = "{q}"; // @token q
const char T_r[]                  PROGMEM = "{r}"; // @token r
const char T_R[]                  PROGMEM = "{R}"; // @token R
const char T_h[]                  PROGMEM = "{h}"; // @token h
const char T_f[]                  PROGMEM = "{f}"; // @token f

// http
const char HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char HTTP_HEAD_CT2[]        PROGMEM = "text/plain";
const char HTTP_HEAD_CORS[]       PROGMEM = "Access-Control-Allow-Origin";
const char HTTP_HEAD_CORS_ALLOW_ALL[]  PROGMEM = "*";

// Debug
#ifdef _A10001986_DBG
const char * const WIFI_STA_STATUS[] PROGMEM =
{
    "WL_IDLE_STATUS",     // 0 STATION_IDLE
    "WL_NO_SSID_AVAIL",   // 1 STATION_NO_AP_FOUND
    "WL_SCAN_COMPLETED",  // 2
    "WL_CONNECTED",       // 3 STATION_GOT_IP
    "WL_CONNECT_FAILED",  // 4 STATION_CONNECT_FAIL, STATION_WRONG_PASSWORD(NI)
    "WL_CONNECTION_LOST", // 5
    "WL_DISCONNECTED",    // 6
    "WL_STATION_WRONG_PASSWORD" // 7 KLUDGE
};
const char* const WIFI_MODES[] PROGMEM = { "NULL", "STA", "AP", "STA+AP" };
#endif

#endif  // _WM_STRINGS_EN_H_
