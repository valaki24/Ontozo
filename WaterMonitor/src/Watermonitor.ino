#include "config.h"

#include <WiFi.h>
#include "water_http.h"
#include "storage.h"
#include "data.h"
#include "sensors.h"
#include "flow.h"
#include "console.h"
#include "pump.h"

#define BLYNK_TEMPLATE_ID "TMPLkuf78AEY"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_FIRMWARE_VERSION "1.4.2"

#define BLYNK_PRINT Serial
#include "BlynkEdgent.h"


// ======================================================
// WATERMONITOR ÖNTÖZÉS VEZÉRLÉS
// ======================================================

static bool irrigationRunning = false;
static bool automaticZoneMode = false;

static bool waitingForZoneAck = false;

static uint64_t zoneStartTotalMilliLiters = 0;
static uint64_t zoneChangeSentTotalMilliLiters = 0;


// V5 = liter / automata zóna
// Megengedett tartomány: 600 - 2000 liter
static uint16_t zoneVolumeLiter = 700;


// ======================================================
// V5 - AUTOMATA ZÓNA LITER
// ======================================================

BLYNK_WRITE(V5)
{
    int value = param.asInt();

    if (value < 600)
        value = 600;

    if (value > 2000)
        value = 2000;

    zoneVolumeLiter = (uint16_t)value;

    Serial.printf(
        "Automata zona V5: %u L\n",
        zoneVolumeLiter
    );
}


// ======================================================
// WATER COMMAND KEZELÉS
// ======================================================

void handleWaterCommands()
{
    if (!waterCommandAvailable())
        return;

    WaterCommand command = getWaterCommand();

    clearWaterCommand();

    Serial.println();
    Serial.println("=================================");
    Serial.println("WATER COMMAND");
    Serial.print("Counter: ");
    Serial.println(command.counter);

    Serial.print("Type: ");
    Serial.println(command.type);

    Serial.println("=================================");


    // --------------------------------------------------
    // START AUTOMATA
    // --------------------------------------------------

    if (command.type == WATER_CMD_STARTA)
    {
        Serial.println("WaterMonitor: STARTA");

        uint64_t currentTotal =
            water.totalMilliLiters;

        bool sent = sendWaterMessage(
            WATER_MSG_START_RESPONSE,
            currentTotal
        );

        if (!sent)
        {
            Serial.println(
                "STARTA: START_RESPONSE kuldese sikertelen."
            );

            return;
        }

        irrigationRunning = true;
        automaticZoneMode = true;

        waitingForZoneAck = false;

        zoneStartTotalMilliLiters =
            currentTotal;

        zoneChangeSentTotalMilliLiters = 0;

        Serial.printf(
            "Automata ontozes START: %.1f L\n",
            currentTotal / 1000.0f
        );

        Serial.printf(
            "V5: %u L\n",
            zoneVolumeLiter
        );

        return;
    }


    // --------------------------------------------------
    // START MANUAL
    // --------------------------------------------------

    if (command.type == WATER_CMD_STARTM)
    {
        Serial.println("WaterMonitor: STARTM");

        uint64_t currentTotal =
            water.totalMilliLiters;

        bool sent = sendWaterMessage(
            WATER_MSG_START_RESPONSE,
            currentTotal
        );

        if (!sent)
        {
            Serial.println(
                "STARTM: START_RESPONSE kuldese sikertelen."
            );

            return;
        }

        irrigationRunning = true;
        automaticZoneMode = false;

        waitingForZoneAck = false;

        zoneStartTotalMilliLiters = 0;
        zoneChangeSentTotalMilliLiters = 0;

        Serial.printf(
            "Manualis ontozes START: %.1f L\n",
            currentTotal / 1000.0f
        );

        return;
    }


    // --------------------------------------------------
    // STOP
    // --------------------------------------------------

    if (command.type == WATER_CMD_STOP)
    {
        Serial.println("WaterMonitor: STOP");

        uint64_t currentTotal =
            water.totalMilliLiters;

        bool sent = sendWaterMessage(
            WATER_MSG_STOP_RESPONSE,
            currentTotal
        );

        if (!sent)
        {
            Serial.println(
                "STOP_RESPONSE kuldese sikertelen."
            );

            return;
        }

        irrigationRunning = false;
        automaticZoneMode = false;

        waitingForZoneAck = false;

        zoneStartTotalMilliLiters = 0;
        zoneChangeSentTotalMilliLiters = 0;

        Serial.printf(
            "Ontozes STOP: %.1f L\n",
            currentTotal / 1000.0f
        );

        return;
    }


    // --------------------------------------------------
    // ÖSSZLITER LEKÉRÉS
    // --------------------------------------------------

    if (command.type == WATER_CMD_TOTAL_LITER_REQUEST)
    {
        Serial.println(
            "WaterMonitor: TOTAL_LITER_REQUEST"
        );

        uint64_t currentTotal =
            water.totalMilliLiters;

        bool sent = sendWaterMessage(
            WATER_MSG_TOTAL_LITER_RESPONSE,
            currentTotal
        );

        if (!sent)
        {
            Serial.println(
                "TOTAL_LITER_RESPONSE kuldese sikertelen."
            );
        }

        return;
    }


    // --------------------------------------------------
    // ACK - ELŐZŐ ZÓNAVÁLTÁS ELFOGADÁSA
    // --------------------------------------------------

    if (command.type == WATER_CMD_ACK)
    {
        Serial.println("WaterMonitor: ACK");

        if (!waitingForZoneAck)
        {
            Serial.println(
                "ACK: nincs varakozo ZONE_CHANGE."
            );

            return;
        }

        // Fontos:
        // nem az Öntöző küldi vissza a litert.
        // A WaterMonitor saját maga tudja,
        // hogy milyen total mellett küldte a ZONE_CHANGE-et.
        zoneStartTotalMilliLiters =
            zoneChangeSentTotalMilliLiters;

        waitingForZoneAck = false;

        Serial.printf(
            "ZONE_CHANGE ACK OK, uj zona kezdet: %.1f L\n",
            zoneStartTotalMilliLiters / 1000.0f
        );

        return;
    }


    // --------------------------------------------------
    // ISMERETLEN PARANCS
    // --------------------------------------------------

    Serial.println(
        "WaterMonitor: ismeretlen command type."
    );
}


