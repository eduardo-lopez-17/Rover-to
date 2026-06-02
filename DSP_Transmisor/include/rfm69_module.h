#ifndef RFM69_MODULE_H
#define RFM69_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_RFM69 false

// Pines SPI y control para el XIAO ESP32-S3
#define RFM69_CS    D1  // Según tu esquemático (J1 Pin 2)
#define RFM69_INT   D7  // El pin que le robaste al TX del GPS (J2 Pin 7)
#define RFM69_RST   -1  // En tu esquemático no veo cable de RESET, así que lo ignoramos


// Set your bought frequency
#define RF69_FREQ   915.0

void initRFM69();
bool sendRFData(uint8_t* payload, uint8_t size);

#endif