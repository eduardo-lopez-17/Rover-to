#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"  
#include "moist.h"
#include "gps_modulo.h" 
#include "bme280_modulo.h" 
#include "sim800_modulo.h" 

// --- UNIFIED DATA PAYLOAD (Matches the Receiver exactly) ---
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_x;       
  float pos_y;       
  float yaw_angle;   
  float distance;   
  float gps_lat;     
  float gps_lng;     
  float env_temp;    
  float env_hum;     
  float env_pres;    
  uint8_t anomaly;   
  uint8_t vision_obj_id;      // <-- Added for XIAO AI integration
  uint8_t vision_confidence;  // <-- Added for XIAO AI integration
} SensorPayload;

SensorPayload txData;

// Receiver MAC Address
uint8_t receiverMac[] = {0x78, 0x1C, 0x3C, 0xDA, 0x3D, 0xB8}; 
esp_now_peer_info_t peerInfo;

// Timers
uint32_t lastCellularSendTime = 0; 

// --- ESP-NOW CALLBACK ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("\r[ESP-NOW] Send Status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TELECOM TRANSMITTER INITIALIZED ===");

  Wire.begin(); // SDA=21, SCL=22

  // Initialize modular peripherals
  inicializarUltrasonico();
  inicializarGPS(); 
  inicializarBME(); 
  inicializarCelular(); 

  // Network configuration
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  txData.timestamp = millis();
  
  // 1. Data Collection
  txData.distance = obtenerDistancia(); 

  GPSData infoGPS = obtenerDatosGPS();
  txData.gps_lat = infoGPS.latitud;
  txData.gps_lng = infoGPS.longitud;

  BMEData infoBME = obtenerDatosBME(); 
  txData.env_temp = infoBME.temperatura;
  txData.env_hum = infoBME.humedad;
  txData.env_pres = infoBME.presion;

  // 2. Dummy data for DSP and Vision (Until sensors are connected)
  txData.pos_x = 0.0; 
  txData.pos_y = 0.0;
  txData.yaw_angle = 0.0;
  txData.vision_obj_id = 0; 
  txData.vision_confidence = 0;
  
  // Collision/Anomaly logic
  txData.anomaly = (txData.distance < 15.0 && txData.distance > 0) ? 1 : 0;

  // 3. Fast Local Transmission (ESP-NOW to OLED)
  esp_now_send(receiverMac, (uint8_t *) &txData, sizeof(txData));

  // 4. Slow Cloud Transmission (Every 10 seconds via Cellular)
  if (millis() - lastCellularSendTime > 10000) {
      
      // Build JSON Payload for Ubidots (Using English keys)
      String payload = "{";
      payload += "\"temperature\":" + String(txData.env_temp) + ",";
      payload += "\"humidity\":" + String(txData.env_hum) + ",";
      payload += "\"pressure\":" + String(txData.env_pres) + ",";
      payload += "\"distance\":" + String(txData.distance) + ",";
      
      // Ubidots Map formatting context:
      payload += "\"gps\":{\"value\":1, \"context\":{\"lat\":" + String(txData.gps_lat, 6) + ", \"lng\":" + String(txData.gps_lng, 6) + "}}";
      payload += "}";
      
      enviarDatosNube(payload);
      lastCellularSendTime = millis();
  }
  
  delay(500); // Base loop delay
}