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

/* MAC del receptor */
static uint8_t peerAddress[6] = WIRELESS_COM_BROADCAST_ADDRESS;

void wireless_com_init()
{
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	// esp_now_register_send_cb(onDataSent);

	esp_now_peer_info_t peerInfo = {};

	memcpy(peerInfo.peer_addr, peerAddress, sizeof(peerAddress));

	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer");
		return;
	}

	espnowQueue = xQueueCreate(10, sizeof(EspNowMessage));

	if (!espnowQueue) {
		Serial.println("Failed to create queue");
	}
}

bool wireless_com_send_text(const char *fmt, ...)
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

bool wireless_com_get_message(EspNowMessage *msg, TickType_t timeout)
{
	if (!espnowQueue)
		return false;

	return xQueueReceive(espnowQueue, msg, timeout) == pdTRUE;
}
