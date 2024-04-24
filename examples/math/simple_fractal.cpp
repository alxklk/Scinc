#include <stdio.h>

int main()
{
	for(int i=0; i<24;i++)
	{
		float y = i / 12. - 1;
		for(int j=0;j<80;j++)
		{
			float x = j / 30. - 2;
			float x0 = x;
			float y0 = y;
			int iter = 0;
			while((iter < 11) && (x0 * x0 + y0 * y0 <= 4))
			{
				float x1 = (x0 * x0) - (y0 * y0) + x;
				float y1 = 2 * x0 * y0 + y;
				x0 = x1;
				y0 = y1;
				iter = iter + 1;
			}
			printf("%c"," .-:;+=xX$& "[iter]);
		}
		printf("\n");
	}
	return 0;
}