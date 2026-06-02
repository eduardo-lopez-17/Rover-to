#include "rfm69_module.h"

#if USE_RFM69
    #include <SPI.h>
    #include <RH_RF69.h>

    // Radio instance
    RH_RF69 rf69(RFM69_CS, RFM69_INT);
#endif

void initRFM69() {
#if USE_RFM69
    Serial.println("[RFM69] Initializing radio at 915 MHz...");

    // Hardware Reset (Highly recommended to avoid SPI lockups)
    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, LOW);
    delay(10);
    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);

    if (!rf69.init()) {
        Serial.println("[RFM69] ERROR: Init failed. Check SPI wiring.");
        return;
    }

    if (!rf69.setFrequency(RF69_FREQ)) {
        Serial.println("[RFM69] ERROR: setFrequency failed.");
        return;
    }

    // Set transmit power to 13dBm as per your spec sheet
    rf69.setTxPower(13, true); 
    
    Serial.println("[RFM69] Radio ready and configured.");
#else
    Serial.println("[RFM69] Module disabled by software (DNP).");
#endif
}

bool sendRFData(uint8_t* payload, uint8_t size) {
#if USE_RFM69
    // RadioHead requires waiting for the packet to be fully sent
    rf69.send(payload, size);
    rf69.waitPacketSent();
    return true;
#else
    // If DNP is active, just pretend it sent successfully
    return true; 
#endif
}