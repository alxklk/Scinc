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
// template <int N>struct TUserType
// {
// 	static const int size_of=sizeof(TUserType<N>);
// 	char content[N];
// };

template<typename T>struct TSSizeOf                {static const int size=T::size_of;};
template <>         struct TSSizeOf<float>         {static const int size=sizeof(TFloat);};
template <>         struct TSSizeOf<double>        {static const int size=sizeof(TFloat);};
template <>         struct TSSizeOf<int>           {static const int size=4;};
template <>         struct TSSizeOf<char>          {static const int size=4;};
template <>         struct TSSizeOf<char*>         {static const int size=4;};
template <>         struct TSSizeOf<const char*>   {static const int size=4;};
template <>         struct TSSizeOf<int*>          {static const int size=4;};
template <>         struct TSSizeOf<const int*>    {static const int size=4;};
template <>         struct TSSizeOf<double*>       {static const int size=4;};
template <>         struct TSSizeOf<const double *>{static const int size=4;};
template <>         struct TSSizeOf<void>          {static const int size=0;};

template<typename I, typename T>class GS
{
public:
	static void Get(I* pit, T& val, int offs)
	{
		memcpy(&val,&(pit->mem[pit->rsp-offs]),T::size_of);
	}
};

template<typename I>class GS<I, float>      {public:static void Get(I* pit, float & val, int offs){val=pit->GetFloat(pit->rsp-offs);}};
template<typename I>class GS<I, double>     {public:static void Get(I* pit, double& val, int offs){val=pit->GetFloat(pit->rsp-offs);}};
template<typename I>class GS<I, int>        {public:static void Get(I* pit, int   & val, int offs){val=pit->GetInt  (pit->rsp-offs);}};
template<typename I>class GS<I, char>       {public:static void Get(I* pit, char  & val, int offs){val=pit->GetChar (pit->rsp-offs);}};
template<typename I>class GS<I, char*>      {public:static void Get(I* pit, char*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(char*)&(pit->mem[ptr]);
}};
template<typename I>class GS<I, const char*>{public:static void Get(I* pit, const char*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const char*)&(pit->mem[ptr]);
}};
template<typename I>class GS<I, int*>{public:static void Get(I* pit, int*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(int*)&(pit->mem[ptr]);
}};
template<typename I>class GS<I, const int*>{public:static void Get(I* pit, const int*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const int*)&(pit->mem[ptr]);
}};
template<typename I>class GS<I, double*>{public:static void Get(I* pit, double*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(double*)&(pit->mem[ptr]);
}};
template<typename I>class GS<I, const double*>{public:static void Get(I* pit, const double*& val, int offs)
{
	int ptr=pit->GetInt(pit->rsp-offs);
	val=(const double*)&(pit->mem[ptr]);
}};

template<typename I, typename T>class PS
{
public:
	static void Put(I* pit, T& val, int offs)
	{
		memcpy(&(pit->mem[pit->rsp-offs]),&val,T::size_of);
	}
};
template<typename I>class PS<I, float> {public:static void Put(I* pit, float & val, int offs){pit->PutFloat(pit->rsp-offs,val);}};
template<typename I>class PS<I, double>{public:static void Put(I* pit, double& val, int offs){pit->PutFloat(pit->rsp-offs,val);}};
template<typename I>class PS<I, int>   {public:static void Put(I* pit, int   & val, int offs){pit->PutInt  (pit->rsp-offs,val);}};
template<typename I>class PS<I, char>  {public:static void Put(I* pit, char  & val, int offs){pit->PutChar (pit->rsp-offs,val);}};


