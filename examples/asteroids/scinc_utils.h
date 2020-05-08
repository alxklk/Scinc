#ifndef SCINC_UTILS

#define SCINC_UTILS
float bell_curve3(float x)
{
	float x2=x*x;
	return x2*16.*(x2-2.*x+1.);
}

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed)>>11)/65536.0;
}

float Abs(float x)
{
	if(x<0.)
		return -x;
	return x;
}

float Len(float x, float y)
{
	return sqrt(x*x+y*y);
}

float fnear(float x, float y)
{
	float rf=x/y;
	int r=rf+.5;
	if(x<-0.5*y)
		r--;
	float res=x-r*y;
	//if(res<0)res=-res;
	return res;
}
#endif