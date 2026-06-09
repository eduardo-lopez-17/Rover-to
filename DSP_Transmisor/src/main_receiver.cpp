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
/* Link monitor — tracks last packet time for each channel */
static uint32_t s_last_rx_espnow_ms;
static uint32_t s_last_rx_rfm69_ms;
static bool     s_link_warned;

static void check_link(void)
{
    uint32_t now = millis();
    /* Only warn after at least one packet has been received */
    bool espnow_lost = s_last_rx_espnow_ms > 0 &&
                       (now - s_last_rx_espnow_ms) > LINK_TIMEOUT_MS;
    bool rfm69_lost  = s_last_rx_rfm69_ms > 0 &&
                       (now - s_last_rx_rfm69_ms)  > LINK_TIMEOUT_MS;

    if ((espnow_lost || rfm69_lost) && !s_link_warned) {
        s_link_warned = true;
        Serial.printf("[LINK] WARNING: TX signal lost (ESP-NOW: %s, RFM69: %s)\n",
                      espnow_lost ? "LOST" : "ok",
                      rfm69_lost  ? "LOST" : "ok");
    } else if (!espnow_lost && !rfm69_lost && s_link_warned) {
        s_link_warned = false;
        Serial.println("[LINK] TX signal restored");
    }
}

#if USE_ESPNOW
static volatile bool s_espnow_pending;
static SensorPayload s_espnow_buf;

static void on_data_recv(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len != sizeof(SensorPayload))
        return;
    memcpy((void *)&s_espnow_buf, data, sizeof(s_espnow_buf));
    s_espnow_pending    = true;
    s_last_rx_espnow_ms = millis();
}

static void espnow_init(void)
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] ERROR: init failed");
        return;
    }

#if USE_ENCRYPTION
    esp_now_set_pmk((const uint8_t *)AES_KEY_16);

    /* Add TX as encrypted peer so we can decrypt incoming packets.
     * Fill TX_MAC in board_config.h from the TX's Serial output at boot. */
    static const uint8_t k_tx_mac[] = TX_MAC;
    esp_now_peer_info_t  tx_peer    = {};
    memcpy(tx_peer.peer_addr, k_tx_mac, 6);
    tx_peer.encrypt = true;
    memcpy(tx_peer.lmk, AES_KEY_16, 16);
    esp_now_add_peer(&tx_peer);
    Serial.println("[ESP-NOW] OK — AES-128 enabled, listening");
#else
    Serial.println("[ESP-NOW] OK — listening");
#endif

    esp_now_register_recv_cb(on_data_recv);
}
#endif /* USE_ESPNOW */

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

#if USE_ESPNOW
    espnow_init();
#else
    Serial.println("[ESP-NOW] disabled (DNP)");
#endif

    rfm69_init();
    rfm69_self_test();

    Serial.println("[SETUP] Complete. Waiting for packets...");
    Serial.println("----------------------------------------");
}

void loop(void)
{
    /* Drain ESP-NOW pending flag — safe to print here (single task) */
#if USE_ESPNOW
    if (s_espnow_pending) {
        s_espnow_pending = false;
        SensorPayload d;
        memcpy(&d, (const void *)&s_espnow_buf, sizeof(d));
        print_payload(&d, "ESP-NOW");
    }
#endif

    /* Poll RFM69 — non-blocking */
    uint8_t buf[sizeof(SensorPayload)];
    uint8_t len = sizeof(buf);
    if (rfm69_recv(buf, &len)) {
        if (len == sizeof(SensorPayload)) {
            SensorPayload d;
            memcpy(&d, buf, sizeof(d));
            s_last_rx_rfm69_ms = millis();
            print_payload(&d, "RFM69");
        } else {
            Serial.printf("[RFM69] WARNING: unexpected packet size %u\n", len);
        }
    }

    check_link();

    /* Print "waiting" every 5 s so we know the receiver is alive */
    static uint32_t s_last_alive_ms;
    if (millis() - s_last_alive_ms >= 5000) {
        s_last_alive_ms = millis();
        Serial.printf("[RX] waiting... t=%lus  (ESP-NOW last: %lus ago, RFM69 last: %lus ago)\n",
                      (unsigned long)(millis() / 1000),
                      s_last_rx_espnow_ms ? (unsigned long)((millis() - s_last_rx_espnow_ms) / 1000) : 0UL,
                      s_last_rx_rfm69_ms  ? (unsigned long)((millis() - s_last_rx_rfm69_ms)  / 1000) : 0UL);
    }
}
