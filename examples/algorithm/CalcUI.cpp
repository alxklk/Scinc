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
#define G_SCREEN_SCALE  3
#define G_SCREEN_MODE   1


#include "graphics.h"
#include "Calc.h"
#include "../ui/gui/GUI.h"

#define NO_GREEDY_EVENT

#include "../ws.h"

#include "../graphic/font.h"
#include "../graphic/letters.h"
#include "../graphic/gtext.h"

Calc c;
char text[MAX_TEXT_LEN];
float result;
Graph g;
CFont* pfont;

void UpdateResult()
{
	c.t[0].SetEnd();
	MakeTokens(c.t,text);
	if(c.t[0].IsEnd())
	{
		return;
	}
	Stx s={&c, 0, YES};

	char id[MAX_TEXT_LEN];
	Stx plot=s.TryId(id);
	if(plot.Yes())
	{
		float min=0;
		float max=1;
		if(streq(id,"plot2d",TOK_LEN))
		{
			Stx dims=plot.TryExpr(min).TryExpr(max);
			if(dims.Yes())
			{
				//printf("Draw %f .. %f\n", min, max);
				int xidx=c.AddVariable("x");
				for(int i=0;i<=200;i++)
				{
					float x=min+(max-min)/200.*i;
					c.vars[xidx].value=x;
					float result;
					dims.TryExpr(result);
					//if(i%10==0)printf("%i %f => %f\n",  i, x, result);
					g.Circle(i+280,200+result,0,1,1,0xffff00ff);
				}
			}
			return;
		}
		if(streq(id,"plotpar",TOK_LEN))
		{
			Stx dims=plot.TryExpr(min).TryExpr(max);
			if(dims.Yes())
			{
				int tidx=c.AddVariable("t");
				for(int i=0;i<=1000;i++)
				{
					float t=min+(max-min)/1000.*i;
					c.vars[tidx].value=t;
					float resultx;
					float resulty;
					dims.TryExpr(resultx).TryExpr(resulty);
					g.Circle(280+100*resultx,200+100*resulty,0,1,1,0xffff00ff);
				}
			}
			return;
		}
	}

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
	for(int i=0;i<MAX_TEXT_LEN;i++)text[i]=0;
	strcp(text,"plotpar 0 pi*4 sin(t)*(1+sin(10*t)*.5)*(.1+t*.1) cos(t)*(1+sin(10*t)*.5)*(.1+t*.1)",MAX_TEXT_LEN);
	c.Init();
	CGUI gui;
	strcp(&(c.vars[0].name[0]),"pi",TOK_LEN);
	c.vars[0].value=M_PI;

	pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);

	gui.AddLineEdit(text, 10,10,380,20)
	.SetActionCB([](SWidget*p)->int{
		UpdateResult();
		p->i0=0;
		for(int i=0;i<MAX_TEXT_LEN;i++)p->text[i]=0;
		return 0;
		})
	.SetRenderCB([](SWidget*p, int style)->int{
		g.fillrect(p->x0,p->y0,p->x1-p->x0,p->y1-p->y0,0xffc0d0e0);
		g.clear();
		DrawText(g,*pfont,p->text,0,p->x0+.5,p->y0+14.5,1,1);
		g.fin();
		g.rgba32(0xff000000);
		g.width(1,1);
		g.stroke();
		int curPos=p->x0+p->i0*6;
		int offset=0;
		int w=p->x1-p->x0;
		if(curPos>w-11)
		{
			offset=curPos-(w-11);
			curPos=w-11;
		}
		if(int(Time()*2)%2)
			g.Rect(curPos,p->y0+1,1,17,0xffff00ff);
		else
			g.Rect(curPos,p->y0+1,1,17,0xff0000ff);
		return 0;
		}

	);

	char resultText[MAX_TEXT_LEN];
	UpdateResult();

	while(1)
	{
		g.rgba32(0xff808790);
		g.FillRT();
		WaitForScincEvent(0.01);
		SScincEvent event;
		while(GetScincEvent(event))
		{
			gui.Event(event);
		}
		UpdateResult();
		gui.Render(g);

		snprintf(resultText, MAX_TEXT_LEN, "=%f", result);
		//stexts(resultText, 400, 14, 0xffc0b0c0,1);
		//stexts("Variables:", 10, 35, 0xffc0b0c0, 1);
		g.clear();
		DrawText(g, font, resultText,  0, 402.5, 25.5, 1, 1);
		DrawText(g, font, "Variables:", 0, 11.5, 46.5, 1, 1.);
		for(int i=0;i<NVARS;i++)
		{
			if(c.vars[i].name[0]!=0)
			{
				char svar[128];
				snprintf(svar, 128, "%s=%f", &(c.vars[i].name[0]), c.vars[i].value);
				//stexts(svar, 20, 55+i*20, 0xff000000,1);
				DrawText(g, font, svar, 21.5, 11.5, 55+i*20+11.5, 1, 1);
			}
			else break;
		}

		g.fin();
		g.rgba32(0xff000000);
		g.width(1.,1.);
		g.stroke();
		Present();
	}	
	return 0;
}
