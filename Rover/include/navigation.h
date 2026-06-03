/**
 * @file navigation.h
 * @brief Header file for navigation control of the rover
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
};

bool navigation_init();

void navigation_update();

NavigationState navigation_get();

#endif // NAVIGATION_H