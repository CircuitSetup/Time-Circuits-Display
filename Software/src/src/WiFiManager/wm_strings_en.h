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

#define HTTP_RED  "#be5c9c"   //"#dc3630"
#define HTTP_BLUE "#4f529d"   //"#225a98"

static const char HTTP_HEAD_START[] PROGMEM =
    "<!DOCTYPE html>"
    "<html lang='en'><head>"
    "<meta name='format-detection' content='telephone=no'>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>"
    "<title>{v}</title>";

static const char HTTP_SCRIPT[]     PROGMEM =
    "<script>"
    "his=false;ra=[];"
    "function ge(x){return document.getElementById(x)}"
    "function dlel(x){fg=ge(x);if(fg)fg.remove()}"
    "function f(){x=ge('p');x.type==='password'?x.type='text':x.type='password'}"
    "function ih(){if(!his){his=true;window.addEventListener('pageshow',(e)=>{if(e.persisted){shsp(0);ra.forEach(rep);ra=[]}})}}"
    "function rep(i){i.disabled=false;if(i.BU){i.innerHTML=i.BU}}"
    "function shsp(x){aa=ge('spi');if(aa){if(x){ih();aa.classList.add('sp')}else{aa.classList.remove('sp')}}return true}"
    "function hi(x){ra.push(x);x.disabled=true}"
    "function dbpw(n){ih();aa=ge(n);if(aa){hi(aa);aa.BU=aa.innerHTML;aa.innerHTML=\"Please wait\"}shsp(1);return true}"
    "function sspr(n){ih();aa=ge(n);if(aa){hi(aa)}return shsp(1)}";

static const char HTTP_SCRIPT_QI[]  PROGMEM =
    "function c(l){ge('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;ge('b').value=l.getAttribute('title')||'';"
    "var p=l.nextElementSibling.classList.contains('l');var pp=ge('p');"
    "pp.disabled=!p;pp.placeholder='';var x=ge('fg');if(x){x.style.display='none'}if(p){pp.focus()}else{pp.value=''}return false}"
    "function d(l){return false}";

static const char HTTP_SCRIPT_UPL[] PROGMEM =
    "function uplsub(e1,e2){dbpw(e1);aa=ge(e2);if(aa){hi(aa)}}"
    "function uplchg(e1,e2){ge(e2).style.display=e1.value==''?'none':'initial'}";

static const char HTTP_STYLE[]      PROGMEM =
    "</script><style>"
    ".c,body,button{text-align:center;font-family:-apple-system,system-ui,'Segoe UI',Roboto,'Helvetica Neue',Verdana,Helvetica}"
    "body{background:#f4f4f4}"
    "div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
    "input,button,select,.msg{border-radius:.3rem;width:100%}"
    "input[type=checkbox],input[type=radio]{display:inline-block;margin-top:10px;margin-right:5px;width:auto;}"
    "button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background:" HTTP_BLUE ";color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
    "#wrap{text-align:left;display:inline-block;min-width:260px;max-width:500px}"
    ".h{display:none}"
    // links
    "a{color:#000;font-weight:bold;text-decoration:underline}"
    "a:hover{color:" HTTP_BLUE "}"
    // msg boxes (overruled on some pages)
    ".msg{padding:20px;margin:20px 0;border-top:1px solid #000;border-bottom:1px solid #000;border-radius:0;text-align:center;}"
    ".msg.P{}"
    ".msg.D{border-color:" HTTP_RED ";border-width:3px}"
    ".msg.S{}"
    // buttons
    "button{transition:opacity 0s 250ms;margin-top:10px;margin-bottom:10px;font-variant-caps:small-caps;border-bottom:0.2em solid " HTTP_BLUE "}"
    "button.D{background-color:" HTTP_RED ";border-color:" HTTP_RED "}"
    "button:active{opacity:50% !important;transition-delay:0s;cursor:wait}"
    ":disabled{opacity:0.5}"
    "@media screen and (any-pointer:coarse){abbr{display:none}}"
    "@keyframes spnr{0%,100%{animation-timing-function:cubic-bezier(0.25,0,1,0.75)}0%{transform:rotateY(0deg)}50%{transform:rotateY(1800deg);animation-timing-function:cubic-bezier(0,0.25,0.75,1)}100%{transform:rotateY(3600deg)}}"
    "img.sp{animation:spnr 15s 1.5s cubic-bezier(0,0.2,0.8,1)infinite}";  // animation-delay + page-load does not work on Safari.

