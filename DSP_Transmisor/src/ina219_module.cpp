#include "ina219_module.h"

#if USE_INA219
    #include <Wire.h>
    #include <Adafruit_INA219.h>

    Adafruit_INA219 ina219;
#endif

void initINA219() {
#if USE_INA219
    Serial.println("[INA219] Initializing power monitor...");
    
    // The INA219 uses the standard I2C bus initialized in setup()
    if (!ina219.begin()) {
        Serial.println("[INA219] ERROR: Failed to find INA219 chip");
        return;
    }
    
    // Default calibration for 32V and 2A limits
    ina219.setCalibration_32V_2A();
    Serial.println("[INA219] Ready.");
#else
    Serial.println("[INA219] Module disabled by software (DNP).");
#endif
}

PowerData getPowerData() {
    PowerData data = {0.0, 0.0, 0.0};
#if USE_INA219
    data.bus_voltage = ina219.getBusVoltage_V();
    data.current_mA = ina219.getCurrent_mA();
    data.power_mW = ina219.getPower_mW();
#else
    // Simulated nominal data for testing the Digital Twin
    data.bus_voltage = 12.0; 
    data.current_mA = 250.5;
    data.power_mW = 3006.0;
#endif
    return data;
}