#ifndef INA219_MODULE_H
#define INA219_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_INA219 false

typedef struct {
    float bus_voltage;
    float current_mA;
    float power_mW;
} PowerData;

void initINA219();
PowerData getPowerData();

#endif