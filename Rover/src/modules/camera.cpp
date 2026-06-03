#include "camera.h"
#include "config.h"
#include "pins.h"

#define sensor_t esp_sensor_t
#include <esp_camera.h>
#undef sensor_t

static camera_fb_t *fb = nullptr;

bool camera_init()
{
	camera_config_t config;

	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;

	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;

	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;

	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;

	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;

	config.xclk_freq_hz = 20000000;

	config.pixel_format = PIXFORMAT_GRAYSCALE;
	config.frame_size = CAMERA_FRAME_SIZE;

	config.jpeg_quality = 12;

	config.fb_count = 1;

	return esp_camera_init(&config) == ESP_OK;
}

bool camera_capture()
{
	if (fb) {
		esp_camera_fb_return(fb);
		fb = nullptr;
	}

	fb = esp_camera_fb_get();

	return fb != nullptr;
}

uint8_t *camera_get_frame()
{
	if (!fb)
		return nullptr;

	return fb->buf;
}

uint16_t camera_get_width() { return CAMERA_FRAME_WIDTH; }

uint16_t camera_get_height() { return CAMERA_FRAME_HEIGHT; }
