#define G_SCREEN_SCALE 2
#include "graphics.h"
#include "GUI.h"
#include <00out.h>
#include "../../ws.h"

Graph g;


int main()
{
	SScincEvent event;
	SetPresentWait(true);

	CGUI gui;
	char text[MAX_TEXT_LEN];
	for(int i=0;i<MAX_TEXT_LEN;i++)text[i]=0;
	
	gui.AddLineEdit(text, 100,50,100,20);

	while(1)
	{
		WaitForScincEvent(0.1);
		while(GetScincEvent(event))
		{
			gui.Event(event);
		}
		g.rgba32(0xff808790);
		g.FillRT();
		gui.Render(g);
		Present();
	}	
	return 0;
}