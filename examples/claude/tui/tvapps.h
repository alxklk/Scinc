// Four more desktop applications:
//   Sonar         a mine-charting game: numbers count the mines around a cell, buoys mark
//                 them, and three sonar pings per game reveal a 3x3 patch safely
//   Hex viewer    the bytes of any file, with offsets and the ASCII column
//   ANSI paint    a 76x16 picture of coloured characters, saved and loaded as .ans
//   Theme editor  the colours of every part of the desktop, kept as named themes
//
// Sonar uses ToyRandom from tvtoys.h, and the program routes the hooks of these windows.
// A program that defines TVAPPS_TOOLS before including this gets only the hex viewer
// and the theme editor, without tvtoys.h (../t2fc does). TE_FILE may name the themes
// file elsewhere.
#ifndef TVAPPS_H
#define TVAPPS_H

// guarded here too: Scinc reads a header again on every #include, and it can only
// read 16 files in all
#ifndef TV_H
#include "../tui/tv.h"
#endif

#define W_SONAR 43
#define W_HEX 44
#define W_PAINT 45

#ifndef TVAPPS_TOOLS
// ---------------------------------------------------------------- Sonar

#define SN_N 480
#define SN_SAVE "sonar.sav"

int snWin=-1;
int snW=9;
int snH=9;
int snMines=10;
int snSize=0;
int snMine[SN_N];
int snOpen[SN_N];      // 0 hidden, 1 open, 2 buoy
int snNum[SN_N];
int snStack[SN_N];
int snState=0;         // 0 before the first move, 1 playing, 2 sunk, 3 charted
int snStart=0;
int snEnd=0;
int snPings=3;
int snCurX=0;
int snCurY=0;
int snBoom=-1;
int snPingX=0;
int snPingY=0;
int snPingEnd=0;       // term_ms() when the ping rings stop
int snBest[3];
int snBestRead=0;

void SnBestIO(int save)
{
	char buf[64];
	if(save)
	{
		snprintf(&buf[0],64,"%i %i %i\n",snBest[0],snBest[1],snBest[2]);
		if(fs_save_begin(SN_SAVE))
		{
			int ok=fs_save_write(&buf[0],StrLen(&buf[0]));
			fs_save_end(ok);
		}
		return;
	}
	for(int k=0;k<3;k++)
		snBest[k]=0;
	int n=fs_load(SN_SAVE,&buf[0],63);
	if(n<=0)return;
	buf[n]=0;
	int k=0;
	int i=0;
	while((k<3)&&(buf[i]!=0))
	{
		if((buf[i]>='0')&&(buf[i]<='9'))
		{
			int v=0;
			while((buf[i]>='0')&&(buf[i]<='9'))
			{
				v=v*10+buf[i]-'0';
				i++;
			}
			snBest[k]=v;
			k++;
		}
		else i++;
	}
}

// size 0 small, 1 medium, 2 large
void SnNew(int size)
{
	snSize=size;
	snW=9;
	snH=9;
	snMines=10;
	if(size==1)
	{
		snW=16;
		snH=16;
		snMines=40;
	}
	if(size==2)
	{
		snW=30;
		snH=16;
		snMines=99;
	}
	for(int i=0;i<SN_N;i++)
	{
		snMine[i]=0;
		snOpen[i]=0;
		snNum[i]=0;
	}
	snState=0;
	snPings=3;
	snBoom=-1;
	snCurX=snW/2;
	snCurY=snH/2;
	snPingEnd=0;
	if(snWin>=0)
	{
		TWin& t=tvWin[snWin];
		int w=snW*2+4;
		if(w<46)w=46;
		t.w=w;
		t.h=snH+4;
		if(t.x+t.w>tvW)t.x=tvW-t.w;
		if(t.y+t.h>tvH-1)t.y=tvH-1-t.h;
		if(t.x<0)t.x=0;
		if(t.y<1)t.y=1;
	}
	tvDirty=1;
}

// Lays the mines, none in the 3x3 patch around the first move.
void SnPlace(int sx, int sy)
{
	toyRng=toyRng+term_ms();
	int left=snMines;
	while(left>0)
	{
		int x=ToyRandom()%snW;
		int y=ToyRandom()%snH;
		int dx=x-sx;
		int dy=y-sy;
		int near=(dx>=-1)&&(dx<=1)&&(dy>=-1)&&(dy<=1);
		if((!near)&&(!snMine[y*snW+x]))
		{
			snMine[y*snW+x]=1;
			left--;
		}
	}
	for(int y=0;y<snH;y++)
		for(int x=0;x<snW;x++)
		{
			int n=0;
			for(int j=-1;j<=1;j++)
				for(int i=-1;i<=1;i++)
				{
					int ax=x+i;
					int ay=y+j;
					if((ax>=0)&&(ax<snW)&&(ay>=0)&&(ay<snH))
						if(snMine[ay*snW+ax])n++;
				}
			snNum[y*snW+x]=n;
		}
	snState=1;
	snStart=term_ms();
}

// Opens a cell and, around cells with no mines next to them, all their neighbours.
// Returns 1 if it was a mine.
int SnOpenCell(int x, int y)
{
	if(snState==0)SnPlace(x,y);
	int i=y*snW+x;
	if(snOpen[i]!=0)return 0;
	if(snMine[i])
	{
		snOpen[i]=1;
		snBoom=i;
		snState=2;
		snEnd=term_ms();
		return 1;
	}
	int sp=0;
	snOpen[i]=1;
	snStack[sp]=i;
	sp++;
	while(sp>0)
	{
		sp--;
		int c=snStack[sp];
		if(snNum[c]==0)
		{
			int cx=c%snW;
			int cy=c/snW;
			for(int j=-1;j<=1;j++)
				for(int k=-1;k<=1;k++)
				{
					int ax=cx+k;
					int ay=cy+j;
					if((ax>=0)&&(ax<snW)&&(ay>=0)&&(ay<snH))
					{
						int a=ay*snW+ax;
						if(snOpen[a]==0)
						{
							snOpen[a]=1;
							snStack[sp]=a;
							sp++;
						}
					}
				}
		}
	}
	// charted when every safe cell is open
	int hidden=0;
	for(int k=0;k<snW*snH;k++)
		if((snOpen[k]!=1)&&(!snMine[k]))hidden++;
	if(hidden==0)
	{
		snState=3;
		snEnd=term_ms();
		for(int k=0;k<snW*snH;k++)
			if(snMine[k])snOpen[k]=2;
		int sec=(snEnd-snStart)/1000;
		if(!snBestRead)
		{
			SnBestIO(0);
			snBestRead=1;
		}
		if((snBest[snSize]==0)||(sec<snBest[snSize]))
		{
			snBest[snSize]=sec;
			SnBestIO(1);
		}
	}
	return 0;
}

// A move at x,y: open it, or on an open number with as many buoys around it, open
// the rest of its neighbours (a ping does a 3x3 patch safely).
void SnAct(int x, int y, int how)
{
	if((x<0)||(x>=snW)||(y<0)||(y>=snH))return;
	if(snState>=2)return;
	snCurX=x;
	snCurY=y;
	tvDirty=1;
	int i=y*snW+x;
	if(how==1)
	{
		if(snOpen[i]==0)snOpen[i]=2;
		else if(snOpen[i]==2)snOpen[i]=0;
		return;
	}
	if(how==2)
	{
		if(snPings<=0)return;
		if(snState==0)SnPlace(x,y);
		snPings--;
		snPingX=x;
		snPingY=y;
		snPingEnd=term_ms()+700;
		for(int j=-1;j<=1;j++)
			for(int k=-1;k<=1;k++)
			{
				int ax=x+k;
				int ay=y+j;
				if((ax>=0)&&(ax<snW)&&(ay>=0)&&(ay<snH))
				{
					int a=ay*snW+ax;
					if(snMine[a])snOpen[a]=2;
					else if(snOpen[a]!=1)
					{
						snOpen[a]=0;
						SnOpenCell(ax,ay);
					}
				}
			}
		return;
	}
	if(snOpen[i]==2)return;
	if((snOpen[i]==1)&&(snNum[i]>0))
	{
		int buoys=0;
		for(int j=-1;j<=1;j++)
			for(int k=-1;k<=1;k++)
			{
				int ax=x+k;
				int ay=y+j;
				if((ax>=0)&&(ax<snW)&&(ay>=0)&&(ay<snH))
					if(snOpen[ay*snW+ax]==2)buoys++;
			}
		if(buoys!=snNum[i])return;
		for(int j=-1;j<=1;j++)
			for(int k=-1;k<=1;k++)
			{
				int ax=x+k;
				int ay=y+j;
				if((ax>=0)&&(ax<snW)&&(ay>=0)&&(ay<snH))
					if(snState==1)SnOpenCell(ax,ay);
			}
		return;
	}
	SnOpenCell(x,y);
}

void SnOpen()
{
	if(snWin>=0)
	{
		TvToFront(snWin);
		return;
	}
	if(!snBestRead)
	{
		SnBestIO(0);
		snBestRead=1;
	}
	snWin=TvNewCentered(W_SONAR,"Sonar",46,13,WF_DIALOG);
	if(snWin<0)return;
	SnNew(snSize);
}

void SnDraw(int win)
{
	TWin& t=tvWin[win];
	int ox=t.x+1+(t.w-2-snW*2)/2;
	int oy=t.y+2;
	int now=term_ms();
	int buoys=0;
	for(int i=0;i<snW*snH;i++)
		if(snOpen[i]==2)buoys++;
	int sec=0;
	if(snState==1)sec=(now-snStart)/1000;
	if(snState>=2)sec=(snEnd-snStart)/1000;
	char s[80];
	snprintf(&s[0],80,"Mines %i  Pings %i  Time %i",snMines-buoys,snPings,sec);
	TvText(t.x+2,t.y+1,&s[0],A_DLG);
	if(snState==2)TvText(t.x+t.w-12,t.y+1,"Sunk!",0x74);
	else if(snState==3)TvText(t.x+t.w-12,t.y+1,"Charted!",0x71);
	else if(snBest[snSize]>0)
	{
		snprintf(&s[0],80,"Best %i",snBest[snSize]);
		TvText(t.x+t.w-12,t.y+1,&s[0],A_DLG_PASSIVE);
	}
	int ring=-1;
	if(now<snPingEnd)ring=(700-(snPingEnd-now))/120;
	for(int y=0;y<snH;y++)
		for(int x=0;x<snW;x++)
		{
			int i=y*snW+x;
			int ch=G_LIGHT;
			int ch2=G_LIGHT;
			int at=0x31;
			if(snOpen[i]==1)
			{
				ch=' ';
				ch2=' ';
				at=0x10;
				if(snMine[i])
				{
					ch=0x25CF;
					at=0x1F;
					if(i==snBoom)at=0x4F;
				}
				else if(snNum[i]>0)
				{
					ch2='0'+snNum[i];
					int n=snNum[i];
					at=0x1B;
					if(n==2)at=0x1A;
					if(n==3)at=0x1E;
					if(n==4)at=0x1D;
					if(n>=5)at=0x1C;
				}
			}
			else if(snOpen[i]==2)
			{
				ch=' ';
				ch2=G_UP;
				at=0x3C;
				if((snState==2)&&(!snMine[i]))ch2='x';
			}
			else if((snState==2)&&snMine[i])
			{
				ch=' ';
				ch2=0x25CF;
				at=0x17;
			}
			if(ring>=0)
			{
				int dx=x-snPingX;
				int dy=y-snPingY;
				if(dx<0)dx=-dx;
				if(dy<0)dy=-dy;
				int d=dx;
				if(dy>d)d=dy;
				if(d==ring)at=(at&0x0F)|0xB0;
			}
			if((x==snCurX)&&(y==snCurY)&&(snState<2))at=(at&0x0F)|0x70;
			TvPut(ox+x*2,oy+y,ch,at);
			TvPut(ox+x*2+1,oy+y,ch2,at);
		}
	TvText(t.x+2,t.y+t.h-1," Space open  B buoy  P ping  N new  1-3 size ",A_DLG_PASSIVE);
}

int SnKey(int win, int key)
{
	if(key==K_LEFT)snCurX--;
	else if(key==K_RIGHT)snCurX++;
	else if(key==K_UP)snCurY--;
	else if(key==K_DOWN)snCurY++;
	else if((key==' ')||(key==K_ENTER))SnAct(snCurX,snCurY,0);
	else if((key=='b')||(key=='B')||(key=='f')||(key=='F'))SnAct(snCurX,snCurY,1);
	else if((key=='p')||(key=='P'))SnAct(snCurX,snCurY,2);
	else if((key=='n')||(key=='N'))SnNew(snSize);
	else if((key>='1')&&(key<='3'))SnNew(key-'1');
	else return 0;
	if(snCurX<0)snCurX=0;
	if(snCurY<0)snCurY=0;
	if(snCurX>=snW)snCurX=snW-1;
	if(snCurY>=snH)snCurY=snH-1;
	tvDirty=1;
	return 1;
}

