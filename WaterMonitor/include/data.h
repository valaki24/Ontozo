#pragma once

#include <Arduino.h>

struct WaterData
{
    /**************************************************
     * Pressure
     **************************************************/

    float pressureInBar = 0.0f;
    float pressureOutBar = 0.0f;
    float deltaPressureBar = 0.0f;

    /**************************************************
     * Calibration
     **************************************************/

    float pressureInOffset = 0.0f;
    float pressureOutOffset = 0.0f;

    /**************************************************
     * Flow
     **************************************************/

    float flowLmin = 0.0f;
    float flowM3h = 0.0f;

    uint64_t totalMilliLiters = 0;

    /**************************************************
     * Filter health
     **************************************************/

    float filterPercent = 0.0f;

    /**************************************************
     * Sensor status
     **************************************************/

    bool pressureInValid = false;
    bool pressureOutValid = false;
    bool flowValid = false;

    /**************************************************
     * Communication
     **************************************************/

    bool controllerOnline = false;
    uint32_t packetCounter = 0;
    uint32_t lastSendMillis = 0;

    /**************************************************
     * Diagnostics
     **************************************************/

    uint32_t uptimeSeconds = 0;

    uint32_t freeHeap = 0;

    int8_t wifiRSSI = 0;
};

extern WaterData water;
