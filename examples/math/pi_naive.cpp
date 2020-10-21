#include <stdio.h>

float calcPI(float N)
{
	float sum=2.;
	float nx=2.;
	for(float n=1.;n<N;n++)
	{
		nx=nx*n/(2.*n+1.);
		sum=sum+nx;
	}
	return sum;
}

int main()
{
	printf("Naive 10 iterations:           \n   %.18f\n",calcPI(10));
	printf("Naive 30 iterations:           \n   %.18f\n",calcPI(30));
	printf("Naive 50 iterations:           \n   %.18f\n",calcPI(50));
	puts  ("Reference value, 28 digits:    \n   3.141592653589793238462643383");
	printf("Rational approximation 355/113:\n   %.15f\n",355.0/113.0);
	return 0;
}