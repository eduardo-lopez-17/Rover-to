#include "bno085_module.h"
#include "board_config.h"

#if USE_BNO085
#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include <math.h>

static Adafruit_BNO08x s_imu(-1); /* -1 = no reset pin wired */
static bool            s_imu_ok;

static void enable_reports(void)
{
    /* Rotation vector at ~100 Hz (10 000 µs interval).
     * This report gives a quaternion fused with magnetometer for absolute heading. */
    if (!s_imu.enableReport(SH2_ROTATION_VECTOR, 10000))
        Serial.println("[BNO085] WARNING: could not enable rotation vector report");
}
#endif

void bno085_init(void)
{
#if USE_BNO085
    s_imu_ok = s_imu.begin_I2C(BNO085_I2C_ADDR, &Wire);
    if (!s_imu_ok) {
        Serial.println("[BNO085] ERROR: not found on I2C — check address/wiring");
        return;
    }
    enable_reports();
    Serial.println("[BNO085] OK");
#else
    Serial.println("[BNO085] disabled (DNP)");
#endif
}

ImuData bno085_read(void)
{
    ImuData d = {0.0f, 0.0f, 0.0f, false};
#if USE_BNO085
    if (!s_imu_ok)
        return d;

    /* Re-enable reports after an unexpected sensor reset */
    if (s_imu.wasReset())
        enable_reports();

    sh2_SensorValue_t ev;
    if (!s_imu.getSensorEvent(&ev))
        return d;
    if (ev.sensorId != SH2_ROTATION_VECTOR)
        return d;

    /* Quaternion components from the BNO08x report */
    float w = ev.un.rotationVector.real;
    float x = ev.un.rotationVector.i;
    float y = ev.un.rotationVector.j;
    float z = ev.un.rotationVector.k;

    /* Quaternion → Euler (ZYX convention, degrees) */
    d.yaw_deg   = atan2f(2.0f * (w * z + x * y),
                         1.0f - 2.0f * (y * y + z * z)) * (180.0f / M_PI);
    d.pitch_deg = asinf(2.0f * (w * y - z * x)) * (180.0f / M_PI);
    d.roll_deg  = atan2f(2.0f * (w * x + y * z),
                         1.0f - 2.0f * (x * x + y * y)) * (180.0f / M_PI);

    /* Normalize yaw to 0-360 */
    if (d.yaw_deg < 0.0f)
        d.yaw_deg += 360.0f;

    d.valid = true;
#endif
    return d;
}
