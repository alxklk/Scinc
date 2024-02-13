#include <stdio.h>

#ifdef __SCINC__
#define const
#endif

#ifndef FIXED_DIGITS
#define FIXED_DIGITS 250
#endif

#ifndef FIXED_FRACTS
#define FIXED_FRACTS (FIXED_DIGITS/2)
#endif

char* stdDividers[9]=
{
"-999",
"5.0"    ,
"3.33333",
"2.5"    ,
"2.0"    ,
"1.66666",
"1.42857",
"1.25"   ,
"1.11111"
};
class fixed
{
public:
	static int pow_of_2(int v)
	{
		int r=0;
		while(v>>=1)
		{
			r++;
		}
		return r;
	}
	int sign;
	int digits[FIXED_DIGITS];
	void Zero()
	{
		sign=1;
		for(int i=0;i<FIXED_DIGITS;i++)digits[i]=0;
	}
	fixed& operator=(int r)
	{
		Seti(r);
		return *this;
	}
	void Seti(int x)
	{
		Zero();
		if(x)
		{
			if(x<0)
			{
				sign=-1;
				x=-x;
			}
			for(int i=0;i<(FIXED_DIGITS-FIXED_FRACTS);i++)
			{
				digits[FIXED_FRACTS+i]=x%10;
				x/=10;
				if(x==0)
					break;
			}
		}
	}

	static fixed FromS(char* s)
	{
		fixed r;
		r.Sets(s);
		return r;
	}

	static fixed FromI(int i)
	{
		fixed r;
		r.Seti(i);
		return r;
	}

	void Sets(char* num)
	{
		Zero();
		int istart=0;
		sign=1;
		if(num[0]=='-')
		{
			sign=-1;
			istart=1;
		}
		int iend=0;
		for(int i=istart;i<FIXED_DIGITS;i++)
		{
			if(num[i]=='.')
			{
				iend=i;
				break;
			}
			if(num[i]==0)
			{
				iend=i;
				break;
			}
		}
		for(int i=iend-1;i>=istart;i--)
		{
			int index=FIXED_FRACTS-1+(iend-istart)-(i-istart);
			digits[index]=num[i]-'0';
		}
		if(num[iend]=='.')
		{
			for(int i=0;i<FIXED_FRACTS;i++)
			{
				int index=iend+i+1;
				if(num[index]==0)
					break;
				digits[FIXED_FRACTS-1-i]=num[index]-'0';
			}
		}
	}

	int Lead() const
	{
		int lead=FIXED_DIGITS-1;
		for(int i=FIXED_DIGITS-1;i>0;i--)
		{
			if(digits[i]==0)
				lead=i-1;
			else
				break;
		}
		return lead;
	}

	int End() const
	{
		int end=0;
		for(int i=0;i<FIXED_DIGITS-1;i++)
		{
			if(digits[i]==0)
				end=i;
			else
				break;
		}
		//printf(" >>>>>>End=%i\n", end);
		return end;
	}

	int Shift01()
	{
		int lead=Lead();
		if(lead<FIXED_FRACTS-1)
		{
			return FIXED_FRACTS-lead-1;
		}
		else if(lead>FIXED_FRACTS-1)
		{
			return FIXED_FRACTS-lead-1;
		}
		else return 0;
	}

	int modgt(const fixed& right)const
	{
		for(int i=FIXED_DIGITS-1;i>=0;i--)
		{
			int a=digits[i];
			int b=right.digits[i];
			if(a<b)
			{
				return 0;
			}
			else if(a>b)
			{
				return 1;
			}
		}
		return 0;
	}

	fixed operator+(const fixed& right)
	{
		if(sign!=right.sign)
		{
			fixed tmp=right;
			tmp.sign=-tmp.sign;
			return *this-tmp;
		}
		fixed ret;
		ret.sign=sign;
		for(int i=0;i<FIXED_DIGITS;i++)
			ret.digits[i]=0;
		int carry=0;
		for(int i=0;i<FIXED_DIGITS;i++)
		{
			int a=digits[i];
			int b=right.digits[i];
			int res=a+b+carry;
			carry=0;
			if(res>9)
			{
				carry=1;
				res=res-10;
			}
			ret.digits[i]=res;
		}
		return ret;
	}

