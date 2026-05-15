#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// 1. DEFINICIÓN DE LA TRAMA DE DATOS
typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  float pos_X;       // Posición calculada (DSP)
  float pos_Y;       // Posición calculada (DSP)
  float yaw_angle;   // Orientación del BNO055
  uint8_t anomaly;   // 1 si hay choque, 0 normal
} SensorPayload;

SensorPayload misDatosDSP;

// Dirección MAC del Receptor (Steren NodeMCU) - FF's es Broadcast para pruebas
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

// 2. CREAMOS LOS NOMBRES DE LAS TAREAS DE FREERTOS
TaskHandle_t TareaDSP;
TaskHandle_t TareaComms;

// --- TAREA 1: PROCESAMIENTO DE SEÑALES (Corre en el Núcleo 1) ---
void TareaDSPCode( void * pvParameters ){
  for(;;){
    // Aquí irá tu código de DSP:
    // 1. Leer aceleración del BNO055
    // 2. Aplicar filtro digital (pasabajas/media móvil)
    // 3. Doble integración para sacar Posición X y Y
    
    // Simulamos que ya hicimos los cálculos:
    misDatosDSP.timestamp = millis();
    misDatosDSP.pos_X += 0.01; // Simulando movimiento
    misDatosDSP.pos_Y += 0.02;
    misDatosDSP.yaw_angle = 90.0;
    
    // Nos esperamos 10 milisegundos exactos (Muestreo a 100Hz)
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

// --- TAREA 2: TELEMETRÍA ESP-NOW (Corre en el Núcleo 0) ---
void TareaCommsCode( void * pvParameters ){
  for(;;){
    // Disparamos el paquete procesado por el aire
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &misDatosDSP, sizeof(misDatosDSP));
    
    // Transmitimos a 10Hz (cada 100ms) para no saturar el receptor
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inicializamos Radio
  WiFi.mode(WIFI_STA);
  esp_now_init();
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Inicializamos datos en cero
  misDatosDSP.pos_X = 0;
  misDatosDSP.pos_Y = 0;

  // 3. INICIAMOS LAS TAREAS EN SUS RESPECTIVOS NÚCLEOS
  xTaskCreatePinnedToCore(TareaCommsCode, "Comunicaciones", 10000, NULL, 1, &TareaComms, 0); // Núcleo 0
  xTaskCreatePinnedToCore(TareaDSPCode, "Procesamiento", 10000, NULL, 1, &TareaDSP, 1);    // Núcleo 1
}

void loop() {
  // En FreeRTOS, el loop se queda vacío. Las tareas hacen todo el trabajo.
}