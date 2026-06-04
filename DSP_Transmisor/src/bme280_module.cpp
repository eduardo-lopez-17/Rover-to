#include "bme280_module.h"
#include "board_config.h"

#if USE_BME280
#include <Adafruit_BME280.h>
#include <Wire.h>
static Adafruit_BME280 s_bme;
static bool            s_bme_ok;
#endif

void bme280_init(void)
{
#if USE_BME280
    s_bme_ok = s_bme.begin(BME280_I2C_ADDR, &Wire);
    if (!s_bme_ok)
        Serial.println("[BME280] ERROR: not found on I2C — check address/wiring");
    else
        Serial.println("[BME280] OK");
#else
    Serial.println("[BME280] disabled (DNP)");
#endif
}

BmeData bme280_read(void)
{
    BmeData d = {0.0f, 0.0f, 0.0f};
#if USE_BME280
    if (s_bme_ok) {
        d.temp_c        = s_bme.readTemperature();
        d.humidity_pct  = s_bme.readHumidity();
        d.pressure_hpa  = s_bme.readPressure() / 100.0f;
    }
#else
    /* Simulated nominal values for bench testing without sensor */
    d.temp_c       = 25.0f;
    d.humidity_pct = 50.0f;
    d.pressure_hpa = 1013.0f;
#endif
    return d;
}
