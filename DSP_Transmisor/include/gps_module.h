#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>

typedef struct {
    float lat;
    float lng;
} GpsData;

void    gps_init(void);
GpsData gps_read(void);

#endif /* GPS_MODULE_H */
