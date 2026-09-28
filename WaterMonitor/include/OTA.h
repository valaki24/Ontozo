#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>


/*
 * BlynkEdgent.h-ból érkezik:
 *
 *   restartMCU()
 *   BlynkState
 *   MODE_OTA_UPGRADE
 *   edgentTimer
 */


/*
 * =========================================================
 * OTA state
 * =========================================================
 */

String overTheAirURL;


/*
 * =========================================================
 * OTA fatal error
 * =========================================================
 */

#define OTA_FATAL(...)                  \
{                                       \
    DEBUG_PRINT(__VA_ARGS__);           \
    delay(1000);                        \
    restartMCU();                       \
}


/*
 * =========================================================
 * URL parser
 * =========================================================
 */

bool parseURL(
    String url,
    String& protocol,
    String& host,
    int& port,
    String& uri
)
{
    protocol = "";
    host = "";
    uri = "/";
    port = 0;


    int index =
        url.indexOf(':');


    if (index < 0)
    {
        return false;
    }


    protocol =
        url.substring(
            0,
            index
        );

    protocol.toLowerCase();


    if (
        !url.startsWith(
            "://",
            index
        )
    )
    {
        return false;
    }


    url.remove(
        0,
        index + 3
    );


    index =
        url.indexOf('/');


    String server;


    if (index >= 0)
    {
        server =
            url.substring(
                0,
                index
            );

        url.remove(
            0,
            index
        );
    }
    else
    {
        server =
            url;

        url = "/";
    }


    index =
        server.indexOf(':');


    if (index >= 0)
    {
        host =
            server.substring(
                0,
                index
            );

        port =
            server.substring(
                index + 1
            ).toInt();
    }
    else
    {
        host =
            server;

        if (protocol == "http")
        {
            port = 80;
        }
        else if (protocol == "https")
        {
            port = 443;
        }
        else
        {
            return false;
        }
    }


    if (url.length() > 0)
    {
        uri = url;
    }
    else
    {
        uri = "/";
    }


    return (
        host.length() > 0 &&
        port > 0
    );
}


/*
 * =========================================================
 * HTTPS connection
 * =========================================================
 */

static WiFiClientSecure* connectSSL(
    const String& host,
    int port
)
{
    WiFiClientSecure* client =
        new WiFiClientSecure();


    /*
     * Blynk CA tanúsítvány.
     */

    client->setCACert(
        BLYNK_DEFAULT_ROOT_CA
    );


    client->setTimeout(
        10
    );


    DEBUG_PRINT(
        String("OTA HTTPS connect: ") +
        host +
        ":" +
        port
    );


    if (
        !client->connect(
            host.c_str(),
            port
        )
    )
    {
        delete client;

        OTA_FATAL(
            F("Secure OTA connection failed")
        );

        return nullptr;
    }


    DEBUG_PRINT(
        "OTA TLS connection OK"
    );


    return client;
}


/*
 * =========================================================
 * HTTP connection
 * =========================================================
 */

static WiFiClient* connectTCP(
    const String& host,
    int port
)
{
    WiFiClient* client =
        new WiFiClient();


    client->setTimeout(
        10
    );


    DEBUG_PRINT(
        String("OTA HTTP connect: ") +
        host +
        ":" +
        port
    );


    if (
        !client->connect(
            host.c_str(),
            port
        )
    )
    {
        delete client;

        OTA_FATAL(
            F("OTA client connection failed")
        );

        return nullptr;
    }


    return client;
}


/*
 * =========================================================
 * OTA
 * =========================================================
 */

