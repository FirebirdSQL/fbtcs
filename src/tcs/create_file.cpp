#include <cstdio>
//
// Stupid program to create stdout from stdin

int main(int argc, char* argv[]){
	char c;
	while ((c = getc(stdin)) != EOF){
		putc(c, stdout);
	}
	return 0;
}