#ifdef WM_PARAM3
static const char HTTP_STYLE_C80 [] PROGMEM =
    "button.b::first-letter{font-size:180%;vertical-align:-.12em;color:#6cb4e7;text-shadow:2px 2px #000;font-style:italic;font-weight:bold;margin-right:0.09em}"
    "button.g::first-letter{color:#66ae6e;background-color:#000;font-weight:bold;padding: 0px 4px 0 4px}"
    "button.p::first-letter{font-size:200%;vertical-align:-.12em;color:#be5c9c;text-shadow:2px 2px #111;font-family:Courier}"
    "button.y::first-letter{color:" HTTP_BLUE ";background-color:#ebe74c;font-weight:bold;padding:5px 2px 5px 4px;border-radius:20px 20px 0px}"
    #if defined(WM_PARAM2) && defined(WM_DYNPARM3)
    "button.x::first-letter{color:#ebe74c}"
    #endif
    "hr.z{background-image:linear-gradient(40deg,rgba(13,13,13,0),rgba(13,13,13,0)33.33%,#0d0d0d 33.33%,#0d0d0d 66.67%,rgba(13,13,13,0)66.67%,rgba(13,13,13,0)100%);background-size:8px 100%;height:10px;border:none;width:90%}";
#else
static const char HTTP_STYLE_C80[]  PROGMEM =
    "button.b::first-letter{font-size:180%;vertical-align:-.12em;color:#6cb4e7;text-shadow:2px 2px #000;font-style:italic;font-weight:bold;margin-right:0.09em}"
    "button.g::first-letter{color:#66ae6e;background-color:#000;font-weight:bold;padding: 0px 4px 0 4px}"
    "button.p::first-letter{font-size:200%;vertical-align:-.12em;color:#ebe74c;text-shadow:2px 2px #111;font-family:Courier}"
    "hr.z{background-image:linear-gradient(40deg,rgba(13,13,13,0),rgba(13,13,13,0)33.33%,#0d0d0d 33.33%,#0d0d0d 66.67%,rgba(13,13,13,0)66.67%,rgba(13,13,13,0)100%);background-size:8px 100%;height:10px;border:none;width:90%}";
#endif

static const char HTTP_STYLE_UPLN[] PROGMEM =
    "span.bf{display:none}";

static const char HTTP_STYLE_UPLF[] PROGMEM =
    "@keyframes bf{0%{opacity:0}12%{opacity:1}25%{opacity:0}36%{opacity:1}100%{opacity:0}}"
    //"@keyframes bf{0%{transform:rotate(0deg)}12%{transform:rotate(-30deg)}25%{transform:rotate(30deg)}36%{transform:rotate(-30deg)}50%{transform:rotate(0deg)}100%{transform:rotate(0deg)}}"
    "span.bf{color:#ebe74c;animation:bf 1s ease-in-out infinite}";

static const char HTTP_STYLE_STA[]  PROGMEM =
    ".sta{font-variant:small-caps;color:#888;text-align:center;line-height:1.1em}"
    ".sta>span.n{color:#888}"
    ".sta>span.g{color:#609b71}"
    ".sta>span.r{color:" HTTP_RED "}"
    ".sta>span.o{color:#fa0}";

static const char HTTP_STYLE_MSG[]  PROGMEM =
    ".msg.S{padding:60px 20px 20px 20px;margin:20px 0;border:1px inset #000;border-bottom:1px solid #000;border-radius:0;text-align:center;box-shadow:2px 2px 5px 4px inset rgba(0,0,0,0.1);"
    "background: no-repeat url('data:image/png;base64,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') 5px 5px #fff"
    "}";

