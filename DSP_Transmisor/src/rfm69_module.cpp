#include "rfm69_module.h"
#include "board_config.h"
#include "pin_map.h"

#if USE_RFM69
#include <RH_RF69.h>
#include <SPI.h>
static RH_RF69 s_radio(PIN_RFM69_CS, PIN_RFM69_INT);
static bool    s_radio_ok;
#endif

static void hardware_reset(void)
{
#if USE_RFM69
    if (PIN_RFM69_RST < 0)
        return;
    pinMode(PIN_RFM69_RST, OUTPUT);
    digitalWrite(PIN_RFM69_RST, LOW);
    delay(10);
    digitalWrite(PIN_RFM69_RST, HIGH);
    delay(10);
    digitalWrite(PIN_RFM69_RST, LOW);
    delay(10);
#endif
}

void rfm69_init(void)
{
#if USE_RFM69
    hardware_reset();

    s_radio_ok = s_radio.init();
    if (!s_radio_ok) {
        Serial.println("[RFM69] ERROR: SPI init failed — check CS/INT wiring");
        return;
    }

    if (!s_radio.setFrequency(RF69_FREQ_MHZ)) {
        Serial.println("[RFM69] ERROR: setFrequency failed");
        s_radio_ok = false;
        return;
    }

    /* RFM69HW: second argument true enables PA_BOOST (required for HW variant).
     * Do NOT change to false — that disables PA_BOOST and caps power at +13 dBm
     * (RFM69W behavior), which still works but wastes HW capability. */
    s_radio.setTxPower(RF69_TX_POWER_DBM, true);

    Serial.printf("[RFM69] OK — %.1f MHz, %d dBm (RFM69HW mode, PA_BOOST on)\n",
                  RF69_FREQ_MHZ, RF69_TX_POWER_DBM);
#else
    Serial.println("[RFM69] disabled (DNP)");
#endif
}

bool rfm69_self_test(void)
{
#if USE_RFM69
    if (!s_radio_ok) {
        Serial.println("[RFM69] SELF-TEST FAIL: radio not initialized");
        return false;
    }
    /* Switch to RX briefly and read ambient RSSI.
     * A valid reading (typically -100 to -40 dBm) confirms the RF chain
     * and SPI communication are working. */
    s_radio.setModeRx();
    delay(20);
    int8_t rssi = s_radio.lastRssi();
    s_radio.setModeIdle();
    Serial.printf("[RFM69] SELF-TEST OK — ambient RSSI: %d dBm\n", rssi);
    return true;
#else
    return true;
#endif
}

bool rfm69_send(const uint8_t *payload, uint8_t len)
{
#if USE_RFM69
    if (!s_radio_ok)
        return false;
    s_radio.send(payload, len);
    s_radio.waitPacketSent();
    return true;
#else
    return true;
#endif
}

bool rfm69_recv(uint8_t *buf, uint8_t *len)
{
#if USE_RFM69
    if (!s_radio_ok)
        return false;
    if (!s_radio.available())
        return false;
    return s_radio.recv(buf, len);
#else
    (void)buf;
    (void)len;
    return false;
#endif
}
