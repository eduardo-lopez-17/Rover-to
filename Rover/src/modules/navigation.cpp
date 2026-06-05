#include "navigation.h"

#include "config.h"
#include "math.h"
#include <Arduino.h>

#include "flow.h"
#include "imu.h"

// =====================================================
// STATE VARIABLES
// =====================================================
static NavigationState state;

// Initial yaw for calibration
static float initialYaw = -1000.0f;

// Calibration flag
static bool isCalibrated = false;
static unsigned long calibrationStartTime = 0;

// Last update time for dt calculation
static unsigned long lastUpdateUs = 0;

// =====================================================
// INITIALIZATION
// =====================================================
bool navigation_init()
{
	state.posX = 0;
	state.posY = 0;

	state.velX = 0;
	state.velY = 0;

	state.yawDeg = 0;
	state.yawRad = 0;

	// EKF Covariance initialization
	state.P_velX = VIO_P_VEL_INIT;
	state.P_velY = VIO_P_VEL_INIT;

	state.flowValid = false;

	lastUpdateUs = micros();

	return true;
}

void navigation_start_calibration()
{
	isCalibrated = false;
	calibrationStartTime = millis();
	initialYaw = -1000.0f;

	Serial.println("\n==========================================");
	Serial.println("   CALIBRACIÓN VIO INICIADA");
	Serial.println("==========================================");
	Serial.println("-> Mueve el móvil dibujando un '8' en el aire...");
}

bool navigation_is_calibrated()
{
	return isCalibrated;
}

// =====================================================
// EKF UPDATE WITH ZUPT
// =====================================================
void navigation_update()
{
	unsigned long now = micros();
	float dt = (now - lastUpdateUs) * 1e-6f;
	lastUpdateUs = now;

	if (dt <= 0 || dt > 0.5f) {
		return;  // Skip invalid dt
	}

	// Get sensor data
	FlowData flow = flow_get();
	float yaw = imu_get_yaw_rad();
	float linAccelX = imu_get_lin_accel_x();
	float linAccelY = imu_get_lin_accel_y();

	// Calibration: set initial yaw on first valid IMU reading
	if (initialYaw < -999.0f) {
		initialYaw = yaw;
		isCalibrated = true;
		Serial.println("-> Coloca el móvil FIRME en el punto de partida (0,0).");
		Serial.println("-> Iniciando tracking...");
		return;
	}

	// Compute yaw relative to initial
	float relativeYaw = yaw - initialYaw;
	if (relativeYaw > PI) relativeYaw -= 2.0f * PI;
	if (relativeYaw < -PI) relativeYaw += 2.0f * PI;

	state.yawRad = relativeYaw;
	state.yawDeg = relativeYaw * 180.0f / PI;

	// =====================================================
	// 1. PREDICTION STEP (IMU)
	// =====================================================
	float accLocal_x = linAccelX;
	float accLocal_y = linAccelY;

	// Apply deadband to accelerometer for stationary noise
	if (fabs(accLocal_x) < VIO_IMU_DEADBAND) accLocal_x = 0.0f;
	if (fabs(accLocal_y) < VIO_IMU_DEADBAND) accLocal_y = 0.0f;

	// Transform to global frame using yaw (now stored as state.yawRad)
	float cosYaw = cosf(state.yawRad);
	float sinYaw = sinf(state.yawRad);

	float globalAccelX = accLocal_x * cosYaw - accLocal_y * sinYaw;
	float globalAccelY = accLocal_x * sinYaw + accLocal_y * cosYaw;

	// Velocity prediction
	float velX_pred = state.velX + globalAccelX * dt;
	float velY_pred = state.velY + globalAccelY * dt;

	// Covariance prediction
	state.P_velX += VIO_Q_ACCEL * dt;
	state.P_velY += VIO_Q_ACCEL * dt;

	// =====================================================
	// 2. OPTICAL FLOW PROCESSING
	// =====================================================
	float camVX = 0.0f, camVY = 0.0f;
	float globalVX = 0.0f, globalVY = 0.0f;

	if (flow.valid) {
		camVX = -flow.dx * VIO_FLOW_SCALE / dt;
		camVY = flow.dy * VIO_FLOW_SCALE / dt;

		globalVX = camVX * cosYaw - camVY * sinYaw;
		globalVY = camVX * sinYaw + camVY * cosYaw;
	}

	state.flowValid = flow.valid;

	// =====================================================
	// 3. ZUPT LOGIC (Zero Velocity Update)
	// =====================================================
	bool isStationary = (accLocal_x == 0.0f && accLocal_y == 0.0f) &&
			(flow.valid && fabs(camVX) < VIO_FLOW_DEADBAND &&
			 fabs(camVY) < VIO_FLOW_DEADBAND);

	if (isStationary) {
		// Force high gain update, collapse error covariance
		state.velX = 0.0f;
		state.velY = 0.0f;
		state.P_velX = VIO_P_ZUPT;
		state.P_velY = VIO_P_ZUPT;
	} else {
		// =====================================================
		// 4. EKF UPDATE STEP
		// =====================================================
		float R_actual_x = flow.valid ? VIO_R_FLOW_OK : VIO_R_FLOW_ERR;
		float R_actual_y = flow.valid ? VIO_R_FLOW_OK : VIO_R_FLOW_ERR;

		// Kalman gain
		float K_x = state.P_velX / (state.P_velX + R_actual_x);
		float K_y = state.P_velY / (state.P_velY + R_actual_y);

		// Measurement: if flow invalid, trust IMU prediction
		float measurementX = flow.valid ? globalVX : velX_pred;
		float measurementY = flow.valid ? globalVY : velY_pred;

		state.velX = velX_pred + K_x * (measurementX - velX_pred);
		state.velY = velY_pred + K_y * (measurementY - velY_pred);

		// Covariance update
		state.P_velX = (1.0f - K_x) * state.P_velX;
		state.P_velY = (1.0f - K_y) * state.P_velY;
	}

	// Clean up tiny noise
	if (fabs(state.velX) < VIO_VEL_MIN_THRESHOLD) state.velX = 0.0f;
	if (fabs(state.velY) < VIO_VEL_MIN_THRESHOLD) state.velY = 0.0f;

	// =====================================================
	// 5. POSITION INTEGRATION
	// =====================================================
	state.posX += state.velX * dt;
	state.posY += state.velY * dt;
}

NavigationState navigation_get() { return state; }
