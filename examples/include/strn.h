bool strneq(const char* l, const char* r, int n)
{
	for(int i=0;i<n;i++)
	{
		if(l[i]!=r[i])
		{
			return false;
		}
		else if(l[i]==0)
		{
			break;
		}
	}
	return true;
}

void strncp(char* l, const char* r, int n)
{
	for(int i=0;i<n;i++)
	{
		l[i]=r[i];
		if(r[i]==0)
		{
			break;
		}
	}
	l[n]=0;
}

void strncat(char* res, const char* left, const char* right, int n)
{
	const char* src=left;
	int srci=0;
	bool first=true;
	int i=0;
	while(true)
	{
		if(i==n)
		{
			break;
		}
		if(src[srci]==0)
		{
			if(first)
			{
				first=false;
				src=right;
				srci=0;
			}
			else
			{
				break;
			}
		}
		res[i]=src[srci];
		i++;
		srci++;
	}
	res[i]=0;
}

int strnlen(const char* s, int n)
{
	for(int i=0;i<n;i++)
	{
		if(s[i]==0)
			return i;
	}
	return n;
}

char strnlast(const char* s, int n)
{
	if(s[0]==0)return s[0];

	for(int i=1;i<n;i++)
	{
		if(s[i]==0)
			return s[i-1];
	}
	return s[n-1];
}
