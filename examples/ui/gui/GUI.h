bool IsMouseEvent(int eventType)
{
	if(eventType=='MMOV')return true;
	if(eventType=='MLUP')return true;
	if(eventType=='MLDN')return true;
	if(eventType=='MMUP')return true;
	if(eventType=='MMDN')return true;
	if(eventType=='MRUP')return true;
	if(eventType=='MRDN')return true;
	return false;
}


bool IsMouseEventCase(int eventType)
{
	switch(eventType)
	{
	case 'MMOV':
	case 'MLUP':
	case 'MLDN':
	case 'MMUP':
	case 'MMDN':
	case 'MRUP':
	case 'MRDN':
		return true;
	default:
		return false;
	}
	return false;
}

#define CT_BUTTON 1
#define CT_CHECK 2
#define CT_SELECT 3
#define CT_HSLIDE 4
#define CT_VSLIDE 5
#define CT_STATIC 6
#define CT_FRAME 7

struct SButton;

typedef int(*PFButtonPressCB)(SButton* p);

struct SButton
{
	int type;
	char* text;
	int x0;
	int y0;
	int x1;
	int y1;
	int tag;
	int selVal;
	int* iDest;
	float* fDest;
	float f0;
	float f1;
	PFButtonPressCB CB;
	SButton& SetCB(PFButtonPressCB newCB)
	{
		CB=newCB;
		return *this;
	}
	SButton& SetTitle(char* title)
	{
		text=title;
		return *this;
	}
};

//void DrawRect(Graph g, int rectX, int rectY, int rectW, int rectH, int col)
//{
//	if(rectW<0){rectW=-rectW;rectX-=rectW;}
//
//	g.lineH(rectX,rectY, rectW,col);
//	g.lineV(rectX,rectY, rectH,col);
//	g.lineH(rectX+rectW+1,rectY+rectH,-rectW-1,col);
//	g.lineV(rectX+rectW  ,rectY+rectH,-rectH,col);
//}
//

int StrNLen(char* s, int n)
{
	for(int i=0;i<n;i++)
	{
		if(*s==0)
			return i;
		s++;
	}
	return n;
}

