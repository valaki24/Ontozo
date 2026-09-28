#include <Arduino.h>

#include "console.h"
#include "storage.h"
#include "data.h"
#include "pump.h"

/******************************************************
 * KONZOL
 ******************************************************/

static String cmd;


/******************************************************
 * PUMP TESZT
 ******************************************************/

static bool pumpTestActive = false;

static bool pumpTestLastState = false;

static uint32_t pumpTestStateStartMillis = 0;

static uint32_t pumpTestOnStartMillis = 0;

static uint32_t pumpTestOffStartMillis = 0;

static uint64_t pumpTestOnStartTotalMilliLiters = 0;

static uint32_t pumpTestCycleCount = 0;

static uint32_t pumpTestTotalOnMillis = 0;

static uint64_t pumpTestTotalMilliLiters = 0;

static uint32_t pumpTestTotalOffMillis = 0;


/******************************************************
 * PUMP ÁLLAPOT LEKÉRÉSE
 *
 * A pump.cpp meglévő vezérlését nem módosítjuk.
 * Itt csak a relé aktuális állapotát figyeljük.
 ******************************************************/

static bool getPumpState()
{
    return isPumpRunning();
}


/******************************************************
 * PUMP TESZT INDÍTÁSA
 ******************************************************/

static void startPumpTest()
{
    if (pumpTestActive)
    {
        Serial.println("PumpTest mar aktiv.");
        return;
    }

    pumpTestActive = true;

    pumpTestLastState =
        getPumpState();

    pumpTestStateStartMillis =
        millis();

    pumpTestOnStartMillis = 0;

    pumpTestOffStartMillis = 0;

    pumpTestOnStartTotalMilliLiters = 0;

    pumpTestCycleCount = 0;

    pumpTestTotalOnMillis = 0;

    pumpTestTotalMilliLiters = 0;

    pumpTestTotalOffMillis = 0;

    if (pumpTestLastState)
    {
        pumpTestOnStartMillis =
            millis();

        pumpTestOnStartTotalMilliLiters =
            water.totalMilliLiters;

        Serial.println();
        Serial.println("===== PUMP TESZT =====");
        Serial.println("Pump jelenleg: ON");
        Serial.println("Meras elindult.");
        Serial.println("======================");
    }
    else
    {
        pumpTestOffStartMillis =
            millis();

        Serial.println();
        Serial.println("===== PUMP TESZT =====");
        Serial.println("Pump jelenleg: OFF");
        Serial.println("Meras elindult.");
        Serial.println("======================");
    }
}


/******************************************************
 * PUMP TESZT LEÁLLÍTÁSA
 ******************************************************/

static void stopPumpTest()
{
    if (!pumpTestActive)
    {
        Serial.println("PumpTest nem aktiv.");
        return;
    }

    bool currentState =
        getPumpState();

    uint32_t now =
        millis();

    /*
     * Ha éppen ON állapotban állítjuk le,
     * az aktuális ON ciklust lezárjuk.
     */
    if (currentState)
    {
        uint32_t onTime =
            now - pumpTestOnStartMillis;

        uint64_t liters =
            water.totalMilliLiters -
            pumpTestOnStartTotalMilliLiters;

        pumpTestTotalOnMillis +=
            onTime;

        pumpTestTotalMilliLiters +=
            liters;

        Serial.println();
        Serial.println(
            "Aktualis ON ciklus lezárva:"
        );

        Serial.printf(
            "  ON ido: %.1f s\n",
            onTime / 1000.0f
        );

        Serial.printf(
            "  Viz: %.1f L\n",
            liters / 1000.0f
        );
    }
    else
    {
        /*
         * OFF állapotban csak az OFF időt
         * számoljuk, vízfogyasztást nem.
         */
        if (pumpTestOffStartMillis > 0)
        {
            pumpTestTotalOffMillis +=
                now - pumpTestOffStartMillis;
        }
    }

    pumpTestActive = false;

    Serial.println();
    Serial.println(
        "===== PUMP TESZT VEGE ====="
    );

    Serial.printf(
        "Ciklusok: %lu\n",
        (unsigned long)pumpTestCycleCount
    );

    Serial.printf(
        "Osszes ON ido: %.1f s\n",
        pumpTestTotalOnMillis / 1000.0f
    );

    Serial.printf(
        "Osszes OFF ido: %.1f s\n",
        pumpTestTotalOffMillis / 1000.0f
    );

    Serial.printf(
        "Osszes atfolyt viz: %.1f L\n",
        pumpTestTotalMilliLiters / 1000.0f
    );

    if (pumpTestCycleCount > 0)
    {
        Serial.printf(
            "Atlag ON ido: %.1f s\n",
            (pumpTestTotalOnMillis /
             1000.0f) /
            pumpTestCycleCount
        );

        Serial.printf(
            "Atlag viz/ciklus: %.1f L\n",
            (pumpTestTotalMilliLiters /
             1000.0f) /
            pumpTestCycleCount
        );
    }

    Serial.println(
        "============================"
    );
}


