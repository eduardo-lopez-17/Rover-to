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
#define CAMERA_TASK_PRIORITY 3
#define NAVIGATION_TASK_PRIORITY 2
#define TELEMETRY_TASK_PRIORITY 1

#define IMU_PERIOD_MS 2
#define CAMERA_PERIOD_MS 33
#define NAVIGATION_PERIOD_MS 33
#define TELEMETRY_PERIOD_MS 100

// =====================================================
// SERIAL CONFIGURATION
// =====================================================

#define SERIAL_BAUDRATE 921600

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
#define CAMERA_FRAME_WIDTH 96
#define CAMERA_FRAME_HEIGHT 96
#define CAMERA_FRAME_SIZE FRAMESIZE_96X96

/// =====================================================
/// FLOW CONFIGURATION
/// =====================================================

#define FLOW_SCALE 0.0018f
#define FLOW_THRESHOLD 15          // Intensity change threshold for feature detection
#define FLOW_MIN_FEATURES 20       // Minimum features required for valid flow

/// =====================================================
/// VIO (VISUAL-INERTIAL ODOMETRY) CONFIGURATION
/// =====================================================

#define ENABLE_NAVIGATION

// EKF Parameters
#define VIO_P_VEL_INIT 0.05f      // Initial velocity covariance
#define VIO_P_ZUPT 0.001f         // Covariance when stationary (ZUPT)
#define VIO_Q_ACCEL 0.04f         // Process noise (accelerometer)

// Measurement noise
#define VIO_R_FLOW_OK 0.01f       // Low noise when camera has texture
#define VIO_R_FLOW_ERR 1000.0f    // High noise when camera is blind

// Deadbands for robust estimation
#define VIO_IMU_DEADBAND 0.35f    // m/s^2 - Vibration filter
#define VIO_FLOW_DEADBAND 0.015f  // m/s - Optical flow threshold

// Velocity thresholding
#define VIO_VEL_MIN_THRESHOLD 0.005f // Zero small velocities

// Optical flow scale (pixels to m/s conversion)
#define VIO_FLOW_SCALE BASE_FLOW_SCALE * FACTOR_CORRECCION
#define BASE_FLOW_SCALE 0.00024f
#define FACTOR_CORRECCION 3.0f

/// =====================================================
/// TELEMETRY CONFIGURATION
/// =====================================================

#define ENABLE_TELEMETRY

#endif // CONFIG_H