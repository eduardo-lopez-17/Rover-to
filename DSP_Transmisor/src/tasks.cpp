/**
 * @file tasks.cpp
 * @brief Implementación de las tareas del sistema.
 */

#include "tasks.h"

#include "ultrasonico.h"
#include "moist.h"

Moist moistureSensor(1); // Pin analógico para el sensor de humedad

void Tasks::init_tasks()
{
    // Init Moist sensor
    moistureSensor.init_task();
    // Init Ultrasonic sensor
    inicializarUltrasonico();
}
