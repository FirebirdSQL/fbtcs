#include <stdlib.h>
#include <cstdio>
//
// wrapper platform independent utility to compare two files
//
int main(int argc, char* argv[]){
	if (argc<3){
		printf("Compare file needs two arguments\n");
		fflush(stdout);
		exit(1);
	}
//	printf("arg1 %s\n",argv[1]);
//	printf("arg2 %s\n",argv[2]);

	FILE* file1;
	FILE* file2;
	file1 = fopen(argv[1], "rb");
	file2 = fopen(argv[2], "rb");

	if (!file1 || !file2){
		printf("Can't open files\n");
		fflush(stdout);
		exit(1);
	}
	int count = 1;
	char c = getc(file1);
	char d = getc(file2);
	
	while (c == d){
		count++;
		c = getc(file1);
		d = getc(file2);
	}
	if (c != EOF || d != EOF){
		if (c != EOF && d != EOF)
			printf("Binary files are different in byte %d\n", count);
		else if (c != EOF)
			printf("File %s have only %d bytes\n", argv[2], count-1);
		else
			printf("File %s have only %d bytes\n", argv[1], count-1);
	}
	else
		printf("Binary files are equal\n");
	fflush(stdout);

	fclose(file1);
	fclose(file2);

	return 0;
}
