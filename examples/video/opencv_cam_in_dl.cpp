#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <opencv2/opencv.hpp>

#include <cstdio>
#include <cstdint>

namespace
{
cv::VideoCapture g_capture;
cv::Mat g_frame;
cv::Mat g_resized;
int g_width = 0;
int g_height = 0;

char g_errorText[512];

int fail(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(g_errorText, sizeof(g_errorText), format, args);
	va_end(args);
	puts(g_errorText);
	return -1;
}
}

int CAM_In_Init(int width, int height, int deviceIndex)
{
	if(width <= 0 || height <= 0)
		return fail("camera init failed: width and height must be positive");

	if(g_capture.isOpened())
		g_capture.release();

	if(!g_capture.open(deviceIndex, cv::CAP_ANY))
		return fail("camera init failed: cannot open camera device %i", deviceIndex);

	g_capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
	g_capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);
	g_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

	g_width = width;
	g_height = height;
	return 1;
}

int CAM_In_Capture(int* buf)
{
	if(!buf)
		return fail("camera capture failed: output buffer is null");

	if(!g_capture.isOpened())
		return fail("camera capture failed: camera is not initialized");

	if(!g_capture.read(g_frame) || g_frame.empty())
		return fail("camera capture failed: unable to read frame");

	cv::Mat* source = &g_frame;
	if(g_frame.cols != g_width || g_frame.rows != g_height)
	{
		cv::resize(g_frame, g_resized, cv::Size(g_width, g_height), 0.0, 0.0, cv::INTER_LINEAR);
		source = &g_resized;
	}

	if(!source->isContinuous())
		*source = source->clone();

	const unsigned char* src = source->ptr<unsigned char>(0);
	const int pixelCount = g_width * g_height;
	const int channels = source->channels();
	if(channels != 1 && channels != 3 && channels != 4)
		return fail("camera capture failed: unsupported channel count %i", channels);

	for(int i = 0; i < pixelCount; ++i)
	{
		const unsigned char* px = src + i * channels;
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a = 0xff;

		if(channels == 1)
		{
			r = px[0];
			g = px[0];
			b = px[0];
		}
		else if(channels == 3)
		{
			b = px[0];
			g = px[1];
			r = px[2];
		}
		else
		{
			b = px[0];
			g = px[1];
			r = px[2];
			a = px[3];
		}

		// Packed RGBA channels in a 32-bit integer as 0xAARRGGBB.
		buf[i] = (int(uint32_t(a)) << 24) | (int(uint32_t(r)) << 16) | (int(uint32_t(g)) << 8) | int(uint32_t(b));
	}

	return pixelCount;
}

int CAM_In_Done()
{
	g_capture.release();
	g_frame.release();
	g_resized.release();
	g_width = 0;
	g_height = 0;
	return 0;
}

extern "C" EXPORT int CAM_In_Init_Wrapper(ScincVM* s, void*) { ScincBoundCall(s, CAM_In_Init); return 0; }
extern "C" EXPORT int CAM_In_Capture_Wrapper(ScincVM* s, void*) { ScincBoundCall(s, CAM_In_Capture); return 0; }
extern "C" EXPORT int CAM_In_Done_Wrapper(ScincVM* s, void*) { ScincBoundCall(s, CAM_In_Done); return 0; }