// quality icons
static const char HTTP_STYLE_QI[]   PROGMEM =
    "button.s{width:initial;line-height:1.3em;margin:0}"
    ".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}"
    ".q.q-0::after{background-position-x:0}"
    ".q.q-1::after{background-position-x:0}"
    ".q.q-2::after{background-position-x:-21px}"
    ".q.q-3::after{background-position-x:-42px}"
    ".q.q-4::after{background-position-x:-63px}"
    ".q.l::before{background-position-x:-84px}"
    ".ql .q{float:left}"
    ".q::after,.q::before{content:'';width:21px;height:16px;display:inline-block;"
    "background-repeat:no-repeat;background-position:21px 0;"
    "background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMQAAAAgCAMAAAB6rSfNAAABJlBMVEUAAAAAAAAAAAAAAADHyckAAADHyckAAADHycnHyckAAADHyckAAADHycnHyckAAAAAAAAAAAAAAADHyckAAADHyckAAAAAAAAAAADHycnHyckAAADHyckAAADHycnHycnHyckAAAAAAADHyckAAAAAAAAAAAAAAADHyckAAAAAAAAAAAAAAADHycnHyckAAADHyckAAADHyckAAADHyckAAADHyckAAADHyckAAAAAAADHyckAAADHycnHycnHycnHyckAAADHycnHycnHycnHyckAAADHycnHycnHycnHycnHycnHyckAAADHycnHycnHycnHyckAAADHyckAAAAAAAAAAADHyckAAADHyckAAAAAAAAAAADHycmXmZmQkpIAAADHycms9KC6AAAAYHRSTlMA3u8J/LxKI+rfBHFFJg359Oa1sHFXOTInHdvTy5eWd1BIE4J3Uywd1rGhTT/3ko+JWDYvF+rSyq2siW1nYUAK49m3tKNoX0Y7Lyry7sysppwhDwalkoJdxnx7beK+jWod6B+WAAADSklEQVRYw9WV21raQBCANyAECFTOQSByEEREUVREEEVEOSlqFbGe2tn3f4nGsJQsSQhNP/jof7PJzn8zu7MziOKpfZr3Re02mz3qW+fOa1Rw7qY5+P6cLTHFIlPKCv4dMzJALXB/gyl2716+Lco0F/puoHCXm4/o7/i+votVWHlwLcJkhVVQYVUIodk5useanKzN28z0QZNyBs3IxviUtnzrG7HYxrrveAWP2J+vORiffCQrDFKpgZCNjG+mMlslhfGQ26u4rF6/WR9sJHDhmp/J1mGIaTsnewPmxHaRBD5mqKkAHnIcV8asURI8m5dZgCGRtNJM9EiwiXR4xRL2c/XwwRa5/vmYbyDB7KibwbpGSbEVi0XgESGPJU5lV+7qHHRlJXA2NK7+xXwM8XwoqTB/PoPEu7bZHBp9RFFlpAKskt9rLBIeNYs1rhHelR6j/TI22jySDi42s8nRZsbvZValZ8t4UxnK/FWVriEzzUxGQCRF5wAi8m0rxpeT1TpZ0ScYBwyauR5QRNKUmQDw6pllgAJdS2JU4NmKuPCju2gM13YYK7ggVX0XM2YGGVDw4ZSbVa++WU4higpAmawCHbnSGEtPYsyo2dcYYA6DJkG8MlaqNABGvr9mJ4PpkrN2XK5Oi/ORwWQT+7kxMzM6XG8qcciymwl/lowwT8iYSRDtJPkwyXs3mUzckaz3nJK5FECGzASZYamkrPe8e0AibcCUJWFSSyJO+vYEG2QoGTJz6h1+QMaXAVPnJtDBCr45UOpd8Yhb4mrIDAK4g0pzswjgNGISeMtXW5YQP3pywWrvSutTfL8RjTby8drwBYRfiKAwHbk9b6nk3UubtcwcE1IzM/WmIZNgAQqT0uBuMeHmFU3FbwKC+82xOFM/CZedav0dpAlLtXXP4aJM/SQCeIJrpEEBJqguyNRPooW/COc5Lr+lbJnKplh/9vv3ItJnYVGmfjl1d7GNtJj2BcZtpMmmG4pO0lo8AMHFmfoPu/X5Z4bVPgNoCk5LEhHMlvQCTWUS/yOzJeHYcZon95bILANFT1ViPWJ6k/1tiUyeoS6CV5V+gEgE0SyhOY1NkNhZdnMqIZDgl92czjaIZJffnIpDcK9um5ffHPMbdfGaiuWnXuoAAAAASUVORK5CYII=');"
    "background-size:98px 16px}";

