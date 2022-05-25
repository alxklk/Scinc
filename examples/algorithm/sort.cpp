#include <stdio.h>
#include "graphics.h"

Graph g;

#define N 50

char* message;

int nswaps;
int ncomps;

void DrawArray(int* arr, int len, int highlight, int highlight1, bool comp)
{
	float s=600./len;
	float h=400./len;
	g.rgb(.2,.3,.1);
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
		{
			if(comp)
				g.rgb(1,1,1);
			else
				g.rgb(0,0,1);
		}
		else if(l==highlight1)
		{
			if(comp)
				g.rgb(0,0,0);
			else
				g.rgb(1,0,1);
		}
		else
		{
			float f=arr[l]/float(len);
			g.rgb(f,1-f*.5,1-f);
		}
		g.stroke();
	}
	g.t_0(20,25);
	g.t_x(7,0);
	g.t_y(2,-8);
	g.clear();
	gtext(message);
	char ss[64];
	snprintf(ss,64,"%i swaps", nswaps);

	g.t_0(20,45);
	g.t_x(4,0);
	g.t_y(0,-5);

	gtext(ss);

	snprintf(ss,64,"%i comares", ncomps);

	g.t_0(20,60);

	gtext(ss);

	g.fin();
	g.width(3,3);
	g.rgb(1.0,.5,0.0);
	g.stroke();
	g.width(1.,1.5);
	g.rgb(1,1,1);
	g.stroke();
	g.t_0(0,0);
	g.t_x(1,0);
	g.t_y(0,1);


	Present();
}

void StrangeSort(int* a, int n)
{
	for(int i=0;i<n;i++)
	{
		for(int j=i+1;j<n-1;j++)
		{
			ncomps++;
			DrawArray(a, n, i, j, true);
			if(a[i]<a[j])
			{
				int d=a[i];
				a[i]=a[j];
				a[j]=d;
				nswaps++;
				DrawArray(a, n, i, j, false);
				continue;
			}
			else
			{
				continue;
			}
		}
	}
}

void BubbleSort(int* arr, int len)
{
	int n = len;
	while(n > 1)
	{
		int i = 0;
		while(i < n - 1)
		{
			ncomps++;
			DrawArray(arr, len, i, i+1, true);
			if(arr[i]>arr[i+1])
			{
				int tmp = arr[i];
				arr[i] = arr[i+1];
				arr[i+1] = tmp;
				nswaps++;
				DrawArray(arr, len, i, i+1, false);
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
		if(i%20==0)DrawArray(arr, len, n0, n1, false);
	}
}

void QuickSort(int*A, int i0, int j0)
{
	int len=j0-i0;
	if(len<2)
	{
		return;
	}
	int ipivot=i0+len/2;
	int pivot=A[ipivot];
	int i=i0;
	int j=j0-1;
	for (;;)
	{
		while(A[i]<pivot){i++;ncomps++;DrawArray(A, N, i, ipivot, true);}
		while(A[j]>pivot){j--;ncomps++;DrawArray(A, N, ipivot, j, true);}
		if(i>=j)
			break;
		int temp = A[i];
		A[i]=A[j];
		A[j]=temp;
		nswaps++;
		i++;
		j--;
		DrawArray(A, N, i-1, j+1, false);
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
				DrawArray(a,n,i,j-h,true);
				ncomps++;				
				a[j]=a[j-h];
				nswaps++;
				DrawArray(a,n,j,j-h,false);
			}
			a[j]=t;
			nswaps++;
			DrawArray(a,n,j,i,false);
		}
	}
}

int main()
{
	message="Sorting algorithms";
	int testArray[N];

	nswaps=0;ncomps=0;
	message="Mixing array";
	for(int i=0;i<N/2;i++){testArray[i]=i;}
	Mix(testArray,N/2);
	message="Strange sort";
	StrangeSort(testArray,N/2);

	nswaps=0;ncomps=0;
	message="Mixing array";
	Mix(testArray,N/2);
	message="Bubble sort";
	BubbleSort(testArray,N/2);

	nswaps=0;ncomps=0;
	message="Mixing array";
	for(int i=0;i<N;i++){testArray[i]=i;}
	Mix(testArray,N);
	message="Quick sort";
	QuickSort(testArray,0,N);

	nswaps=0;ncomps=0;
	message="Quick sort sorted";
	for(int i=0;i<N;i++){testArray[i]=i;}
	QuickSort(testArray,0,N);

	nswaps=0;ncomps=0;
	message="Quick sort reversed";
	for(int i=0;i<N;i++){testArray[i]=N-i-1;}
	QuickSort(testArray,0,N);

	nswaps=0;ncomps=0;
	for(int i=0;i<N;i++){testArray[i]=i;}
	message="Mixing array";
	Mix(testArray,N);
	message="Shell sort";
	ShellSort(testArray,N);

	nswaps=0;ncomps=0;
	message="Shell sort reversed";
	for(int i=0;i<N;i++){testArray[i]=N-i-1;}
	ShellSort(testArray,N);

	return 0;
}