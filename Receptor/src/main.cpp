#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> // <-- Cambiada la librería aquí

// --- CONFIGURACIÓN DE LA PANTALLA OLED SH110X ---
#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA 64
#define OLED_RESET     -1 
// Inicializamos el objeto específico para el chip SH110X en modo I2C
Adafruit_SH1106G display = Adafruit_SH1106G(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, OLED_RESET);

// --- ASIGNACIÓN DE PINES I2C ESTÁNDAR ESP32 STEREN ---
#define PIN_SDA_REC 21
#define PIN_SCL_REC 22

// 1. LA TRAMA DE DATOS (Idéntica a tu transmisor)
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;        
  float pos_Y;        
  float yaw_angle;   
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
  display.setTextColor(SH110X_WHITE); // Nota: SH110X usa sus propias constantes de color
  
  // Línea 1: Título / Estatus
  display.setCursor(0, 0);
  if (datosRecibidos.anomaly == 1) {
    display.print("STATUS: !ANOMALIA!");
  } else {
    display.print("STATUS: TELEMETRIA OK");
  }
  
  // Línea 2: Datos de Aceleración
  display.setCursor(0, 16);
  display.print("AccX: "); display.print(datosRecibidos.pos_X, 2);
  display.print(" Y: "); display.print(datosRecibidos.pos_Y, 2);
  
  // Línea 3: Ángulo de Orientación (DSP)
  display.setCursor(0, 32);
  display.print("Yaw Angle: "); display.print(datosRecibidos.yaw_angle, 1); display.print(" deg");
  
  // Línea 4: Métricas de Red (Latencia y Calidad de Señal)
  display.setCursor(0, 48);
  display.print("Lat: "); display.print(latenciaE2E); display.print("ms");
  display.print(" SNR: "); display.print(snr_estimado, 0); display.print("dB");
  
  display.display(); // Envía los datos físicos a la pantalla
}

// 2. FUNCIÓN DE RECEPCIÓN (Interrupción de hardware)
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
        rssi = -50; // Simulación de señal excelente en laboratorio
    }

    float pisoRuido = -96.0;
    snr_estimado = (float)rssi - pisoRuido;
    if (snr_estimado < 0) snr_estimado = 0;

    // Salida CSV para Python por el puerto Serie
    Serial.print(datosRecibidos.timestamp); Serial.print(",");
    Serial.print(datosRecibidos.pos_X, 4); Serial.print(",");
    Serial.print(datosRecibidos.pos_Y, 4); Serial.print(",");
    Serial.print(datosRecibidos.yaw_angle, 2); Serial.print(",");
    Serial.print(datosRecibidos.anomaly); Serial.print(",");
    Serial.print(latenciaE2E); Serial.print(",");
    Serial.print(rssi); Serial.print(",");
    Serial.println(snr_estimado, 1);
    
    tiempoUltimoPaquete = tiempoRecepcion;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar bus I2C forzando los pines 21 y 22 para el ESP32 clásico de Steren
  Wire.begin(PIN_SDA_REC, PIN_SCL_REC);

  // Inicializar pantalla OLED SH1106G (La dirección I2C más común para esta librería es 0x3C)
  // Nota: La función en esta librería se llama begin(direccion, switchvcc) al revés que la otra.
  if(!display.begin(0x3C, true)) {
    Serial.println("Error: No se encontro pantalla OLED SH110X");
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
  // Refresco de pantalla controlado fuera de la interrupción a 10 Hz
  if (tiempoUltimoPaquete != 0) {
    actualizarPantalla();
  }
  
  // Watchdog en caso de perder la señal
  if (millis() - tiempoUltimoPaquete > 2000 && tiempoUltimoPaquete != 0) {
    display.clearDisplay();
    display.setCursor(0,20);
    display.print("ALERTA: ENLACE PERDIDO");
    display.display();
  }

  delay(100); 
}