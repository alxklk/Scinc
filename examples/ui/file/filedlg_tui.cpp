#include "filedlg.h"
#include <unistd.h>

int main(int argc, pchar* argv)
{
	SItemList l;
	l.Init();
	chdir("/home/klk");
	char cwd[512];
	getcwd(cwd,512);
	listdir(cwd, l);
	for(int i=0;i<l.nItems;i++)
	{
		printf("%i %s\n", l.GetFlag(i), l.GetName(i));
	}
	return 0;
}
