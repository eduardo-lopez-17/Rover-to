/**
 * @file wireless_com.cpp
 * @brief Wireless communication module for the ESP32-S3 Xiao Sense Rover
 * project
 */

#include "wireless_com.h"
#include "sensor_payload.h"

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
	esp_now_set_pmk((const uint8_t *)"PlantioSecKey123");

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

bool wireless_com_transmit_payload(const SensorPayload *payload)
{
	static const uint8_t peer_mac[] = WIRELESS_COM_BROADCAST_ADDRESS;

	if (!esp_now_is_peer_exist(peer_mac)) {
		esp_now_peer_info_t peer = {};
		memcpy(peer.peer_addr, peer_mac, 6);
		peer.channel = 0;
		peer.encrypt = true;
		memcpy(peer.lmk, "PlantioSecKey123", 16);
		esp_now_add_peer(&peer);
	}

	esp_err_t result = esp_now_send(peer_mac, (const uint8_t *)payload,
					sizeof(SensorPayload));
	return result == ESP_OK;
}