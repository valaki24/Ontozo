#ifndef CONFIG_MODE_H
#define CONFIG_MODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "Settings.h"
#include "ConfigStore.h"


#ifndef BLYNK_FS

const char* config_form = R"html(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>WiFi setup</title>
  <style>
    body {
      background-color: #fcfcfc;
      box-sizing: border-box;
      margin: 0;
      padding: 20px;
    }

    body, input, button {
      font-family: Arial, sans-serif;
      font-size: 16px;
    }

    .centered {
      max-width: 500px;
      margin: 40px auto;
      padding: 25px;
      background-color: #ddd;
      border-radius: 8px;
      box-sizing: border-box;
    }

    h2 {
      margin-top: 0;
    }

    table {
      width: 100%;
    }

    td {
      padding: 6px 0;
    }

    td:first-child {
      width: 130px;
    }

    label {
      white-space: nowrap;
    }

    input {
      width: 100%;
      box-sizing: border-box;
      padding: 8px;
    }

    input[type="submit"] {
      margin-top: 15px;
      padding: 10px;
      cursor: pointer;
    }

    .info {
      font-size: 13px;
      color: #555;
      margin-top: 15px;
    }
  </style>
</head>

<body>

<div class="centered">

  <h2>WiFi setup</h2>

  <form method="get" action="/config">

    <table>

      <tr>
        <td>
          <label for="ssid">WiFi SSID:</label>
        </td>
        <td>
          <input type="text"
                 name="ssid"
                 maxlength="32"
                 required>
        </td>
      </tr>

      <tr>
        <td>
          <label for="pass">Password:</label>
        </td>
        <td>
          <input type="password"
                 name="pass"
                 maxlength="63">
        </td>
      </tr>

      <tr>
        <td>
          <label for="blynk">Auth token:</label>
        </td>
        <td>
          <input type="text"
                 name="blynk"
                 maxlength="32"
                 required>
        </td>
      </tr>

      <tr>
        <td>
          <label for="host">Host:</label>
        </td>
        <td>
          <input type="text"
                 name="host"
                 value="blynk.cloud"
                 maxlength="63">
        </td>
      </tr>

      <tr>
        <td>
          <label for="port_ssl">Port:</label>
        </td>
        <td>
          <input type="number"
                 name="port_ssl"
                 value="443"
                 min="1"
                 max="65535">
        </td>
      </tr>

    </table>

    <input type="submit" value="Apply">

  </form>

  <div class="info">
    Device: %DEVICE_NAME%
  </div>

</div>

</body>
</html>
)html";

#endif


// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;


// ============================================================
// RESTART
// ============================================================

void restartMCU()
{
    delay(100);

    ESP.restart();

    while (true)
    {
        delay(100);
    }
}


// ============================================================
// ESP32 DEVICE NAME
// ============================================================

static String getWiFiName(bool withPrefix = true)
{
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char unique[7];

    snprintf(
        unique,
        sizeof(unique),
        "%02X%02X%02X",
        mac[3],
        mac[4],
        mac[5]
    );

    String name = BLYNK_TEMPLATE_NAME;

    // AP név hossza maradjon kezelhető
    if (name.length() > 20)
    {
        name = name.substring(0, 20);
    }

    if (withPrefix)
    {
        return String(CONFIG_DEVICE_PREFIX) +
               "-" +
               name +
               "-" +
               unique;
    }

    return name + "-" + unique;
}


// ============================================================
// MAC -> STRING
// ============================================================