void enterOTA()
{
    /*
     * Biztonsági ellenőrzés.
     */

    if (
        WiFi.status() != WL_CONNECTED
    )
    {
        OTA_FATAL(
            F("OTA requires WiFi")
        );

        return;
    }


    if (
        overTheAirURL.length() == 0
    )
    {
        OTA_FATAL(
            F("OTA URL is empty")
        );

        return;
    }


    DEBUG_PRINT(
        String("OTA URL: ") +
        overTheAirURL
    );


    String protocol;
    String host;
    String uri;

    int port = 0;


    if (
        !parseURL(
            overTheAirURL,
            protocol,
            host,
            port,
            uri
        )
    )
    {
        OTA_FATAL(
            F("Cannot parse OTA URL")
        );

        return;
    }


    DEBUG_PRINT(
        String("OTA protocol: ") +
        protocol
    );

    DEBUG_PRINT(
        String("OTA host: ") +
        host
    );

    DEBUG_PRINT(
        String("OTA port: ") +
        port
    );

    DEBUG_PRINT(
        String("OTA URI: ") +
        uri
    );


    Client* client =
        nullptr;


    /*
     * HTTPS.
     */

    if (
        protocol == "https"
    )
    {
        client =
            connectSSL(
                host,
                port
            );
    }


    /*
     * HTTP.
     */

    else if (
        protocol == "http"
    )
    {
        client =
            connectTCP(
                host,
                port
            );
    }


    else
    {
        OTA_FATAL(
            String("Unsupported OTA protocol: ") +
            protocol
        );

        return;
    }


    if (client == nullptr)
    {
        OTA_FATAL(
            F("OTA client unavailable")
        );

        return;
    }


    /*
     * HTTP GET.
     */

    client->print(
        String("GET ") +
        uri +
        " HTTP/1.0\r\n"
        "Host: " +
        host +
        "\r\n"
        "Connection: close\r\n"
        "\r\n"
    );


    /*
     * Válaszra várás.
     */

    uint32_t waitStart =
        millis();


    while (
        client->connected() &&
        !client->available()
    )
    {
        if (
            millis() - waitStart >
            10000UL
        )
        {
            client->stop();
            delete client;

            OTA_FATAL(
                F("OTA response timeout")
            );

            return;
        }


        delay(10);
    }


    /*
     * HTTP status.
     */

    String statusLine =
        client->readStringUntil('\n');

    statusLine.trim();


    DEBUG_PRINT(
        String("OTA status: ") +
        statusLine
    );


    if (
        !statusLine.startsWith(
            "HTTP/1.1 200"
        ) &&
        !statusLine.startsWith(
            "HTTP/1.0 200"
        )
    )
    {
        client->stop();
        delete client;

        OTA_FATAL(
            String("OTA HTTP error: ") +
            statusLine
        );

        return;
    }


    /*
     * HTTP headers.
     */

    int contentLength = 0;

    String md5;


    while (
        client->connected()
    )
    {
        String line =
            client->readStringUntil('\n');

        line.trim();


        if (
            line.length() == 0
        )
        {
            break;
        }


        String lower =
            line;

        lower.toLowerCase();


        if (
            lower.startsWith(
                "content-length:"
            )
        )
        {
            contentLength =
                lower.substring(
                    lower.indexOf(':') + 1
                ).toInt();
        }


        else if (
            lower.startsWith(
                "x-md5:"
            )
        )
        {
            md5 =
                line.substring(
                    line.indexOf(':') + 1
                );

            md5.trim();
            md5.toLowerCase();
        }
    }


    DEBUG_PRINT(
        String("OTA size: ") +
        contentLength
    );


    if (
        contentLength <= 0
    )
    {
        client->stop();
        delete client;

        OTA_FATAL(
            F("Content-Length not defined")
        );

        return;
    }


    /*
     * Flash update indítása.
     */

    if (
        !Update.begin(
            contentLength
        )
    )
    {
        Update.printError(
            Serial
        );

        client->stop();
        delete client;

        OTA_FATAL(
            F("OTA Update.begin failed")
        );

        return;
    }


    /*
     * MD5 ellenőrzés.
     */

    if (
        md5.length() > 0
    )
    {
        DEBUG_PRINT(
            String("Expected MD5: ") +
            md5
        );


        if (
            !Update.setMD5(
                md5.c_str()
            )
        )
        {
            client->stop();
            delete client;

            OTA_FATAL(
                F("Cannot set OTA MD5")
            );

            return;
        }
    }


    DEBUG_PRINT(
        "OTA flashing..."
    );


    /*
     * OTA buffer.
     */

    uint8_t buffer[1024];

    size_t written = 0;

    int previousProgress = -10;


    while (
        client->connected() &&
        written <
        (size_t)contentLength
    )
    {
        if (
            !client->available()
        )
        {
            delay(1);
            continue;
        }


        int available =
            client->available();


        int remaining =
            contentLength -
            written;


        int toRead =
            min(
                available,
                (int)sizeof(buffer)
            );


        toRead =
            min(
                toRead,
                remaining
            );


        if (
            toRead <= 0
        )
        {
            break;
        }


        int len =
            client->read(
                buffer,
                toRead
            );


        if (
            len <= 0
        )
        {
            continue;
        }


        size_t result =
            Update.write(
                buffer,
                len
            );


        if (
            result !=
            (size_t)len
        )
        {
            Update.printError(
                Serial
            );

            client->stop();
            delete client;

            OTA_FATAL(
                F("OTA write failed")
            );

            return;
        }


        written += len;


        int progress =
            (written * 100) /
            contentLength;


        if (
            progress >=
            previousProgress + 10
        )
        {
            DEBUG_PRINT(
                String("OTA: ") +
                progress +
                "%"
            );

            previousProgress =
                progress;
        }
    }


    client->stop();
    delete client;


    /*
     * Teljes firmware megérkezett?
     */

    if (
        written !=
        (size_t)contentLength
    )
    {
        Update.abort();

        OTA_FATAL(
            String("OTA incomplete: ") +
            written +
            " / " +
            contentLength
        );

        return;
    }


    /*
     * Flash lezárása.
     */

    if (
        !Update.end(true)
    )
    {
        Update.printError(
            Serial
        );

        OTA_FATAL(
            F("OTA Update.end failed")
        );

        return;
    }


    if (
        !Update.isFinished()
    )
    {
        OTA_FATAL(
            F("OTA update not finished")
        );

        return;
    }


    /*
     * SIKER
     */

    DEBUG_PRINT(
        "================================"
    );

    DEBUG_PRINT(
        "OTA SUCCESS"
    );

    DEBUG_PRINT(
        "Rebooting..."
    );

    DEBUG_PRINT(
        "================================"
    );


    delay(500);

    restartMCU();
}


