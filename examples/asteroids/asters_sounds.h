#include "scinc_utils.h"

int MakeShootSound(int len)
{
	int res=snd_add(len);
	float dlen=1.0/len;
	{
		float t=0.;
		float f=0.;
		for(int i=0;i<len;i++)
		{
			float v=bell_curve3(f)*.05;
			float x=t*440.*12.*M_PI;
			float l=sin(x)*v*sin(f*M_PI*14);
			if(f<0.25)l=l+sin(x*8.)*bell_curve3(f/0.25)*0.25*cos(f*M_PI*14.);
			snd_data(res,i,l,l);
			t+=1./44100.*(f*f)*2.2;
			f+=dlen;
		}
	}
	return res;
}

int MakeExplodeSound(int len, int seed)
{
	int res=snd_add(len);
	{
		float t=0.;
		float f=0.;
		float dlen=1./len;
		float fr0=frand(seed);
		float fr1=frand(seed);
		for(int i=0;i<len;i++)
		{
			float v=(1.-f)*(1-f)*bell_curve3(f);
			float l=fr1*v*.15;
			if(!(i%128))fr0=frand(seed);
			if(!(i%32))fr1=frand(seed);
			if(f<0.07)l=l+fr0*bell_curve3(f/0.07)*0.4;
			snd_data(res,i,l,l);
			f+=dlen;
		}
	}
	return res;
}

int MakeEngineSound(int len, int seed)
{
	int res=snd_add(len);
	{
		float t=0.;
		float fr0=frand(seed);
		float f=0.;
		float dlen=1./len;
		for(int i=0;i<len;i++)
		{
			float f=i/12000.0;
			float v=bell_curve3(f);
			float l=fr0*v*.125+frand(seed)*v*0.125;
			if(!(i%64))
				fr0=frand(seed);
			snd_data(res,i,l,l);
			f+=dlen;
		}
	}
	return res;
}
