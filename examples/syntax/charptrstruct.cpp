#include <stdio.h>

struct SButton
{
	int type;
};

class CGUI
{
public:
	SButton buttons[1];
	SButton but;
	SButton& but0()
	{
		buttons[0].type=123;
		return buttons[0];
	}
	SButton& but1()
	{
		but.type=234;
		return but;
	}
	SButton& but2()
	{
		int n=0;
		buttons[n].type=345;
		return buttons[n];
	}
	SButton but3()
	{
		buttons[0].type=456;
		return buttons[0];
	}
};

int main()
{
	CGUI gui;
	{SButton& b=gui.but0();printf("type=%i\n", b.type);}
	{SButton& b=gui.but1();printf("type=%i\n", b.type);}
	{SButton& b=gui.but2();printf("type=%i\n", b.type);}
	{SButton  b=gui.but3();printf("type=%i\n", b.type);}
	{
		SButton but;  but. type=123; 
		SButton butA; butA.type=234; 
		SButton buts[2]; buts[0].type=345;  buts[1].type=456; 
		int i=0;
		SButton& but0=but;       printf("type=%i\n", but0.type);
		SButton& but1=buts[0];   printf("type=%i\n", but1.type);
		SButton& but2=buts[i];   printf("type=%i\n", but2.type);
		SButton& but3=buts[i+1]; printf("type=%i\n", but3.type);
		but0=butA; printf("type=%i\n", but0.type);
		but2=butA; printf("type=%i\n", but2.type);
		but0=but1; printf("type=%i\n", but0.type);
		but2=but3; printf("type=%i\n", but2.type);
	}
	return 0; 
}
