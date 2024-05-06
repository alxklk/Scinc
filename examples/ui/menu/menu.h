//#include "graphics.h"

int Strlen(char* s)
{
	int i=0;
	while(true)
	{
		if(s[i]==0)
			return i;
		i++;
	}
	return 0;
}

struct SMenuItem;
typedef int(*TMenuItemCallback)(SMenuItem& item);

struct SMenuItem
{
	int parent;
	char* name;
	char* cmd;
	int cmdArg;
	// calculated values
	int smW; // submenu width
	int smH; // submenu heigth
	int x;
	int y;
	int w; // width
	TMenuItemCallback cb;
};

int Max(int x, int y){return x>y?x:y;}

bool checkrect(int x, int y, int w, int h, int cx, int cy)
{
	if(cx<x)
		return false;
	if(cy<y)
		return false;
	if(cx>(x+w))
		return false;
	if(cy>(y+h))
		return false;
	return true;
}

typedef int(*TMenuCommand)(char* command, int arg);

int MenuCommand(char* command, int arg);

struct Pnt
{
	int x;
	int y;
};

struct SMenu
{
	int misize;
	int curPar;
	SMenuItem mi[64];
	SMenu& Create()
	{
		misize=0;
		curPar=-1;
		return *this;
	}
	SMenu& GetIndex(int& index)
	{
		index=misize-1;
		return *this;
	}
	SMenu& P(char *name)
	{
		mi[misize].name=name;
		mi[misize].parent=-1;
		mi[misize].cb=0;
		curPar=misize;
		misize++;
		return *this;
	}
	SMenu& Arg(int arg)
	{
		if(misize>0)
			mi[misize-1].cmdArg=arg;
		return *this;
	}
	SMenu& M(char *name, char* cmd)
	{
		if(curPar==-1)P("???");
		mi[misize].name=name;
		mi[misize].parent=curPar;
		mi[misize].cmd=cmd;
		mi[misize].cb=0;
		misize++;
		return *this;
	}
	SMenu& C(TMenuItemCallback icb)
	{
		mi[misize-1].cb=icb;
		return *this;
	}

	TMenuCommand cmdHandler;
	int mh;
	bool menuOpen;
	int menuSelected;
	int menuHover;
	int bg;
	void Init()
	{
		mh=14;
		menuOpen=false;
		menuSelected=-1;
		menuHover=-1;
		bg=0xfff0f8ff;
	}

	int MenuUpdate()
	{
		for(int i=0;i<misize;i++)
		{
			mi[i].smW=0;
			mi[i].smH=0;
		}

		int x=0;
		for(int i=0;i<misize;i++)
		{
			mi[i].w=Strlen(mi[i].name)*6;

			if(mi[i].parent==-1)
			{
				mi[i].x=x;
				mi[i].y=0;
				x+=mi[i].w+10;
			}
			else
			{
				mi[mi[i].parent].smW=Max(mi[mi[i].parent].smW,mi[i].w+10);
				mi[mi[i].parent].smH++;
			}
		}
		return 0;
	}

	void MenuDraw(Graph& g, int x0, int y0, int w, int h)
	{

		MenuUpdate();

		g.lineH(x0,y0+mh+0,w,0x60000000);
		g.lineH(x0,y0+mh+1,w,0x40000000);
		g.lineH(x0,y0+mh+2,w,0x20000000);

		if(menuOpen)
		{
			int ms=menuSelected;
			if(ms!=-1)
			{
				int smw=mi[ms].smW;
				int mx0=x0+mi[ms].x;
				int mx1=mx0+smw;

				//OuterShadow0(r,mx0,y0+mh,mi[ms].smW-1, mi[ms].smH*mh-1,5,0.5);
				g.fillrect(mx0,y0+mh,mi[ms].smW, mi[ms].smH*mh,bg);
				//g.drawrect(mx0,y0+mh,mi[ms].smW, mi[ms].smH*mh,0xff808080);
				int mp=0;
				for(int j=0;j<misize;j++)
				{
					if(mi[j].parent==ms)
					{
						int ex=mx0+5;
						int ey=y0+mh+mp*mh;
						int tc=0xff000000;
						if(menuHover==j)
						{
							g.fillrect(mx0, ey, smw, mh,0xff2080ff);
							tc=0xffffffff;
						}

						stext(mi[j].name,ex, ey+1,0xff000000);
						mp++;
					}
				}
			}
		}

		g.fillrect(x0,y0,w,mh,bg);
		//r.drawrectO(x0,y0,w,mh,0xff808080);

		for(int i=0;i<misize;i++)
		{
			if(mi[i].parent==-1)
			{
				int mx0=x0+mi[i].x;
				int mx1=mx0+mi[i].w+10;

				int tc=0xff000000;
				if(
					(menuHover==i)||
					(menuSelected==i)
				)
				{
					g.fillrect(mx0, y0, mx1-mx0, mh,0xff2080ff);
					tc=0xffffffff;
				}
				//char sss[64];snprintf(sss,64,"%i",y0);stext(sss,100,100,0xffffffff);
				stext(mi[i].name, mx0+5, y0+1,0xff000000);
			}
		}
	}
	


