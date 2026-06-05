#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* =========================================================================
 * FEATURE FLAGS — 1 = enabled, 0 = DNP (Do Not Populate)
 * Edit this file to match your physical build.
 * ========================================================================= */
#define USE_RFM69       1
#define USE_GPS         0
#define USE_BME280      1
#define USE_INA219      1
#define USE_BNO085      1
#define USE_ULTRASONIC  1
#define USE_BH1750      1
#define USE_SOIL        0
#define USE_SIM800      0
#define USE_VISION_SD   0

/* =========================================================================
 * CLOUD / CELLULAR CREDENTIALS (relevant only when USE_SIM800 = 1)
 * ========================================================================= */
#define UBIDOTS_TOKEN "BBUS-67yfRXfS0vL5IwGU7Fink8b9hQCUGA"
#define DEVICE_LABEL  "rover-telemetria"
#define APN_NAME      "internet.itelcel.com"
#define APN_USER      "webgprs"
#define APN_PASS      "webgprs2002"

/* =========================================================================
 * I2C DEVICE ADDRESSES
 * ========================================================================= */
#define OLED_I2C_ADDR        0x3C
#define BME280_I2C_ADDR      0x76  /* SDO low  → 0x76, SDO high → 0x77 */
#define INA219_I2C_ADDR      0x40  /* A0=GND, A1=GND → 0x40 (default)  */
#define BNO085_I2C_ADDR      0x4A  /* PS1=0, PS0=0   → 0x4A (default); PS0=1 → 0x4B */
#define ULTRASONIC_I2C_ADDR  0x57  /* RCWL-9200 */
#define BH1750_I2C_ADDR      0x23  /* ADDR low  → 0x23, ADDR high → 0x5C */

/* =========================================================================
 * RADIO SETTINGS
 * ========================================================================= */
#define RF69_FREQ_MHZ      915.0f
/* RFM69HW variant — PA_BOOST enabled in rfm69_module.cpp (setTxPower second
 * arg = true). Max for HW is +20 dBm; 17 is a safe default indoors. */
#define RF69_TX_POWER_DBM  20

/* =========================================================================
 * PERIPHERAL BAUD RATES
 * ========================================================================= */
#define GPS_BAUD_RATE    9600
#define SIM800_BAUD_RATE 9600

/* =========================================================================
 * ULTRASONIC — choose I2C or Trig/Echo per board
 * ========================================================================= */
#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_SEEED_XIAO_ESP32S3)
#define ULTRASONIC_USE_I2C 1
#else
#define ULTRASONIC_USE_I2C 0
#endif

/* =========================================================================
 * SOIL MOISTURE CALIBRATION (12-bit ADC)
 * ========================================================================= */
#define SOIL_ADC_AIR   3500 /* raw reading in dry air   */
#define SOIL_ADC_WATER 1500 /* raw reading submerged    */

/* =========================================================================
 * APPLICATION TIMING
 * ========================================================================= */
#define PRESENCE_THRESHOLD_CM  400.0f
#define DISPLAY_TIMEOUT_MS     5000U
#define PHOTO_COOLDOWN_MS      15000U
#define CELLULAR_INTERVAL_MS   10000U

#endif /* BOARD_CONFIG_H */
