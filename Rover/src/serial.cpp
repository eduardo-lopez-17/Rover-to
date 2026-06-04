#include "serial.h"
#include "config.h"

#ifndef SERIAL_BAUDRATE
#warning "SERIAL_BAUDRATE not defined in config.h, defaulting to 115200"
#define SERIAL_BAUDRATE 115200
#endif

void serial_init()
{
	Serial.begin(SERIAL_BAUDRATE);
	Serial.println("Serial communication initialized.");
}