// Left opens, right drops a buoy, middle pings.
int SnMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int ox=1+(t.w-2-snW*2)/2;
	int cx=(x-ox)/2;
	int cy=y-2;
	if((x<ox)||(cx>=snW)||(cy<0)||(cy>=snH))return 0;
	if(ev==EV_RDOWN)
	{
		if(x>=0)SnAct(cx,cy,1);
		return 1;
	}
	if(ev!=EV_MDOWN)return 0;
	if(evBtn==1)SnAct(cx,cy,2);
	else SnAct(cx,cy,0);
	return 1;
}

#endif // TVAPPS_TOOLS

// ---------------------------------------------------------------- hex viewer

#define HX_MAX 16777216

int hxWin=-1;
char* hxBuf;
int hxSize=0;
int hxFull=0;          // the size of the file, which may be more than was loaded
int hxTop=0;           // the first row shown
int hxCur=0;           // the selected byte

int HxPerRow(int win)
{
	if(tvWin[win].w-3>=76)return 16;
	return 8;
}

void HxOpen(char* path)
{
	int size=fs_size(path);
	if(size<0)
	{
		TvMessage("Hex view","Cannot read this file.",0);
		return;
	}
	int n=size;
	if(n>HX_MAX)n=HX_MAX;
	char* buf=malloc(n+1);
	int got=fs_load(path,buf,n);
	if(got<0)
	{
		free(buf);
		TvMessage("Hex view","Cannot read this file.",0);
		return;
	}
	if(hxWin>=0)free(hxBuf);
	hxBuf=buf;
	hxSize=got;
	hxFull=size;
	hxTop=0;
	hxCur=0;
	char title[48];
	int base=0;
	int k=0;
	while(path[k]!=0)
	{
		if(path[k]=='/')base=k+1;
		k++;
	}
	snprintf(&title[0],48,"Hex: %s",&path[base]);
	if(hxWin<0)
	{
		int w=79;
		if(w>tvW)w=tvW;
		hxWin=TvNew(W_HEX,&title[0],(tvW-w)/2,1,w,tvH-3,WF_RESIZE);
	}
	else
	{
		StrCpy(&tvWin[hxWin].title[0],&title[0],48);
		TvToFront(hxWin);
	}
	if(hxSize<hxFull)TvMessage("Hex view","Only the first 16 MB of this file are shown.",0);
}

// Keeps the selected byte on screen.
void HxReveal(int win)
{
	TWin& t=tvWin[win];
	int per=HxPerRow(win);
	int rows=t.h-2;
	if(hxCur>=hxSize)hxCur=hxSize-1;
	if(hxCur<0)hxCur=0;
	int row=hxCur/per;
	if(row<hxTop)hxTop=row;
	if(row>=hxTop+rows)hxTop=row-rows+1;
	int last=(hxSize+per-1)/per-rows;
	if(last<0)last=0;
	if(hxTop>last)hxTop=last;
	if(hxTop<0)hxTop=0;
	tvDirty=1;
}

void HxDraw(int win)
{
	TWin& t=tvWin[win];
	int per=HxPerRow(win);
	int rows=t.h-2;
	HxReveal(win);
	char s[16];
	for(int r=0;r<rows;r++)
	{
		int off=(hxTop+r)*per;
		if(off>=hxSize)break;
		int y=t.y+1+r;
		snprintf(&s[0],16,"%08X",off);
		TvText(t.x+1,y,&s[0],0x17);
		for(int k=0;k<per;k++)
		{
			int i=off+k;
			if(i<hxSize)
			{
				int b=hxBuf[i]&255;
				int at=0x1B;
				if(b==0)at=0x18;
				else if((b>=32)&&(b<127))at=0x1F;
				if(i==hxCur)at=0x2F;
				snprintf(&s[0],16,"%02X",b);
				int hx=t.x+11+k*3;
				if(k>=8)hx++;
				TvText(hx,y,&s[0],at);
				int ch=b;
				if((b<32)||(b>=127))ch=0xB7;
				TvPut(t.x+13+per*3+k,y,ch,at);
			}
		}
	}
	int last=(hxSize+per-1)/per-rows;
	if(last<0)last=0;
	if(last>0)TvVBar(t.x+t.w-1,t.y+1,rows,hxTop,last,0x13);
	char st[80];
	int v=0;
	if(hxSize>0)v=hxBuf[hxCur]&255;
	snprintf(&st[0],80," offset %X of %X  (%i of %i)  byte %i ",hxCur,hxSize,hxCur,hxSize,v);
	TvClip(t.x,t.y,t.x+t.w-2,t.y+t.h);
	TvText(t.x+2,t.y+t.h-1,&st[0],0x1F);
}

void HxGoto(int win)
{
	int dlg=TvNewCentered(0,"Go to offset",40,9,WF_DIALOG);
	if(dlg<0)return;
	TvAdd(dlg,C_LABEL,3,2,10,1,"~O~ffset:",0);
	int in=TvAdd(dlg,C_INPUT,13,2,22,1,"0x",0);
	tvCtl[in].val=2;
	TvAdd(dlg,C_LABEL,3,3,34,1,"hex after 0x, else decimal",0);
	int b=TvAdd(dlg,C_BUTTON,6,5,12,2,"O~K~",CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(dlg,C_BUTTON,21,5,12,2,"Cancel",CM_CANCEL);
	tvWin[dlg].focus=in;
	int res=TvExec(dlg);
	char* s=&tvCtl[in].text[0];
	int v=0;
	int k=0;
	int hex=0;
	if((s[0]=='0')&&((s[1]=='x')||(s[1]=='X')))
	{
		hex=1;
		k=2;
	}
	while(s[k]!=0)
	{
		int c=LowerKey(s[k]);
		int d=-1;
		if((c>='0')&&(c<='9'))d=c-'0';
		if(hex&&(c>='a')&&(c<='f'))d=c-'a'+10;
		if((d>=0)&&(v<0x7000000))
		{
			if(hex)v=v*16+d;
			else v=v*10+d;
		}
		k++;
	}
	TvClose(dlg);
	if(res!=CM_OK)return;
	hxCur=v;
	HxReveal(win);
}

int HxKey(int win, int key)
{
	int per=HxPerRow(win);
	int page=(tvWin[win].h-2)*per;
	int ctrl=evMod&MOD_CTRL;
	if(key==K_LEFT)hxCur--;
	else if(key==K_RIGHT)hxCur++;
	else if(key==K_UP)hxCur=hxCur-per;
	else if(key==K_DOWN)hxCur=hxCur+per;
	else if(key==K_PGUP)hxCur=hxCur-page;
	else if(key==K_PGDN)hxCur=hxCur+page;
	else if((key==K_HOME)&&ctrl)hxCur=0;
	else if((key==K_END)&&ctrl)hxCur=hxSize-1;
	else if(key==K_HOME)hxCur=hxCur-hxCur%per;
	else if(key==K_END)hxCur=hxCur-hxCur%per+per-1;
	else if((key=='g')||(key=='G'))HxGoto(win);
	else return 0;
	if(hxCur<0)hxCur=0;
	HxReveal(win);
	return 1;
}

int HxMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int per=HxPerRow(win);
	int rows=t.h-2;
	int last=(hxSize+per-1)/per-rows;
	if(last<0)last=0;
	if(ev==EV_WHEEL)
	{
		hxTop=hxTop-3*evWheel;
		if(hxTop<0)hxTop=0;
		if(hxTop>last)hxTop=last;
		// keep the selection inside the rows shown
		if(hxCur<hxTop*per)hxCur=hxCur%per+hxTop*per;
		if(hxCur>=(hxTop+rows)*per)hxCur=hxCur%per+(hxTop+rows-1)*per;
		HxReveal(win);
		return 1;
	}
	if((ev==EV_MDRAG)||((ev==EV_MDOWN)&&(x==t.w-1)))
	{
		if(ev==EV_MDOWN)
		{
			int hit=TvVBarHit(1,rows,hxTop,last,y);
			if(hit!=2)
			{
				if(hit==0)hxTop--;
				if(hit==1)hxTop=hxTop-rows;
				if(hit==3)hxTop=hxTop+rows;
				if(hit==4)hxTop++;
				if(hxTop<0)hxTop=0;
				if(hxTop>last)hxTop=last;
				hxCur=hxCur%per+hxTop*per;
				tvDirty=1;
				return 1;
			}
			TvCaptureMouse(win);
		}
		hxTop=TvVBarPos(1,rows,last,y);
		hxCur=hxCur%per+hxTop*per;
		tvDirty=1;
		return 1;
	}
	if(ev!=EV_MDOWN)return 0;
	int r=y-1;
	int k=-1;
	int hx=x-11;
	if(hx>=24)hx--;
	if((hx>=0)&&(hx<per*3))k=hx/3;
	int ax=x-(13+per*3);
	if((ax>=0)&&(ax<per))k=ax;
	if((k<0)||(r<0)||(r>=rows))return 0;
	int i=(hxTop+r)*per+k;
	if(i<hxSize)hxCur=i;
	tvDirty=1;
	return 1;
}

#ifndef TVAPPS_TOOLS
// ---------------------------------------------------------------- ANSI paint

#define PT_W 76
#define PT_H 16
#define PT_N 1216
#define PT_UNDO 24
#define PT_NCHARS 48

int ptWin=-1;
int ptCh[PT_N];
int ptAt[PT_N];        // VGA colours, bg<<4|fg
int ptUndoCh[29184];   // PT_UNDO pictures
int ptUndoAt[29184];
int ptUndoN=0;
int ptFg=15;
int ptBg=0;
int ptChar=0x2588;
int ptTool=0;          // 0 brush, 1 text, 2 fill, 3 pick
int ptTX=0;
int ptTY=0;
int ptTX0=0;           // where Enter takes the text cursor back to
int ptLX=0;
int ptLY=0;
int ptTyped=0;         // the typing since the last click has its undo picture
int ptMod=0;
int ptChars[PT_NCHARS];
int ptStack[PT_N];
char ptPath[1024];

// The ANSI number of a VGA colour, which also indexes xterm's first 16 colours.
int PtAnsi(int v)
{
	return tvVga[v&7]+(v&8);
}

int PtAttr(int at)
{
	return TV_C256|(PtAnsi(at&15)<<8)|PtAnsi((at>>4)&15);
}

void PtSnap()
{
	if(ptUndoN==PT_UNDO)
	{
		for(int i=0;i<(PT_UNDO-1)*PT_N;i++)
		{
			ptUndoCh[i]=ptUndoCh[i+PT_N];
			ptUndoAt[i]=ptUndoAt[i+PT_N];
		}
		ptUndoN--;
	}
	int o=ptUndoN*PT_N;
	for(int i=0;i<PT_N;i++)
	{
		ptUndoCh[o+i]=ptCh[i];
		ptUndoAt[o+i]=ptAt[i];
	}
	ptUndoN++;
	ptMod=1;
}

void PtUndo()
{
	if(ptUndoN==0)return;
	ptUndoN--;
	int o=ptUndoN*PT_N;
	for(int i=0;i<PT_N;i++)
	{
		ptCh[i]=ptUndoCh[o+i];
		ptAt[i]=ptUndoAt[o+i];
	}
	tvDirty=1;
}

void PtTitle()
{
	int base=0;
	int k=0;
	while(ptPath[k]!=0)
	{
		if(ptPath[k]=='/')base=k+1;
		k++;
	}
	if(ptPath[0]==0)StrCpy(&tvWin[ptWin].title[0],"ANSI paint",48);
	else snprintf(&tvWin[ptWin].title[0],48,"ANSI paint: %s",&ptPath[base]);
}

