/**
 * @file Calc.cpp
 * @author klk
 * @brief Simple calculator
 * @version 0.1
 * @date 2024-03-28
 * 
 * @copyright Copyright (c) 2024
 * 
 * Supports float values, - + / * operators, parenthesis, pi() and pow(x,y) built-in functions

	Parser accepts following PEG and interprets expressions on the fly

	sum =  product ( "+" product | "-" product ) * ;
	product = terminal ( "*" terminal | "/" terminal ) * ;
	terminal = "-" terminal | id "(" ")" | id "(" arguments ")" | "(" sum ")" | number ;
	arguments =  sum ( "," sum ) * ;

	Lexical analyser:
	id = alpha ( alphanum ) *
	fraction = "." ( digit ) *
	float = digit + fraction 
	number = int | float
	operator = "-" | "+" | "/" | "*" | "(" | ")" | "," 

 * 
 */


#include <math.h>
#include <stdio.h>

#define M_PI 3.14159265358979323846

bool streq(char* l, char* r, int n)
{
	for(int i=0;i<n;i++)
	{
		if(l[i]!=r[i])
		{
			return false;
		}
		else if(l[i]==0)
		{
			break;
		}
	}
	return true;
}

#define END  0
#define NUM  1
#define OP   2
#define ID   3

#define ERROR 0
#define YES   1
#define NO    2

#define TOK_LEN 16

struct SToken
{
	int type;
	char value[TOK_LEN];
	void SetNumber(float val){type=NUM;*((float*)value)=val;}
	void SetOperator(int val){type=OP;value[0]=val;}
	void SetEnd(){type=END;}
	void SetId(char* id)
	{
		type=ID;
		for(int i=0;i<TOK_LEN;i++)
		{
			value[i]=id[i];
			if(id[i]==0)
			{
				break;
			}
		}
		value[TOK_LEN-1]=0;
	}
	bool IsNumber()
	{
		if(type==NUM)
			return true;
		return false;
	}
	bool IsId()
	{
		if(type==ID)
			return true;
		return false;
	}
	float GetNumber(){return *((float*)value);}
	bool IsOperator(int op){return (type==OP)&&(value[0]==op);}
};

#define NTOK 64

SToken t[NTOK]={};

#define STX_START_CHECK if(!Yes()){return {p, status};}
//printf("%s %i\n", __func__, p);

char* Status(int status)
{
	switch(status)
	{
		case NO: return "NO"; break;
		case YES: return "YES"; break;
		case ERROR: return "ERROR"; break;
		default: return "???";
	}
}

