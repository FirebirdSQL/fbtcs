#include <stdio.h>
#include <stdlib.h>
#ifdef WIN_NT
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(int argc, char *argv[]) 
{

	if (argc < 2){
		printf("sleep_sec needs one parameter");
		fflush(stdout);
	}

	long seconds = atoi(argv[1]);
#ifdef WIN_NT
	Sleep(seconds*1000);
#else
	sleep(seconds);
#endif
	return 0;
}
