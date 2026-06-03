#include "task.h"
#include "config.h"

#include "flow.h"
#include "imu.h"
#include "serial.h"

/// =====================================================
/// Function prototypes
/// =====================================================

static void task_imu(void *pvParameters);
static void task_camera(void *arg);
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
	// Create the IMU task pinned to core 1 with higher priority
	xTaskCreatePinnedToCore(task_imu, "IMU", 4096, nullptr,
				IMU_TASK_PRIORITY, &imuTaskHandle, 1);

	// Create the camera task pinned to core 1 with medium priority
	xTaskCreatePinnedToCore(task_camera, "CAM", 4096, nullptr,
				CAMERA_TASK_PRIORITY, nullptr, 1);

	// Create the telemetry task pinned to core 0 with lower priority
	xTaskCreatePinnedToCore(task_telemetry, "TEL", 4096, nullptr,
				TELEMETRY_TASK_PRIORITY, nullptr, 0);
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

		vTaskDelay(pdMS_TO_TICKS(CAMERA_PERIOD_MS));
	}
}

static void task_telemetry(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		Serial.printf("Yaw: %.2f deg\n", imu_get_yaw_deg());

		vTaskDelayUntil(&lastWakeTime,
				pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
	}
}
