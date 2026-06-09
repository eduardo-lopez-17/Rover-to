
#ifndef WIRELESS_COM_H
#define WIRELESS_COM_H

#include "config.h"
#include <stdint.h>

#include <WiFi.h>
#include <esp_now.h>


typedef struct {
	char text[WIRELESS_COM_MAX_PAYLOAD];
} EspNowMessage;

void wireless_com_init();
bool wireless_com_send_text(const char *fmt, ...);

bool wireless_com_get_message(EspNowMessage *msg, TickType_t timeout);

#endif // WIRELESS_COM_H
