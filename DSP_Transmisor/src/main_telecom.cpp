#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"  
#include "moist.h"
#include "gps_modulo.h" 
#include "bme280_modulo.h" 
#include "sim800_modulo.h" // <-- 1. IMPORTAMOS EL CELULAR

// Estructura de datos unificada
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       
  float pos_Y;       
  float yaw_angle;   
  float distancia;   
  float gps_lat;     
  float gps_lng;     
  float temp_amb;    
  float hum_amb;     
  float pres_amb;    
  uint8_t anomaly;   
} SensorPayload;

SensorPayload datosAMandar;

uint8_t macReceptor[] = {0x78, 0x1C, 0x3C, 0xDA, 0x3D, 0xB8}; 
esp_now_peer_info_t peerInfo;

// Temporizador para no saturar el celular
uint32_t ultimoEnvioCelular = 0; 

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Callback de ESP-NOW (comentado para no ensuciar la consola)
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TRANSMISOR TELECOM UNIFICADO ===");

  Wire.begin(); // SDA=21, SCL=22

  inicializarUltrasonico();
  inicializarGPS(); 
  inicializarBME(); 
  inicializarCelular(); // <-- 2. INICIALIZAMOS EL CELULAR

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, macReceptor, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  datosAMandar.timestamp = millis();
  
  // 1. Recolección de datos modulares
  datosAMandar.distancia = obtenerDistancia(); 

  GPSData infoGPS = obtenerDatosGPS();
  datosAMandar.gps_lat = infoGPS.latitud;
  datosAMandar.gps_lng = infoGPS.longitud;

  BMEData infoBME = obtenerDatosBME(); 
  datosAMandar.temp_amb = infoBME.temperatura;
  datosAMandar.hum_amb = infoBME.humedad;
  datosAMandar.pres_amb = infoBME.presion;

  datosAMandar.pos_X = 0.0; 
  datosAMandar.pos_Y = 0.0;
  datosAMandar.yaw_angle = 0.0;
  datosAMandar.anomaly = (datosAMandar.distancia < 15.0 && datosAMandar.distancia > 0) ? 1 : 0;

  // 2. Transmisión local rápida (ESP-NOW a la OLED)
  esp_now_send(macReceptor, (uint8_t *) &datosAMandar, sizeof(datosAMandar));

// 3. Transmisión Lenta a la Nube (Cada 10 segundos)
  if (millis() - ultimoEnvioCelular > 10000) {
      
      // Armamos el JSON con la sintaxis exacta para Ubidots
      String payload = "{";
      payload += "\"temperatura\":" + String(datosAMandar.temp_amb) + ",";
      payload += "\"humedad\":" + String(datosAMandar.hum_amb) + ",";
      payload += "\"presion\":" + String(datosAMandar.pres_amb) + ",";
      payload += "\"distancia\":" + String(datosAMandar.distancia) + ",";
      
      // Formato especial de Ubidots para que se grafique en el mapa:
      payload += "\"gps\":{\"value\":1, \"context\":{\"lat\":" + String(datosAMandar.gps_lat, 6) + ", \"lng\":" + String(datosAMandar.gps_lng, 6) + "}}";
      payload += "}";
      
      enviarDatosNube(payload);
      ultimoEnvioCelular = millis();
  }
  
  delay(500); // Retardo base del ciclo
}