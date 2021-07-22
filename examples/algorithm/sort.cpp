#include <stdio.h>
#include "graphics.h"

Graph g;

#define N 100

char* message;

void DrawArray(int* arr, int len, int highlight)
{
	float s=600./len;
	float h=460./len;
	g.rgb(.4,.1,.0);
	g.clear();
	g.FillRT();
	g.width(s/2,s/2-1);
	for(int l=0;l<len;l++)
	{
		g.clear();
		g.M(l*s+20,470);
		g.l(0,-arr[l]*h);
		g.fin();
		if(l==highlight)
			g.rgb(0,1,1);
		else
			g.rgb(.2,.6,0);
		g.stroke();
	}
	g.t_0(20,40);
	g.t_x(10,0);
	g.t_y(2,-15);
	g.clear();
	gtext(message);
	g.fin();
	g.width(5,5);
	g.rgb(1.0,.5,0.0);
	g.stroke();
	g.width(2,2);
	g.rgb(1,1,1);
	g.stroke();
	g.t_0(0,0);
	g.t_x(1,0);
	g.t_y(0,1);


	Present();
}

void BubbleSort(int* arr, int len)
{
	int n = len;
	while(n > 1)
	{
		int i = 0;
		while(i < n - 1)
		{
			if(arr[i]>arr[i+1])
			{
				int tmp = arr[i];
				arr[i] = arr[i+1];
				arr[i+1] = tmp;
				DrawArray(arr, len, i+1);
			}
			i += 1;
		}
		n -= 1;
	}
}

int iseed=3453245;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

void Mix(int* arr, int len)
{
	for(int i=0;i<2000;i++)
	{
		int n0=irand(iseed)%len;
		int n1=irand(iseed)%len;
		int tmp=arr[n0];
		arr[n0]=arr[n1];
		arr[n1]=tmp;
		if(i%20==0)DrawArray(arr, len, n0);
	}
}

void QuickSort(int*A, int i0, int j0)
{
	int len=j0-i0;
	if(len<2)
	{
		return;
	}
	int pivot=A[i0+len/2];
	int i=i0;
	int j=j0-1;
	for (;;)
	{
		while(A[i]<pivot)i++;
		while(A[j]>pivot)j--;
		if(i>=j)
			break;
		int temp = A[i];
		A[i]=A[j];
		A[j]=temp;
		i++;
		j--;
		DrawArray(A, N, i);
	}
	QuickSort(A, i0, i);
	QuickSort(A, i, j0);
}

void ShellSort(int *a, int n)
{
	int h;
	int i;
	int j;
	int t;
	for(h=n;h/=2;)
	{
		for(i=h;i<n;i++)
		{
			t=a[i];
			for(j=i;j>=h&&t<a[j-h];j-=h)
			{
				a[j]=a[j-h];
				DrawArray(a,n,j);
			}
			a[j]=t;
			DrawArray(a,n,j);
		}
	}
}

int main()
{
	message="Sorting algorithms";
	int testArray[N];
	for(int i=0;i<N/2;i++){testArray[i]=i;}
	message="Mixing array";
	Mix(testArray,N/2);
	message="Bubble sort";
	BubbleSort(testArray,N/2);
	message="Mixing array";
	for(int i=0;i<N;i++){testArray[i]=i;}
	Mix(testArray,N);
	message="Quick sort";
	QuickSort(testArray,0,N);
	message="Quick sort sorted";
	for(int i=0;i<N;i++){testArray[i]=i;}
	QuickSort(testArray,0,N);
	message="Quick sort reversed";
	for(int i=0;i<N;i++){testArray[i]=N-i-1;}
	QuickSort(testArray,0,N);
	for(int i=0;i<N;i++){testArray[i]=i;}
	message="Mixing array";
	Mix(testArray,N);
	message="Shell sort";
	ShellSort(testArray,N);
	message="Shell sort reversed";
	for(int i=0;i<N;i++){testArray[i]=N-i-1;}
	ShellSort(testArray,N);
	return 0;
}