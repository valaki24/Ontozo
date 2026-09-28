#include <Arduino.h>
#include <LittleFS.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "packet.h"
#include "water_receiver.h"


/******************************************************
 * BEÁLLÍTÁSOK
 ******************************************************/

#define WATERMONITOR_IP "192.168.2.137"

#define WATER_HTTP_PORT 80

#define WATERMONITOR_TIMEOUT_MS 7000UL

#define WATER_HTTP_TIMEOUT_MS 3000


/******************************************************
 * HTTP SZERVER
 ******************************************************/

static ESP8266WebServer waterServer(
    WATER_HTTP_PORT
);


/******************************************************
 * WATERMONITOR ADATAI
 ******************************************************/

WaterMessage lastWaterMessage;

volatile bool waterMessageReceived = false;


/******************************************************
 * BELSŐ ÁLLAPOT
 ******************************************************/

static bool receiverInitialized = false;

static uint32_t lastWaterMessageMillis = 0;


/******************************************************
 * WATERMESSAGE COUNTER VÉDELEM
 ******************************************************/

/*
 * Az utolsó elfogadott WaterMessage countere.
 */

static uint32_t lastWaterMessageCounter = 0;

static bool haveWaterMessageCounter = false;


/******************************************************
 * ÖNTÖZŐ → WATERMONITOR COUNTER
 ******************************************************/

static uint32_t commandCounter = 0;
static const char* COMMAND_COUNTER_FILE =
    "/water_command_counter.txt";


/******************************************************
 * COMMAND COUNTER BETÖLTÉSE
 ******************************************************/

static void loadCommandCounter()
{
    if(!LittleFS.exists(
        COMMAND_COUNTER_FILE
    ))
    {
        commandCounter = 0;

        return;
    }


    File f = LittleFS.open(
        COMMAND_COUNTER_FILE,
        "r"
    );


    if(!f)
    {
        commandCounter = 0;

        return;
    }


    String value =
        f.readStringUntil('\n');

    f.close();


    value.trim();


    if(value.length() == 0)
    {
        commandCounter = 0;

        return;
    }


    commandCounter =
        strtoul(
            value.c_str(),
            nullptr,
            10
        );


    Serial.print(
        "WaterCommand counter betoltve: "
    );

    Serial.println(
        commandCounter
    );
}


/******************************************************
 * COMMAND COUNTER MENTÉSE
 ******************************************************/

static void saveCommandCounter()
{
    File f = LittleFS.open(
        COMMAND_COUNTER_FILE,
        "w"
    );


    if(!f)
    {
        Serial.println(
            "WaterCommand counter mentesi hiba"
        );

        return;
    }


    f.println(
        commandCounter
    );


    f.close();
}


/******************************************************
 * SEGÉDFÜGGVÉNY
 *
 * Újabb-e a counter?
 *
 * A signed különbség miatt a uint32_t
 * túlcsordulását is helyesen kezeli.
 *
 * Példa:
 *
 * 100 -> 101     új
 * 101 -> 101     dupla
 * 101 -> 100     régi
 *
 * 4294967295 -> 0     új
 ******************************************************/

static bool isCounterNewer(
    uint32_t newCounter,
    uint32_t oldCounter
)
{
    return (
        (int32_t)(newCounter - oldCounter) > 0
    );
}


/******************************************************
 * EGYSZERŰ JSON KIOLVASÁS
 ******************************************************/

static bool jsonGetUint32(
    const String& json,
    const char* key,
    uint32_t& value
)
{
    String search = "\"";
    search += key;
    search += "\"";

    int pos = json.indexOf(search);

    if(pos < 0)
    {
        return false;
    }

    pos = json.indexOf(
        ':',
        pos + search.length()
    );

    if(pos < 0)
    {
        return false;
    }

    pos++;

    while(
        pos < (int)json.length() &&
        (
            json[pos] == ' ' ||
            json[pos] == '\t'
        )
    )
    {
        pos++;
    }

    int end = pos;

    while(
        end < (int)json.length() &&
        isDigit(json[end])
    )
    {
        end++;
    }

    if(end == pos)
    {
        return false;
    }

    value = strtoul(
        json.substring(
            pos,
            end
        ).c_str(),
        nullptr,
        10
    );

    return true;
}


/******************************************************
 * uint8 KIOLVASÁS
 ******************************************************/

