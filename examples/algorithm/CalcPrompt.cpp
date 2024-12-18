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
	strncp(&(c.vars[0].name[0]),"pi",TOK_LEN);
	c.vars[0].value=M_PI;
	while(1)
	{
		char g[256];
		printf("\n>");
		if(fgets(g,256,stdin))
		{
			c.t[0].SetEnd();
			MakeTokens(c.t,g);
			if(c.t[0].IsEnd())
			{
				continue;
			}

			Stx s={&c, 0, YES};

			char id[32];
			if(s.TryId(id).Yes())
			{
				if(strneq(id,"plot2d",TOK_LEN))
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
