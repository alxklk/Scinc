struct flt2
{
	float x;
	float y;
	float length()
	{
		return sqrt(x*x+y*y);
	}
	static flt2 New(float x, float y)
	{
		flt2 result;
		result.x=x;
		result.y=y;
		return result;
	}
	flt2 normalized()
	{
		float r=1.0/length();
		flt2 result;
		result.x=x*r;
		result.y=y*r;
		return result;
	}
	flt2 perp()
	{
		flt2 result;
		result.x=y;
		result.y=-x;
		return result;
	}
	void normalize()
	{
		float r=1.0/length();
		x*=r;
		y*=r;
	}
	void Zero()
	{
		x=0;
		y=0;
	}
	flt2 operator- ()
	{
		flt2 result;
		result.x=-x;
		result.y=-y;
		return result;
	}
	flt2 operator- (flt2 r)
	{
		flt2 result;
		result.x=x-r.x;
		result.y=y-r.y;
		return result;
	}
	flt2 operator+ (flt2 r)
	{
		flt2 result;
		result.x=x+r.x;
		result.y=y+r.y;
		return result;
	}
	flt2 operator+= (flt2 r)
	{
		flt2 result;
		result.x=x=x+r.x;
		result.y=y=y+r.y;
		return result;
	}
	flt2 operator-= (flt2 r)
	{
		flt2 result;
		result.x=x=x-r.x;
		result.y=y=y-r.y;
		return result;
	}
	flt2 operator* (float r)
	{
		flt2 result;
		result.x=x*r;
		result.y=y*r;
		return result;
	}
	flt2 operator/ (float r)
	{
		float rr=1.0/r;
		flt2 result;
		result.x=x*rr;
		result.y=y*rr;
		return result;
	}
	flt2 operator/= (float r)
	{
		float rr=1.0/r;
		flt2 result;
		result.x=x*rr;
		result.y=y*rr;
		*this=result;
		return result;
	}
};

float vdot(const flt2& l,const flt2& r){return l.x*r.x+l.y*r.y;}

flt2 reflect(const flt2& r, const flt2& n)
{
	return n*vdot(n,r)*2.0f-r;
}
