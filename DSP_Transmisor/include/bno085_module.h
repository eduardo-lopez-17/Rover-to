#ifndef BNO085_MODULE_H
#define BNO085_MODULE_H

#include <Arduino.h>

typedef struct {
    float yaw_deg;        /* heading, 0-360 */
    float pitch_deg;
    float roll_deg;
    bool  valid;          /* false until first sensor event arrives */
    bool  shock_detected; /* linear acceleration spike > SHOCK_THRESHOLD_G */
    bool  tilt_alert;     /* roll or pitch jumped > TILT_THRESHOLD_DEG suddenly */
} ImuData;

void    bno085_init(void);
ImuData bno085_read(void);

#endif /* BNO085_MODULE_H */
