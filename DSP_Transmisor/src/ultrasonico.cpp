#include "ultrasonico.h"

#if ULTRASONIC_USE_I2C
    #include <Wire.h> // Necesario para hablar por I2C
#endif

void inicializarUltrasonico() {
#if USE_ULTRASONIC
    Serial.println("[ULTRASONIC] Initializing...");
    
    #if ULTRASONIC_USE_I2C
        // En modo I2C no necesitamos declarar pines, el bus Wire ya se inició en el main
        Serial.println("[ULTRASONIC] Configured for I2C mode (0x57).");
    #else
        // Modo clásico para la placa Steren
        pinMode(TRIG_PIN, OUTPUT);
        pinMode(ECHO_PIN, INPUT);
        digitalWrite(TRIG_PIN, LOW);
        Serial.println("[ULTRASONIC] Configured for Trig/Echo mode.");
    #endif
#else
    Serial.println("[ULTRASONIC] Module disabled by software (DNP).");
#endif
}

float obtenerDistancia() {
#if USE_ULTRASONIC
    
    #if ULTRASONIC_USE_I2C
        // --- LECTURA POR I2C (XIAO S3) ---
        uint32_t data = 0;
        
        // 1. Mandar comando para disparar el sonido (0x01)
        Wire.beginTransmission(ULTRASONIC_I2C_ADDR);
        Wire.write(0x01);
        if (Wire.endTransmission() != 0) {
            return 0.0; // Error de conexión
        }
        
        delay(120); // Esperar a que el sonido vaya y regrese (120ms recomendado)
        
        // 2. Leer 3 bytes de respuesta
        Wire.requestFrom(ULTRASONIC_I2C_ADDR, 3);
        if (Wire.available() == 3) {
            data  = Wire.read() << 16; // Byte 1 (Más significativo)
            data |= Wire.read() << 8;  // Byte 2
            data |= Wire.read();       // Byte 3 (Menos significativo)
            
            // Fórmula oficial del fabricante para convertir esos 3 bytes a cm
            float distancia_cm = (float)data / 10000.0; 
            
            // Filtro para evitar lecturas locas
            if(distancia_cm > 400.0) return 400.0;
            return distancia_cm;
        }
        return 0.0;

    #else
        // --- LECTURA CLÁSICA (Steren ESP32) ---
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        long duracion = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout de 30ms
        if (duracion == 0) return 0.0;
        
        float distancia_cm = duracion * 0.034 / 2.0;
        if(distancia_cm > 400.0) return 400.0;
        return distancia_cm;
    #endif

#else
    return 45.0; // Valor simulado si el DNP está apagado (false)
#endif
}