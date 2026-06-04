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
    s_display.clearDisplay();
    s_display.setTextSize(1);
    s_display.setTextColor(SH110X_WHITE);
    s_display.setCursor(0, 0);
    s_display.print("--- NODE STATUS ---");
    s_display.setCursor(0, 15);
    s_display.printf("T:%.1fC H:%.0f%% P:%.0f",
                     d->env_temp_c, d->env_humidity_pct, d->env_pressure_hpa);
    s_display.setCursor(0, 30);
    s_display.printf("PWR: %.1fV %.0fmA", d->pwr_voltage_v, d->pwr_current_ma);
    s_display.setCursor(0, 43);
    s_display.printf("Dist: %.1f cm", d->distance_cm);
    s_display.setCursor(0, 54);
    s_display.printf("GPS:%.4f,%.4f", d->gps_lat, d->gps_lng);
    s_display.display();
}

static void oled_blank(void)
{
    if (!s_display_ok)
        return;
    s_display.clearDisplay();
    s_display.display();
}

/* =========================================================================
 * ESP-NOW
 * ========================================================================= */
static const uint8_t  k_receiver_mac[] = {0x90, 0x70, 0x69, 0x12, 0xBE, 0x48};
static esp_now_peer_info_t s_peer;

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    (void)mac_addr;
    (void)status;
}

static void espnow_init(void)
{
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_register_send_cb(on_data_sent);
    memcpy(s_peer.peer_addr, k_receiver_mac, 6);
    s_peer.channel = 0;
    s_peer.encrypt = false;
    esp_now_add_peer(&s_peer);
    Serial.println("[ESP-NOW] OK");
}

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
    /* Native USB on XIAO S3 — wait for terminal to connect before logging */
    while (!Serial)
        delay(10);
#endif
    Serial.println("=== TELECOM TRANSMITTER v2 ===");

    /* Start I2C bus with a per-transaction timeout.
     * Without this, a missing sensor can hang Wire indefinitely. */
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setTimeOut(100);

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
    rfm69_self_test(); /* prints ambient RSSI to confirm RF chain is alive */

    /* Cellular — disabled by default (USE_SIM800 = 0 in board_config.h) */
    sim800_init();

    /* Vision + SD — disabled by default (USE_VISION_SD = 0 in board_config.h) */
#if USE_VISION_SD
    vision_sd_init();
#endif

    espnow_init();
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

    /* BNO085 — yaw fed into payload; pos_x/pos_y require odometry (encoders) */
    ImuData imu               = bno085_read();
    tx_data.yaw_angle         = imu.valid ? imu.yaw_deg : 0.0f;
    tx_data.pos_x             = 0.0f; /* TODO: dead-reckoning with wheel encoders */
    tx_data.pos_y             = 0.0f;
    tx_data.vision_confidence = 0;

    /* Collision flag */
    tx_data.anomaly = (tx_data.distance_cm > 0.1f &&
                       tx_data.distance_cm < 15.0f) ? 1 : 0;

    /* --- Wake-on-approach + photo capture ------------------------------- */
    if (tx_data.distance_cm > 0.1f &&
        tx_data.distance_cm < PRESENCE_THRESHOLD_CM) {
        s_last_presence_ms = millis();
        if (millis() - s_last_photo_ms > PHOTO_COOLDOWN_MS ||
            s_last_photo_ms == 0) {
            s_last_photo_ms = millis(); /* stamp before capture to avoid spam */
#if USE_VISION_SD
            if (vision_capture_photo())
                tx_data.vision_obj_id = 1;
#endif
        }
    }

    /* --- Display --------------------------------------------------------- */
    if (millis() - s_last_presence_ms < DISPLAY_TIMEOUT_MS)
        oled_update(&tx_data);
    else
        oled_blank();

    /* --- Transmit: ESP-NOW (short range) -------------------------------- */
    esp_now_send(k_receiver_mac, (const uint8_t *)&tx_data, sizeof(tx_data));

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

    delay(500);
}
