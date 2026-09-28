#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <BlynkSimpleEsp32_SSL.h>

#include "Settings.h"
#include "ConfigStore.h"


/*
 * =========================================================
 * Blynk state machine
 * =========================================================
 */

enum State
{
    MODE_WAIT_CONFIG,
    MODE_CONFIGURING,
    MODE_CONNECTING_NET,
    MODE_CONNECTING_CLOUD,
    MODE_RUNNING,
    MODE_OTA_UPGRADE,
    MODE_SWITCH_TO_STA,
    MODE_RESET_CONFIG,
    MODE_ERROR,
    MODE_MAX_VALUE
};


namespace BlynkState
{
    volatile State state =
        MODE_MAX_VALUE;


    State get()
    {
        return state;
    }


    bool is(State m)
    {
        return state == m;
    }


    void set(State m)
    {
        if (
            state != m &&
            m < MODE_MAX_VALUE
        )
        {
#ifdef APP_DEBUG

            static const char* StateStr[] =
            {
                "WAIT_CONFIG",
                "CONFIGURING",
                "CONNECTING_NET",
                "CONNECTING_CLOUD",
                "RUNNING",
                "OTA_UPGRADE",
                "SWITCH_TO_STA",
                "RESET_CONFIG",
                "ERROR"
            };


            if (
                state < MODE_MAX_VALUE
            )
            {
                DEBUG_PRINT(
                    String(StateStr[state]) +
                    " => " +
                    StateStr[m]
                );
            }

#endif

            state = m;
        }
    }
}


/*
 * =========================================================
 * Timer
 * =========================================================
 */

BlynkTimer edgentTimer;


/*
 * =========================================================
 * Configuration server
 * =========================================================
 */

static WebServer configServer(80);

static DNSServer dnsServer;

static const byte DNS_PORT = 53;


/*
 * =========================================================
 * Connection state
 * =========================================================
 */

static uint32_t wifiConnectStarted = 0;

static uint32_t blynkConnectStarted = 0;

static uint32_t nextWiFiAttempt = 0;

static uint32_t nextBlynkAttempt = 0;

static uint8_t wifiRetries = 0;

static uint8_t blynkRetries = 0;


/*
 * =========================================================
 * Restart
 * =========================================================
 */

void restartMCU()
{
    delay(100);

    ESP.restart();


    while (true)
    {
        delay(100);
    }
}

/*
 * =========================================================
 * OTA forward declaration
 * =========================================================
 *
 * Az OTA.h tényleges implementációja a fájl végén
 * kerül beillesztésre, de az Edgent::run() már
 * előbb használja az enterOTA() függvényt.
 */

void enterOTA();

/*
 * =========================================================
 * Unique device name
 * =========================================================
 */

static String encodeUniquePart(
    uint32_t n,
    unsigned len
)
{
    static constexpr char alphabet[] =
        "0W8N4Y1HP5DF9K6JM3C2UA7R";

    static constexpr int base =
        sizeof(alphabet) - 1;


    char buf[16] = {0};

    char prev = 0;


    for (
        unsigned i = 0;
        i < len;
        n /= base
    )
    {
        char c =
            alphabet[n % base];


        if (
            c == prev
        )
        {
            c =
                alphabet[(n + 1) % base];
        }


        prev =
            buf[i++] = c;
    }


    return String(buf);
}


static String getWiFiName(
    bool withPrefix = true
)
{
    uint8_t mac[6] = {0};

    WiFi.macAddress(mac);


    uint32_t unique = 0;


    for (
        int i = 0;
        i < 6;
        i++
    )
    {
        unique =
            (unique * 33) ^
            mac[i];
    }


    String devUnique =
        encodeUniquePart(
            unique,
            4
        );


    String devPrefix =
        CONFIG_DEVICE_PREFIX;


    String devName =
        String(BLYNK_TEMPLATE_NAME);


    int maxNameLength =
        31 -
        6 -
        devPrefix.length();


    if (
        maxNameLength > 0 &&
        devName.length() >
        (unsigned)maxNameLength
    )
    {
        devName =
            devName.substring(
                0,
                maxNameLength
            );
    }


    if (withPrefix)
    {
        return
            devPrefix +
            " " +
            devName +
            "-" +
            devUnique;
    }


    return
        devName +
        "-" +
        devUnique;
}


