/**
 * @file wireless_com.cpp
 * @brief Wireless communication module for the ESP32-S3 Xiao Sense Rover
 * project
 */

#include "wireless_com.h"

#include "config.h"

#include <WiFi.h>
#include <esp_now.h>

static QueueHandle_t espnowQueue = nullptr;

void wireless_com_init()
{
	// Initialize WiFi in station mode
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

	// Initialize ESP-NOW
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	// Create a queue for outgoing messages
	espnowQueue = xQueueCreate(10, sizeof(EspNowMessage));
	if (!espnowQueue) {
		Serial.println("Failed to create ESP-NOW queue");
	}
}

bool espnow_send_text(const char *fmt, ...)
{
	if (!espnowQueue)
		return false;

	EspNowMessage msg;

	va_list args;
	va_start(args, fmt);

	vsnprintf(msg.text, sizeof(msg.text), fmt, args);

	va_end(args);

	return xQueueSend(espnowQueue, &msg, 0) == pdTRUE;
}