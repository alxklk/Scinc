#include <vector>
#include "base.h"
class Scinc
{
public:
	union{
		struct
		{
			int rsp;
			int rthis;
			int _glob;
		};
		int segs[3];
	};
	std::vector<unsigned char>mem;
	int GetInt(int addr)const
	{
		return (*((int*)(&mem[addr])));
	}
	int GetChar(int addr) const
	{
		return mem[addr];
	}
	TFloat GetFloat(int addr) const
	{
		return (*((TFloat*)(&mem[addr])));
	}
	void PutInt(int addr, int val)
	{
		(*((int*)(&mem[addr])))=val;
	}
	void PutChar(int addr, char val)
	{
		(*((char*)(&mem[addr])))=val;
	}
	void PutFloat(int addr, const TFloat& val)
	{
		(*((TFloat*)(&mem[addr])))=val;
	}

};