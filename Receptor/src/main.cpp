#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> 

// --- CONFIGURACIÓN DE LA PANTALLA OLED SH110X ---
#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA 64
#define OLED_RESET     -1 
Adafruit_SH1106G display = Adafruit_SH1106G(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, OLED_RESET);

#define PIN_SDA_REC 21
#define PIN_SCL_REC 22

// 1. LA TRAMA DE DATOS UNIFICADA (¡Idéntica a la final del transmisor!)
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;        
  float pos_Y;        
  float yaw_angle;   
  float distancia;   
  float gps_lat;     
  float gps_lng;     
  float temp_amb;    /
  float hum_amb;     
  float pres_amb;    
  uint8_t anomaly;   
  uint8_t vision_id_objeto;  // <-- IA del XIAO
  uint8_t vision_confianza;  // <-- IA del XIAO
} SensorPayload;

SensorPayload datosRecibidos;

// Variables de rendimiento
uint32_t tiempoUltimoPaquete = 0;
int32_t latenciaE2E = 0;
float snr_estimado = 0;
int16_t rssi = -100;

// --- FUNCIÓN PARA ACTUALIZAR LA PANTALLA ---
void actualizarPantalla() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE); 
  
  // Línea 1: Estatus y Anomalías (Y=0)
  display.setCursor(0, 0);
  if (datosRecibidos.anomaly == 1) {
    display.print("STATUS: !CHOQUE!");
  } else {
    display.print("STATUS: OK");
  }

  // Línea 2: Variables Ambientales BME280 (Y=12)
  display.setCursor(0, 12);
  display.printf("T:%.1fC H:%.0f%% P:%.0f", datosRecibidos.temp_amb, datosRecibidos.hum_amb, datosRecibidos.pres_amb);
  
  // Línea 3: Visión Artificial / XIAO (Y=24)
  display.setCursor(0, 24);
  if (datosRecibidos.vision_id_objeto == 0) {
      display.print("CAM: Despejado");
  } else {
      display.printf("CAM: Obj %d (%d%%)", datosRecibidos.vision_id_objeto, datosRecibidos.vision_confianza);
  }

  // Línea 4: GPS (Y=36)
  display.setCursor(0, 36);
  if (datosRecibidos.gps_lat == 0.0) {
      display.print("GPS: Buscando...");
  } else {
      display.printf("Lat:%.4f Lng:%.4f", datosRecibidos.gps_lat, datosRecibidos.gps_lng);
  }
  
  // Línea 5: Métricas de Red (Y=52)
  display.setCursor(0, 52);
  display.printf("Lat:%dms SNR:%.0fdB", latenciaE2E, snr_estimado);
  
  display.display(); 
}

// 2. FUNCIÓN DE RECEPCIÓN
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  uint32_t tiempoRecepcion = millis();

  if (len == sizeof(SensorPayload)) {
    memcpy(&datosRecibidos, incomingData, sizeof(datosRecibidos));
    
    // Cálculos de Red
    latenciaE2E = tiempoRecepcion - datosRecibidos.timestamp;
    if(latenciaE2E < 0) latenciaE2E = 0;

    wifi_ap_record_t info_enlace;
    if (esp_wifi_sta_get_ap_info(&info_enlace) == ESP_OK) {
        rssi = info_enlace.rssi;
    } else {
        rssi = -50; 
    }

    float pisoRuido = -96.0;
    snr_estimado = (float)rssi - pisoRuido;
    if (snr_estimado < 0) snr_estimado = 0;

    // Salida CSV para el Gemelo Digital
    Serial.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%.1f\n",
                  datosRecibidos.timestamp, datosRecibidos.pos_X, datosRecibidos.pos_Y, 
                  datosRecibidos.yaw_angle, datosRecibidos.distancia, datosRecibidos.gps_lat, 
                  datosRecibidos.gps_lng, datosRecibidos.temp_amb, datosRecibidos.hum_amb, 
                  datosRecibidos.pres_amb, datosRecibidos.anomaly, datosRecibidos.vision_id_objeto, 
                  datosRecibidos.vision_confianza, latenciaE2E, rssi, snr_estimado);
    
    tiempoUltimoPaquete = tiempoRecepcion;
  } else {
    Serial.println("Error: Tamaño de paquete discordante. Revisa el struct.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA_REC, PIN_SCL_REC);

  if(!display.begin(0x3C, true)) {
    Serial.println("Error: No se encontro pantalla OLED");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,20);
    display.println("Buscando Transmisor...");
    display.display();
  }
  
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (tiempoUltimoPaquete != 0) {
    actualizarPantalla();
  }
  
  if (millis() - tiempoUltimoPaquete > 2000 && tiempoUltimoPaquete != 0) {
    display.clearDisplay();
    display.setCursor(0,20);
    display.print("ALERTA: ENLACE PERDIDO");
    display.display();
  }
  delay(100); 
}