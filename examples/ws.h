CWinSys wsys;
int mainWin=-1;
int quitReq=0;

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

	SetEventCallback([](SScincEvent e)->int
	{
		if(e.type=='WMOV')
		{
			//printf("%i,%i %ix%i\n",e.x,e.y,e.z,e.h);
		}
		if(e.type=='WQIT')
		{
			//fputs("Window kill event\n",stderr);

			//wsys.DeleteWindow(mainWin);
			//mainWin=-1;
			if(quitReq)
				exit(0);
			quitReq++;
		}
		return 0;
	});

	mainWin=wsys.CreateWindow(width,height,scale,scale,mode);
	wsys.SetWindowPos(mainWin,200,200);
	return 0;
}

int dummy=InitWS();

void Present()
{
	Poll();
	if(quitReq)
	{
		Graph g;
		g.fillrect(10,10,6*50,10*4,0x80000000);
		stext(" ------------------------------------------------ ",10,10,0xffffffff);
		stext("|   For quit press Q or click [X] button again   |",10,20,0xffffffff);
		stext("|             Otherwise press esc                |",10,30,0xffffffff);
		stext(" ------------------------------------------------ ",10,40,0xffffffff);
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
