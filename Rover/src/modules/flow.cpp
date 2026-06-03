#include "flow.h"
#include "camera.h"

#include <Arduino.h>

static uint8_t *prevFrame = nullptr;

static bool firstFrame = true;

static FlowData flowData;

bool flow_init()
{
	uint32_t size = camera_get_width() * camera_get_height();

	prevFrame = (uint8_t *)ps_malloc(size);

	if (!prevFrame)
		return false;

	memset(prevFrame, 0, size);

	return true;
}

static void computeFlow(uint8_t *current, uint8_t *previous, int width,
			int height, float &dx, float &dy, uint32_t &features)
{
	long sumX = 0;
	long sumY = 0;

	features = 0;

	for (int y = 10; y < height - 10; y += 4) {
		for (int x = 10; x < width - 10; x += 4) {
			int idx = y * width + x;

			int diff = current[idx] - previous[idx];

			if (abs(diff) > 15) {
				sumX += diff * (x - width / 2);

				sumY += diff * (y - height / 2);

				features++;
			}
		}
	}

	if (features > 20) {
		dx = -(float)sumX / features;
		dy = -(float)sumY / features;
	} else {
		dx = 0;
		dy = 0;
	}
}

bool flow_update()
{
	if (!camera_capture())
		return false;

	uint8_t *frame = camera_get_frame();

	const uint32_t size = camera_get_width() * camera_get_height();

	if (firstFrame) {
		memcpy(prevFrame, frame, size);

		firstFrame = false;

		return true;
	}

	computeFlow(frame, prevFrame, camera_get_width(), camera_get_height(),
		    flowData.dx, flowData.dy, flowData.features);

	memcpy(prevFrame, frame, size);

	return true;
}

FlowData flow_get() { return flowData; }
