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

void Moist::init_task()
{
    // Code to initialize the task for reading moisture levels
    xTaskCreate(
        [](void *pvParameters) { static_cast<Moist *>(pvParameters)->task(pvParameters); },
        "MoistureTask",
        2048,
        this,
        1,
        nullptr);
}

void Moist::task(void *pvParameters)
{
    for (;;)
    {
        // This function can be called in a loop or as a task to continuously read moisture levels
        uint16_t current_moisture = get_moisture_level();
        // You can add code here to process the moisture level, such as sending it via ESP-NOW or logging it
        
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second before the next reading
    }
}
