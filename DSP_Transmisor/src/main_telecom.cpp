#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "board_config.h"
#include "pin_map.h"

#include "bme280_module.h"
#include "bh1750_module.h"
#include "bno085_module.h"
#include "gps_module.h"
#include "ina219_module.h"
#include "rfm69_module.h"
#include "sim800_module.h"
#include "soil_moisture_module.h"
#include "ultrasonic_module.h"
#include "vision_sd_module.h"
#include "audio_module.h"

/* =========================================================================
 * Payload — struct must be byte-identical on TX and RX sides.
 * Any field addition or reorder requires updating both nodes.
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

static SensorPayload tx_data;

/* =========================================================================
 * OLED display
 * ========================================================================= */
static Adafruit_SH1106G s_display(128, 64, &Wire, -1);
static bool             s_display_ok;

static void oled_init(void)
{
    s_display_ok = s_display.begin(OLED_I2C_ADDR, true);
    if (!s_display_ok) {
        Serial.println("[OLED] ERROR: not found — display disabled, all else continues");
        return;
    }
    s_display.clearDisplay();
    s_display.setTextSize(1);
    s_display.setTextColor(SH110X_WHITE);
    s_display.setCursor(0, 20);
    s_display.println("  Transmitter Ready");
    s_display.display();
    delay(2000);
    Serial.println("[OLED] OK");
}

static void oled_update(const SensorPayload *d)
{
    if (!s_display_ok)
        return;

    /* Rotate pages every 3 s so all sensors fit on the 128x64 screen */
    static uint8_t  page;
    static uint32_t page_ms;
    if (millis() - page_ms > 3000) {
        page_ms = millis();
        page    = (page + 1) % 3;
    }

    s_display.clearDisplay();
    s_display.setTextSize(1);
    s_display.setTextColor(SH110X_WHITE);

    /* Header line */
    s_display.setCursor(0, 0);
    if (d->anomaly)
        s_display.print("!! ALERTA COLISION !!");
    else if (d->vision_obj_id)
        s_display.print("!! INTRUSO !! foto SD");
    else
        s_display.printf("TX t=%lus  [%u/3]",
                         (unsigned long)(d->timestamp / 1000), page + 1);

    switch (page) {
    case 0: /* Environment + soil */
        s_display.setCursor(0, 14);
        s_display.printf("T:%.1fC H:%.0f%%", d->env_temp_c, d->env_humidity_pct);
        s_display.setCursor(0, 25);
        s_display.printf("P:%.0fhPa", d->env_pressure_hpa);
        s_display.setCursor(0, 36);
        s_display.printf("Soil: %.0f%%", d->env_soil_moisture_pct);
        s_display.setCursor(0, 47);
        s_display.printf("Dist: %.1f cm", d->distance_cm);
        break;

    case 1: /* Power + IMU */
        s_display.setCursor(0, 14);
        s_display.printf("V: %.2f V", d->pwr_voltage_v);
        s_display.setCursor(0, 25);
        s_display.printf("I: %.1f mA", d->pwr_current_ma);
        s_display.setCursor(0, 36);
        s_display.printf("Yaw: %.1f deg", d->yaw_angle);
        s_display.setCursor(0, 47);
        s_display.printf("X:%.1f Y:%.1f m", d->pos_x, d->pos_y);
        break;

    case 2: /* GPS + vision */
        s_display.setCursor(0, 14);
        s_display.printf("Lat:%.5f", d->gps_lat);
        s_display.setCursor(0, 25);
        s_display.printf("Lng:%.5f", d->gps_lng);
        s_display.setCursor(0, 36);
        s_display.printf("Vision obj:%u", d->vision_obj_id);
        s_display.setCursor(0, 47);
        s_display.printf("Anomaly:%u", d->anomaly);
        break;
    }

    s_display.display();
}

static void oled_blank(void)
{
    if (!s_display_ok)
        //return;
    s_display.clearDisplay();
    s_display.display();
}

/* =========================================================================
 * ESP-NOW
 * ========================================================================= */
#if USE_ESPNOW
static const uint8_t       k_receiver_mac[] = {0x90, 0x70, 0x69, 0x12, 0xBE, 0x48};
static esp_now_peer_info_t s_peer;
static uint8_t             s_espnow_fail_count;

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    (void)mac_addr;
    if (status == ESP_NOW_SEND_SUCCESS) {
        s_espnow_fail_count = 0;
    } else if (++s_espnow_fail_count >= LINK_FAIL_COUNT) {
        Serial.printf("[LINK] WARNING: RX not responding (%u consecutive failures)\n",
                      s_espnow_fail_count);
    }
}

