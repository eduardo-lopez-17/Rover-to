#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// --- UNIFIED DATA PAYLOAD (Must match the TX exactly!) ---
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
  float env_soil_moist;       // Sensor de humedad de suelo
  float pwr_voltage;          // Batería (INA219)
  float pwr_current;          // Consumo (INA219)
  uint8_t anomaly;   
  uint8_t vision_obj_id;      // IA del XIAO
  uint8_t vision_confidence;  // IA del XIAO
} SensorPayload;

SensorPayload rxData;

// --- PERFORMANCE VARIABLES ---
uint32_t lastPacketTime = 0;
int32_t e2eLatency = 0;
float estimatedSNR = 0.0;
int16_t currentRSSI = -100;

// --- ESP-NOW RECEIVE CALLBACK (ESP32 Core 3.x format) ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  uint32_t rxTime = millis();

  if (len == sizeof(SensorPayload)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    
    // 1. Network Calculations (Latency, RSSI, SNR)
    e2eLatency = rxTime - rxData.timestamp;
    if(e2eLatency < 0) e2eLatency = 0;

    wifi_ap_record_t linkInfo;
    if (esp_wifi_sta_get_ap_info(&linkInfo) == ESP_OK) {
        currentRSSI = linkInfo.rssi;
    } else {
        currentRSSI = -50; 
    }

    float noiseFloor = -96.0;
    estimatedSNR = (float)currentRSSI - noiseFloor;
    if (estimatedSNR < 0) estimatedSNR = 0;

    // 2. CSV Output for PC Logging / Digital Twin
    // Format: Timestamp, Dist, Lat, Lng, Temp, Hum, Pres, Soil, BatV, BatmA, VisionID, VisionConf, Latency, RSSI, SNR
    Serial.printf("%lu,%.1f,%.6f,%.6f,%.1f,%.0f,%.0f,%.1f,%.2f,%.0f,%d,%d,%d,%d,%.1f\n",
                  rxData.timestamp, 
                  rxData.distance, 
                  rxData.gps_lat, 
                  rxData.gps_lng, 
                  rxData.env_temp, 
                  rxData.env_hum, 
                  rxData.env_pres, 
                  rxData.env_soil_moist,
                  rxData.pwr_voltage,
                  rxData.pwr_current,
                  rxData.vision_obj_id, 
                  rxData.vision_confidence, 
                  e2eLatency, 
                  currentRSSI, 
                  estimatedSNR);
    
    lastPacketTime = rxTime;
  } else {
    // Si el tamaño no coincide, significa que el TX y RX tienen structs diferentes
    Serial.printf("Error: Payload size mismatch. Expected: %d bytes, Received: %d bytes\n", sizeof(SensorPayload), len);
  }
}

void setup() {
  // Inicializamos a alta velocidad para que el CSV salga rápido hacia la PC
  Serial.begin(115200);
  delay(1000);
  
  // Imprimimos los encabezados del CSV una sola vez al arrancar
  Serial.println("Timestamp,Distance,Lat,Lng,Temp,Hum,Pressure,SoilMoist,Bat_V,Bat_mA,VisionID,VisionConf,Latency_ms,RSSI,SNR_dB");

  // Configuración de red (Modo Estación para ESP-NOW)
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // El receptor ya no hace nada en el loop. Todo el trabajo pesado de 
  // desempaquetar e imprimir ocurre automáticamente en la interrupción (OnDataRecv).
  // Solo avisamos si perdimos conexión por más de 5 segundos.
  if (millis() - lastPacketTime > 5000 && lastPacketTime != 0) {
    Serial.println("--- ALERT: LINK LOST ---");
    lastPacketTime = 0; // Evita que se imprima repetitivamente
  }
  delay(100); 
}