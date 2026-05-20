/**
 * moist.h - This file contains the declaration for the moisture sensor class.
 */

#ifndef MOIST_H
#define MOIST_H

#include "Arduino.h"

class Moist
{
    public:
        Moist(uint8_t sensor_pin);
        void init_moisture_sensor(uint8_t sensor_pin);
        uint16_t get_moisture_level();

    private:
        uint8_t MOISTURE_SENSOR_PIN;
};

#endif