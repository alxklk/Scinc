#include <stdio.h>
#include <locale.h>

#ifdef __SCINC__
	#define const
	#define unsigned
#endif

struct SUtf8
{
	int p;
	const char* s;
};

int U8Next(SUtf8& S)
{
	const char* s=S.s;
	int p=S.p;

	int c0=(unsigned char)s[p];
	S.p++;

	if(c0<0x80)
	{
		return int(c0);
	}
	else if(c0<0xE0)
	{
		int c1=s[p+1];
		if(!c1)return 0;
		S.p++;
		return ((c0&0x1f)<<6)|(c1&0x3f);
	}
	else if(c0<0xF0)
	{
		int c1=s[p+1];
		if(!c1)return 0;
		int c2=s[p+2];
		if(!c2)return 0;
		S.p+=2;
		return ((c0&0x1f)<<12)|((c1&0x3f)<<6)|(c2&0x3f);
	}
	else
	{
		c0=c0&0x07;
		int c1=s[p+1];
		if(!c1)return 0;
		int c2=s[p+2];
		if(!c2)return 0;
		int c3=s[p+3];
		if(!c3)return 0;
		S.p+=3;
		return ((c0&0x1f)<<18)|((c1&0x3f)<<12)|((c2&0x3f)<<6)|(c3&0x3f);
	}
	return 0;
}

void CheckUtfError(const char *s0)
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
		printf("'%s'\n>",s);
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

void CheckUtf(const char *s)
{
		printf("'%s'\n>",s);
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
}

int main()
{
#ifndef __SCINC__
	setlocale(LC_ALL,"");
#endif
	CheckUtf("перевірка");
	CheckUtf("AöЖ€𝄞 ❤ ➆ 🁂  🃗 ❓ ㏆ ㏆㏆㏆  ﬃ ﬃ 🀛");
	CheckUtf("ПЕРЕВІРКА");
	return 0;
}