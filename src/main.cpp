#define BLYNK_TEMPLATE_ID "TMPLkuf78AEY"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_FIRMWARE_VERSION "1.7.5"

#define BLYNK_PRINT Serial

#include "BlynkEdgent.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "packet.h"
#include "water_receiver.h"
#include "logger.h"
#include "time_manager.h"
#include "csv_server.h"
#include "blynk_terminal.h"


/******************************************************
 * RELÉK
 *
 * Zóna 1 = GPIO16
 * Zóna 2 = GPIO14
 * Zóna 3 = GPIO12
 * Zóna 4 = GPIO13
 * Zóna 5 = GPIO15
 * Zóna 6 = GPIO0
 * Zóna 7 = GPIO4
 *
 * Tó töltés = GPIO5
 ******************************************************/

#define RELAY_COUNT 8
#define ZONE_COUNT 7
#define POND_RELAY_INDEX 7

int relayPins[RELAY_COUNT] =
{
    16,     // Zóna 1
    14,     // Zóna 2
    12,     // Zóna 3
    13,     // Zóna 4
    15,     // Zóna 5
    0,      // Zóna 6
    4,      // Zóna 7
    5       // Tó töltés
};

bool activeLow[RELAY_COUNT] =
{
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false
};


/******************************************************
 * WATERMONITOR PARANCSOK
 ******************************************************/

bool sendWaterMonitorAutoStart()
{
    Serial.println(
        "WaterMonitor STARTA kuldese..."
    );

    return sendWaterCommand(
        WATER_CMD_STARTA
    );
}


bool sendWaterMonitorManualStart()
{
    Serial.println(
        "WaterMonitor STARTM kuldese..."
    );

    return sendWaterCommand(
        WATER_CMD_STARTM
    );
}


bool sendWaterMonitorStop()
{
    Serial.println(
        "WaterMonitor STOP kuldese..."
    );

    return sendWaterCommand(
        WATER_CMD_STOP
    );
}


bool sendWaterMonitorTotalLiterRequest()
{
    Serial.println(
        "WaterMonitor osszliter lekerese..."
    );

    return sendWaterCommand(
        WATER_CMD_TOTAL_LITER_REQUEST
    );
}


bool sendWaterMonitorAck()
{
    Serial.println(
        "WaterMonitor ACK kuldese..."
    );

    return sendWaterCommand(
        WATER_CMD_ACK
    );
}

/******************************************************
 * ÖNTÖZÉSI ÁLLAPOT
 ******************************************************/

bool autoMode = false;

int currentRelay = 1;


/******************************************************
 * WATERMONITOR ÁLLAPOT
 ******************************************************/

float previousWaterTotalLiter = 0.0f;
float currentWaterTotalLiter = 0.0f;

bool waterStartValueReceived = false;


/******************************************************
 * WATERMONITOR ÁLLAPOTGÉP
 ******************************************************/

enum WaterMonitorState
{
    WATER_IDLE = 0,

    WATER_WAIT_START,

    WATER_RUNNING,

    WATER_WAIT_STOP
};

WaterMonitorState waterMonitorState =
    WATER_IDLE;


/******************************************************
 * TÓ TÖLTÉS ÁLLAPOT
 ******************************************************/

enum PondState
{
    POND_IDLE = 0,

    POND_WAIT_START_TOTAL,

    POND_RUNNING,

    POND_WAIT_STOP_TOTAL
};

PondState pondState =
    POND_IDLE;


/******************************************************
 * KÉZI ZÓNA ÁLLAPOT
 ******************************************************/

enum ManualWaterState
{
    MANUAL_IDLE = 0,

    MANUAL_WAIT_START,

    MANUAL_RUNNING,

    MANUAL_WAIT_ZONE_CHANGE_TOTAL
};

ManualWaterState manualWaterState =
    MANUAL_IDLE;

int pendingManualZone = 0;

bool manualWaterRunning = false;


/******************************************************
 * MŰVELETI ÁLLAPOT
 ******************************************************/

String lastOperationStatus = "";


/******************************************************
 * RELÉ KEZELÉS
 ******************************************************/

