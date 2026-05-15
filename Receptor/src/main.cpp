#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// 1. LA TRAMA DE DATOS (Debe ser idéntica a la del Transmisor)
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       
  float pos_Y;       
  float yaw_angle;   
  uint8_t anomaly;   
} SensorPayload;

// Creamos una variable para guardar lo que llegue
SensorPayload datosRecibidos;

// 2. FUNCIÓN DE RECEPCIÓN (Versión clásica para PlatformIO Core v2.x)
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(SensorPayload)) {
    memcpy(&datosRecibidos, incomingData, sizeof(datosRecibidos));
    
    // Formato CSV: más ligero y fácil de leer para Python
    // timestamp, pos_X, pos_Y, yaw_angle, anomaly
    Serial.print(datosRecibidos.timestamp);
    Serial.print(",");
    Serial.print(datosRecibidos.pos_X, 4); // 4 decimales para mayor precisión en integración
    Serial.print(",");
    Serial.print(datosRecibidos.pos_Y, 4);
    Serial.print(",");
    Serial.print(datosRecibidos.yaw_angle, 2);
    Serial.print(",");
    Serial.println(datosRecibidos.anomaly);
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Receptor de Telemetría Iniciado. Esperando datos...");
}

void loop() {
  // Todo se maneja en la interrupción OnDataRecv
  delay(1000);
}