template<typename R, typename ...As>class Wrapper
{
public:
	template<typename I>static int func(I* pit, void* userdata)
	{
		std::tuple<As...>args;
		int offs=TSSizeOf<R>::size;
		if constexpr(sizeof...(As)>0){offs+=TSSizeOf<typename std::tuple_element<0, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<0, std::tuple<As...> >::type>::Get(pit,std::get<0>(args),offs);}
		if constexpr(sizeof...(As)>1){offs+=TSSizeOf<typename std::tuple_element<1, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<1, std::tuple<As...> >::type>::Get(pit,std::get<1>(args),offs);}
		if constexpr(sizeof...(As)>2){offs+=TSSizeOf<typename std::tuple_element<2, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<2, std::tuple<As...> >::type>::Get(pit,std::get<2>(args),offs);}
		if constexpr(sizeof...(As)>3){offs+=TSSizeOf<typename std::tuple_element<3, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<3, std::tuple<As...> >::type>::Get(pit,std::get<3>(args),offs);}
		if constexpr(sizeof...(As)>4){offs+=TSSizeOf<typename std::tuple_element<4, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<4, std::tuple<As...> >::type>::Get(pit,std::get<4>(args),offs);}
		if constexpr(sizeof...(As)>5){offs+=TSSizeOf<typename std::tuple_element<5, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<5, std::tuple<As...> >::type>::Get(pit,std::get<5>(args),offs);}
		if constexpr(sizeof...(As)>6){offs+=TSSizeOf<typename std::tuple_element<6, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<6, std::tuple<As...> >::type>::Get(pit,std::get<6>(args),offs);}
		if constexpr(sizeof...(As)>7){offs+=TSSizeOf<typename std::tuple_element<7, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<7, std::tuple<As...> >::type>::Get(pit,std::get<7>(args),offs);}
		if constexpr(sizeof...(As)>8){offs+=TSSizeOf<typename std::tuple_element<8, std::tuple<As...> >::type>::size;GS<I,typename std::tuple_element<8, std::tuple<As...> >::type>::Get(pit,std::get<8>(args),offs);}

		R(*fptr)(As...)=(R(*)(As...))userdata;

		if constexpr(!std::is_same<R,void>())
		{
			R retval=std::apply(fptr,args);
			PS<I,R>::Put(pit,retval,TSSizeOf<R>::size);
		}
		else
		{
			std::apply(fptr,args);
		}
		return 0;
	}
};

template<typename T> struct TSType;
template<>           struct TSType<float>         {static const int type=Type::Float;};
template<>           struct TSType<double>        {static const int type=Type::Float;};
template<>           struct TSType<int>           {static const int type=Type::Int;};
template<>           struct TSType<void>          {static const int type=Type::Void;};
template<>           struct TSType<char>          {static const int type=Type::Char;};
template<>           struct TSType<char*>         {static const int type=Type::CharPtr;};
template<>           struct TSType<const char*>   {static const int type=Type::CharPtr;};
template<>           struct TSType<int*>          {static const int type=Type::IntPtr;};
template<>           struct TSType<const int*>    {static const int type=Type::IntPtr;};
template<>           struct TSType<double*>       {static const int type=Type::FloatPtr;};
template<>           struct TSType<const double*> {static const int type=Type::FloatPtr;};

template<typename C> class Binder
{
public:
	template<typename R, typename... As>static void Bind(C& ctx, const char* name, R(*function)(As...))
	{
		std::vector<long>argTypes;
		if constexpr(sizeof...(As)>0){argTypes.push_back(TSType<typename std::tuple_element<0, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>1){argTypes.push_back(TSType<typename std::tuple_element<1, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>2){argTypes.push_back(TSType<typename std::tuple_element<2, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>3){argTypes.push_back(TSType<typename std::tuple_element<3, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>4){argTypes.push_back(TSType<typename std::tuple_element<4, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>5){argTypes.push_back(TSType<typename std::tuple_element<5, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>6){argTypes.push_back(TSType<typename std::tuple_element<6, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>7){argTypes.push_back(TSType<typename std::tuple_element<7, std::tuple<As...> >::type>::type);}
		if constexpr(sizeof...(As)>8){argTypes.push_back(TSType<typename std::tuple_element<8, std::tuple<As...> >::type>::type);}

		AddFunc(ctx, TSType<R>::type, "", name, argTypes,&Wrapper<R,As...>::func,{},(void*)function);
	}
};

template<typename I, typename R, typename... As>int ScincBoundCall(I*pit, R(*function)(As...))
{
	Wrapper<R,As...>::func(pit,(void*)function);
	return 1;
}


#endif //SCRIP_SCINCBINDER_H
