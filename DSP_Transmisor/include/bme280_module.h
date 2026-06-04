#ifndef BME280_MODULE_H
#define BME280_MODULE_H

#include <Arduino.h>

typedef struct {
    float temp_c;
    float humidity_pct;
    float pressure_hpa;
} BmeData;

void    bme280_init(void);
BmeData bme280_read(void);

#endif /* BME280_MODULE_H */
