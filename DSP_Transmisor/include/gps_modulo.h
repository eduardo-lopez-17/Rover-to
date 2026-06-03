#ifndef GPS_MODULO_H
#define GPS_MODULO_H

#include <Arduino.h>

// --- INTERRUPTOR DNP ---
#define USE_GPS true

// --- CONFIGURACIÓN DE PINES XIAO S3 ---
#define GPS_BAUD 9600
#define GPS_RX_PIN D6
#define GPS_TX_PIN -1 // El -1 aísla este pin para dejárselo libre al RFM69

typedef struct {
    float latitud;
    float longitud;
} GPSData;

void inicializarGPS();
GPSData obtenerDatosGPS();

#endif