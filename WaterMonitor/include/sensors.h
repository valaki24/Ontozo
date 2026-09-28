#pragma once

// Inicializálja az ADC-t és a nyomásszenzorokat
void initSensors();

// Beolvassa mindkét nyomásszenzort,
// kiszámolja a nyomásokat és a ΔP-t
void readSensors();

// Beállítja a nyomásszenzorok kalibrációs offsetjeit
void setPressureOffsets(float inOffset, float outOffset);

// Újraméri és elmenti a jelenlegi nullpontot
void calibratePressureZero();
