#ifndef ASTER_MUSIC

#define ASTER_MUSIC

#define M_PI 3.141592654

float s(float t)
{
	return sin((t-(int)t)*2.*M_PI)*.5;
	//return (t-(int)t)<0.5?-.5:.5;

}

float s1(float t)
{
	//if(t<0.2)return (t-(int)t)<0.1?-.5:.5;
	if(t<0.2)return sin((t-(int)t)/.2*2.*M_PI)*.5;
	return 0;
}

float mod(float a, float b)
{
	int r=a/b;
	return a-b*r;
}

float sndVal(float t)
{
	int idx0=t/4096.;idx0=idx0&31;
	int idx1=t/65536.;idx1=idx1&15;
	int idx2=t/65536./2.;idx2=idx2&15;

	float w=t/4096.;
	w-=float(int(w));
	w=1.-w;

	float i1=(t*("08676043566021210123456787654320"[idx0]-'0')*("0120421012034200"[idx1]-'0'));
	float i2=(t*("01020301020301201010101010101010"[idx0]-'0')*("2432342324323323"[idx2]-'0'));
	return (s(mod(i1,1024.)/1024.)*.35*w*w-s(mod(i1,512.)/512.)*.27-s1(mod(i2,1024.)/1024.)*.35*w);

}

class CMusic
{
public:
	int sample;
	int echoPos;
	bool do_out;
	float echo[24000];
	void Init()
	{
		sample=0;
		echoPos=0;
		do_out=true;
		for(int i=0;i<24000;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			float ts=sample*(.68);
			float l=sndVal(ts+sin(sample/40000.)*150);
			float r=sndVal(ts-sin(sample/40000.)*150);
//			float l=sndVal(sample*.375+sin(sample/5000.)*500);
			sample++;
			int ep=echoPos*2;
			l=(l+echo[ep  ]*0.9)*.75;
			r=(r+echo[ep+1]*0.9)*.75;
			//l=r=sin(sample/44100.*6*800);
			echo[ep  ]=l;
			echo[ep+1]=r;
			echoPos++;
			echoPos=echoPos%12000;
			//if(do_out)snd_out(l,r);
		}
		if(do_out)
		{
			CSound s;
			int idx=echoPos-nSamples;
			int count=nSamples;
			if(idx<0)
			{
				count+=idx;
				s.snd_out_buf(&(echo[(12000+idx)*2]),-idx);
				idx=0;
			}
			s.snd_out_buf(&(echo[idx*2]),count);
		}
	}	
};

#endif