void setRelay(
    int index,
    bool on
)
{
    if (
        index < 0 ||
        index >= RELAY_COUNT
    )
    {
        return;
    }

    const bool pinHigh =
        activeLow[index]
            ? !on
            : on;

    digitalWrite(
        relayPins[index],
        pinHigh ? HIGH : LOW
    );
}


bool isRelayOn(
    int index
)
{
    if (
        index < 0 ||
        index >= RELAY_COUNT
    )
    {
        return false;
    }

    const int pinState =
        digitalRead(
            relayPins[index]
        );

    if (activeLow[index])
    {
        return pinState == LOW;
    }

    return pinState == HIGH;
}


void setupRelays()
{
    for (
        int i = 0;
        i < RELAY_COUNT;
        i++
    )
    {
        pinMode(
            relayPins[i],
            OUTPUT
        );

        setRelay(
            i,
            false
        );
    }
}



void refreshOperationStatus();
void terminalRelayCommand(
    const String& command
)
{
    String value = command;
    value.trim();


    /**************************************************
     * MINDEN RELÉ KI
     **************************************************/

    if(value.equalsIgnoreCase("off"))
    {
        for(int i = 0; i < RELAY_COUNT; i++)
        {
            setRelay(i, false);
        }

        refreshOperationStatus();

        terminalPrint(
            "Minden rele OFF"
        );

        return;
    }


    /**************************************************
     * RELÉK FELDOLGOZÁSA
     **************************************************/

    bool relaySelected[RELAY_COUNT] = {
        false
    };

    int start = 0;

    while(start < value.length())
    {
        int plusPos =
            value.indexOf('+', start);

        String number;

        if(plusPos == -1)
        {
            number =
                value.substring(start);

            start =
                value.length();
        }
        else
        {
            number =
                value.substring(
                    start,
                    plusPos
                );

            start =
                plusPos + 1;
        }

        number.trim();

        if(number.length() == 0)
        {
            terminalPrint(
                "Hibas rele parancs."
            );

            return;
        }


        int relay =
            number.toInt();


        if(
            relay < 1 ||
            relay > RELAY_COUNT
        )
        {
            terminalPrint(
                "Hibas rele. Ervenyes: 1-8"
            );

            return;
        }


        relaySelected[
            relay - 1
        ] = true;
    }


    /**************************************************
     * RELÉK BEÁLLÍTÁSA
     **************************************************/

    for(int i = 0; i < RELAY_COUNT; i++)
    {
        setRelay(
            i,
            relaySelected[i]
        );
    }


    refreshOperationStatus();


    /**************************************************
     * VISSZAJELZÉS
     **************************************************/

    String result =
        "Relek ON: ";

    bool first = true;

    for(int i = 0; i < RELAY_COUNT; i++)
    {
        if(relaySelected[i])
        {
            if(!first)
                result += "+";

            result +=
                String(i + 1);

            first = false;
        }
    }

    terminalPrint(result);
}

/******************************************************
 * AKTÍV ZÓNA
 ******************************************************/

int getActiveZone()
{
    for (
        int i = 0;
        i < ZONE_COUNT;
        i++
    )
    {
        if (isRelayOn(i))
        {
            return i + 1;
        }
    }

    return 0;
}


/******************************************************
 * BLYNK MŰVELETI ÁLLAPOT
 ******************************************************/

void refreshOperationStatus()
{
    String newStatus;

    if (
        isRelayOn(
            POND_RELAY_INDEX
        )
    )
    {
        newStatus =
            "Tó töltés";
    }
    else
    {
        const int activeZone =
            getActiveZone();

        if (activeZone > 0)
        {
            newStatus =
                "Zóna " +
                String(activeZone);
        }
        else
        {
            newStatus =
                "Kikapcsolva";
        }
    }

    if (
        newStatus !=
        lastOperationStatus
    )
    {
        lastOperationStatus =
            newStatus;

        Blynk.virtualWrite(
            V2,
            newStatus
        );
    }
}


/******************************************************
 * ZÓNÁK KIKAPCSOLÁSA
 ******************************************************/

void turnOffZones()
{
    for (
        int i = 0;
        i < ZONE_COUNT;
        i++
    )
    {
        setRelay(
            i,
            false
        );
    }

    refreshOperationStatus();
}