void PtOpen()
{
	if(ptWin>=0)
	{
		TvToFront(ptWin);
		return;
	}
	ptWin=TvNewCentered(W_PAINT,"ANSI paint",PT_W+2,PT_H+6,WF_DIALOG);
	if(ptWin<0)return;
	for(int i=0;i<PT_N;i++)
	{
		ptCh[i]=' ';
		ptAt[i]=0x07;
	}
	char* c="\xe2\x96\x88\xe2\x96\x93\xe2\x96\x92\xe2\x96\x91\xe2\x96\x80\xe2\x96\x84\xe2\x96\x8c\xe2\x96\x90\xe2\x96\xa0\xe2\x97\x8f\xe2\x97\x8b\xe2\x97\x98\xe2\x97\x99\xe2\x99\xa5\xe2\x99\xa6\xe2\x99\xa3\xe2\x99\xa0\xe2\x98\xba\xe2\x98\xbb\xe2\x99\xaa\xe2\x99\xab\xe2\x98\xbc\xe2\x96\xba\xe2\x97\x84\xe2\x96\xb2\xe2\x96\xbc\xe2\x86\x91\xe2\x86\x93\xe2\x86\x92\xe2\x86\x90\xe2\x94\x80\xe2\x94\x82\xe2\x94\x8c\xe2\x94\x90\xe2\x94\x94\xe2\x94\x98\xe2\x94\x9c\xe2\x94\xa4\xe2\x94\xac\xe2\x94\xb4\xe2\x94\xbc\xe2\x95\x90\xe2\x95\x91\xe2\x95\x94\xe2\x95\x97\xe2\x95\x9a\xe2\x95\x9d ";
	int i=0;
	for(int k=0;k<PT_NCHARS;k++)
		ptChars[k]=TvUtf8(c,i);
	ptUndoN=0;
	ptMod=0;
	ptPath[0]=0;
	ptTool=0;
	PtTitle();
}

void PtDraw(int win)
{
	TWin& t=tvWin[win];
	tvWinDirty[win]=ptMod;   // for the window list
	int x0=t.x+1;
	int y0=t.y+1;
	for(int y=0;y<PT_H;y++)
		for(int x=0;x<PT_W;x++)
			TvPut(x0+x,y0+y,ptCh[y*PT_W+x],PtAttr(ptAt[y*PT_W+x]));
	int py=y0+PT_H;
	for(int x=0;x<PT_W;x++)
		TvPut(x0+x,py,G_HORZ,A_DLG_PASSIVE);
	// colours: 16 for the ink, 8 for the paper, then the brush
	py++;
	for(int k=0;k<16;k++)
	{
		int mark=0;
		if(k<8)mark=15;
		int at=TV_C256|(PtAnsi(mark)<<8)|PtAnsi(k);
		int a=' ';
		int b=' ';
		if(k==ptFg)
		{
			a=G_RIGHT;
			b=G_LEFT;
		}
		TvPut(x0+k*2,py,a,at);
		TvPut(x0+k*2+1,py,b,at);
	}
	for(int k=0;k<8;k++)
	{
		int at=TV_C256|(PtAnsi(15)<<8)|PtAnsi(k);
		int a=' ';
		int b=' ';
		if(k==ptBg)
		{
			a=G_RIGHT;
			b=G_LEFT;
		}
		TvPut(x0+34+k*2,py,a,at);
		TvPut(x0+34+k*2+1,py,b,at);
	}
	TvText(x0+52,py,"Brush",A_DLG);
	int bat=PtAttr((ptBg<<4)|ptFg);
	TvPut(x0+58,py,' ',bat);
	TvPut(x0+59,py,ptChar,bat);
	TvPut(x0+60,py,' ',bat);
	// characters for the brush; any typed key picks one too
	py++;
	for(int k=0;k<PT_NCHARS;k++)
	{
		int at=A_DLG;
		if(ptChars[k]==ptChar)at=0x2F;
		TvPut(x0+k,py,ptChars[k],at);
	}
	TvText(x0+PT_NCHARS+1,py,"or type one",A_DLG_PASSIVE);
	py++;
	char* names="Brush|Text|Fill|Pick";
	char nm[8];
	int x=x0;
	for(int k=0;k<4;k++)
	{
		TvItemAt(names,k,&nm[0],8);
		int at=A_DLG;
		if(k==ptTool)at=0x2F;
		TvPut(x,py,' ',at);
		x=TvText(x+1,py,&nm[0],at);
		TvPut(x,py,' ',at);
		x=x+2;
	}
	TvText(x+1,py,"Tab tool  Ctrl+Z undo  F2 save",A_DLG_PASSIVE);
	if((ptTool==1)&&(TvTop()==win))
	{
		tvCurX=x0+ptTX;
		tvCurY=y0+ptTY;
	}
}

void PtSet(int x, int y)
{
	if((x<0)||(x>=PT_W)||(y<0)||(y>=PT_H))return;
	ptCh[y*PT_W+x]=ptChar;
	ptAt[y*PT_W+x]=(ptBg<<4)|ptFg;
}

// Fills the area of cells that look like the one at x,y.
void PtFill(int x, int y)
{
	int i0=y*PT_W+x;
	int ch=ptCh[i0];
	int at=ptAt[i0];
	int nat=(ptBg<<4)|ptFg;
	if((ch==ptChar)&&(at==nat))return;
	int sp=1;
	ptStack[0]=i0;
	ptCh[i0]=ptChar;
	ptAt[i0]=nat;
	while(sp>0)
	{
		sp--;
		int c=ptStack[sp];
		int cx=c%PT_W;
		int cy=c/PT_W;
		for(int d=0;d<4;d++)
		{
			int ax=cx;
			int ay=cy;
			if(d==0)ax--;
			if(d==1)ax++;
			if(d==2)ay--;
			if(d==3)ay++;
			if((ax>=0)&&(ax<PT_W)&&(ay>=0)&&(ay<PT_H))
			{
				int a=ay*PT_W+ax;
				if((ptCh[a]==ch)&&(ptAt[a]==at))
				{
					ptCh[a]=ptChar;
					ptAt[a]=nat;
					ptStack[sp]=a;
					sp++;
				}
			}
		}
	}
}

int PtMouse(int win, int ev, int x, int y)
{
	int cx=x-1;
	int cy=y-1;
	if((ev==EV_MDRAG)&&(ptTool==0))
	{
		// a line from the last point, so that fast strokes have no gaps
		int dx=cx-ptLX;
		int dy=cy-ptLY;
		int n=dx;
		if(n<0)n=-n;
		int ay=dy;
		if(ay<0)ay=-ay;
		if(ay>n)n=ay;
		if(n==0)n=1;
		for(int k=1;k<=n;k++)
			PtSet(ptLX+(dx*k+n/2)/n,ptLY+(dy*k+n/2)/n);
		ptLX=cx;
		ptLY=cy;
		tvDirty=1;
		return 1;
	}
	if(ev!=EV_MDOWN)return 0;
	tvDirty=1;
	if(cy<PT_H)
	{
		if((cx<0)||(cx>=PT_W)||(cy<0))return 0;
		if(ptTool==3)
		{
			ptChar=ptCh[cy*PT_W+cx];
			ptFg=ptAt[cy*PT_W+cx]&15;
			ptBg=(ptAt[cy*PT_W+cx]>>4)&7;
			ptTool=0;
			return 1;
		}
		if(ptTool==1)
		{
			ptTX=cx;
			ptTY=cy;
			ptTX0=cx;
			ptTyped=0;
			return 1;
		}
		PtSnap();
		if(ptTool==2)
		{
			PtFill(cx,cy);
			return 1;
		}
		PtSet(cx,cy);
		ptLX=cx;
		ptLY=cy;
		TvCaptureMouse(win);
		return 1;
	}
	int row=cy-PT_H-1;
	if(row==0)
	{
		if((cx>=0)&&(cx<32))ptFg=cx/2;
		if((cx>=34)&&(cx<50))ptBg=(cx-34)/2;
	}
	else if(row==1)
	{
		if((cx>=0)&&(cx<PT_NCHARS))ptChar=ptChars[cx];
	}
	else if(row==2)
	{
		// Brush Text Fill Pick, each its name plus three columns
		int k=-1;
		if((cx>=0)&&(cx<7))k=0;
		if((cx>=9)&&(cx<15))k=1;
		if((cx>=17)&&(cx<23))k=2;
		if((cx>=25)&&(cx<31))k=3;
		if(k>=0)ptTool=k;
		ptTyped=0;
	}
	return 1;
}

int PtKey(int win, int key)
{
	if(key==K_TAB)
	{
		ptTool=(ptTool+1)%4;
		ptTyped=0;
		tvDirty=1;
		return 1;
	}
	if(ptTool!=1)
	{
		// a typed character becomes the brush
		if(evUtfN>0)
		{
			int i=0;
			ptChar=TvUtf8(&evUtf[0],i);
			return 1;
		}
		if((key>' ')&&(key<127))
		{
			ptChar=key;
			return 1;
		}
		return 0;
	}
	if(key==K_ESC)
	{
		ptTool=0;
		return 1;
	}
	int c=0;
	if(evUtfN>0)
	{
		int i=0;
		c=TvUtf8(&evUtf[0],i);
	}
	else if((key>=' ')&&(key<127))c=key;
	if(c!=0)
	{
		if(!ptTyped)PtSnap();
		ptTyped=1;
		if((ptTX<PT_W)&&(ptTY<PT_H))
		{
			ptCh[ptTY*PT_W+ptTX]=c;
			ptAt[ptTY*PT_W+ptTX]=(ptBg<<4)|ptFg;
			ptMod=1;
		}
		if(ptTX<PT_W-1)ptTX++;
		return 1;
	}
	if(key==K_BACKSPACE)
	{
		if(ptTX>0)
		{
			if(!ptTyped)PtSnap();
			ptTyped=1;
			ptTX--;
			ptCh[ptTY*PT_W+ptTX]=' ';
			ptAt[ptTY*PT_W+ptTX]=(ptBg<<4)|ptFg;
		}
		return 1;
	}
	if(key==K_ENTER)
	{
		ptTX=ptTX0;
		if(ptTY<PT_H-1)ptTY++;
		return 1;
	}
	if(key==K_LEFT)ptTX--;
	else if(key==K_RIGHT)ptTX++;
	else if(key==K_UP)ptTY--;
	else if(key==K_DOWN)ptTY++;
	else return 0;
	if(ptTX<0)ptTX=0;
	if(ptTY<0)ptTY=0;
	if(ptTX>=PT_W)ptTX=PT_W-1;
	if(ptTY>=PT_H)ptTY=PT_H-1;
	return 1;
}

// Writes the picture with SGR colours (9x and 10x for bright ones) and UTF-8
// characters, one line per row. Returns 1 if it was saved.
int PtSave(char* path)
{
	char* out=malloc(PT_N*24+PT_H*16+16);
	int n=0;
	char sgr[32];
	for(int y=0;y<PT_H;y++)
	{
		int cur=-1;
		for(int x=0;x<PT_W;x++)
		{
			int at=ptAt[y*PT_W+x];
			if(at!=cur)
			{
				int f=PtAnsi(at&15);
				int b=PtAnsi((at>>4)&15);
				int fc=30+f;
				if(f>7)fc=90+f-8;
				int bc=40+b;
				if(b>7)bc=100+b-8;
				snprintf(&sgr[0],32,"\x1b[0;%i;%im",fc,bc);
				int k=0;
				while(sgr[k]!=0)
				{
					out[n]=sgr[k];
					n++;
					k++;
				}
				cur=at;
			}
			int c=ptCh[y*PT_W+x];
			if(c<0x80)
			{
				out[n]=c;
				n++;
			}
			else if(c<0x800)
			{
				out[n]=0xC0|(c>>6);
				out[n+1]=0x80|(c&63);
				n=n+2;
			}
			else
			{
				out[n]=0xE0|(c>>12);
				out[n+1]=0x80|((c>>6)&63);
				out[n+2]=0x80|(c&63);
				n=n+3;
			}
		}
		char* end="\x1b[0m\r\n";
		int k=0;
		while(end[k]!=0)
		{
			out[n]=end[k];
			n++;
			k++;
		}
	}
	int ok=0;
	if(fs_save_begin(path))
	{
		ok=fs_save_write(out,n);
		ok=fs_save_end(ok);
	}
	free(out);
	if(!ok)
	{
		TvMessage("ANSI paint","The picture could not be saved.",0);
		return 0;
	}
	StrCpy(&ptPath[0],path,1024);
	ptMod=0;
	PtTitle();
	return 1;
}

