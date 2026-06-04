#ifndef BH1750_MODULE_H
#define BH1750_MODULE_H

#include <Arduino.h>

void  bh1750_init(void);
float bh1750_read_lux(void);

#endif /* BH1750_MODULE_H */
