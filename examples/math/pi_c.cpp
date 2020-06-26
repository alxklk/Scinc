#include <stdio.h>
// This example is originated from z88dk (https://github.com/z88dk/z88dk/wiki/pi)
// with some modifications

#define N 2800
#define N1 (N+1)

int main()
{
	int r[N+1];
	int i;
	int k;
	int b;
	int d;
	int c;

	c=0;
	for(i=0;i<N;i++)r[i]=2000;
	for(k=N;k>0;k-=14)
	{
		d=0;
		i=k;
		while(1)
		{
			d+=r[i]*10000;
			b=i*2-1;

			r[i]=d%b;
			d/=b;

			i--;
			if(i<=0)break;
			d*=i;
		}
		printf("%04i", (c+d/10000));
		c=d%10000;
	}
	printf("\nFin\n");
	return 0;
}