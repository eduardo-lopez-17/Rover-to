#include "bh1750_modulo.h"

#if USAR_LUZ
    #include <Wire.h>
    #include <BH1750.h> // La librería oficial que acabamos de instalar

    BH1750 luxometro;
#endif

void inicializarLuz() {
#if USAR_LUZ
    Serial.println("[BH1750] Inicializando...");
    
    // El bus Wire ya se inició en el main_telecom.cpp, aquí solo enganchamos el sensor
    if (luxometro.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
        Serial.println("[BH1750] OK.");
    } else {
        Serial.println("[BH1750] ERROR: No se encontró el sensor");
    }
#else
    Serial.println("[BH1750] Módulo desactivado por software (DNP).");
#endif
}

float obtenerLuz() {
#if USAR_LUZ
    return luxometro.readLightLevel();
#else
    // Dato simulado de luz solar directa para cuando el sensor está apagado
    return 10000.0; 
#endif
}