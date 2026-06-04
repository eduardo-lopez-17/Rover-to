#include "sim800_module.h"
#include "board_config.h"
#include "pin_map.h"

#if USE_SIM800
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
static HardwareSerial s_serial_at(1);
static TinyGsm        s_modem(s_serial_at);
static TinyGsmClient  s_client(s_modem);
static const char     k_server[] = "industrial.api.ubidots.com";
static const int      k_port     = 80;
#endif

void sim800_init(void)
{
#if USE_SIM800
    s_serial_at.begin(SIM800_BAUD_RATE, SERIAL_8N1, PIN_SIM800_RX, PIN_SIM800_TX);
    delay(3000);
    if (!s_modem.restart()) {
        Serial.println("[SIM800] ERROR: modem not responding");
        return;
    }
    if (!s_modem.gprsConnect(APN_NAME, APN_USER, APN_PASS))
        Serial.println("[SIM800] ERROR: GPRS connect failed");
    else
        Serial.println("[SIM800] OK — GPRS connected");
#else
    Serial.println("[SIM800] disabled (DNP)");
#endif
}

bool sim800_send_cloud(const String &json_payload)
{
#if USE_SIM800
    if (!s_modem.isGprsConnected())
        s_modem.gprsConnect(APN_NAME, APN_USER, APN_PASS);
    if (!s_client.connect(k_server, k_port)) {
        Serial.println("[SIM800] ERROR: server connect failed");
        return false;
    }
    String path = "/api/v1.6/devices/" + String(DEVICE_LABEL);
    s_client.print(String("POST ") + path + " HTTP/1.1\r\n" +
                   "Host: " + k_server + "\r\n" +
                   "X-Auth-Token: " + UBIDOTS_TOKEN + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + json_payload.length() + "\r\n\r\n" +
                   json_payload);
    s_client.stop();
    Serial.println("[SIM800] cloud upload OK");
    return true;
#else
    return true;
#endif
}
