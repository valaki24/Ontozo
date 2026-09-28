#pragma once

#include <Arduino.h>

/******************************************************
 * GPIO
 ******************************************************/

constexpr uint8_t PIN_PRESSURE_IN  = 34;
constexpr uint8_t PIN_PRESSURE_OUT = 35;
constexpr uint8_t PIN_FLOW         = 27;
constexpr uint8_t PIN_PUMP_RELAY = 26;

/******************************************************
 * Pressure sensor
 *
 * Sensor:
 * 0.5V = 0 bar
 * 4.5V = 12 bar
 *
 * Nincs feszültségosztó.
 * A rendszerben várható maximum kb. 5,5 bar,
 * ami kb. 2,33 V szenzorjelet jelent.
 ******************************************************/

constexpr float PRESSURE_MIN_BAR = 0.0f;
constexpr float PRESSURE_MAX_BAR = 12.0f;

constexpr float PRESSURE_MIN_VOLTAGE = 0.5f;
constexpr float PRESSURE_MAX_VOLTAGE = 4.5f;

constexpr float DIVIDER_GAIN = 1.0f;

constexpr uint16_t SENSOR_MIN_MV = 400;
constexpr uint16_t SENSOR_MAX_MV = 3200;

/******************************************************
 * Filtering
 ******************************************************/

constexpr uint8_t PRESSURE_FILTER_SIZE = 8;

/******************************************************
 * Flow sensor
 *
 * F = 4.5 × Q
 * Q = Hz / 4.5
 ******************************************************/

constexpr float FLOW_FACTOR = 0.45f;

/******************************************************
 * Storage (NVS)
 ******************************************************/

constexpr char NVS_NAMESPACE[] = "water";

/******************************************************
 * Communication
 ******************************************************/

constexpr uint32_t DEBUG_INTERVAL_MS = 2000;
constexpr uint32_t SENSOR_INTERVAL_MS = 200;
constexpr uint32_t FLOW_INTERVAL_MS = 1000;
constexpr uint32_t SEND_INTERVAL_MS = 1000;
