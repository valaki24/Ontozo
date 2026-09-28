#pragma once

#include <Arduino.h>

#include "packet.h"


/******************************************************
 * HTTP WATER RECEIVER
 ******************************************************/

void initWaterReceiver();

void updateWaterReceiver();


/******************************************************
 * WATERMONITOR → ÖNTÖZŐ
 ******************************************************/

bool waterMonitorResponseAvailable();

WaterMessage getWaterMonitorResponse();

void clearWaterMonitorResponse();


/******************************************************
 * ÖNTÖZŐ → WATERMONITOR
 ******************************************************/

bool sendWaterCommand(
    uint8_t commandType
);


/******************************************************
 * WATERMONITOR ELÉRHETŐSÉG
 ******************************************************/

bool isWaterMonitorOnline();

uint32_t getWaterMonitorLastSeen();