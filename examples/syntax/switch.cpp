#include <stdio.h>

int check1()
{
    int a=0;
    int b=0;
	for(int i=3;i<7;i++)
	{
		switch(i%4)
		{
			case 1:a=1;b=2;break;
			case 2:a=2;printf("Seconc clause\n");break;
			case 3:printf("Third clause no break\n");
			default:b=3;
		};
		printf("a=%i b=%i\n",a,b);
	}
	return 0;
}

int check2()
{
	for(int i=0;i<10;i++)
	{
		switch(i)
		{
			case 2:
				continue;
			case 5: return 3;
		}
		printf("%i\n",i);
	}
	return 0;
}


int check3()
{
	for(int i=0;i<10;i++)
	{
		printf("\n%i: ",i);
		int g=0;
		switch(i%5)
		{
			for(;g<8;g++)
			{
				break;
			int b;
			case 0:
				printf("0 ");
			case 1:
				printf("1 ");
			}
				break;
			case 2:
				{
					int j=0;
					while(j<3)
					{
						printf("j ");
						break;
					}
					break;
				}
			case 3:
				continue;
			case 4:
				printf("4");
			default:
				break;
		}
	}
	printf("\n");
	return 0;
}


int check4()
{
	for(int i=1;i<=30;i++)
	{
		printf("%i: ",i);
		switch(i%5)
		{
			case 4:case 1:case 2:case 3:
			switch(i%3)
			{
				case 2:case 1:break;
				default:printf("Fizz");
			}
			break;
			case 0:
			switch(i%3)
			{
				case 0:printf("Fizz");break;
			}
			for(int i=0;i<4;i++)printf("%c","Buzz"[i]);
		}
		printf("\n");
	}
	return 0;
}

int check5()
{
    for(int i=1;i<100;i++)
    {
        switch(i%15)
        {
            default:printf("%i\n",i);break;
            case 3:case 6:case 9:case 12:printf("Fizz\n");break;
            case 5:case 10:printf("Buzz\n");break;
            case 0:printf("FizzBuzz\n");break;
        }
    }
    return 0;
}

int check6()
{
    int i=0;
    switch(i)
    {
        while(true)
        {
            printf("%i\n",i-4);
            printf("Fizz\n");
            printf("%i\n",i-2);
            printf("%i\n",i-1);
            printf("FizzBuzz\n");
            case 0:
            printf("%i\n",i+1);
            printf("%i\n",i+2);
            printf("Fizz\n");
            printf("%i\n",i+4);
            printf("Buzz\n");
            printf("Fizz\n");
            printf("%i\n",i+7);
            printf("%i\n",i+8);
            printf("Fizz\n");
            printf("Buzz\n");
            i+=15;
            if(i>90)break;
        }
    }
    return 0;
}


int duff_device(int count)
{
	int n = (count + 7) / 8;
	int i=0;
	switch (count % 8)
	{
		case 0: while(true){printf("%i ",++i);
		case 7:printf("%i ",++i);
		case 6:printf("%i ",++i);
		case 5:printf("%i ",++i);
		case 4:printf("%i ",++i);
		case 3:printf("%i ",++i);
		case 2:printf("%i ",++i);
		case 1:printf("%i ",++i);
					if(--n<=0)break;
					}
	}
	return 0;
}

int main()
{
	check1();
	check2();
	check3();
	check4();
	check5();
	check6();
	for(int i=1;i<35;i+=4)
	{
		duff_device(i);
		printf("\n");
	}
	return 0;
}