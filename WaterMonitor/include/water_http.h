#pragma once

#include <Arduino.h>
#include "packet.h"

/******************************************************
 * HTTP WATER COMMUNICATION
 ******************************************************/

void initWaterHttp();
void updateWaterHttp();


/******************************************************
 * WATERMONITOR → ÖNTÖZŐ
 ******************************************************/

bool sendWaterMessage(
    uint8_t messageType,
    uint64_t totalMilliLiters
);


/******************************************************
 * ÖNTÖZŐ → WATERMONITOR
 ******************************************************/

bool waterCommandAvailable();

WaterCommand getWaterCommand();

void clearWaterCommand();


/******************************************************
 * ÁLLAPOT
 ******************************************************/

bool isIrrigationOnline();

uint32_t getIrrigationLastSeen();
