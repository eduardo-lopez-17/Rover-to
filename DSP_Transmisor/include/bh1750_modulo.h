#ifndef BH1750_MODULO_H
#define BH1750_MODULO_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

// Asignación automática de pines I2C según la placa
#ifdef ARDUINO_USB_MODE
    #define I2C_SDA 8  // Pines para el ESP32-S3
    #define I2C_SCL 9
#else
    #define I2C_SDA 21 // Pines I2C por defecto en ESP32 Steren
    #define I2C_SCL 22
#endif

// Interruptor DNP (Ponlo en false si desconectas el sensor)
#define USAR_LUZ true  

void inicializarLuz();
float obtenerLuz();

#endif