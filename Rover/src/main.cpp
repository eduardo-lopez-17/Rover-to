#include <Arduino.h>

#include "serial.h"
#include "task.h"

void setup()
{
	serial_init();
	task_init();
}

void loop()
{
	// Nothing to do here, all tasks are handled by FreeRTOS
}
