#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"
#include "moist.h"
#include "gps_modulo.h" // <-- 1. IMPORTAMOS TU NUEVO ARCHIVO GPS

// Estructura de datos unificada que viajará por el aire hacia la aplicación e IA
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       
  float pos_Y;       
  float yaw_angle;   
  float distancia;   
  float gps_lat;     // <-- 2. AGREGAMOS CAMPOS DE GPS A LA TRAMA UNIFICADA
  float gps_lng;     
  uint8_t anomaly;   
} SensorPayload;

SensorPayload datosAMandar;

// Dirección MAC de tu placa receptora (La que obtuvimos en el paso anterior)
uint8_t macReceptor[] = {0x78, 0x1C, 0x3C, 0xDA, 0x3D, 0xB8}; 
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r[ESP-NOW] Estado de envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito al entregar" : "Fallo en entrega");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TRANSMISOR TELECOM UNIFICADO ===");

  // Inicializar periféricos modulares
  inicializarUltrasonico();
  inicializarGPS(); // <-- 3. INICIALIZAMOS EL GPS (Se adaptará según el switch)

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, macReceptor, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallo al agregar al receptor");
    return;
  }
}

void loop() {
  datosAMandar.timestamp = millis();
  
  // 1. Lectura del ultrasónico (Real)
  datosAMandar.distancia = obtenerDistancia();

  // 2. Lectura del GPS (Si está en false, jalará las coordenadas del Tec fijas)
  GPSData infoGPS = obtenerDatosGPS();
  datosAMandar.gps_lat = infoGPS.latitud;
  datosAMandar.gps_lng = infoGPS.longitud;

  // 3. Simulación temporal del IMU para pruebas
  datosAMandar.pos_X = 0.0; 
  datosAMandar.pos_Y = 0.0;
  datosAMandar.yaw_angle = 0.0;
  datosAMandar.anomaly = (datosAMandar.distancia < 15.0 && datosAMandar.distancia > 0) ? 1 : 0;

  // Monitor serie local
  Serial.printf("Distancia: %.2f cm | Lat: %.6f | Lng: %.6f\n", 
                datosAMandar.distancia, datosAMandar.gps_lat, datosAMandar.gps_lng);

  // 4. Enviar paquete unificado por el aire
  esp_now_send(macReceptor, (uint8_t *) &datosAMandar, sizeof(datosAMandar));
  
  delay(500); 
}