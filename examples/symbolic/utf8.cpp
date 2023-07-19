#include <stdio.h>
#include <locale.h>

#ifdef __SCINC__
	#define const
#endif

struct SUtf8
{
	int p;
	char* s;
};

int U8Next(SUtf8& S)
{
	const char* s=S.s;
	int p=S.p;

	int c0=s[p];
	S.p++;
	if(c0&0x80)
	{
		int c1=s[p+1];
		c0=c0&0x1f;
		c1=c1&0x3f;
		if(c1)
			S.p++;
		return ((c0<<6)+c1)&0xffff;
	}
	else
	{
		return int(c0);
	}
}

void CheckU(const char *s0)
{
	char s[50];
	for(int j=0;j<50;j++)
	{
		for(int i=0;i<50;i++)
		{
			s[i]=s0[i];
			if(s[i]==0)
				break;
		}
		s[j]=0;
		printf("%s\n",s);
		SUtf8 u;
		u.p=0;
		u.s=&s[0];

		for(int i=0;i<128;i++)
		{
			int c=U8Next(u);
			if(c==0)
				break;
			printf("%lc", c);
		}
		printf("\n");
		if(s0[j]==0)
			break;
	}
}

int main()
{
#ifndef __SCINC__
	setlocale(LC_ALL,"");
#endif
	CheckU("перевірка");
	CheckU("ПЕРЕВІРКА");
	return 0;
}