#ifndef ASTER_MUSIC

#define ASTER_MUSIC

#define M_PI 3.141592654

float s(float t)
{
	return sin(t*2.*M_PI)*.5;
}

float s1(float t)
{
	if(t<0.2)return sin(t/.2*2.*M_PI)*.5;
	return 0;
}

float mod(float a, float b)
{
	int r=a/b;
	return a-b*r;
}

float sndVal(float t)
{
	int idx0=t/4096.;idx0=idx0&15;
	int idx1=t/65536.;idx1=idx1&15;
	int idx2=t/65536./2.;idx2=idx2&15;

	float i1=(t*("0867604356602121"[idx0]-'0')*("0120421012034200"[idx1]-'0'));
	float i2=(t*("0102030102030120"[idx0]-'0')*("2432342324323323"[idx2]-'0'));
	return (s(mod(i1,1024)/1024.)*.35-s(mod(i1,256)/256.)*.17-s1(mod(i2,1024)/1024.)*.35);

}

class CMusic
{
public:
	int sample;
	int echoPos;
	float echo[16384];
	void Init()
	{
		sample=0;
		echoPos=0;
		for(int i=0;i<16384;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			float l=sndVal(sample*.75+sin(sample/20000.)*150);
//			float l=sndVal(sample*.375+sin(sample/5000.)*500);
			sample++;
			l=(l+echo[echoPos]*0.9)*.7;
			echo[echoPos]=l;
			echoPos++;
			echoPos=echoPos%11150;
			snd_out(l,l);
		}
	}	
};

#endif