static bool jsonGetUint8(
    const String& json,
    const char* key,
    uint8_t& value
)
{
    uint32_t temp = 0;

    if(!jsonGetUint32(
        json,
        key,
        temp
    ))
    {
        return false;
    }

    if(temp > 255)
    {
        return false;
    }

    value = (uint8_t)temp;

    return true;
}


/******************************************************
 * uint64 KIOLVASÁS
 ******************************************************/

static bool jsonGetUint64(
    const String& json,
    const char* key,
    uint64_t& value
)
{
    String search = "\"";
    search += key;
    search += "\"";

    int pos = json.indexOf(search);

    if(pos < 0)
    {
        return false;
    }

    pos = json.indexOf(
        ':',
        pos + search.length()
    );

    if(pos < 0)
    {
        return false;
    }

    pos++;

    while(
        pos < (int)json.length() &&
        (
            json[pos] == ' ' ||
            json[pos] == '\t'
        )
    )
    {
        pos++;
    }

    int end = pos;

    while(
        end < (int)json.length() &&
        isDigit(json[end])
    )
    {
        end++;
    }

    if(end == pos)
    {
        return false;
    }

    String number = json.substring(
        pos,
        end
    );

    value = strtoull(
        number.c_str(),
        nullptr,
        10
    );

    return true;
}


/******************************************************
 * WATER MESSAGE PARSER
 ******************************************************/

static bool parseWaterMessage(
    const String& json,
    WaterMessage& message
)
{
    uint32_t counter = 0;
    uint8_t type = 0;
    uint64_t total = 0;

    if(!jsonGetUint32(
        json,
        "counter",
        counter
    ))
    {
        return false;
    }

    if(!jsonGetUint8(
        json,
        "type",
        type
    ))
    {
        return false;
    }

    if(!jsonGetUint64(
        json,
        "totalMilliLiters",
        total
    ))
    {
        return false;
    }

    message.counter = counter;
    message.type = type;
    message.totalMilliLiters = total;

    return true;
}


/******************************************************
 * WATER MESSAGE FOGADÁSA
 ******************************************************/

static void handleWaterMessage()
{
    if(!waterServer.hasArg("plain"))
    {
        waterServer.send(
            400,
            "text/plain",
            "Missing body"
        );

        return;
    }

    String body =
        waterServer.arg("plain");

    WaterMessage message;

    if(!parseWaterMessage(
        body,
        message
    ))
    {
        Serial.println(
            "WaterMonitor: hibas HTTP adat"
        );

        Serial.println(body);

        waterServer.send(
            400,
            "text/plain",
            "Invalid WaterMessage"
        );

        return;
    }


    /**************************************************
     * COUNTER ELLENŐRZÉS
     **************************************************/

    if(
        haveWaterMessageCounter &&
        !isCounterNewer(
            message.counter,
            lastWaterMessageCounter
        )
    )
    {
        Serial.print(
            "WaterMessage DUPLIKALT/REGI: counter="
        );

        Serial.println(
            message.counter
        );

        /*
         * 200-at küldünk.
         *
         * A küldő számára az üzenet HTTP szinten
         * sikeresen megérkezett, csak már feldolgoztuk.
         */

        waterServer.send(
            200,
            "text/plain",
            "Already processed"
        );

        return;
    }


    /**************************************************
     * COUNTER ELFOGADÁSA
     **************************************************/

    lastWaterMessageCounter =
        message.counter;

    haveWaterMessageCounter =
        true;


    /**************************************************
     * ADATOK ÁTVÉTELE
     **************************************************/

    noInterrupts();

    lastWaterMessage = message;

    waterMessageReceived = true;

    interrupts();


    lastWaterMessageMillis =
        millis();


    Serial.print(
        "WaterMonitor HTTP RX: "
    );

    Serial.print(
        "counter="
    );

    Serial.print(
        message.counter
    );

    Serial.print(
        " type="
    );

    Serial.print(
        message.type
    );

    Serial.print(
        " total="
    );

    Serial.print(
        (double)message.totalMilliLiters / 1000.0
    );

    Serial.println(
        " L"
    );


    waterServer.send(
        200,
        "text/plain",
        "OK"
    );
}


/******************************************************
 * PING
 ******************************************************/

static void handleWaterPing()
{
    waterServer.send(
        200,
        "text/plain",
        "WaterController OK"
    );
}


