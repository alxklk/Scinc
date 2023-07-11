#define PI 3.141592654
#include <cmath>

struct cplx
{
	double re;
	double im;
};

cplx I;

#define CPLX_SET(o,new_re,new_im) (o).re=(new_re);(o).im=(new_im);
#define CPLX_MUL(o,l,r) {double re=(l).re*(r).re-(l).im*(r).im; double im=(l).im*(r).re+(r).im*(l).re; (o).re=re; (o).im=im;}
#define CPLX_ADD(o,l,r) (o).re=(l).re+(r).re;(o).im=(l).im+(r).im;
#define CPLX_SUB(o,l,r) (o).re=(l).re-(r).re;(o).im=(l).im-(r).im;
#define CPLX_SWAP(l,r) {double t=(l).re;(l).re=(r).re;(r).re=t;t=(l).im;(l).im=(r).im;(r).im=t;}

int ILog2(int x)
{
	if(x<0)x=-x;
	int i=-1;
	while(true)
	{
		if(!x)
			return i;
		i++;
		x=x>>1;
	}
}

int FFT_reversedBits[NFFT];

void FFT_Init()
{
	int m = 32-ILog2(NFFT);
	for(int i=0;i<NFFT;i++)
	{
		int b=i;
		b = (((b >> 1) & 0x55555555) | ((b & 0x55555555) << 1));
		b = (((b >> 2) & 0x33333333) | ((b & 0x33333333) << 2));
		b = (((b >> 4) & 0x0f0f0f0f) | ((b & 0x0f0f0f0f) << 4));
		b = (((b >> 8) & 0x00ff00ff) | ((b & 0x00ff00ff) << 8));
		b = (((b >> 16) & 0x0000ffff) | (b << 16));
		b = (b >> m)&(0x7fffffff>>(m-1));
		FFT_reversedBits[i]=b;
	}
}

void _fft(cplx* x, int N)
{
	int k = N;
	int n;
	double thetaT = 3.141592653 / N;
	cplx phiT;
	phiT.re = cos(thetaT);
	phiT.im =-sin(thetaT);
	cplx T;
	while (k > 1)
	{
		n = k;
		k = k >> 1;
		CPLX_MUL(phiT, phiT, phiT);
		CPLX_SET(T, 1.0, 0.0);
		for (int l = 0; l < k; l++)
		{
			for (int a = l; a < N; a += n)
			{
				int b = a + k;
				cplx& xa = x[a];
				cplx& xb = x[b];
				cplx t; CPLX_SUB(t,xa,xb);
				CPLX_ADD(xa,xa,xb);
				CPLX_MUL(xb,t,T);
			}
			CPLX_MUL(T,T,phiT);
		}
	}
	for (int a = 0; a < N; a++)
	{
		int b = FFT_reversedBits[a];
		if (b > a)
		{
			cplx& xa = x[a];
			cplx& xb = x[b];
			CPLX_SWAP(xa,xb);
		}
	}
}

void fft(cplx* buf, cplx* out, int n)
{
	memcpy(out,buf,n*sizeof(cplx));
	_fft(out, n);
}
