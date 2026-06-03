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

/// =====================================================
/// TASKS CONFIGURATION
/// =====================================================

#define IMU_TASK_PRIORITY 4
#define CAMERA_TASK_PRIORITY 2
#define TELEMETRY_TASK_PRIORITY 1

#define IMU_PERIOD_MS 2
#define CAMERA_PERIOD_MS 33
#define TELEMETRY_PERIOD_MS 100

// =====================================================
// SERIAL CONFIGURATION
// =====================================================

#define SERIAL_BAUDRATE 115200

// =====================================================
// IMU CONFIGURATION
// =====================================================

#define ENABLE_IMU
// Do you want to block the program if the IMU is not found? If not defined, it
// will print a warning and continue.
#define IMU_NON_BLOCKING

/// =====================================================
/// CAMERA CONFIGURATION
/// =====================================================

#define ENABLE_CAMERA
#define CAMERA_FRAME_WIDTH 160
#define CAMERA_FRAME_HEIGHT 120
#define CAMERA_FRAME_SIZE FRAMESIZE_QQVGA

/// =====================================================
/// FLOW CONFIGURATION
/// =====================================================

#define FLOW_SCALE 0.0018f

#define FLOW_THRESHOLD 15

#define FLOW_MIN_FEATURES 20

#endif // CONFIG_H