// Reads a picture with SGR colours, as PtSave writes; other escapes are skipped.
void PtLoad(char* path)
{
	int size=fs_size(path);
	if(size<0)
	{
		TvMessage("ANSI paint","Cannot read this file.",0);
		return;
	}
	if(size>262144)size=262144;
	char* buf=malloc(size+1);
	int n=fs_load(path,buf,size);
	if(n<0)
	{
		free(buf);
		TvMessage("ANSI paint","Cannot read this file.",0);
		return;
	}
	buf[n]=0;
	PtOpen();
	if(ptWin<0)
	{
		free(buf);
		return;
	}
	TvToFront(ptWin);
	PtSnap();
	for(int i=0;i<PT_N;i++)
	{
		ptCh[i]=' ';
		ptAt[i]=0x07;
	}
	int fg=7;
	int bg=0;
	int x=0;
	int y=0;
	int i=0;
	// the ANSI order of the VGA colours
	int vga[8];
	for(int k=0;k<8;k++)
		vga[tvVga[k]]=k;
	while((i<n)&&(y<PT_H))
	{
		int c=buf[i]&255;
		if(c==0x1A)break;
		if(c==27)
		{
			i++;
			if(buf[i]=='[')
			{
				i++;
				int p=0;
				int got=0;
				while(i<n)
				{
					int d=buf[i];
					if((d>='0')&&(d<='9'))
					{
						p=p*10+d-'0';
						got=1;
					}
					else if((d==';')||(d=='m'))
					{
						if((p==0)||!got)
						{
							fg=7;
							bg=0;
						}
						else if(p==1)fg=fg|8;
						else if((p>=30)&&(p<=37))fg=(fg&8)|vga[p-30];
						else if((p>=90)&&(p<=97))fg=8|vga[p-90];
						else if((p>=40)&&(p<=47))bg=vga[p-40];
						else if((p>=100)&&(p<=107))bg=vga[p-100];
						else if(p==39)fg=7;
						else if(p==49)bg=0;
						p=0;
						got=0;
						if(d=='m')
						{
							i++;
							break;
						}
					}
					else
					{
						// some other sequence, which ends with this letter
						i++;
						break;
					}
					i++;
				}
			}
		}
		else if(c==13)i++;
		else if(c==10)
		{
			x=0;
			y++;
			i++;
		}
		else
		{
			int ch=TvUtf8(buf,i);
			if((x<PT_W)&&(y<PT_H))
			{
				ptCh[y*PT_W+x]=ch;
				ptAt[y*PT_W+x]=(bg<<4)|fg;
			}
			x++;
		}
	}
	free(buf);
	StrCpy(&ptPath[0],path,1024);
	ptMod=0;
	PtTitle();
	tvDirty=1;
}

int RunSaveAs(int ew);

// Asks about unsaved changes. Returns 0 to keep the window open.
int PtCanClose(int win)
{
	if(win!=ptWin)return 1;
	if(!ptMod)return 1;
	TvToFront(win);
	int r=TvChoose("Unsaved changes","Save the picture?","~S~ave|~D~iscard|~C~ancel");
	if(r==0)
	{
		if(ptPath[0]!=0)return PtSave(&ptPath[0]);
		return RunSaveAs(-1);
	}
	return r==1;
}

#endif // TVAPPS_TOOLS

// ---------------------------------------------------------------- theme editor

// Edits tv.h's custom theme: the colours of every role (menus, windows, dialogs,
// controls, the editor and its syntax colours), each a text and a background colour,
// and the 16 palette colours other windows (games, paint) draw with. The colours are
// sent as the terminal's own 16 (4 bit), xterm's 256 (8 bit) or as 24-bit colours.
// The desktop shows each change at once. At 24 bits a square and six sliders pick the
// colour in RGB or HSV; a radio button picks the channel the square leaves out, as in
// Sectorly's colour dialog. The syntax colours and line numbers can have a clear
// background, which is the editor's.
//
// Classic blue is built in and stays as it is; the other themes (Dark, Mono,
// Monokai, Nord, Dracula, One Dark, Midnight Commander to begin with, and any saved
// with Save as) are kept in tvthemes.cfg, each as [Name], the depth, the 16 palette text and
// background colours, then the role text and background colours, in hex ('-' a
// clear background).

#define W_THEME 47
#define CM_TEDEPTH 310
#define CM_TECHAN 311
#define CM_TEPART 313
#define CM_TECLEAR 314
#define CM_TETHEME 315
#define CM_TESAVE 316
#define CM_TESAVEAS 317
#define W_TENAME 48
#ifndef TE_OLDFILE
#define TE_OLDFILE "tvtheme.cfg"   // the one custom theme of before
#endif
#ifndef TE_FILE
#define TE_FILE "tvthemes.cfg"
#endif
#define TE_MAXTH 16
#define TE_VALS 149                // 33+R_N*2: depth, palette, roles
#define TE_NAMELEN 32
#define TE_SQW 16      // the square, in cells; it has two pixels a cell down
#define TE_SQH 8
#define TE_SLW 16      // the sliders
#define TE_ITEMS 74    // R_N roles, then the 16 palette colours
#define TE_PX 32       // the right-hand column
#define TE_PIXID 900   // TvPixels ids: the square, the six sliders, Was and Now

int teThN=1;                  // the themes: 0 Classic blue, the rest from tvthemes.cfg
char teThName[512];           // TE_MAXTH*TE_NAMELEN
int teThData[2384];           // TE_MAXTH*TE_VALS
int teEdit=0;                 // the theme in the editor
int teDirty=0;                // changed since it was loaded or saved
int teThemeCtl=-1;

int teWin=-1;
int teItem=0;          // a role, or R_N+i for palette colour i
int tePart=1;          // 0 its text colour, 1 its background
int teChan=3;          // r g b h s v: the channel the square leaves out
float teV[6];          // the colour: r g b h s v, 0..1
int teList=-1;
int tePick=-1;
int teHex=-1;
int teClear=-1;
int tePartR[2];
int teDrag=-1;         // the view a press went to
int teWasRgb=0;        // the Was swatch, for TePixels
int teDepthR[3];
int teChanR[6];
int teSlider[6];
int teSaveDepth=4;     // the custom theme as it was, for Cancel and "Saved"
int teSaveFg[16];
int teSaveBg[16];
int teSaveRFg[R_N];
int teSaveRBg[R_N];
char* teNames="Black|Blue|Green|Cyan|Red|Magenta|Brown|Light grey|Dark grey|Light blue|Light green|Light cyan|Light red|Light magenta|Yellow|White";
char* teRoleNames="Desktop|Menu|Menu: hot letter|Menu: selected|Menu: selected hot|Status line|Status: hot key|Status: pressed|Status: pressed hot|Shadow|Window|Window: frame|Window: inactive frame|Window: icons|Window: pressed icon|Dialog|Dialog: frame|Dialog: inactive, dim|Dialog: icons|Dialog: pressed icon|Help window|Help: frame|Help: inactive frame|Help: icons|Help: pressed icon|Label: hot letter|Button|Button: default|Button: focused|Button: hot letter|Button: shadow|Check box|Check box: focused|Check box: hot letter|Input line|Input line: arrow|Slider|Slider: track|Slider: focused|Progress bar|List|List: selected|List: focused|Scroll bar|Combo box: arrow|Editor text|Syntax: keyword|Syntax: comment|Syntax: string|Syntax: number|Syntax: directive|Syntax: punctuation|Line numbers|Line number: current|Selection|Found text|Matching bracket|Editor scroll bar";

// Monokai's roles, text and background colour each, in role order; '-' is clear.
char* teMonokai="3e3d32 1e1f1c f8f8f2 3e3d32 f92672 3e3d32 f8f8f2 75715e e6db74 75715e "
	"f8f8f2 3e3d32 f92672 3e3d32 f8f8f2 75715e e6db74 75715e 49483e 141411 "
	"f8f8f2 272822 f8f8f2 272822 75715e 272822 a6e22e 272822 e6db74 272822 "
	"f8f8f2 3e3d32 f8f8f2 3e3d32 90908a 3e3d32 a6e22e 3e3d32 e6db74 3e3d32 "
	"f8f8f2 1e1f1c 66d9ef 1e1f1c 75715e 1e1f1c a6e22e 1e1f1c e6db74 1e1f1c "
	"fd971f 3e3d32 f8f8f2 49483e 272822 a6e22e 272822 fd971f f92672 49483e 1e1f1c 3e3d32 "
	"f8f8f2 3e3d32 e6db74 3e3d32 fd971f 3e3d32 f8f8f2 272822 a6e22e 272822 "
	"a6e22e 3e3d32 75715e 3e3d32 fd971f 3e3d32 272822 a6e22e "
	"f8f8f2 272822 e6db74 49483e 272822 a6e22e 75715e 1e1f1c 272822 a6e22e "
	"f8f8f2 272822 f92672 - 75715e - e6db74 - ae81ff - a6e22e - f92672 - 90908a - f8f8f2 - "
	"f8f8f2 49483e 272822 e6db74 f8f8f2 75715e 75715e 1e1f1c";
// Nord, Dracula and One Dark the same way, from their published palettes.
char* teNord="3b4252 242933 e5e9f0 3b4252 88c0d0 3b4252 e5e9f0 4c566a ebcb8b 4c566a "
	"e5e9f0 3b4252 88c0d0 3b4252 e5e9f0 4c566a ebcb8b 4c566a 434c5e 1b1f26 "
	"e5e9f0 2e3440 e5e9f0 2e3440 616e88 2e3440 88c0d0 2e3440 ebcb8b 2e3440 "
	"e5e9f0 3b4252 e5e9f0 3b4252 8a94a8 3b4252 88c0d0 3b4252 ebcb8b 3b4252 "
	"e5e9f0 242933 8fbcbb 242933 616e88 242933 88c0d0 242933 ebcb8b 242933 "
	"d08770 3b4252 e5e9f0 434c5e 2e3440 88c0d0 2e3440 d08770 88c0d0 434c5e "
	"242933 3b4252 e5e9f0 3b4252 ebcb8b 3b4252 d08770 3b4252 e5e9f0 2e3440 "
	"88c0d0 2e3440 88c0d0 3b4252 616e88 3b4252 d08770 3b4252 2e3440 88c0d0 "
	"e5e9f0 2e3440 ebcb8b 434c5e 2e3440 88c0d0 616e88 242933 2e3440 88c0d0 "
	"d8dee9 2e3440 81a1c1 - 616e88 - a3be8c - b48ead - "
	"5e81ac - 81a1c1 - 4c566a - d8dee9 - d8dee9 434c5e "
	"2e3440 ebcb8b d8dee9 4c566a 616e88 242933";
char* teDracula="343746 21222c f8f8f2 343746 ff79c6 343746 f8f8f2 6272a4 f1fa8c 6272a4 "
	"f8f8f2 343746 ff79c6 343746 f8f8f2 6272a4 f1fa8c 6272a4 44475a 15161c "
	"f8f8f2 282a36 f8f8f2 282a36 6272a4 282a36 bd93f9 282a36 f1fa8c 282a36 "
	"f8f8f2 343746 f8f8f2 343746 a3a8c3 343746 bd93f9 343746 f1fa8c 343746 "
	"f8f8f2 21222c 8be9fd 21222c 6272a4 21222c bd93f9 21222c f1fa8c 21222c "
	"ffb86c 343746 f8f8f2 44475a 282a36 bd93f9 282a36 ffb86c ff79c6 44475a "
	"21222c 343746 f8f8f2 343746 f1fa8c 343746 ffb86c 343746 f8f8f2 282a36 "
	"bd93f9 282a36 bd93f9 343746 6272a4 343746 ffb86c 343746 282a36 bd93f9 "
	"f8f8f2 282a36 f1fa8c 44475a 282a36 bd93f9 6272a4 21222c 282a36 bd93f9 "
	"f8f8f2 282a36 ff79c6 - 6272a4 - f1fa8c - bd93f9 - "
	"50fa7b - ff79c6 - 6272a4 - f8f8f2 - f8f8f2 44475a "
	"282a36 f1fa8c f8f8f2 6272a4 6272a4 21222c";
char* teOneDark="2c313a 21252b d7dae0 2c313a 61afef 2c313a d7dae0 4b5263 e5c07b 4b5263 "
	"d7dae0 2c313a 61afef 2c313a d7dae0 4b5263 e5c07b 4b5263 3e4451 16191d "
	"d7dae0 282c34 d7dae0 282c34 5c6370 282c34 61afef 282c34 e5c07b 282c34 "
	"d7dae0 2c313a d7dae0 2c313a 7f848e 2c313a 61afef 2c313a e5c07b 2c313a "
	"d7dae0 21252b 56b6c2 21252b 5c6370 21252b 61afef 21252b e5c07b 21252b "
	"d19a66 2c313a d7dae0 3e4451 282c34 61afef 282c34 d19a66 61afef 3e4451 "
	"21252b 2c313a d7dae0 2c313a e5c07b 2c313a d19a66 2c313a d7dae0 282c34 "
	"61afef 282c34 61afef 2c313a 5c6370 2c313a d19a66 2c313a 282c34 61afef "
	"d7dae0 282c34 e5c07b 3e4451 282c34 61afef 5c6370 21252b 282c34 61afef "
	"abb2bf 282c34 c678dd - 5c6370 - 98c379 - d19a66 - "
	"e06c75 - 56b6c2 - 4b5263 - abb2bf - abb2bf 3e4451 "
	"282c34 e5c07b abb2bf 4b5263 5c6370 21252b";
