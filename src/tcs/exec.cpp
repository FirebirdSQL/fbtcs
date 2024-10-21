/*
 *      PROGRAM:        Test Control System
 *      MODULE:         exec.e
 *      DESCRIPTION:    Run tests,Evaluate test results,provide runtime reports
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
 * $Id$
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN_NT
#include <winsock.h>
#else
#include <unistd.h>
#endif	// WIN_NT
#ifdef MINGW
#include<io.h>
#include<process.h>
#endif
#ifndef DARWIN
#include <ibase.h>
#else
#include <Firebird/ibase.h>
#endif
#include "tcs.h"
#include "do_diffs_proto.h"
#include "trns_proto.h"
#include "tcs_proto.h"

static const char* DAT_FILE		= "tcs.data%d";

#if defined(WIN_NT)
static const char* CMD_STRING	= "%s >%s 2>&1";
static const char* SCRIPT_FILE	= "tcscmd.bat";
#else
static const char* CMD_STRING	= "/bin/sh %s >%s 2>&1";
static const char* SCRIPT_FILE	= "tcs.script";
#endif

// External declarations
extern bool		sw_quiet;
extern bool		sw_save;
extern bool		sw_times;
extern bool		disk_io_error;
extern char*	gBaseDir;
extern char*	lBaseDir;
extern char*	oBaseDir;

// Static declarations

static int script_parse(char*, FILE*, ISC_USHORT*);
static int execute_script(char*, char*);
static int run_text(char*, char*, char*, U_LONG*, int*, char*);
static int compare_files(char*, char*, ISC_SHORT);
static TEST_RESULT compare_initialized_file(char*, char*, char*, ISC_SHORT);
static int next_line(FILE*, char*, ISC_USHORT);
static int set_dollar_verb(char*, char*);
//
//  A variety of utilities put unwanted messages in
//  unreasonable places.  This table tells us how many
//  lines to skip when we find a line put in by the linker
//  or one of its friends.
//
//  The mechanism is around in case other operating systems
//  decide to become more verbose.
//

/*
typedef struct pita
{
	char	*line;
	ISC_SHORT	skip;
}
*PITA;

static struct pita nuisances [] =
{

		{ "External file:", 1 },			// isql message Output uppercase and backslashes
		{ "shutdown", 1 },					// isql message Don't include host in name
		{ "", 0   }
};
*/

int EXEC_process_env(char *script, char *name)
{
/**************************************
 *
 *      E X E C _ e n v
 *
 **************************************
 *
 * Functional description
 *   dump the prologue of an environment
 *   to a file and call the system execution
 *   routine on it
 *
 **************************************/

	ISC_USHORT linetype;
	char* line_buf;

	// split lines
	char** lines = split_tokens(script, "\n\r");

	int pos = 0;
	// loop every line
	while ((line_buf = lines[pos++])!= NULL)
	{
		if (line_buf[0]=='v' && line_buf[1]=='e'){
			if (!(linetype = set_dollar_verb(line_buf + 3, NULL))) {
				return false;
			}
		}
		else if (line_buf[0]='t' && line_buf[1]=='e'){
			if (!(linetype = PTSL_symbol(line_buf + 3))) {
				return false;
			}
		}
	}

	return true;
}

static int set_dollar_verb(char *verb, char *def)
{
	/**************************************
	 *
	 *      s e t _ d o l l a r _ v e r b
	 *
	 **************************************
	 *
	 * Functional description
	 *      Call a function (set_ptl_lookup) in trns.c to
	 *      change the ptl_lookup table.
	 *
	 **************************************/
	char*	ptr;
	char*	value;

	if ( ptr = strchr(verb, '=') )
	{
		*ptr = '\0';
		value = strdup(ptr + 1);
	} else
		value = strdup(def);

	if ( set_ptl_lookup(verb, value) )
		printf("\tEnvironment verb: (%s) = (%s)\n", verb, value);
	else
		printf("\tFailed to Set\t%s\n", verb);

	if (ptr)
		*ptr = '=';
	return true;
}


