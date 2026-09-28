#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include "packet.h"
#include "water_http.h"


/******************************************************
 * BEÁLLÍTÁSOK
 ******************************************************/

#define IRRIGATION_IP "192.168.2.194"

#define WATER_HTTP_PORT 80

#define WATER_HTTP_TIMEOUT_MS 3000

#define IRRIGATION_TIMEOUT_MS 7000UL


/******************************************************
 * HTTP SERVER
 ******************************************************/

static WebServer waterServer(
    WATER_HTTP_PORT
);


/******************************************************
 * WATER MESSAGE
 ******************************************************/

static uint32_t messageCounter = 0;


/******************************************************
 * WATER COMMAND
 ******************************************************/

static WaterCommand lastWaterCommand;

volatile bool waterCommandReceived = false;

static uint32_t lastIrrigationCommandMillis = 0;


/******************************************************
 * COMMAND COUNTER VÉDELEM
 ******************************************************/

static uint32_t lastCommandCounter = 0;

static bool haveCommandCounter = false;


/******************************************************
 * ÁLLAPOT
 ******************************************************/

static bool httpInitialized = false;


/******************************************************
 * COUNTER ELLENŐRZÉS
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
 * JSON UINT32
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
 * JSON UINT8
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
 * WATER COMMAND FELDOLGOZÁS
 ******************************************************/

static void handleWaterCommand()
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


    uint32_t counter = 0;

    uint8_t type = 0;


    if(!jsonGetUint32(
        body,
        "counter",
        counter
    ))
    {
        waterServer.send(
            400,
            "text/plain",
            "Invalid counter"
        );

        return;
    }


    if(!jsonGetUint8(
        body,
        "type",
        type
    ))
    {
        waterServer.send(
            400,
            "text/plain",
            "Invalid type"
        );

        return;
    }


    /**************************************************
     * COUNTER ELLENŐRZÉS
     **************************************************/

    if(
        haveCommandCounter &&
        !isCounterNewer(
            counter,
            lastCommandCounter
        )
    )
    {
        Serial.print(
            "WaterCommand DUPLIKALT/REGI: counter="
        );

        Serial.println(
            counter
        );


        /*
         * 200:
         *
         * A HTTP csomag megérkezett,
         * csak már feldolgoztuk.
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

    lastCommandCounter =
        counter;

    haveCommandCounter =
        true;


    /**************************************************
     * PARANCS ÖSSZEÁLLÍTÁSA
     **************************************************/

    WaterCommand command;

    command.counter = counter;
    command.type = type;


    /**************************************************
     * ÁTVÉTEL
     **************************************************/

    noInterrupts();

    lastWaterCommand = command;

    waterCommandReceived = true;

    interrupts();


    lastIrrigationCommandMillis =
        millis();


    Serial.print(
        "WaterCommand RX: "
    );

    Serial.print(
        "counter="
    );

    Serial.print(
        command.counter
    );

    Serial.print(
        " type="
    );

    Serial.println(
        command.type
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

static void handleIrrigationPing()
{
    waterServer.send(
        200,
        "text/plain",
        "WaterMonitor OK"
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
 * HTTP INDÍTÁS
 ******************************************************/

void initWaterHttp()
{
    if(httpInitialized)
    {
        return;
    }

    Serial.println();
    Serial.println("================================");
    Serial.println("HTTP WaterMonitor inditasa");
    Serial.println("================================");

    Serial.println("HTTP: 1 - /water/command");

    waterServer.on(
        "/water/command",
        HTTP_POST,
        handleWaterCommand
    );

    Serial.println("HTTP: 2 - /water/ping");

    waterServer.on(
        "/water/ping",
        HTTP_GET,
        handleIrrigationPing
    );

    Serial.println("HTTP: 3 - onNotFound");

    waterServer.onNotFound(
        handleNotFound
    );

    Serial.println("HTTP: 4 - waterServer.begin()");

    waterServer.begin();

    Serial.println("HTTP: 5 - begin OK");

    Serial.print("WaterMonitor HTTP port: ");
    Serial.println(WATER_HTTP_PORT);

    Serial.print("Irrigation IP: ");
    Serial.println(IRRIGATION_IP);

    httpInitialized = true;

    Serial.println("HTTP WaterMonitor READY");
}

/******************************************************
 * HTTP SERVER FRISSÍTÉSE
 ******************************************************/

void updateWaterHttp()
{
    if(!httpInitialized)
    {
        return;
    }

    waterServer.handleClient();
}


/******************************************************
 * WATERMONITOR → ÖNTÖZŐ
 ******************************************************/

bool sendWaterMessage(
    uint8_t messageType,
    uint64_t totalMilliLiters
)
{
    if(
        messageType == WATER_MSG_NONE
    )
    {
        return false;
    }


    /**************************************************
     * ÜZENETSZÁMLÁLÓ
     **************************************************/

    messageCounter++;


    /**************************************************
     * JSON
     **************************************************/

    String json;

    json.reserve(120);

    json += "{\"counter\":";
    json += messageCounter;

    json += ",\"type\":";
    json += messageType;

    json += ",\"totalMilliLiters\":";
    json += String(
        (unsigned long long)totalMilliLiters
    );

    json += "}";


    /**************************************************
     * HTTP
     **************************************************/

    WiFiClient client;

    HTTPClient http;


    String url =
        String("http://") +
        IRRIGATION_IP +
        "/water/message";


    Serial.print(
        "WaterMessage TX: "
    );

    Serial.print(
        "counter="
    );

    Serial.print(
        messageCounter
    );

    Serial.print(
        " type="
    );

    Serial.print(
        messageType
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
            "WaterMessage: HTTP begin hiba"
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
            "WaterMessage OK HTTP "
        );

        Serial.println(
            httpCode
        );
    }
    else
    {
        Serial.print(
            "WaterMessage HIBA HTTP "
        );

        Serial.println(
            httpCode
        );
    }


    http.end();


    return success;
}


/******************************************************
 * VAN-E ÚJ PARANCS?
 ******************************************************/

bool waterCommandAvailable()
{
    return waterCommandReceived;
}


/******************************************************
 * PARANCS LEKÉRÉSE
 ******************************************************/

WaterCommand getWaterCommand()
{
    WaterCommand command;

    noInterrupts();

    command = lastWaterCommand;

    interrupts();

    return command;
}


/******************************************************
 * PARANCS FELDOLGOZVA
 ******************************************************/

void clearWaterCommand()
{
    noInterrupts();

    waterCommandReceived = false;

    interrupts();
}


/******************************************************
 * ÖNTÖZŐ ONLINE?
 ******************************************************/

bool isIrrigationOnline()
{
    if(lastIrrigationCommandMillis == 0)
    {
        return false;
    }

    return (
        millis() -
        lastIrrigationCommandMillis
        <= IRRIGATION_TIMEOUT_MS
    );
}


/******************************************************
 * UTOLSÓ PARANCS
 ******************************************************/

uint32_t getIrrigationLastSeen()
{
    return lastIrrigationCommandMillis;
}
