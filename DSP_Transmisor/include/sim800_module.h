#ifndef SIM800_MODULE_H
#define SIM800_MODULE_H

#include <Arduino.h>

void sim800_init(void);
bool sim800_send_cloud(const String &json_payload);

#endif /* SIM800_MODULE_H */
