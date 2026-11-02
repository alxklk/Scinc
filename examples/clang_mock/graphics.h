// clangd mock of the Scinc intrinsic "graphics.h". Never read by Scinc: the
// interpreter handles this include itself. Signatures come from `Scinc -hdr`.
#ifndef GRAPHICS_H
#define GRAPHICS_H

#define KEYCODE_A 97
#define KEYCODE_B 98
#define KEYCODE_BACKSPACE 4008
#define KEYCODE_BREAK 2022
#define KEYCODE_C 99
#define KEYCODE_CAPSLK 3010
#define KEYCODE_CMD 3015
#define KEYCODE_COMMA 44
#define KEYCODE_CTX 3017
#define KEYCODE_D 100
#define KEYCODE_DEL 4011
#define KEYCODE_DIGIT_0 48
#define KEYCODE_DIGIT_1 49
#define KEYCODE_DIGIT_2 50
#define KEYCODE_DIGIT_3 51
#define KEYCODE_DIGIT_4 52
#define KEYCODE_DIGIT_5 53
#define KEYCODE_DIGIT_6 54
#define KEYCODE_DIGIT_7 55
#define KEYCODE_DIGIT_8 56
#define KEYCODE_DIGIT_9 57
#define KEYCODE_DIGIT_BACKSLASH 92
#define KEYCODE_DIGIT_EQ 61
#define KEYCODE_DIGIT_MINUS 45
#define KEYCODE_DOWN 4001
#define KEYCODE_E 101
#define KEYCODE_END 4007
#define KEYCODE_ENTER 4013
#define KEYCODE_ESC 1000
#define KEYCODE_F 102
#define KEYCODE_F1 2001
#define KEYCODE_F10 2010
#define KEYCODE_F11 2011
#define KEYCODE_F12 2012
#define KEYCODE_F13 2013
#define KEYCODE_F14 2014
#define KEYCODE_F15 2015
#define KEYCODE_F16 2016
#define KEYCODE_F17 2017
#define KEYCODE_F18 2018
#define KEYCODE_F19 2019
#define KEYCODE_F2 2002
#define KEYCODE_F3 2003
#define KEYCODE_F4 2004
#define KEYCODE_F5 2005
#define KEYCODE_F6 2006
#define KEYCODE_F7 2007
#define KEYCODE_F8 2008
#define KEYCODE_F9 2009
#define KEYCODE_G 103
#define KEYCODE_GRAVE 96
#define KEYCODE_GR_0 5000
#define KEYCODE_GR_1 5001
#define KEYCODE_GR_2 5002
#define KEYCODE_GR_3 5003
#define KEYCODE_GR_4 5004
#define KEYCODE_GR_5 5005
#define KEYCODE_GR_6 5006
#define KEYCODE_GR_7 5007
#define KEYCODE_GR_8 5008
#define KEYCODE_GR_9 5009
#define KEYCODE_GR_ADD 5103
#define KEYCODE_GR_DIV 5100
#define KEYCODE_GR_ENTER 5013
#define KEYCODE_GR_MUL 5101
#define KEYCODE_GR_NUMLOCK 5099
#define KEYCODE_GR_POINT 5010
#define KEYCODE_GR_RETURN 5013
#define KEYCODE_GR_SUB 5102
#define KEYCODE_H 104
#define KEYCODE_HOME 4006
#define KEYCODE_I 105
#define KEYCODE_INS 4012
#define KEYCODE_J 106
#define KEYCODE_K 107
#define KEYCODE_L 108
#define KEYCODE_LALT 3002
#define KEYCODE_LCMD 3015
#define KEYCODE_LCTRL 3000
#define KEYCODE_LEFT 4002
#define KEYCODE_LSHIFT 3004
#define KEYCODE_LWIN 3015
#define KEYCODE_M 109
#define KEYCODE_N 110
#define KEYCODE_O 111
#define KEYCODE_P 112
#define KEYCODE_PAUSE 2022
#define KEYCODE_PERIOD 46
#define KEYCODE_PGDN 4005
#define KEYCODE_PGUP 4004
#define KEYCODE_PRTSCR 2020
#define KEYCODE_Q 113
#define KEYCODE_R 114
#define KEYCODE_RALT 3003
#define KEYCODE_RCMD 3016
#define KEYCODE_RCTRL 3001
#define KEYCODE_RETURN 4013
#define KEYCODE_RIGHT 4003
#define KEYCODE_RSHIFT 3005
#define KEYCODE_RWIN 3016
#define KEYCODE_S 115
#define KEYCODE_SCROLL 2021
#define KEYCODE_SLASH 47
#define KEYCODE_SPACE 32
#define KEYCODE_SYSRQ 2020
#define KEYCODE_T 116
#define KEYCODE_TAB 4010
#define KEYCODE_U 117
#define KEYCODE_UNDEF -1
#define KEYCODE_UP 4000
#define KEYCODE_V 118
#define KEYCODE_W 119
#define KEYCODE_X 120
#define KEYCODE_Y 121
#define KEYCODE_Z 122