/******************************************************
 * ZÓNA KAPCSOLÁSA
 ******************************************************/

void switchRelay(
    int zone
)
{
    if (
        zone < 1 ||
        zone > ZONE_COUNT
    )
    {
        return;
    }

    /*
     * Tó töltés kikapcsolása,
     * ha zónát kapcsolunk.
     */

    setRelay(
        POND_RELAY_INDEX,
        false
    );

    for (
        int i = 0;
        i < ZONE_COUNT;
        i++
    )
    {
        setRelay(
            i,
            i == zone - 1
        );
    }

    currentRelay =
        zone;

    refreshOperationStatus();
}


/******************************************************
 * WATERMONITOR START
 ******************************************************/

void startWaterMonitor()
{
    waterMonitorState =
        WATER_WAIT_START;

    if (!sendWaterMonitorAutoStart())
    {
        waterMonitorState =
            WATER_IDLE;

        Serial.println(
            "WaterMonitor STARTA sikertelen."
        );
    }
}

/******************************************************
 * WATERMONITOR STOP
 ******************************************************/

void stopWaterMonitor()
{
    waterMonitorState =
        WATER_WAIT_STOP;

    if (!sendWaterMonitorStop())
    {
        waterMonitorState =
            WATER_IDLE;

        Serial.println(
            "WaterMonitor STOP sikertelen."
        );
    }
}


/******************************************************
 * WATERMONITOR VÁLASZ FELDOLGOZÁSA
 ******************************************************/