TEST_RESULT EXEC_test(char *test_name,
						char *script,
						int *file_count,
						char* boilerplate_name)
{
/**************************************
 *
 *      E X E C _ t e s t
 *
 **************************************
 *
 * Functional description
 *    drive the execution of a test
 *    Return the result of running the test
 *
 **************************************/
	TEST_RESULT		result;
	U_LONG			run_time;
	char			output_file_name[128];

	sprintf(output_file_name, "%s/%s.out", oBaseDir, test_name);

	result = passed;

	time_t now = time(0);
	struct tm *t = localtime(&now);

	//  If phase == 0 (it will always unless '-n' no system flag is
	//  set) then run the test, by calling run().

	fprintf (stdout, "%02d:%02d:%02d\ttest %25s\t%s",
					t->tm_hour, t->tm_min, t->tm_sec, test_name,
					test_name[-1] == 'l' ? "local" : "global");
	fflush (stdout);

	bool test_result = run_text(script, const_cast<char*>(SCRIPT_FILE),
								output_file_name, &run_time, file_count,
								boilerplate_name);

	if (!test_result){
		printf ("\t*** failed **** %ld s\n", run_time);
		return test_system_failed;
	}

	//  If the ignore initialization flag is not set then compare the
	//  output with the the initialization.

	ISC_USHORT count = 0;
	char test_output_name[128];
	char compare_file_name[128];
	FILE * pFile;
	FILE * compare_file;
	char * buffer;

	sprintf(compare_file_name,"%s/%s.cmp",oBaseDir,test_name);
	//
	// Try global then local
	//
	int best_location = 0;
	bool knownBug = true;
	sprintf(test_output_name, "%s/tests/%s.knownbug", lBaseDir, test_name);
	pFile = fopen(test_output_name, "rb");
	if (pFile == NULL){
		best_location = 0;
		knownBug = false;
		sprintf(test_output_name, "%s/tests/%s.output", lBaseDir, test_name);
		pFile = fopen(test_output_name, "rb");
	}
	if (pFile == NULL){
		best_location = 1;
		knownBug = true;
		sprintf(test_output_name, "%s/tests/%s.knownbug", gBaseDir, test_name);
		pFile = fopen(test_output_name, "rb");
	}
	if (pFile == NULL){
		best_location = 1;
		knownBug = false;
		sprintf(test_output_name, "%s/tests/%s.output", gBaseDir, test_name);
		pFile = fopen(test_output_name, "rb");
	}

	if (pFile != NULL){
		count++;
		buffer = read_file(pFile, false);
		// Process output with handle_where_gdb
		buffer = handle_where_gdb(buffer);
		//
		compare_file = fopen(compare_file_name, "wb");
		if (compare_file == NULL)
		{
			printf("internal error file %s not found\n", compare_file_name);
			fflush(stdout);
			return 1;
		}
		fwrite(buffer,1,strlen(buffer), compare_file);
		fclose(compare_file);
		free(buffer);
	}

	if (count){
		result = compare_initialized_file(compare_file_name, output_file_name,
										  test_name, best_location);
		if (result == failed)
			printf ("\t*FAIL*");
		else
			printf ("\tpass");

		if (best_location==0)
			printf ("\tlocal ");
		else
			printf ("\tglobal");

		if (knownBug)
			printf(" KB %.3lds\n", run_time);
		else
			printf("    %.3lds\n", run_time);
	}
	else
		printf ("\tNon reference output\n");


	// If the test did not fail and the timing flag is set '-t' then
	// store the timing information.
	return (result);
}

static int compare_files(char *compare_file, char *output_file, ISC_SHORT global)
{
	/**************************************
	 *
	 *      c o m p a r e
	 *
	 **************************************
	 *
	 * Functional description
	 *      Do a quick compare of two files.
	 *      Return identical (true) or difference
	 *      (false).
	 *      I have changed this to compare
	 *      two files instead of a blob
	 *      and a file to be able to use
	 *      blobs with any segment length
	 *      FSG 12.Nov.2000
	 **************************************/
	FILE	*file,*blob_file;
	char	f_buff [2048], b_buff [2048], *p, *q, c, lastc;
	ISC_USHORT	eof_blob, eof_file;

	//  Check to make sure an output file exists.

	if (!(file = fopen (output_file, "r")))
		return false;

	if (!(blob_file = fopen (compare_file, "r")))
		return false;

	do {
		eof_blob = next_line (blob_file, b_buff, sizeof (b_buff));
		eof_file = next_line (file, f_buff, sizeof (f_buff));
		if (eof_blob || eof_file)
			break;


		//  Map multiple white space characters to single spaces
		p = q = f_buff;
		lastc = 0;

		while (c = *p++)
		{
			if (c == '\t')
				c = ' ';
			if (c != ' ' || lastc != ' ')
				lastc = *q++ = c;
		}

		if (q != f_buff && *q == '\n')
			q--;
		if (q != f_buff && *q == ' ')
			q--;
		*q = 0;

		//  Map multiple white space characters to single spaces
		p = q = b_buff;
		lastc = 0;

		while (c = *p++)
		{
			if (c == '\t')
				c = ' ';
			if (c != ' ' || lastc != ' ')
				lastc = *q++ = c;
		}

		if (q != b_buff && *q == '\n')
			q--;
		if (q != b_buff && *q == ' ')
			q--;
		*q = 0;

		//  Scan the lines for a difference
		for (p = f_buff, q = b_buff; *p && *p == *q; p++, q++)
			continue;

		if (*p != *q)
			break;
	} while (*p == *q);

	fclose (file);
	fclose (blob_file);
	return (eof_blob && eof_file) ? true : false;
}

