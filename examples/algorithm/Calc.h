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

	expr:
	sum =  product ( "+" product | "-" product ) * ;
	product = terminal ( "*" terminal | "/" terminal ) * ;
	terminal = "-" terminal | id "(" ")" | id "(" arguments ")" | "(" sum ")" | number | id;
	arguments =  sum ( "," sum ) * ;

	id = expr

	Lexical analyser:
	id = alpha ( alphanum ) *
	fraction = "." digit *
	float = digit * fraction | fraction // This seem to accept single . as float constant, TODO: fix
	int = digit +
	number = int | float
	operator = "-" | "+" | "/" | "*" | "(" | ")" | "," | "=" 

 * 
 */


#include <math.h>
#include <stdio.h>
#include "../include/strn.h"

#include "Lex.h"

#define M_PI 3.14159265358979323846

struct SVariable
{
	char name[TOK_LEN];
	float value;
};

struct Calc
{
	#define NTOK 64
	SToken t[NTOK];
	#define NVARS 64
	SVariable vars[NVARS];
	void Init()
	{
		for(int i=0;i<NTOK;i++)t[i].type=END;
		for(int i=0;i<NVARS;i++)vars[i].name[0]=0;
	}
	int AddVariable(char* name)
	{
		int found=FindVariable(name);
		if(found==-1)
		{
			for(int i=0;i<NVARS;i++)
			{
				if(vars[i].name[0]==0)
				{
					strncp(&(vars[i].name[0]),name,TOK_LEN);
					return i;
				}
			}
		}
		return found;
	}
	int FindVariable(char* name)
	{
		for(int i=0;i<NVARS;i++)
		{
			if(vars[i].name[0]==0)break;
			if(strneq(&(vars[i].name[0]),name,TOK_LEN))
			{
				return i;
			}
		}
		return -1;
	}
};

#define STX_START_CHECK if(!Yes()){return {calc, p, status};}

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
	Calc* calc;
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
			return {calc, e1.p, YES};
		}
		return {calc, p, NO};
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
			return {calc, e1.p, YES};
		}
		return {calc, p, NO};
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
						return {calc, p, ERROR};
					}
				}
				else
				{
					return {calc, e0.p, YES};
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
				// printf("Function call %s()\n", id);
				if(strneq(id,"pi",TOK_LEN))
				{
					val=M_PI;
				}
				if(strneq(id,"vars",TOK_LEN))
				{
					int cnt=0;
					for(int i=0;i<NVARS;i++)
					{
						if(calc->vars[i].name[0]==0)
							break;
						printf("  %s=%f\n", calc->vars[i].name, calc->vars[i].value);
						cnt++;
					}
					val=cnt;
				}
				return e0;
			}
			float args[8];
			int nargs;
			e0=e.TryArguments(args, nargs).TryOperator(')');
			if(e0.Yes())
			{
				/*
				printf("Function call %s(", id);
				for(int i=0;i<nargs;i++)
				{
					if(i)printf(",");
					printf("%f", args[i]);
				}
				printf(")\n");
				*/
				if(strneq(id,"pow",TOK_LEN)&&(nargs==2))
				{
					val=pow(args[0],args[1]);
				}
				if(strneq(id,"sqrt",TOK_LEN)&&(nargs==1))
				{
					val=sqrt(args[0]);
				}
				if(strneq(id,"sin",TOK_LEN)&&(nargs==1))
				{
					val=sin(args[0]);
				}
				if(strneq(id,"cos",TOK_LEN)&&(nargs==1))
				{
					val=cos(args[0]);
				}
				return e0;
			}
			return {calc, e.p, ERROR};
		}
		e=TryValue(val0);
		if(e.Yes())
		{
			val=val0;
			return e;
		}
		return {calc, p, NO};
	}
	Stx TryValue(float& val)
	{
		STX_START_CHECK
		if(calc->t[p].IsNumber())
		{
			val=calc->t[p].GetNumber();
			return {calc, p+1, YES};
		}
		if(calc->t[p].IsId())
		{
			int n=calc->FindVariable(&(calc->t[p].value[0]));
			if(n>=0)
				val=calc->vars[n].value;
			else
			{
				// printf("Variable %s not found\n", calc->t[p].value);
			}
			return {calc, p+1, YES};
		}
		return {calc, p, NO};
	}
	Stx TryId(char* id)
	{
		STX_START_CHECK
		if(calc->t[p].IsId())
		{
			for(int i=0;i<TOK_LEN;i++)
			{
				id[i]=calc->t[p].value[i];
			}
			return {calc, p+1, YES};
		}
		return {calc, p, NO};
	}
	Stx TryOperator(int op)
	{
		STX_START_CHECK
		char c=char(op);
		if(calc->t[p].IsOperator(op))
		{
			return {calc, p+1, YES};
		}
		return {calc, p, NO};
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
			return {calc, p, NO};
		}
		return {calc, p, ERROR};
	}
	Stx TryAssign(float& result)
	{
		STX_START_CHECK
		Stx c=*this;
		float val;
		char id[TOK_LEN];
		Stx e=c.TryId(id).TryOperator('=').TryExpr(val);
		if(e.Yes())
		{
			int index=calc->FindVariable(&id[0]);
			if(index<0)
			{
				for(int i=0;i<NVARS;i++)
				{
					if(calc->vars[i].name[0]==0)
					{
						index=i;
						break;
					}
				}
			}
			if(index>=0)
			{
				strncp(&(calc->vars[index].name[0]),id,TOK_LEN);
				calc->vars[index].value=val;
			}
			result=val;
			return e;
		}
		if(e.No())
		{
			return {calc, p, NO};
		}
		return {calc, p, ERROR};
	}
};


