#include "vision_sd_module.h"
#include "board_config.h"

#if USE_VISION_SD
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_camera.h"

/* Camera pin map — fixed for XIAO ESP32-S3 Sense */
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1
#define CAM_PIN_XCLK   10
#define CAM_PIN_SIOD   40
#define CAM_PIN_SIOC   39
#define CAM_PIN_D7     48
#define CAM_PIN_D6     11
#define CAM_PIN_D5     12
#define CAM_PIN_D4     14
#define CAM_PIN_D3     16
#define CAM_PIN_D2     18
#define CAM_PIN_D1     17
#define CAM_PIN_D0     15
#define CAM_PIN_VSYNC  38
#define CAM_PIN_HREF   47
#define CAM_PIN_PCLK   13
#define SD_CS_PIN      21

static int s_photo_count = 1;
#endif

void vision_sd_init(void)
{
#if USE_VISION_SD
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[VISION] ERROR: SD card mount failed");
        return;
    }

    camera_config_t cfg = {};
    cfg.ledc_channel    = LEDC_CHANNEL_0;
    cfg.ledc_timer      = LEDC_TIMER_0;
    cfg.pin_d0          = CAM_PIN_D0;
    cfg.pin_d1          = CAM_PIN_D1;
    cfg.pin_d2          = CAM_PIN_D2;
    cfg.pin_d3          = CAM_PIN_D3;
    cfg.pin_d4          = CAM_PIN_D4;
    cfg.pin_d5          = CAM_PIN_D5;
    cfg.pin_d6          = CAM_PIN_D6;
    cfg.pin_d7          = CAM_PIN_D7;
    cfg.pin_xclk        = CAM_PIN_XCLK;
    cfg.pin_pclk        = CAM_PIN_PCLK;
    cfg.pin_vsync       = CAM_PIN_VSYNC;
    cfg.pin_href        = CAM_PIN_HREF;
    cfg.pin_sccb_sda    = CAM_PIN_SIOD;
    cfg.pin_sccb_scl    = CAM_PIN_SIOC;
    cfg.pin_pwdn        = CAM_PIN_PWDN;
    cfg.pin_reset       = CAM_PIN_RESET;
    cfg.xclk_freq_hz    = 20000000;
    cfg.frame_size      = FRAMESIZE_UXGA;
    cfg.pixel_format    = PIXFORMAT_JPEG;
    cfg.grab_mode       = CAMERA_GRAB_WHEN_EMPTY;
    cfg.fb_location     = CAMERA_FB_IN_PSRAM;
    cfg.jpeg_quality    = 12; /* 0-63, lower = better quality */
    cfg.fb_count        = 1;

    if (esp_camera_init(&cfg) != ESP_OK) {
        Serial.println("[VISION] ERROR: camera init failed");
        return;
    }
    Serial.println("[VISION] OK — camera and SD ready");
#else
    Serial.println("[VISION] disabled (DNP)");
#endif
}

bool vision_capture_photo(void)
{
#if USE_VISION_SD
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[VISION] ERROR: camera capture failed");
        return false;
    }
    String path = "/photo_" + String(s_photo_count++) + ".jpg";
    File   file = SD.open(path.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("[VISION] ERROR: failed to open file for writing");
        esp_camera_fb_return(fb);
        return false;
    }
    file.write(fb->buf, fb->len);
    file.close();
    esp_camera_fb_return(fb);
    Serial.printf("[VISION] saved: %s\n", path.c_str());
    return true;
#else
    return true;
#endif
}
