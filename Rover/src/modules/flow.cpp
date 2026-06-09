#include "flow.h"
#include "camera.h"
#include "config.h"

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
	flowData.valid = false;

	return true;
}

/**
 * @brief Compute optical flow using block matching with SAD (Sum of Absolute
 * Differences) This is the proven algorithm from the original working code
 * @param current Current frame
 * @param previous Previous frame
 * @param width Frame width
 * @param height Frame height
 * @param dx Output horizontal flow
 * @param dy Output vertical flow
 */
static void computeOpticalFlow(uint8_t *current, uint8_t *previous, int width,
			       int height, float &dx, float &dy)
{
	int blkSize = 24;
	int searchRange = 8;

	int bestDx = 0, bestDy = 0;
	unsigned long minSAD = 0xFFFFFFFF;

	int startX = (width / 2) - (blkSize / 2);
	int startY = (height / 2) - (blkSize / 2);

	uint8_t minPix = 255, maxPix = 0;
	for (int y = 0; y < blkSize; y++) {
		for (int x = 0; x < blkSize; x++) {
			uint8_t p =
			    previous[(startY + y) * width + (startX + x)];
			if (p < minPix)
				minPix = p;
			if (p > maxPix)
				maxPix = p;
		}
	}

	// Serial.printf("Contrast=%d\n", maxPix - minPix);

	// If texture is too flat (low contrast), flow is invalid
	if ((maxPix - minPix) < 10) {
		dx = 0;
		dy = 0;
		flowData.valid = false;
		return;
	}

	// Block matching with SAD
	for (int sY = -searchRange; sY <= searchRange; sY++) {
		for (int sX = -searchRange; sX <= searchRange; sX++) {
			unsigned long sad = 0;
			for (int y = 0; y < blkSize; y++) {
				for (int x = 0; x < blkSize; x++) {
					int prevIdx =
					    (startY + y) * width + (startX + x);
					int currIdx =
					    (startY + y + sY) * width +
					    (startX + x + sX);
					sad += abs(current[currIdx] -
						   previous[prevIdx]);
				}
			}
			if (sad < minSAD) {
				minSAD = sad;
				bestDx = sX;
				bestDy = sY;
			}
		}
	}

	// if (abs(bestDx) <= 1)
	// 	bestDx = 0;

	// if (abs(bestDy) <= 1)
	// 	bestDy = 0;

	dx = (float)bestDx;
	dy = (float)bestDy;

	// Serial.printf("bestDx=%d bestDy=%d minSAD=%lu\n", bestDx, bestDy,
	//   minSAD);

	bool edgeMatch =
	    (abs(bestDx) >= searchRange) || (abs(bestDy) >= searchRange);

	// Rechazar matches de mala calidad
	flowData.valid = !edgeMatch;
}

bool flow_update()
{
	static uint32_t lastFrameTime = millis();

	uint32_t now = millis();

	flowData.frameDt = (now - lastFrameTime) * 0.001f;

	lastFrameTime = now;

	if (!camera_capture()) {
		Serial.println("[FLOW] camera_capture failed");
		flowData.valid = false;
		return false;
	}

	uint8_t *frame = camera_get_frame();

	// Serial.printf("Pixels: %u %u %u %u\n", frame[0], frame[100],
	//   frame[1000], frame[2000]);

	const uint32_t size = camera_get_width() * camera_get_height();

	if (firstFrame) {
		memcpy(prevFrame, frame, size);
		firstFrame = false;
		flowData.valid = false;
		return true;
	}

	computeOpticalFlow(frame, prevFrame, camera_get_width(),
			   camera_get_height(), flowData.dx, flowData.dy);

	memcpy(prevFrame, frame, size);

	return true;
}

FlowData flow_get() { return flowData; }
