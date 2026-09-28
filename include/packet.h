#pragma once

#include <Arduino.h>

/******************************************************
 * WATERMONITOR → ÖNTÖZŐ
 ******************************************************/

enum WaterMessageType : uint8_t
{
    WATER_MSG_NONE = 0,

    WATER_MSG_START_RESPONSE,
    WATER_MSG_STOP_RESPONSE,
    WATER_MSG_TOTAL_LITER_RESPONSE,
    WATER_MSG_ZONE_CHANGE,
    WATER_MSG_HEATPUMP_START,
    WATER_MSG_HEATPUMP_STOP
};


/******************************************************
 * ÖNTÖZŐ → WATERMONITOR
 ******************************************************/

enum WaterCommandType : uint8_t
{
    WATER_CMD_NONE = 0,

    WATER_CMD_STARTA,              // automata indítás
    WATER_CMD_STARTM,              // manuális indítás
    WATER_CMD_STOP,
    WATER_CMD_TOTAL_LITER_REQUEST,
    WATER_CMD_ACK
};


/******************************************************
 * WATERMONITOR ÜZENET
 ******************************************************/

struct WaterMessage
{
    uint32_t counter;
    uint8_t type;
    uint64_t totalMilliLiters;
};


/******************************************************
 * ÖNTÖZŐ PARANCS
 ******************************************************/

struct WaterCommand
{
    uint32_t counter;
    uint8_t type;
};


/******************************************************
 * SEGÉDFÜGGVÉNY
 ******************************************************/

static inline float waterTotalLiter(
    const WaterMessage& message
)
{
    return (float)message.totalMilliLiters / 1000.0f;
}