// ======================================================
// AUTOMATA ZÓNA VÁLTÁS
// ======================================================

void handleAutomaticZoneChange()
{
    if (!irrigationRunning)
        return;

    if (!automaticZoneMode)
        return;

    if (waitingForZoneAck)
        return;

    uint64_t requiredMilliLiters =
        (uint64_t)zoneVolumeLiter * 1000ULL;

    uint64_t currentTotal =
        water.totalMilliLiters;

    uint64_t targetTotal =
        zoneStartTotalMilliLiters +
        requiredMilliLiters;

    if (currentTotal < targetTotal)
        return;


    // Elérte az adott zónára beállított litert.
    Serial.println();
    Serial.println("*********************************");
    Serial.println("AUTOMATA ZONA VALTAS");
    Serial.printf(
        "Zona kezdet: %.1f L\n",
        zoneStartTotalMilliLiters / 1000.0f
    );
    Serial.printf(
        "V5: %u L\n",
        zoneVolumeLiter
    );
    Serial.printf(
        "Aktualis total: %.1f L\n",
        currentTotal / 1000.0f
    );
    Serial.println("*********************************");


    // Ezt a konkrét total értéket küldjük az Öntözőnek.
    bool sent = sendWaterMessage(
        WATER_MSG_ZONE_CHANGE,
        currentTotal
    );

    if (!sent)
    {
        Serial.println(
            "ZONE_CHANGE kuldese sikertelen - kesobb ujraprobaljuk."
        );

        return;
    }


    // Megjegyezzük pontosan azt a total értéket,
    // amelyet elküldtünk.
    zoneChangeSentTotalMilliLiters =
        currentTotal;

    waitingForZoneAck = true;

    Serial.printf(
        "ZONE_CHANGE elkuldve: %.1f L\n",
        zoneChangeSentTotalMilliLiters / 1000.0f
    );

    Serial.println(
        "Varakozas az Ontozo ACK-jara..."
    );
}


// ======================================================
// TASKOK
// ======================================================

void taskReadSensors()
{
    readSensors();
    water.packetCounter++;
}

void taskFlow()
{
    updateFlow();
}


// ======================================================
// DEBUG
// ======================================================

