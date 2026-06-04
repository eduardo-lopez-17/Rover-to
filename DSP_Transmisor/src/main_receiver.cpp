#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>

#include "board_config.h"
#include "pin_map.h"
#include "rfm69_module.h"

/* =========================================================================
 * Payload — must be byte-identical to the TX side in main_telecom.cpp.
 * ========================================================================= */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    float    pos_x;
    float    pos_y;
    float    yaw_angle;
    float    distance_cm;
    float    gps_lat;
    float    gps_lng;
    float    env_temp_c;
    float    env_humidity_pct;
    float    env_pressure_hpa;
    float    env_soil_moisture_pct;
    float    pwr_voltage_v;
    float    pwr_current_ma;
    uint8_t  anomaly;
    uint8_t  vision_obj_id;
    uint8_t  vision_confidence;
} SensorPayload;

/* =========================================================================
 * Print helpers
 * ========================================================================= */
static void print_payload(const SensorPayload *d, const char *channel)
{
    Serial.printf("\n[%s] t=%lums\n", channel, (unsigned long)d->timestamp);
    Serial.printf("  POS    x=%.2f m  y=%.2f m  yaw=%.1f deg\n",
                  d->pos_x, d->pos_y, d->yaw_angle);
    Serial.printf("  GPS    lat=%.6f  lng=%.6f\n",
                  d->gps_lat, d->gps_lng);
    Serial.printf("  ENV    T=%.1fC  H=%.0f%%  P=%.1f hPa  soil=%.0f%%\n",
                  d->env_temp_c, d->env_humidity_pct,
                  d->env_pressure_hpa, d->env_soil_moisture_pct);
    Serial.printf("  PWR    %.2fV  %.1f mA\n",
                  d->pwr_voltage_v, d->pwr_current_ma);
    Serial.printf("  DIST   %.1f cm%s\n",
                  d->distance_cm, d->anomaly ? "  *** COLLISION ALERT ***" : "");
    if (d->vision_obj_id)
        Serial.printf("  VISION obj=%u  conf=%u%%\n",
                      d->vision_obj_id, d->vision_confidence);
}

/* =========================================================================
 * ESP-NOW — callback runs in WiFi task; defer print to main loop to avoid
 * concurrent Serial writes with RFM69 polling.
 * ========================================================================= */
static volatile bool s_espnow_pending;
static SensorPayload s_espnow_buf;

static void on_data_recv(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len != sizeof(SensorPayload)) {
        return; /* size mismatch — silently drop */
    }
    memcpy((void *)&s_espnow_buf, data, sizeof(s_espnow_buf));
    s_espnow_pending = true;
}

static void espnow_init(void)
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] ERROR: init failed");
        return;
    }
    esp_now_register_recv_cb(on_data_recv);
    Serial.println("[ESP-NOW] OK — listening");
}

/* =========================================================================
 * setup / loop
 * ========================================================================= */
void setup(void)
{
    Serial.begin(115200);
#ifdef ARDUINO_USB_MODE
    while (!Serial)
        delay(10);
    delay(300);
#endif
    Serial.println("=== TELECOM RECEIVER ===");

    espnow_init();

    rfm69_init();
    rfm69_self_test();

    Serial.println("[SETUP] Complete. Waiting for packets...");
    Serial.println("----------------------------------------");
}

void loop(void)
{
    /* Drain ESP-NOW pending flag — safe to print here (single task) */
    if (s_espnow_pending) {
        s_espnow_pending = false;
        SensorPayload d;
        memcpy(&d, (const void *)&s_espnow_buf, sizeof(d));
        print_payload(&d, "ESP-NOW");
    }

    /* Poll RFM69 — non-blocking */
    uint8_t buf[sizeof(SensorPayload)];
    uint8_t len = sizeof(buf);
    if (rfm69_recv(buf, &len)) {
        if (len == sizeof(SensorPayload)) {
            SensorPayload d;
            memcpy(&d, buf, sizeof(d));
            print_payload(&d, "RFM69");
        } else {
            Serial.printf("[RFM69] WARNING: unexpected packet size %u\n", len);
        }
    }
}
