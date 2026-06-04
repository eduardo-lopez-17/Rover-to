#include "bh1750_module.h"
#include "board_config.h"

#if USE_BH1750
#include <BH1750.h>
#include <Wire.h>
static BH1750 s_bh;
static bool   s_bh_ok;
#endif

void bh1750_init(void)
{
#if USE_BH1750
    s_bh_ok = s_bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_I2C_ADDR, &Wire);
    if (!s_bh_ok)
        Serial.println("[BH1750] ERROR: not found on I2C");
    else
        Serial.println("[BH1750] OK");
#else
    Serial.println("[BH1750] disabled (DNP)");
#endif
}

float bh1750_read_lux(void)
{
#if USE_BH1750
    return s_bh_ok ? s_bh.readLightLevel() : 0.0f;
#else
    return 10000.0f; /* simulated direct sunlight */
#endif
}
