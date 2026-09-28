#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>

/******************************************************
 * Pumpa
 *
 * GPIO26
 *
 * A pumpa vezérlése a WaterMonitor feladata.
 *
 * V10 = minimális nyomás
 * V11 = maximális nyomás
 ******************************************************/

void initPump();

/*
 * Nagy prioritású pumpavezérlés.
 *
 * Ezt közvetlenül a nyomásmérés után kell meghívni.
 */
void updatePump(float pressureBar);

/*
 * Blynk V10 / V11 beállítások
 */
void setPumpMinPressure(float pressureBar);
void setPumpMaxPressure(float pressureBar);

/*
 * Aktuális beállítások lekérdezése
 */
float getPumpMinPressure();
float getPumpMaxPressure();

/*
 * Pumpa állapota
 */
bool isPumpRunning();

#endif
