#include "task.h"
#include "config.h"

#include "flow.h"
#include "imu.h"
#include "navigation.h"
#include "serial.h"
#include "wireless_com.h"

#include <esp_now.h>

/// =====================================================
/// Function prototypes
/// =====================================================

static void task_imu(void *pvParameters);
static void task_camera(void *arg);
static void task_navigation(void *arg);
static void task_telemetry(void *pvParameters);
static void task_wireless_com_tx(void *arg);

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
				IMU_TASK_PRIORITY, &imuTaskHandle, 0);
#endif

#ifdef ENABLE_CAMERA
	Serial.println("Initializing Camera...");
	// Create the camera task pinned to core 1 with medium priority
	xTaskCreatePinnedToCore(task_camera, "CAM", 4096, nullptr,
				CAMERA_TASK_PRIORITY, nullptr, 1);
#endif

#ifdef ENABLE_NAVIGATION
	Serial.println("Initializing Navigation...");
	// Create the navigation task pinned to core 0 with medium priority
	xTaskCreatePinnedToCore(task_navigation, "NAV", 4096, nullptr,
				NAVIGATION_TASK_PRIORITY, nullptr, 0);

	// Start VIO calibration routine
	navigation_start_calibration();
#endif

#ifdef ENABLE_TELEMETRY
	Serial.println("Initializing Telemetry...");
	// Create the telemetry task pinned to core 0 with lower priority
	xTaskCreatePinnedToCore(task_telemetry, "TEL", 4096, nullptr,
				TELEMETRY_TASK_PRIORITY, nullptr, 0);
#endif

#ifdef ENABLE_WIRELESS_COM
	Serial.println("Initializing Wireless Communication...");
	// Create the wireless communication task pinned to core 0 with lower
	// priority
	xTaskCreatePinnedToCore(task_wireless_com_tx, "ESP", 4096, nullptr,
				WIRELESS_COM_TASK_PRIORITY, nullptr, 0);
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

		static uint32_t last = millis();

		uint32_t now = millis();

		// Serial.printf("Frame dt=%lu ms\n", now - last);

		last = now;

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

		Serial.printf(
		    "[%lu ms] POS_2D: X:%.1fcm Y:%.1fcm | VEL: VX:%.1fcm/s "
		    "VY:%.1fcm/s | FLOW:%s | YAW:%.1f deg\n",
		    millis(), nav.posX * 100.0f, nav.posY * 100.0f,
		    nav.velX * 100.0f, nav.velY * 100.0f,
		    nav.flowValid ? "OK " : "ERR", nav.yawDeg);

		FlowData flow = flow_get();

		// Serial.printf("Flow %.1f %.1f Valid %d\n", flow.dx, flow.dy,
		//   flow.valid);

		vTaskDelayUntil(&lastWakeTime,
				pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
	}
}

static void task_wireless_com_tx(void *arg)
{
	EspNowMessage msg;

	for (;;) {
		if (wireless_com_get_message(&msg, portMAX_DELAY)) {
			wireless_com_send_text("%s", msg.text);
		}
	}
}
