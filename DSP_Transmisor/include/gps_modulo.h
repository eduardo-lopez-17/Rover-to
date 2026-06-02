#ifndef GPS_MODULO_H
#define GPS_MODULO_H

#include <Arduino.h>
#include <TinyGPS++.h>

// --- CONFIGURACIÓN DE PINES (Serial 2) ---
#define GPS_RX_PIN D6  // Pin que lee los datos del NEO-6M
#define GPS_TX_PIN -1  // ¡EL TRUCO! -1 deshabilita la transmisión y libera el pin

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