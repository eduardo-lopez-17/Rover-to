#include "gps_module.h"
#include "board_config.h"
#include "pin_map.h"

#if USE_GPS
#include <HardwareSerial.h>
#include <TinyGPS++.h>
static TinyGPSPlus    s_gps;
static HardwareSerial s_gps_serial(1);
#endif

void gps_init(void)
{
#if USE_GPS
    /* PIN_GPS_TX = -1 isolates that pin, freeing D7 for RFM69 INT */
    s_gps_serial.begin(GPS_BAUD_RATE, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    Serial.println("[GPS] OK — searching for satellites...");
#else
    Serial.println("[GPS] disabled (DNP)");
#endif
}

GpsData gps_read(void)
{
    GpsData d = {0.0f, 0.0f};
#if USE_GPS
    while (s_gps_serial.available() > 0)
        s_gps.encode(s_gps_serial.read());
    if (s_gps.location.isUpdated()) {
        d.lat = (float)s_gps.location.lat();
        d.lng = (float)s_gps.location.lng();
    }
#else
    /* ITESM Campus Monterrey — simulated fix for indoor testing */
    d.lat = 25.6514f;
    d.lng = -100.2895f;
#endif
    return d;
}
