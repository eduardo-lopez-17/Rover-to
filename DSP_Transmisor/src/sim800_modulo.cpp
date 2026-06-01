#include "sim800_modulo.h"

#if USAR_CELULAR
    #define TINY_GSM_MODEM_SIM800
    #include <TinyGsmClient.h>

    HardwareSerial SerialAT(1); 
    TinyGsm modem(SerialAT);
    TinyGsmClient client(modem);
    
    // Servidor oficial de Ubidots
    const char server[] = "industrial.api.ubidots.com";
    const int  port = 80;
#endif

void inicializarCelular() {
#if USAR_CELULAR
    Serial.println("[CELULAR] Iniciando módem SIM800...");
    SerialAT.begin(SIM800_BAUD, SERIAL_8N1, SIM800_RX, SIM800_TX);
    delay(3000); 

    if (!modem.restart()) {
        Serial.println("[CELULAR] ERROR: El módem no responde.");
        return;
    }

    Serial.print("[CELULAR] Conectando a APN: ");
    Serial.println(APN_CELULAR);
    
    if (!modem.gprsConnect(APN_CELULAR, USER_CELULAR, PASS_CELULAR)) {
        Serial.println("[CELULAR] ERROR: No se pudo conectar a la red.");
    } else {
        Serial.println("[CELULAR] Conectado a la red celular exitosamente.");
    }
#else
    Serial.println("[CELULAR] Módulo deshabilitado por software (DNP).");
#endif
}

bool enviarDatosNube(String payload_json) {
#if USAR_CELULAR
    if (!modem.isGprsConnected()) {
        Serial.println("[CELULAR] Reconectando GPRS...");
        modem.gprsConnect(APN_CELULAR, USER_CELULAR, PASS_CELULAR);
    }

    if (client.connect(server, port)) {
        // Ruta dinámica con el nombre de tu dispositivo
        String path = "/api/v1.6/devices/" + String(DEVICE_LABEL);
        
        // Petición HTTP estructurada exactamente para Ubidots
        client.print(String("POST ") + path + " HTTP/1.1\r\n" +
                     "Host: " + server + "\r\n" +
                     "X-Auth-Token: " + UBIDOTS_TOKEN + "\r\n" +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + payload_json.length() + "\r\n\r\n" +
                     payload_json);
        
        client.stop();
        Serial.println("[UBIDOTS] Datos subidos exitosamente.");
        return true;
    } else {
        Serial.println("[UBIDOTS] Fallo la conexión al servidor.");
        return false;
    }
#else
    return true; 
#endif
}