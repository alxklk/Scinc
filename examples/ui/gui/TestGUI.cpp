#define G_SCREEN_SCALE 4
#include "graphics.h"
#include "GUI.h"

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

	gui.AddButton("Go", 20,20,30,20,'go__').OnPress([](SButton*p)->int
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

	gui.AddSelect("Select 0", 20,50,50,20,'sel0',1,&sel);
	gui.AddSelect("Select 1", 20,80,50,20,'sel1',2,&sel);
	gui.AddCheck("Check 0", 20,110,50,20,'ch_0',&ch0);
	gui.AddCheck("Check 1", 20,140,50,20,'ch_1',&ch1);
	gui.AddSlide("val0", 20,170,201,15,'val0',&val[0],false,0,100);
	gui.AddSlide("", 130,10,15,100,'val1',&val[1],true,0,1);
	gui.AddSlide("", 150,10,15,100,'val2',&val[2],true,0,1);
	gui.AddSlide("", 170,10,15,100,'val3',&val[3],true,0,1);
	gui.AddSlide("", 190,10,15,100,'val4',&val[4],true,0,1);

	SetPresentWait(true);

	while(1)
	{
		while(GetScincEvent(event))
		{
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