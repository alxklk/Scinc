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

template <int N>struct __native__
{
	static const int size_of=N;
	char content[N];
};
