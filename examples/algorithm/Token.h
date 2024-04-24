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
	bool IsEnd()
	{
		return (type==END);
	}
	float GetNumber(){return *((float*)value);}
	bool IsOperator(int op){return (type==OP)&&(value[0]==op);}
};