// Syntax context
struct Stx
{
	int p;
	int status;
	bool Yes(){return status==YES;}
	bool No(){return status==NO;}
	bool Error(){return status==ERROR;}
	Stx TryProduct(float& val)
	{
		STX_START_CHECK
		Stx e;
		float val0;
		float val1;
		e=TryTerminal(val0);
		if(e.Yes())
		{
			Stx e1=e;
			val=val0;
			while(true)
			{
				Stx e2=e1.TryOperator('*').TryTerminal(val1);
				if(e2.Yes())
				{
					val=val*val1;
					e1=e2;
				}
				else
				{
					Stx e2=e1.TryOperator('/').TryTerminal(val1);
					if(e2.Yes())
					{
						val=val/val1;
						e1=e2;
					}
					else
					{
						return e1;
					}
				}
			}
			return {e1.p, YES};
		}
		return {p,NO};
	}
	Stx TrySum(float& val)
	{
		STX_START_CHECK
		Stx e;
		float val0;
		float val1;
		e=TryProduct(val0);
		if(e.Yes())
		{
			Stx e1=e;
			val=val0;
			while(true)
			{
				Stx e2=e1.TryOperator('+').TryProduct(val1);
				if(e2.Yes())
				{
					val=val+val1;
					e1=e2;
				}
				else
				{
					Stx e2=e1.TryOperator('-').TryProduct(val1);
					if(e2.Yes())
					{
						val=val-val1;
						e1=e2;
					}
					else
					{
						return e1;
					}
				}
			}
			return {e1.p, YES};
		}
		return {p,NO};
	}
	Stx TryArguments(float* values, int& n)
	{
		STX_START_CHECK
		Stx e;
		e=TrySum(values[0]);
		int i=1;
		if(e.Yes())
		{
			n=1;
			Stx e0=e;
			while(true)
			{
				e0=e0.TryOperator(',');
				if(e0.Yes())
				{
					e0=e0.TrySum(values[i]);
					if(e0.Yes())
					{
						i++;
						n++;
						continue;
					}
					else
					{
						return {p, ERROR};
					}
				}
				else
				{
					return {e0.p, YES};
				}
			}
		}
		return e;
	}
	Stx TryTerminal(float& val)
	{
		STX_START_CHECK
		float val0;
		Stx e;
		e=TryOperator('(').TrySum(val0).TryOperator(')');
		if(e.Yes())
		{
			val=val0;
			return e;
		}
		e=TryOperator('-').TryTerminal(val0);
		if(e.Yes())
		{
			val=-val0;
			return e;
		}
		char id[TOK_LEN];
		e=TryId(id).TryOperator('(');
		if(e.Yes())
		{
			Stx e0=e.TryOperator(')');
			if(e0.Yes())
			{
				printf("Function call %s()\n", id);
				if(streq(id,"pi",8))
				{
					val=M_PI;
				}
				return e0;
			}
			float args[8];
			int nargs;
			e0=e.TryArguments(args, nargs).TryOperator(')');
			if(e0.Yes())
			{
				printf("Function call %s(", id);
				for(int i=0;i<nargs;i++)
				{
					if(i)printf(",");
					printf("%f", args[i]);
				}
				printf(")\n");
				if(streq(id,"pow",8)&&(nargs==2))
				{
					val=pow(args[0],args[1]);
				}
				return e0;
			}
			return {e.p, ERROR};
		}
		e=TryValue(val0);
		if(e.Yes())
		{
			val=val0;
			return e;
		}
		return {p,NO};
	}
	Stx TryValue(float& val)
	{
		STX_START_CHECK
		if(t[p].IsNumber())
		{
			val=t[p].GetNumber();
			return {p+1, YES};
		}
		// TODO: add variables
		return {p,NO};
	}
	Stx TryId(char* id)
	{
		STX_START_CHECK
		if(t[p].IsId())
		{
			for(int i=0;i<TOK_LEN;i++)
			{
				id[i]=t[p].value[i];
			}
			return {p+1, YES};
		}
		return {p,NO};
	}
	Stx TryOperator(int op)
	{
		STX_START_CHECK
		char c=char(op);
		if(t[p].IsOperator(op))
		{
			return {p+1, YES};
		}
		return {p,NO};
	}
	Stx TryExpr(float& result)
	{
		STX_START_CHECK
		Stx c=*this;
		float op;
		Stx e=c.TrySum(op);
		if(e.Yes())
		{
			result=op;
			return e;
		}
		if(e.No())
		{
			return {p,NO};
		}
		return {p,ERROR};
	}
};

char* hs;

// Lexical context

#define LTX_START_CHECK if(!Yes()){return {p, status};}

