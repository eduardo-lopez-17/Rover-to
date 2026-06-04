#ifndef ULTRASONIC_MODULE_H
#define ULTRASONIC_MODULE_H

#include <Arduino.h>

void  ultrasonic_init(void);
float ultrasonic_read_cm(void);

#endif /* ULTRASONIC_MODULE_H */
