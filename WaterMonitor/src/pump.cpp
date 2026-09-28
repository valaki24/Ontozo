#include <Arduino.h>

#include "config.h"
#include "pump.h"
 
static float pumpMinPressure = 2.0f;
static float pumpMaxPressure = 4.0f;

static bool pumpRunning = false;

static inline void pumpOn()
{
    if (pumpRunning)
        return;

    digitalWrite(PIN_PUMP_RELAY, LOW);
    pumpRunning = true;
}

static inline void pumpOff()
{
    if (!pumpRunning)
        return;

    digitalWrite(PIN_PUMP_RELAY, HIGH);
    pumpRunning = false;
}

void initPump()
{
    pinMode(PIN_PUMP_RELAY, OUTPUT);

    // Biztonságos indulás
    digitalWrite(PIN_PUMP_RELAY, HIGH);

    pumpRunning = false;
}

void updatePump(float pressureBar)
{
    /*
       Érvénytelen határérték:
       pumpa kikapcsolva.
    */
    if (pumpMinPressure < 0.0f ||
        pumpMaxPressure <= pumpMinPressure)
    {
        pumpOff();
        return;
    }

    /*
       V11:
       maximum bemenő nyomás elérésekor
       azonnal kikapcsoljuk a pumpát.
    */
    if (pressureBar >= pumpMaxPressure)
    {
        pumpOff();
        return;
    }

    /*
       V10:
       minimum bemenő nyomás alatt
       bekapcsoljuk a pumpát.
    */
    if (pressureBar <= pumpMinPressure)
    {
        pumpOn();
        return;
    }

    /*
       V10 és V11 között:
       megtartjuk az előző állapotot.
    */
}

void setPumpMinPressure(float pressureBar)
{
    if (pressureBar < 0.0f)
        pressureBar = 0.0f;

    pumpMinPressure = pressureBar;
}

void setPumpMaxPressure(float pressureBar)
{
    if (pressureBar < 0.0f)
        pressureBar = 0.0f;

    pumpMaxPressure = pressureBar;
}

float getPumpMinPressure()
{
    return pumpMinPressure;
}

float getPumpMaxPressure()
{
    return pumpMaxPressure;
}

bool isPumpRunning()
{
    return pumpRunning;
}