// Midnight Commander's default skin the same way, at depth 4 (VGA colour numbers):
// the panels are Classic blue's already, the rest is the cyan menus and button bar,
// grey dialogs with blue hot letters, flat buttons and black on cyan inputs.
char* teMidnight="7 1 f 3 e 3 f 0 e 0 0 3 f 0 0 7 1 7 8 0 "
	"7 1 f 1 7 1 a 1 e 1 0 7 0 7 8 7 1 7 0 3 "
	"0 7 0 7 8 7 1 7 0 3 1 7 0 7 0 7 0 3 1 7 7 7 "
	"0 7 0 3 1 7 0 3 1 3 0 3 8 7 f 1 f 1 "
	"0 7 f 8 0 3 1 3 1 3 "
	"7 1 e - 6 - 2 - f - c - b - f 3 e 3 "
	"0 3 0 2 f 4 3 1";
#define TE_MCNAME "Midnight Commander"

// The custom theme's roles from its palette and the built-in attributes.
void TeRolesFromPalette()
{
	for(int r=0;r<R_N;r++)
	{
		int def=tvRoleDef[r];
		tvCustRFg[r]=tvCustFg[def&15];
		tvCustRBg[r]=tvCustBg[(def>>4)&15];
		if(TvRoleClear(r))tvCustRBg[r]=-1;
	}
}

// The custom theme of the Classic colours.
void TeDefault()
{
	tvCustDepth=4;
	for(int i=0;i<16;i++)
	{
		tvCustFg[i]=i;
		tvCustBg[i]=i;
	}
	TeRolesFromPalette();
}

int TeHexDigit(int c)
{
	if((c>='0')&&(c<='9'))return c-'0';
	if((c>='a')&&(c<='f'))return c-'a'+10;
	if((c>='A')&&(c<='F'))return c-'A'+10;
	return -1;
}

// Reads up to 'max' values from s from 'i' on: hex, '-' for -1, or a decimal depth
// first; stops at '[' or the end. Returns the count and moves i.
int TeTokens(char* s, int& i, int* v, int max, int depthFirst)
{
	int k=0;
	while((k<max)&&(s[i]!=0)&&(s[i]!='['))
	{
		if(s[i]=='-')
		{
			v[k]=-1;
			k++;
			i++;
		}
		else if(TeHexDigit(s[i])>=0)
		{
			int x=0;
			while(TeHexDigit(s[i])>=0)
			{
				if(depthFirst&&(k==0))x=x*10+s[i]-'0';
				else x=x*16+TeHexDigit(s[i]);
				i++;
			}
			v[k]=x;
			k++;
		}
		else i++;
	}
	return k;
}

// The custom theme from values read by TeTokens: k of them, all or the palette only.
int TeFromValues(int* v, int k)
{
	if(k<33)return 0;
	if((v[0]!=4)&&(v[0]!=8)&&(v[0]!=24))return 0;
	tvCustDepth=v[0];
	for(int j=0;j<16;j++)
	{
		tvCustFg[j]=v[1+j];
		tvCustBg[j]=v[17+j];
	}
	TeRolesFromPalette();
	if(k>=TE_VALS)
		for(int r=0;r<R_N;r++)
		{
			tvCustRFg[r]=v[33+r];
			tvCustRBg[r]=v[33+R_N+r];
			if(tvCustRFg[r]<0)tvCustRFg[r]=0;
			if((tvCustRBg[r]<0)&&!TvRoleClear(r))tvCustRBg[r]=0;
		}
	return 1;
}

// The one custom theme earlier versions kept in tvtheme.cfg; 0 without one.
int TeLoadOld()
{
	char* buf=malloc(2001);
	int n=fs_load(TE_OLDFILE,buf,2000);
	int ok=0;
	if(n>0)
	{
		buf[n]=0;
		int v[TE_VALS];
		int i=0;
		int k=TeTokens(buf,i,&v[0],TE_VALS,1);
		ok=TeFromValues(&v[0],k);
	}
	free(buf);
	return ok;
}

void TeAppendValue(char* buf, int x, int last)
{
	char tmp[16];
	if(x<0)StrCpy(&tmp[0],"-",16);
	else snprintf(&tmp[0],16,"%06x",x&0xFFFFFF);
	int l=StrLen(buf);
	StrCpy(&buf[l],&tmp[0],16);
	l=StrLen(buf);
	buf[l]=' ';
	if(last)buf[l]=10;
	buf[l+1]=0;
}

char* TeThNameAt(int i)
{
	return &teThName[i*TE_NAMELEN];
}

// Theme i takes the custom theme's colours, and back.
void TeThStore(int i)
{
	int* d=&teThData[i*TE_VALS];
	d[0]=tvCustDepth;
	for(int j=0;j<16;j++)
	{
		d[1+j]=tvCustFg[j];
		d[17+j]=tvCustBg[j];
	}
	for(int r=0;r<R_N;r++)
	{
		d[33+r]=tvCustRFg[r];
		d[33+R_N+r]=tvCustRBg[r];
	}
}

void TeThLoad(int i)
{
	TeFromValues(&teThData[i*TE_VALS],TE_VALS);
}

// The theme called name, or -1.
int TeThFind(char* name)
{
	for(int i=0;i<teThN;i++)
	{
		char* a=TeThNameAt(i);
		int k=0;
		while((a[k]!=0)&&(LowerKey(a[k])==LowerKey(name[k])))k++;
		if((a[k]==0)&&(name[k]==0))return i;
	}
	return -1;
}

// Keeps the custom theme as theme 'name', a new one or the one of that name; -1 when
// there is no room.
int TeThAdd(char* name)
{
	int i=TeThFind(name);
	if(i<0)
	{
		if(teThN>=TE_MAXTH)return -1;
		i=teThN;
		teThN++;
		StrCpy(TeThNameAt(i),name,TE_NAMELEN);
	}
	TeThStore(i);
	return i;
}

void TeSaveThemes()
{
	int cap=teThN*(TE_VALS*7+80)+16;
	char* buf=malloc(cap);
	buf[0]=0;
	for(int i=1;i<teThN;i++)
	{
		int l=StrLen(buf);
		snprintf(&buf[l],cap-l,"[%s]\n%i\n",TeThNameAt(i),teThData[i*TE_VALS]);
		int* d=&teThData[i*TE_VALS];
		for(int j=1;j<TE_VALS;j++)
			TeAppendValue(buf,d[j],(j==16)||(j==32)||(j==32+R_N)||(j==TE_VALS-1));
	}
	if(fs_save_begin(TE_FILE))
	{
		int ok=fs_save_write(buf,StrLen(buf));
		fs_save_end(ok);
	}
	free(buf);
}

void TeFromTheme(int n, int d);

// A theme of roles written as teMonokai is: Dark's palette for the other windows,
// its own roles.
void TeMakePresetFrom(int theme, int depth, char* roles)
{
	TeFromTheme(theme,depth);
	int v[116];
	int i=0;
	TeTokens(roles,i,&v[0],116,0);
	for(int r=0;r<R_N;r++)
	{
		tvCustRFg[r]=v[r*2];
		tvCustRBg[r]=v[r*2+1];
	}
}

void TeMakePreset(char* roles)
{
	TeMakePresetFrom(TH_DARK,24,roles);
}

// Midnight Commander: Classic blue's palette, its own roles.
void TeAddMidnight()
{
	TeMakePresetFrom(TH_CLASSIC,4,teMidnight);
	TeThAdd(TE_MCNAME);
}

// Reads tvthemes.cfg; without it the themes are Dark, Mono, the custom theme of
// before if there is one, Monokai, Nord, Dracula, One Dark and Midnight Commander.
void TeLoadThemes()
{
	teThN=1;
	StrCpy(TeThNameAt(0),"Classic blue",TE_NAMELEN);
	int cap=TE_MAXTH*(TE_VALS*7+80);
	char* buf=malloc(cap+1);
	int n=fs_load(TE_FILE,buf,cap);
	if(n>0)
	{
		buf[n]=0;
		int v[TE_VALS];
		int i=0;
		while(buf[i]!=0)
		{
			if(buf[i]!='[')
			{
				i++;
				continue;
			}
			i++;
			char name[TE_NAMELEN];
			int o=0;
			while((buf[i]!=0)&&(buf[i]!=']')&&(buf[i]!=10))
			{
				if(o<TE_NAMELEN-1)
				{
					name[o]=buf[i];
					o++;
				}
				i++;
			}
			name[o]=0;
			if(buf[i]==']')i++;
			int k=TeTokens(buf,i,&v[0],TE_VALS,1);
			if((o>0)&&(TeThFind(&name[0])<0)&&TeFromValues(&v[0],k))TeThAdd(&name[0]);
		}
	}
	free(buf);
	if(teThN>1)
	{
		// a tvthemes.cfg from before it was built in gets it too, at the end
		if(TeThFind(TE_MCNAME)<0)TeAddMidnight();
		return;
	}
	TeFromTheme(TH_DARK,24);
	TeThAdd("Dark");
	TeFromTheme(TH_MONO,24);
	TeThAdd("Mono");
	if(TeLoadOld())TeThAdd("Custom");
	TeMakePreset(teMonokai);
	TeThAdd("Monokai");
	TeMakePreset(teNord);
	TeThAdd("Nord");
	TeMakePreset(teDracula);
	TeThAdd("Dracula");
	TeMakePreset(teOneDark);
	TeThAdd("One Dark");
	TeAddMidnight();
}

// The desktop's pattern: 0 as the theme has it, or one of these.
int teDeskPick=0;
char* teDeskNames="As the theme has it|Light shade \xe2\x96\x91|Medium shade \xe2\x96\x92|Quadrants \xe2\x96\x9a|Checker board \xf0\x9f\xae\x95|Braille dots \xe2\xa2\x95";
int teDeskChars[5]={G_LIGHT,G_MEDIUM,G_QUADS,G_CHECKER,G_BRAILLE};
#define TE_DESKN 6

// Puts theme i in use.
void TeUseTheme(int i)
{
	// Midnight Commander's desktop is the checkered one of old Turbo Vision programs,
	// where the terminal draws it; the graphic versions mix the shades flat
	tvDeskCh=G_LIGHT;
#ifndef TV_GFX
	if((i>0)&&(i==TeThFind(TE_MCNAME)))tvDeskCh=G_MEDIUM;
#endif
	if((teDeskPick>0)&&(teDeskPick<TE_DESKN))tvDeskCh=teDeskChars[teDeskPick-1];
	if((i<=0)||(i>=teThN))
	{
		TvSetTheme(TH_CLASSIC);
		return;
	}
	TeThLoad(i);
	TvSetTheme(TH_CUSTOM);
}

// The theme names for a combo box: "Classic blue|Dark|...".
void TeThemeNames(char* out, int max)
{
	out[0]=0;
	for(int i=0;i<teThN;i++)
	{
		int l=StrLen(out);
		if(l+TE_NAMELEN+1>=max)break;
		if(i>0)
		{
			out[l]='|';
			l++;
		}
		StrCpy(&out[l],TeThNameAt(i),max-l);
	}
}

// The terminal's palette number of VGA colour v, and back (-1 for none).
int TeAnsi(int v)
{
	int a=tvVga[v&7];
	if(v>7)a=a+8;
	return a;
}

int TeFromAnsi(int a)
{
	for(int v=0;v<16;v++)
		if(TeAnsi(v)==a)return v;
	return -1;
}

int TeNearest16(int c)
{
	int vga[16];
	TvParsePal(&vga[0],tvVgaHex);
	int best=0;
	int bd=1000000;
	for(int v=0;v<16;v++)
	{
		int d=0;
		for(int sh=0;sh<24;sh=sh+8)
		{
			int e=((c>>sh)&255)-((vga[v]>>sh)&255);
			d=d+e*e;
		}
		if(d<bd)
		{
			bd=d;
			best=v;
		}
	}
	return best;
}

// A custom theme value at depth 'from' as the nearest at depth 'to'; -1 stays.
int TeConvert(int from, int to, int v)
{
	if(v<0)return v;
	if(from==to)return v;
	if((from==4)&&(to==8))return TeAnsi(v);
	if((from==8)&&(to==4)&&(v<16))return TeFromAnsi(v);
	int c=TvCustRgb(from,v)&0xFFFFFF;
	if(to==24)return c;
	if(to==8)return TvTo256(c);
	return TeNearest16(c);
}

