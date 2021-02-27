#pragma once

struct Type
{
	static const int Error=-1;
	static const int Auto=0;
	static const int Void=1;
	static const int Char=2;
	static const int Int=3;
	static const int Float=4;
	static const int VoidPtr=5;
	static const int CharPtr=6;
	static const int IntPtr=7;
	static const int FloatPtr=8;
};

typedef double TFloat;
//typedef float TFloat;

enum STEP_RESULT
{
	SR_UNDEF=0,
	SR_N_USSUED,
	SR_INTERRUPT,
	SR_ERROR,
	SR_RETURN,
	SR_CALL_NATIVE,
	SR_FINISHED
};

struct SShortOp
{
	const char* str;
	int pri; // priority
	int opn; // operand number
	int opra; // 1 if right associativity
};

struct SKeyWord
{
	const char* str;
	int kwval;
};

#define XSTR(x) #x
#define ASTR(x) XSTR(x)
#define ERT(s) s
//#define ERT(s) s "@" ASTR(__FILE__) ":" ASTR(__LINE__)