/******************************************************
 * PUMP TESZT FRISSÍTÉSE
 *
 * FONTOS:
 * Itt NEM kapcsoljuk a pumpát.
 * Csak figyeljük az állapotát.
 ******************************************************/

void updatePumpTest()
{
    if (!pumpTestActive)
        return;

    bool currentState =
        getPumpState();

    uint32_t now =
        millis();

    /*
     * Nincs állapotváltozás.
     */
    if (currentState == pumpTestLastState)
        return;

    /*
     * OFF -> ON
     */
    if (currentState)
    {
        uint32_t offTime =
            now - pumpTestOffStartMillis;

        pumpTestTotalOffMillis +=
            offTime;

        pumpTestOnStartMillis =
            now;

        pumpTestOnStartTotalMilliLiters =
            water.totalMilliLiters;

        pumpTestCycleCount++;

        Serial.println();
        Serial.println(
            "PUMP ON"
        );

        Serial.printf(
            "OFF ido: %.1f s\n",
            offTime / 1000.0f
        );

        Serial.printf(
            "Kezdo total: %.1f L\n",
            water.totalMilliLiters / 1000.0f
        );
    }

    /*
     * ON -> OFF
     */
    else
    {
        uint32_t onTime =
            now - pumpTestOnStartMillis;

        uint64_t liters =
            water.totalMilliLiters -
            pumpTestOnStartTotalMilliLiters;

        pumpTestTotalOnMillis +=
            onTime;

        pumpTestTotalMilliLiters +=
            liters;

        pumpTestOffStartMillis =
            now;

        Serial.println();
        Serial.println(
            "PUMP OFF"
        );

        Serial.printf(
            "ON ido: %.1f s\n",
            onTime / 1000.0f
        );

        Serial.printf(
            "Atfolyt viz: %.1f L\n",
            liters / 1000.0f
        );
    }

    pumpTestLastState =
        currentState;
}


/******************************************************
 * KONZOL
 ******************************************************/

void handleConsole()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (cmd.length() == 0)
                continue;

            cmd.trim();

            if (cmd == "status")
            {
                Serial.println();
                Serial.println("===== STATUS =====");

                Serial.printf(
                    "IN  : %.2f bar\n",
                    water.pressureInBar
                );

                Serial.printf(
                    "OUT : %.2f bar\n",
                    water.pressureOutBar
                );

                Serial.printf(
                    "dP  : %.2f bar\n",
                    water.deltaPressureBar
                );

                Serial.printf(
                    "Flow : %.2f L/min\n",
                    water.flowLmin
                );

                Serial.printf(
                    "IN offset : %.3f\n",
                    water.pressureInOffset
                );

                Serial.printf(
                    "OUT offset: %.3f\n",
                    water.pressureOutOffset
                );

                Serial.println("==================");
            }

            else if (cmd == "save")
            {
                saveCalibration();
                saveTotals();

                Serial.println("Saved.");
            }

            else if (cmd == "reset")
            {
                water.pressureInOffset = 0.0f;
                water.pressureOutOffset = 0.0f;

                saveCalibration();

                Serial.println(
                    "Calibration reset."
                );
            }

            else if (cmd == "cal")
            {
                water.pressureInOffset -=
                    water.pressureInBar;

                water.pressureOutOffset -=
                    water.pressureOutBar;

                saveCalibration();

                Serial.println(
                    "Calibration done."
                );
            }
            
            else if (cmd == "pumptest stop")
            {
                stopPumpTest();
            }

            else if (cmd == "pumptest")
            {
                startPumpTest();
            }


            else if (cmd == "help")
            {
                Serial.println();
                Serial.println(
                    "Available commands:"
                );

                Serial.println("help");
                Serial.println("status");
                Serial.println("cal");
                Serial.println("save");
                Serial.println("reset");
                Serial.println("pumptest");
                Serial.println("pumptest stop");
            }

            else
            {
                Serial.println(
                    "Unknown command"
                );
            }

            cmd = "";
        }
        else
        {
            cmd += c;
        }
    }
}