static TEST_RESULT compare_initialized_file(char *compare_file,
											char *output_file,
											char *test_name,
											ISC_SHORT global)
{
	/**************************************
	 *
	 *      c o m p a r e _ i n i t i a l i z e d
	 *
	 **************************************
	 *
	 * Functional description
	 *    When an initialization record exists.
	 *    use the stored results for a comparison
	 *
	 **************************************/
	ISC_USHORT			result;
	TEST_RESULT		test_result;
	char			diff_file[128];

	sprintf(diff_file,"%s/%s.diff", oBaseDir, test_name);
	test_result = passed;

	//  Check to see if the test passed.  If it didn't then store the
	//  diff of the expected result with the actual result in the DB
	//  and compare it with the test output
	if (!compare_files(compare_file, output_file, global))
	{
		test_result = failed;
		//  Call do_diffs which is linked in at compile time, in order to
		//  do the actual diff.  Diff the expected result(compare_file)
		//  with the actual result(output_file) and put the diff in the
		//  diff_file.
		result = do_diffs (compare_file, output_file, diff_file, 0, 0, 0);
		// if there is a difference always preserve the files
	}
	else {
		// if save is not set remove compare and output
		if (!sw_save) {
			unlink(compare_file);
			unlink(output_file);
		}
	}
	return (test_result);
}

static int execute_script(char *script_file, char *output_file)
{
	/**************************************
	 *
	 *      e x e c u t e _ s c r i p t
	 *
	 **************************************
	 *
	 * Functional description
	 *        run a command file with attendant
	 *        data files.
	 **************************************/
	char	buffer [512];

	//  Move in appropriate execute string to buffer and then system()
	//  as appropriate.
	sprintf(buffer, CMD_STRING, script_file, output_file);

#ifdef WIN_NT
	fflush(NULL);
#endif

	system(buffer);

	return true;
}


static int next_line (FILE *file, char *buffer, ISC_USHORT buff_len)
{
	/**************************************
	 *
	 *      n e x t _ l i n e
	 *
	 **************************************
	 *
	 * Functional description
	 *      Get the line from a file  and
	 *      return it as a null terminated string.
	 *      Skip uninteresting lines.  Return true
	 *      at end of file.
	 *
	 **************************************/
	int		eof;
	ISC_USHORT	skip = 0;

	do {
		eof = ! fgets (buffer, buff_len, file);
	} while (!eof && skip--);

	return feof(file);

}