static const char HTTP_STYLE_SET[]  PROGMEM =
    "label{display:inline-block;vertical-align:text-top;margin:0 10px 0 0;padding:0 10px 0 0;white-space:normal}"
    ".hl{margin:0 0 7px 0;padding:0}"
    ".ss{background-color:#e4e4e4;border-radius:7px;margin-bottom:20px;padding:7px 10px 7px 10px;white-space:nowrap}"
    "label.mp0{margin:0;padding:0}";

static const char HTTP_STYLE_UPL[]  PROGMEM =
    "input[type='file']{background:#fff;border:1px solid " HTTP_BLUE "}"
    "div.ba{font-size:80%;border-radius:5px;margin:0}"
    "div.bar{background:#dc3630;color:#fff}";

static const char HTTP_STYLE_END [] PROGMEM = "</style>";

static const char HTTP_HEAD_END[]   PROGMEM = "</head><body><div id='wrap'>";

static const char HTTP_ROOT_MAIN[]  PROGMEM = "<h1 id='h1'>{t}</h1><h3 id='h3'>{v}</h3>";

static const char * const HTTP_PORTAL_MENU[] PROGMEM =
{
    "<form action='/wifi' method='get' onsubmit='return sspr(\"wbut\")'><button class='b' id='wbut'>WiFi Configuration</button></form>\n",
    "<form action='/param' method='get' onsubmit='return shsp(1)'><button class='g'>Settings</button></form>\n",
    #ifdef WM_PARAM2
    "<form action='/param2' method='get' onsubmit='return shsp(1)'><button class='p'>" WM_PARAM2_CAPTION "</button></form>\n",
    #else
    "",
    #endif
    #ifdef WM_PARAM3
    "<form action='/param3' method='get' onsubmit='return shsp(1)'><button class='y'>" WM_PARAM3_CAPTION "</button></form>\n",
    #else
    "",
    #endif
    #ifdef WM_UPLOAD
    "<form action='/update' method='get' onsubmit='return shsp(1)'><button class='D b'>Update & Upload <span class='bf'>&#x260e;&#xfe0e;</span></button></form>\n",
    #else
    "<form action='/update' method='get' onsubmit='return shsp(1)'><button class='D b'>Update <span class='bf'>&#x260e;&#xfe0e;</span></button></form>\n",
    #endif
    "<hr>",           // sep
    "<hr class='z'>", // sep fancy
    "",               // custom, if _customMenuHTML is NULL
    #if defined(WM_PARAM2) && defined(WM_DYNPARM3)
    "<form action='/param2' method='get' onsubmit='return shsp(1)'><button class='p x'>" WM_PARAM2_CAPTION "</button></form>\n"
    #endif
};

static const char HTTP_DIV_END[]          PROGMEM = "</div>";

static const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
static const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
static const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr>";
static const char HTTP_FORM_PARAM[]       PROGMEM = "<input id='{i}' name='{n}' {l} value='{v}' {c} {f}>";
static const char HTTP_FORM_END[]         PROGMEM = "<button type='submit'>Save</button></form>";