/*
 * =========================================================
 * WiFi information
 * =========================================================
 */

static String getWiFiMacAddress()
{
    return WiFi.macAddress();
}


static String getWiFiApBSSID()
{
    return WiFi.softAPmacAddress();
}


static String getWiFiNetworkSSID()
{
    return WiFi.SSID();
}


static String getWiFiNetworkBSSID()
{
    return WiFi.BSSIDstr();
}


/*
 * =========================================================
 * Configuration HTML
 * =========================================================
 */

static const char config_form[] PROGMEM = R"html(
<!DOCTYPE HTML>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">

<title>WaterMonitor WiFi setup</title>

<style>

body {
    background:#f5f5f5;
    font-family:Arial,sans-serif;
    font-size:16px;
    margin:0;
}

.centered {
    position:absolute;
    top:50%;
    left:50%;
    transform:translate(-50%,-50%);
    padding:25px;
    background:#ddd;
    border-radius:8px;
    box-shadow:0 2px 10px rgba(0,0,0,.2);
}

input {
    width:280px;
    max-width:90%;
    padding:9px;
    margin:4px 0 12px;
    box-sizing:border-box;
}

input[type=submit] {
    width:150px;
    cursor:pointer;
}

h2 {
    margin-top:0;
}

</style>

</head>

<body>

<div class="centered">

<h2>WaterMonitor WiFi setup</h2>

<form method="get" action="/config">

<label>WiFi SSID</label><br>

<input
    type="text"
    name="ssid"
    required
><br>


<label>WiFi password</label><br>

<input
    type="password"
    name="pass"
><br>


<label>Blynk token</label><br>

<input
    type="text"
    name="blynk"
    required
    maxlength="32"
><br>


<label>Blynk host</label><br>

<input
    type="text"
    name="host"
    value="blynk.cloud"
><br>


<label>Blynk port</label><br>

<input
    type="number"
    name="port"
    value="443"
><br><br>


<input
    type="submit"
    value="Save"
>

</form>

</div>

</body>
</html>
)html";


/*
 * =========================================================
 * Stop configuration server
 * =========================================================
 */

static void stopConfigServer()
{
    configServer.stop();

    dnsServer.stop();

    WiFi.softAPdisconnect(true);
}


/*
 * =========================================================
 * Enter configuration mode
 * =========================================================
 */

