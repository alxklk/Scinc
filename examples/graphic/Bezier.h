#ifndef BEZIER_H
#define BEZIER_H
class BezCalc3
{
public:
	float x0;
	float x1;
	float x2;
	float x3;
	float D1;
	float D2;
	float D3;
	float A;
	float B;
	float C;
	float D;
	float DA;
	float DB;
	float DC;
	float DC2;
	void Init(float ix0, float ix1, float ix2, float ix3)
	{
		x0=ix0;
		x1=ix1;
		x2=ix2;
		x3=ix3;
		C=(x1-x0)*3.0f;
		float d2=(x1-x2)*3.0f;
		A=d2-x0+x3;
		B=-C-d2;
		D=x0;
		DA=3.0f*(x1-x0);
		DB=6.0f*(x0-2.0f*x1+x2);
		DC=3.0f*(3.0f*(x1-x2)+x3-x0);
		DC2=2.0f*DC;
	}
	float Calc(float t){return ((A*t+B)*t+C)*t+D;}
	float Diff(float t){return DA+(DB+(DC)*t)*t;}
};
#endif
