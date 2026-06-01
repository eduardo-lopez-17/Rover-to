#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> 

// --- OLED DISPLAY CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PIN_SDA_RX 21
#define PIN_SCL_RX 22

// 1. UNIFIED DATA PAYLOAD (Must match the TX exactly!)
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
  uint8_t vision_obj_id;  
  uint8_t vision_confidence;  
} SensorPayload;

SensorPayload rxData;

// --- PERFORMANCE VARIABLES ---
uint32_t lastPacketTime = 0;
int32_t e2eLatency = 0;
float estimatedSNR = 0.0;
int16_t currentRSSI = -100;

// --- DISPLAY UPDATE FUNCTION ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE); 
  
  // Line 1: Status & Anomalies (Y=0)
  display.setCursor(0, 0);
  if (rxData.anomaly == 1) {
    display.print("STATUS: !COLLISION!");
  } else {
    display.print("STATUS: OK");
  }

  // Line 2: Environment Variables BME280 (Y=12)
  display.setCursor(0, 12);
  display.printf("T:%.1fC H:%.0f%% P:%.0f", rxData.env_temp, rxData.env_hum, rxData.env_pres);
  
  // Line 3: AI Vision / XIAO (Y=24)
  display.setCursor(0, 24);
  if (rxData.vision_obj_id == 0) {
      display.print("CAM: CLEAR");
  } else {
      display.printf("CAM: Obj %d (%d%%)", rxData.vision_obj_id, rxData.vision_confidence);
  }

  // Line 4: GPS (Y=36)
  display.setCursor(0, 36);
  if (rxData.gps_lat == 0.0) {
      display.print("GPS: Searching...");
  } else {
      display.printf("Lat:%.4f Lng:%.4f", rxData.gps_lat, rxData.gps_lng);
  }
  
  // Line 5: Network Metrics (Y=52)
  display.setCursor(0, 52);
  display.printf("Lat:%dms SNR:%.0fdB", e2eLatency, estimatedSNR);
  
  display.display(); 
}

// 2. RECEIVE CALLBACK (Fixed for ESP32 Core 3.x)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  uint32_t rxTime = millis();

  if (len == sizeof(SensorPayload)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    
    // Network Calculations
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

    // CSV Output for Digital Twin
    Serial.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%.1f\n",
                  rxData.timestamp, rxData.pos_x, rxData.pos_y, 
                  rxData.yaw_angle, rxData.distance, rxData.gps_lat, 
                  rxData.gps_lng, rxData.env_temp, rxData.env_hum, 
                  rxData.env_pres, rxData.anomaly, rxData.vision_obj_id, 
                  rxData.vision_confidence, e2eLatency, currentRSSI, estimatedSNR);
    
    lastPacketTime = rxTime;
  } else {
    Serial.println("Error: Payload size mismatch. Check the struct.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA_RX, PIN_SCL_RX);

  if(!display.begin(0x3C, true)) {
    Serial.println("Error: OLED not found");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,20);
    display.println("Searching for");
    display.println("Transmitter...");
    display.display();
  }
  
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (lastPacketTime != 0) {
    updateDisplay();
  }
  
  if (millis() - lastPacketTime > 2000 && lastPacketTime != 0) {
    display.clearDisplay();
    display.setCursor(0,20);
    display.print("ALERT: LINK LOST");
    display.display();
  }
  delay(100); 
}