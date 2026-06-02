#ifndef RFM69_MODULE_H
#define RFM69_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_RFM69 false

// --- SPI PINS FOR ESP32 ---
// The module uses standard VSPI (MOSI=23, MISO=19, SCK=18)
#define RFM69_CS    5   // Chip Select
#define RFM69_INT   34  // Interrupt / DIO0
#define RFM69_RST   33  // Hard Reset Pin

// Set your bought frequency
#define RF69_FREQ   915.0

void initRFM69();
bool sendRFData(uint8_t* payload, uint8_t size);

#endif