	bool operator<(const fixed& right)
	{
		if(sign<right.sign)
			return true;
		else if(sign>right.sign)
			return false;
		fixed r=*this-right;
		return r.sign<0;
	}

	fixed operator-(const fixed& right)
	{
		if(sign!=right.sign)
		{
			fixed tmp=right;
			tmp.sign=-tmp.sign;
			return *this+tmp;
		}
		fixed ret;
		ret.sign=sign;
		for(int i=0;i<FIXED_DIGITS;i++)
			ret.digits[i]=0;
		int carry=0;
		int swap=0;

		if(!modgt(right))
			swap=1;
		if(swap)
			ret.sign=-ret.sign;
		for(int i=0;i<FIXED_DIGITS;i++)
		{
			int res;
			int a=digits[i];
			int b=right.digits[i];
			if(swap)
				res=b-a-carry;
			else
				res=a-b-carry;
			carry=0;
			if(res<0)
			{
				carry=1;
				res=res+10;
			}
			ret.digits[i]=res;
		}
		return ret;
	}

	fixed operator*(const fixed& right)
	{
		fixed ret;
		ret.Zero();
		ret.sign=sign*right.sign;

		int lead0=Lead()+1;
		int end0=End()+1;

		int lead1=right.Lead()+1;
		int end1=right.End()+1;

		for(int i=end0;i<lead0+1;i++)
		{
			int carry=0;
			for(int j=end1;j<lead1+1;j++)
			{
				int index=i-FIXED_FRACTS+j;
				if(index>=0)
				{
					int rd=digits[i]*right.digits[j];
					if(index>=FIXED_DIGITS)
					{
						if(rd)
						{
						// overflow;
						}
					}
					else
					{
						rd+=carry;
						ret.digits[index]+=rd;
						carry=ret.digits[index]/10;
						ret.digits[index]=ret.digits[index]%10;
					}
				}
			}
		}
		return ret;
	}

	fixed operator-()const
	{
		fixed ret=*this;
		ret.sign=-ret.sign;
		return ret;
	}

	void Mul10()
	{
		for(int i=FIXED_DIGITS-1;i>0;i--)
		{
			digits[i]=digits[i-1];
		}
		digits[0]=0;
	}

	void Div10()
	{
		for(int i=0;i<FIXED_DIGITS-1;i++)
		{
			digits[i]=digits[i+1];
		}
		digits[FIXED_DIGITS-1]=0;
	}

	fixed operator/(const fixed& right)
	{
		fixed d=right;
		fixed n=*this;

		int newsign=sign*d.sign;

		d.sign=1;
		n.sign=1;

		int shift=d.Shift01();
		if(shift<0)
		{
			for(int i=0;i< -shift;i++)
			{
				n.Div10();
				d.Div10();
			}
		}
		else if(shift>0)
		{
			for(int i=0;i<shift;i++)
			{
				n.Mul10();
				d.Mul10();
			}
		}

		int iter=0;

		for(int i=0;i<3;i++)
		{
			int topdigit=d.digits[FIXED_FRACTS-1];
			if(topdigit==9)
				break;
			else if(topdigit==0)
			{
				fixed inf;
				inf.Sets("-99.99");
				return inf;
			}
			fixed mul;
			mul.Sets(stdDividers[topdigit]);
			// switch(topdigit)
			// {
			// 	case 1:mul.Sets("5.0"    );break;
			// 	case 2:mul.Sets("3.33333");break;
			// 	case 3:mul.Sets("2.5"    );break;
			// 	case 4:mul.Sets("2.0"    );break;
			// 	case 5:mul.Sets("1.66666");break;
			// 	case 6:mul.Sets("1.42857");break;
			// 	case 7:mul.Sets("1.25"   );break;
			// 	case 8:mul.Sets("1.11111");break;
			// 	default:break;
			// }

#ifdef UNDEIN 
			/* */if(topdigit==1){mul.Sets("5.0"    );}
			else if(topdigit==2){mul.Sets("3.33333");}
			else if(topdigit==3){mul.Sets("2.5"    );}
			else if(topdigit==4){mul.Sets("2.0"    );}
			else if(topdigit==5){mul.Sets("1.66666");}
			else if(topdigit==6){mul.Sets("1.42857");}
			else if(topdigit==7){mul.Sets("1.25"   );}
			else if(topdigit==8){mul.Sets("1.11111");}
#endif
			d=d*mul;
			n=n*mul;
			iter++;
		}

		fixed two;
		two.Seti(2);
		while(true)
		{
			fixed mul=two-d;
			d=d*mul;
			n=n*mul;
			iter++;
			if(iter>5+pow_of_2(FIXED_FRACTS))
				break;
		}
		n.sign=newsign;
		return n;
	}

