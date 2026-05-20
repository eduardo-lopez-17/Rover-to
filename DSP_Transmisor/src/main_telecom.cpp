#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"

// Estructura de datos idéntica a la que usarás con la IA/Gemelo Digital
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       // Simulado de momento
  float pos_Y;       // Simulado de momento
  float yaw_angle;   // Simulado de momento
  float distancia;   // El dato REAL de tu ultrasónico
  uint8_t anomaly;   // Alerta simulada
} SensorPayload;

SensorPayload datosAMandar;

// Dirección MAC de tu SEGUNDO ESP32 (El receptor). 
// Reemplázala con la dirección real de tu otra placa (ver abajo cómo obtenerla)
uint8_t macReceptor[] = {0x78, 0x1C, 0x3C, 0xDA, 0x3D, 0xB8};
esp_now_peer_info_t peerInfo;

// Callback para verificar si el dato realmente llegó al receptor
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r[ESP-NOW] Estado de envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito al entregar" : "Fallo en entrega");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TRANSMISOR TELECOM: INICIADO ===");

  // Inicializar tu sensor ultrasónico modular
  inicializarUltrasonico();

  // Configurar Wi-Fi en modo Estación (necesario para ESP-NOW)
  WiFi.mode(WIFI_STA);
  Serial.print("[Wi-Fi] Dirección MAC de esta placa: ");
  Serial.println(WiFi.macAddress()); // Copia este valor cuando uses la otra placa

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Registrar el callback de envío
  esp_now_register_send_cb(OnDataSent);
  
  // Registrar el dispositivo receptor
  memcpy(peerInfo.peer_addr, macReceptor, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallo al agregar al receptor");
    return;
  }
}

void loop() {
  // 1. Lectura del sensor real
  datosAMandar.timestamp = millis();
  datosAMandar.distancia = obtenerDistancia();

  // 2. Simulación de datos del IMU ausente (para pruebas de la trama)
  datosAMandar.pos_X = 1.23; 
  datosAMandar.pos_Y = -0.45;
  datosAMandar.yaw_angle = 45.0;
  datosAMandar.anomaly = (datosAMandar.distancia < 15.0 && datosAMandar.distancia > 0) ? 1 : 0;

  // Imprimir en monitor serie local para corroborar
  Serial.printf("Distancia: %.2f cm | Anomaly: %d\n", datosAMandar.distancia, datosAMandar.anomaly);

  // 3. Enviar la estructura completa por el aire
  esp_err_t resultado = esp_now_send(macReceptor, (uint8_t *) &datosAMandar, sizeof(datosAMandar));
  
  delay(500); // Muestreo a 2Hz cómodo para leer en pantalla
}