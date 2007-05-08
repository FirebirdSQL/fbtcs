/*
 *      PROGRAM:        Drop database
 *      MODULE:         drop.c
 *      DESCRIPTION:    Drop database
 *
 * The contents of this file are subject to the InterBase Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.
 *
 * You may obtain a copy of the License at http://www.Inprise.com/IPL.html.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.  The Original Code was created by Inprise
 * Corporation and its predecessors.
 *
 * Portions created by Inprise Corporation are Copyright (C) Inprise
 * Corporation. All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DARWIN
#include <ibase.h>
#else
#include <Firebird/ibase.h>
#endif

#ifdef WIN_NT
#include <windows.h>
#else
#include <unistd.h>
#endif

const char* VERSION	= "drop_gdb version 1.5";
const char* USER	= "SYSDBA";
const char* PASS	= "masterkey";

typedef char *DbName;

char machine[4];
int version;

bool verbose = false;
char fake_it = 0;
char username = 0;
char password = 0;

void get_version(void *format, const char *arg);

int main(int argc, char **argv, char **envp)
{
	int iCntr=0;
	int iDbIdx=0;

	char buffer[256];
	char* dpb;
	char* lpUsername;
	char* lpPassword;

	DbName arDatabases[25];   /* holds all database names */

	isc_db_handle db_handle = 0;

	ISC_STATUS_ARRAY status;

	short dpb_length=0;

	/* process all command line arguments and store the database names in arDatabases */
	for(argv++; *argv; argv++) {
		if (**argv == '-') {
			switch((*argv)[1]) {
			case 'u':  /* server username */
				argv++;
				lpUsername = *argv;
				username = 1;
				break;

			case 'p':  /* server password */
				argv++;
				lpPassword = *argv;
				password = 1;
				break;

			case 'v': /* turn on verbose mode */
				verbose = true;
				break;

			case 'f':  /* just detach from the database, don't drop it */
				fake_it = 1;
				break;

			case 'z':  /* print the current version */
				printf("\t%s\n", VERSION);
				break;

			case 'h':
			case '?':  /* display command line options */
				printf("\nUsage: drop_gdb [-upvfz] db1[,db2,...]\n");
				printf("\t-u specify a username (default sysdba)\n");
				printf("\t-p specify a password (default masterkey)\n");
				printf("\t-v verbose mode\n");
				printf("\t-f don't drop, just detach\n");
				printf("\t-z displays the current version\n");
				printf("\t-h displays this message\n\n");
				return 0;
				break;
			}
		} else {
			// catch the names of all databases here
			// and change backslashes to forward slashes
			char* ptr1;
			while (ptr1 = strchr(*argv,'\\')){
				*ptr1 = '/';
			}
			arDatabases[iDbIdx++] = *argv;
		}
	}

	/* initialize dpb and only insert a username and password if the user
	 * provided one on the command line
	 */
	dpb=NULL;
	if (username && password) {
		isc_expand_dpb(&dpb, &dpb_length, isc_dpb_user_name, lpUsername,
		               isc_dpb_password, lpPassword, NULL);
	} else {
		lpUsername = const_cast<char*>(USER);
		lpPassword = const_cast<char*>(PASS);
	}

	/* loop through all the database names */
	for(iCntr=0; iCntr < iDbIdx; iCntr++) {

		/* print the database name, if verbose */
		if (verbose){
			printf("%s:\n", arDatabases[iCntr]);
			fflush(stdout);
		}

		/* try to attach to the database, ignore file if fail */
		isc_attach_database(status, strlen(arDatabases[iCntr]), arDatabases[iCntr],
		                    &db_handle, dpb_length, dpb);

		if (status[1]) {
			fprintf(stdout, "ERROR: can't attach to '%s'\n", arDatabases[iCntr]);
			fflush(stdout);
			if (verbose)
				isc_print_status(status);
			continue;
		}

		/* extract the version into global variables */
		version = 0;
/*		isc_version(&db_handle, (void (*)())get_version, 0);
		if (version == 0) {
			printf("ERROR: no version information for '%s'\n", arDatabases[iCntr]);
			fflush(stdout);
			continue;
		}*/

		if (verbose){
			printf("\n");
			fflush(stdout);
		}

		if (verbose){
			sprintf(buffer, "#drop database '%s'", arDatabases[iCntr]);
			fflush(stdout);
		}
		int count = 0;
		while (true) {
			if (!fake_it) {
				if (verbose){
					printf("\t%s\n", buffer+1);
					fflush(stdout);
				}
				isc_drop_database(status, &db_handle);
			} else {
				if (verbose){
					printf("\t%s\n", buffer);
					fflush(stdout);
				}
				isc_detach_database(status, &db_handle);
			}
			if (status[1]) {
				if (count < 5) {
					count++;
#ifdef WIN_NT
					Sleep(2000);
#else
					sleep(2);
#endif
				}
				else {
					fprintf(stdout, "ERROR: can't drop database '%s' count %d\n",
						arDatabases[iCntr], count);
					fflush(stdout);
					isc_print_status(status);
					isc_detach_database(status, &db_handle);
					break;
				}
			}
			else 
				break;
		}
	}
	return 0;
}

void get_version(void *format, const char *arg)
/*
 * This funtion parses the version string information, and returns
 * machine (O.S) string and InterBase version number calculated from the
 * MAJOR and MINOR version numbers.
 */
{
const char* VERSION_STRING	= "version \"%[A-Z]-%c%c.%c";
const int VERSION_MAJOR		= 1;
const int VERSION_MINOR		= 2;

	char ch[6],
	*ptr;
	int  tmp;

	/* if verbose, print all the version info */
	if (verbose)
		printf("\t%s\n", arg);

	/* look to see if the line contains the access method */
	if ((ptr = strstr(arg, "access method")) && (ptr = strstr(ptr, "version")))
	{
		/* extract the vital version info from the string */
		if (sscanf(ptr, VERSION_STRING, machine,
		           &ch[0], &ch[VERSION_MAJOR], &ch[VERSION_MINOR]))
			version = (ch[VERSION_MAJOR] - '0') * 10 + (ch[VERSION_MINOR] - '0');
	}

	/* look to see if the line contains the remote interface */
	if ((ptr = strstr(arg, "remote interface")) && (ptr = strstr(ptr, "version")))
	{
		if (sscanf(ptr, VERSION_STRING, machine,
		           &ch[0], &ch[VERSION_MAJOR], &ch[VERSION_MINOR])) {
			tmp = (ch[VERSION_MAJOR] - '0') * 10 + (ch[VERSION_MINOR] - '0');

			/* use the lesser of the two versions */
			if (tmp < version)
				version = tmp;
		}
	}
}
