#ifndef RFM69_MODULE_H
#define RFM69_MODULE_H

#include <Arduino.h>

/* Initialize the RFM69W radio over SPI. */
void rfm69_init(void);

/* Sample ambient RSSI to confirm the radio is alive.
 * Prints result to Serial. Call once after rfm69_init(). */
bool rfm69_self_test(void);

/* Send a raw byte buffer. Returns false if radio not initialized. */
bool rfm69_send(const uint8_t *payload, uint8_t len);

/* Non-blocking receive. Returns true and fills buf/len if a packet arrived.
 * len must be initialized to the buffer size before calling. */
bool rfm69_recv(uint8_t *buf, uint8_t *len);

#endif /* RFM69_MODULE_H */