void handleWaterMonitor()
{
    if (
        !waterMonitorResponseAvailable()
    )
    {
        return;
    }

    WaterMessage message =
        getWaterMonitorResponse();

    clearWaterMonitorResponse();


    const float totalLiter =
        waterTotalLiter(
            message
        );


    /**************************************************
     * ÖSSZLITER VÁLASZ
     **************************************************/

    if (
        message.type ==
        WATER_MSG_TOTAL_LITER_RESPONSE
    )
    {
        currentWaterTotalLiter =
            totalLiter;

        Serial.print(
            "WaterMonitor osszliter: "
        );

        Serial.print(
            currentWaterTotalLiter,
            2
        );

        Serial.println(
            " L"
        );


        /************************************************
         * TÓ TÖLTÉS INDÍTÁSA
         ************************************************/

        if (
            pondState ==
            POND_WAIT_START_TOTAL
        )
        {
            pondState =
                POND_RUNNING;

            loggerStartPond(
                currentWaterTotalLiter
            );

            setRelay(
                POND_RELAY_INDEX,
                true
            );

            refreshOperationStatus();

            Serial.println(
                "Tó töltés elindult."
            );

            return;
        }


        /************************************************
         * TÓ TÖLTÉS LEÁLLÍTÁSA
         ************************************************/

        if (
            pondState ==
            POND_WAIT_STOP_TOTAL
        )
        {
            loggerFinishPond(
                currentWaterTotalLiter
            );

            setRelay(
                POND_RELAY_INDEX,
                false
            );

            pondState =
                POND_IDLE;

            refreshOperationStatus();

            Serial.println(
                "Tó töltés leállt."
            );

            return;
        }


        /************************************************
         * KÉZI ZÓNAVÁLTÁS
         ************************************************/

        if (
            manualWaterState ==
            MANUAL_WAIT_ZONE_CHANGE_TOTAL
        )
        {
            /*
             * currentWaterTotalLiter már
             * tartalmazza a most kapott
             * pontos WaterMonitor értéket.
             */

            Serial.print(
                "Kézi zóna "
            );

            Serial.print(
                currentRelay
            );

            Serial.println(
                " lezárva."
            );


            /*
             * Régi zóna lezárása.
             */

            loggerFinishZone(
                currentWaterTotalLiter
            );


            /*
             * Új zóna.
             */

            currentRelay =
                pendingManualZone;


            /*
             * Új zóna logger indul.
             *
             * A kezdőérték ugyanaz az
             * összliter, amelynél a váltás történt.
             */

            loggerStartZone(
                currentRelay,
                currentWaterTotalLiter
            );


            switchRelay(
                currentRelay
            );


            manualWaterState =
                MANUAL_RUNNING;


            Serial.print(
                "Kézi zóna "
            );

            Serial.print(
                currentRelay
            );

            Serial.println(
                " elindult."
            );

            return;
        }

        return;
    }


    /**************************************************
     * START VÁLASZ
     **************************************************/

    if (
        message.type ==
        WATER_MSG_START_RESPONSE
    )
    {
        currentWaterTotalLiter =
            totalLiter;

        previousWaterTotalLiter =
            currentWaterTotalLiter;

        waterStartValueReceived =
            true;

        waterMonitorState =
            WATER_RUNNING;


        Serial.print(
            "WaterMonitor indulasi osszliter: "
        );

        Serial.print(
            currentWaterTotalLiter,
            2
        );

        Serial.println(
            " L"
        );


        /*
         * Kézi indítás:
         *
         * currentRelay már tartalmazza
         * a felhasználó által kiválasztott zónát.
         */

        if (manualWaterRunning)
        {
            manualWaterState =
                MANUAL_RUNNING;
        }
        else
        {
            /*
             * Automata indítás:
             * mindig zóna 1.
             */

            currentRelay =
                1;
        }


        loggerStartZone(
            currentRelay,
            currentWaterTotalLiter
        );


        switchRelay(
            currentRelay
        );

        return;
    }


    /**************************************************
     * ZÓNA VÁLTÁS
     **************************************************/

    if (
        message.type ==
        WATER_MSG_ZONE_CHANGE
    )
    {
        if (
            waterMonitorState !=
            WATER_RUNNING
        )
        {
            Serial.println(
                "ZONE_CHANGE figyelmen kivul hagyva."
            );

            return;
        }


        previousWaterTotalLiter =
            currentWaterTotalLiter;

        currentWaterTotalLiter =
            totalLiter;


        /*
         * Előző zóna lezárása.
         */

        loggerFinishZone(
            currentWaterTotalLiter
        );


        Serial.print(
            "Zóna "
        );

        Serial.print(
            currentRelay
        );

        Serial.println(
            " lezárva."
        );


        /*
         * Következő zóna.
         */

        int nextZone =
            currentRelay + 1;

        if (
            nextZone > ZONE_COUNT
        )
        {
            nextZone = 1;
        }

        currentRelay =
            nextZone;


        /*
         * Következő zóna logger indul.
         */

        loggerStartZone(
            currentRelay,
            currentWaterTotalLiter
        );


        switchRelay(
            currentRelay
        );


        /*
         * WaterMonitor csak ACK után
         * kezdi újra a mérési periódust.
         */

        sendWaterMonitorAck();

        return;
    }


    /**************************************************
     * STOP VÁLASZ
     **************************************************/

    if (
        message.type ==
        WATER_MSG_STOP_RESPONSE
    )
    {
        currentWaterTotalLiter =
            totalLiter;


        /*
         * Az utolsó zóna lezárása.
         */

        if (
            waterStartValueReceived
        )
        {
            loggerFinishZone(
                currentWaterTotalLiter
            );
        }


        Serial.print(
            "Öntözés lezárva. "
        );

        Serial.print(
            currentWaterTotalLiter,
            2
        );

        Serial.println(
            " L"
        );


        turnOffZones();


        waterMonitorState =
            WATER_IDLE;

        waterStartValueReceived =
            false;

        manualWaterRunning =
            false;

        manualWaterState =
            MANUAL_IDLE;

        pendingManualZone =
            0;

        return;
    }


    /**************************************************
     * HEATPUMP START
     **************************************************/

    if (
        message.type ==
        WATER_MSG_HEATPUMP_START
    )
    {
        currentWaterTotalLiter =
            totalLiter;

        loggerStartHeatpump(
            currentWaterTotalLiter
        );

        Serial.print(
            "HEATPUMP START: "
        );

        Serial.print(
            currentWaterTotalLiter,
            2
        );

        Serial.println(
            " L"
        );

        return;
    }


    /**************************************************
     * HEATPUMP STOP
     **************************************************/

    if (
        message.type ==
        WATER_MSG_HEATPUMP_STOP
    )
    {
        currentWaterTotalLiter =
            totalLiter;

        loggerFinishHeatpump(
            currentWaterTotalLiter
        );

        Serial.print(
            "HEATPUMP STOP: "
        );

        Serial.print(
            currentWaterTotalLiter,
            2
        );

        Serial.println(
            " L"
        );

        return;
    }
}


