#include "gps_modulo.h"

// Inicializar los objetos globales de forma segura
#if USAR_GPS
    TinyGPSPlus gps;
    HardwareSerial gpsSerial(2);
#endif

void inicializarGPS() {
#if USAR_GPS
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.println("[GPS] Inicializado en HardwareSerial 2.");
#else
    Serial.println("[GPS] Módulo deshabilitado por software (DNP).");
#endif
}

GPSData obtenerDatosGPS() {
    GPSData datos = {0.0, 0.0, false};

#if USAR_GPS
    // Alimentar la librería con los bytes que van llegando por el puerto serie
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    // Verificar si la lectura actual ya es válida
    if (gps.location.isValid()) {
        datos.latitud = gps.location.lat();
        datos.longitud = gps.location.lng();
        datos.valido = true;
    }
#else
    // Si está apagado, regresamos datos simulados estáticos para no romper la trama
    datos.latitud = 25.6514;  // Coordenadas aproximadas del Tec de Monterrey
    datos.longitud = -100.2895;
    datos.valido = false;
#endif

    return datos;
}