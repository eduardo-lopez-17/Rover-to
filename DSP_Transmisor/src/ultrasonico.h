#ifndef ULTRASONICO_H
#define ULTRASONICO_H

#include <Arduino.h>

// Detectar automáticamente si estás compilando en el S3 o en el clásico
#ifdef ARDUINO_USB_MODE
    // Pines para el ESP32-S3 de Ángel
    #define TRIG_PIN 13  
    #define ECHO_PIN 14
#else
    // Pines para tu ESP32 clásico de Steren
    #define TRIG_PIN 12  
    #define ECHO_PIN 13
#endif

// Funciones públicas
void inicializarUltrasonico();
float obtenerDistancia();

#endif