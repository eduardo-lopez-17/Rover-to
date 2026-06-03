/**
 * @file config.h
 * @brief Configuración global para el proyecto de DSP en ESP32-S3 Xiao Sense
 */

#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// TARGET PLATFORM
// =====================================================

// Please select the target platform by uncommenting the corresponding line.
// Only one platform should be enabled at a time.
#define ESP32_S3_XIAO_SENSE
// #define ESP32_S3
// #define ESP32_DOIT

// =====================================================
// SERIAL CONFIGURATION
// =====================================================

#define SERIAL_BAUDRATE 115200

// =====================================================
// IMU CONFIGURATION
// =====================================================

// Do you want to block the program if the IMU is not found? If not defined, it
// will print a warning and continue.
#define IMU_NON_BLOCKING

#endif // CONFIG_H