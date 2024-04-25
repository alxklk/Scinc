#include "Token.h"

// Lexical context

#define LTX_START_CHECK if(!Yes()){return {hs, p, status};}

// Global state because multiline comments are multiline by definition
bool multilineComment=false;

struct Ltx
{
	char* hs;
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
			digit=c;return {hs, p+1, YES};
		}
		return {hs, p, NO};
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
			return {hs, p+1, YES};
		}
		return {hs, p, NO};
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
			return {hs, p+1, YES};
		}
		return {hs, p, NO};
	}
	Ltx TryId(char* id)
	{
		LTX_START_CHECK
		Ltx e=*this;
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
			return {hs, e.p, YES};
		}
		return {hs, p, NO};
	}
	Ltx TryDot()
	{
		LTX_START_CHECK
		if(hs[p]=='.'){return {hs, p+1, YES};}
		return {hs, p, NO};
	}
	Ltx SkipSpaces()
	{
		LTX_START_CHECK
		int i=p;
		bool lineComment=false;
		while(true)
		{
			int c=hs[i];
			if(c==0)
			{
				break;
			}
			if((!lineComment)&&(c=='/')&&(hs[i+1]=='*'))
			{
				multilineComment=true;
				i+=2;
				continue;
			}
			if((!lineComment)&&multilineComment)
			{
				if((c=='*')&&((hs[i+1]=='/')))
				{
					multilineComment=false;
					i+=2;
				}
				else
				{
					i++;
				}
				continue;
			}
			if((c=='/')&&(hs[i+1]=='/'))
			{
				lineComment=true;
				i+=2;
				continue;
			}
			if(lineComment)
			{
				if(c=='\n')
				{
					lineComment=false;
				}
				else
				{
					i++;
				}
				continue;
			}
			if((c==' ')||(c=='\t')||(c=='\n')){i++;}
			else return {hs, i, YES};
		}
		return {hs, p, YES};
	}
	Ltx TryOperator(int& op)
	{
		LTX_START_CHECK
		Ltx e=*this;
		int c=hs[e.p];
		if(
			(c=='-')||(c=='+')||
			(c=='*')||(c=='/')||
			(c=='(')||(c==')')||
			(c==',')||(c=='='))
		{
			op=c;
			return {hs, e.p+1, YES};
		}
		return {hs, p, NO};
	}
	Ltx TryInt(int& val)
	{
		LTX_START_CHECK
		int digit;
		Ltx e=*this;
		e=e.TryDigit(digit);
		if(e.Yes())
		{
			val=digit;
			while(true)
			{
				e=e.TryDigit(digit);
				if(e.Yes()){val=val*10+digit;}
				else return {hs, e.p, YES};
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
				else return {hs, e.p, YES};
			}
		}
		else return {hs, e.p, NO};
	}
	Ltx TryFloat(float& val)
	{
		LTX_START_CHECK
		int digit=0;
		Ltx e=*this;
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
		Ltx c={s, 0,YES};
		int i=0;
		while(true)
		{
			int num;
			float fnum=0.0;
			c=c.SkipSpaces();
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
			//printf("End of stream\n");
			break;
		}
	return i;
}
