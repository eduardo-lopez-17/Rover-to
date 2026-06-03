#include "gps_modulo.h"

#if USE_GPS
    #include <TinyGPS++.h>
    #include <HardwareSerial.h>

    TinyGPSPlus gps;
    // Usamos el UART 1 del ESP32 para no interferir con el USB (UART 0)
    HardwareSerial gpsSerial(1); 
#endif

void inicializarGPS() {
#if USE_GPS
    Serial.println("[GPS] Inicializando...");
    
    // Aquí inyectamos nuestros pines personalizados D6 y el -1
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    Serial.println("[GPS] OK (Buscando satélites...)");
#else
    Serial.println("[GPS] Módulo deshabilitado por software (DNP).");
#endif
}

GPSData obtenerDatosGPS() {
    GPSData data = {0.0, 0.0};
    
#if USE_GPS
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
    
    if (gps.location.isUpdated()) {
        data.latitud = gps.location.lat();
        data.longitud = gps.location.lng();
    }
#else
    // Coordenadas simuladas (ITESM Campus Monterrey) para pruebas en interiores
    data.latitud = 25.6514; 
    data.longitud = -100.2895;
#endif  

    return data;
}