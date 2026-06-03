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
  #include "vision_sd_module.h"

  // --- OLED DISPLAY CONFIGURATION ---
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  #define OLED_RESET -1 
  Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  // --- PRESENCE & VISION LOGIC ---
  const float PRESENCE_THRESHOLD_CM = 60.0; 
  const uint32_t DISPLAY_TIMEOUT_MS = 5000; 
  uint32_t lastPresenceTime = 0;

  uint32_t lastPhotoTime = 0;                 
  const uint32_t PHOTO_COOLDOWN_MS = 15000;  


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

  SensorPayload txData;

  // 4. pantalla
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

    // Acomodamos distancia y coordenadas en los últimos dos renglones
    display.setCursor(0, 43);
    display.printf("Dist: %.1f cm", txData.distance);
    
    display.setCursor(0, 54);
    display.printf("GPS:%.4f,%.4f", txData.gps_lat, txData.gps_lng);
    
    display.display(); 
  }

  // Receiver MAC Address
  uint8_t receiverMac[] = {0x90, 0x70, 0x69, 0x12, 0xBE, 0x48}; 
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

    // 1. PRIMERO FIJAMOS LOS PINES DEL BUS I2C
    Wire.begin(D4, D5);
    delay(100); 

    // 2. AHORA SÍ INICIALIZAMOS LA PANTALLA Y LOS SENSORES
    if(!display.begin(0x3C, true)) {
      Serial.println("Error: OLED not found");
    } else {
      display.clearDisplay();
      display.setCursor(0,20);
      display.println("Transmitter Ready");
      display.display();
      delay(3000); 
    }

    inicializarUltrasonico();
    inicializarGPS(); 
    inicializarBME(); 
    inicializarCelular(); 
    initRFM69();
    initINA219();
    
    // 3. LA CÁMARA HASTA EL FINAL (Usando el interruptor DNP)
    #if USE_VISION_SD
        initVisionAndSD(); 
    #endif

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


  // --- LÓGICA DE AHORRO DE ENERGÍA Y FOTOGRAFÍA (WAKE-ON-APPROACH) ---
    if (txData.distance > 0.1 && txData.distance < PRESENCE_THRESHOLD_CM) {
        lastPresenceTime = millis(); 
        
        // ¿Han pasado 15 segundos desde la última foto?
        if (millis() - lastPhotoTime > PHOTO_COOLDOWN_MS || lastPhotoTime == 0) {
            Serial.println("[ALERTA] Movimiento detectado. Tomando foto de prueba...");
            
            // ---> ¡AQUÍ DEBE ESTAR LA LÍNEA! <---
            // Actualizamos el reloj de inmediato para evitar el spam, aunque la cámara falle.
            lastPhotoTime = millis(); 
            
            // Intentamos tomar la foto
            if(takeAndSavePhoto()) {
                txData.vision_obj_id = 1; // Encendemos la alarma
            }
        }
    }

    // Decidir si la pantalla OLED debe estar encendida o apagada
    if (millis() - lastPresenceTime < DISPLAY_TIMEOUT_MS) {
        updateLocalDisplay();
    } else {
        display.clearDisplay();
        display.display();
    }


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
        payload += "\"vision_alert\":" + String(txData.vision_obj_id) + ",";
        
        // Ubidots Map formatting context:
        payload += "\"gps\":{\"value\":1, \"context\":{\"lat\":" + String(txData.gps_lat, 6) + ", \"lng\":" + String(txData.gps_lng, 6) + "}}";
        payload += "}";
        
        enviarDatosNube(payload);
        lastCellularSendTime = millis();
        txData.vision_obj_id = 0;
    }
    
    delay(500); // Base loop delay
  }