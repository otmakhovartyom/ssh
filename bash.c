#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	char *bash_argv[] = {"sh", NULL};

	int master = posix_openpt(O_RDWR | O_NOCTTY);
	
	if (master < 0)
	{
		perror("openpt");
		return 1;
	}

	if (grantpt(master))
	{
		
	}

	execvp("sh", bash_argv);

	return 0;
}