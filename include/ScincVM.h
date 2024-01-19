#include <vector>
#include <cstdint>
#include <cstring>
#include "ScincBase.h"

typedef uint_fast32_t VM_REG_TYPE;

class ScincVM
{
public:
	VM_REG_TYPE segs[6];
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
		TFloat val; memcpy(&val, &mem[addr], sizeof(val)); return val;
		//return (*((TFloat*)(&mem[addr])));
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
		memcpy(&mem[addr],&val, sizeof(val));
		//(*((TFloat*)(&mem[addr])))=val;
	}
	// if different dlls want to exchange data
	std::vector<unsigned char>hostdata;
	int PutHostData(int size, const void* srcData)
	{
		hostdata.resize(size);
		memcpy(hostdata.data(),srcData,size);
		return size;
	}
	int GetHostData(int size, void* destData)
	{
		if(size!=(int)hostdata.size())
			return 0;
		memcpy(destData,hostdata.data(), size);
		return size;
	}
};
