#include "soil_moisture_module.h"
#include "board_config.h"
#include "pin_map.h"

void soil_init(void)
{
#if USE_SOIL
    analogReadResolution(12);
    Serial.println("[SOIL] OK");
#else
    Serial.println("[SOIL] disabled (DNP)");
#endif
}

float soil_read_pct(void)
{
#if USE_SOIL
    int  raw = analogRead(PIN_SOIL);
    long pct = map(raw, SOIL_ADC_AIR, SOIL_ADC_WATER, 0, 100);
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    return (float)pct;
#else
    return 45.5f;
#endif
}