static int run_text(char *script,
					char *script_file_name,
					char *output_file_name,
					U_LONG *run_time,
					int *file_count,
					char* boilerplate_name)
{
	/**************************************
	 *
	 *      r u n
	 *
	 **************************************
	 *
	 * Functional description
	 *      Map a script into a set of script files and run them.
	 *
	 **************************************/
	FILE	*script_file;
	ISC_USHORT	success, n;
	char	data_file [15];

	n = 0;

	//  Open script file

	if (!(script_file = fopen (script_file_name, "w")))
	{
		print_error("Can't create file \"%s\"\n", script_file_name,0,0);
		disk_io_error = true;
		return false;
	}

	//  Dump boilerplate, if any, into script file
	char lBPList[128];
	sprintf(lBPList,"%s/%s/%s",lBaseDir,"boiler_plates",boilerplate_name);
	FILE* pFile = fopen(lBPList,"rb");
	char *buffer;

	if (pFile != NULL){
		buffer = read_file(pFile, true);
		buffer = handle_where_gdb(buffer);
		if (fprintf(script_file,"%s\n",buffer) == EOF) {
			print_error("IO error on write to \"%s\"\n", script_file_name,0,0);
			fclose(script_file);
			disk_io_error = true;
			return false;
		};
		free(buffer);
	}

	// Copy script to script file, counting data files as created.

	success = script_parse(script, script_file, &n);

	fclose(script_file);

	// If we failed at creating script file return false

	if (!success)
		return false;

	success = false;

	//  Execute the script...
	const time_t clock = time(NULL);
	struct tm times1 = *localtime (&clock);

	success = execute_script(script_file_name, output_file_name);

	*run_time = get_elapsed_time(times1);
	//  Delete unwanted data files, if the '-s' save files switch is
	//  not thrown.

	if (!sw_save)
	{
		while (n)
		{
			sprintf(data_file, DAT_FILE, --n);
			unlink(data_file);
		}
		unlink(SCRIPT_FILE);
	}

	//  If executing the test was not successful, return false

	if (!success)
		return false;

	return true;
}


