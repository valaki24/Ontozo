#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

void timeManagerBegin();
void timeManagerUpdate();

bool timeIsValid();

uint32_t getUnixTime();

String getTimeString();

String formatUnixTime(uint32_t ts);

#endif