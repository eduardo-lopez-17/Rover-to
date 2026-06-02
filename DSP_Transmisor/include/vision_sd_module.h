#ifndef VISION_SD_MODULE_H
#define VISION_SD_MODULE_H

#include <Arduino.h>

// --- ACTIVATION SWITCH ---
#define USE_VISION_SD true

void initVisionAndSD();
bool takeAndSavePhoto();

#endif