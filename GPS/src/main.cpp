#include <Arduino.h>
#include <TinyGPS++.h> // Better for parsing data

// Define the RX and TX pins for Serial 2
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
uint32_t lastPrint = 0;

// Task Handle for FreeRTOS
TaskHandle_t TareaGPS;

void TareaGPSCode(void * pvParameters) {
  for(;;) {
    // 1. Feed the GPS data to the library as fast as it comes in
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    // 2. Only print the data once every 1000 milliseconds (1 second)
    if (millis() - lastPrint > 1000) {
      if (gps.location.isValid()) {
        Serial.print("Lat: "); Serial.println(gps.location.lat(), 6);
        Serial.print("Lng: "); Serial.println(gps.location.lng(), 6);
        Serial.println("----------------------"); // Visual separator
      }
      lastPrint = millis(); // Reset the timer
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to FreeRTOS
  }
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

  // Create the task to run on Core 1 (leaving Core 0 for Comms later)
  xTaskCreatePinnedToCore(TareaGPSCode, "GPS_Task", 5000, NULL, 1, &TareaGPS, 1);
}

void loop() {
  // Empty - FreeRTOS handles the task
}