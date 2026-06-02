#ifndef BH1750_MODULO_H
#define BH1750_MODULO_H

#include <Arduino.h>

// Asignación automática de pines I2C según la placa
#ifdef ARDUINO_USB_MODE
    // Pines I2C compartidos para el XIAO ESP32-S3 (Según tu esquemático)
    #define I2C_SDA D4 
    #define I2C_SCL D5
#else
    // Pines I2C por defecto en ESP32 Steren
    #define I2C_SDA 21 
    #define I2C_SCL 22
#endif

// Interruptor DNP (Ponlo en false para la prueba de "Cerebro Limpio")
#define USAR_LUZ false  

void inicializarLuz();
float obtenerLuz();

#endif