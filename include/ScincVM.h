#include <vector>
#include <cstdint>
#include "ScincBase.h"

typedef uint_fast32_t VM_REG_TYPE;

class ScincVM
{
public:
	union{
		struct
		{
			VM_REG_TYPE rsp;
			VM_REG_TYPE rthis;
			VM_REG_TYPE _glob;
		};
		VM_REG_TYPE segs[3];
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