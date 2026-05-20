#include "ultrasonico.h"

void inicializarUltrasonico() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
}

float obtenerDistancia() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duracion = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout

    if (duracion == 0) {
        return -1.0; // Fuera de rango o error
    }
    return (duracion * 0.0343) / 2.0;
}