/******************************************************
 * BLYNK V0
 *
 * TÓ TÖLTÉS
 ******************************************************/

BLYNK_WRITE(V0)
{
    const bool state =
        param.asInt() != 0;


    /*
     * INDÍTÁS
     */

    if (state)
    {
        if (
            pondState !=
            POND_IDLE
        )
        {
            return;
        }


        /*
         * Zónák kikapcsolása.
         */

        turnOffZones();


        pondState =
            POND_WAIT_START_TOTAL;


        /*
         * Aktuális összliter lekérése.
         */

        if (
            !sendWaterMonitorTotalLiterRequest()
        )
        {
            pondState =
                POND_IDLE;

            Serial.println(
                "Tó töltés indítása sikertelen."
            );
        }

        return;
    }


    /*
     * LEÁLLÍTÁS
     */

    if (
        pondState ==
        POND_RUNNING
    )
    {
        pondState =
            POND_WAIT_STOP_TOTAL;


        if (
            !sendWaterMonitorTotalLiterRequest()
        )
        {
            pondState =
                POND_RUNNING;

            Serial.println(
                "Tó töltés leállítása sikertelen."
            );
        }

        return;
    }


    /*
     * Ha még csak várakozott az indításra,
     * töröljük a várakozást.
     */

    if (
        pondState ==
        POND_WAIT_START_TOTAL
    )
    {
        pondState =
            POND_IDLE;

        setRelay(
            POND_RELAY_INDEX,
            false
        );

        refreshOperationStatus();

        return;
    }
}


/******************************************************
 * BLYNK V4
 *
 * AUTOMATA ÖNTÖZÉS
 ******************************************************/

BLYNK_WRITE(V4)
{
    const bool state =
        param.asInt() != 0;


    /**************************************************
     * INDÍTÁS
     **************************************************/

    if (
        state &&
        !autoMode
    )
    {
        /*
         * Tó töltés alatt ne induljon öntözés.
         */

        if (
            pondState !=
            POND_IDLE
        )
        {
            Serial.println(
                "Öntözés nem indítható: tó töltés aktív."
            );

            return;
        }


        autoMode =
            true;

        waterStartValueReceived =
            false;

        currentRelay =
            1;

        startWaterMonitor();

        return;
    }


    /**************************************************
     * LEÁLLÍTÁS
     **************************************************/

    if (
        !state &&
        autoMode
    )
    {
        autoMode =
            false;


        /*
         * Ha már fut az öntözés,
         * STOP -> WaterMonitor elküldi
         * az aktuális összlitert.
         */

        if (
            waterMonitorState ==
            WATER_RUNNING
        )
        {
            stopWaterMonitor();
        }
        else
        {
            turnOffZones();

            waterMonitorState =
                WATER_IDLE;

            waterStartValueReceived =
                false;
        }

        return;
    }
}


/******************************************************
 * BLYNK V6
 *
 * KÉZI ZÓNA
 *
 * 0 = kikapcsolás
 * 1-7 = zóna
 ******************************************************/

