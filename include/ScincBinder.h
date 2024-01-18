//
// Created by klk on 9/5/20.
//

#ifndef SCRIP_SCINCBINDER_H
#define SCRIP_SCINCBINDER_H

#include "ScincBase.h"
#include <cstring>
#include <tuple>

// User type must include static const int field 'size_of'
// Can contain native pointers and opaque pointers, structures, handles etc

// maps C++ types to corresponding sizes inside Scinc
template<typename T>struct TSBSizeOf                {static const int size=T::size_of;};
template <>         struct TSBSizeOf<float>         {static const int size=sizeof(TFloat);};
template <>         struct TSBSizeOf<double>        {static const int size=sizeof(TFloat);};
template <>         struct TSBSizeOf<int>           {static const int size=4;};
template <>         struct TSBSizeOf<char>          {static const int size=4;};
template <>         struct TSBSizeOf<char*>         {static const int size=4;};
template <>         struct TSBSizeOf<const char*>   {static const int size=4;};
template <>         struct TSBSizeOf<int*>          {static const int size=4;};
template <>         struct TSBSizeOf<const int*>    {static const int size=4;};
template <>         struct TSBSizeOf<double*>       {static const int size=4;};
template <>         struct TSBSizeOf<const double*> {static const int size=4;};
template <>         struct TSBSizeOf<void>          {static const int size=0;}; // Unlike C++, Scinc allows types with zero size
template <>         struct TSBSizeOf<void*>         {static const int size=4;};
template <>         struct TSBSizeOf<const void*>   {static const int size=4;};

template<typename I, typename T>class TSBGetter
{
public:
	static void Get(I* pit, T& val, int offs)
	{
		memcpy(&val,&(pit->mem[pit->rsp-offs]),T::size_of);
	}
};

template<typename I>class TSBGetter<I, float>      {public:static void Get(I* pit, float & val, int offs){val=pit->GetFloat(pit->rsp-offs);}};
template<typename I>class TSBGetter<I, double>     {public:static void Get(I* pit, double& val, int offs){val=pit->GetFloat(pit->rsp-offs);}};
template<typename I>class TSBGetter<I, int>        {public:static void Get(I* pit, int   & val, int offs){val=pit->GetInt  (pit->rsp-offs);}};
template<typename I>class TSBGetter<I, char>       {public:static void Get(I* pit, char  & val, int offs){val=pit->GetChar (pit->rsp-offs);}};
template<typename I>class TSBGetter<I, char*>      {public:static void Get(I* pit, char* & val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(char*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, const char*>{public:static void Get(I* pit, const char*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const char*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, int*>{public:static void Get(I* pit, int*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(int*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, const int*>{public:static void Get(I* pit, const int*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const int*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, double*>{public:static void Get(I* pit, double*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(double*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, const double*>{public:static void Get(I* pit, const double*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const double*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, void*>      {public:static void Get(I* pit, void* & val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(void*)&(pit->mem[ptr]);
}};
template<typename I>class TSBGetter<I, const void*>      {public:static void Get(I* pit, const void* & val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const void*)&(pit->mem[ptr]);
}};

template<typename I, typename T>class TSBPutter
{
public:
	static void Put(I* pit, T& val, int offs)
	{
		memcpy(&(pit->mem[pit->rsp-offs]),&val,T::size_of);
	}
};
template<typename I>class TSBPutter<I, float> {public:static void Put(I* pit, float & val, int offs){pit->PutFloat(pit->rsp-offs,val);}};
template<typename I>class TSBPutter<I, double>{public:static void Put(I* pit, double& val, int offs){pit->PutFloat(pit->rsp-offs,val);}};
template<typename I>class TSBPutter<I, int>   {public:static void Put(I* pit, int   & val, int offs){pit->PutInt  (pit->rsp-offs,val);}};
template<typename I>class TSBPutter<I, char>  {public:static void Put(I* pit, char  & val, int offs){pit->PutChar (pit->rsp-offs,val);}};


template<typename R, typename ...As>class Wrapper
{
	template<typename I, int i, typename ...A>static void IterateArgs(I* pit, int& offs, std::tuple<A...>&args)
	{
		if constexpr(i<sizeof ...(A))
		{
			offs+=TSBSizeOf<typename std::tuple_element<i, std::tuple<As...> >::type>::size;
			TSBGetter<I,typename std::tuple_element<i, std::tuple<As...> >::type>::Get(pit,std::get<i>(args),offs);
			IterateArgs<I,i+1,A...>(pit,offs,args);
		}
	}
public:
	template<typename I>static int func(I* pit, void* userdata)
	{
		std::tuple<As...>args;
		int offs=TSBSizeOf<R>::size;
		IterateArgs<I,0,As...>(pit,offs,args);

		R(*fptr)(As...)=(R(*)(As...))userdata;

		if constexpr(!std::is_same<R,void>())
		{
			R retval=std::apply(fptr,args);
			TSBPutter<I,R>::Put(pit,retval,TSBSizeOf<R>::size);
		}
		else
		{
			std::apply(fptr,args);
		}
		return 0;
	}
};

template<typename T> struct TSBType;
template<>           struct TSBType<float>         {static const int type=Type::Float;};
template<>           struct TSBType<double>        {static const int type=Type::Float;};
template<>           struct TSBType<int>           {static const int type=Type::Int;};
template<>           struct TSBType<void>          {static const int type=Type::Void;};
template<>           struct TSBType<char>          {static const int type=Type::Char;};
template<>           struct TSBType<char*>         {static const int type=Type::CharPtr;};
template<>           struct TSBType<const char*>   {static const int type=Type::CharPtr;};
template<>           struct TSBType<int*>          {static const int type=Type::IntPtr;};
template<>           struct TSBType<const int*>    {static const int type=Type::IntPtr;};
template<>           struct TSBType<double*>       {static const int type=Type::FloatPtr;};
template<>           struct TSBType<const double*> {static const int type=Type::FloatPtr;};
template<>           struct TSBType<void*>         {static const int type=Type::VoidPtr;};
template<>           struct TSBType<const void*>   {static const int type=Type::VoidPtr;};

template<typename C> class Binder
{
	template<int i=0, typename...A>static void IterateArgTypes(std::vector<long>&argTypes)
	{
		if constexpr(i<sizeof...(A))
		{
			argTypes.push_back(TSBType<typename std::tuple_element<i, std::tuple<A...> >::type>::type);
			IterateArgTypes<i+1, A...>(argTypes);
		}
	}
public:
	template<typename R, typename... As>static void Bind(C& ctx, const char* name, R(*function)(As...))
	{
		std::vector<long>argTypes;
		IterateArgTypes<0,As...>(argTypes);
		AddFunc(ctx, TSBType<R>::type, "", name, argTypes,&Wrapper<R,As...>::func,{},(void*)function);
	}
};

template<typename I, typename R, typename... As>int ScincBoundCall(I*pit, R(*function)(As...))
{
	Wrapper<R,As...>::func(pit,(void*)function);
	return 1;
}


#endif //SCRIP_SCINCBINDER_H
