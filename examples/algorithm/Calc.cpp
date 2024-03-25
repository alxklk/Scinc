#include <stdio.h>

#define NUM  1
#define OP 2
#define END 3

#define YES 1
#define NO 2
#define ERROR 0

struct SToken
{
	int type;
	float value;
	bool IsNum()
	{
		if(type==NUM)
			return true;
		return false;
	}
	bool IsOp(int op){return (type==OP)&&(value==float(op));}
};

#define NTOK 32

SToken t[NTOK]={{NUM,3},{OP,'+'},{NUM,2},{END}};

#define STANDARD_CHECK if(!Yes()){return {p, status};}
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

/*
sum =  product ( "+" product | "-" product ) *;
product = term ( "*" term | "/" term ) * ;
term = "-" term | "(" sum ")" | number ;
*/

struct Ctx
{
	int p;
	int status;
	bool Yes(){return status==YES;}
	bool No(){return status==NO;}
	bool Error(){return status==ERROR;}
	Ctx IsProduct(float& val)
	{
		STANDARD_CHECK
		Ctx e;
		float val0;
		float val1;
		e=IsTerm(val0);
		if(e.Yes())
		{
			Ctx e1=e;
			val=val0;
			while(true)
			{
				Ctx e2=e1.IsOp('*').IsTerm(val1);
				if(e2.Yes())
				{
					val=val*val1;
					e1=e2;
				}
				else
				{
					Ctx e2=e1.IsOp('/').IsTerm(val1);
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
	Ctx IsSum(float& val)
	{
		STANDARD_CHECK
		Ctx e;
		float val0;
		float val1;
		e=IsProduct(val0);
		if(e.Yes())
		{
			Ctx e1=e;
			val=val0;
			while(true)
			{
				Ctx e2=e1.IsOp('+').IsProduct(val1);
				if(e2.Yes())
				{
					val=val+val1;
					e1=e2;
				}
				else
				{
					Ctx e2=e1.IsOp('-').IsProduct(val1);
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
	Ctx IsTerm(float& val)
	{
		STANDARD_CHECK
		float val0;
		Ctx e;
		e=IsOp('(').IsSum(val0).IsOp(')');
		if(e.Yes())
		{
			val=val0;
			return e;
		}
		e=IsOp('-').IsTerm(val0);
		if(e.Yes())
		{
			val=-val0;
			return e;
		}
		e=IsValue(val0);
		if(e.Yes())
		{
			val=val0;
			return e;
		}
		return {p,NO};
	}
	Ctx IsValue(float& val)
	{
		STANDARD_CHECK
		if(t[p].IsNum())
		{
			val=t[p].value;
			return {p+1, YES};
		}
		// TODO: add variables
		return {p,NO};
	}
	Ctx IsOp(int op)
	{
		STANDARD_CHECK
		char c=char(op);
		if(t[p].IsOp(op))
		{
			return {p+1, YES};
		}
		return {p,NO};
	}
	Ctx IsExpr(float& result)
	{
		STANDARD_CHECK
		Ctx c=*this;
		float op;
		Ctx e=c.IsSum(op);
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

bool IsNumeric(int x)
{
	return ((x>='0')&&(x<='9'));
}

int MakeTokens(SToken*t, char* s)
{
	int i=0;
	int j=0;
	while(1)
	{
		int c=s[i];
		switch(c)
		{
			case '0':case '1':case '2':case '3':case '4':
			case '5':case '6':case '7':case '8':case '9':
				t[j].type=NUM;
				t[j].value=c-'0';
				i++;
				j++;
			break;
			case '(':case ')':case '+':case '-':case '/':case '*':
				t[j].type=OP;
				t[j].value=c;
				i++;
				j++;
			break;
			case ' ':case '\t':
				i++;
				break;
			default:
				t[j].type=END;
				return 1;
		}
	}
	return 0;
}

int main()
{
	//char* s="(2+3)*4";
	char* s="-(2*1*(6-5))";
	while(1)
	{
		char g[256];
		gets_s(g,256);
		s=g;
		printf("Got \"%s\"\n", g);
		MakeTokens(t,s);
		bool end=false;
		for(int i=0;i<NTOK;i++)
		{
			if(i)printf(" ");
			int type=t[i].type;
			switch(type)
			{
				case NUM: printf("%f", t[i].value);break;
				case OP:  printf("%c", (int)t[i].value);break;
				case END: printf("\n"); end=true; break;
			}
			if(end)
			{
				break;
			}
		}
		Ctx c={0,YES};
		//c.Print();
		float res;
		Ctx e=c.IsExpr(res);
		if(e.Yes())
		{
			printf("Result: %f\n", res);
		}
		else
		{
			printf("No expr or error\n");
		}
	}
	return 0;
}
