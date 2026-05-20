#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "moist.h"  // Moisture sensor header not defining Moist in this project

// --- DEFINICIÓN EXPLÍCITA DE PINES I2C (Lolin S3) ---
#define PIN_SDA 8
#define PIN_SCL 9

// --- UMBRAL DE COLISIÓN (m/s^2) ---
#define UMBRAL_CHOQUE 15.0

// --- INICIALIZACIÓN DEL SENSOR BNO085 ---
Adafruit_BNO08x bno08x;
sh2_SensorValue_t sensorValue;

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

// --- TAREA 1: PROCESAMIENTO DE SEÑALES (Corre en el Núcleo 1) ---
void TareaDSPCode(void *pvParameters)
{
    for (;;)
    {
        if (bno08x.getSensorEvent(&sensorValue))
        {

            misDatosDSP.timestamp = millis();

            switch (sensorValue.sensorId)
            {
            case SH2_LINEAR_ACCELERATION:
                misDatosDSP.pos_X = sensorValue.un.linearAcceleration.x;
                misDatosDSP.pos_Y = sensorValue.un.linearAcceleration.y;

                // --- DETECCIÓN DE ANOMALÍAS (COLISIÓN) ---
                // Calculamos la magnitud absoluta de la aceleración para ver si superó el umbral
                if (abs(misDatosDSP.pos_X) > UMBRAL_CHOQUE || abs(misDatosDSP.pos_Y) > UMBRAL_CHOQUE)
                {
                    misDatosDSP.anomaly = 1; // ¡Choque detectado!
                }
                else
                {
                    misDatosDSP.anomaly = 0; // Operación normal
                }
                break;

            case SH2_ARVR_STABILIZED_RV:
                float qr = sensorValue.un.arvrStabilizedRV.real;
                float qi = sensorValue.un.arvrStabilizedRV.i;
                float qj = sensorValue.un.arvrStabilizedRV.j;
                float qk = sensorValue.un.arvrStabilizedRV.k;

                float ysqr = qj * qj;
                float t3 = +2.0 * (qr * qk + qi * qj);
                float t4 = +1.0 - 2.0 * (ysqr + qk * qk);
                misDatosDSP.yaw_angle = atan2(t3, t4) * 180.0 / PI;
                break;
            }
        }

        // Muestreo controlado a 100Hz
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

    // 1. Forzamos los pines I2C específicos de tu placa antes de iniciar el sensor
    Wire.begin(PIN_SDA, PIN_SCL);

    // 2. Iniciar BNO085 pasándole el bus I2C configurado
    if (!bno08x.begin_I2C(0x4A, &Wire))
    { // 0x4A es la dirección I2C común del BNO085
        Serial.println("¡Fallo al encontrar el BNO085! Revisa los cables SDA y SCL.");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("BNO085 detectado correctamente.");

    // 3. Configurar reportes a 10,000 uS (100 Hz)
    bno08x.enableReport(SH2_LINEAR_ACCELERATION, 10000);
    bno08x.enableReport(SH2_ARVR_STABILIZED_RV, 10000);

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

    // 5. Iniciar FreeRTOS
    xTaskCreatePinnedToCore(TareaCommsCode, "Comms", 10000, NULL, 1, &TareaComms, 0);
    xTaskCreatePinnedToCore(TareaDSPCode, "DSP", 10000, NULL, 1, &TareaDSP, 1);
}

void loop()
{
    // FreeRTOS manda
}