// The value of the item's part; -1 a clear background.
int TeGet()
{
	if(teItem>=R_N)
	{
		if(tePart==0)return tvCustFg[teItem-R_N];
		return tvCustBg[teItem-R_N];
	}
	if(tePart==0)return tvCustRFg[teItem];
	return tvCustRBg[teItem];
}

// The value shown: a clear background shows the editor's.
int TeShown()
{
	int v=TeGet();
	if(v<0)v=tvRBgI[teItem];
	return v;
}

void TeSet(int v)
{
	if(teItem>=R_N)
	{
		if(tePart==0)tvCustFg[teItem-R_N]=v;
		else tvCustBg[teItem-R_N]=v;
	}
	else
	{
		if(tePart==0)tvCustRFg[teItem]=v;
		else tvCustRBg[teItem]=v;
	}
	if(teClear>=0)tvCtl[teClear].val=(v<0);
	teDirty=1;
	TvSetTheme(TH_CUSTOM);
}

float TeClamp(float v)
{
	if(v<0)return 0.;
	if(v>1)return 1.;
	return v;
}

// RGB -> HSV in teV; a grey keeps its hue, black its hue and saturation.
void TeFromRgb()
{
	float r=teV[0];
	float g=teV[1];
	float b=teV[2];
	float mx=r;
	if(g>mx)mx=g;
	if(b>mx)mx=b;
	float mn=r;
	if(g<mn)mn=g;
	if(b<mn)mn=b;
	teV[5]=mx;
	float d=mx-mn;
	if(mx<0.000001)return;
	teV[4]=d/mx;
	if(d<0.000001)return;
	float h=0.;
	if(mx==r)
	{
		h=(g-b)/d;
		if(g<b)h=h+6;
	}
	else if(mx==g)h=(b-r)/d+2;
	else h=(r-g)/d+4;
	teV[3]=h/6;
}

void TeHsvToRgb(float h, float s, float v, float& r, float& g, float& b)
{
	float h6=h*6;
	if(h6>=6)h6=0.;
	int i=int(h6);
	float f=h6-i;
	float p=v*(1-s);
	float q=v*(1-s*f);
	float t=v*(1-s*(1-f));
	r=v;
	g=t;
	b=p;
	if(i==1){r=q;g=v;b=p;}
	if(i==2){r=p;g=v;b=t;}
	if(i==3){r=p;g=q;b=v;}
	if(i==4){r=t;g=p;b=v;}
	if(i==5){r=v;g=p;b=q;}
}

int TePack(float r, float g, float b)
{
	int R=int(TeClamp(r)*255+0.5);
	int G=int(TeClamp(g)*255+0.5);
	int B=int(TeClamp(b)*255+0.5);
	return (R<<16)|(G<<8)|B;
}

// The colour with channel k1 set to t1 and k2 to t2 (k2<0: k1 only), both of one family.
int TeColorWith(int k1, float t1, int k2, float t2)
{
	float v[6];
	for(int i=0;i<6;i++)
		v[i]=teV[i];
	v[k1]=t1;
	if(k2>=0)v[k2]=t2;
	if(k1>=3)TeHsvToRgb(v[3],v[4],v[5],v[0],v[1],v[2]);
	return TePack(v[0],v[1],v[2]);
}

void TeHexText()
{
	if(teHex<0)return;
	if(tvWin[teWin].focus==teHex)return;
	snprintf(&tvCtl[teHex].text[0],16,"#%06x",TvCustRgb(tvCustDepth,TeShown())&0xFFFFFF);
	tvCtl[teHex].val=7;
	tvCtl[teHex].top=0;
}

// teV from the item's colour.
void TeLoadColour()
{
	int c=TvCustRgb(tvCustDepth,TeShown());
	teV[0]=((c>>16)&255)/255.;
	teV[1]=((c>>8)&255)/255.;
	teV[2]=(c&255)/255.;
	TeFromRgb();
	TeHexText();
}

// The item takes teV after channels of the RGB (hsv 0) or HSV family changed.
void TeApply(int hsv)
{
	if(hsv)TeHsvToRgb(teV[3],teV[4],teV[5],teV[0],teV[1],teV[2]);
	else TeFromRgb();
	TeSet(TePack(teV[0],teV[1],teV[2]));
	TeHexText();
}

void TeSetChan(int k, float v)
{
	teV[k]=TeClamp(v);
	TeApply(k>=3);
}

// The square's across and up channels: the other two of the left-out one's family.
void TeAxes(int& ax, int& ay)
{
	int base=0;
	if(teChan>=3)base=3;
	int k=teChan-base;
	ax=base+1;
	if(k!=0)ax=base;
	ay=base+2;
	if(k==2)ay=base+1;
}

// Where palette colour i sits in the 256 colour grid: the 16 two cells wide on the
// first row, the cube as six rows of six 6x6 blocks, the greys on the last row.
void Te256Pos(int i, int& col, int& row)
{
	if(i<16)
	{
		col=i*2;
		row=0;
	}
	else if(i<232)
	{
		int k=i-16;
		row=1+k/36;
		col=k%36;
	}
	else
	{
		col=i-232;
		row=7;
	}
}

int Te256At(int col, int row)
{
	if(col<0)return -1;
	if(row==0)
	{
		if(col<32)return col/2;
		return -1;
	}
	if((row>=1)&&(row<=6))
	{
		if(col<36)return 16+(row-1)*36+col;
		return -1;
	}
	if((row==7)&&(col<24))return 232+col;
	return -1;
}

// The list's item names.
void TeListItem(int ctl, int i, char* out, int max)
{
	if(ctl==teThemeCtl)
	{
		StrCpy(out,TeThNameAt(i),max);
		return;
	}
	if(i<R_N)
	{
		TvItemAt(teRoleNames,i,out,max);
		return;
	}
	char nm[32];
	TvItemAt(teNames,i-R_N,&nm[0],32);
	snprintf(out,max,"Palette %i: %s",i-R_N,&nm[0]);
}

// Whether the item's part can be clear.
int TeCanClear()
{
	return (tePart==1)&&(teItem<R_N)&&TvRoleClear(teItem);
}

#define TE_FDEPTH 0
#define TE_FLIST 1
#define TE_FPART 2
#define TE_FCLEAR 3
#define TE_FTHEME 4

// Builds the dialog's controls for the theme's depth and the item, the focus on one
// of the TE_F kinds.
void TeBuild(int win, int focusOn)
{
	int listTop=0;
	if(teList>=0)listTop=tvCtl[teList].top;
	for(int i=0;i<TV_MAXCTL;i++)
		if(tvCtl[i].used)
			if(tvCtl[i].win==win)tvCtl[i].used=0;
	if(comboCtl>=0)
		if(!tvCtl[comboCtl].used)comboCtl=-1;
	tvWin[win].focus=-1;
	teHex=-1;
	teClear=-1;
	for(int k=0;k<6;k++)
	{
		teChanR[k]=-1;
		teSlider[k]=-1;
	}
	int d=tvCustDepth;
	TvAdd(win,C_LABEL,3,2,9,1,"Colours",0);
	teDepthR[0]=TvAdd(win,C_RADIO,12,2,11,1,"~4~ bit",CM_TEDEPTH);
	teDepthR[1]=TvAdd(win,C_RADIO,23,2,11,1,"~8~ bit",CM_TEDEPTH);
	teDepthR[2]=TvAdd(win,C_RADIO,34,2,12,1,"~2~4 bit",CM_TEDEPTH);
	for(int k=0;k<3;k++)
		tvCtl[teDepthR[k]].group=1;
	if(d==4)tvCtl[teDepthR[0]].val=1;
	if(d==8)tvCtl[teDepthR[1]].val=1;
	if(d==24)tvCtl[teDepthR[2]].val=1;
	TvAdd(win,C_LABEL,49,2,7,1,"T~h~eme",0);
	teThemeCtl=TvAdd(win,C_COMBO,56,2,20,1,"",CM_TETHEME);
	tvCtl[teThemeCtl].virt=1;
	tvCtl[teThemeCtl].count=teThN;
	tvCtl[teThemeCtl].val=teEdit;
	TvAdd(win,C_LABEL,3,3,10,1,"~I~tem",0);
	teList=TvAdd(win,C_LIST,2,4,28,16,"",0);
	tvCtl[teList].virt=1;
	tvCtl[teList].count=TE_ITEMS;
	tvCtl[teList].val=teItem;
	tvCtl[teList].top=listTop;
	tePartR[0]=TvAdd(win,C_RADIO,TE_PX,4,10,1,"~T~ext",CM_TEPART);
	tePartR[1]=TvAdd(win,C_RADIO,TE_PX+11,4,16,1,"~B~ackground",CM_TEPART);
	for(int k=0;k<2;k++)
	{
		tvCtl[tePartR[k]].group=3;
		tvCtl[tePartR[k]].val=(k==tePart);
	}
	if(TeCanClear())
	{
		teClear=TvAdd(win,C_CHECK,TE_PX+28,4,11,1,"~C~lear",CM_TECLEAR);
		tvCtl[teClear].val=(TeGet()<0);
	}
	if(d==4)tePick=TvAdd(win,C_VIEW,TE_PX,7,40,3,"",0);
	if(d==8)tePick=TvAdd(win,C_VIEW,TE_PX,7,36,8,"",0);
	if(d==24)
	{
		tePick=TvAdd(win,C_VIEW,TE_PX,7,TE_SQW,TE_SQH,"",0);
		// no hot letters: B, H and S have other uses here
		char* lab="R|G|B|H|S|V";
		char item[8];
		for(int k=0;k<6;k++)
		{
			TvItemAt(lab,k,&item[0],8);
			teChanR[k]=TvAdd(win,C_RADIO,TE_PX+17,7+k,7,1,&item[0],CM_TECHAN);
			tvCtl[teChanR[k]].group=2;
			tvCtl[teChanR[k]].val=(k==teChan);
			teSlider[k]=TvAdd(win,C_VIEW,TE_PX+24,7+k,TE_SLW,1,"",0);
		}
		TvAdd(win,C_LABEL,TE_PX+18,16,5,1,"He~x~",0);
		teHex=TvAdd(win,C_INPUT,TE_PX+24,16,10,1,"",0);
	}
	int ok=TvAdd(win,C_BUTTON,TE_PX,19,10,2,"O~K~",CM_OK);
	tvCtl[ok].isDefault=1;
	TvAdd(win,C_BUTTON,TE_PX+11,19,10,2,"Cancel",CM_CANCEL);
	TvAdd(win,C_BUTTON,TE_PX+22,19,10,2,"~S~ave",CM_TESAVE);
	TvAdd(win,C_BUTTON,TE_PX+33,19,11,2,"Save ~a~s",CM_TESAVEAS);
	int f=teList;
	if(focusOn==TE_FDEPTH)
	{
		int k=0;
		if(d==8)k=1;
		if(d==24)k=2;
		f=teDepthR[k];
	}
	if(focusOn==TE_FPART)f=tePartR[tePart];
	if(focusOn==TE_FTHEME)f=teThemeCtl;
	if(focusOn==TE_FCLEAR)
	{
		f=tePartR[tePart];
		if(teClear>=0)f=teClear;
	}
	tvWin[win].focus=f;
	TeLoadColour();
	tvDirty=1;
}

// The focus kind for TeBuild that keeps the focus where it is.
int TeFocusKind(int win)
{
	int f=tvWin[win].focus;
	if((f==tePartR[0])||(f==tePartR[1]))return TE_FPART;
	if((f>=0)&&(f==teClear))return TE_FCLEAR;
	if((f==teDepthR[0])||(f==teDepthR[1])||(f==teDepthR[2]))return TE_FDEPTH;
	if((f>=0)&&(f==teThemeCtl))return TE_FTHEME;
	return TE_FLIST;
}

// Another item or part: the picker moves to its colour, and Clear comes or goes.
void TeChoose(int win, int item, int part)
{
	int had=TeCanClear();
	teItem=item;
	tePart=part;
	if(TeCanClear()!=had)TeBuild(win,TeFocusKind(win));
	else
	{
		if(teClear>=0)tvCtl[teClear].val=(TeGet()<0);
		TeLoadColour();
	}
}

int TeContrast(int c)
{
	if(TvLuma(c)>=128)return 0;
	return 0xFFFFFF;
}

// A cell of colour bg, with ch in black or white over it.
void TeCell(int x, int y, int ch, int bg)
{
	TvPut(x,y,ch,TV_RGB|(TvTo15(TeContrast(bg))<<15)|TvTo15(bg));
}