static void enterConfigMode()
{
    DEBUG_PRINT(
        "Entering WiFi configuration mode."
    );


    BlynkState::set(
        MODE_CONFIGURING
    );


    /*
     * AP mód.
     */

    WiFi.mode(
        WIFI_AP
    );


    String apName =
        getWiFiName();


    WiFi.softAPConfig(
        WIFI_AP_IP,
        WIFI_AP_IP,
        WIFI_AP_Subnet
    );


    WiFi.softAP(
        apName.c_str()
    );


    delay(300);


    IPAddress apIP =
        WiFi.softAPIP();


    DEBUG_PRINT(
        String("Configuration AP: ") +
        apName
    );


    DEBUG_PRINT(
        String("AP IP: ") +
        apIP.toString()
    );


    /*
     * DNS captive portal.
     */

    dnsServer.start(
        DNS_PORT,
        "*",
        apIP
    );


    /*
     * Főoldal.
     */

    configServer.on(
        "/",
        HTTP_GET,
        []()
        {
            configServer.send(
                200,
                "text/html",
                config_form
            );
        }
    );


    /*
     * Config mentés.
     */

    configServer.on(
        "/config",
        HTTP_GET,
        []()
        {
            String ssid =
                configServer.arg("ssid");

            String pass =
                configServer.arg("pass");

            String token =
                configServer.arg("blynk");

            String host =
                configServer.arg("host");

            String port =
                configServer.arg("port");


            if (
                ssid.length() == 0 ||
                token.length() == 0
            )
            {
                configServer.send(
                    400,
                    "text/plain",
                    "Invalid configuration"
                );

                return;
            }


            DEBUG_PRINT(
                String("New WiFi SSID: ") +
                ssid
            );


            /*
             * Új konfiguráció.
             */

            configStore =
                configDefault;


            CopyString(
                ssid,
                configStore.wifiSSID
            );


            CopyString(
                pass,
                configStore.wifiPass
            );


            CopyString(
                token,
                configStore.cloudToken
            );


            if (
                host.length() > 0
            )
            {
                CopyString(
                    host,
                    configStore.cloudHost
                );
            }


            int selectedPort =
                port.toInt();


            if (
                selectedPort > 0 &&
                selectedPort <= 65535
            )
            {
                configStore.cloudPort =
                    selectedPort;
            }


            configStore.setFlag(
                CONFIG_FLAG_VALID,
                true
            );


            /*
             * Statikus IP-t itt nem állítunk.
             * Később bővíthető.
             */

            configStore.setFlag(
                CONFIG_FLAG_STATIC_IP,
                false
            );


            if (
                !config_save()
            )
            {
                configServer.send(
                    500,
                    "text/plain",
                    "EEPROM save failed"
                );

                return;
            }


            configServer.send(
                200,
                "text/html",
                "<html>"
                "<body>"
                "<h2>Configuration saved.</h2>"
                "<p>Rebooting...</p>"
                "</body>"
                "</html>"
            );


            delay(500);

            restartMCU();
        }
    );


    /*
     * Reset.
     */

    configServer.on(
        "/reset",
        HTTP_GET,
        []()
        {
            resetConfigStorage();


            configServer.send(
                200,
                "application/json",
                "{\"status\":\"ok\"}"
            );


            delay(300);

            restartMCU();
        }
    );


    /*
     * Reboot.
     */

    configServer.on(
        "/reboot",
        HTTP_GET,
        []()
        {
            configServer.send(
                200,
                "text/plain",
                "Rebooting"
            );


            delay(300);

            restartMCU();
        }
    );


    configServer.begin();


    DEBUG_PRINT(
        "Configuration server started."
    );


    /*
     * Konfigurációs mód.
     *
     * Csak akkor blokkolunk itt,
     * amikor ténylegesen nincs WiFi konfiguráció.
     */

    while (
        BlynkState::is(
            MODE_CONFIGURING
        )
    )
    {
        dnsServer.processNextRequest();

        configServer.handleClient();

        delay(5);
    }


    stopConfigServer();


    WiFi.mode(
        WIFI_STA
    );
}


/*
 * =========================================================
 * Start WiFi
 * =========================================================
 */

static void startWiFiConnection()
{
    if (
        strlen(
            configStore.wifiSSID
        ) == 0
    )
    {
        BlynkState::set(
            MODE_WAIT_CONFIG
        );

        return;
    }


    DEBUG_PRINT(
        String("Connecting to WiFi: ") +
        configStore.wifiSSID
    );


    WiFi.mode(
        WIFI_STA
    );


    String hostname =
        getWiFiName();


    hostname.replace(
        " ",
        "-"
    );


    WiFi.setHostname(
        hostname.c_str()
    );


    /*
     * Statikus IP.
     */

    if (
        configStore.getFlag(
            CONFIG_FLAG_STATIC_IP
        )
    )
    {
        IPAddress ip(
            configStore.staticIP
        );

        IPAddress mask(
            configStore.staticMask
        );

        IPAddress gw(
            configStore.staticGW
        );

        IPAddress dns(
            configStore.staticDNS
        );

        IPAddress dns2(
            configStore.staticDNS2
        );


        if (
            !WiFi.config(
                ip,
                gw,
                mask,
                dns,
                dns2
            )
        )
        {
            DEBUG_PRINT(
                "Failed to configure Static IP."
            );


            config_set_last_error(
                BLYNK_PROV_ERR_CONFIG
            );


            BlynkState::set(
                MODE_WAIT_CONFIG
            );

            return;
        }
    }


    WiFi.begin(
        configStore.wifiSSID,
        configStore.wifiPass
    );


    wifiConnectStarted =
        millis();


    nextWiFiAttempt =
        millis() + 1000;


    BlynkState::set(
        MODE_CONNECTING_NET
    );
}


