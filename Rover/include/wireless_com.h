#ifndef WIRELESS_COM_H
#define WIRELESS_COM_H

#include "config.h"
#include "sensor_payload.h"
#include <freertos/FreeRTOS.h> // <-- agrega esta línea
#include <stdint.h>

typedef struct {
	char text[WIRELESS_COM_MAX_PAYLOAD];
} EspNowMessage;

void wireless_com_init();
bool wireless_com_transmit(const char *text);
bool wireless_com_transmit_payload(const SensorPayload *payload);

bool wireless_com_get_message(EspNowMessage *msg, TickType_t timeout);
bool wireless_com_get_peer_address(uint8_t *addr);

#endif // WIRELESS_COM_H