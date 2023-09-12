#include <stdio.h>

#define APPLY(F, x, y) 12*F((x),(y))+5
#define MUL(a,b) ((a)*(b))
#define ADD(a,b) ((a)+(b))
#define MADD(a,b,c) ADD(MUL((a),(b)),(c))

#define S(x) (x)+(x)
#define T(x) (x*2)
#define U(x) S(x/2)

#define L(x) #x"%" 
#define M(x) x 
#define N0(x) L(MUL(x,x)+3)
#define N1(x) M(MUL(x,x)+3)

int main()
{
	printf("%i\n", ADD(1,MUL(2,3)));
	printf("%i\n", APPLY(MUL,1,2));
	printf("%i\n", APPLY(ADD,ADD(1,2),MUL(2,3)));
	printf("%i\n", MADD(1,2,3));
#define ADD(a,b) (((a)*4)/(b)-253)
	printf("4: %i\n", APPLY(ADD,MADD(2,3,4),MUL(2,3)));
#define ADD(a,b) (((a)*8)/(b))
	printf("8: %i\n", APPLY(ADD,MADD(2,3,4),MUL(2,3)));
#define ADD(a,b) (((a)*128)/(b)+5)
	printf("128: %i\n", APPLY(ADD,MADD(2,3,4),MUL(2,3)));
	printf("%i\n", S(T(5)));
	printf("%i\n", U(5));
	printf("22 '%s'\n", L(1));
	printf("23 '%s'\n", N0(1));
	printf("24 '%i'\n", N1(1));
	return 0;
}
