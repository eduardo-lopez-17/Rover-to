#ifndef SOIL_MOISTURE_MODULE_H
#define SOIL_MOISTURE_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_SOIL_SENSOR false

// Usa un pin analógico seguro en el ESP32 (como el 32, 33, 34 o 35)
// Pin analógico en el XIAO
#define SOIL_PIN D0

void initSoilMoisture();
float getSoilMoisture();

#endif