/******************************************************
 * 404
 ******************************************************/

static void handleNotFound()
{
    waterServer.send(
        404,
        "text/plain",
        "Not found"
    );
}


/******************************************************
 * RECEIVER INDÍTÁSA
 ******************************************************/

void initWaterReceiver()
{
    if(receiverInitialized)
    {
        return;
    }

    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "HTTP Water Receiver inditasa"
    );

    Serial.println(
        "================================"
    );


    waterServer.on(
        "/water/message",
        HTTP_POST,
        handleWaterMessage
    );


    waterServer.on(
        "/water/ping",
        HTTP_GET,
        handleWaterPing
    );


    waterServer.onNotFound(
        handleNotFound
    );


    waterServer.begin();

    loadCommandCounter();


    Serial.print(
        "Water HTTP server port: "
    );

    Serial.println(
        WATER_HTTP_PORT
    );


    Serial.print(
        "WaterMonitor IP: "
    );

    Serial.println(
        WATERMONITOR_IP
    );


    receiverInitialized = true;


    Serial.println(
        "HTTP Water Receiver READY"
    );
}


/******************************************************
 * HTTP SERVER FRISSÍTÉSE
 ******************************************************/

void updateWaterReceiver()
{
    if(!receiverInitialized)
    {
        return;
    }

    waterServer.handleClient();
}


/******************************************************
 * VAN-E ÚJ VÁLASZ?
 ******************************************************/

bool waterMonitorResponseAvailable()
{
    return waterMessageReceived;
}


/******************************************************
 * VÁLASZ LEKÉRÉSE
 ******************************************************/

WaterMessage getWaterMonitorResponse()
{
    WaterMessage message;

    noInterrupts();

    message = lastWaterMessage;

    interrupts();

    return message;
}


/******************************************************
 * VÁLASZ FELDOLGOZVA
 ******************************************************/

void clearWaterMonitorResponse()
{
    noInterrupts();

    waterMessageReceived = false;

    interrupts();
}


/******************************************************
 * WATERMONITOR ONLINE?
 ******************************************************/

bool isWaterMonitorOnline()
{
    if(lastWaterMessageMillis == 0)
    {
        return false;
    }

    return (
        millis() - lastWaterMessageMillis
        <= WATERMONITOR_TIMEOUT_MS
    );
}


/******************************************************
 * UTOLSÓ WATERMONITOR ÜZENET
 ******************************************************/

uint32_t getWaterMonitorLastSeen()
{
    return lastWaterMessageMillis;
}


/******************************************************
 * ÖNTÖZŐ → WATERMONITOR
 ******************************************************/

bool sendWaterCommand(
    uint8_t commandType
)
{
    if(
        commandType == WATER_CMD_NONE
    )
    {
        return false;
    }


    /**************************************************
     * COMMAND COUNTER
     **************************************************/

    commandCounter++;
    saveCommandCounter();


    /**************************************************
     * JSON
     **************************************************/

    String json;

    json.reserve(80);

    json += "{\"counter\":";
    json += commandCounter;

    json += ",\"type\":";
    json += commandType;

    json += "}";


    /**************************************************
     * HTTP
     **************************************************/

    WiFiClient client;

    HTTPClient http;


    String url =
        String("http://") +
        WATERMONITOR_IP +
        "/water/command";


    Serial.print(
        "WaterCommand TX: "
    );

    Serial.print(
        "counter="
    );

    Serial.print(
        commandCounter
    );

    Serial.print(
        " type="
    );

    Serial.print(
        commandType
    );

    Serial.print(
        " -> "
    );

    Serial.println(
        url
    );


    http.setTimeout(
        WATER_HTTP_TIMEOUT_MS
    );


    if(!http.begin(
        client,
        url
    ))
    {
        Serial.println(
            "WaterCommand: HTTP begin hiba"
        );

        return false;
    }


    http.addHeader(
        "Content-Type",
        "application/json"
    );


    int httpCode =
        http.POST(json);


    bool success =
        httpCode >= 200 &&
        httpCode < 300;


    if(success)
    {
        Serial.print(
            "WaterCommand OK HTTP "
        );

        Serial.println(
            httpCode
        );
    }
    else
    {
        Serial.print(
            "WaterCommand HIBA HTTP "
        );

        Serial.println(
            httpCode
        );
    }


    http.end();


    return success;
}