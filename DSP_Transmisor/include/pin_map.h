#ifndef PIN_MAP_H
#define PIN_MAP_H

#include <Arduino.h>

/* =========================================================================
 * PIN MAP — Seeed Studio XIAO ESP32-S3  (primary target)
 * ========================================================================= */
#if defined(ARDUINO_SEEED_XIAO_ESP32S3)

/* I2C — shared by OLED, BME280, INA219, Ultrasonic, BH1750 */
#define PIN_I2C_SDA D4
#define PIN_I2C_SCL D5

/* RFM69 — SPI chip-select and interrupt */
#define PIN_RFM69_CS  D1
#define PIN_RFM69_INT D7
#define PIN_RFM69_RST -1 /* reset not wired on this board */

/* GPS — UART1, TX isolated to free D7 for RFM69 INT */
#define PIN_GPS_RX D6
#define PIN_GPS_TX -1

/* SIM800 — UART1 (conflicts with GPS; only one active at a time) */
#define PIN_SIM800_RX D3
#define PIN_SIM800_TX D2

/* Soil moisture — analog input */
#define PIN_SOIL D0

/* =========================================================================
 * PIN MAP — Generic ESP32 dev board (Steren / fallback)
 * ========================================================================= */
#else

#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

#define PIN_RFM69_CS  5
#define PIN_RFM69_INT 4
#define PIN_RFM69_RST -1

#define PIN_GPS_RX 16
#define PIN_GPS_TX 17

#define PIN_SIM800_RX 26
#define PIN_SIM800_TX 27

/* Trig/Echo used only on generic ESP32 (XIAO uses I2C ultrasonic) */
#define PIN_ULTRASONIC_TRIG 12
#define PIN_ULTRASONIC_ECHO 13

#define PIN_SOIL 32

#endif /* board selection */

#endif /* PIN_MAP_H */
