#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/******************************************************
 * LOGGER FORRÁSOK
 ******************************************************/

enum LoggerSource
{
    SOURCE_NONE = 0,

    SOURCE_ZONE1,
    SOURCE_ZONE2,
    SOURCE_ZONE3,
    SOURCE_ZONE4,
    SOURCE_ZONE5,
    SOURCE_ZONE6,
    SOURCE_ZONE7,

    SOURCE_POND,
    SOURCE_HEATPUMP
};


/******************************************************
 * NAPLÓ BEJEGYZÉS
 ******************************************************/

struct LogEntry
{
    uint32_t startTime;
    uint32_t stopTime;

    float startLiter;
    float stopLiter;
    float usedLiter;

    uint8_t source;
};


/******************************************************
 * LOGGER FÁJL
 ******************************************************/

#define LOGGER_FILE "/logger.csv"


/******************************************************
 * INDÍTÁS
 ******************************************************/

void loggerBegin();


/******************************************************
 * ÖNTÖZÉSI ZÓNA
 ******************************************************/

void loggerStartZone(
    uint8_t zone,
    float startLiter
);

void loggerFinishZone(
    float stopLiter
);


/******************************************************
 * ÖNTÖZÉS LEÁLLÍTÁSA
 ******************************************************/

void loggerStop(
    float stopLiter
);


/******************************************************
 * POND
 ******************************************************/

void loggerStartPond(
    float startLiter
);

void loggerFinishPond(
    float stopLiter
);


/******************************************************
 * HŐSZIVATTYÚ
 ******************************************************/

void loggerStartHeatpump(
    float startLiter
);

void loggerFinishHeatpump(
    float stopLiter
);


/******************************************************
 * ÚJ BEJEGYZÉS
 ******************************************************/

bool loggerHasNewEntry();

LogEntry loggerGetEntry();

void loggerClearEntry();


/******************************************************
 * NAPLÓ LEKÉRDEZÉS
 ******************************************************/

uint16_t loggerCount();

LogEntry loggerGet(
    uint16_t index
);

uint8_t loggerGetActiveSource();


/******************************************************
 * FORRÁS NEVE
 ******************************************************/

const char* loggerSourceName(
    uint8_t source
);

#endif