static int script_parse(char *script, FILE *script_file, ISC_USHORT *n)
{
	/**************************************
	 *
	 *      s c r i p t _ p a r s e _ t e x t
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 *      Script parsing routine
	 *      This returns true if datafiles were opened or false if none are opened.
	 *      Number of files opened is returned via n counter.
	 *      Datafile naming remains 0 based as to change it would change the results of
	 *      a number of already initializd tests
	 *      buffer lines to hand to ptsltrns/ptsl2ap
	 *      which necessitates some fancy \n handling in the
	 *      file building : we don't know whether we have a
	 *      data file to open or close until we read the next line
	 *
	 **************************************/
	FILE	*data = NULL;
	bool	command;
	bool	file_open;
	ISC_USHORT	cmdcount, count, linetype;
	char	*buff, *c, buffer [BUFSIZE], result[BUFSIZE], data_file [32];
	ISC_SHORT	ch;

	file_open = command = false;
	cmdcount = 0;

	//  Loop on every character until we get to the end of the file
	int pos = 0;

	while ((ch = script[pos++]) != 0)
	{
		count = BUFSIZE;
		buff = buffer;

		//  Loop on every character -- moving a line of text into buff
		while (ch!= 0)
		{

			//  If we hit a new line character or we max out on the buffer
			//  size, then stop moving char's into ch and exit the loop.

			if (((*buff++ = (char)ch) == '\n') || (!--count))
				break;
			ch = script[pos++];
		}

		//  Assign a NULL character to the end of the text in buff.
		*buff = 0;

		//  Call PTSL_2ap() to check for '$' verbs in the line and process
		//  them properly.  If not successful processing the line the
		//  close up shop and return false.
		if (!(linetype = PTSL_2ap(buffer, result))) {

			if (file_open == true)
				fclose (data);

			fclose (script_file);
			return false;
		}
//printf("linetype=%d \n%s", linetype, result);

		//  If we are processing a TCS environment variable definition,
		//  PTSL_2ap() already finished processing the line, so move to
		//  the next line.

		if (linetype == SYMDFN)
			continue;

		//  Copy script to script_file file, making data files where required.

		c = result;
		if (linetype == CMD)
		{
			while ((*c == ' ') || (*c == '\t') || (*c == '$'))
				c++;

			//  Since we are processing a '$' verb then we must be done dumping
			//  a data file, so if one is open close it.

			if (file_open)
			{
				fclose (data);
				file_open = false;
			}
/*
			else if (command)
			{
				if (putc ('\n',script_file) == EOF)
				{
					print_error("IO error on write to \"%s\"\n",
								const_cast<char*>(SCRIPT_FILE),0,0);
					if (file_open)
						fclose (data);

					fclose (script_file);
					disk_io_error = true;
					return false;
				}
			}
			command = true;			//  Set command flag to true...
*/
			cmdcount++;				//  Increment cmdcount...

			//  Throw the rest of the line in the file.

			while (*c != '\n')
			{
				if (putc ((unsigned char)*c++, script_file) == EOF) {
					print_error("IO error on write to \"%s\"\n",
								const_cast<char*>(SCRIPT_FILE),0,0);
					if (file_open)
						fclose (data);

					fclose (script_file);
					disk_io_error = true;
					return false;
				}
			}
			if (putc ('\n',script_file) == EOF)
			{
				print_error("IO error on write to \"%s\"\n",
							const_cast<char*>(SCRIPT_FILE),0,0);
				if (file_open)
					fclose (data);

				fclose (script_file);
				disk_io_error = true;
				return false;
			}
		}
		else if (linetype == INPUTFILE)
		{
			if (file_open)
			{
				fclose (data);
				file_open = false;
			}

			//  Throw the rest of the line in the file.
/*
			while (*c != '\n')
			{
				if (putc ((unsigned char)*c++, script_file) == EOF) {
					print_error("IO error on write to \"%s\"\n",
								const_cast<char*>(SCRIPT_FILE),0,0);
					if (file_open)
						fclose (data);

					fclose (script_file);
					disk_io_error = true;
					return false;
				}
			}
*/

			strcpy(data_file, c);
/*
			if (fprintf (script_file, "create_file > cf_test.sql <%s\n", data_file) == EOF)
			{
				print_error("IO error on write to \"%s\"\n",
							const_cast<char*>(SCRIPT_FILE),0,0);
				fclose (script_file);
				disk_io_error = true;
				return false;
			};
*/

			//  Try to open the data file with the appropriate number.
			if (!(data = fopen (data_file, "w")))
			{
				print_error ("Can't create file \"%s\"\n", data_file,0,0);
				fclose (script_file);
				disk_io_error = true;
				return false;
			}
/*
			if (fputs (c, data) == EOF)		//  Dump to the data file.
			{
				print_error ("IO error on write to \"%s\"\n", data_file,0,0);
				fclose (data);
				fclose (script_file);
				disk_io_error = true;
				return false;
			}

			command = false;	//  Set the command flag to false
*/
			file_open = true;	//  Set the file_open flag to true
		}

		//  If processing something besides a command or environment
		//  variable definition, i.e. plain text -- put in data file.

		else
		{
			if (file_open){
				if (fputs (c, data) == EOF) {
					print_error ("IO error on write to \"%s\"\n", data_file,0,0);
					fclose (data);
					fclose (script_file);
					disk_io_error = true;
					return false;
				}
			}
			else if (linetype != COMML && *c != '\n'){
					print_error("Regular text line outside input file\nline=\"%s\"\n",c,0,0);
					fclose(script_file);
					disk_io_error = true;
					return false;
			}

			//  If we just finished processing a command then we need to create
			//  and open a new data file, so do it.
/*
			if (command)
			{
				sprintf (data_file, DAT_FILE, (*n)++);
				if (fprintf (script_file, "<%s\n", data_file) == EOF)
				{
					print_error("IO error on write to \"%s\"\n",
								const_cast<char*>(SCRIPT_FILE),0,0);
					fclose (script_file);
					disk_io_error = true;
					return false;
				};

				//  Try to open the data file with the appropriate number.

				if (!(data = fopen (data_file, "w")))
				{
					print_error ("Can't create file \"%s\"\n", data_file,0,0);
					fclose (script_file);
					disk_io_error = true;
					return false;
				}

				if (fputs (c, data) == EOF)		//  Dump to the data file.
				{
					print_error ("IO error on write to \"%s\"\n", data_file,0,0);
					fclose (data);
					fclose (script_file);
					disk_io_error = true;
					return false;
				}
				command = false;	//  Set the command flag to false
				file_open = true;	//  Set the file_open flag to true
			}
			//  If we are processing a data file then dump into it.

			else if (cmdcount) {
				if (fputs (c, data) == EOF) {
					print_error ("IO error on write to \"%s\"\n", data_file,0,0);
					fclose (data);
					fclose (script_file);
					disk_io_error = true;
					return false;
				}
			}
*/
		}
	}

	//  Finished processing, so clean up.  If a command was the last
	//  thing we processed then put a new line at the end.  If we
	//  just finished with a data file then close it.
/*
	if (command)
		if (putc ('\n', script_file) == EOF) {
			print_error("IO error on write to \"%s\"\n",
						const_cast<char*>(SCRIPT_FILE),0,0);
			if (file_open)
				fclose (data);

			fclose (script_file);
			disk_io_error = true;
			return false;
		}
*/
	if (file_open)
		fclose (data);

	return cmdcount;
}

