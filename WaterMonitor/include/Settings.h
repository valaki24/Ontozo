#pragma once

#include <Arduino.h>
#include <IPAddress.h>

/*
 * =========================================================
 * Board configuration
 * =========================================================
 *
 * WaterMonitor ESP32 - egyedi panel
 *
 * Ha később lesz fizikai konfigurációs gombod vagy LED-ed,
 * itt lehet megadni a GPIO-kat.
 */

/*
 * Jelenleg nincs Edgent reset/configuration gomb használva.
 *
 * A konfiguráció a Blynk AP weboldalán történik.
 *
 * #define BOARD_BUTTON_PIN ...
 */


/*
 * LED beállítás
 *
 * Ha nincs használatban LED, ez csak konfigurációs adat.
 */
#define BOARD_LED_INVERSE       false
#define BOARD_LED_BRIGHTNESS    64


/*
 * =========================================================
 * Blynk / configuration
 * =========================================================
 */

#ifndef CONFIG_DEVICE_PREFIX
#define CONFIG_DEVICE_PREFIX    "Blynk"
#endif

#ifndef CONFIG_AP_URL
#define CONFIG_AP_URL            "blynk.setup"
#endif

#ifndef CONFIG_DEFAULT_SERVER
#define CONFIG_DEFAULT_SERVER    "blynk.cloud"
#endif

#ifndef CONFIG_DEFAULT_PORT
#define CONFIG_DEFAULT_PORT      443
#endif


/*
 * =========================================================
 * WiFi / Blynk
 * =========================================================
 *
 * Ezek már NEM hosszú blokkoló várakozások.
 * A tényleges csatlakozás a loopban történik.
 */

#define WIFI_CLOUD_MAX_RETRIES       5

#define WIFI_NET_CONNECT_TIMEOUT     15000
#define WIFI_CLOUD_CONNECT_TIMEOUT   5000


/*
 * =========================================================
 * Configuration AP
 * =========================================================
 */

#define WIFI_AP_IP                   IPAddress(192, 168, 4, 1)
#define WIFI_AP_Subnet               IPAddress(255, 255, 255, 0)


/*
 * =========================================================
 * Firmware
 * =========================================================
 *
 * A main.cpp-ben lévő BLYNK_FIRMWARE_VERSION érték
 * használható.
 */

#ifndef BLYNK_FIRMWARE_VERSION
#define BLYNK_FIRMWARE_VERSION       "1.3.6"
#endif


/*
 * =========================================================
 * Blynk
 * =========================================================
 */

#define BLYNK_NO_DEFAULT_BANNER


/*
 * =========================================================
 * Debug
 * =========================================================
 */

#if defined(APP_DEBUG)

    #define DEBUG_PRINT(...)   BLYNK_LOG1(__VA_ARGS__)
    #define DEBUG_PRINTF(...)  BLYNK_LOG(__VA_ARGS__)

#else

    #define DEBUG_PRINT(...)
    #define DEBUG_PRINTF(...)

#endif