static const char HTTP_FORM_WIFI[]        PROGMEM = "<div class='ss'><div class='hl'>WiFi connection: Network selection</div><label for='s'>Network name (SSID)</label><br><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{V}' oninput='var x=ge(\"fg\");var y=ge(\"p\");y.disabled=false;if(!this.value.length&&this.placeholder.length){if(x&&!y.value.length){x.style.display=\"\"}y.placeholder=y.getAttribute(\"data-ph\")||\"********\";}else{if(x){x.style.display=\"none\"}y.placeholder=\"\"}'><br><label for='p'>Password</label><br><input id='p' name='p' maxlength='64' type='password' placeholder='{p}' data-ph='{p}' oninput='var x=ge(\"fg\");if(x){var y=ge(\"s\");if(!y.value.length&&y.placeholder.length){if(this.value.length){x.style.display=\"none\"}else{x.style.display=\"\"}}}'><br><label><input type='checkbox' onclick='f()' style='margin:0px 5px 10px 0px'>Show password when typing</label><br><label for='s'>BSSID (Access Point MAC)<br><span>Leave this empty unless you have multiple APs with the same SSID and want to connect to a specific AP: Click 'Scan for Networks', 'Show All' and select AP. Format: <i>XX:XX:XX:XX:XX:XX</i></span></label><br><input id='b' name='b' maxlength='17' autocorrect='off' autocomplete='off' value='{h}' pattern='^([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$'><br>";
#define HTTP_FORM_WIFI_END                HTTP_DIV_END
static const char HTTP_WIFI_ITEM[]        PROGMEM = "<div><a href='#p' onclick='return {t}(this)' data-ssid='{V}' title='{R}'>{v}</a>{c}<div role='img' aria-label='{r}dBm' title='{r}dBm' class='q q-{q} {i}'></div></div>";
static const char HTTP_FORM_SECT_HEAD[]   PROGMEM = "<div class='ss'><div class='hl'>WiFi connection: Static IP settings</div>";
#define HTTP_FORM_SECT_FOOT               HTTP_DIV_END
static const char HTTP_FORM_WIFI_PH[]     PROGMEM = "placeholder='Leave this section empty for DHCP'";
static const char HTTP_MSG_NONETWORKS[]   PROGMEM = "<div class='msg'>No networks found.</div>";
static const char HTTP_MSG_SCANFAIL[]     PROGMEM = "<div class='msg D'>Scan failed.<br>Click 'Scan for Networks' to retry.</div>";
static const char HTTP_MSG_NOSCAN[]       PROGMEM = "<div class='msg'>Device busy, WiFi scan prohibited. Try again later.</div>";
static const char HTTP_SCAN_LINK[]        PROGMEM = "<form action='/wifi?refresh=1' method='POST' onsubmit='if(confirm(\"This will reload the page, changes are not saved. Proceed?\")){return dbpw(\"wrefr\")}return false;'><button id='wrefr' name='refresh' value='1'>Scan for Networks</button></form>";
static const char HTTP_ERASE_BUTTON[]     PROGMEM = "<div id='fg' class='c' style='border:2px solid " HTTP_RED ";border-radius:7px'><label><input id='fgn' name='fgn' type='checkbox' style='margin-top:0'>Forget saved WiFi network</label></div>";
static const char HTTP_SHOWALL[]          PROGMEM = "<div class='c'><button class='s' id='sab' form='saf' type='submit'>Show all</button></div>";
static const char HTTP_SHOWALL_FORM[]     PROGMEM = "<form id='saf' action='/wifi?showall=1' method='POST' onsubmit='return dbpw(\"sab\")'></form>";

static const char HTTP_PARAMSAVED[]       PROGMEM = "<div id='lc' class='msg S'>Settings saved. Rebooting.<br>";
static const char HTTP_SAVED_NORMAL[]     PROGMEM = "Trying to connect to network.<br>In case of error, device boots in AP mode.";
static const char HTTP_SAVED_CARMODE[]    PROGMEM = "<br>Device is run in <strong>car mode</strong> and will <em>not</em><br>connect to WiFi network after reboot.";
static const char HTTP_SAVED_ERASED[]     PROGMEM = "WiFi network credentials deleted.<br>Restarting in AP mode.<br>";
#define HTTP_PARAMSAVED_END               HTTP_DIV_END

