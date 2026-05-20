/**
 * moist.cpp - This file contains code related to the moisture sensor.
 */

#include "moist.h"

Moist::Moist(uint8_t sensor_pin)
{
    init_moisture_sensor(sensor_pin);
}

void Moist::init_moisture_sensor(uint8_t sensor_pin)
{
    // Code to initialize the moisture sensor
    MOISTURE_SENSOR_PIN = sensor_pin;
    
    digitalWrite(MOISTURE_SENSOR_PIN, LOW); // Ensure the pin is LOW before setting it as INPUT
    pinMode(MOISTURE_SENSOR_PIN, INPUT);
}
uint16_t Moist::get_moisture_level()
{
    // Code to read moisture level from the sensor
    uint16_t moisture_level = analogRead(MOISTURE_SENSOR_PIN);
    return moisture_level;
}
