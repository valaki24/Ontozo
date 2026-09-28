#include <Preferences.h>

#include "storage.h"
#include "config.h"
#include "data.h"

Preferences prefs;

/******************************************************
 * Inicializálás
 ******************************************************/

void initStorage()
{
    prefs.begin(NVS_NAMESPACE, false);
}

/******************************************************
 * Kalibráció
 ******************************************************/

void loadCalibration()
{
    water.pressureInOffset =
        prefs.getFloat("pInOff", 0.0f);

    water.pressureOutOffset =
        prefs.getFloat("pOutOff", 0.0f);
}

void saveCalibration()
{
    prefs.putFloat(
        "pInOff",
        water.pressureInOffset);

    prefs.putFloat(
        "pOutOff",
        water.pressureOutOffset);
}

/******************************************************
 * Összes fogyasztás
 ******************************************************/

void loadTotals()
{
    water.totalMilliLiters =
        prefs.getULong64("total", 0);
}

void saveTotals()
{
    prefs.putULong64(
        "total",
        water.totalMilliLiters);
}
