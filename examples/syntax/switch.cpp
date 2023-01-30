#include <stdio.h>

int a=0;
int b=0;
int check1()
{
    for(int i=3;i<7;i++)
    {
        switch(i%4)
        {
            case 1:a=1;b=2;break;
            case 2:a=2;printf("Seconc clause\n");break;
            case 3:printf("Third clause no break\n");
            default:
            b=3;
        };
    }
    return 123;
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


int main()
{
    check1();
    check2();
    check3();
    return 0;
}