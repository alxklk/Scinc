#include <stdio.h>

#define CAM_W 320
#define CAM_H 240

#define G_SCREEN_WIDTH CAM_W
#define G_SCREEN_HEIGHT CAM_H
#define G_SCREEN_SCALE 3
#define G_SCREEN_MODE 1

#include "graphics.h"
#include "../ws.h"
#include "opencv_cam_in.h"

int frameBuf[CAM_W * CAM_H];

int SwapRedBlue(int rgba)
{
	return
		(rgba & 0xff00ff00) |
		((rgba & 0x00ff0000) >> 16) |
		((rgba & 0x000000ff) << 16);
}

int main()
{
	if(CAM_In_Init(CAM_W, CAM_H, 0) <= 0)
	{
		printf("Camera init failed\n");
		return 1;
	}

	int frame = 0;
	double t0 = Time();

	for(;;)
	{
		if(CAM_In_Capture(frameBuf) <= 0)
			break;

		for(int y = 0; y < CAM_H; ++y)
		{
			int row = y * CAM_W;
			for(int x = 0; x < CAM_W; ++x)
			{
				PutPixel(x, y, frameBuf[row + x]);
			}
		}

		char s[64];
		snprintf(s, sizeof(s), "camera fps %.2f", (frame + 1) / (Time() - t0 + 1e-9));
		stext(s, 10, 10, 0xffffffff);
		stext("Press Q to quit", 10, 24, 0xffffffff);

		Present();

		if(KeyPressed('q') || KeyPressed('Q'))
			break;

		frame++;
	}

	CAM_In_Done();
	return 0;
}
