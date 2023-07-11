#define FIXED_DIGITS 155
#define FIXED_FRACTS 145

#include "fixed.h"

fixed  muller_recurrence(fixed& y, fixed& z)
{
	fixed f108;
	f108.Seti(108);
	fixed f815;
	f815.Seti(815);
	fixed f1500;
	f1500.Seti(1500);
	return f108-((f815-f1500/z)/y);
}

double muller_recurrence(double& y, double& z)
{
	return 108.-((815.-1500./z)/y);
}


int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	fixed fi0;fi0.Sets("4");
	fixed fi1;fi1.Sets("4.25");
	double fl0=4;
	double fl1=4.25;
	printf("   #  |    double    |    fixed %i.%i\n",FIXED_DIGITS,FIXED_FRACTS);
	printf("------+--------------+------------------------------------------\n");
	for(int i=2;i<100;i++)
	{
		fixed fi2=muller_recurrence(fi1,fi0);
		double fl2=muller_recurrence(fl1,fl0);
		printf("% 4i: | %12.8f | ",i,fl2);fi2.Print();printf("\n");
		fi0=fi1;fi1=fi2;
		fl0=fl1;fl1=fl2;
	}
	return 0;
}