struct Ltx
{
	int p;
	int status;
	bool Yes(){return status==YES;}
	bool No(){return status==NO;}
	bool Error(){return status==ERROR;}
	Ltx TryDigit(int& digit)
	{
		LTX_START_CHECK
		int c=hs[p]-'0';
		if((c>=0)&&(c<=9))
		{
			digit=c;return {p+1, YES};
		}
		return {p, NO};
	}
	Ltx TryAlpha(int& sym)
	{
		LTX_START_CHECK
		int c=hs[p];
		if(
			((c>='a')&&(c<='z'))
			||((c>='A')&&(c<='Z'))
			||(c=='_')
			)
		{
			sym=c;
			return {p+1, YES};
		}
		return {p, NO};
	}
	Ltx TryAlphaNum(int& sym)
	{
		LTX_START_CHECK
		int c=hs[p];
		if(
			(((c>='a')&&(c<='z'))||((c>='A')&&(c<='Z')))
			||((c>='0')&&(c<='9'))
			||(c=='_')
			)
		{
			sym=c;
			return {p+1, YES};
		}
		return {p, NO};
	}
	Ltx TryId(char* id)
	{
		LTX_START_CHECK
		Ltx e=SkipSpaces();
		int id0;
		e=e.TryAlpha(id0);
		if(e.Yes())
		{
			id[0]=id0;
			id[1]=0;
			int i=1;
			while(true)
			{
				e=e.TryAlphaNum(id0);
				if(e.Yes())
				{
					if(i<TOK_LEN-1)
					{
						id[i]=id0;
						id[i+1]=0;
					}
					i++;
				}
				else break;
			}
			return {e.p, YES};
		}
		return {p, NO};
	}
	Ltx TryDot()
	{
		LTX_START_CHECK
		if(hs[p]=='.'){return {p+1, YES};}
		return {p, NO};
	}
	Ltx SkipSpaces()
	{
		LTX_START_CHECK
		int i=p;
		while(true)
		{
			int c=hs[i];
			if((c==' ')||(c=='\t')||(c=='\n')){i++;}
			else return {i, YES};
		}
		return {p, YES};
	}
	Ltx TryOperator(int& op)
	{
		LTX_START_CHECK
		Ltx e=SkipSpaces();
		int c=hs[e.p];
		if((c=='-')||(c=='+')||(c=='*')||(c=='/')||(c=='(')||(c==')')||(c==',')){op=c;return {e.p+1, YES};}
		return {p, NO};
	}
	Ltx TryInt(int& val)
	{
		LTX_START_CHECK
		int digit;
		Ltx e=SkipSpaces();
		e=e.TryDigit(digit);
		if(e.Yes())
		{
			val=digit;
			while(true)
			{
				e=e.TryDigit(digit);
				if(e.Yes()){val=val*10+digit;}
				else return {e.p, YES};
			}
		}
		return e;
	}
	Ltx TryFraction(float& val)
	{
		LTX_START_CHECK
		Ltx e=TryDot();
		if(e.Yes())
		{
			float scale=10.;
			while(true)
			{
				int digit;
				e=e.TryDigit(digit);
				if(e.Yes()){val+=digit/scale;scale*=10.;}
				else return {e.p, YES};
			}
		}
		else return {e.p, NO};
	}
	Ltx TryFloat(float& val)
	{
		LTX_START_CHECK
		int digit=0;
		Ltx e=SkipSpaces();
		e=e.TryDigit(digit);
		if(e.Yes())
		{
			val=digit;
			while(true)
			{
				e=e.TryDigit(digit);
				if(e.Yes()){val=val*10+digit;}
				else break;
			}
		}
		{
			float f=0;
			e.status=YES;
			e=e.TryFraction(f);
			if(e.Yes())
			{
				val+=f;
				return e;
			}
		}
		return e;
	}
};

int MakeTokens(SToken*t, char* s)
{
		hs=s;
		Ltx c={0,YES};
		int i=0;
		while(true)
		{
			int num;
			float fnum=0.0;
			Ltx e;
			e=c.TryFloat(fnum);
			if(e.Yes())
			{
				t[i].SetNumber(fnum);t[i+1].SetEnd();
				i++;
				c=e;
				continue;
			}
			e=c.TryInt(num);
			if(e.Yes())
			{
				t[i].SetNumber(num);t[i+1].SetEnd();
				i++;
				c=e;
				continue;
			}
			int oper;
			e=c.TryOperator(oper);
			if(e.Yes())
			{
				t[i].SetOperator(oper);t[i+1].SetEnd();
				i++;
				c=e;
				continue;
			}
			char id0[TOK_LEN];
			e=c.TryId(id0);
			if(e.Yes())
			{
				t[i].SetId(id0);t[i+1].SetEnd();
				i++;
				c=e;
				continue;
			}
			printf("End of stream\n");
			break;
		}
	return 0;
}

int main()
{
	while(1)
	{
		char g[256];
		if(fgets(g,256,stdin))
		{
			printf("Got line `%s`\n", g);
			MakeTokens(t,g);
			bool end=false;
			for(int i=0;i<NTOK;i++)
			{
				if(i)printf(" ");
				int type=t[i].type;
				switch(type)
				{
					case NUM: printf("%f", t[i].GetNumber());break;
					case ID:  printf("%s", t[i].value);break;
					case OP:  printf("%c", t[i].value[0]);break;
					case END: printf("\n"); end=true; break;
				}
				if(end)
				{
					break;
				}
			}
			Stx c={0,YES};
			float res;
			Stx e=c.TryExpr(res);
			if(e.Yes())
			{
				printf("Result: %f\n", res);
			}
			else
			{
				printf("No expr or error\n");
			}
		}else return 0;
	}
	return 0;
}