/*
 * =========================================================
 * WiFi connection state
 * =========================================================
 */

static void processWiFiConnection()
{
    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        DEBUG_PRINT(
            String("WiFi connected. IP: ") +
            WiFi.localIP().toString()
        );


        wifiRetries = 0;

        blynkRetries = 0;

        blynkConnectStarted =
            0;


        BlynkState::set(
            MODE_CONNECTING_CLOUD
        );

        return;
    }


    if (
        millis() -
        wifiConnectStarted >
        WIFI_NET_CONNECT_TIMEOUT
    )
    {
        wifiRetries++;


        DEBUG_PRINT(
            String("WiFi connection timeout. Retry ") +
            wifiRetries
        );


        WiFi.disconnect();

        if (
            wifiRetries >=
            WIFI_CLOUD_MAX_RETRIES
        )
        {
            wifiRetries = 0;


            config_set_last_error(
                BLYNK_PROV_ERR_NETWORK
            );


            /*
             * Újrapróbálkozás.
             *
             * Nem lépünk végleg ERROR-ba.
             */

            wifiConnectStarted =
                millis();

            WiFi.begin(
                configStore.wifiSSID,
                configStore.wifiPass
            );

            return;
        }


        wifiConnectStarted =
            millis();


        WiFi.begin(
            configStore.wifiSSID,
            configStore.wifiPass
        );
    }
}


/*
 * =========================================================
 * Blynk connection
 * =========================================================
 */

static void startBlynkConnection()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        BlynkState::set(
            MODE_CONNECTING_NET
        );

        return;
    }


    DEBUG_PRINT(
        String("Connecting to Blynk: ") +
        configStore.cloudHost +
        ":" +
        configStore.cloudPort
    );


    Blynk.config(
        configStore.cloudToken,
        configStore.cloudHost,
        configStore.cloudPort
    );


    /*
     * Rövid timeout.
     *
     * Nem használunk hosszú blokkoló
     * Blynk.connect(50000) hívást.
     */

    Blynk.connect(
        1000
    );


    blynkConnectStarted =
        millis();


    nextBlynkAttempt =
        millis() + 3000;


    if (
        Blynk.connected()
    )
    {
        DEBUG_PRINT(
            "Blynk connected."
        );


        blynkRetries = 0;


        configStore.last_error =
            BLYNK_PROV_ERR_NONE;


        configStore.setFlag(
            CONFIG_FLAG_VALID,
            true
        );


        config_save();


        BlynkState::set(
            MODE_RUNNING
        );


        return;
    }


    DEBUG_PRINT(
        "Blynk not connected."
    );


    blynkRetries++;


    if (
        blynkRetries >=
        WIFI_CLOUD_MAX_RETRIES
    )
    {
        blynkRetries = 0;


        config_set_last_error(
            BLYNK_PROV_ERR_CLOUD
        );
    }
}


/*
 * =========================================================
 * Edgent
 * =========================================================
 */

class Edgent
{
public:

    void begin()
    {
        DEBUG_PRINT(
            "Starting ESP32 BlynkEdgent..."
        );


        /*
         * EEPROM.
         */

        if (
            !config_init()
        )
        {
            BlynkState::set(
                MODE_WAIT_CONFIG
            );

            return;
        }


        /*
         * Nincs konfiguráció.
         */

        if (
            !configStore.getFlag(
                CONFIG_FLAG_VALID
            ) ||
            strlen(
                configStore.wifiSSID
            ) == 0 ||
            strlen(
                configStore.cloudToken
            ) == 0
        )
        {
            BlynkState::set(
                MODE_WAIT_CONFIG
            );

            return;
        }


        /*
         * Van konfiguráció.
         */

        wifiRetries = 0;

        blynkRetries = 0;


        BlynkState::set(
            MODE_CONNECTING_NET
        );
    }


