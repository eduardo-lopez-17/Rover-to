#include "gps_modulo.h"

// Inicializar los objetos globales de forma segura
#if USAR_GPS
    TinyGPSPlus gps;
    HardwareSerial gpsSerial(2);

    unsigned long lastPrint = 0; // Para controlar la frecuencia de impresión en consola
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

void TareaGPSCode(void * pvParameters) {
  for(;;) {
    // 1. Feed the GPS data to the library as fast as it comes in
    while (Serial2.available() > 0) {
      gps.encode(Serial2.read());
    }

    // 2. Only print the data once every 1000 milliseconds (1 second)
    if (millis() - lastPrint > 1000) {
      if (gps.location.isValid()) {
        Serial.print("Lat: "); Serial.println(gps.location.lat(), 6);
        Serial.print("Lng: "); Serial.println(gps.location.lng(), 6);
        Serial.println("----------------------"); // Visual separator
      } else {
        Serial.println("Buscando satelites... (Asegurate de que el GPS parpadea azul)");
      }
      lastPrint = millis(); // Reset the timer
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to FreeRTOS
  }
}