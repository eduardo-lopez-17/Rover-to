#ifndef SOIL_MOISTURE_MODULE_H
#define SOIL_MOISTURE_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_SOIL_SENSOR false

// Usa un pin analógico seguro en el ESP32 (como el 32, 33, 34 o 35)
// Nota: Evita los pines ADC2 (del 25 al 27) si estás usando WiFi
#define SOIL_PIN 32 

void initSoilMoisture();
float getSoilMoisture();

#endif