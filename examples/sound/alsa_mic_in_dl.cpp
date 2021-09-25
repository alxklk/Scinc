#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <alsa/asoundlib.h>

int bufferFrames;
int16_t* buffer=0;
unsigned int rate=48000;
snd_pcm_t *captureHandle;
snd_pcm_hw_params_t *hwParams;
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
//const char* deviceName = "default";
const char* deviceName = "hw:1";

char errorText[512];

int MIC_In_Init(int buflen)
{
	puts("Initialization of ALSA mic in\n");
	bufferFrames=buflen;
	struct PrErr
	{
		~PrErr(){puts(errorText);}
	};
	PrErr pe;
	int error=0;
	error=snd_pcm_open(&captureHandle, deviceName, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
	if(error<0)
	{
		snprintf(errorText, 512, "cannot open audio device %s (%s)\n", deviceName, snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_hw_params_malloc(&hwParams);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot allocate hardware parameter structure (%s)\n",snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_hw_params_any(captureHandle, hwParams);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot initialize hardware parameter structure (%s)\n",snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_hw_params_set_access(captureHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot set access type (%s)\n", snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_hw_params_set_format(captureHandle, hwParams, format);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot set sample format (%s)\n", snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_hw_params_set_rate_near(captureHandle, hwParams, &rate, 0);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot set sample rate (%s)\n", snd_strerror(error));
		puts(errorText);
		return -1;
	}
	else
	{
		
	}

	int numChannels=1;
	error=snd_pcm_hw_params_set_channels(captureHandle, hwParams, numChannels);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot set channel count (%s)\n", snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error = snd_pcm_hw_params(captureHandle, hwParams);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot set parameters (%s)\n", snd_strerror(error));
		puts(errorText);
		return -1;
	}

	snd_pcm_hw_params_free(hwParams);
	error = snd_pcm_prepare(captureHandle);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot prepare audio interface for use (%s)\n",snd_strerror(error));
		puts(errorText);
		return -1;
	}

	error=snd_pcm_nonblock(captureHandle,1);
	if(error<0)
	{
		snprintf(errorText, 512,  "cannot make nonblock (%s)\n",snd_strerror(error));
		puts(errorText);
		return -1;
	}

	int bufferSize=bufferFrames*snd_pcm_format_width(format)/8*numChannels;
	buffer=(int16_t*)malloc(bufferSize);

	puts("... OK\n");
	return 1;
}

int MIC_In_Record(double* buf)
{
	int error=snd_pcm_readi(captureHandle, buffer, bufferFrames);
	if(error!=bufferFrames)
	{
		snprintf(errorText, 512,  "read from audio interface failed (%s)\n", snd_strerror(error));
		return -1;
	}
	for(int i=0;i<bufferFrames;i++)
	{
		buf[i]=buffer[i]/32767.;
	}
	snprintf(errorText, 512, "OK\n");
	return error;
}

int MIC_In_Done()
{
	free(buffer);
	snd_pcm_close(captureHandle);
	return 0;
}

extern "C" EXPORT int MIC_In_Init_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Init);return 0;}
extern "C" EXPORT int MIC_In_Record_Wrapper(ScincVM* s, void*){ScincBoundCall(s,MIC_In_Record);return 0;}
extern "C" EXPORT int MIC_In_Done_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Done);return 0;}
