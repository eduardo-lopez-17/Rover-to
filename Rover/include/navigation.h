/**
 * @file navigation.h
 * @brief Header file for VIO navigation with EKF and ZUPT
 */

#ifndef NAVIGATION_H
#define NAVIGATION_H

#pragma once

struct NavigationState {
	float posX;
	float posY;

	float velX;
	float velY;

	float yawDeg;
	float yawRad;

	// EKF covariance
	float P_velX;
	float P_velY;

	// Validity
	bool flowValid;
};

bool navigation_init();

void navigation_update();

NavigationState navigation_get();

// Calibration
void navigation_start_calibration();

bool navigation_is_calibrated();

#endif // NAVIGATION_H