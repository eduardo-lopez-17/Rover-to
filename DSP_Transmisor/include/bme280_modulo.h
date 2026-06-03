#ifndef BME280_MODULO_H
#define BME280_MODULO_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// =========================================================================
// INTERRUPTOR DE ACTIVACIÓN (Ponlo en true cuando conectes el sensor)
// =========================================================================
#define USAR_BME true 

// Estructura para empaquetar los tres datos ambientales
struct BMEData {
    float temperatura; // °C
    float humedad;     // %
    float presion;     // hPa
};

void inicializarBME();
BMEData obtenerDatosBME();

#endif