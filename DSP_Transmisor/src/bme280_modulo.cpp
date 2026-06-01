#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"  
#include "moist.h"
#include "gps_modulo.h" 
#include "bme280_modulo.h" // <-- 1. IMPORTAMOS EL BME280

// Estructura de datos unificada (¡OJO! Esta debe ser idéntica en el receptor)
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       
  float pos_Y;       
  float yaw_angle;   
  float distancia;   
  float gps_lat;     
  float gps_lng;     
  float temp_amb;    // <-- 2. AÑADIMOS TEMPERATURA
  float hum_amb;     // <-- 2. AÑADIMOS HUMEDAD
  float pres_amb;    // <-- 2. AÑADIMOS PRESIÓN
  uint8_t anomaly;   
} SensorPayload;

SensorPayload datosAMandar;

uint8_t macReceptor[] = {0x78, 0x1C, 0x3C, 0xDA, 0x3D, 0xB8}; 
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Comentado para no saturar la terminal, pero puedes descomentarlo para debug
  // Serial.print("\r[ESP-NOW] Estado: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "Fallo");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TRANSMISOR TELECOM UNIFICADO ===");

  // Iniciar bus I2C (Steren usa 21 SDA, 22 SCL por defecto)
  Wire.begin();

  inicializarUltrasonico();
  inicializarGPS(); 
  inicializarBME(); // <-- 3. INICIALIZAMOS EL BME280

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
  
  // Lecturas
  datosAMandar.distancia = obtenerDistancia(); // O poner a 0.0 temporalmente

  GPSData infoGPS = obtenerDatosGPS();
  datosAMandar.gps_lat = infoGPS.latitud;
  datosAMandar.gps_lng = infoGPS.longitud;

  BMEData infoBME = obtenerDatosBME(); // <-- 4. LEEMOS EL SENSOR
  datosAMandar.temp_amb = infoBME.temperatura;
  datosAMandar.hum_amb = infoBME.humedad;
  datosAMandar.pres_amb = infoBME.presion;

  // Simulación temporal del IMU
  datosAMandar.pos_X = 0.0; 
  datosAMandar.pos_Y = 0.0;
  datosAMandar.yaw_angle = 0.0;
  datosAMandar.anomaly = (datosAMandar.distancia < 15.0 && datosAMandar.distancia > 0) ? 1 : 0;

  // Monitor serie local
  Serial.printf("T: %.1fC | H: %.1f%% | P: %.1fhPa | Lat: %.6f | Lng: %.6f\n", 
                datosAMandar.temp_amb, datosAMandar.hum_amb, datosAMandar.pres_amb, 
                datosAMandar.gps_lat, datosAMandar.gps_lng);

  esp_now_send(macReceptor, (uint8_t *) &datosAMandar, sizeof(datosAMandar));
  
  delay(500); 
}