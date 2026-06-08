
#ifndef WIRELESS_COM_H
#define WIRELESS_COM_H

#include "config.h"
#include <stdint.h>

typedef struct {
	char text[ESPNOW_MAX_PAYLOAD];
} EspNowMessage;

void wireless_com_init();
bool wireless_com_send_text(const char *fmt, ...);

#endif // WIRELESS_COM_H
