#include "bno085_module.h"
#include "board_config.h"

#if USE_BNO085
#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include <math.h>

static Adafruit_BNO08x s_imu(-1); /* -1 = no reset pin wired */
static bool            s_imu_ok;

/* Previous angles for tilt-jump detection */
static float s_prev_pitch;
static float s_prev_roll;
static bool  s_prev_angles_valid;

static void enable_reports(void)
{
    /* Rotation vector at ~100 Hz — absolute heading quaternion */
    if (!s_imu.enableReport(SH2_ROTATION_VECTOR, 10000))
        Serial.println("[BNO085] WARNING: could not enable rotation vector report");
    /* Linear acceleration (gravity subtracted) for shock/theft detection */
    if (!s_imu.enableReport(SH2_LINEAR_ACCELERATION, 10000))
        Serial.println("[BNO085] WARNING: could not enable linear accel report");
}
#endif /* USE_BNO085 */

void bno085_init(void)
{
#if USE_BNO085
    s_imu_ok = s_imu.begin_I2C(BNO085_I2C_ADDR, &Wire);
    if (!s_imu_ok) {
        Serial.println("[BNO085] ERROR: not found on I2C — check address/wiring");
        return;
    }
    enable_reports();
    Serial.println("[BNO085] OK — rotation vector + linear accel enabled");
#else
    Serial.println("[BNO085] disabled (DNP)");
#endif
}

ImuData bno085_read(void)
{
    ImuData d = {0.0f, 0.0f, 0.0f, false, false, false};
#if USE_BNO085
    if (!s_imu_ok)
        return d;

    if (s_imu.wasReset())
        enable_reports();

    sh2_SensorValue_t ev;
    if (!s_imu.getSensorEvent(&ev))
        return d;

    /* ---- Linear acceleration report (shock / theft detection) ----------- */
    if (ev.sensorId == SH2_LINEAR_ACCELERATION) {
        float ax = ev.un.linearAcceleration.x;
        float ay = ev.un.linearAcceleration.y;
        float az = ev.un.linearAcceleration.z;
        /* Magnitude in g (1 g = 9.81 m/s²) */
        float mag_g = sqrtf(ax*ax + ay*ay + az*az) / 9.81f;
        if (mag_g > SHOCK_THRESHOLD_G) {
            d.shock_detected = true;
            Serial.printf("[BNO085] SHOCK detected: %.2f g\n", mag_g);
        }
        return d; /* caller will get another event on next call for rotation */
    }

    /* ---- Rotation vector report (orientation) --------------------------- */
    if (ev.sensorId != SH2_ROTATION_VECTOR)
        return d;

    float w = ev.un.rotationVector.real;
    float x = ev.un.rotationVector.i;
    float y = ev.un.rotationVector.j;
    float z = ev.un.rotationVector.k;

    /* Quaternion → Euler ZYX (degrees) */
    d.yaw_deg   = atan2f(2.0f*(w*z + x*y), 1.0f - 2.0f*(y*y + z*z)) * (180.0f/M_PI);
    d.pitch_deg = asinf(2.0f*(w*y - z*x)) * (180.0f/M_PI);
    d.roll_deg  = atan2f(2.0f*(w*x + y*z), 1.0f - 2.0f*(x*x + y*y)) * (180.0f/M_PI);

    if (d.yaw_deg < 0.0f)
        d.yaw_deg += 360.0f;

    /* Tilt-jump detection — compare with previous stable orientation */
    if (s_prev_angles_valid) {
        float dp = fabsf(d.pitch_deg - s_prev_pitch);
        float dr = fabsf(d.roll_deg  - s_prev_roll);
        if (dp > TILT_THRESHOLD_DEG || dr > TILT_THRESHOLD_DEG) {
            d.tilt_alert = true;
            Serial.printf("[BNO085] TILT ALERT: pitch Δ%.1f° roll Δ%.1f° — node moved?\n",
                          dp, dr);
        }
    }
    s_prev_pitch         = d.pitch_deg;
    s_prev_roll          = d.roll_deg;
    s_prev_angles_valid  = true;
    d.valid              = true;
#endif
    return d;
}
