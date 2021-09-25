#include <cstdio>
#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

struct decoder
{
	mp3dec_ex_t mp3d;
	float mp3_duration;
	float spectrum[32][2]; // for visualization
};

decoder dec;

void decode_samples(void* buf, int bytes)
{
	memset(buf, 0, bytes);
	size_t res=mp3dec_ex_read(&dec.mp3d, (mp3d_sample_t*)buf, bytes/sizeof(mp3d_sample_t));
}

int open_dec(const char *file_name)
{
    if (!file_name || !*file_name)
        return 0;

    memset(&dec, 0, sizeof(dec));

    mp3dec_ex_open(&dec.mp3d, file_name, MP3D_SEEK_TO_SAMPLE);
    if (!dec.mp3d.samples)
        return 0;
    return 1;
}

int close_dec()
{
    mp3dec_ex_close(&dec.mp3d);
    memset(&dec, 0, sizeof(dec));
    return 0;
}

extern "C" EXPORT int open_dec_wrapper(ScincVM* s, void*){ScincBoundCall(s,open_dec);return 0;}
extern "C" EXPORT int decode_samples_wrapper(ScincVM* s, void*){ScincBoundCall(s,decode_samples);return 0;}
extern "C" EXPORT int close_dec_wrapper(ScincVM* s, void*){ScincBoundCall(s,open_dec);return 0;}
