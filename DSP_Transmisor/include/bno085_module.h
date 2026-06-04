#ifndef BNO085_MODULE_H
#define BNO085_MODULE_H

#include <Arduino.h>

typedef struct {
    float yaw_deg;   /* heading, 0-360 */
    float pitch_deg;
    float roll_deg;
    bool  valid;     /* false until first sensor event arrives */
} ImuData;

void    bno085_init(void);
ImuData bno085_read(void);

#endif /* BNO085_MODULE_H */
