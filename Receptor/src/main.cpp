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

// 1. LA TRAMA DE DATOS UNIFICADA (Debe ser idéntica al transmisor)
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;        
  float pos_Y;        
  float yaw_angle;   
  float distancia;   // <-- Agregado para igualar al TX
  float gps_lat;     // <-- Agregado para el GPS
  float gps_lng;     // <-- Agregado para el GPS
  uint8_t anomaly;   
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
  
  // Línea 1: Título / Estatus (Y=0)
  display.setCursor(0, 0);
  if (datosRecibidos.anomaly == 1) {
    display.print("STATUS: !ANOMALIA!");
  } else {
    display.print("STATUS: TELEMETRIA OK");
  }
  
  // Línea 2: IMU (Y=12)
  display.setCursor(0, 12);
  display.print("AccX:"); display.print(datosRecibidos.pos_X, 1);
  display.print(" Y:"); display.print(datosRecibidos.pos_Y, 1);
  
  // Línea 3: Orientación y Distancia (Y=24)
  display.setCursor(0, 24);
  display.print("Yaw:"); display.print(datosRecibidos.yaw_angle, 1);
  display.print(" Dist:"); display.print(datosRecibidos.distancia, 0); display.print("cm");

  // Línea 4: GPS (Y=36)
  display.setCursor(0, 36);
  if (datosRecibidos.gps_lat == 0.0 && datosRecibidos.gps_lng == 0.0) {
      display.print("GPS: Sin senal/Apagado");
  } else {
      display.print("Lat:"); display.print(datosRecibidos.gps_lat, 4);
      display.setCursor(0, 46); // Un poco más abajo para la longitud si no cabe
      display.print("Lng:"); display.print(datosRecibidos.gps_lng, 4);
  }
  
  // Línea 5: Métricas de Red (Y=56)
  display.setCursor(0, 56);
  display.print("Lat:"); display.print(latenciaE2E); display.print("ms");
  display.print(" SNR:"); display.print(snr_estimado, 0); display.print("dB");
  
  display.display(); 
}

// 2. FUNCIÓN DE RECEPCIÓN
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  uint32_t tiempoRecepcion = millis();

  // Verificamos que el tamaño del paquete sea el correcto
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

    // Salida CSV actualizada para incluir todos los datos
    Serial.print(datosRecibidos.timestamp); Serial.print(",");
    Serial.print(datosRecibidos.pos_X, 4); Serial.print(",");
    Serial.print(datosRecibidos.pos_Y, 4); Serial.print(",");
    Serial.print(datosRecibidos.yaw_angle, 2); Serial.print(",");
    Serial.print(datosRecibidos.distancia, 2); Serial.print(",");
    Serial.print(datosRecibidos.gps_lat, 6); Serial.print(",");
    Serial.print(datosRecibidos.gps_lng, 6); Serial.print(",");
    Serial.print(datosRecibidos.anomaly); Serial.print(",");
    Serial.print(latenciaE2E); Serial.print(",");
    Serial.print(rssi); Serial.print(",");
    Serial.println(snr_estimado, 1);
    
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
    display.println("Buscando Nodo");
    display.println("Transmisor...");
    display.display();
  }
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  
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