int TeRgbAt(int c)
{
	return TV_RGB|(TvTo15(c)<<15)|TvTo15(c);
}

void TeDraw(int win)
{
	TWin& t=tvWin[win];
	snprintf(&t.title[0],48,"Theme editor: %s",TeThNameAt(teEdit));
	if(teDirty)StrCpy(&t.title[StrLen(&t.title[0])],"*",2);
	// the list moved
	if((teList>=0)&&(tvCtl[teList].val!=teItem))TeChoose(win,tvCtl[teList].val,tePart);
	int f=t.focus;
	int d=tvCustDepth;
	char s[64];
	int px=t.x+TE_PX;
	int py=t.y+7;
	// a sample in the item's colours, and the value
	int v=TeGet();
	int sv=TeShown();
	if(teItem<R_N)TvText(px,t.y+5," Sample text 123 ",TV_ROLE|teItem);
	else
	{
		int i=teItem-R_N;
		TvText(px,t.y+5," Text ",i);
		TvText(px+6,t.y+5," on background ",i<<4|15);
	}
	if(v<0)StrCpy(&s[0],"clear: the editor's",64);
	else if(d==4)TvItemAt(teNames,v,&s[0],64);
	else if(d==8)snprintf(&s[0],64,"colour %i",v);
	else snprintf(&s[0],64,"#%06x",v);
	TvText(px+22,t.y+5,&s[0],A_DLG);

	int pmark=A_DLG;
	if(f==tePick)pmark=A_BTN_FOCUS;
	if(d==4)
	{
		// the terminal's own 16, in VGA order: the bright ones under the others
		for(int i=0;i<16;i++)
		{
			int x=px+(i&7)*5;
			int y=py+(i/8)*2;
			int a=TeAnsi(i);
			TvFill(x+1,y,3,1,' ',TV_C256|(a<<8)|a);
			if(i==sv)
			{
				TvPut(x,y,'[',pmark);
				TvPut(x+4,y,']',pmark);
			}
		}
	}
	else if(d==8)
	{
		for(int i=0;i<256;i++)
		{
			int col=0;
			int row=0;
			Te256Pos(i,col,row);
			int at=TV_C256|(i<<8)|i;
			int ch=' ';
			if(i==sv)
			{
				int fg=231;
				if(TvLuma(tvRgb256[i])>=128)fg=16;
				at=TV_C256|(fg<<8)|i;
				ch=0x25CB;
				if(f==tePick)ch=0x25D8;
			}
			TvPut(px+col,py+row,ch,at);
			if(i<16)TvPut(px+col+1,py+row,' ',TV_C256|(i<<8)|i);
		}
		snprintf(&s[0],64,"#%06x",tvRgb256[sv]&0xFFFFFF);
		TvText(px,py+9,&s[0],A_DLG);
		TvText(px+9,py+9,"0-15 are the terminal's own",A_DLG_PASSIVE);
	}
	else
	{
		int ax=0;
		int ay=0;
		TeAxes(ax,ay);
		// in a graphic window the square and the sliders are pixels (TePixels),
		// with the key telling the backend when they change
		int pix=TvHasPixels();
		int key=teChan*3+(f==tePick);
		for(int k=0;k<6;k++)
			key=key*31+int(teV[k]*65535.);
		if(pix)TvPixels(TE_PIXID,px,py,TE_SQW,TE_SQH,key);
		else
		{
			for(int j=0;j<TE_SQH;j++)
				for(int i=0;i<TE_SQW;i++)
				{
					float tx=(i+0.5)/TE_SQW;
					int top=TeColorWith(ax,tx,ay,1-(j*2+0.5)/(TE_SQH*2));
					int bot=TeColorWith(ax,tx,ay,1-(j*2+1.5)/(TE_SQH*2));
					TvPixRgb(px+i,py+j,top,bot);
				}
			int mx=int(teV[ax]*TE_SQW);
			if(mx>TE_SQW-1)mx=TE_SQW-1;
			int my=int((1-teV[ay])*TE_SQH);
			if(my>TE_SQH-1)my=TE_SQH-1;
			int ch=0x25CB;
			if(f==tePick)ch=0x25D8;
			TeCell(px+mx,py+my,ch,TeColorWith(ax,(mx+0.5)/TE_SQW,ay,1-(my+0.5)/TE_SQH));
		}
		for(int k=0;k<6;k++)
		{
			TCtl& lc=tvCtl[teSlider[k]];
			int lx=t.x+lc.x;
			int ly=t.y+lc.y;
			if(pix)TvPixels(TE_PIXID+1+k,lx,ly,TE_SLW,1,key);
			else
			{
				for(int i=0;i<TE_SLW;i++)
					TvPut(lx+i,ly,' ',TeRgbAt(TeColorWith(k,(i+0.5)/TE_SLW,-1,0.)));
				int th=int(teV[k]*(TE_SLW-1)+0.5);
				TeCell(lx+th,ly,0x2666,TeColorWith(k,(th+0.5)/TE_SLW,-1,0.));
			}
			int val=int(teV[k]*255+0.5);
			if(k==3)val=int(teV[k]*360+0.5)%360;
			if(k>3)val=int(teV[k]*100+0.5);
			snprintf(&s[0],64,"%3i",val);
			if(k==3)StrCpy(&s[3],"\xc2\xb0",4);
			if(k>3)StrCpy(&s[3],"%",4);
			int at=A_DLG;
			if(f==teSlider[k])at=A_BTN_FOCUS;
			TvText(lx+TE_SLW+1,ly,&s[0],at);
		}
		// what it was when the editor opened, and now
		int old=0;
		if(teItem>=R_N)
		{
			int i=teItem-R_N;
			if(tePart==0)old=teSaveFg[i];
			else old=teSaveBg[i];
		}
		else
		{
			if(tePart==0)old=teSaveRFg[teItem];
			else old=teSaveRBg[teItem];
		}
		TvText(px,py+9,"Was",A_DLG);
		TvText(px+9,py+9,"Now",A_DLG);
		// cells hold 15 bits of colour, pixels all 24
		int was=0;
		if(old>=0)was=TvCustRgb(teSaveDepth,old)&0xFFFFFF;
		teWasRgb=was;
		int now=TePack(teV[0],teV[1],teV[2])&0xFFFFFF;
		if(old<0)TvText(px+4,py+9,"clr",A_DLG_PASSIVE);
		else if(pix)TvPixels(TE_PIXID+7,px+4,py+9,4,1,was);
		else TvFill(px+4,py+9,4,1,' ',TeRgbAt(was));
		if(pix)TvPixels(TE_PIXID+8,px+13,py+9,4,1,now);
		else TvFill(px+13,py+9,4,1,' ',TeRgbAt(now));
	}
}

void TePickValue(int i)
{
	TeSet(i);
	TeLoadColour();
}

// Moves within the 256 grid; 0 at its top or bottom edge, for the dialog's focus.
int TeMove8(int dx, int dy)
{
	int col=0;
	int row=0;
	Te256Pos(TeShown(),col,row);
	if(dx!=0)
	{
		int step=dx;
		if(row==0)step=dx*2;
		int i=Te256At(col+step,row);
		if(i>=0)TePickValue(i);
		return 1;
	}
	int nr=row+dy;
	if((nr<0)||(nr>7))return 0;
	int nc=col;
	if((nr==0)&&(nc>31))nc=31;
	if((nr==7)&&(nc>23))nc=23;
	if((nr>=1)&&(nr<=6)&&(nc>35))nc=35;
	TePickValue(Te256At(nc,nr));
	return 1;
}

// One step of channel k: a unit of 255, 360 or 100.
float TeUnit(int k)
{
	if(k<3)return 1./255.;
	if(k==3)return 1./360.;
	return 1./100.;
}

int TeKey(int win, int key)
{
	TWin& t=tvWin[win];
	int f=t.focus;
	int d=tvCustDepth;
	if(f<0)return 0;
	if(f==teHex)
	{
		if(key!=K_ENTER)return 0;
		char* tx=&tvCtl[teHex].text[0];
		int i=0;
		if(tx[0]=='#')i=1;
		int c=0;
		int n=0;
		while(TeHexDigit(tx[i])>=0)
		{
			c=c*16+TeHexDigit(tx[i]);
			i++;
			n++;
		}
		if(n==3)c=((c>>8)&15)*0x110000+((c>>4)&15)*0x1100+(c&15)*0x11;
		if((n==3)||(n==6))
		{
			TeSet(c);
			t.focus=-1;
			TeLoadColour();
			t.focus=teHex;
		}
		return 1;
	}
	if((f==tePick)&&(d==4))
	{
		int v=TeShown();
		if(key==K_LEFT){if((v&7)>0)TePickValue(v-1);return 1;}
		if(key==K_RIGHT){if((v&7)<7)TePickValue(v+1);return 1;}
		if((key==K_UP)&&(v>=8)){TePickValue(v-8);return 1;}
		if((key==K_DOWN)&&(v<8)){TePickValue(v+8);return 1;}
		return 0;
	}
	if((f==tePick)&&(d==8))
	{
		if(key==K_LEFT)return TeMove8(-1,0);
		if(key==K_RIGHT)return TeMove8(1,0);
		if(key==K_UP)return TeMove8(0,-1);
		if(key==K_DOWN)return TeMove8(0,1);
		return 0;
	}
	if((f==tePick)&&(d==24))
	{
		int ax=0;
		int ay=0;
		TeAxes(ax,ay);
		// a pixel a press (Ctrl: a cell) in a graphic window, half a cell in a terminal
		float sx=1./TE_SQW;
		float sy=1./(TE_SQH*2);
		if(TvHasPixels()&&!(evMod&MOD_CTRL))
		{
			sx=1./(TE_SQW*TvCellPxW());
			sy=1./(TE_SQH*TvCellPxH());
		}
		else if(TvHasPixels())sy=1./TE_SQH;
		if(key==K_LEFT){TeSetChan(ax,teV[ax]-sx);return 1;}
		if(key==K_RIGHT){TeSetChan(ax,teV[ax]+sx);return 1;}
		if(key==K_UP)
		{
			if(teV[ay]>=1)return 0;
			TeSetChan(ay,teV[ay]+sy);
			return 1;
		}
		if(key==K_DOWN)
		{
			if(teV[ay]<=0)return 0;
			TeSetChan(ay,teV[ay]-sy);
			return 1;
		}
		return 0;
	}
	for(int k=0;k<6;k++)
		if((teSlider[k]>=0)&&(f==teSlider[k]))
		{
			float u=TeUnit(k);
			if(key==K_LEFT){TeSetChan(k,teV[k]-u);return 1;}
			if(key==K_RIGHT){TeSetChan(k,teV[k]+u);return 1;}
			if(key==K_PGDN){TeSetChan(k,teV[k]-u*10);return 1;}
			if(key==K_PGUP){TeSetChan(k,teV[k]+u*10);return 1;}
			if(key==K_HOME){TeSetChan(k,0.);return 1;}
			if(key==K_END){TeSetChan(k,1.);return 1;}
			return 0;
		}
	return 0;
}