class CGUI
{
	int prevmx;
	int prevmy;
public:
	int nb;
	SButton buttons[64];
	int hoverButton;
	int downButton;
	void Init()
	{
		nb=0;
		hoverButton=-1;
		downButton=-1;
	}
	bool Event(SScincEvent& event)
	{
		if(IsMouseEvent(event.type))
		{
			int mx=event._1;
			int my=event._2;
			UpdateMousePos(mx, my);
			if(event.type=='MLDN')
			{
				if(MouseDown(mx,my)==-1)
				{

				}
			}
			if(event.type=='MLUP')
			{
				int but=MouseUp();
			}
		}
		return false;
	}
	void UpdateMousePos(int x, int y)
	{
		hoverButton=-1;
		if(downButton!=-1)
		{
			//printf("mp %i %i\n", x,y);
			SButton& b=buttons[downButton];
			if(b.type==CT_VSLIDE)
			{
				float v=(y-b.y0)/float(b.y1-b.y0-1);
				if(v<0)v=0;
				if(v>1)v=1;
				*b.fDest=b.f0+(b.f1-b.f0)*v;
				if(b.CB)
				{
					b.CB(&b);
				}
			}
			else if(b.type==CT_HSLIDE)
			{
				float v=(x-b.x0)/float(b.x1-b.x0-1);
				if(v<0)v=0;
				if(v>1)v=1;
				*b.fDest=b.f0+(b.f1-b.f0)*v;
				if(b.CB)
				{
					b.CB(&b);
				}
			}
		}
		for(int i=nb-1;i>=0;i--)
		{
			if((x>buttons[i].x0)&&(x<buttons[i].x1))
			{
				if((y>buttons[i].y0)&&(y<buttons[i].y1))
				{
					hoverButton=i;
					break;
				}
			}
		}
		int mx;int my;int mb;
		GetMouseState(mx,my,mb);
		if((downButton!=-1)&&(mb&2))
		{
			SButton& b=buttons[downButton];
			if(KeyPressed(3000)||KeyPressed(3001))
			{
				b.x0+=x-prevmx;
				b.y0+=y-prevmy;
			}
			b.x1+=x-prevmx;
			b.y1+=y-prevmy;
		}
		prevmx=x;
		prevmy=y;
	}
	int MouseDown(int x, int y)
	{
		downButton=hoverButton;
		UpdateMousePos(x,y);
		return hoverButton;
	}
	int MouseUp()
	{
		int ret=-1;
		if((hoverButton!=-1)&&(downButton==hoverButton))
		{
			SButton& b=buttons[hoverButton];
			if(b.type==CT_SELECT)
			{
				*b.iDest=b.selVal;
				if(b.CB)
				{
					ret=b.CB(&b);
				}
			}
			else if(b.type==CT_CHECK)
			{
				*b.iDest=!*b.iDest;
				if(b.CB)
				{
					ret=b.CB(&b);
				}
			}
			else if(b.type==CT_BUTTON)
			{
				if(b.CB)
				{
					ret=b.CB(&b);
				}
			}
			downButton=-1;
			ret=buttons[hoverButton].tag;
		}
		else
		{
			downButton=-1;
			ret=-1;
		}
		return ret;
	}
	void Render(Graph g)
	{
		for(int i=0;i<nb;i++)
		{
			int col=0xff202020;
			if(downButton==i)
			{
				col=0xff202720;
			}
			else if(hoverButton==i)
			{
				col=0xff405750;
			}
			SButton& b=buttons[i];
			//g.Rect(b.x0, b.y0,b.x1-b.x0,b.y1-b.y0,0x30ff00ff);
			//g.Rect(b.x0+1, b.y0+1,b.x1-b.x0-2,b.y1-b.y0-2,0x3000ff00);
			g.Clip(b.x0, b.y0,b.x1-b.x0+1,b.y1-b.y0+1);
			if(b.type==CT_BUTTON)
			{
				if(downButton==i)
				{
					g.Rect(b.x0, b.y0,b.x1-b.x0,b.y1-b.y0,0xff202720);
					g.lineH(b.x0, b.y0,b.x1-b.x0,0xff202020);
					g.lineH(b.x0, b.y1,b.x1-b.x0+1,0xff202020);
					g.lineV(b.x0, b.y0,b.y1-b.y0,0xff202020);
					g.lineV(b.x1, b.y0,b.y1-b.y0,0xff202020);
				}
				else if(hoverButton==i)
				{
					g.Rect(b.x0, b.y0,b.x1-b.x0,b.y1-b.y0,0xff405750);
					g.lineH(b.x0, b.y0,b.x1-b.x0,0xffc0c0c0);
					g.lineH(b.x0, b.y1,b.x1-b.x0+1,0xffc0c0c0);
					g.lineV(b.x0, b.y0,b.y1-b.y0,0xffc0c0c0);
					g.lineV(b.x1, b.y0,b.y1-b.y0,0xffc0c0c0);
				}
				else
				{
					g.Rect(b.x0, b.y0,b.x1-b.x0,b.y1-b.y0,0xff303740);
					g.lineH(b.x0, b.y0,b.x1-b.x0,  0xff606060);
					g.lineH(b.x0, b.y1,b.x1-b.x0+1,0xff606060);
					g.lineV(b.x0, b.y0,b.y1-b.y0,  0xff606060);
					g.lineV(b.x1, b.y0,b.y1-b.y0,  0xff606060);
				}
				int l=StrNLen(b.text,128);
				stext(b.text, b.x0+(b.x1-b.x0-l*6)/2+2, b.y0+(b.y1-b.y0-11)/2+1,0x80000000);
				stext(b.text, b.x0+(b.x1-b.x0-l*6)/2+1, b.y0+(b.y1-b.y0-11)/2,0xffffffff);
				//stext(b.text, b.x0+10, b.y1-15,0xffffffff);
			}
			else if(b.type==CT_SELECT)
			{
				int h=b.y1-b.y0;
				g.Circle((float)b.x0+9,(float)(b.y0+h/2),0,8,1,0xffc0c0c0);
				g.Circle((float)b.x0+9,(float)(b.y0+h/2),8,0,1,col);
				if(*b.iDest==b.selVal)
				{
					g.Circle((float)b.x0+9  ,(float)b.y0+h/2,0,4,1,0xff000000);
					g.Circle((float)b.x0+9-2,(float)b.y0+h/2-2,0,0,2,0xffffffff);
				}
				stext(b.text, b.x0+22, b.y1-h/2-5,0xffffffff);
			}
			else if(b.type==CT_CHECK)
			{
				int h=b.y1-b.y0;
				g.Rect (b.x0+2, b.y0+h/2-9.5+2 ,16,16,0xffc0c0c0);
				g.lineH(b.x0+1, b.y0+h/2-9.5+1 ,18,col);
				g.lineH(b.x0+1, b.y0+h/2-9.5+18,18,col);
				g.lineV(b.x0+1, b.y0+h/2-9.5+2 ,16,col);
				g.lineV(b.x0+18,b.y0+h/2-9.5+2 ,16,col);
				if(*b.iDest)
				{
					g.clear();
					g.M((float)b.x0+4,(float)b.y0-1.5+h/2);
					g.l(6.5,7);
					g.l(11,-11);
					g.l(0,1);
					g.l(-11,15);
					g.l(-6.5,-10.5);
					g.close();
					g.fin();
					g.rgba32(0xff000000);
					g.width(1,0.2);
					g.fill2();
					g.clear();
					g.M((float)b.x0+3,(float)b.y0-4+h/2);
					g.l(6.5,7);
					g.l(11,-11);
					g.l(0,1);
					g.l(-11,15);
					g.l(-6.5,-10.5);
					g.close();
					g.fin();
					g.width(1,1);
					g.rgba32(0xff206020);
					g.stroke();
					g.rgba32(0xff30c040);
					g.fill1();
				}
				stext(b.text, b.x0+25, b.y1-h/2-5,0xffffffff);
			}
			else if(b.type==CT_VSLIDE)
			{
				g.lineH(b.x0, b.y0 ,b.x1-b.x0,col);
				g.lineH(b.x0, b.y1-1,b.x1-b.x0,col);
				g.lineV(b.x0, b.y0+1,b.y1-b.y0-2,col);
				g.lineV(b.x1-1, b.y0+1,b.y1-b.y0-2,col);
				int v=(1.-(*b.fDest-b.f0)/(b.f1-b.f0))*(b.y1-b.y0-2);
				g.Rect(b.x0+1, b.y0+1, b.x1-b.x0-2,b.y1-b.y0-2,0xff404740);
				g.Rect(b.x0+1, b.y1-v-1, b.x1-b.x0-2,v,0xff80b080);
			}
			else if(b.type==CT_HSLIDE)
			{
				g.lineH(b.x0, b.y0 ,b.x1-b.x0,col);
				g.lineH(b.x0, b.y1-1,b.x1-b.x0,col);
				g.lineV(b.x0, b.y0+1,b.y1-b.y0-2,col);
				g.lineV(b.x1-1, b.y0+1,b.y1-b.y0-2,col);
				int v=((*b.fDest-b.f0)/(b.f1-b.f0))*(b.x1-b.x0-2);
				g.Rect(b.x0+1, b.y0+1, b.x1-b.x0-2,b.y1-b.y0-2,0xff404740);
				g.Rect(b.x0+1, b.y0+1, v,b.y1-b.y0-2,0xff80b080);
				char s[128];
				int l=snprintf(s,128,"%s: %f", b.text, *b.fDest);
				stext(s, b.x0+(b.x1-b.x0-l*6)/2, b.y0+(b.y1-b.y0-11)/2,0xffffffff);
			}
			else if(b.type==CT_STATIC)
			{
				int l=StrNLen(b.text,128);
				stext(b.text, b.x0+(b.x1-b.x0-l*6)/2+1, b.y0+(b.y1-b.y0-11)/2,0xffffffff);
			}
			else if(b.type==CT_FRAME)
			{
				g.Rect(b.x0+1, b.y0+1, b.x1-b.x0-2,b.y1-b.y0-2,0xff404740);
			}
			g.UnClip();
		}
	}
	SButton& AddButton(char* text, int x, int y, int w, int h, int tag)
	{
		buttons[nb].type=CT_BUTTON;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].tag=tag;
		buttons[nb].CB=0;
		buttons[nb].text=text;
		nb++;
		return buttons[nb-1];
	}
	SButton& AddCheck(char* text, int x, int y, int w, int h, int tag, int* dest)
	{
		buttons[nb].type=CT_CHECK;
		buttons[nb].text=text;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].tag=tag;
		buttons[nb].iDest=dest;
		buttons[nb].CB=0;
		nb++;
		return buttons[nb-1];
	}
	SButton& AddSelect(char* text, int x, int y, int w, int h, int tag, int value, int* dest)
	{
		buttons[nb].type=CT_SELECT;
		buttons[nb].text=text;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].tag=tag;
		buttons[nb].selVal=value;
		buttons[nb].iDest=dest;
		buttons[nb].CB=0;
		nb++;
		return buttons[nb-1];
	}
	SButton& AddSlide(char* text, int x, int y, int w, int h, int tag, float* dest, bool vert, float f0, float f1)
	{
		buttons[nb].type=vert?CT_VSLIDE:CT_HSLIDE;
		buttons[nb].text=text;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].tag=tag;
		buttons[nb].fDest=dest;
		buttons[nb].f0=f0;
		buttons[nb].f1=f1;
		buttons[nb].CB=0;
		nb++;
		return buttons[nb-1];
	}
	SButton& AddStatic(char* text, int x, int y, int w, int h)
	{
		buttons[nb].type=CT_STATIC;
		buttons[nb].text=text;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].CB=0;
		nb++;
		return buttons[nb-1];
	}
	SButton& AddFrame(char* text, int x, int y, int w, int h)
	{
		buttons[nb].type=CT_FRAME;
		buttons[nb].text=text;
		buttons[nb].x0=x;
		buttons[nb].y0=y;
		buttons[nb].x1=x+w;
		buttons[nb].y1=y+h;
		buttons[nb].CB=0;
		nb++;
		return buttons[nb-1];
	}
};
