/**
 * @file imu.h
 * @brief Interface for IMU BNO085 with VIO support
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

/// Linear acceleration (VIO support)
float imu_get_lin_accel_x();
float imu_get_lin_accel_y();
float imu_get_lin_accel_z();

/// Quaternion data (VIO support)
void imu_get_quaternion(float &qR, float &qI, float &qJ, float &qK);

#endif // IMU_H