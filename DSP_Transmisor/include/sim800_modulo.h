#ifndef SIM800_MODULO_H
#define SIM800_MODULO_H

#include <Arduino.h>

#define USAR_CELULAR false

// --- CREDENCIALES UBIDOTS ---
#define UBIDOTS_TOKEN "BBUS-67yfRXfS0vL5IwGU7Fink8b9hQCUGA" 
#define DEVICE_LABEL  "rover-telemetria" // Así se llamará en la página

// --- CONFIGURACIÓN DE RED CELULAR ---
#define APN_CELULAR "internet.itelcel.com"
#define USER_CELULAR "webgprs"
#define PASS_CELULAR "webgprs2002"

// --- CONFIGURACIÓN DE PINES (Hardware Serial 1) ---
#define SIM800_RX 4  
#define SIM800_TX 2 
#define SIM800_BAUD 9600

void inicializarCelular();
bool enviarDatosNube(String payload_json);

#endif