/*
 * =========================================================
 * Blynk OTA trigger
 * =========================================================
 *
 * A Blynk InternalPinOTA adja át az OTA URL-t.
 *
 * NEM használunk:
 *
 *     Blynk.logEvent("sys_ota", ...)
 *
 * így nincs szükség sys_ota Event létrehozására.
 * =========================================================
 */

BLYNK_WRITE(InternalPinOTA)
{
    Serial.println(">>> InternalPinOTA meghivva!");

    String url = param.asString();

    Serial.print(">>> OTA URL hossza: ");
    Serial.println(url.length());

    Serial.print(">>> OTA URL: [");
    Serial.print(url);
    Serial.println("]");

    if (url.length() == 0)
    {
        Serial.println(">>> OTA URL URES!");
        return;
    }

    overTheAirURL = url;

    Serial.print(">>> OTA requested: ");
    Serial.println(overTheAirURL);
Serial.println(">>> OTA timer beallitasa");
    edgentTimer.setTimeout(
        250,
        []()
        {
            Serial.println(">>> OTA TIMER LEFUTOTT!");

            Blynk.disconnect();
            Serial.println(">>> Blynk.disconnect OK");

            BlynkState::set(
                MODE_OTA_UPGRADE
            );
            Serial.println(">>> MODE_OTA_UPGRADE beallitva");
        }
    );
}
