#include "bme280_modulo.h"

#if USE_BME280
    #include <Wire.h>
    #include <Adafruit_Sensor.h>
    #include <Adafruit_BME280.h>
    
    Adafruit_BME280 bme;
#endif

void inicializarBME() {
#if USE_BME280
    Serial.println("[BME280] Inicializando...");
    
    // El BME280 suele tener la dirección I2C 0x76 (a veces 0x77)
    if (!bme.begin(0x76, &Wire)) {
        Serial.println("[BME280] ERROR: No se encontró el sensor I2C");
    } else {
        Serial.println("[BME280] OK.");
    }
#else
    Serial.println("[BME280] Módulo desactivado (DNP).");
#endif
}

BMEData obtenerDatosBME() {
    BMEData data = {0.0, 0.0, 0.0};
#if USE_BME280
    data.temperatura = bme.readTemperature();
    data.humedad = bme.readHumidity();
    data.presion = bme.readPressure() / 100.0F; // Convertir a hPa
#else
    // Datos simulados perfectos para probar la transmisión sin sensor conectado
    data.temperatura = 25.0;
    data.humedad = 50.0;
    data.presion = 1013.0;
#endif
    return data;
}