void taskDebug()
{
    Serial.println("--------------------------------");

    Serial.printf(
        "Pressure IN : %.2f bar\n",
        water.pressureInBar
    );

    Serial.printf(
        "Pressure OUT: %.2f bar\n",
        water.pressureOutBar
    );

    Serial.printf(
        "Delta P     : %.2f bar\n",
        water.deltaPressureBar
    );

    Serial.printf(
        "Flow        : %.2f L/min\n",
        water.flowLmin
    );

    Serial.printf(
        "Flow        : %.3f m3/h\n",
        water.flowM3h
    );

    Serial.printf(
        "Total       : %.1f L\n",
        water.totalMilliLiters / 1000.0f
    );

    Serial.printf(
        "Irrigation  : %s\n",
        irrigationRunning ? "RUNNING" : "STOP"
    );

    Serial.printf(
        "Auto zone   : %s\n",
        automaticZoneMode ? "YES" : "NO"
    );

    Serial.printf(
        "V5          : %u L\n",
        zoneVolumeLiter
    );

    if (automaticZoneMode)
    {
        Serial.printf(
            "Zone start  : %.1f L\n",
            zoneStartTotalMilliLiters / 1000.0f
        );
    }

    Serial.printf(
        "Zone ACK    : %s\n",
        waitingForZoneAck ? "WAIT" : "READY"
    );

    Serial.println("--------------------------------");
}


// ======================================================
// BLYNK
// ======================================================

BLYNK_WRITE(V10)
{
    float minPressure = param.asFloat();

    setPumpMinPressure(minPressure);

    Serial.printf(
        "Pump MIN nyomas: %.2f bar\n",
        minPressure
    );
}


BLYNK_WRITE(V11)
{
    float maxPressure = param.asFloat();

    setPumpMaxPressure(maxPressure);

    Serial.printf(
        "Pump MAX nyomas: %.2f bar\n",
        maxPressure
    );
}

// ======================================================
// BLYNK MÉRÉSI ADATOK
// ======================================================

void updateBlynkMeasurements()
{
    Blynk.virtualWrite(
        V1,
        water.pressureInBar
    );

    Blynk.virtualWrite(
        V3,
        water.pressureOutBar
    );

    Blynk.virtualWrite(
        V7,
        water.deltaPressureBar
    );

    Blynk.virtualWrite(
        V8,
        water.flowLmin
    );

    Blynk.virtualWrite(
        V9,
        water.totalMilliLiters / 1000.0f
    );
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("WaterMonitor START");
    Serial.println("=================================");

    Serial.println("1. initPump()");
    initPump();

    Serial.println("2. initStorage()");
    initStorage();

    Serial.println("3. loadCalibration()");
    loadCalibration();

    Serial.println("4. loadTotals()");
    loadTotals();

    Serial.println("5. initSensors()");
    initSensors();

    Serial.println("6. initFlow()");
    initFlow();

    Serial.println("7. BlynkEdgent.begin()");
    BlynkEdgent.begin();

    Serial.println("8. BlynkEdgent.begin() KESZ");

    Serial.println("=================================");
    Serial.println("SETUP KESZ - LOOP INDUL");
    Serial.println("=================================");
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
    static uint32_t lastSensors = 0;
    static uint32_t lastFlow = 0;
    static uint32_t lastBlynkUpdate = 0;

    static bool waterHttpStarted = false;

    const uint32_t now = millis();


    // ------------------------------------------
    // BlynkEdgent
    // ------------------------------------------

    BlynkEdgent.run();


    // ------------------------------------------
    // WiFi / HTTP
    // ------------------------------------------

    if (!waterHttpStarted &&
        WiFi.status() == WL_CONNECTED)
    {
        initWaterHttp();

        waterHttpStarted = true;

        Serial.println(
            "HTTP WaterMonitor ELINDULT"
        );
    }


    // ------------------------------------------
    // HTTP szerver
    // ------------------------------------------

    updateWaterHttp();


    // ------------------------------------------
    // WaterMonitor parancsok
    // ------------------------------------------

    handleWaterCommands();


    // ------------------------------------------
    // Szenzorok
    // ------------------------------------------

    if (now - lastSensors >= 200)
    {
        lastSensors = now;

        taskReadSensors();

        updatePump(
            water.pressureInBar
        );
    }


    // ------------------------------------------
    // Flow
    // ------------------------------------------

    if (now - lastFlow >= FLOW_INTERVAL_MS)
    {
        lastFlow = now;

        taskFlow();
    }


    // ------------------------------------------
    // Blynk mérési adatok
    // ------------------------------------------

    if (now - lastBlynkUpdate >= 2000)
    {
        lastBlynkUpdate = now;

        if (Blynk.connected())
        {
            updateBlynkMeasurements();
        }
    }


    // ------------------------------------------
    // Automata zónaváltás
    // ------------------------------------------

    handleAutomaticZoneChange();


    // ------------------------------------------
    // Konzol
    // ------------------------------------------

    handleConsole();
    updatePumpTest();
}
