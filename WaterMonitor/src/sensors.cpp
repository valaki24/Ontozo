#include <Arduino.h>

#include "config.h"
#include "data.h"
#include "sensors.h"

/******************************************************
 * Moving average filter
 ******************************************************/

static float inHistory[PRESSURE_FILTER_SIZE] = {0};
static float outHistory[PRESSURE_FILTER_SIZE] = {0};

static uint8_t filterIndex = 0;

/******************************************************/

static float adcToSensorVoltage(uint16_t mv)
{
    return (mv / 1000.0f) * DIVIDER_GAIN;
}

/******************************************************/

static float voltageToPressure(float voltage)
{
    float pressure =
        (voltage - PRESSURE_MIN_VOLTAGE) *
        ((PRESSURE_MAX_BAR - PRESSURE_MIN_BAR) /
        (PRESSURE_MAX_VOLTAGE - PRESSURE_MIN_VOLTAGE));

    if (pressure < PRESSURE_MIN_BAR)
        pressure = PRESSURE_MIN_BAR;

    if (pressure > PRESSURE_MAX_BAR)
        pressure = PRESSURE_MAX_BAR;

    return pressure;
}

/******************************************************/

static float readPressureSensor(uint8_t pin, bool &valid)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < 32; i++)
    {
        sum += analogReadMilliVolts(pin);
        delayMicroseconds(150);
    }

    uint16_t adcMv = sum / 32;

    valid =
        (adcMv >= SENSOR_MIN_MV) &&
        (adcMv <= SENSOR_MAX_MV);

    return voltageToPressure(
        adcToSensorVoltage(adcMv));
}

/******************************************************/

void initSensors()
{
    analogReadResolution(12);

    memset(inHistory, 0, sizeof(inHistory));
    memset(outHistory, 0, sizeof(outHistory));
}

/******************************************************/

void setPressureOffsets(float inOffset,
                        float outOffset)
{
    water.pressureInOffset = inOffset;
    water.pressureOutOffset = outOffset;
}

/******************************************************/

void calibratePressureZero()
{
    water.pressureInOffset -= water.pressureInBar;
    water.pressureOutOffset -= water.pressureOutBar;
}

/******************************************************/

void readSensors()
{
    float in =
        readPressureSensor(
            PIN_PRESSURE_IN,
            water.pressureInValid);

    float out =
        readPressureSensor(
            PIN_PRESSURE_OUT,
            water.pressureOutValid);

    inHistory[filterIndex] = in;
    outHistory[filterIndex] = out;

    filterIndex++;

    if (filterIndex >= PRESSURE_FILTER_SIZE)
        filterIndex = 0;

    float sumIn = 0;
    float sumOut = 0;

    for (uint8_t i = 0; i < PRESSURE_FILTER_SIZE; i++)
    {
        sumIn += inHistory[i];
        sumOut += outHistory[i];
    }

    water.pressureInBar =
        (sumIn / PRESSURE_FILTER_SIZE) +
        water.pressureInOffset;

    water.pressureOutBar =
        (sumOut / PRESSURE_FILTER_SIZE) +
        water.pressureOutOffset;

    water.deltaPressureBar =
        water.pressureInBar -
        water.pressureOutBar;

    if (water.deltaPressureBar < 0)
        water.deltaPressureBar = 0;

    water.uptimeSeconds = millis() / 1000;
    water.freeHeap = ESP.getFreeHeap();
}