static void espnow_init(void)
{
    WiFi.mode(WIFI_STA);
    esp_now_init();

#if USE_ENCRYPTION
    esp_now_set_pmk((const uint8_t *)AES_KEY_16);
#endif

    esp_now_register_send_cb(on_data_sent);
    memcpy(s_peer.peer_addr, k_receiver_mac, 6);
    s_peer.channel = 0;

#if USE_ENCRYPTION
    s_peer.encrypt = true;
    memcpy(s_peer.lmk, AES_KEY_16, 16);
    Serial.println("[ESP-NOW] OK — AES-128 enabled");
#else
    s_peer.encrypt = false;
    Serial.println("[ESP-NOW] OK");
#endif

    esp_now_add_peer(&s_peer);

    /* Print own MAC so it can be set as TX_MAC on the receiver */
    Serial.printf("[ESP-NOW] TX MAC: %s\n", WiFi.macAddress().c_str());
}
#endif /* USE_ESPNOW */

/* =========================================================================
 * Timers
 * ========================================================================= */
static uint32_t s_last_presence_ms;
static uint32_t s_last_photo_ms;
static uint32_t s_last_cellular_ms;

/* =========================================================================
 * setup
 * ========================================================================= */
void setup(void)
{
    Serial.begin(115200);
#ifdef ARDUINO_USB_MODE
    /* Native USB on XIAO S3 — wait for CDC to connect, then give the host
     * terminal app 300 ms to finish attaching before we start logging. */
    while (!Serial)
        delay(10);
    delay(300);
#endif
    Serial.println("=== TELECOM TRANSMITTER v2 ===");
    Serial.flush();

    /* Start I2C bus with a per-transaction timeout.
     * Without this, a missing sensor can hang Wire indefinitely. */
    Wire.begin();

    /* OLED goes first — it must boot even if every other I2C device is absent */
    oled_init();

    /* Sensors — each init is non-fatal; missing hardware just logs an error */
    bme280_init();
    ina219_init();
    bno085_init();
    ultrasonic_init();
    gps_init();
    bh1750_init();
    soil_init();

    /* Radio */
    rfm69_init();
    rfm69_self_test();

    /* Cellular — disabled by default (USE_SIM800 = 0 in board_config.h) */
    sim800_init();

    /* Vision + SD — disabled by default (USE_VISION_SD = 0 in board_config.h) */
#if USE_VISION_SD
    vision_sd_init();
#endif

    /* Audio — PDM microphones on XIAO ESP32-S3 Sense */
    audio_init();

#if USE_ESPNOW
    espnow_init();
#else
    Serial.println("[ESP-NOW] disabled (DNP)");
#endif
    Serial.println("[SETUP] Complete.");
}

/* =========================================================================
 * loop
 * ========================================================================= */
