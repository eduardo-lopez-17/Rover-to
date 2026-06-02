#include "soil_moisture_module.h"

// --- VALORES DE CALIBRACIÓN (Ajustar físicamente) ---
// Estos valores son ejemplos para el ADC de 12-bits del ESP32
const int AIR_VALUE = 3500;   // Valor crudo al estar seco en el aire
const int WATER_VALUE = 1500; // Valor crudo al sumergir la punta en agua

void initSoilMoisture() {
#if USE_SOIL_SENSOR
    Serial.println("[SOIL] Soil moisture sensor initialized.");
    // Asegurar que leemos a la máxima resolución de 12 bits (0-4095)
    analogReadResolution(12);
#else
    Serial.println("[SOIL] Module disabled by software (DNP).");
#endif
}

float getSoilMoisture() {
#if USE_SOIL_SENSOR
    int rawValue = analogRead(SOIL_PIN);
    
    // Mapeamos el valor crudo a un porcentaje (0% a 100%)
    // La función map() de Arduino no soporta floats nativamente, así que mapeamos enteros
    long percentage = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);
    
    // Filtro de seguridad: Si la tierra está EXTREMADAMENTE seca o mojada, 
    // los valores pueden salirse del rango 0-100. Esto lo recorta.
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    return (float)percentage;
#else
    // Si el módulo está apagado, simulamos un 45.5% de humedad en la tierra
    return 45.5; 
#endif
}