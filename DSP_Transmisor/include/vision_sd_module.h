#ifndef VISION_SD_MODULE_H
#define VISION_SD_MODULE_H

#include <Arduino.h>

void vision_sd_init(void);

/* Capture a JPEG photo and save to SD. Returns false on error. */
bool vision_capture_photo(void);

/* Non-blocking monitor: grab one frame, run frame differencing, then EI
 * inference if motion was detected. Returns true if "Instruso" found. */
bool vision_run_monitor(void);

#endif /* VISION_SD_MODULE_H */
