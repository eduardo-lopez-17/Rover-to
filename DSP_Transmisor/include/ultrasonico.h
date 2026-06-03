#ifndef ULTRASONICO_H
#define ULTRASONICO_H

#include <Arduino.h>

// --- INTERRUPTOR DNP (Ponlo en false para aislar el sensor) ---
#define USE_ULTRASONIC true

// Detectar automáticamente si estás compilando en el XIAO S3 o en el clásico
#ifdef ARDUINO_USB_MODE
    // --- MODO I2C (Para el XIAO S3) ---
    #define ULTRASONIC_USE_I2C true
    #define ULTRASONIC_I2C_ADDR 0x57 // Dirección I2C del chip RCWL-9200
#else
    // --- MODO CLÁSICO TRIG/ECHO (Para tu ESP32 Steren) ---
    #define ULTRASONIC_USE_I2C false
    #define TRIG_PIN 12  
    #define ECHO_PIN 13
#endif

// Funciones públicas
void inicializarUltrasonico();
float obtenerDistancia();

#endif