BLYNK_WRITE(V6)
{
    /*
     * Automata módban a kézi zónaválasztás
     * nem engedélyezett.
     */

    if (autoMode)
    {
        return;
    }


    const int value =
        param.asInt();


    /**************************************************
     * KIKAPCSOLÁS
     **************************************************/

    if (value == 0)
    {
        turnOffZones();


        if (manualWaterRunning)
        {
            manualWaterRunning =
                false;

            manualWaterState =
                MANUAL_IDLE;

            pendingManualZone =
                0;


            /*
             * WaterMonitor STOP.
             */

            if (
                waterMonitorState ==
                WATER_RUNNING
            )
            {
                stopWaterMonitor();
            }
            else
            {
                waterMonitorState =
                    WATER_IDLE;
            }
        }

        return;
    }


    /**************************************************
     * ÉRVÉNYES KÉZI ZÓNA
     **************************************************/

    if (
        value < 1 ||
        value > ZONE_COUNT
    )
    {
        return;
    }


    /**************************************************
     * ELSŐ KÉZI INDÍTÁS
     **************************************************/

    if (!manualWaterRunning)
    {
        manualWaterRunning =
            true;

        manualWaterState =
            MANUAL_WAIT_START;

        waterMonitorState =
            WATER_WAIT_START;

        currentRelay =
            value;


        Serial.print(
            "Kézi öntözés indítása, zóna: "
        );

        Serial.println(
            currentRelay
        );


        if (
            !sendWaterMonitorManualStart()
        )
        {
            manualWaterRunning =
                false;

            manualWaterState =
                MANUAL_IDLE;

            waterMonitorState =
                WATER_IDLE;

            return;
        }

        return;
    }


    /**************************************************
     * UGYANAZ A ZÓNA
     **************************************************/

    if (
        value == currentRelay
    )
    {
        return;
    }


    /**************************************************
     * KÉZI ZÓNAVÁLTÁS
     **************************************************/

    pendingManualZone =
        value;

    manualWaterState =
        MANUAL_WAIT_ZONE_CHANGE_TOTAL;


    Serial.print(
        "Kézi zónaváltás kérése: "
    );

    Serial.print(
        currentRelay
    );

    Serial.print(
        " -> "
    );

    Serial.println(
        pendingManualZone
    );


    /*
     * Előbb lekérjük a WaterMonitor aktuális
     * összliter értékét.
     */

    if (
        !sendWaterMonitorTotalLiterRequest()
    )
    {
        manualWaterState =
            MANUAL_RUNNING;

        Serial.println(
            "Kézi zónaváltás: összliter lekérés sikertelen."
        );

        return;
    }
}


/******************************************************
 * BLYNK CONNECTED
 ******************************************************/

BLYNK_CONNECTED()
{
    lastOperationStatus = "";

    refreshOperationStatus();
}


/******************************************************
 * TERMINAL
 ******************************************************/

void terminalBlynkOutput(
    const String& text
)
{
    Blynk.virtualWrite(
        V12,
        text
    );
}


BLYNK_WRITE(V12)
{
    String cmd =
        param.asStr();

    cmd.trim();

    if (
        cmd.length() == 0
    )
    {
        return;
    }

    Blynk.virtualWrite(
        V12,
        "> " + cmd
    );

    terminalCommand(
        cmd
    );
}


/******************************************************
 * TERMINAL KIMENET
 ******************************************************/

void terminalBlynkPrint(
    const String& text
)
{
    Blynk.virtualWrite(
        V12,
        text
    );
}


/******************************************************
 * SETUP
 ******************************************************/

void setup()
{
    Serial.begin(
        115200
    );

    delay(100);


    /**************************************************
     * RELÉK
     **************************************************/

    setupRelays();


    /**************************************************
     * BLYNK
     **************************************************/

    BlynkEdgent.begin();


    /**************************************************
     * TIME MANAGER
     **************************************************/

    timeManagerBegin();


    /**************************************************
     * LOGGER
     **************************************************/

    loggerBegin();


    /**************************************************
     * WATERMONITOR HTTP
     **************************************************/

    initWaterReceiver();


    /**************************************************
     * CSV SERVER
     **************************************************/

    csvServerBegin();


    /**************************************************
     * TERMINAL
     **************************************************/

    terminalInit();

    terminalSetOutputCallback(
        terminalBlynkOutput
    );
}


/******************************************************
 * LOOP
 ******************************************************/

void loop()
{
    /**************************************************
     * BLYNK
     **************************************************/

    BlynkEdgent.run();


    /**************************************************
     * IDŐKEZELÉS
     **************************************************/

    timeManagerUpdate();


    /**************************************************
     * WATERMONITOR HTTP
     **************************************************/

    updateWaterReceiver();


    /**************************************************
     * WATERMONITOR ÜZENETEK
     **************************************************/

    handleWaterMonitor();


    /**************************************************
     * CSV SERVER
     **************************************************/

    csvServerRun();
}