#include <cstdio>
#include <string.h>
#include <stdlib.h>
#ifdef WIN_NT
#include <winsock2.h>
#else
#include <netdb.h>
#include <unistd.h>
#endif

#ifdef MINGW 
#include <io.h> // unlink
#endif

#ifdef SINIXZ
extern "C" {
extern int gethostname(char *name, size_t len);
}
#endif

void remove_local(char*);
void remove_remote(char*, char*);

//
// Utility to delete a file with url
//
int main(int argc, char* argv[]){

	char* arg;
	argv++; /* Skip command name */
	if (argc < 4)
	{
		printf("delete_file need at least 3 parameters\n");
		exit(1);
	}

	char* where_gdb = *argv++;
	char* where_external = *argv++;

	while ((arg = *argv++)!=NULL){

		char* file = (char*) malloc (strlen(where_external)+strlen(arg)+1);
		sprintf(file,"%s%s",where_external,arg);

		if (strcmp(where_gdb,where_external)==0){
			remove_local(file);
			continue;
		}

		char* machine_name = NULL;
		char* ptr1 = arg;
		// Windows format
		if (where_gdb[0]=='/' && where_gdb[0]=='/') {
			ptr1 = strchr(&where_gdb[2],'/');
			int size = ptr1 - &where_gdb[2];
			machine_name = (char*) malloc (size);
			strncpy(machine_name, &where_gdb[2], size);
			machine_name[size] = '\0';
		}
		// url format
		else if ((ptr1 = strchr(where_gdb,':')) != NULL){
			int size = ptr1 - where_gdb;
			machine_name = (char*) malloc (size);
			strncpy(machine_name, where_gdb, size);
			machine_name[size] = '\0';
		}
		
		//
		// if the machine name is localhost or 127.0.0.1 the host is this machine
		// 
		if (machine_name == NULL)
		{
			remove_local(file);
			continue;
		}

		if (strcmp(machine_name,"localhost")==0 || strcmp(machine_name,"127.0.0.1")==0)
		{
			remove_local(file);
			continue;
		}

		char host_name[256];

#if defined (WIN_NT)
		WSADATA p;
		WSAStartup ((2<<8) | 2, &p);
#endif
		int status = gethostname(host_name,256);
	//	printf("status %d\n", status);
		fflush(stdout);

		if (status){
			printf("Problem in gethostname\n");
			fflush(stdout);
		}

		if (strcmp(host_name,machine_name)==0)
		{
			remove_local(file);
			continue;
		}

		hostent * host = gethostbyname(host_name);
	//	printf("h_name %s\n",host->h_name);
		int pos = 0;
		while (true){
			if (host->h_aliases[pos]==NULL)
				break;
	//		printf("h_aliases %s\n",host->h_aliases[pos++]);
		}
	//	printf("h_addrtype %d\n",host->h_addrtype);
	//	printf("h_length %d\n",host->h_length);
		pos = 0;
		char addr[16];
		while (true){
			if (host->h_addr_list[pos]==NULL)
				break;
			sprintf(addr,"%d.%d.%d.%d",host->h_addr_list[pos][0] & 0xff,
				host->h_addr_list[pos][1] & 0xff, host->h_addr_list[pos][2]& 0xff,
				host->h_addr_list[pos][3] & 0xff);
			pos++;
	//		printf("ip direction %s\n", addr);
		}

		if (strcmp(addr,machine_name)==0)
		{
			remove_local(file);
			continue;
		}

		remove_remote(machine_name, ptr1+1);
	}
	return 0;
}

void remove_local(char* file){
//printf("Fichero %s\n", file);
//fflush(stdout);
	unlink(file);
//	printf("local host, Remove file %s\n",file);
}

void remove_remote(char* machine, char* file){
	printf("Remove from remote host %s file %s\n", machine, file);
}
