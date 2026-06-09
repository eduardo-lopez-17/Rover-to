#ifndef SENSOR_PAYLOAD_H
#define SENSOR_PAYLOAD_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
	uint32_t timestamp;
	// Navegación DSP
	float pos_x;
	float pos_y;
	float vel_x;
	float vel_y;
	float yaw_angle;
	uint8_t flow_valid;
	// Sensores telecom
	float distance_cm;
	float gps_lat;
	float gps_lng;
	float env_temp_c;
	float env_humidity_pct;
	float env_pressure_hpa;
	float env_soil_moisture_pct;
	float pwr_voltage_v;
	float pwr_current_ma;
	uint8_t anomaly;
	uint8_t vision_obj_id;
	uint8_t vision_confidence;
} SensorPayload;

#endif // SENSOR_PAYLOAD_H