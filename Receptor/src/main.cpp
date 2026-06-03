#include <Arduino.h>

// ==============================================================================
// 1. CÓDIGO DEL NUEVO RECEPTOR (XIAO S3 con ESP-NOW + LoRa)
// Se activa automáticamente si eliges env:receptor_xiao_s3 en PlatformIO
// ==============================================================================
#if defined(ARDUINO_SEEED_XIAO_ESP32S3)

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SPI.h>
#include <RH_RF69.h>

// Pines LoRa para el XIAO S3
#define RFM69_CS   D1
#define RFM69_INT  D7

RH_RF69 rf69(RFM69_CS, RFM69_INT);

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
  float env_soil_moist;       
  float pwr_voltage;          
  float pwr_current;          
  uint8_t anomaly;   
  uint8_t vision_obj_id;      
  uint8_t vision_confidence;  
} SensorPayload;

SensorPayload rxData;

uint32_t lastPacketTime = 0;
int32_t e2eLatency = 0;
float estimatedSNR = 0.0;
int16_t currentRSSI = -100;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  uint32_t rxTime = millis();

  if (len == sizeof(SensorPayload)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    
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

    // Output CSV indicando que llegó por Wi-Fi (ESP-NOW)
    Serial.printf("ESPNOW,%lu,%.1f,%.6f,%.6f,%.1f,%.0f,%.0f,%.1f,%.2f,%.0f,%d,%d,%d,%d,%.1f\n",
                  rxData.timestamp, rxData.distance, rxData.gps_lat, rxData.gps_lng, 
                  rxData.env_temp, rxData.env_hum, rxData.env_pres, rxData.env_soil_moist,
                  rxData.pwr_voltage, rxData.pwr_current, rxData.vision_obj_id, 
                  rxData.vision_confidence, e2eLatency, currentRSSI, estimatedSNR);
    
    lastPacketTime = rxTime;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Forzamos al ESP32 a esperar a que abras la terminal
  while(!Serial) {
      delay(10); 
  }
  delay(1000);
  
  // Encabezados del CSV (Nueva columna para saber el protocolo)
  Serial.println("Protocol,Timestamp,Distance,Lat,Lng,Temp,Hum,Pressure,SoilMoist,Bat_V,Bat_mA,VisionID,VisionConf,Latency_ms,RSSI,SNR_dB");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (rf69.init()) {
    rf69.setFrequency(915.0);
    rf69.setTxPower(20, true);
  } else {
    Serial.println("Error: RFM69 no detectado. Revisa pines y puente D7.");
  }
}

void loop() {
  // Escucha de LoRa
  if (rf69.available()) {
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf69.recv(buf, &len)) {
      if (len == sizeof(SensorPayload)) {
          memcpy(&rxData, buf, sizeof(SensorPayload));
          currentRSSI = rf69.lastRssi();
          
          // Output CSV indicando que llegó por Radio (LORA)
          Serial.printf("LORA,%lu,%.1f,%.6f,%.6f,%.1f,%.0f,%.0f,%.1f,%.2f,%.0f,%d,%d,N/A,%d,N/A\n",
                        rxData.timestamp, rxData.distance, rxData.gps_lat, rxData.gps_lng, 
                        rxData.env_temp, rxData.env_hum, rxData.env_pres, rxData.env_soil_moist,
                        rxData.pwr_voltage, rxData.pwr_current, rxData.vision_obj_id, 
                        rxData.vision_confidence, currentRSSI);
                        
          lastPacketTime = millis();
      }
    }
  }

  if (millis() - lastPacketTime > 5000 && lastPacketTime != 0) {
    Serial.println("--- ALERT: LINK LOST ---");
    lastPacketTime = 0; 
  }
  delay(10); 
}

// ==============================================================================
// 2. TU CÓDIGO VIEJO DE DSP (ESP32 Clásico de Steren)
// Se activa automáticamente si eliges el entorno viejo (ej. env:dsp_esp32_clasico)
// ==============================================================================
#else

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

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

uint32_t lastPacketTime = 0;
int32_t e2eLatency = 0;
float estimatedSNR = 0.0;
int16_t currentRSSI = -100;

// Versión antigua de la librería ESP-NOW
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  uint32_t rxTime = millis();

  if (len == sizeof(SensorPayload)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    
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
    Serial.printf("Error: Payload size mismatch. Expected: %d bytes, Received: %d bytes\n", sizeof(SensorPayload), len);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Timestamp,Distance,Lat,Lng,Temp,Hum,Pressure,SoilMoist,Bat_V,Bat_mA,VisionID,VisionConf,Latency_ms,RSSI,SNR_dB");

  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (millis() - lastPacketTime > 5000 && lastPacketTime != 0) {
    Serial.println("--- ALERT: LINK LOST ---");
    lastPacketTime = 0; 
  }
  delay(100); 
}

#endif