/**
 * @file camera.h
 * @brief Interfaz para la cámara del ESP32-S3 Xiao Sense
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <stdint-gcc.h>

bool camera_init();

bool camera_capture();

uint8_t *camera_get_frame();

uint16_t camera_get_width();

uint16_t camera_get_height();

#endif // CAMERA_H