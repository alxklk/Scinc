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

#define G_SCREEN_WIDTH  512
#define G_SCREEN_HEIGHT 512
#define G_SCREEN_SCALE  2
#define G_SCREEN_MODE   1


#include "graphics.h"
#include "Calc.h"
#include "../ui/gui/GUI.h"
#include "../ws.h"

Calc c;
char text[MAX_TEXT_LEN];
float result;

void UpdateResult()
{
	c.t[0].SetEnd();
	MakeTokens(c.t,text);
	if(c.t[0].IsEnd())
	{
		return;
	}
	Stx s={&c, 0, YES};
	float res;
	Stx e=s.TryAssign(res);
	if(!e.Yes())
		e=s.TryExpr(res);
	if(e.Yes())
	{
		result=res;
	}
}

int main()
{
	c.Init();
	CGUI gui;
	Graph g;
	strcp(&(c.vars[0].name[0]),"pi",TOK_LEN);
	c.vars[0].value=M_PI;
	for(int i=0;i<MAX_TEXT_LEN;i++)text[i]=0;
	
	gui.AddLineEdit(text, 10,10,400,20).SetCB([](SButton*p)->int{
		UpdateResult();
		p->cur=0;
		p->text[0]=0;
		return 0;
		}
	);

	char resultText[MAX_TEXT_LEN];

	while(1)
	{
		WaitForScincEvent(0.1);
		SScincEvent event;
		while(GetScincEvent(event))
		{
			gui.Event(event);
		}
		UpdateResult();
		g.rgba32(0xff808790);
		g.FillRT();
		gui.Render(g);

		snprintf(resultText, MAX_TEXT_LEN, "=%f", result);
		stexts(resultText, 415, 14, 0xff000000,1);
		stexts("Variables:", 10, 35, 0xff000000, 1);
		for(int i=0;i<NVARS;i++)
		{
			if(c.vars[i].name[0]!=0)
			{
				char svar[128];
				snprintf(svar, 128, "%s=%f", &(c.vars[i].name[0]), c.vars[i].value);
				stexts(svar, 20, 55+i*20, 0xff000000,1);
			}
			else break;
		}

		Present();
	}	
	return 0;
}
