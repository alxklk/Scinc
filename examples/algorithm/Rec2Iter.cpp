// Converting recursive algorythm
// to iterative. As an exsmple
// using following task:
// Divide interval into several
// subintervals of relative sizes,
// repeat for each subinterval
// until certain nesting depth met.

#include <stdio.h>

float d[3]={.5,.3,.2};
int np=3;
int maxDepth=3;

float p;

void Rec0(float x, int depth)
{
	if(depth>=maxDepth)
	{
		for(int i=0;i<depth;i++)printf(" - ");printf("[%4.2f %4.2f]\n", p, p+x);
		p+=x;
	}
	else
	{
		for(int i=0;i<depth;i++)printf(" - ");printf("%4.2f %4.2f\n", p, p+x);
		for(int i=0;i<np;i++)
		{
			//printf("d++\n");
			Rec0(x*d[i],depth+1);
			//printf("d--\n");
		}
	}
}

void Rec(float x)
{
	Rec0(x,0);
}

struct RState
{
	int i;
	float d;
};

bool Iter0(int& depth, RState* s)
{
	if(depth==maxDepth)
	{
		float p0=p;
		p+=s[depth].d;
		for(int i=0;i<depth;i++)printf(" - ");printf("[%4.2f %4.2f]\n", p0, p);
		//printf("d--\n");
		depth--;
		return false;
	}
	else
	{
		if(s[depth].i==0)
		{
			for(int i=0;i<depth;i++)printf(" - ");
			printf("%4.2f %4.2f\n", p, p+s[depth].d);
		}
		if(s[depth].i<np)
		{
			s[depth+1].i=0;
			s[depth+1].d=s[depth].d*d[s[depth].i];
			s[depth].i++;
			//printf("d++\n");
			depth++;
			return false;
		}
		else
		{
			if(depth==0)
			{
				// Finished
				return true;
			}
			//printf("d--\n");
			depth--;
			return false;
		}
	}
}

void Iter(float x)
{
	RState s[10];
	s[0].d=x;
	s[0].i=0;
	int depth=0;
	while(true)
	{
		if(Iter0(depth, s))break;
	}
}

int main()
{
	printf("* * * Recursive * * *\n");
	p=0;
	Rec(1);
	printf("* * * Nonrecursive * * *\n");
	p=0;
	Iter(1);
	return 0;
}