void loop(void)
{
    tx_data.timestamp = millis();

    /* --- Sensor reads ---------------------------------------------------- */
    tx_data.distance_cm = ultrasonic_read_cm();

    GpsData gps                   = gps_read();
    tx_data.gps_lat               = gps.lat;
    tx_data.gps_lng               = gps.lng;

    BmeData bme                   = bme280_read();
    tx_data.env_temp_c            = bme.temp_c;
    tx_data.env_humidity_pct      = bme.humidity_pct;
    tx_data.env_pressure_hpa      = bme.pressure_hpa;

    PowerData pwr                 = ina219_read();
    tx_data.pwr_voltage_v         = pwr.bus_voltage_v;
    tx_data.pwr_current_ma        = pwr.current_ma;

    tx_data.env_soil_moisture_pct = soil_read_pct();

    /* BNO085 — heading + shock/theft detection */
    ImuData imu               = bno085_read();
    tx_data.yaw_angle         = imu.valid ? imu.yaw_deg : 0.0f;
    tx_data.pos_x             = 0.0f;
    tx_data.pos_y             = 0.0f;
    tx_data.vision_confidence = 0;

    /* anomaly bit-flags: bit0=collision, bit1=shock, bit2=tilt */
    tx_data.anomaly = 0;
    if (tx_data.distance_cm > 0.1f && tx_data.distance_cm < 15.0f)
        tx_data.anomaly |= (1 << 0);
    if (imu.shock_detected)
        tx_data.anomaly |= (1 << 1);
    if (imu.tilt_alert)
        tx_data.anomaly |= (1 << 2);

    /* --- Vision monitor + intruder alert -------------------------------- */
#if USE_VISION_SD
    if (vision_run_monitor()) {
        /* Intruder confirmed by EI — save photo + audio clip */
        if (millis() - s_last_photo_ms > PHOTO_COOLDOWN_MS || s_last_photo_ms == 0) {
            s_last_photo_ms = millis();
            if (vision_capture_photo())
                tx_data.vision_obj_id = 1;
#if USE_AUDIO
            audio_record_to_sd(); /* 5s clip — returns immediately if SD absent */
#endif
        }
        tx_data.anomaly = 1;
        s_last_presence_ms = millis(); /* keep OLED on */
    }
#endif

    /* --- Display --------------------------------------------------------- */
    if (millis() - s_last_presence_ms < DISPLAY_TIMEOUT_MS)
        oled_update(&tx_data);
    else
        oled_blank();

    /* --- Transmit: ESP-NOW (short range) -------------------------------- */
#if USE_ESPNOW
    esp_now_send(k_receiver_mac, (const uint8_t *)&tx_data, sizeof(tx_data));
#endif

    /* --- Transmit: LoRa (medium range) ---------------------------------- */
    rfm69_send((const uint8_t *)&tx_data, sizeof(tx_data));

    /* --- Transmit: Cloud via cellular (rate-limited) -------------------- */
    if (millis() - s_last_cellular_ms > CELLULAR_INTERVAL_MS) {
        String json;
        json  = "{";
        json += "\"temperature\":"  + String(tx_data.env_temp_c)            + ",";
        json += "\"humidity\":"     + String(tx_data.env_humidity_pct)      + ",";
        json += "\"pressure\":"     + String(tx_data.env_pressure_hpa)      + ",";
        json += "\"distance\":"     + String(tx_data.distance_cm)           + ",";
        json += "\"battery_v\":"    + String(tx_data.pwr_voltage_v)         + ",";
        json += "\"current_ma\":"   + String(tx_data.pwr_current_ma)        + ",";
        json += "\"vision_alert\":" + String(tx_data.vision_obj_id)         + ",";
        json += "\"gps\":{\"value\":1,\"context\":{\"lat\":";
        json += String(tx_data.gps_lat, 6) + ",\"lng\":";
        json += String(tx_data.gps_lng, 6) + "}}";
        json += "}";
        sim800_send_cloud(json);
        s_last_cellular_ms    = millis();
        tx_data.vision_obj_id = 0;
    }

    /* --- Serial telemetry (rate-limited to 1 Hz) ------------------------ */
    static uint32_t s_last_print_ms;
    if (millis() - s_last_print_ms >= 1000) {
        s_last_print_ms = millis();
        Serial.printf("\n--- TX t=%lus ---\n", (unsigned long)(tx_data.timestamp / 1000));
#if USE_BME280
        Serial.printf("  ENV    T:%.1fC  H:%.0f%%  P:%.1fhPa\n",
                      tx_data.env_temp_c, tx_data.env_humidity_pct, tx_data.env_pressure_hpa);
#endif
#if USE_INA219
        Serial.printf("  PWR    %.2fV  %.1fmA\n",
                      tx_data.pwr_voltage_v, tx_data.pwr_current_ma);
#endif
#if USE_SOIL
        Serial.printf("  SOIL   %.0f%%\n", tx_data.env_soil_moisture_pct);
#endif
#if USE_ULTRASONIC
        Serial.printf("  DIST   %.1f cm%s\n",
                      tx_data.distance_cm, tx_data.anomaly ? "  !! COLLISION" : "");
#endif
#if USE_BNO085
        Serial.printf("  IMU    yaw:%.1f deg  (valid:%s)\n",
                      tx_data.yaw_angle, tx_data.yaw_angle > 0.01f ? "yes" : "no");
#endif
#if USE_GPS
        Serial.printf("  GPS    %.6f, %.6f\n", tx_data.gps_lat, tx_data.gps_lng);
#endif
#if USE_BH1750
        Serial.printf("  LIGHT  (enabled)\n");
#endif
#if USE_VISION_SD
        if (tx_data.vision_obj_id)
            Serial.printf("  VISION *** INTRUDER DETECTED ***\n");
#endif
    }

    delay(500);
}
