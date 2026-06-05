#include "vision_sd_module.h"
#include "board_config.h"

#if USE_VISION_SD
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_camera.h"
#include <samu0805-project-1_inferencing.h>

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

/* Confidence threshold to call it an intruder (0.0–1.0) */
#define INTRUDER_THRESHOLD   0.70f
/* Average pixel luminance diff (0–255) to consider motion present */
#define MOTION_THRESHOLD_PIX 8

static bool     s_cam_ok;
static bool     s_sd_ok;
static int      s_photo_count;

/* Previous RGB565 frame for frame differencing */
static uint16_t s_prev_frame[EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT];
static bool     s_prev_frame_valid;

/* Pointer shared with EI signal callback */
static const uint16_t *s_ei_pixels;

/* -------------------------------------------------------------------------
 * EI signal callback — converts each RGB565 pixel to packed RGB888 float.
 * --------------------------------------------------------------------- */
static int ei_get_data(size_t offset, size_t len, float *out)
{
    for (size_t i = 0; i < len; i++) {
        uint16_t px = s_ei_pixels[offset + i];
        uint8_t  r  = (px >> 11) << 3;
        uint8_t  g  = ((px >> 5) & 0x3F) << 2;
        uint8_t  b  = (px & 0x1F) << 3;
        out[i]      = (float)((r << 16) | (g << 8) | b);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Frame differencing on the red channel (cheap, good enough for motion).
 * Stores current frame as previous before returning.
 * --------------------------------------------------------------------- */
static bool motion_detected(const uint16_t *curr)
{
    static const size_t N = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    if (!s_prev_frame_valid) {
        memcpy(s_prev_frame, curr, N * sizeof(uint16_t));
        s_prev_frame_valid = true;
        return false;
    }
    uint32_t diff = 0;
    for (size_t i = 0; i < N; i++) {
        uint8_t r_c = (curr[i]        >> 11) << 3;
        uint8_t r_p = (s_prev_frame[i] >> 11) << 3;
        diff += (uint32_t)abs((int)r_c - (int)r_p);
    }
    memcpy(s_prev_frame, curr, N * sizeof(uint16_t));
    return (diff / N) > MOTION_THRESHOLD_PIX;
}

/* Switch camera to JPEG + high-res for saving a good-quality photo */
static void cam_set_photo_mode(void)
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_pixformat(s, PIXFORMAT_JPEG);
    s->set_framesize(s, FRAMESIZE_SXGA);   /* 1280x1024 */
}

/* Switch back to RGB565 96x96 for continuous inference */
static void cam_set_inference_mode(void)
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_pixformat(s, PIXFORMAT_RGB565);
    s->set_framesize(s, FRAMESIZE_96X96);
}
#endif /* USE_VISION_SD */

/* =========================================================================
 * vision_sd_init
 * ========================================================================= */
void vision_sd_init(void)
{
#if USE_VISION_SD
    s_sd_ok = SD.begin(SD_CS_PIN);
    if (!s_sd_ok)
        Serial.println("[VISION] ERROR: SD mount failed — photos disabled");
    else
        Serial.println("[VISION] SD OK");

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
    /* Start in inference mode: RGB565 96x96 */
    cfg.frame_size      = FRAMESIZE_96X96;
    cfg.pixel_format    = PIXFORMAT_RGB565;
    cfg.grab_mode       = CAMERA_GRAB_WHEN_EMPTY;
    cfg.fb_location     = CAMERA_FB_IN_PSRAM;
    cfg.fb_count        = 1;

    s_cam_ok = (esp_camera_init(&cfg) == ESP_OK);
    if (!s_cam_ok) {
        Serial.println("[VISION] ERROR: camera init failed");
        return;
    }
    Serial.println("[VISION] OK — camera 96x96 RGB565, EI model loaded");
#else
    Serial.println("[VISION] disabled (DNP)");
#endif
}

/* =========================================================================
 * vision_capture_photo
 * Temporarily switches camera to JPEG / high-res, saves to SD, restores.
 * ========================================================================= */
bool vision_capture_photo(void)
{
#if USE_VISION_SD
    if (!s_cam_ok) return false;

    cam_set_photo_mode();
    delay(150); /* let sensor settle after mode switch */

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[VISION] ERROR: photo capture failed");
        cam_set_inference_mode();
        return false;
    }

    bool saved = false;
    if (s_sd_ok) {
        String path = "/intruder_" + String(++s_photo_count) + ".jpg";
        File file = SD.open(path.c_str(), FILE_WRITE);
        if (file) {
            file.write(fb->buf, fb->len);
            file.close();
            Serial.printf("[VISION] photo saved: %s (%u B)\n",
                          path.c_str(), (unsigned)fb->len);
            saved = true;
        }
    }

    esp_camera_fb_return(fb);
    cam_set_inference_mode();
    delay(100);
    return saved;
#else
    return true;
#endif
}

/* =========================================================================
 * vision_run_monitor
 * Grabs one RGB565 frame, runs frame differencing, and if motion is
 * detected runs EI inference. Returns true when "Instruso" score exceeds
 * INTRUDER_THRESHOLD.
 * ========================================================================= */
bool vision_run_monitor(void)
{
#if USE_VISION_SD
    if (!s_cam_ok) return false;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return false;

    bool intruder = false;

    if (motion_detected((const uint16_t *)fb->buf)) {
        s_ei_pixels = (const uint16_t *)fb->buf;

        signal_t sig;
        sig.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
        sig.get_data     = ei_get_data;

        ei_impulse_result_t result = {};
        EI_IMPULSE_ERROR    err    = run_classifier(&sig, &result, false);

        if (err == EI_IMPULSE_OK) {
            for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                Serial.printf("[EI] %s: %.0f%%\n",
                              result.classification[i].label,
                              result.classification[i].value * 100.0f);
                if (strcmp(result.classification[i].label, "Instruso") == 0 &&
                    result.classification[i].value >= INTRUDER_THRESHOLD) {
                    intruder = true;
                }
            }
        } else {
            Serial.printf("[EI] inference error: %d\n", err);
        }

        s_ei_pixels = nullptr;
    }

    esp_camera_fb_return(fb);
    return intruder;
#else
    return false;
#endif
}