class CWinSys
{
public:
	int CreateWindow(int,int,int,int,int);
	void DeleteWindow(int);
	int GetWindowRT(int);
	int GetWindowPos(int,int*,int*);
	int SetWindowPos(int,int,int);
	void Present(int);
	void MakeBorderless(int);
};

class SScincEvent
{
public:
	int type;
	int x;
	int y;
	int z;
	int w;
	int h;
};

class Graph
{
public:
	void M(float,float);
	void m(float,float);
	void L(float,float);
	void l(float,float);
	void pl(float,float);
	void pm(float,float);
	void close();
	void clear();
	void C(float,float,float,float,float,float);
	void c(float,float,float,float,float,float);
	void Q(float,float,float,float);
	void q(float,float,float,float);
	void A(float,float,float,int,int,float,float);
	void a(float,float,float,int,int,float,float);
	void fill1();
	void fill2();
	int getpos(float,float*,float*);
	void getshapepos(float,int,float*,float*);
	float getlen();
	float getshapelen(int);
	int getnshapes();
	void calclen();
	void cut(float,int);
	void stroke();
	void miterlim(float);
	void fin();
	void rgba(float,float,float,float);
	void rgba32(int);
	void rgb(float,float,float);
	void Circle(float,float,float,float,float,int);
	void Rect(int,int,int,int,int);
	void hairline(float,float,float,float);
	void lineH(int,int,int);
	void lineH(int,int,int,int);
	void lineV(int,int,int);
	void lineV(int,int,int,int);
	void fillrect(int,int,int,int);
	void fillrect(int,int,int,int,int);
	void gray(float);
	void alpha(float);
	void width(float,float);
	void graddef(int);
	void gradend();
	void gradstop(float,float,float,float,float);
	void graduse(int);
	void gradtype(int);
	void gradmethod(int);
	void g_t(float,float,float,float,float,float);
	void g_0(float,float);
	void g_x(float,float);
	void g_y(float,float);
	void t_t(float,float,float,float,float,float);
	void t_0(float,float);
	void t_x(float,float);
	void t_y(float,float);
	void next();
	int GetActiveRT();
	void SetActiveRT(int);
	int CreateRT(int,int);
	void DeleteRT(int);
	void BitBlt(int,int,int);
	void BitBlt(void*,int,int,int,int);
	void FillRT();
	int LoadTTFont(const char*);
	void VSetFont(int);
	void VChar(int);
	void VText(const char*);
	void WriteImage(const char*,const char*,const char*);
	int MpegCreate(const char*);
	void MpegWriteFrame(int,int);
	void MpegDone(int);
	void Clip(int,int,int,int);
	void UnClip();
	void GetHostRT();
};

void WaitForScincEvent(float);
void SetPresentWait(int);
int SetEventCallback(int(*func)(SScincEvent));
void Poll();
int GetScincEvent(SScincEvent&);
void GetMouseState(int&,int&,int&);
int GetKeyEvent(int&,int&);
int CountKeyEvents();
int KeyPressed(int);
int stext(const char*,int,int,int);
int stexts(const char*,int,int,int,int);
void PutPixel(int,int,int);
int GetPixel(int,int);

#endif // GRAPHICS_H
