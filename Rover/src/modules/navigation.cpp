#include "navigation.h"

#include "config.h"
#include "math.h"
#include <Arduino.h>

#include "flow.h"
#include "imu.h"

static NavigationState state;

bool navigation_init()
{
	state.posX = 0;
	state.posY = 0;

	state.velX = 0;
	state.velY = 0;

	state.yawDeg = 0;
	state.yawRad = 0;

	return true;
}

void navigation_update()
{
	FlowData flow = flow_get();

	float yaw = imu_get_yaw_rad();

	// Local to global

	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);

	float globalVX = flow.dx * cosYaw - flow.dy * sinYaw;

	float globalVY = flow.dx * sinYaw + flow.dy * cosYaw;

	// Filtering

	state.velX = 0.8f * state.velX + 0.2f * globalVX;

	state.velY = 0.8f * state.velY + 0.2f * globalVY;

	// Integration

	static uint32_t lastUpdateUs = micros();

	uint32_t now = micros();

	float dt = (now - lastUpdateUs) * 1e-6f;

	lastUpdateUs = now;

	state.posX += state.velX * dt;
	state.posY += state.velY * dt;

	state.yawRad = yaw;
	state.yawDeg = imu_get_yaw_deg();
}

NavigationState navigation_get() { return state; }