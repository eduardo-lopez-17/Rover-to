#ifndef GPS_MODULO_H
#define GPS_MODULO_H

#include <Arduino.h>
#include <TinyGPS++.h>

// --- CONFIGURACIÓN DE PINES (Serial 2) ---
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

// =========================================================================
// INTERRUPTOR DE ACTIVACIÓN (Ponlo en true cuando conectes el GPS real)
// =========================================================================
#define USAR_GPS false  // Al estar en false, el código del GPS no se ejecutará

// Estructura para empaquetar los datos que irán al Gemelo Digital
typedef struct {
    float latitud;
    float longitud;
    bool valido;
} GPSData;

// Funciones públicas
void inicializarGPS();
GPSData obtenerDatosGPS();

#endif