// The pixels of the square (TE_PIXID) and the sliders after it, for a graphic
// backend: fullW*fullH of them for the whole, of which the w*h at ox,oy go into buf,
// as 0xAARRGGBB. The square's mark is a ring, thicker with the focus on the square;
// a slider's is a diamond, both black or white against the colour under them. 0 if
// id is not one of these, for TvAppPixels to try its own.
int TePixels(int id, int* buf, int fullW, int fullH, int ox, int oy, int w, int h)
{
	if((id<TE_PIXID)||(id>TE_PIXID+8))return 0;
	if(id>=TE_PIXID+7)
	{
		int c=0xFF000000|TePack(teV[0],teV[1],teV[2]);
		if(id==TE_PIXID+7)c=0xFF000000|teWasRgb;
		for(int i=0;i<w*h;i++)
			buf[i]=c;
		return 1;
	}
	float r=0.;
	float g=0.;
	float b=0.;
	if(id==TE_PIXID)
	{
		int ax=0;
		int ay=0;
		TeAxes(ax,ay);
		float v[6];
		for(int k=0;k<6;k++)
			v[k]=teV[k];
		float mx=teV[ax]*fullW;
		float my=(1-teV[ay])*fullH;
		float ro=0.4*fullW/TE_SQW;
		if(ro<3.)ro=3.;
		float th=1.2;
		if((teWin>=0)&&(tvWin[teWin].focus==tePick))th=2.4;
		float ri=ro-th;
		float ro2=ro*ro;
		float ri2=ri*ri;
		int ring=0xFF000000|TeContrast(TeColorWith(ax,teV[ax],ay,teV[ay]));
		for(int j=0;j<h;j++)
		{
			float py=oy+j+0.5;
			v[ay]=1-py/fullH;
			float dy=py-my;
			for(int i=0;i<w;i++)
			{
				float px=ox+i+0.5;
				float dx=px-mx;
				float d2=dx*dx+dy*dy;
				if((d2<=ro2)&&(d2>=ri2))buf[j*w+i]=ring;
				else
				{
					v[ax]=px/fullW;
					if(ax>=3)TeHsvToRgb(v[3],v[4],v[5],r,g,b);
					else
					{
						r=v[0];
						g=v[1];
						b=v[2];
					}
					buf[j*w+i]=0xFF000000|TePack(r,g,b);
				}
			}
		}
		return 1;
	}
	int k=id-TE_PIXID-1;
	float tx=teV[k]*(fullW-1)+0.5;
	float hh=fullH*0.4;
	float hw=hh*0.6;
	if(hw<2.)hw=2.;
	float cy=fullH/2.;
	int mark=0xFF000000|TeContrast(TeColorWith(k,teV[k],-1,0.));
	for(int i=0;i<w;i++)
	{
		float px=ox+i+0.5;
		int c=0xFF000000|TeColorWith(k,px/fullW,-1,0.);
		float dx=px-tx;
		if(dx<0)dx=-dx;
		for(int j=0;j<h;j++)
		{
			float dy=oy+j+0.5-cy;
			if(dy<0)dy=-dy;
			if(dx/hw+dy/hh<=1.)buf[j*w+i]=mark;
			else buf[j*w+i]=c;
		}
	}
	return 1;
}

int TeMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int d=tvCustDepth;
	if(ev==EV_WHEEL)
	{
		for(int k=0;k<6;k++)
			if(teSlider[k]>=0)
			{
				TCtl& c=tvCtl[teSlider[k]];
				if((y==c.y)&&(x>=c.x)&&(x<c.x+c.w))
				{
					t.focus=teSlider[k];
					TeSetChan(k,teV[k]+evWheel*TeUnit(k));
					return 1;
				}
			}
		return 0;
	}
	if(ev==EV_MDOWN)
	{
		// the view under the press, which then has the focus
		teDrag=-1;
		for(int i=0;i<TV_MAXCTL;i++)
			if(tvCtl[i].used&&(tvCtl[i].win==win)&&(tvCtl[i].type==C_VIEW))
			{
				TCtl& vc=tvCtl[i];
				if((x>=vc.x)&&(x<vc.x+vc.w)&&(y>=vc.y)&&(y<vc.y+vc.h))teDrag=i;
			}
		// the square and the sliders follow the mouse pixel by pixel
		tvPixDrag=TvHasPixels()&&(teDrag>=0)&&(tvCustDepth==24);
	}
	if((ev!=EV_MDOWN)&&(ev!=EV_MDRAG))return 0;
	int v=teDrag;
	if(v<0)return 0;
	TCtl& c=tvCtl[v];
	int cx=x-c.x;
	int cy=y-c.y;
	tvDirty=1;
	if((v==tePick)&&(d==4))
	{
		if((cx<0)||(cy<0)||(cx>=40)||(cy==1)||(cy>2))return 1;
		TePickValue((cy/2)*8+cx/5);
		return 1;
	}
	if((v==tePick)&&(d==8))
	{
		int i=Te256At(cx,cy);
		if(i>=0)TePickValue(i);
		return 1;
	}
	if((v==tePick)&&(d==24))
	{
		int ax=0;
		int ay=0;
		TeAxes(ax,ay);
		// to the pixel in a graphic window, else to the middle of the cell
		float fx=0.5;
		float fy=0.5;
		if(TvHasPixels()&&(evSubX>=0))
		{
			fx=(evSubX+0.5)/TvCellPxW();
			fy=(evSubY+0.5)/TvCellPxH();
		}
		teV[ax]=TeClamp((cx+fx)/TE_SQW);
		teV[ay]=TeClamp(1-(cy+fy)/TE_SQH);
		TeApply(teChan>=3);
		return 1;
	}
	for(int k=0;k<6;k++)
		if(v==teSlider[k])
		{
			if(TvHasPixels()&&(evSubX>=0))
			{
				// the pixel, as TePixels puts the thumb
				int cw=TvCellPxW();
				TeSetChan(k,(cx*cw+evSubX)/(TE_SLW*cw-1.));
			}
			else TeSetChan(k,cx/(TE_SLW-1.));
			return 1;
		}
	return 0;
}

// The custom theme from built-in theme n, at depth d.
void TeFromTheme(int n, int d)
{
	TvSetTheme(n);
	int from=tvDepth;
	for(int i=0;i<16;i++)
	{
		int a=tvFg[i]&0xFFFFFF;
		int b=tvBg[i]&0xFFFFFF;
		if(from==4)
		{
			a=i;
			b=i;
		}
		tvCustFg[i]=TeConvert(from,d,a);
		tvCustBg[i]=TeConvert(from,d,b);
	}
	for(int r=0;r<R_N;r++)
	{
		int a=tvRFg[r]&0xFFFFFF;
		int b=tvRBg[r]&0xFFFFFF;
		if(from==4)
		{
			a=tvRFgI[r];
			b=tvRBgI[r];
		}
		tvCustRFg[r]=TeConvert(from,d,a);
		tvCustRBg[r]=TeConvert(from,d,b);
		if(TvRoleClear(r))tvCustRBg[r]=-1;
	}
	tvCustDepth=d;
}

// What the colours were, for Was and the clear backgrounds' history.
void TeKeepAsSaved()
{
	teSaveDepth=tvCustDepth;
	for(int i=0;i<16;i++)
	{
		teSaveFg[i]=tvCustFg[i];
		teSaveBg[i]=tvCustBg[i];
	}
	for(int r=0;r<R_N;r++)
	{
		teSaveRFg[r]=tvCustRFg[r];
		teSaveRBg[r]=tvCustRBg[r];
	}
}

// Theme n into the editor: Classic blue as the custom theme of its colours.
void TeOpenTheme(int n)
{
	if((n<=0)||(n>=teThN))
	{
		n=0;
		TeFromTheme(TH_CLASSIC,4);
	}
	else TeThLoad(n);
	teEdit=n;
	teDirty=0;
	TeKeepAsSaved();
	TvSetTheme(TH_CUSTOM);
}

void TeButton(int win, int ctl, int cmd)
{
	if(cmd==CM_TECHAN)
	{
		for(int k=0;k<6;k++)
			if(ctl==teChanR[k])teChan=k;
		return;
	}
	if(cmd==CM_TEPART)
	{
		int part=0;
		if(ctl==tePartR[1])part=1;
		TeChoose(win,teItem,part);
		return;
	}
	if(cmd==CM_TECLEAR)
	{
		if(tvCtl[ctl].val)TeSet(-1);
		else TeSet(TeShown());
		TeLoadColour();
		return;
	}
	if(cmd==CM_TEDEPTH)
	{
		int nd=4;
		if(ctl==teDepthR[1])nd=8;
		if(ctl==teDepthR[2])nd=24;
		int od=tvCustDepth;
		if(nd==od)return;
		for(int i=0;i<16;i++)
		{
			tvCustFg[i]=TeConvert(od,nd,tvCustFg[i]);
			tvCustBg[i]=TeConvert(od,nd,tvCustBg[i]);
		}
		for(int r=0;r<R_N;r++)
		{
			tvCustRFg[r]=TeConvert(od,nd,tvCustRFg[r]);
			tvCustRBg[r]=TeConvert(od,nd,tvCustRBg[r]);
		}
		tvCustDepth=nd;
		teDirty=1;
		TvSetTheme(TH_CUSTOM);
		TeBuild(win,TE_FDEPTH);
		return;
	}
	if(cmd==CM_TETHEME)
	{
		int n=tvCtl[ctl].val;
		if(n==teEdit)return;
		if(teDirty)
		{
			char q[96];
			snprintf(&q[0],96,"Leave the changes to %s?",TeThNameAt(teEdit));
			if(TvMessage("Theme editor",&q[0],1)!=CM_YES)
			{
				tvCtl[ctl].val=teEdit;
				return;
			}
		}
		TeOpenTheme(n);
		TeBuild(win,TE_FTHEME);
	}
}

// Asks for a theme name; 0 when cancelled.
int TeAskName(char* name)
{
	int win=TvNewCentered(W_TENAME,"Save theme as",44,9,WF_DIALOG);
	if(win<0)return 0;
	TvAdd(win,C_LABEL,3,2,10,1,"~N~ame",0);
	int in=TvAdd(win,C_INPUT,10,2,30,1,name,0);
	tvCtl[in].val=StrLen(name);
	int ok=TvAdd(win,C_BUTTON,8,5,12,2,"O~K~",CM_OK);
	tvCtl[ok].isDefault=1;
	TvAdd(win,C_BUTTON,24,5,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=in;
	int res=TvExec(win);
	// trimmed, and kept to what the file can hold: no brackets or line ends
	char* t=&tvCtl[in].text[0];
	int a=0;
	while(t[a]==' ')a++;
	int o=0;
	while((t[a]!=0)&&(o<TE_NAMELEN-1))
	{
		int c=t[a];
		if((c!='[')&&(c!=']')&&(c>=32))
		{
			name[o]=c;
			o++;
		}
		a++;
	}
	while((o>0)&&(name[o-1]==' '))o--;
	name[o]=0;
	TvClose(win);
	return (res==CM_OK)&&(o>0);
}

// Save as: 1 when the theme was kept under a new or chosen name.
int TeSaveAs(int win)
{
	char name[TE_NAMELEN];
	if(teEdit==0)StrCpy(&name[0],"My theme",TE_NAMELEN);
	else snprintf(&name[0],TE_NAMELEN,"%s 2",TeThNameAt(teEdit));
	while(1)
	{
		if(!TeAskName(&name[0]))return 0;
		int i=TeThFind(&name[0]);
		if(i==0)
		{
			TvMessage("Save theme as","Classic blue is built in and stays as it is.\nPlease choose another name.",0);
			continue;
		}
		if((i>0)&&(i!=teEdit))
		{
			char q[96];
			snprintf(&q[0],96,"Replace the theme %s?",TeThNameAt(i));
			if(TvMessage("Save theme as",&q[0],1)!=CM_YES)continue;
		}
		break;
	}
	int i=TeThAdd(&name[0]);
	if(i<0)
	{
		TvMessage("Save theme as","There is no room for more themes.",0);
		return 0;
	}
	TeSaveThemes();
	teEdit=i;
	teDirty=0;
	TeKeepAsSaved();
	TeBuild(win,TE_FTHEME);
	return 1;
}

// Save: Classic blue is protected, so it goes to Save as.
int TeSave(int win)
{
	if(teEdit==0)
	{
		TvMessage("Save theme","Classic blue is built in and stays as it is.\nSave as keeps your colours under a new name.",0);
		return TeSaveAs(win);
	}
	TeThStore(teEdit);
	TeSaveThemes();
	teDirty=0;
	TeKeepAsSaved();
	return 1;
}

// Runs the editor on theme 'cur' (0 Classic blue); returns the theme OK put in use, or
// -1 when Cancel put 'cur' back. OK keeps changes: Save for a saved theme, Save as for
// Classic blue.
int TeRun(int cur)
{
	TeOpenTheme(cur);
	int win=TvNewCentered(W_THEME,"Theme editor",78,22,WF_DIALOG);
	if(win<0)return -1;
	teWin=win;
	teList=-1;
	teDrag=-1;
	TeBuild(win,TE_FLIST);
	int res=0;
	while(1)
	{
		res=TvExec(win);
		if(res==CM_TESAVE)TeSave(win);
		else if(res==CM_TESAVEAS)TeSaveAs(win);
		else if((res==CM_OK)&&teDirty)
		{
			if(teEdit>0)
			{
				TeSave(win);
				break;
			}
			int r=TvChoose("Theme editor","Classic blue is built in and stays as it is.\nSave your colours as a new theme?","~S~ave as|~L~eave them|Cancel");
			if((r==0)&&TeSaveAs(win))break;
			if(r==1)break;
		}
		else break;
	}
	TvClose(win);
	teWin=-1;
	teHex=-1;
	teClear=-1;
	teList=-1;
	teThemeCtl=-1;
	if(res==CM_OK)
	{
		TeUseTheme(teEdit);
		return teEdit;
	}
	TeUseTheme(cur);
	return -1;
}

#endif // TVAPPS_H
