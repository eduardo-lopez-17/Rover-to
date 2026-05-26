#include "bh1750_modulo.h"

#if USAR_LUZ
    BH1750 luxometro;
#endif

void inicializarLuz() {
#if USAR_LUZ
    Wire.begin(I2C_SDA, I2C_SCL);
    // Inicializar en modo continuo de alta resolución
    if (luxometro.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
        Serial.println("[BH1750] Sensor de luz inicializado.");
    } else {
        Serial.println("[BH1750] ERROR: No se encontró el sensor.");
    }
#else
    Serial.println("[BH1750] Módulo de luz deshabilitado (DNP).");
#endif
}

float obtenerLuz() {
#if USAR_LUZ
    return luxometro.readLightLevel();
#else
    return 150.0; // Valor simulado si el sensor está apagado
#endif
}