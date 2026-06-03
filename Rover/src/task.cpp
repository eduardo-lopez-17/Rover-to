#include "task.h"
#include "config.h"

#include "flow.h"
#include "imu.h"
#include "navigation.h"
#include "serial.h"

/// =====================================================
/// Function prototypes
/// =====================================================

static void task_imu(void *pvParameters);
static void task_camera(void *arg);
static void task_navigation(void *arg);
static void task_telemetry(void *pvParameters);

/// =====================================================
/// Variables
/// =====================================================

static TaskHandle_t imuTaskHandle = nullptr;

/// =====================================================
/// Task implementations
/// =====================================================

void task_init()
{
#ifdef ENABLE_IMU
	Serial.println("Initializing IMU...");

	// Create the IMU task pinned to core 1 with higher priority
	xTaskCreatePinnedToCore(task_imu, "IMU", 4096, nullptr,
				IMU_TASK_PRIORITY, &imuTaskHandle, 1);
#endif

#ifdef ENABLE_CAMERA
	Serial.println("Initializing Camera...");
	// Create the camera task pinned to core 1 with medium priority
	xTaskCreatePinnedToCore(task_camera, "CAM", 4096, nullptr,
				CAMERA_TASK_PRIORITY, nullptr, 1);
#endif

#ifdef ENABLE_NAVIGATION
	Serial.println("Initializing Navigation...");
	// Create the navigation task pinned to core 1 with medium priority
	xTaskCreatePinnedToCore(task_navigation, "NAV", 4096, nullptr,
				NAVIGATION_TASK_PRIORITY, nullptr, 1);
#endif

#ifdef ENABLE_TELEMETRY
	Serial.println("Initializing Telemetry...");
	// Create the telemetry task pinned to core 0 with lower priority
	xTaskCreatePinnedToCore(task_telemetry, "TEL", 4096, nullptr,
				TELEMETRY_TASK_PRIORITY, nullptr, 0);
#endif
}

static void task_imu(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		imu_update();

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(IMU_PERIOD_MS));
	}
}

static void task_camera(void *arg)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		flow_update();

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CAMERA_PERIOD_MS));
	}
}

static void task_navigation(void *arg)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		navigation_update();

		vTaskDelayUntil(&lastWakeTime,
				pdMS_TO_TICKS(NAVIGATION_PERIOD_MS));
	}
}

static void task_telemetry(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		NavigationState nav = navigation_get();
		FlowData flow = flow_get();

		Serial.printf("Yaw %.1f  Flow %.2f %.2f  Pos %.2f %.2f\n",
			      nav.yawDeg, flow.dx, flow.dy, nav.posX, nav.posY);

		vTaskDelayUntil(&lastWakeTime,
				pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
	}
}
