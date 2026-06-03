#include <Adafruit_BNO08x.h>
#include <Wire.h>

Adafruit_BNO08x bno08x(-1);
sh2_SensorValue_t sensorValue;

// =====================================================
// STATE
// =====================================================

static float qR = 1.0f;
static float qI = 0.0f;
static float qJ = 0.0f;
static float qK = 0.0f;

static float gyroZ = 0.0f;

static float yawRad = 0.0f;
static float yawDeg = 0.0f;

bool imu_init()
{
	if (!bno08x.begin_I2C()) {
		Serial.println("Failed to find BNO08x chip");
#ifdef IMU_NON_BLOCKING
		while (1) {
			delay(10);
		}
#endif
		return false;
	}
	Serial.println("BNO08x Found!");

	imu_set_reports();

	return true;
}

static void imu_set_reports()
{
	bno08x.enableReport(SH2_ROTATION_VECTOR, 10000);
	bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED, 10000);
}

void imu_update()
{
	while (bno08x.getSensorEvent(&sensorValue)) {
		switch (sensorValue.sensorId) {
		case SH2_ROTATION_VECTOR:

			get_rotation_vector();
			break;

		case SH2_GYROSCOPE_CALIBRATED:

			gyroZ = sensorValue.un.gyroscope.z;
			break;
		}
	}
}

static void get_rotation_vector()
{
	qR = sensorValue.un.rotationVector.real;
	qI = sensorValue.un.rotationVector.i;
	qJ = sensorValue.un.rotationVector.j;
	qK = sensorValue.un.rotationVector.k;

	yawRad = atan2f(2.0f * (qR * qK + qI * qJ),
			1.0f - 2.0f * (qJ * qJ + qK * qK));

	yawDeg = yawRad * 180.0f / PI;

	if (yawDeg < 0.0f)
		yawDeg += 360.0f;
}

float imu_get_yaw_deg() { return yawDeg; }

float imu_get_yaw_rad() { return yawRad; }

float imu_get_gyro_z() { return gyroZ; }

float imu_get_heading_deg() { return fmodf(yawDeg + 360.0f, 360.0f); }