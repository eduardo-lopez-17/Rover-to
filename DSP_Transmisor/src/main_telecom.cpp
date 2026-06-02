#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonico.h"  
#include "soil_moisture_module.h"
#include "gps_modulo.h" 
#include "bme280_modulo.h" 
#include "sim800_modulo.h" 
#include "rfm69_module.h" 
#include "ina219_module.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// --- OLED DISPLAY CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PRESENCE DETECTION LOGIC ---
const float PRESENCE_THRESHOLD_CM = 60.0; // Distancia para despertar la pantalla
const uint32_t DISPLAY_TIMEOUT_MS = 5000; // Tiempo que la pantalla se queda encendida (5 seg)
uint32_t lastPresenceTime = 0;

// --- FUNCIÓN PARA ACTUALIZAR LA PANTALLA LOCALMENTE ---
void updateLocalDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE); 
  
  display.setCursor(0, 0);
  display.print("--- NODE STATUS ---");

  display.setCursor(0, 15);
  display.printf("T:%.1fC H:%.0f%% P:%.0f", txData.env_temp, txData.env_hum, txData.env_pres);
  
  display.setCursor(0, 30);
  display.printf("PWR: %.1fV %.0fmA", txData.pwr_voltage, txData.pwr_current);

  display.setCursor(0, 45);
  display.printf("Dist: %.1f cm", txData.distance);
  
  display.display(); 
}

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
  float env_soil_moist; 
  float pwr_voltage;        
  float pwr_current;          
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

  // Inicialización de OLED en el setup()
  if(!display.begin(0x3C, true)) {
    Serial.println("Error: OLED not found");
  } else {
    display.clearDisplay();
    display.setCursor(0,20);
    display.println("Transmitter Ready");
    display.display();
    delay(3000); // Mostrar mensaje inicial por 3 segundos
  }


  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TELECOM TRANSMITTER INITIALIZED ===");

  Wire.begin(); // SDA=21, SCL=22

  // Initialize modular peripherals
  inicializarUltrasonico();
  inicializarGPS(); 
  inicializarBME(); 
  inicializarCelular(); 
  initRFM69();
  initINA219();

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

  PowerData infoPower = getPowerData();
  txData.pwr_voltage = infoPower.bus_voltage;
  txData.pwr_current = infoPower.current_mA;


  // --- LÓGICA DE AHORRO DE ENERGÍA (WAKE-ON-APPROACH) ---
  
  // 1. Verificar si hay alguien cerca (ignorando lecturas de 0 que a veces dan error)
  if (txData.distance > 0.1 && txData.distance < PRESENCE_THRESHOLD_CM) {
      lastPresenceTime = millis(); // Reiniciamos el cronómetro
  }

  // 2. Decidir si la pantalla debe estar encendida o apagada
  if (millis() - lastPresenceTime < DISPLAY_TIMEOUT_MS) {
      // Alguien está cerca o se acaba de ir (aún no se acaba el tiempo)
      updateLocalDisplay();
  } else {
      // Nadie cerca, apagar la pantalla para ahorrar batería
      display.clearDisplay();
      display.display();
  }

  // ... (Sigue tu código de transmisión por ESP-NOW y Ubidots) ...

  // 3. Fast Local Transmission (ESP-NOW to OLED)
  esp_now_send(receiverMac, (uint8_t *) &txData, sizeof(txData));
  
  // 4. Medium Range Transmission (RFM69 up to 800 meters)
  sendRFData((uint8_t *) &txData, sizeof(txData));

  // 5. Slow Cloud Transmission (Every 10 seconds via Cellular)
  if (millis() - lastCellularSendTime > 10000) {
      
      // Build JSON Payload for Ubidots (Using English keys)
      String payload = "{";
      payload += "\"temperature\":" + String(txData.env_temp) + ",";
      payload += "\"humidity\":" + String(txData.env_hum) + ",";
      payload += "\"pressure\":" + String(txData.env_pres) + ",";
      payload += "\"distance\":" + String(txData.distance) + ",";
      payload += "\"battery_v\":" + String(txData.pwr_voltage) + ",";
      payload += "\"current_ma\":" + String(txData.pwr_current) + ",";
      
      // Ubidots Map formatting context:
      payload += "\"gps\":{\"value\":1, \"context\":{\"lat\":" + String(txData.gps_lat, 6) + ", \"lng\":" + String(txData.gps_lng, 6) + "}}";
      payload += "}";
      
      enviarDatosNube(payload);
      lastCellularSendTime = millis();
  }
  
  delay(500); // Base loop delay
}