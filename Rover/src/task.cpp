#include "task.h"

#include "flow.h"
#include "imu.h"
#include "serial.h"

static TaskHandle_t imuTaskHandle = nullptr;

void task_init()
{
	// Create the IMU task pinned to core 1 with higher priority
	xTaskCreatePinnedToCore(task_imu, "IMU", 4096, nullptr, 4,
				&imuTaskHandle, 1);

	// Create the telemetry task pinned to core 0 with lower priority
	xTaskCreatePinnedToCore(task_telemetry, "TEL", 4096, nullptr, 1,
				nullptr, 0);
}

static void task_imu(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		imu_update();

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2));
	}
}

static void task_camera(void *arg)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		flow_update();

		vTaskDelay(pdMS_TO_TICKS(33));
	}
}

static void task_telemetry(void *pvParameters)
{
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		Serial.printf("Yaw: %.2f deg\n", imu_get_yaw_deg());

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
	}
}
