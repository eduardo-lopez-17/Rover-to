#include "ina219_module.h"
#include "board_config.h"

#if USE_INA219
#include <Adafruit_INA219.h>
#include <Wire.h>
static Adafruit_INA219 s_ina(INA219_I2C_ADDR);
static bool            s_ina_ok;
#endif

void ina219_init(void)
{
#if USE_INA219
    s_ina_ok = s_ina.begin();
    if (!s_ina_ok) {
        Serial.println("[INA219] ERROR: chip not found on I2C");
        return;
    }
    s_ina.setCalibration_32V_2A();
    Serial.println("[INA219] OK");
#else
    Serial.println("[INA219] disabled (DNP)");
#endif
}

PowerData ina219_read(void)
{
    PowerData d = {0.0f, 0.0f, 0.0f};
#if USE_INA219
    if (s_ina_ok) {
        d.bus_voltage_v = s_ina.getBusVoltage_V();
        d.current_ma    = s_ina.getCurrent_mA();
        d.power_mw      = s_ina.getPower_mW();
    }
#else
    /* Simulated nominal values for digital twin testing */
    d.bus_voltage_v = 12.0f;
    d.current_ma    = 250.5f;
    d.power_mw      = 3006.0f;
#endif
    return d;
}
