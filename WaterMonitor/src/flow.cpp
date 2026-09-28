#include <Arduino.h>

#include "config.h"
#include "data.h"
#include "flow.h"

/******************************************************
 * Interrupt-safe pulse counter
 ******************************************************/

static volatile uint32_t pulseCounter = 0;
static volatile uint32_t lastPulseUs = 0;

static uint32_t lastCounter = 0;
static uint32_t lastUpdateMs = 0;

constexpr uint32_t MIN_PULSE_INTERVAL_US = 1000;

/******************************************************
 * Mozgóátlag
 ******************************************************/

constexpr uint8_t FLOW_AVERAGE_SIZE = 5;

static float flowSamples[FLOW_AVERAGE_SIZE] = {0};
static uint8_t flowSampleIndex = 0;
static uint8_t flowSampleCount = 0;

/******************************************************/

static void IRAM_ATTR flowISR()
{
    uint32_t nowUs = micros();

    if (nowUs - lastPulseUs >= MIN_PULSE_INTERVAL_US)
    {
        pulseCounter++;
        lastPulseUs = nowUs;
    }
}

/******************************************************/

void initFlow()
{
    pinMode(PIN_FLOW, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(PIN_FLOW),
        flowISR,
        FALLING);

    noInterrupts();
    pulseCounter = 0;
    lastPulseUs = 0;
    interrupts();

    lastCounter = 0;
    lastUpdateMs = millis();

    water.flowLmin = 0.0f;
    water.flowM3h = 0.0f;
    water.flowValid = true;
}

/******************************************************/

void updateFlow()
{
    uint32_t now = millis();
    uint32_t elapsed = now - lastUpdateMs;

    if (elapsed < 1000)
        return;

    noInterrupts();
    uint32_t currentCount = pulseCounter;
    interrupts();

    uint32_t delta = currentCount - lastCounter;

    lastCounter = currentCount;
    lastUpdateMs = now;

    // Ha nincs impulzus, azonnal nullázunk
    if (delta == 0)
    {
        water.flowLmin = 0.0f;
        water.flowM3h = 0.0f;

        for (uint8_t i = 0; i < FLOW_AVERAGE_SIZE; i++)
            flowSamples[i] = 0.0f;

        flowSampleIndex = 0;
        flowSampleCount = 0;

        return;
    }

    // Frekvencia
    float seconds = elapsed / 1000.0f;
    float hz = delta / seconds;

    // Pillanatnyi átfolyás
    float measuredFlow = hz / FLOW_FACTOR;

    // Mozgóátlag
    flowSamples[flowSampleIndex] = measuredFlow;

    flowSampleIndex++;

    if (flowSampleIndex >= FLOW_AVERAGE_SIZE)
        flowSampleIndex = 0;

    if (flowSampleCount < FLOW_AVERAGE_SIZE)
        flowSampleCount++;

    float sum = 0.0f;

    for (uint8_t i = 0; i < flowSampleCount; i++)
        sum += flowSamples[i];

    water.flowLmin = sum / flowSampleCount;
    water.flowM3h = water.flowLmin * 0.06f;

    // Összes fogyasztás impulzus alapján
    const float mlPerPulse =
        1000.0f / (FLOW_FACTOR * 60.0f);

    water.totalMilliLiters +=
        (uint64_t)(delta * mlPerPulse);

    water.flowValid = true;
}
