#define G_SCREEN_SCALE 2
#include "graphics.h"
#include "menu.h"

#include "../../ws.h"

Graph g;
SMenu menu;

int MenuCommand0(char* command, int arg)
{
	printf(" Callback for menu '%s' %i\n", command, arg);
	return 0;
}

int main()
{
	menu.Init();
	menu.cmdHandler=&MenuCommand0;

	menu.Create()
	.P("=")
		.M("About","about")
		.M("Exit","exit").C([](SMenuItem&i)->int{exit(0);return 0;})
	.P("File")
		.M("New","file_new").C([](SMenuItem&i)->int{printf("Item %s\n", i.name);return 0;})
		.M("Load","file_load")
		.M("Save","file_save")
		.M("Save As...","file_save_as")
	.P("View")
		.M("Show helper","view_showall")
		.M("Hide","view_hide")
	.P("Help")
		.M("Context","help_context")
		.M("Index","help_index")
	;

	while(true)
	{
		g.rgba32(0xff304050);
		g.clear();
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.fill1();
		g.clear();
		menu.MenuDraw(g,0,0,640,20);

		SScincEvent ev;
		while(GetScincEvent(ev))
		{
			if(menu.MenuHandleEvent(0,0,640,20,ev))
			{
				//printf(" Event %c%c%c%c handled by menu\n",(ev.type&0xff000000)>>24,(ev.type&0xff0000)>>16,(ev.type&0xff00)>>8,(ev.type&0xff));
				//printf("      handled by menu\n");
				continue;
			}
			if(ev.type=='MMDN')
			{
				printf("%i %i %i %i\n", ev.type, ev.x, ev.y, ev.z);
			}
			if(ev.type=='KBDN')
			{
				printf("Key %i %i %i %i\n", ev.type, ev.x, ev.y, ev.z);
			}
			if(((ev.type>>16)&0xffff)=='MW')
			{
				printf("Wheel\n");
			}
		}
		Present();
	}
	return 0;
}