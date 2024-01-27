CWinSys wsys;
int mainWin;

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
		if(e.type=='WKIL')
		{
			fputs("Window killed\n",stderr);
			exit(0);
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
	wsys.Present(mainWin);
}