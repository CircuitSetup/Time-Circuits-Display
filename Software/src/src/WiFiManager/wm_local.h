/**
 * WiFiManager.h
 *
 * Based on:
 * WiFiManager, a library for the ESP32/Arduino platform
 * Creator tzapu (tablatronix)
 * Version 2.0.15
 * License MIT
 *
 * Adapted by Thomas Winischhofer (A10001986)
 */

#ifndef wm_local_h
#define wm_local_h

//#define _A10001986_DBG
//#define _A10001986_V_DBG

#define WM_MDNS
#define WM_PARAM2
// #define WM_AP_STATIC_IP
// #define WM_APCALLBACK
// #define WM_PRECONNECTCB
// #define WM_PRESAVECB
// #define WM_EVENTCB
// #define WM_ADDLGETTERS
// #define WM_ADDLSETTERS

#ifdef WM_PARAM2
#define WM_PARAM2_CAPTION "HA/MQTT Settings"
#define WM_PARAM2_TITLE "HA/MQTT Settings"
#endif

#endif