static String macToString(const uint8_t mac[6])
{
    char buff[20];

    snprintf(
        buff,
        sizeof(buff),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    return String(buff);
}


// ============================================================
// WIFI SECURITY STRING
// ============================================================

static const char* wifiSecToStr(wifi_auth_mode_t type)
{
    switch (type)
    {
        case WIFI_AUTH_OPEN:
            return "OPEN";

        case WIFI_AUTH_WEP:
            return "WEP";

        case WIFI_AUTH_WPA_PSK:
            return "WPA";

        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";

        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA+WPA2";

        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-ENT";

        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";

        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2+WPA3";

        default:
            return "UNKNOWN";
    }
}


// ============================================================
// WIFI INFORMATION
// ============================================================

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


// ============================================================
// CONFIG PAGE
// ============================================================

static void handleConfigRoot()
{
#ifndef BLYNK_FS

    String page = config_form;

    page.replace(
        "%DEVICE_NAME%",
        getWiFiName()
    );

    server.send(
        200,
        "text/html; charset=utf-8",
        page
    );

#else

    server.send(
        404,
        "text/plain",
        "Configuration page not available"
    );

#endif
}


// ============================================================
// CAPTIVE PORTAL
// ============================================================

static void handleRoot()
{
    handleConfigRoot();
}


static void handleNotFound()
{
#ifdef WIFI_CAPTIVE_PORTAL_ENABLE

    server.sendHeader(
        "Location",
        String("http://") + WiFi.softAPIP().toString(),
        true
    );

    server.send(
        302,
        "text/plain",
        ""
    );

#else

    server.send(
        404,
        "text/plain",
        "Not found"
    );

#endif
}


// ============================================================
// ENTER CONFIG MODE
// ============================================================

void enterConfigMode()
{
    BlynkState::set(MODE_WAIT_CONFIG);

    DEBUG_PRINT("Entering WiFi configuration mode...");

    // --------------------------------------------------------
    // AP MODE
    // --------------------------------------------------------

    WiFi.mode(WIFI_AP);

    delay(100);

    WiFi.softAPConfig(
        WIFI_AP_IP,
        WIFI_AP_IP,
        WIFI_AP_Subnet
    );

    String apName = getWiFiName();

    if (!WiFi.softAP(apName.c_str()))
    {
        DEBUG_PRINT("ERROR: Failed to start AP");

        config_set_last_error(
            BLYNK_PROV_ERR_INTERNAL
        );

        BlynkState::set(MODE_ERROR);

        return;
    }

    delay(300);

    IPAddress apIP = WiFi.softAPIP();

    DEBUG_PRINT(
        String("Configuration AP: ") +
        apName
    );

    DEBUG_PRINT(
        String("AP IP: ") +
        apIP.toString()
    );


    // --------------------------------------------------------
    // DNS SERVER
    // --------------------------------------------------------

    dnsServer.setTTL(300);

    dnsServer.setErrorReplyCode(
        DNSReplyCode::ServerFailure
    );

#ifdef WIFI_CAPTIVE_PORTAL_ENABLE

    dnsServer.start(
        DNS_PORT,
        "*",
        apIP
    );

    server.onNotFound(handleRoot);

#else

    dnsServer.start(
        DNS_PORT,
        CONFIG_AP_URL,
        apIP
    );

    server.onNotFound(handleNotFound);

#endif


    // --------------------------------------------------------
    // ROOT
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        handleConfigRoot
    );


    // --------------------------------------------------------
    // CONFIG
    // --------------------------------------------------------

    server.on(
        "/config",
        HTTP_GET,
        []()
        {
            DEBUG_PRINT("Applying configuration...");

            String ssid = server.arg("ssid");
            String pass = server.arg("pass");

            String ssidManual = server.arg("ssidManual");

            if (ssidManual.length() > 0)
            {
                ssid = ssidManual;
            }

            String token = server.arg("blynk");
            String host  = server.arg("host");
            String port  = server.arg("port_ssl");

            String ip   = server.arg("ip");
            String mask = server.arg("mask");
            String gw   = server.arg("gw");
            String dns  = server.arg("dns");
            String dns2 = server.arg("dns2");

            bool forceSave =
                server.arg("save").toInt() != 0;


            DEBUG_PRINT(
                String("WiFi SSID: ") +
                ssid
            );

            DEBUG_PRINT(
                String("Blynk host: ") +
                host +
                ":" +
                port
            );


            // ------------------------------------------------
            // VALIDATION
            // ------------------------------------------------

            if (
                ssid.length() == 0 ||
                token.length() != 32
            )
            {
                DEBUG_PRINT(
                    "Configuration invalid"
                );

                server.send(
                    400,
                    "application/json",
                    R"json({"status":"error","msg":"Configuration invalid"})json"
                );

                return;
            }


            // ------------------------------------------------
            // DEFAULT CONFIG
            // ------------------------------------------------

            configStore = configDefault;


            // ------------------------------------------------
            // WIFI
            // ------------------------------------------------

            CopyString(
                ssid,
                configStore.wifiSSID
            );

            CopyString(
                pass,
                configStore.wifiPass
            );


            // ------------------------------------------------
            // BLYNK
            // ------------------------------------------------

            CopyString(
                token,
                configStore.cloudToken
            );


            if (host.length() > 0)
            {
                CopyString(
                    host,
                    configStore.cloudHost
                );
            }


            if (port.length() > 0)
            {
                uint32_t p = port.toInt();

                if (p >= 1 && p <= 65535)
                {
                    configStore.cloudPort =
                        (uint16_t)p;
                }
            }


            // ------------------------------------------------
            // STATIC IP
            // ------------------------------------------------

            IPAddress addr;


            if (
                ip.length() > 0 &&
                addr.fromString(ip)
            )
            {
                configStore.staticIP =
                    (uint32_t)addr;

                configStore.setFlag(
                    CONFIG_FLAG_STATIC_IP,
                    true
                );
            }
            else
            {
                configStore.setFlag(
                    CONFIG_FLAG_STATIC_IP,
                    false
                );
            }


            if (
                mask.length() > 0 &&
                addr.fromString(mask)
            )
            {
                configStore.staticMask =
                    (uint32_t)addr;
            }


            if (
                gw.length() > 0 &&
                addr.fromString(gw)
            )
            {
                configStore.staticGW =
                    (uint32_t)addr;
            }


            if (
                dns.length() > 0 &&
                addr.fromString(dns)
            )
            {
                configStore.staticDNS =
                    (uint32_t)addr;
            }


            if (
                dns2.length() > 0 &&
                addr.fromString(dns2)
            )
            {
                configStore.staticDNS2 =
                    (uint32_t)addr;
            }


            // ------------------------------------------------
            // SAVE
            // ------------------------------------------------

            if (forceSave)
            {
                configStore.setFlag(
                    CONFIG_FLAG_VALID,
                    true
                );

                configStore.last_error =
                    BLYNK_PROV_ERR_NONE;

                config_save();

                DEBUG_PRINT(
                    "Configuration saved."
                );

                server.send(
                    200,
                    "application/json",
                    R"json({"status":"ok","msg":"Configuration saved"})json"
                );
            }
            else
            {
                // A normál Edgent működésben is mentsük,
                // mert itt már érvényes konfigurációt kaptunk.

                configStore.setFlag(
                    CONFIG_FLAG_VALID,
                    true
                );

                configStore.last_error =
                    BLYNK_PROV_ERR_NONE;

                config_save();

                server.send(
                    200,
                    "application/json",
                    R"json({"status":"ok","msg":"Configuration saved. Restarting..."})json"
                );
            }


            delay(500);

            BlynkState::set(
                MODE_SWITCH_TO_STA
            );
        }
    );


    // --------------------------------------------------------
    // BOARD INFO
    // --------------------------------------------------------

    server.on(
        "/board_info.json",
        HTTP_GET,
        []()
        {
            BlynkState::set(
                MODE_CONFIGURING
            );

            char buff[768];

            snprintf(
                buff,
                sizeof(buff),

                R"json({
                    "board":"%s",
                    "tmpl_id":"%s",
                    "fw_type":"%s",
                    "fw_ver":"%s",
                    "ssid":"%s",
                    "bssid":"%s",
                    "mac":"%s",
                    "last_error":%d,
                    "wifi_scan":true,
                    "static_ip":true
                })json",

                BLYNK_TEMPLATE_NAME,
                BLYNK_TEMPLATE_ID,
                BLYNK_FIRMWARE_TYPE,
                BLYNK_FIRMWARE_VERSION,
                getWiFiName().c_str(),
                getWiFiApBSSID().c_str(),
                getWiFiMacAddress().c_str(),
                configStore.last_error
            );

            server.send(
                200,
                "application/json",
                buff
            );
        }
    );


    // --------------------------------------------------------
    // WIFI SCAN
    // --------------------------------------------------------

    server.on(
        "/wifi_scan.json",
        HTTP_GET,
        []()
        {
            DEBUG_PRINT(
                "Scanning WiFi networks..."
            );

            int wifi_nets =
                WiFi.scanNetworks(
                    true,
                    true
                );

            uint32_t start =
                millis();


            while (
                wifi_nets < 0 &&
                millis() - start < 20000
            )
            {
                delay(20);

                dnsServer.processNextRequest();

                server.handleClient();

                wifi_nets =
                    WiFi.scanComplete();
            }


            DEBUG_PRINT(
                String("Found networks: ") +
                wifi_nets
            );


            if (wifi_nets <= 0)
            {
                WiFi.scanDelete();

                server.send(
                    200,
                    "application/json",
                    "[]"
                );

                return;
            }


            // ------------------------------------------------
            // SORT BY RSSI
            // ------------------------------------------------

            int indices[wifi_nets];

            for (int i = 0; i < wifi_nets; i++)
            {
                indices[i] = i;
            }


            for (int i = 0; i < wifi_nets; i++)
            {
                for (
                    int j = i + 1;
                    j < wifi_nets;
                    j++
                )
                {
                    if (
                        WiFi.RSSI(indices[j]) >
                        WiFi.RSSI(indices[i])
                    )
                    {
                        int tmp = indices[i];

                        indices[i] =
                            indices[j];

                        indices[j] =
                            tmp;
                    }
                }
            }


            int count =
                min(15, wifi_nets);


            // ------------------------------------------------
            // JSON RESPONSE
            // ------------------------------------------------

            String json = "[";


            for (int i = 0; i < count; i++)
            {
                int id = indices[i];

                if (i > 0)
                {
                    json += ",";
                }


                json += "{";

                json +=
                    "\"ssid\":\"" +
                    WiFi.SSID(id) +
                    "\",";

                json +=
                    "\"bssid\":\"" +
                    WiFi.BSSIDstr(id) +
                    "\",";

                json +=
                    "\"rssi\":" +
                    String(WiFi.RSSI(id)) +
                    ",";

                json +=
                    "\"sec\":\"" +
                    String(
                        wifiSecToStr(
                            WiFi.encryptionType(id)
                        )
                    ) +
                    "\",";

                json +=
                    "\"ch\":" +
                    String(
                        WiFi.channel(id)
                    ) +
                    ",";

                json +=
                    "\"hidden\":" +
                    String(
                        WiFi.isHidden(id) ?
                        1 :
                        0
                    );

                json += "}";
            }


            json += "]";


            WiFi.scanDelete();


            server.send(
                200,
                "application/json",
                json
            );
        }
    );


    // --------------------------------------------------------
    // RESET CONFIGURATION
    // --------------------------------------------------------

    server.on(
        "/reset",
        HTTP_GET,
        []()
        {
            DEBUG_PRINT(
                "Reset configuration requested."
            );

            configStore =
                configDefault;

            configStore.setFlag(
                CONFIG_FLAG_VALID,
                false
            );

            config_save();

            server.send(
                200,
                "application/json",
                R"json({"status":"ok","msg":"Configuration reset"})json"
            );

            delay(500);

            restartMCU();
        }
    );


    // --------------------------------------------------------
    // REBOOT
    // --------------------------------------------------------

    server.on(
        "/reboot",
        HTTP_GET,
        []()
        {
            server.send(
                200,
                "application/json",
                R"json({"status":"ok","msg":"Rebooting"})json"
            );

            delay(300);

            restartMCU();
        }
    );


    // --------------------------------------------------------
    // START SERVER
    // --------------------------------------------------------

    server.begin();

    DEBUG_PRINT(
        "Configuration web server started."
    );


    // ========================================================
    // CONFIG MODE LOOP
    // ========================================================
    //
    // Ez csak akkor fut, amikor konfigurációs AP módban
    // vagyunk. Normál üzemben nem blokkolja a programot.
    //
    // ========================================================

    while (
        BlynkState::is(MODE_WAIT_CONFIG) ||
        BlynkState::is(MODE_CONFIGURING)
    )
    {
        dnsServer.processNextRequest();

        server.handleClient();

        delay(2);

        yield();

        // Ha konfiguráció után STA módba váltottunk,
        // kilépünk.
        if (
            BlynkState::is(MODE_SWITCH_TO_STA) ||
            BlynkState::is(MODE_CONNECTING_NET) ||
            BlynkState::is(MODE_ERROR)
        )
        {
            break;
        }
    }


    // ========================================================
    // STOP AP SERVER
    // ========================================================

    server.stop();

    dnsServer.stop();

    WiFi.softAPdisconnect(true);

    DEBUG_PRINT(
        "Configuration mode stopped."
    );
}


