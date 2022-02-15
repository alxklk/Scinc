#include "sound.h"
#include "asters_music.h"

int main()
{
	CMusic music;
	music.Init();
	music.do_out=false;
	int n=0;
	FILE* f=fopen("out.raw", "wb");
	while(true)
	{
		float* pf=&(music.echo[music.echoPos*2]);
		music.GenerateSamples(1000);
		fwrite(pf,1000*2*sizeof(float),1,f);
		if(n>5000)
			break;
		n++;
	}
	fclose(f);
	return 0;
}