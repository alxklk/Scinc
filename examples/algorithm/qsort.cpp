#include <stdio.h>

#define N 100

int iseed=3453245;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

void Mix(int* arr)
{
	for(int i=0;i<1000;i++)
	{
		int n0=irand(iseed)%N;
		int n1=irand(iseed)%N;
		int tmp=arr[n0];
		arr[n0]=arr[n1];
		arr[n1]=tmp;
	}
}

void quicksort(int*A, int i0, int j0)
{
	int len=j0-i0;
	if (len < 2)
		return;
	int pivot = A[i0+len/2];
	int i=i0;
	int j=j0-1;
	for (;;)
	{
		while(A[i]<pivot)i++;
		while(A[j]>pivot)j--;
		if(i>=j)
			break;
		int temp = A[i];
		A[i] = A[j];
		A[j] = temp;
		i++;
		j--;
	}
	quicksort(A, i0, i);
	quicksort(A, i, j0);
}

int main()
{
	int testArray[N];
	for(int i=0;i<N;i++)
	{
		testArray[i]=i;
	}
	puts("Before");
	Mix(testArray);
	for(int i=0;i<N;i++)printf("%i ", testArray[i]);
	quicksort(testArray,0,N);
	puts("\nAfter");
	for(int i=0;i<N;i++)printf("%i ", testArray[i]);
	puts("\n-----");
	return 0;
}