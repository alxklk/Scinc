// Three more desktop applications:
//   Sonar       a mine-charting game: numbers count the mines around a cell, buoys mark
//               them, and three sonar pings per game reveal a 3x3 patch safely
//   Hex viewer  the bytes of any file, with offsets and the ASCII column
//   ANSI paint  a 76x16 picture of coloured characters, saved and loaded as .ans
//
// It uses ToyRandom from tvtoys.h, and the program routes the hooks of these windows.
#ifndef TVAPPS_H
#define TVAPPS_H

// guarded here too: Scinc reads a header again on every #include, and it can only
// read 16 files in all
#ifndef TV_H
#include "tv.h"
#endif

#define W_SONAR 43
#define W_HEX 44
#define W_PAINT 45

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
	TvText(t.x+2,t.y+1,&s[0],0x70);
	if(snState==2)TvText(t.x+t.w-12,t.y+1,"Sunk!",0x74);
	else if(snState==3)TvText(t.x+t.w-12,t.y+1,"Charted!",0x71);
	else if(snBest[snSize]>0)
	{
		snprintf(&s[0],80,"Best %i",snBest[snSize]);
		TvText(t.x+t.w-12,t.y+1,&s[0],0x78);
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
	TvText(t.x+2,t.y+t.h-1," Space open  B buoy  P ping  N new  1-3 size ",0x78);
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
	TvVBar(t.x+t.w-1,t.y+1,rows,hxTop,last,0x13);
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
	int x0=t.x+1;
	int y0=t.y+1;
	for(int y=0;y<PT_H;y++)
		for(int x=0;x<PT_W;x++)
			TvPut(x0+x,y0+y,ptCh[y*PT_W+x],PtAttr(ptAt[y*PT_W+x]));
	int py=y0+PT_H;
	for(int x=0;x<PT_W;x++)
		TvPut(x0+x,py,G_HORZ,0x78);
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
	TvText(x0+52,py,"Brush",0x70);
	int bat=PtAttr((ptBg<<4)|ptFg);
	TvPut(x0+58,py,' ',bat);
	TvPut(x0+59,py,ptChar,bat);
	TvPut(x0+60,py,' ',bat);
	// characters for the brush; any typed key picks one too
	py++;
	for(int k=0;k<PT_NCHARS;k++)
	{
		int at=0x70;
		if(ptChars[k]==ptChar)at=0x2F;
		TvPut(x0+k,py,ptChars[k],at);
	}
	TvText(x0+PT_NCHARS+1,py,"or type one",0x78);
	py++;
	char* names="Brush|Text|Fill|Pick";
	char nm[8];
	int x=x0;
	for(int k=0;k<4;k++)
	{
		TvItemAt(names,k,&nm[0],8);
		int at=0x70;
		if(k==ptTool)at=0x2F;
		TvPut(x,py,' ',at);
		x=TvText(x+1,py,&nm[0],at);
		TvPut(x,py,' ',at);
		x=x+2;
	}
	TvText(x+1,py,"Tab tool  Ctrl+Z undo  F2 save",0x78);
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

#endif // TVAPPS_H
