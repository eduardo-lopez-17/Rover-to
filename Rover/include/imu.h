/**
 * @file imu.h
 * @brief Interface for IMU BNO085
 */

#ifndef IMU_H
#define IMU_H

bool imu_init();

void imu_update();

/// Getters for IMU data
float imu_get_yaw_deg();
float imu_get_yaw_rad();
float imu_get_gyro_z();
float imu_get_heading_deg();

#endif // IMU_H