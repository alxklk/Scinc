#define G_SCREEN_SCALE 4
#include "graphics.h"
#include "GUI.h"
#include <00out.h>
#include "../../ws.h"

int main()
{
	CGUI gui;
	gui.Init();
	Graph g;

	SScincEvent event;
	int sel=0;
	int ch0=0;
	int ch1=1;
	float val[5]={.5,.3,.7,0.,1.};

	gui.AddButton("Go", 20,20,30,20,'go__').SetCB([](SButton*p)->int
	{
		SButton& self=*p;
		if(*(self.text)=='G')
		{
			self.text="Stop";
			self.x1+=5;
			self.y1-=10;
		}
		else
		{
			self.text="Go";
			self.x1-=5;
			self.y1+=10;
		}
		return 1;
	}).SetTitle("Stop");

	gui.AddSelect("Select A", 20,50,80,20,'sel0',1,&sel).SetCB([](SButton*p)->int{p->text="002";return 0;});
	gui.AddSelect("Select B", 20,80,80,20,'sel1',2,&sel);
	gui.AddCheck("Check 0", 20,110,80,20,'ch_0',&ch0).SetCB([](SButton*p)->int{p->text="001";return 0;});
	gui.AddCheck("Check 1", 20,140,80,20,'ch_1',&ch1);
	gui.AddSlide("val0", 20,170,201,15,'val0',&val[0],false,0,100);
	gui.AddFrame("", 120,10,95,130);
	gui.AddStatic("Values", 140,10,50,20);
	gui.AddSlide("", 130,30,15,100,'val1',&val[1],true,0,1).SetCB([](SButton*p)->int{printf("%f\n",*p->fDest);return 0;});
	gui.AddSlide("", 150,30,15,100,'val2',&val[2],true,0,1);
	gui.AddSlide("", 170,30,15,100,'val3',&val[3],true,0,1);
	gui.AddSlide("", 190,30,15,100,'val4',&val[4],true,0,1);
	char editLine[MAX_TEXT_LEN]={};
	gui.AddStatic(editLine, 20,210,100,20);
	gui.AddLineEdit(editLine, 20,190,100,20).SetCB([](SButton*p)->int{printf("'%s'\n",p->text);return 0;});

	SetPresentWait(true);

	while(1)
	{
		while(GetScincEvent(event))
		{
			printf("event\n");
			if(gui.Event(event))
				continue;
		}
		g.rgba32(0xff505050);
		g.FillRT();
		gui.Render(g);
		Present();

	}	
	return 0;
}