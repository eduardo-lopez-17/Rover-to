/**
 * @file flow.h
 * @brief Header file for camera vision flow control with VIO support
 */

#ifndef FLOW_H
#define FLOW_H

#include <stdint-gcc.h>

#pragma once

struct FlowData {
	float dx;
	float dy;
	uint32_t features;
	bool valid;    // Validity flag for optical flow
	float frameDt; // Time between frames in seconds
};

bool flow_init();

bool flow_update();

FlowData flow_get();

#endif // FLOW_H