	int MenuHandleEvent(int x0, int y0, int w, int h, SScincEvent& ev)
	{
		MenuUpdate();

		if(((ev.type&0xffff0000)>>16)=='KB')
		{
			if((ev.type&0xffff)=='DN')
			{
				if((ev.x==3002)||(ev.x==3003))
				{
					menuOpen=!menuOpen;
					if(menuOpen)
					{
						menuSelected=0;
						menuHover=-1;
					}
					else
					{
						menuSelected=-1;
						menuHover=-1;
					}
					return 1;
				}
				if(!menuOpen)
				{
					return 0;
				}
				if(ev.x==1000)
				{
					menuOpen=false;
					menuSelected=-1;
					menuHover=-1;
				}
				if(ev.x==4003)
				{
					if(menuOpen)
					{
						int oldParent=mi[menuSelected].parent;
						while(1)
						{
							menuSelected++;
							if(menuSelected>misize-1)
								menuSelected=0;
							if(mi[menuSelected].parent==oldParent)
								break;
						}
						menuHover=-1;
					}
				}
				if(ev.x==4002)
				{
					if(menuOpen)
					{
						if(menuSelected==-1)
							menuSelected=0;
						int oldParent=mi[menuSelected].parent;
						while(1)
						{
							menuSelected--;
							if(menuSelected<0)
								menuSelected=misize-1;
							if(mi[menuSelected].parent==oldParent)
								break;
						}
						menuHover=-1;
					}
				}
				if(ev.x==4000)
				{
					if(menuOpen)
					{
						int oldParent;
						if(menuHover==-1)
						{
							oldParent=menuSelected;
							menuHover=0;
						}
						else
						{
							oldParent=mi[menuHover].parent;
							menuHover--;
						}
						while(1)
						{
							if(menuHover<0)
								menuHover=misize-1;
							if(mi[menuHover].parent==oldParent)
								break;
							menuHover--;
						}
					}
				}
				if(ev.x==4001)
				{
					if(menuOpen)
					{
						int oldParent;
						if(menuHover==-1)
						{
							oldParent=menuSelected;
							menuHover=0;
						}
						else
						{
							oldParent=mi[menuHover].parent;
							menuHover++;
						}
						int cnt=0;
						while(1)
						{
							if(menuHover>misize-1)
								menuHover=0;
							if(mi[menuHover].parent==oldParent)
								break;
							menuHover++;
							cnt++;
							if(cnt>=misize)
								break;
						}
					}
				}
				if((ev.x==4013)||(ev.x==5013))
				{
					if(menuOpen)
					{
						if(menuHover!=-1)
						{
							if(cmdHandler)cmdHandler(mi[menuHover].cmd,mi[menuHover].cmdArg);
							if(mi[menuHover].cb)
							{
								printf(" Pointer %i\n", mi[menuHover].cb);
								mi[menuHover].cb(mi[menuHover]);
							}
							//MenuCommand(mi[menuHover].cmd,mi[menuHover].cmdArg);
							//printf(" Hit menu %i\n", menuHover);
						}
						menuOpen=false;
						menuSelected=-1;
						menuHover=-1;
					}
				}
			}
		}

		//printf("Event %c%c%c%c\n",(ev.type&0xff000000)>>24,(ev.type&0xff0000)>>16,(ev.type&0xff00)>>8,(ev.type&0xff));
		//printf("(ev.type>>24)&0xff=%c\n",(ev.type>>24)&0xff);
		if(((ev.type>>24)&0xff)!='M')
		{
			return 0;
		}
		Pnt me;
		me.x=ev.x;
		me.y=ev.y;

		char* cmd="";
		int cmdArg=0;

		if(!menuOpen)
		{
			menuSelected=-1;
		}

		bool clickOnMenu=false;

		menuHover=-1;
		for(int i=0;i<misize;i++)
		{
			if(mi[i].parent==-1)
			{
				int mx0=x0+mi[i].x;
				int mx1=mx0+mi[i].w+10;

				if(checkrect(mx0,y0,mx1-mx0,mh,me.x,me.y))
				{
					menuHover=i;
					if(menuOpen)
						menuSelected=i;
					if((ev.type&0xffffff)=='LDN')
					{
						menuOpen=!menuOpen;
						if(menuOpen)
							menuSelected=i;
						else
							menuSelected=-1;
						clickOnMenu=true;
					}
				}
			}
		}

		if(menuOpen)
		{
			int ms=menuSelected;
			if(ms!=-1)
			{
				int smw=+mi[ms].smW;
				int mx0=x0+mi[ms].x;
				int mx1=mx0+smw;

				int j=0;
				int mp=0;
				for(int j=0;j<misize;j++)
				{
					if(mi[j].parent==ms)
					{
						int ex=mx0+5;
						int ey=y0+mh+mp*mh;
						if((me.x>mx0)&&(me.x<mx1)&&(me.y>=ey)&&(me.y<(ey+mh)))
						{
							menuHover=j;
							if(((ev.type&0xffffff)=='LDN')||((ev.type&0xffffff)=='LUP'))
							{
								cmd=mi[j].cmd;
								cmdArg=mi[j].cmdArg;
								clickOnMenu=true;
								menuOpen=false;
							}
						}
						mp++;
					}
				}
			}
		}

		if(((ev.type&0xffff)=='DN')&&(!clickOnMenu))
		{
			menuOpen=false;
		}

		if((menuHover!=-1)&&(clickOnMenu))//if(cmd)
		{
			if(cmdHandler)cmdHandler(cmd,cmdArg);
			if(mi[menuHover].cb)mi[menuHover].cb(mi[menuHover]);			
			//MenuCommand(cmd,cmdArg);
			return 1;
		}

		if(clickOnMenu)
			printf(" Click menu %i\n", menuHover);

		if(clickOnMenu||(((ev.type&0xffff)=='DN')&&(me.y<mh)))
			return 1;

		if(menuHover!=-1)
			return 1;
		return 0;
	}


};