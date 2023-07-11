#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>

std::vector<int16_t>accum;

WAVEHDR hdr0;
WAVEHDR hdr1;

size_t bufferFrames=0;
char* buf0=0;
char* buf1=0;

bool quit=false;
MMRESULT res;

void PrepareHeaders(HWAVEIN hwavein)
{
	hdr0.lpData          =buf0;
	hdr0.dwBufferLength  =bufferFrames*2;
	hdr0.dwBytesRecorded =0;
	hdr0.dwUser          =0;
	hdr0.dwFlags         =0;
	hdr0.dwLoops         =1;
	hdr0.lpNext          =0;
	hdr0.reserved        =0;

	res=waveInPrepareHeader(hwavein, &hdr0, sizeof(WAVEHDR));
	printf("@%i res=%i\n", __LINE__,res);

	hdr1.lpData          =buf1;
	hdr1.dwBufferLength  =bufferFrames*2;
	hdr1.dwBytesRecorded =0;
	hdr1.dwUser          =0;
	hdr1.dwFlags         =0;
	hdr1.dwLoops         =1;
	hdr1.lpNext          =0;
	hdr1.reserved        =0;

	res=waveInPrepareHeader(hwavein, &hdr1, sizeof(WAVEHDR));
	printf("@%i res=%i\n", __LINE__,res);
}

void CALLBACK waveInProc(
		HWAVEIN   hwavein,
		UINT      msg,
		DWORD_PTR instance,
		DWORD_PTR param0,
		DWORD_PTR param1
)
{
	switch(msg)
	{
		case MM_WIM_OPEN:
			printf("MM_OPEN\n");
			break;
		case MM_WIM_DATA:
			//printf("MM_DATA\n");
			{
				WAVEHDR* phdr=(WAVEHDR*)param0;
				int size=phdr->dwBytesRecorded;
				//printf("data %i bytes\n", size);
				if(accum.size()>bufferFrames*3)
				{
					accum.erase(accum.begin(), accum.begin()+bufferFrames-bufferFrames*3);
				}
				int pos=accum.size();
				accum.resize(pos+size/2);
				memcpy(&accum.at(pos), phdr->lpData, size);
				if(quit)
				{
					printf("Close\n");
					res=waveInClose(hwavein);
					printf("@%i res=%i\n", __LINE__,res);
				}
				else
				{
					res=waveInAddBuffer(hwavein, phdr, sizeof(WAVEHDR));
					//printf("@%i res=%i\n", __LINE__,res);
				}
			}
			break;
		case MM_WIM_CLOSE:
			printf("MM_CLOSE\n");
			res=waveInUnprepareHeader(hwavein, &hdr0, sizeof(WAVEHDR));
			printf("@%i res=%i\n", __LINE__,res);
			res=waveInUnprepareHeader(hwavein, &hdr1, sizeof(WAVEHDR));
			printf("@%i res=%i\n", __LINE__,res);
			break;
	}
}

WAVEFORMATEX wformat;
HWAVEIN hwavein;

void WaveInStartup(int nFrames)
{
	bufferFrames=nFrames;
	buf0=new char[bufferFrames*2];
	buf1=new char[bufferFrames*2];

	wformat.wFormatTag      = WAVE_FORMAT_PCM;
	wformat.nChannels       = 1;
	wformat.nSamplesPerSec  = 48000;
	wformat.nAvgBytesPerSec = 48000*2;
	wformat.nBlockAlign     = 2;
	wformat.wBitsPerSample  = 16;
	wformat.cbSize          = sizeof(WAVEFORMATEX);

	printf("Before WaveOpen\n");
	res=waveInOpen(&hwavein, WAVE_MAPPER, &wformat, (int64_t)&waveInProc, 0, CALLBACK_FUNCTION);
	printf("After WaveOpen\n");
	printf("@%i res=%i\n", __LINE__,res);

	PrepareHeaders(hwavein);
	res=waveInAddBuffer(hwavein, &hdr0, sizeof(WAVEHDR));
	printf("@%i res=%i\n", __LINE__,res);
	res=waveInAddBuffer(hwavein, &hdr1, sizeof(WAVEHDR));
	printf("@%i res=%i\n", __LINE__,res);
	res=waveInStart(hwavein);
	printf("@%i res=%i\n", __LINE__,res);

}

int MIC_In_Init(int bufFrames)
{
	WaveInStartup(bufFrames);
	return 0;
}

int MIC_In_Record(double* buf)
{
	int count=std::min(bufferFrames,accum.size());
	for(int i=0;i<count;i++)
	{
		buf[i]=accum[i]/32767.;
	}
	accum.erase(accum.begin(), accum.begin()+count);
	return 0;
}

int MIC_In_Done()
{
	quit=true;
	return 0;
}

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C" EXPORT int MIC_In_Init_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Init);return 0;}
extern "C" EXPORT int MIC_In_Record_Wrapper(ScincVM* s, void*){ScincBoundCall(s,MIC_In_Record);return 0;}
extern "C" EXPORT int MIC_In_Done_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Done);return 0;}
