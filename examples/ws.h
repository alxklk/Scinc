CWinSys wsys;
int mainWin=-1;
int quitReq=0;

#ifdef __SCINC_HOTRELOAD__
int mainWinPosX;
int mainWinPosY;
#endif

int InitWS()
{
	int width=640;
	int height=480;
	int scale=2;
	int mode=0;

#ifdef G_SCREEN_WIDTH
	width=G_SCREEN_WIDTH;
#endif
#ifdef G_SCREEN_HEIGHT
	height=G_SCREEN_HEIGHT;
#endif
#ifdef G_SCREEN_SCALE
	scale=G_SCREEN_SCALE;
#endif
#ifdef G_SCREEN_MODE
	mode=G_SCREEN_MODE;
#endif

#ifdef __SCINC_HOTRELOAD__
	printf("With hot reloading enabled\n");
#endif

	quitReq=0;

	SetEventCallback([](SScincEvent e)->int
	{
		if(e.type=='MLDN')
		{
			if(quitReq)printf("Quit cancelled\n");
			quitReq=0;
		}
		else if(e.type=='WMOV')
		{
			if(e.w!=mainWin)return 1;
			//printf("%i,%i %ix%i\n",e.x,e.y,e.z,e.h);
#ifdef __SCINC_HOTRELOAD__
			mainWinPosX=e.x;
			mainWinPosY=e.y;
			printf("main windows mov %i %i\n", e.x, e.y);
			SetPersistentInt("mainWinPosX", mainWinPosX);
			SetPersistentInt("mainWinPosY", mainWinPosY);
#endif
		}
		else if((e.type=='WDEL')||(e.type=='WDST'))
		{
			//fputs("Window kill event\n",stderr);

			//wsys.DeleteWindow(mainWin);
			//mainWin=-1;
			if(quitReq)
				exit(0);
			quitReq++;
		}
#ifdef __SCINC_HOTRELOAD__
		else if(e.type=='HRLD')
		{
			printf("Hot Reload event\n");
		}
#endif
		return 0;
	});

	mainWin=wsys.CreateWindow(width,height,scale,scale,mode);
#ifdef __SCINC_HOTRELOAD__
	wsys.SetWindowPos(mainWin,GetPersistentInt("mainWinPosX", 200),GetPersistentInt("mainWinPosY", 200));
#endif
	return 0;
}

int dummy=InitWS();

void Present()
{
	Poll();
#ifndef NO_GREEDY_EVENT
	SScincEvent ev;while(GetScincEvent(ev)){}
#endif
	if(quitReq)
	{
		Graph g;
		g.fillrect(10,10,6*50,10*4,0x80000000);
		stext(" ------------------------------------------------ ",10,10,0xffffffff);
		stext("|   For quit press Q or click [X] button again   |",10,20,0xffffffff);
		stext("|      Otherwise press esc or click anywhere     |",10,30,0xffffffff);
		stext(" ------------------------------------------------ ",10,40,0xffffffff);
#ifdef __SCINC_HOTRELOAD__
		g.fillrect(10,50,6*50,10*2,0x80800000);
		stext("|             NOTE: Hotreload enabled            |",10,50,0xffffffff);
		stext(" ------------------------------------------------ ",10,60,0xffffffff);
#endif
		if(KeyPressed(1000))
		{
			printf("Esc\n");
			quitReq=0;
		}
		if(KeyPressed('q')||KeyPressed('Q'))
		{
			exit(0);
		}
	}
	wsys.Present(mainWin);
}
