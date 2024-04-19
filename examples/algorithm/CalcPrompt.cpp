/**
 * @file Calc.cpp
 * @author klk
 * @brief Simple calculator usage example
 * @version 0.1
 * @date 2024-03-28
 * 
 * @copyright Copyright (c) 2024
 * 
 *  See Calc.h for details
 * 
 */

#include "Calc.h"

int main()
{
	Calc c;
	c.Init();
	strcp(&(c.vars[0].name[0]),"pi",TOK_LEN);
	c.vars[0].value=M_PI;
	while(1)
	{
		char g[256];
		printf("\n>");
		if(fgets(g,256,stdin))
		{
			//printf("Got line `%s`", g);
			c.t[0].SetEnd();
			MakeTokens(c.t,g);
			if(c.t[0].IsEnd())
			{
				continue;
			}
			/*
			bool end=false;
			for(int i=0;i<NTOK;i++)
			{
				if(i)printf(" ");
				int type=t[i].type;
				switch(type)
				{
					case NUM: printf("%f", t[i].GetNumber());break;
					case ID:  printf("%s", t[i].value);break;
					case OP:  printf("%c", t[i].value[0]);break;
					case END: printf("\n"); end=true; break;
				}
				if(end)
				{
					break;
				}
			}*/
			Stx s={&c, 0, YES};

			char id[32];
			if(s.TryId(id).Yes())
			{
				if(streq(id,"plot2d",TOK_LEN))
				{
					int xidx=c.AddVariable("x");
					for(float f=-10.;f<=10;f+=0.1)
					{
						c.vars[xidx].value=f;
						Stx s1={&c,1,YES};
						float result;
						s1.TryExpr(result);
						printf("%i %f %f\n", xidx, c.vars[xidx].value, result);
					}
					continue;
				}
			}


			float res;
			Stx e=s.TryAssign(res);
			if(!e.Yes())
				e=s.TryExpr(res);
			if(e.Yes())
			{
				printf("Result: %f\n", res);
			}
			else
			{
				// printf("No expr or error\n");
			}
		}else return 0;
	}
	return 0;
}
