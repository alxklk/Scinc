#define PI 3.141592654

struct cplx
{
	float re;
	float im;

	cplx operator +(cplx r)
	{
		cplx res;
		res.re=re+r.re;
		res.im=im+r.im;
		return res;
	}

	cplx operator -(cplx r)
	{
		cplx res;
		res.re=re-r.re;
		res.im=im-r.im;
		return res;
	}

	cplx operator *(float r)
	{
		cplx res;
		res.re=re*r;
		res.im=im*r;
		return res;
	}

	cplx operator *(cplx r)
	{
		cplx res;
		res.re=re*r.re-im*r.im;
		res.im=im*r.re+r.im*re;
		return res;
	}

	cplx operator /(float r)
	{
		cplx res;
		res.re=re/r;
		res.im=im/r;
		return res;
	}

	cplx operator -()
	{
		cplx res;
		res.re=-re;
		res.im=-im;
		return res;
	}

};

cplx I;

cplx cexp(cplx val)
{
	cplx res;
	float u=exp(val.re);
	res.re=cos(val.im)*u;
	res.im=sin(val.im)*u;
	return res;
}

void _fft(cplx* buf, cplx* out, int n, int step)
{
	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(&(out[step]), &(buf[step]), n, step * 2);
 
		for (int i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}

void fft(cplx* buf, cplx* out, int n)
{
	for (int i = 0; i < n; i++) out[i] = buf[i];
 
	_fft(buf, out, n, 1);
}