// ============================================================
// CONNECT TO WIFI
// ============================================================
//
// FONTOS:
// Ez már csak elindítja a csatlakozást.
// Nem vár 50 másodpercig egy while ciklusban.
//
// ============================================================

void enterConnectNet()
{
    BlynkState::set(
        MODE_CONNECTING_NET
    );

    DEBUG_PRINT(
        String("Connecting to WiFi: ") +
        configStore.wifiSSID
    );


    // --------------------------------------------------------
    // STA MODE
    // --------------------------------------------------------

    WiFi.mode(WIFI_STA);


    // --------------------------------------------------------
    // HOSTNAME
    // --------------------------------------------------------

    String hostname =
        getWiFiName();

    hostname.replace(
        " ",
        "-"
    );

    WiFi.setHostname(
        hostname.c_str()
    );


    // --------------------------------------------------------
    // STATIC IP
    // --------------------------------------------------------

    if (
        configStore.getFlag(
            CONFIG_FLAG_STATIC_IP
        )
    )
    {
        IPAddress localIP(
            configStore.staticIP
        );

        IPAddress gateway(
            configStore.staticGW
        );

        IPAddress subnet(
            configStore.staticMask
        );

        IPAddress dns1(
            configStore.staticDNS
        );

        IPAddress dns2(
            configStore.staticDNS2
        );


        if (
            !WiFi.config(
                localIP,
                gateway,
                subnet,
                dns1,
                dns2
            )
        )
        {
            DEBUG_PRINT(
                "Failed to configure Static IP"
            );

            config_set_last_error(
                BLYNK_PROV_ERR_CONFIG
            );

            BlynkState::set(
                MODE_ERROR
            );

            return;
        }
    }


    // --------------------------------------------------------
    // START WIFI CONNECTION
    // --------------------------------------------------------

    WiFi.begin(
        configStore.wifiSSID,
        configStore.wifiPass
    );


    DEBUG_PRINT(
        "WiFi connection started."
    );
}


// ============================================================
// DISCONNECT WIFI
// ============================================================

static void disconnectWiFi()
{
    WiFi.disconnect(true);

    delay(50);

    WiFi.mode(WIFI_OFF);
}

#endif
