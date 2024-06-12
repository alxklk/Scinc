#include "filedlg.h"
#include "graphics.h"
#include <unistd.h>

#define NO_GREEDY_EVENT

#define G_SCREEN_MODE 1
#define G_SCREEN_SCALE 3


#include "../../ws.h"

int main(int argc, pchar* argv)
{
	SItemList l;
	l.Init();
	chdir("/home/klk");
	char cwd[512];
	getcwd(cwd,512);
	listdir(cwd, l);
	for(int i=0;i<l.nItems;i++)
	{
		printf("%i %s\n", l.GetFlag(i), l.GetName(i));
	}

	Graph g;
	SetPresentWait(true);
	int hover=-1;
	int selected=0;
	int offset=0;
	double clickTime=Time();
	while(1)
	{
		SScincEvent ev;
		WaitForScincEvent(.5);
		while(GetScincEvent(ev))
		{
			if((ev.type&0xff000000)=='M\0\0\0')
			{
				//printf("%i %i\n", ev.x, ev.y);
				hover=(ev.y-40)/10+offset;
				if(ev.type=='MLDN')
				{
					double newT=Time();
					double delta=newT-clickTime;
					printf("delta=%f\n", delta);
					if((hover>=0)&&(hover<l.nItems)&&(selected==hover)&&(delta<0.4))
					{
						printf("doubleclick %s\n", l.GetName(selected));
						if(l.GetFlag(selected))
						{
							chdir(l.GetName(selected));
							l.Init();
							listdir(".", l);
							getcwd(cwd,512);
							selected=0;
							offset=0;
						}
					}
					clickTime=newT;
					selected=hover;
				}
				else if(ev.type=='MWDN')
				{
					if(selected<l.nItems-1)selected++;
				}
				else if(ev.type=='MWUP')
				{
					selected--;
				}
			}
			else if(ev.type=='KBDN')
			{
				if(ev.x==KEYCODE_DOWN)
				{
					if(selected<l.nItems-1)selected++;
					if(selected-offset>15)offset=selected-15;
					if(offset<0)offset=0;
				}
				else if(ev.x==KEYCODE_UP)
				{
					if(selected>0)selected--;
					if(selected-offset<5)
					{
						offset=selected-5;
						if(offset<0)offset=0;
					}
				}
 				else if(ev.x==KEYCODE_HOME)
				{
					selected=0;
					offset=0;
				}				
 				else if(ev.x==KEYCODE_END)
				{
					selected=l.nItems-1;
					if(selected-offset>15)offset=selected-15;
					if(offset<0)offset=0;
				}				
 				else if((ev.x==KEYCODE_ENTER)||(ev.x==KEYCODE_RIGHT))
				{
					if(l.GetFlag(selected))
					{
						chdir(l.GetName(selected));
						l.Init();
						listdir(".", l);
						getcwd(cwd,512);
						selected=0;
						offset=0;
					}
				}
 				else if((ev.x==KEYCODE_LEFT)||(ev.x==KEYCODE_BACKSPACE))
				{
					chdir("..");
					l.Init();
					listdir(".", l);
					getcwd(cwd,512);
					selected=0;
					offset=0;
				}
			}
		}
		if((l.nItems-offset)<20)
		{
			offset-=20-(l.nItems-offset);
			if(offset<0)offset=0;
		}
		g.rgba32(0xff505050);
		g.FillRT();
		stext(cwd,10,10,0xffffffff);
		{
			char s[128];
			snprintf(s,128,"sel=%i ofs=%i ni=%i ni-ofs=%i", selected, offset, l.nItems, l.nItems-offset);
			stext(s,10,20,0xff00ffff);
		}

		g.Rect(10,40,150,200,0xff203050);
		for(int i0=0;i0<20;i0++)
		{
			int i=i0+offset;
			if(i>=l.nItems)
				break;
			if(selected==i)
			{
				g.Rect(10,30+10*i0+10,150,10,0x8080c0ff);
			}
			if(hover==i)
			{
				g.Rect(10,30+10*i0+20,150,1,0x804040ff);
			}
			if(l.GetFlag(i))
			{
				const char* name=l.GetName(i);
				int len=strnlen(name,100);
				stext("[", 10        ,30+10+10*i0,0xff0000ff);
				stext(name,10+6      ,30+10+10*i0,0xff00c0ff);
				stext("]", 10+6+len*6,30+10+10*i0,0xff0000ff);
			}
			else
			{
				stext(l.GetName(i),10,30+10+10*i0,0xff00c0ff);
			}
		}
		Present();
	}	
	return 0;
}
