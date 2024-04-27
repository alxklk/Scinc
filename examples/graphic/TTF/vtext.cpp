#include <stdio.h>

#define G_SCREEN_WIDTH 800
#define G_SCREEN_HEIGHT 600
#define G_SCREEN_SCALE 1

#include "graphics.h"
#include "../../ws.h"

Graph g;


int main()
{
	int decterm=g.LoadTTFont("_decterm.ttf");
	int arial=g.LoadTTFont("AlloyInk-nRLyO.ttf");
	int i=320;
	while(true)
	{
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		double T=Time();
		g.clear();
		g.rgb(.85,.6,.5);
		g.alpha(1.);
		g.M(0,0);g.l(4000,0);g.l(0,4000);g.l(-4000,0);g.close();g.fin();
		g.fill1();
		g.clear();
		g.VSetFont(arial);
		g.t_0(50,150);
		g.t_x(.05,0);
		g.t_y(0,-.05);
		char s[64];
		snprintf(s,64,"%f",T);
		g.VText(s);
		g.t_0(50,200);
		g.VText("font: Alloy Ink");
		g.fin();
		g.rgb(.5,.15,0);
		g.width(2,2);
		g.stroke();
		g.rgb(.2,.9,1);
		g.fill1();

		g.clear();
		g.VSetFont(decterm);
		g.t_0(50,250);
		g.t_x(.027,0.005);
		g.t_y(0.015,-.017);
		g.VText("font: 'Decterm'");
		g.t_0(50,350);
		g.t_x(.017,0);
		g.t_y(0,-.012);
		g.VText("!@#$%^&*()_1234567890");
		g.t_0(50,380);
		g.VText("І навіть працює UTF8 ©");
		g.t_0(50,420);
		g.t_x(.013,0);
		g.t_y(0,-.012);
		g.VText("Λειτουργούν διαφορετικές γλώσσες!");
		g.fin();
		g.rgb(0,0,0);
		g.width(2,2);
		g.stroke();
		g.rgb(0.7,1,0.5);
		g.fill1();

		Present();
	}
	return 0;
}