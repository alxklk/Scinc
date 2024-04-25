#include "flt2.h"

float GetT(float t, float alpha, flt2& p0, flt2& p1)
{
	flt2 d=p1-p0;
	float a=d.length();
	if(a==0)return 1+t;
	return pow(a,alpha)+t; // alpha always .5
}

float alpha=0.;

void CR2Bez(
	flt2 p0, flt2 p1, flt2 p2, flt2 p3,
	flt2& b0, flt2& b1, flt2& b2, flt2& b3
	)
{
	float t0=0.0;
	float t1=GetT(t0, alpha, p0, p1);
	float t2=GetT(t1, alpha, p1, p2);
	float t3=GetT(t2, alpha, p2, p3);
	flt2 C0;
	float rdt10=1./(t1-t0);
	float rdt21=1./(t2-t1);
	float rdt32=1./(t3-t2);
	float rdt20=1./(t2-t0);
	float rdt31=1./(t3-t1);

	flt2 A10=((p1-p0)*t1+(p0*t1-p1*t0))*rdt10; flt2 A11=(p1-p0)*rdt10*(t2-t1);
	flt2 A20=((p2-p1)*t1+(p1*t2-p2*t1))*rdt21; flt2 A21=(p2-p1)*rdt21*(t2-t1);
	flt2 A30=((p3-p2)*t1+(p2*t3-p3*t2))*rdt32; flt2 A31=(p3-p2)*rdt32*(t2-t1);

	//flt2 c3= ((A31-A21)*rdt21*rdt31+(A11-A21)*rdt20*rdt21)*(t2*t2+t1*t1)
	//		+((A21*2.-A31*2.)*rdt21*rdt31
	//		+(A21*2.-A11*2.)*rdt20*rdt21)*t1*t2;
	flt2 c2=(A21*rdt21*rdt31*t2-A21*rdt21*rdt31*t1)*t3+((A30-A20)*rdt21*rdt31
			+(A21-A20-A11*2.+A10)*rdt20*rdt21)*t2*t2+(((-A30*2.-A21+A20*2.)*rdt21*rdt31
			+(-A21*3.+A20*2.+A11*4.-A10*2.)*rdt20*rdt21)*t1+A21*rdt20*rdt21*t0)*t2
			+((A30+A21-A20)*rdt21*rdt31+(A21*2.-A20-A11*2.+A10)*rdt20*rdt21)*t1*t1-A21*rdt20*rdt21*t0*t1;
	flt2 c1=(A20*rdt21*rdt31*t2-A20*rdt21*rdt31*t1)*t3
			+(A20+A11-A10*2.)*rdt20*rdt21*t2*t2
			+(((A21-A20*3.-A11*2.+A10*4.)*rdt20*rdt21-A20*rdt21*rdt31)*t1+(A20-A21)*rdt20*rdt21*t0)*t2
			+(A20*rdt21*rdt31+(-A21+A20*2.+A11-A10*2.)*rdt20*rdt21)*t1*t1
			+(A21-A20)*rdt20*rdt21*t0*t1;
	//flt2 c0=A10*rdt20*rdt21*t2*t2+((A20-A10*2.)*rdt20*rdt21*t1-A20*rdt20*rdt21*t0)*t2+(A10-A20)*rdt20*rdt21*t1*t1+A20*rdt20*rdt21*t0*t1;

	b0=p1;
	b1=(c1+b0*3.)/3.;
	b2=(c2+b1*6.-b0*3.)/3.;
	b3=p2;//(c3+b2*3.-b1*3.+b0);
}
