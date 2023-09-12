#include <stdio.h>

//#define MUL(a,b) ((a)*(b))
//#define ADD(a,b) ((a)+(b))
//#define APPLY(F, x, y) 12*F((x),(y))+5
//#define NONE() puts("Hi\n");

//int mul(int x, int y)
//{
//	return x*y;
//}

#define VA0(fmt,...) printf(fmt,__VA_ARGS__);

#define STR(a) #a
#define S(a) a
#define GLU(a,b) a##b

int main()
{
	int GLU(a,12);
	char* s0=STR(GLU(a,12));
	char* s1=STR(S(GLU(a,12)));
	//NONE()
	//printf("%i\n", MUL((2+3)*5,mul(2+4, mul(5,ADD(3,12)))));
	//printf("%i\n", APPLY(MUL,1,2));
	VA0("%i %s\n",1,"@@@");
	return 0;
}
