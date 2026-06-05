#ifndef AUDIO_MODULE_H
#define AUDIO_MODULE_H

#include <Arduino.h>

/* Initialize the PDM microphones (XIAO ESP32-S3 Sense built-in).
 * Call once in setup(). Non-fatal if mic hardware is absent. */
void audio_init(void);

/* Record AUDIO_RECORD_SEC seconds of audio and save as a WAV file to SD.
 * Returns the file path written, or empty string on failure.
 * Continues silently if SD is not mounted — audio is discarded. */
String audio_record_to_sd(void);

#endif /* AUDIO_MODULE_H */
