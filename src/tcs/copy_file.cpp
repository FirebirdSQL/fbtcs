#include <stdlib.h>
#include <cstdio>
//
// Platform independent utility to copy a file 
//
int main(int argc, char* argv[]){
	if (argc<3){
		printf("Copy file needs two arguments\n");
		fflush(stdout);
		exit(1);
	}

	FILE* file1;
	FILE* file2;
	file1 = fopen(argv[1], "rb");
	file2 = fopen(argv[2], "wb");

	if (!file1){
		printf("Can't open file %s\n",argv[1]);
		fflush(stdout);
		exit(1);
	}
/*
	else {
		printf("Abierto %s\n",argv[1]);
		fflush(stdout);
	}
*/

	if (!file2){
		printf("Can't open file %s\n",argv[2]);
		fflush(stdout);
		exit(1);
	}
/*
	else{
		printf("Abierto %s\n",argv[2]);
		fflush(stdout);
	}
*/

	char c;
	int count=0;
	while (true){
		c = fgetc(file1);
		if (feof(file1))
			break;
		fputc(c,file2);
		count++;
	}
//	printf("ferror %d\n", ferror(file1));
//	printf("feof   %d\n", feof(file1));

//	printf("Copiados %d bytes\n", count);
	fflush(stdout);

	fclose(file1);
	fflush(file2);
	fclose(file2);

	return 0;
}

