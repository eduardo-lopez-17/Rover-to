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
 * @brief Compute optical flow using block matching with SAD (Sum of Absolute Differences)
 * This is the proven algorithm from the original working code
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
	int blkSize = 16;       
	int searchRange = 16;   
	
	int bestDx = 0, bestDy = 0;
	unsigned long minSAD = 0xFFFFFFFF;
	
	int startX = (width / 2) - (blkSize / 2);
	int startY = (height / 2) - (blkSize / 2);
	
	uint8_t minPix = 255, maxPix = 0;
	for (int y = 0; y < blkSize; y++) {
		for (int x = 0; x < blkSize; x++) {
			uint8_t p = previous[(startY + y) * width + (startX + x)];
			if (p < minPix) minPix = p;
			if (p > maxPix) maxPix = p;
		}
	}
	
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
					int prevIdx = (startY + y) * width + (startX + x);
					int currIdx = (startY + y + sY) * width + (startX + x + sX);
					sad += abs(current[currIdx] - previous[prevIdx]);
				}
			}
			if (sad < minSAD) {
				minSAD = sad;
				bestDx = sX;
				bestDy = sY;
			}
		}
	}
	
	dx = (float)bestDx;
	dy = (float)bestDy;
	flowData.valid = true;
}

bool flow_update()
{
	if (!camera_capture()) {
		Serial.println("[FLOW] camera_capture failed");
		flowData.valid = false;
		return false;
	}

	uint8_t *frame = camera_get_frame();

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
