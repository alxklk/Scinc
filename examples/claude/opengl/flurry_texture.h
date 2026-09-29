// flurry_texture.h - Flurry's smoke texture, shared by flurry.cpp and
// ophiura_flurry.cpp. Include after scgl.h; call MakeFlurryTexture() once a
// GL context exists. It builds a 256x256 atlas of 8x8 tiles, each a soft
// round disc with random speckles, smoothed; successive tiles blend into
// each other and the last one is averaged with the first, so stepping the
// tile index (0..63, u = (i & 7) / 8, v = (i >> 3) / 8) animates the smoke.
// Translated from xscreensaver hacks/glx/flurry-texture.c:
//
// Copyright (c) 2002, Calum Robinson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the author nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef FLURRY_TEXTURE_H
#define FLURRY_TEXTURE_H

int ftSeed = 7771;
int FtRand()                           // the texture's own random numbers
{
	ftSeed = ftSeed * 1103515245 + 12345;
	return (ftSeed >> 8) & 0x7fffff;
}

int ftSmall[32 * 32];
int ftFilter[32 * 32];
int ftBig[256 * 256];
int ftPixels[256 * 256];
int ftSmallFirst = 1;

void FtSmooth()
{
	for (int i = 1; i < 31; i++)
		for (int j = 1; j < 31; j++)
		{
			float t = ftSmall[i * 32 + j] * 4;
			t = t + ftSmall[(i - 1) * 32 + j] + ftSmall[(i + 1) * 32 + j];
			t = t + ftSmall[i * 32 + j - 1] + ftSmall[i * 32 + j + 1];
			ftFilter[i * 32 + j] = int(t / 8.0);
		}
	for (int i = 1; i < 31; i++)
		for (int j = 1; j < 31; j++)
			ftSmall[i * 32 + j] = ftFilter[i * 32 + j];
}

void FtSpeckle()
{
	for (int i = 2; i < 30; i++)
		for (int j = 2; j < 30; j++)
		{
			int speck = 1;
			while (speck <= 32 && (FtRand() % 2) != 0)
			{
				int v = ftSmall[i * 32 + j] + speck;
				if (v > 255) v = 255;
				ftSmall[i * 32 + j] = v;
				speck = speck + speck;
			}
			speck = 1;
			while (speck <= 32 && (FtRand() % 2) != 0)
			{
				int v = ftSmall[i * 32 + j] - speck;
				if (v < 0) v = 0;
				ftSmall[i * 32 + j] = v;
				speck = speck + speck;
			}
		}
}

void FtMakeSmall()
{
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 32; j++)
		{
			float r = sqrt((i - 15.5) * (i - 15.5) + (j - 15.5) * (j - 15.5));
			float t = 0.0;
			if (r <= 15.0)
				t = 255.0 * cos(r * 3.14159265358979 / 31.0);
			if (ftSmallFirst)
				ftSmall[i * 32 + j] = int(t);
			else
			{
				int v = int((t + ftSmall[i * 32 + j] + ftSmall[i * 32 + j]) / 3);
				if (v > 255) v = 255;
				ftSmall[i * 32 + j] = v;
			}
		}
	ftSmallFirst = 0;
	FtSpeckle();
	FtSmooth();
	FtSmooth();
}

void FtCopySmallToBig(int k, int l)
{
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 32; j++)
			ftBig[(i + k) * 256 + j + l] = ftSmall[i * 32 + j];
}

void FtAverageLastAndFirst()
{
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 32; j++)
		{
			int v = (ftSmall[i * 32 + j] + ftBig[i * 256 + j]) / 2;
			if (v > 255) v = 255;
			ftSmall[i * 32 + j] = v;
		}
}

// The original uploads luminance+alpha; here grey RGB with the same alpha.
int MakeFlurryTexture()
{
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
		{
			if (i == 7 && j == 7)
				FtAverageLastAndFirst();
			else
				FtMakeSmall();
			FtCopySmallToBig(i * 32, j * 32);
		}
	for (int i = 0; i < 256 * 256; i++)
	{
		int v = ftBig[i];
		ftPixels[i] = (v << 24) | (v << 16) | (v << 8) | v;
	}
	int tex = glwGenTexture();
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
	glwTexImage2D(256, 256, ftPixels);
	return tex;
}

#endif // FLURRY_TEXTURE_H