static const char HTTP_UPDATE_FORM[]      PROGMEM = "<form method='POST' enctype='multipart/form-data' action=";
static const char HTTP_UPDATE1[]          PROGMEM = "'u' onsubmit=\"uplsub('upbu','uacb');\" onchange=\"uplchg(this,'upbu');\"><div class='ss c' style='line-height:140%'>Firmware update<br>";
static const char HTTP_UPLOAD_LINK0[]     PROGMEM = "<div class='c ba";
#ifdef WM_FW_HW_VER
static const char HTTP_UPLOAD_V_REQ[]     PROGMEM = "'>Required version: ";
#endif
static const char HTTP_UPLOAD_LINKX[]     PROGMEM = "'>Firmware is up-to-date. ";
static const char HTTP_UPLOAD_LINKY[]     PROGMEM = " bar'>";
static const char HTTP_UPLOAD_LINKZ[]     PROGMEM = "'>Unable to check for update. ";
static const char HTTP_UPLOAD_LINK1[]     PROGMEM = "<a href='https://";
static const char HTTP_UPLOAD_LINK2[]     PROGMEM = "' style='color:#fff' target=_blank>Update (";
static const char HTTP_UPLOAD_LINK3[]     PROGMEM = ") available</a></div>";
static const char HTTP_UPLOAD_LINKA[]     PROGMEM = "' target=_blank>All releases</a></div>";
static const char HTTP_UPDATE2[]          PROGMEM = "<input type='file' name='update' accept='.bin,application/octet-stream'><br><button id='upbu' type='submit' class='h D'>Update</button></div></form>";
#ifdef WM_UPLOAD
static const char HTTP_UPLOADSND1[]       PROGMEM = "'uac' onsubmit=\"uplsub('uacb','upbu');\" onchange=\"uplchg(this,'uacb');\">";
static const char HTTP_UPLOADSND1A[]      PROGMEM = "<div class='ss c' style='line-height:140%'>Sound pack (";
static const char HTTP_UPLOADSND2[]       PROGMEM = ".bin) and/or mp3 file upload<br>";
static const char HTTP_UPLOAD_SLINK1[]    PROGMEM = "<div class='c ba";
static const char HTTP_UPLOAD_SLINK1A[]   PROGMEM = " bar";
static const char HTTP_UPLOAD_SLINK1B[]   PROGMEM = "'>Required sound-pack: ";
static const char HTTP_UPLOAD_SLINK2[]    PROGMEM = " [";
static const char HTTP_UPLOAD_SLINK2A[]   PROGMEM = "<strong>not</strong> ";
static const char HTTP_UPLOAD_SLINK3[]    PROGMEM = "installed]</div>";
static const char HTTP_UPLOADSND3[]       PROGMEM = "<input type='file' name='upac' multiple accept='.bin,application/octet-stream,.mp3,audio/mpeg'><br><button id='uacb' type='submit' class='h'>Upload</button></div></form>";
static const char HTTP_UPLOAD_SDMSG[]     PROGMEM = "<br>SD card required for sound upload</div>";
#endif

static const char HTTP_UPDATE_FAIL1[]     PROGMEM = "<div class='msg D'><strong>Upload failed.</strong><br>";
#define HTTP_UPDATE_FAIL2                 HTTP_DIV_END
static const char HTTP_UPDATE_SUCCESS[]   PROGMEM = "<div id='lc' class='msg S'><strong>Upload complete.</strong><br>Device rebooting.</div>";

static const char HTTP_STATUS_HEAD[]      PROGMEM = "<div class='sta'><span class='{c}'>&#x25CF;</span> ";
#define HTTP_STATUS_TAIL                  HTTP_DIV_END
static const char HTTP_STATUS_ON[]        PROGMEM = "{v}{I}{V}<br>{i}";
static const char HTTP_STATUS_BADBSSID[]  PROGMEM = "<br>AP with given BSSID not found";
static const char HTTP_STATUS_OFF[]       PROGMEM = "{v}<br>{r}Operating in {V}mode";
static const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "Network not found<br>";             // WL_NO_SSID_AVAIL
static const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "Failed to connect<br>";             // WL_CONNECT_FAILED
static const char HTTP_STATUS_DISCONN[]   PROGMEM = "Disconnected. Wrong Password?<br>"; // WL_DISCONNECTED
static const char HTTP_STATUS_APMODE[]    PROGMEM = "AP-";
static const char HTTP_STATUS_CARMODE[]   PROGMEM = "car ";
static const char HTTP_STATUS_NONE[]      PROGMEM = "No WiFi connection configured";
static const char HTTP_STATUS_NODHCP[]    PROGMEM = "DHCP timeout<br>";

static const char HTTP_BR[]               PROGMEM = "<br>";
static const char HTTP_END[]              PROGMEM = "</div></body></html>";
static const char HTML_CHKBOX[]           PROGMEM = "type='checkbox' autocomplete='off'";  // ac=off to fix FF idiocy
static const char HTTP_SECT_HEAD[]        PROGMEM = "<div class='ss'>";
#define HTTP_SECT_START                   HTTP_DIV_END        // + HTTP_SECT_HEAD
#define HTTP_SECT_FOOT                    HTTP_DIV_END
static const char HTTP_HL_S[]             PROGMEM = "<div class='hl'>";
#define HTTP_HL_E                         HTTP_DIV_END