    void run()
    {
        /*
         * OTA.
         *
         * Az OTA.h az állapot bekapcsolása után
         * itt kerül meghívásra.
         */

        if (
            BlynkState::is(
                MODE_OTA_UPGRADE
            )
        )
        {
             enterOTA();

            return;
        }


        /*
         * Nincs konfiguráció.
         */

        if (
            BlynkState::is(
                MODE_WAIT_CONFIG
            )
        )
        {
            enterConfigMode();

            return;
        }


        /*
         * Konfigurációs állapot.
         */

        if (
            BlynkState::is(
                MODE_CONFIGURING
            )
        )
        {
            return;
        }


        /*
         * WiFi csatlakozás.
         */

        if (
            BlynkState::is(
                MODE_CONNECTING_NET
            )
        )
        {
            if (
                wifiConnectStarted == 0
            )
            {
                startWiFiConnection();

                return;
            }


            processWiFiConnection();

            return;
        }


        /*
         * Blynk csatlakozás.
         */

        if (
            BlynkState::is(
                MODE_CONNECTING_CLOUD
            )
        )
        {
            if (
                WiFi.status() !=
                WL_CONNECTED
            )
            {
                wifiConnectStarted = 0;

                BlynkState::set(
                    MODE_CONNECTING_NET
                );

                return;
            }


            if (
                !Blynk.connected()
            )
            {
                startBlynkConnection();

                return;
            }


            BlynkState::set(
                MODE_RUNNING
            );

            return;
        }


        /*
         * Normál működés.
         */

        if (
            BlynkState::is(
                MODE_RUNNING
            )
        )
        {
            /*
             * Rövid Blynk feldolgozás.
             */

            Blynk.run();

                /*
     * Edgent timer futtatása.
     *
     * Ez hajtja végre az OTA callbackben
     * beállított setTimeout() műveleteket.
     */

    edgentTimer.run();


            /*
             * OTA callback közben az állapot
             * MODE_OTA_UPGRADE-ra változhat.
             */

            if (
                BlynkState::is(
                    MODE_OTA_UPGRADE
                )
            )
            {
                return;
            }


            /*
             * WiFi elveszett.
             */

            if (
                WiFi.status() !=
                WL_CONNECTED
            )
            {
                DEBUG_PRINT(
                    "WiFi connection lost."
                );


                Blynk.disconnect();


                wifiConnectStarted = 0;


                BlynkState::set(
                    MODE_CONNECTING_NET
                );

                return;
            }


            /*
             * Blynk elveszett.
             */

            if (
                !Blynk.connected()
            )
            {
                DEBUG_PRINT(
                    "Blynk connection lost."
                );


                blynkConnectStarted = 0;


                BlynkState::set(
                    MODE_CONNECTING_CLOUD
                );

                return;
            }


            return;
        }


        /*
         * Reset configuration.
         */

        if (
            BlynkState::is(
                MODE_RESET_CONFIG
            )
        )
        {
            resetConfigStorage();

            BlynkState::set(
                MODE_WAIT_CONFIG
            );

            return;
        }


        /*
         * Switch STA.
         */

        if (
            BlynkState::is(
                MODE_SWITCH_TO_STA
            )
        )
        {
            WiFi.mode(
                WIFI_STA
            );


            wifiConnectStarted = 0;


            BlynkState::set(
                MODE_CONNECTING_NET
            );

            return;
        }


        /*
         * Hiba.
         */

        if (
            BlynkState::is(
                MODE_ERROR
            )
        )
        {
            DEBUG_PRINT(
                "BlynkEdgent ERROR state."
            );


            delay(100);


            BlynkState::set(
                MODE_CONNECTING_NET
            );

            return;
        }
    }
};


/*
 * =========================================================
 * Global Edgent object
 * =========================================================
 */

Edgent BlynkEdgent;


/*
 * =========================================================
 * OTA include
 * =========================================================
 *
 * FONTOS:
 * Az OTA.h használja a fenti BlynkState,
 * restartMCU és edgentTimer objektumokat.
 *
 * Ezért itt, az Edgent deklaráció után
 * kerül beillesztésre.
 * =========================================================
 */

#include "OTA.h"
