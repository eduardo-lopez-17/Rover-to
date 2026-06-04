#ifndef INA219_MODULE_H
#define INA219_MODULE_H

#include <Arduino.h>

typedef struct {
    float bus_voltage_v;
    float current_ma;
    float power_mw;
} PowerData;

void      ina219_init(void);
PowerData ina219_read(void);

#endif /* INA219_MODULE_H */
