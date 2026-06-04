#include "ultrasonic_module.h"
#include "board_config.h"
#include "pin_map.h"

#if USE_ULTRASONIC && ULTRASONIC_USE_I2C
#include <Wire.h>
#endif

void ultrasonic_init(void)
{
#if USE_ULTRASONIC
#if ULTRASONIC_USE_I2C
    Serial.printf("[ULTRASONIC] I2C mode — addr 0x%02X\n", ULTRASONIC_I2C_ADDR);
#else
    pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
    pinMode(PIN_ULTRASONIC_ECHO, INPUT);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    Serial.println("[ULTRASONIC] Trig/Echo mode");
#endif
#else
    Serial.println("[ULTRASONIC] disabled (DNP)");
#endif
}

float ultrasonic_read_cm(void)
{
#if USE_ULTRASONIC
#if ULTRASONIC_USE_I2C
    /* RCWL-9200 I2C protocol: send 0x01, wait 120 ms, read 3 bytes */
    Wire.beginTransmission(ULTRASONIC_I2C_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission() != 0)
        return 0.0f;
    delay(120);
    Wire.requestFrom((uint8_t)ULTRASONIC_I2C_ADDR, (uint8_t)3);
    if (Wire.available() == 3) {
        uint32_t raw = (uint32_t)Wire.read() << 16;
        raw |= (uint32_t)Wire.read() << 8;
        raw |= Wire.read();
        float dist = (float)raw / 10000.0f;
        return (dist > 400.0f) ? 400.0f : dist;
    }
    return 0.0f;
#else
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    long dur = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, 30000);
    if (dur == 0)
        return 0.0f;
    float dist = dur * 0.034f / 2.0f;
    return (dist > 400.0f) ? 400.0f : dist;
#endif
#else
    return 45.0f; /* simulated reading */
#endif
}
