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
  
  // Verificamos que el paquete que llegó tenga el tamaño exacto de nuestra estructura
  if (len == sizeof(SensorPayload)) {
    
    // Copiamos los bytes crudos a nuestra estructura para decodificarlos
    memcpy(&datosRecibidos, incomingData, sizeof(datosRecibidos));
    
    // Imprimimos los datos decodificados en el Monitor Serie
    Serial.print("Tiempo: "); 
    Serial.print(datosRecibidos.timestamp);
    Serial.print(" ms | X: "); 
    Serial.print(datosRecibidos.pos_X, 2); 
    Serial.print(" | Y: "); 
    Serial.print(datosRecibidos.pos_Y, 2);
    Serial.print(" | Yaw: "); 
    Serial.print(datosRecibidos.yaw_angle, 2);
    Serial.print(" | Alerta: "); 
    Serial.println(datosRecibidos.anomaly);
    
  } else {
    Serial.println("Error: Tamaño de paquete incorrecto.");
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