	void Print(CDigitStream& out)
	{
		out.begin();
		int end=End();
		int lead=Lead();
		if(lead<end)
		{
			printf("0.0");
		}
		if(lead<FIXED_FRACTS)
			lead=FIXED_FRACTS;
		if(end>FIXED_FRACTS-2)end=FIXED_FRACTS-2;

		int nc=0;
		if(sign==-1)
		{
			out.put('-');
		}
		for(int i=lead;i>end;i--)
		{
			int digit=digits[i];
			out.put(int("0123456789"[digit]));
			if(i==FIXED_FRACTS)
			{
				out.put('.');
			}
		}
		out.end();
		//printf("%i %i %s",lead, end, sd);
	}

	fixed Sqrt()
	{
		fixed a;
		a.Sets("1.0");
		fixed h;
		h.Sets("0.5");
		fixed x=*this;

		int shift=Shift01();

		if(shift<0)
			for(int i=0;i< -shift/2;i++)
				a.Mul10();
		else if(shift>0)
			for(int i=0;i< shift/2;i++)
				a.Div10();

		int cnt=4+pow_of_2(FIXED_DIGITS);
		for(int i=0;i<cnt;i++)
		{
			a=(x/a+a)*h;
			printf("Sqrt approx %i=",i);
			//a.Print();
			printf("\n");
		}
		return a;
	}

	// inf  1   /   4        2        1        1    \ .
	// SUM ---- | ------ - ------ - ------ - ------ | .
	// k=0 16^k \  8k+1     8k+4     8k+5     8k+6  / .

	void PI()
	{
		fixed sum;
		sum.Zero();
		fixed bi16;
		bi16.Seti(16);
		fixed pow16;
		pow16.Seti(1);
		for(int k=0;k<FIXED_FRACTS*4/5;k++)
		{
			fixed term;
			term.Zero();

			fixed over0;
			fixed under0;
			over0.Seti(4);
			under0.Seti(8*k+1);
			term=over0/under0;

			fixed over1;
			fixed under1;
			over1.Seti(-2);
			under1.Seti(8*k+4);
			term=over1/under1+term;

			fixed over2;
			fixed under2;
			over2.Seti(-1);
			under2.Seti(8*k+5);
			term=over2/under2+term;

			fixed over3;
			fixed under3;
			over3.Seti(-1);
			under3.Seti(8*k+6);
			term=over3/under3+term;

			fixed termd16=term*pow16;
			sum=sum+termd16;
			pow16=pow16/bi16;
			//sum.Print();
			printf("\n");
		}
		*this=sum;
	}

	void PI0()
	{
		fixed sum;sum.Seti(2);
		fixed nx;nx.Seti(2);
		for(int n=1;n<FIXED_FRACTS*3;n++)
		{
			fixed top;top.Seti(2*n*n);
			fixed bottom;bottom.Seti(2*n*(2*n+1));
			nx=nx*top/bottom;
			sum=sum+nx;
			//sum.Print();
			printf("\n");
		}
		*this=sum;
	}
};