static const char HTTP_BSSID_FOOT[]       PROGMEM = " <abbr title='BSSID %s'>&#8505;</abbr>";


static const char A_paramsave[]    PROGMEM = "paramsave";
#ifdef WM_PARAM2
static const char A_param2save[]   PROGMEM = "param2save";
#ifdef WM_PARAM3
static const char A_param3save[]   PROGMEM = "param3save";
#endif
#endif
static const char A_wifisave[]     PROGMEM = "wifisave";

static const char S_titlewifi[]    PROGMEM = "WiFi Configuration";

static const char * const S_titleparam[3] PROGMEM = {
    "Settings",
    WM_PARAM2_TITLE,
    WM_PARAM3_TITLE
};

static const char S_titleupd[]     PROGMEM = "Firmware Upload";

static const char S_passph[]       PROGMEM = "********";
static const char S_staticip[]     PROGMEM = "Static IP";
static const char S_staticgw[]     PROGMEM = "Static gateway";
static const char S_staticdns[]    PROGMEM = "Static DNS";
static const char S_subnet[]       PROGMEM = "Subnet mask";

static const char S_brand[]        PROGMEM = "WiFiManager";

static const char S_GET[]          PROGMEM = "GET";
static const char S_POST[]         PROGMEM = "POST";
static const char S_NA[]           PROGMEM = "Unknown";

static const char S_nonprintable[] PROGMEM = "[Non-printable SSID]";

static const char S_notfound[]     PROGMEM = "404 File not found\n\n\n";

// Routes
static const char R_root[]         PROGMEM = "/";
static const char R_wifi[]         PROGMEM = "/wifi";
static const char R_wifisave[]     PROGMEM = "/wifisave";
static const char R_param[]        PROGMEM = "/param";
static const char R_paramsave[]    PROGMEM = "/paramsave";
#ifdef WM_PARAM2
static const char R_param2[]       PROGMEM = "/param2";
static const char R_param2save[]   PROGMEM = "/param2save";
#ifdef WM_PARAM3
static const char R_param3[]       PROGMEM = "/param3";
static const char R_param3save[]   PROGMEM = "/param3save";
#endif
#endif
static const char R_update[]       PROGMEM = "/update";
static const char R_updatedone[]   PROGMEM = "/u";

// Strings
static const char S_ip[]           PROGMEM = WMS_ip;
static const char S_gw[]           PROGMEM = WMS_gw;
static const char S_sn[]           PROGMEM = WMS_sn;
static const char S_dns[]          PROGMEM = WMS_dns;

// Tokens
static const char T_v[]            PROGMEM = "{v}"; // @token v
static const char T_V[]            PROGMEM = "{V}"; // @token v
static const char T_I[]            PROGMEM = "{I}"; // @token I
static const char T_i[]            PROGMEM = "{i}"; // @token i
static const char T_n[]            PROGMEM = "{n}"; // @token n
static const char T_p[]            PROGMEM = "{p}"; // @token p
static const char T_t[]            PROGMEM = "{t}"; // @token t
static const char T_l[]            PROGMEM = "{l}"; // @token l
static const char T_c[]            PROGMEM = "{c}"; // @token c
static const char T_e[]            PROGMEM = "{e}"; // @token e
static const char T_q[]            PROGMEM = "{q}"; // @token q
static const char T_r[]            PROGMEM = "{r}"; // @token r
static const char T_R[]            PROGMEM = "{R}"; // @token R
static const char T_h[]            PROGMEM = "{h}"; // @token h
static const char T_f[]            PROGMEM = "{f}"; // @token f

// http
static const char HTTP_HEAD_CT[]   PROGMEM = "text/html";
static const char HTTP_HEAD_CT2[]  PROGMEM = "text/plain";

// Debug
#ifdef _A10001986_DBG
static const char * const WIFI_STA_STATUS[] PROGMEM =
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
const char * const WIFI_MODES[] PROGMEM = { "NULL", "STA", "AP", "STA+AP" };
#endif

#endif  // _WM_STRINGS_EN_H_
