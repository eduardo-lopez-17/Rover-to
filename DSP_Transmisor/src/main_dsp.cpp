#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "moist.h"  

// --- DEFINICIÓN EXPLÍCITA DE PINES I2C (Lolin S3) ---
#define PIN_SDA 8
#define PIN_SCL 9

// --- UMBRAL DE COLISIÓN (m/s^2) ---
#define UMBRAL_CHOQUE 15.0

// Sensor comentado para evitar bloqueos por falta de hardware
// Adafruit_BNO08x bno08x;
// sh2_SensorValue_t sensorValue;

// 1. DEFINICIÓN DE LA TRAMA DE DATOS
typedef struct __attribute__((packed))
{
    uint32_t timestamp;
    float pos_X;
    float pos_Y;
    float yaw_angle;
    uint8_t anomaly;
} SensorPayload;

SensorPayload misDatosDSP;

// Dirección MAC del Receptor
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

TaskHandle_t TareaDSP;
TaskHandle_t TareaComms;

// --- TAREA 1: PROCESAMIENTO/SIMULACIÓN DE SEÑALES (Núcleo 1) ---
void TareaDSPCode(void *pvParameters)
{
    float t = 0.0; // Variable de tiempo matemático para las señales sintéticas

    for (;;)
    {
            misDatosDSP.timestamp = millis();

        // --- MODO SIMULACIÓN MATEMÁTICA ---
        // Generamos una señal armónica para simular aceleraciones en X e Y (trayectoria curva)
        // Agregamos un pequeño ruido blanco simulado usando random()
        float ruidoX = (random(-100, 100) / 1000.0);
        float ruidoY = (random(-100, 100) / 1000.0);

        misDatosDSP.pos_X = (8.0 * sin(t)) + ruidoX;
        misDatosDSP.pos_Y = (8.0 * cos(t)) + ruidoY;

        // El ángulo de yaw va cambiando progresivamente simulando una rotación de 0 a 360 grados
        misDatosDSP.yaw_angle = fmod(t * 57.2958, 360.0); // 57.2958 pasa radianes a grados

        // --- INYECCIÓN CONTROLADA DE ANOMALÍAS (CHOQUES ARTIFICIALES) ---
        // Cada ~15 segundos forzamos un pico de aceleración que supere el UMBRAL_CHOQUE
        if (int(t) % 15 == 0 && int(t) != 0 && (t - int(t) < 0.2)) 
        {
            misDatosDSP.pos_X = 18.5; // Supera los 15.0 m/s^2
            misDatosDSP.anomaly = 1;  // ¡Choque simulado!
                }
                else
                {
            // Verificación del umbral por código original
            if (abs(misDatosDSP.pos_X) > UMBRAL_CHOQUE || abs(misDatosDSP.pos_Y) > UMBRAL_CHOQUE) {
                misDatosDSP.anomaly = 1;
            } else {
                misDatosDSP.anomaly = 0;
            }
        }

        t += 0.02; // Incremento del paso del tiempo matemático

        // Muestreo controlado estrictamente a 100Hz (10 ms de delay)
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// --- TAREA 2: TELEMETRÍA ESP-NOW (Corre en el Núcleo 0) ---
void TareaCommsCode(void *pvParameters)
{
    for (;;)
    {
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&misDatosDSP, sizeof(misDatosDSP));

        // Transmitimos a 100Hz para no perder resolución en el Gemelo Digital
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);

    // NOTA DE SIMULACIÓN: Desactivamos la inicialización física del sensor I2C 
    // para evitar el congelamiento del sistema en ausencia de hardware.
    Serial.println("[MODO SIMULACIÓN ACTIVADO] Generando señales DSP por software.");

    /* Wire.begin(PIN_SDA, PIN_SCL);
    if (!bno08x.begin_I2C(0x4A, &Wire)) { 
        Serial.println("¡Fallo al encontrar el BNO085!");
        while (1) { delay(10); }
    }
    bno08x.enableReport(SH2_LINEAR_ACCELERATION, 10000);
    bno08x.enableReport(SH2_ARVR_STABILIZED_RV, 10000);
    */

    // 4. Inicializar ESP-NOW
    WiFi.mode(WIFI_STA);
    esp_now_init();
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    misDatosDSP.pos_X = 0;
    misDatosDSP.pos_Y = 0;
    misDatosDSP.yaw_angle = 0;
    misDatosDSP.anomaly = 0;

    // 5. Iniciar FreeRTOS (Mantenemos la arquitectura multi-núcleo intacta)
    xTaskCreatePinnedToCore(TareaCommsCode, "Comms", 10000, NULL, 1, &TareaComms, 0);
    xTaskCreatePinnedToCore(TareaDSPCode, "DSP", 10000, NULL, 1, &TareaDSP, 1);
}

void loop()
{
    // FreeRTOS manda
}