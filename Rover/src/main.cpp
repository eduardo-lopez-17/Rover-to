#include <Arduino.h>

#include "camera.h"
#include "flow.h"
#include "imu.h"
#include "navigation.h"
#include "serial.h"
#include "wireless_com.h"

#include "task.h"

void setup()
{
	serial_init();
	wireless_com_init();

	imu_init();
	camera_init();
	flow_init();
	navigation_init();

	task_init();
}

void loop()
{
	// Nothing to do here, all tasks are handled by FreeRTOS
}
