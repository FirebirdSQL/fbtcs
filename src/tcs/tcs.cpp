/*
 *	PROGRAM:	Test Control System
 *	MODULE:		tcs.e
 *	DESCRIPTION:	Main Module for test control system.
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

#include <ctype.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifdef WIN_NT
#include <winsock.h>
#endif	// WIN_NT
#ifdef MINGW
#include<io.h>
#endif

#ifndef JRD_IBASE_H
#ifndef DARWIN
#include <ibase.h>
#else
#include <Firebird/ibase.h>
#endif
#endif

#include "tcs.h"
#include "tcs_proto.h"
#include "trns_proto.h"
#include "exec_proto.h"

static const char* TCS_NAME		= "tcs";
static const char* TCS_CONFIG	= "tcs.config"
;
static const char* TCS_VERSION	= "V0.90   01-Oct-2003";

static struct test_results
{
	int	tr_test_count;
	int	tr_test_results[NUM_RESULTS];
} series_results;

static int parse_main_options(char* p);
static int parse_series_args(char* , SSHORT*, SSHORT*, bool*);

static int set_config(char*);
static void set_symbol(char*);

struct replacement{
	char* initial;
	char* final;
	int pos;
	replacement* next;
};
char* replace_text(char*, replacement*);
replacement* lookup_env_var(char*);

static int set_env(char* name);
static int set_boilerplate(char* string);
static int test_one(char* string);
static int test_series(char* string, char* start);
static int test_meta_series(char* string, char* start);

static int		ms_count,ms_sequence,s_count,s_sequence,file_count;

static char		environment_name [64];
static char		boilerplate_name [64];


struct symbol {
	char *name;
	char *value;
	symbol* next;
};

static symbol* first_symbol = NULL;
static symbol* last_symbol;

// Exported declarations

bool	sw_save = false;
bool	sw_quiet = false;
bool	disk_io_error = false;
char*	gBaseDir;
char*	lBaseDir;
char*	oBaseDir;

//

int main (int argc, char* argv[], char* envp[])
{
	/**************************************
	 *
	 *	m a i n
	 *
	 **************************************
	 *
	 * Functional description
	 *	Main line routine.  If there are arguments, interpret them,
	 *	otherwise get input from user.
	 *
	 **************************************/
	char	*p;

	// Skip over command
	argv++;
	char* type;
	char* set;
	// Find execution instruction
	if (argc<3) {
		printf("The type and name of test set to run are needed\n");
		exit(1);
	}
	else {
		argc -= 2;
		type = *argv++;
		set = *argv++;
	}

	// handle switches in the 2nd best fashion (this should be a function
	// and jas has a better way to keep count of switches)

	while (--argc > 0)
	{
		p = *argv++;
		if (*p++ != '-')
			continue;

		// Parse the options...

		switch (UPPER (*p)) {

			// Do options that take an argument here

			// Pass off options that do not take an arg to parse_main_options()

			case 'Q':	// Quiet, suppress diff
			case 'S':	// Save script
			case 'X':	// Show TCS command line options
			default:
				parse_main_options( p );
		}
	}
	char config_file[128];
	sprintf(config_file, TCS_CONFIG);
	// Read config file
	set_config(config_file);

	printf ("\n\tWelcome to FBTCS:\t%s\n", TCS_VERSION);
	printf ("Test type %s name %s\n", type, set);

	if (strcmp(type,"rms")==0){
		test_meta_series(set, NULL);
	}
	else if (strcmp(type,"rs")==0){
		test_series(set, NULL);
	}
	else if (strcmp(type,"rt")==0){
		test_one(set);
	}
	else {
		printf("Not known run type %s\n",type);
		exit(1);
	}

	// Done, so shutdown and exit

	exit (0);
	// make compiler happy
	return 0;
}

char* handle_sys_env(char *in_line)
{
	/**************************************
	 *
	 *      h a n d l e _ s y s _ e n v
	 *
	 **************************************
	 *
	 * Functional description
	 *      takes an string and looks for the pattern $(TCS:<token>).
	 *      If it finds such a pattern then it looks for <token>
	 *      in the system environment and does substitution,
	 *      shifting the string right or left if necessary. It
	 *      assumes that the string is big enough to allow any
	 *      necessary shift.
	 *
	 **************************************/
	replacement* first_rep = lookup_env_var(in_line);
	if (first_rep != NULL)
		return replace_text(in_line, first_rep);
	else
		return in_line;
}

replacement* lookup_env_var(char* in_line)
{
	/**************************************
	 *
	 *      l o o k u p _ e n v _ v a r
	 *
	 **************************************
	 *
	 * Functional description
	 *      takes an string and looks for the pattern $(TCS:<token>).
	 *      returns a linked list of patterns to replace
	 *
	 **************************************/
	char* start;
	char* end;
	char* p = in_line;

	replacement* first_rep = NULL;
	replacement* last_rep;

	while (NULL != (start = strstr(p, "$(TCS:"))) {

		// Go to the end of $(TCS: reference
		for (end = start;; end++) {
			if (*end == ' ' || *end == '\t' || *end == '\n' ||
			        *end == '\0' || *end == ')')
				break;
		}

		// Check if we found a termination ')'
		if (*end != ')') {
			p = start+6; // Move p to search next $( 
			continue;
		}
	
		char* envvar = (char*) malloc(end - start + 2);
		strncpy(envvar, start, end - start + 1);
		envvar[end - start + 1] = '\0';

		replacement* new_rep = (replacement*) malloc (sizeof(replacement));
		new_rep->initial = envvar;

		char* envvar2 = (char*) malloc((end - 1) - (start+ 6) + 2);
		strncpy(envvar2, (start + 6), (end - 1) - (start + 6) + 1);
		envvar2[(end - 1) - (start + 6) + 1] = '\0';

		new_rep->final = getenv(envvar2);
		if (new_rep->final == NULL){
			printf("The enviroment variable %s have no value\n",envvar2);
			fflush(stdout);
			exit(1);
		}
		new_rep->next = NULL;
		new_rep->pos = start - in_line;
		
		if (!first_rep){
			first_rep = last_rep = new_rep;
		}
		else {
			last_rep->next = new_rep;
			last_rep = new_rep;
		}

		p = start+6;
	}
	return first_rep;
}

char* replace_text(char* origin, replacement* first_rep){
	/**************************************
	 *
	 *      l o o k u p _ e n v _ v a r
	 *
	 **************************************
	 *
	 * Functional description
	 *      Takes a linked list of replacements to do and a cstring
	 *      returns a new cstring with replacements did.
	 *
	 **************************************/
	int shift = 0;
	replacement* cur_rep = first_rep;
	while (cur_rep!=NULL){
		shift += strlen(cur_rep->final) - strlen(cur_rep->initial); 
		cur_rep = cur_rep->next;
	}

	char* result = (char*) malloc (strlen(origin) + shift +1);

	cur_rep = first_rep;
	int posInitial = 0;
	int posFinal = 0;
	while (cur_rep!=NULL){
		int skip = cur_rep->pos - posInitial;
		strncpy(&result[posFinal], &origin[posInitial], skip);
		posInitial += skip + strlen(cur_rep->initial);
		posFinal += skip;
		strncpy(&result[posFinal] , cur_rep->final, strlen(cur_rep->final));
		posFinal += strlen(cur_rep->final);
		cur_rep = cur_rep->next;
	}
	int finalSkip = strlen(origin) - posInitial;
	strncpy(&result[posFinal], &origin[posInitial], finalSkip);
	posFinal += finalSkip;
	result[posFinal] = '\0';
	return result;
}

replacement* lookup_symbols(char* origin){
	/**************************************
	 *
	 *      l o o k u p _ s y m b o l s
	 *
	 **************************************
	 *
	 * Functional description
	 *      Find the location dependent symbols in the script
	 *      and return a replacement list
	 *
	 **************************************/
	symbol* cur_symbol = first_symbol;
	char* ptr1 = NULL;
	replacement* first_rep = NULL;

	while (cur_symbol)
	{
		ptr1 = origin;
		while ((ptr1 = strstr(ptr1, cur_symbol->name)) != NULL)
		{
			replacement* new_rep = (replacement*) malloc(sizeof(replacement));
			// search for tranforms
			char* cont = ptr1 + strlen(cur_symbol->name);
			const char* upper = "(PLATFORM_UPPER)";
			if (strncmp(cont, upper, sizeof(upper)) == 0){
				new_rep->initial = (char*) malloc (strlen(cur_symbol->name) 
													+ strlen(upper) + 1);
				sprintf(new_rep->initial, "%s%s", cur_symbol->name, upper);
				new_rep->final = (char*) malloc (strlen(cur_symbol->value)+1);
				for (unsigned int pos=0; pos <= strlen(cur_symbol->value); pos++){
#ifdef WIN_NT
					if (cur_symbol->value[pos] == '/')
						new_rep->final[pos] = '\\';
					else
						new_rep->final[pos] = toupper(cur_symbol->value[pos]);
#else
					if (cur_symbol->value[pos] == '\\')
						new_rep->final[pos] = '/';
					else
						new_rep->final[pos] = cur_symbol->value[pos];
#endif
				}
			}
			else {
				new_rep->initial = cur_symbol->name;
				new_rep->final = cur_symbol->value;
			}
			new_rep->pos = ptr1 - origin;
			new_rep->next = NULL;
			if (!first_rep){
				first_rep = new_rep;
			}
			else {
				//
				// find the position
				//
				replacement* cur_rep = first_rep;
				replacement* next_rep = first_rep->next;
				int pos = new_rep->pos;
				if (pos < first_rep->pos) {
					new_rep->next = first_rep;
					first_rep = new_rep;
				}
				else {
					while (next_rep != NULL && pos > next_rep->pos) {
						cur_rep = next_rep;
						next_rep = next_rep->next;
					}
					// Put in the last position
					if (next_rep == NULL){
						cur_rep->next = new_rep;
					}
					else {
						cur_rep->next = new_rep;
						new_rep->next = next_rep;
					}
				}
			}
			ptr1 += strlen(cur_symbol->name);
		}
		cur_symbol = cur_symbol->next;
	}
	return first_rep;
}

char* handle_where_gdb(char *buffer)
{
	/**************************************
	 *
	 *	h a n d l e _ w h e r e _ g d b
	 *
	 **************************************
	 *
	 * Functional description
	 *	Look through the line for all instances of
	 *	WHERE_GDB, REMOTE_DIR, or WHERE_URL and insert the getenv()
	 *	of the word in its place, shifting right or left
	 *	if necessary.
	 *
	 **************************************/
	replacement* first_rep = lookup_symbols(buffer);
	if (first_rep != NULL){
		return replace_text(buffer, first_rep);
	}
	else
		return buffer;
}

static int parse_main_options(char *p)
{
	/**************************************
	 *
	 *	p a r s e _ m a i n _ o p t i o n s
	 *
	 **************************************
	 *
	 * Functional description
	 *	Parse command line switches with no arguments.
	 *
	 **************************************/

	// Scan until end of string of compacted options

	while ( *p != '\0' )
	{
		// operate on one option at a time

		switch (UPPER(*p))
		{
		case 'Q':
			sw_quiet = true;
			break;

		case 'S':
			sw_save = true;
			break;

		default:
			print_error ("unrecognized switch '-%c' ignored.", p,0,0);
		case 'X':
#if (defined WIN_NT)

			printf ("\nUsage:  tcs [ -d local ] [ -g global ] [ -i ] [ -m ] [ -n ] [ -q ] [ -t ]\n\t[ -z ] [ -x ]\n\n");
#else

			printf ("\nUsage:  tcs [ -d local ] [ -g global ] [ -c ] [ -i ] [ -n ] [ -q ] [ -t ]\n\t[ -z ] [ -x ]\n\n");
#endif

			printf ("\t-i\tIgnore initializations.  TCS will act like all tests are\n\t\tuninitialized.\n");
			printf ("\t\tNOTE:  No system calls are made, so TCS does not run the test.\n\t\tIf the user puts the output into a file \"tcs.output\" TCS will\n");
			printf ("\t\tcheck the output when it is restarted.\n");
			printf ("\t-q\tQuiet.  Suppress 'diff' output from failed tests.\n");
			printf ("\t-s\tSave the script files upon exit for the last test run by TCS.\n");
			printf ("\t-x\tPrint this description.\n");
			break;
		}
		p++;
	}
	return true;
}

static int parse_series_args (char *start, SSHORT *first, SSHORT *second, bool *break_flag )
{
	/**************************************
	 *
	 *	p a r s e _ s e r i e s _ a r g s 
	 *
	 **************************************
	 *
	 * Functional description
	 * 	Parse the args passed to the 'rs', or 'is'
	 *	commands.  (ie. rs <some series name> 1-3,5,7 )
	 *	Called by test_series.
	 *
	 **************************************/
	SSHORT range;
	char *previous;
	static char *current;

	// Is this the first time for this series?  If it is, set current
	// pointing to start.

	if (*break_flag == true)
		current = start;

	*break_flag = false;    // Set this to false so we don't break prematurely. 

	previous = current;
	*second = MAX_UPPER;    // Set the upper bound to pseudo - infinity.
	*first = range = 0;     // Set the lower bound and the range flag to zero.

	// Loop through the args, checking for a NULL in *current but
	// first making sure current points to something.

	while( current != NULL && *current != '\0' )
	{

		// Encountered comma and first is still zero, ( just finished
		// parsing a range that starts at zero, or we are just parsing
		// one test.

		if( *current == ',' && !(*first) )
		{
			*current++ = 0;

			// Ignore multiple commas...or junk chars

			if( !isdigit( *previous ) && !(*previous) )
			{
				previous = current;
				continue;
			}

			// If range starts at zero then assign value to second because
			// we must be done parsing the range since we hit a comma.  Else
			// assign value to both second and first for one test.

			if( range )
				*second = atoi( previous );
			else
				*second = *first = atoi( previous );

			break;
		}

		// Encountered comma and first has some value ( may be finished
		// parsing a range or about to start another token )

		else if( *current == ',' && *first)
		{

			// Assign the top of the range if the range is being processed

			if ( range )
				*second = atoi( previous );

			*current++ = 0;               // Zero out comma and increment c
			break;
		}

		// Encountered a dash, so we must be halfway through the range --
		// assign the bottom and move on.

		else if( *current == '-')
		{
			*current = 0;

			// Ignore an incomplete range that is not at the beginning or
			// end of a line of arguments... If previous is not holding a
			// number and current is not the first element of this token

			if( !isdigit( *previous ) && current != start )
			{

				// Skip the dash and set previous to the following char.
				previous = ++current;

				// Set first to the max which is in second so that when we return
				// to the calling function no test in the series will be called
				// NOTE:  This is for the case "rs c_exam 1,-3" -- where '-3'
				// should be ignored.
				if ( !(*first) )
					*first = *second;

				continue;
			}

			// If current == start then it is implied that the range starts
			// from zero.  (rs c_exam -5, this runs 0 through 5 )

			else if ( current == start )
			{
				*first = 0;
				previous = ++current; // Set previous to char after dash
				range = true;	// Set range flag
			}

			// Anything else must be the beginning of a range, so assign
			// first and set previous pointing to the char after the dash.

			else
			{
				*first = atoi( previous ); // Assign bottom of range.
				previous = (current+1);
				range = true;		// Set range flag to true.
			}
		}

		// Ignore garbage characters by moving previous past the garbage

		else if( !isdigit( *current ))
			previous = (current+1);

		current++;	// Increment current for the next iteration
	}

	// If current is NULL char then we are at the end of the line so
	// if first is zero then do appropriate thing for last arg

	if( current != NULL && *current == '\0' && !(*first))
	{

		// If previous is NULL and previous is not the first char then
		// last chars are garbage so ignore by setting break_flag so
		// will jump out of loop in calling function.

		if ( !(*previous) && previous != start )
			*break_flag = true;

		// If previous is NULL and previous is pointing to the starting
		// character then no args were given, so exit with first and
		// second set to max values -- causing all tests in series to be
		// run.

		else if( !(*previous) && previous == start )
			;

		// If we saw a dash ( the range flag is set ), then set second
		// to the value pointed to by previous ( the range must start
		// at zero )
		// NOTE:  This is the case:  "rs c_exam 3-" all tests greater
		// than or equal to 3 should be run.

		else if( range )
			*second = atoi( previous );

		// Just one test to run, only a number was given, so assign to
		// first and second.

		else
			*second = *first = atoi( previous );

		return false;
	}

	// If we are at the end of a line and first has a value ( we are
	// processing a range ) then finish processing by assigning the
	// top of a range.

	else if( current != NULL && *current == '\0' && *first)
	{
		if (( *previous != '\0' ) && ( range ))
			*second = atoi( previous );
		return false;
	}

	// If current is not pointing to an address, exit

	else if ( current == NULL )
		return false;

	return true;
}

void print_error(char *string, char *a, char *b, char *c)
{
	/**************************************
	 *
	 *      p r i n t _ e r r o r
	 *
	 **************************************
	 *
	 * Functional description
	 * 	Print an error that has occured
	 *	in a standard format.
	 *
	 **************************************/
	char buf[160];

	fflush(stdout);
	sprintf(buf, "\n**** %s:  %s\n", TCS_NAME, string);
	fprintf(stderr, buf, a, b, c);
	fflush(stderr);
}


static int set_boilerplate(char *string)
{
	/**************************************
	 *
	 *	s e t _ b o i l e r p l a t e
	 *
	 **************************************
	 *
	 * Functional description
	 *	Set the boilerplate name.
	 *
	 **************************************/

	// Search the local DB to see if the boilerplate with name ==
	// string exists.

	char lBPList[128];
	sprintf(lBPList,"%s/%s/%s",lBaseDir,"boiler_plates",string);
	FILE* fl = fopen(lBPList,"r");
	if (fl != NULL){
		fclose(fl);
		strcpy (boilerplate_name, string);
		return true;
	}

	// The boilerplate does not exist, so print an error.

	print_error ("%s is not a boilerplate. lb to list boilerplates",string,0,0);
	return false;

}


static int set_config(char *rfn)
{
	/*************************************
	 *
	 *	s e t _ c o n f i g
	 *
	 *************************************
	 *
	 * Functional description
	 *	Read SB, SE, SRN and SVR commands from
	 *	the file $HOME/.tcs_config if it exists.
	 *
	 ************************************/
	char*	line_buf;
	FILE*	rfp;
	USHORT	line_idx, start;
	char*	buffer;

	// Open the configuration file.

	if ((rfp = fopen(rfn,"rb")) == NULL){
		printf("No configuration file exists \"%s\"...%s\n", TCS_CONFIG, rfn);
		return false;
	}

	printf("Reading configuration file \"%s\"...\n", TCS_CONFIG);
	buffer = read_file(rfp, true);
	// apply handle_sys_env
	buffer = handle_sys_env(buffer);
	// Get lines
	char** lines = split_tokens(buffer, "\n\r");

	int pos = 0;
	while ((line_buf = lines[pos++])!= NULL)
	{
		line_idx = 0;

		// Move past preceding spaces/tabs if any...

		while ( line_buf[line_idx] == ' ' || line_buf[line_idx] == '\t' )
			line_idx++;

		// Is the current line a blank one?

		if (line_buf[line_idx] == '\n' || line_buf[line_idx] == '#')
			continue;

		start = line_idx;

		// Find out where the command modifier begins.

		if (line_buf[start+2] == ' ')
			line_idx += 3;
		else
			line_idx += 4;

		// Copy the command modifier from LINE_BUF to CMD.  Since fgets()
		// was used, LINE_BUF may not be NULL terminated.

/*
		*cmd = 0;

		// Loop through chars in a line
		USHORT cmd_idx;
		for (cmd_idx = 0; line_buf[line_idx] != '\0' && line_buf[line_idx] != '\n';
			line_idx++, cmd_idx )
		{
				cmd[cmd_idx++] = line_buf[line_idx];
			// Ignore spaces and then assign modifier.

			if (( line_buf[line_idx] != ' ' ) && ( line_buf[line_idx] != '\t' ))
				cmd[cmd_idx++] = line_buf[line_idx];

			// Ignore any tokens following second token.

			else if ( *cmd != '\0' &&
					( line_buf[line_idx] == ' ' || line_buf[line_idx] == '\t' ))
			{
				break;
			}
		}

		cmd[cmd_idx] = 0;
*/

		// Use the 2nd character as a key to the configuration commands
		// just read.

		char *cmd = &line_buf[start+3];

		switch ( line_buf[start+1] ) {

			// sb -- set boilerplate

			case 'b' :
			case 'B' :
				if (set_boilerplate(cmd))
					printf("\tBoilerplate set to: \t%s\n",boilerplate_name);
				else
					print_error("Boilerplate assignment failed.",NULL,NULL,NULL);
				break;

			// se -- set environment

			case 'e' :
			case 'E' :
				if (set_env(cmd))
					printf("\tEnvironment set to: \t%s\n", environment_name);
				else
					print_error("Environment assignment failed.",NULL,NULL,NULL);
				break;

			case 'l' :
			case 'L' :
				lBaseDir = (char*) malloc (strlen(cmd)+1);
				strcpy(lBaseDir, cmd);
				printf("\tLocal tests directory set to: \t%s\n", lBaseDir);
				break;

			case 'g' :
			case 'G' :
				gBaseDir = (char*) malloc (strlen(cmd)+1);
				strcpy(gBaseDir, cmd);
				printf("\tGlobal tests directory set to: \t%s\n", gBaseDir);
				break;

			case 'o' :
			case 'O' :
				oBaseDir = (char*) malloc (strlen(cmd)+1);
				strcpy(oBaseDir, cmd);
				printf("\tOutput directory set to: \t%s\n", oBaseDir);
				break;

			case 's' :
			case 'S' :
				set_symbol(cmd);
				break;
				
			// Unknown commands.

			default :
				print_error("Unintelligible line ignored:  %s",line_buf,0,0);
				break;
		}
	}

	//
	// Aditional symbols
	//
/*
	SLONG clock = time(NULL);
	struct tm times1 = *localtime(&clock);
	char date_yyyy_mm_dd[11];

	sprintf(date_yyyy_mm_dd, "DATE_YYYY-MM-DD=%.4d-%.2d-%.2d", times1.tm_year + 1900, times1.tm_mon + 1, times1.tm_mday);
	set_symbol(date_yyyy_mm_dd);
*/
	printf("\n");
	return true;

}

static void set_symbol(char* exp){
	/**************************************
	 *
	 *      s e t _ s y m b o l
	 *
	 **************************************
	 *
	 * Functional description
	 *      Add a symbol to the external symbol table
	 *
	 **************************************/
	char seps[] = "=";
	char *name;
	char *value;
	name = strtok(exp, seps);
	if (name)
		value = strtok(NULL, seps);
	else
		value = NULL;

	if (value){
		symbol* new_symbol = (symbol*) malloc(sizeof(symbol));
		new_symbol->name = (char*) malloc (strlen(name) + 2);
		sprintf(new_symbol->name, "%s:", name);
		new_symbol->value = (char*) malloc (strlen(value) + 1);
		strcpy(new_symbol->value, value);
		new_symbol->next = NULL;
		if (!first_symbol){
			first_symbol = last_symbol = new_symbol;
		}
		else {
			last_symbol->next = new_symbol;
			last_symbol = new_symbol;
		}
		printf("\tSymbol %s = %s\n", name, value);
	}
	else {
		printf("\tError in symbol definition\n");
	}
}

char** split_tokens(char* buffer, const char *seps){
	/**************************************
	 *
	 *      s p l i t _ t o k e n s
	 *
	 **************************************
	 *
	 * Functional description
	 *      Return a char** pointing to the tokens contained in the buffer
	 *
	 **************************************/

	char* buffer_copy = (char*) malloc(strlen(buffer) +1);
	memcpy(buffer_copy, buffer, strlen(buffer) + 1);

	int line_count = 0;
	char* line_buf = strtok(buffer_copy, seps);
	while (line_buf != NULL){
		line_count++;
		line_buf = strtok(NULL, seps);
	}

	char** lines = (char**) malloc(sizeof(void*)*line_count+1);
	memcpy(buffer_copy, buffer, strlen(buffer) + 1);

	int pos = 0;
	lines[pos++] = strtok(buffer_copy, seps);
	while (pos < line_count)
		lines[pos++] = strtok(NULL, seps);

	lines[pos] = NULL;
	return lines;
}

static int set_env(char *name)
{
	/**************************************
	 *
	 *	s e t _ e n v
	 *
	 **************************************
	 *
	 * Functional description
	 *	Try to execute the user specified environment
	 *      if that succeeds, set the env name.
	 *
	 **************************************/
	// Search local DB for environment with name
	char lEnvFile[128];
	sprintf(lEnvFile,"%s/%s/%s",lBaseDir,"environments",name);
	char* buffer;

	FILE* pFile = fopen(lEnvFile,"rb");
	if (pFile != NULL){
		buffer = read_file(pFile, true);
		buffer = handle_where_gdb(buffer);
		EXEC_process_env(buffer,name);
		free(buffer);
		strcpy (environment_name, name);
		return true;
	}

	// If could not find environment then print an error.

	print_error ("%s is not an environment name",name,0,0);
	return false;
}

char* read_file(FILE* pFile, bool addnl){
	/**************************************
	 *
	 *	r e a d _ f i l e
	 *
	 **************************************
	 *
	 * Functional description
	 *	Read a file and put its content in a char* 
	 *
	 **************************************/
	long lSize;

	fseek (pFile, 0 , SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	int bufsiz = lSize+1;
	if (addnl)
		bufsiz++;

	char* buffer = (char*) malloc(bufsiz);
	if (buffer == NULL)
		exit (2);

	fread(buffer,1,lSize,pFile);
	if (addnl)
		buffer[bufsiz-2] = '\n';
	buffer[bufsiz-1] = '\0';

	fclose(pFile);
	//
	// Replace WIN_NT \r
	//
	int offset = 0;
	int pos = 0;

	while (pos < (bufsiz-1) - offset)
	{
		if (buffer[pos+offset] == 0x0D)
			offset++;
		else {
			buffer[pos] = buffer[pos+offset];
			pos++;
		}
	}
	buffer[pos] = '\0';

	return buffer;
}

static int test_one(char *string)
{
	/**************************************
	 *
	 *	t e s t
	 *
	 **************************************
	 *
	 * Functional description
	 *	Compare the results of a script to the "correct" output.
	 *
	 **************************************/
	USHORT		count;
	TEST_RESULT	result;
	char		test_name[33];
	struct tm	times;

	series_results.tr_test_count++;
	count =  0;

	// Get current time.
	const time_t clock = time (NULL);
	times = *localtime (&clock);

	// The first byte of test_name is used to store a symbol to
	// determine which database the test is in, l=local, g=global.
	// So advance test_name one byte, copy string into test_name and
	// set test_name[-1] = 'l'.
	*test_name = 'l';
	strcpy(test_name+1, string);

	// Execute the main file -- search for the test with name == 
	// string
	char testFileName[128];

	sprintf(testFileName,"%s/tests/%s.script",lBaseDir,test_name+1);
	FILE * pFile;
	char * buffer;
	pFile = fopen(testFileName,"rb");
	if (pFile != NULL){
		buffer = read_file(pFile, true);
		buffer = handle_where_gdb(buffer);

		result = EXEC_test(test_name+1, buffer, &file_count, boilerplate_name);
		free (buffer);
		count++;
	}
	// If count is zero then test could not be found in the Local DB
	// so search the Global DB and try to run.

	if (!count) {
		*test_name = 'g';

		sprintf(testFileName,"%s/tests/%s.script",gBaseDir,test_name+1);
		FILE * pFile;
		char *buffer;
		pFile = fopen(testFileName,"rb");
		if (pFile!=NULL){
			buffer = read_file(pFile, true);
			buffer = handle_where_gdb(buffer);
			result = EXEC_test(test_name+1, buffer, &file_count, boilerplate_name);
			free (buffer);
			count++;
		}
	}

	// If count is not zero then test was executed

	if (count)
	{

		// If the test failed and the quiet switch is not thrown, then
		// print the differences between the failure and the expected
		// result.

		series_results.tr_test_results[result]++;
		return (result == passed) ? 0 : 1;
	}

	// If made it to here then test could not be found, so print error.
	else {
		print_error ("Test \"%s\" not found", string, NULL, NULL);

		series_results.tr_test_results[skipped_notfound]++;
		return 0;
	}
}

static int test_meta_series(char *string, char *start)
{
	/**************************************
	 *
	 *	t e s t _ m e t a _ s e r i e s
	 *
	 **************************************
	 *
	 * Functional description
	 *	Run through a list of series
	 *                          
	 **************************************/
	char gSeriesList[128];
	char lSeriesList[128];
	sprintf(gSeriesList,"%s/series/list",gBaseDir);
	sprintf(lSeriesList,"%s/series/list",lBaseDir);

	USHORT	total;
	SSHORT	first;

	// Set first to the appropriate value depending on if start is
	// NULL or not.  If NULL, set start to zero.

	first = (start != NULL && start[0] != '\0') ? atoi(start) : 0;

	first--;
	total = (ms_count < 0) ? -ms_count : 0;

	// If global variable ms_count is less than or equal to zero
	// check the local DB -- will normally be zero unless using
	// the '-n' command line option.

	char metaName[128];
	char serieName[128];
	int ret;
	sprintf(metaName,"%s",string);
	printf("metaseries %s\n",metaName);
	fflush(stdout);

	if (ms_count <= 0)
	{
		char lSerieList[128];
		// Get series local list 
		sprintf(lSerieList,"%s/meta_series/%s.list",lBaseDir,metaName);
		FILE* lTests = fopen(lSerieList,"r");
		int order;
		if (lTests != NULL){
			while (true){
				ret = fscanf(lTests,"%s\t%d\n",serieName,&order);
				if (ret!= 2)
					break;
				//
				// Try to run the test
				//
				if (serieName[0] != '#'){
					if (disk_io_error)
						break;
					total++;
					ms_count--;
					ms_sequence = order;
					test_series(serieName, NULL);
				}
			}
			fclose(lTests);
		}
	}

	// If total was not incremented, then meta series was not found
	// in the local DB so search the global DB.  If found, loop
	// and run each series in the meta series.


	if (!total)
	{
		total = ms_count;
		// Get series global list 
		char gSerieList[128];
		sprintf(gSerieList,"%s/meta_series/%s.list",gBaseDir,metaName);
		FILE* gTests = fopen(gSerieList,"r");
		int order;
		if (gTests != NULL){
			while (true){
				ret = fscanf(gTests,"%s\t%d\n",serieName,&order);
				if (ret!= 2)
					break;
				//
				// Try to run the test
				//
				if (serieName[0] != '#'){
					if (disk_io_error)
						break;
					total++;
					ms_count++;
					ms_sequence = order;
					test_series(serieName, NULL);
				}
			}
			fclose(gTests);
		}
	}

	// If total has not been incremented, then the meta series could
	// not be found -- print an error.

	if (!total) {
		print_error("\tMetaseries %s isn't there", string,0,0);
		return false;
	}
	printf("end metaseries %s\n",metaName);
	fflush(stdout);
	return true;
}

static int test_series(char *string, char *start)
{
	/**************************************
	 *
	 *	t e s t _ s e r i e s
	 *
	 **************************************
	 *
	 * Functional description
	 *	Run a series of tests.
	 *
	 **************************************/
	char gSeriesList[128];
	char lSeriesList[128];
	sprintf(gSeriesList,"%s/series/list",gBaseDir);
	sprintf(lSeriesList,"%s/series/list",lBaseDir);

	USHORT	total, count;
	SSHORT	first, second;
	bool break_flag = true;
	int 	loop = true;

	struct 	tm times1;

	// This test will set up an AUDIT system for a test series.
	// Currently it deletes existing database files so a test
	// won't fail due to pre-existing test files.


	// Initialize the series results

	memset ((char *) &series_results, 0, sizeof (series_results));

	// Loop for each command line argument.

	while( loop )
	{
		// loop will be set by parse_series_args() which will determine
		// how long to keep parsing.

		loop = parse_series_args( start, &first, &second, &break_flag);

		// break_flag will be set appropriately within parse_series_args.

		if ( break_flag ){
			break;
		}

		// Increase bounds by one, so can use < and > without equal sign

		first--;
		second++;
		count = 0;

		// This is done for the '-n' TCS command line option.  s_count
		// is stored in the tcs.info file.

		total = (s_count < 0) ? -s_count : 0;

		// Get the current time to print out before running a series
		const time_t clock = time (NULL);
		times1 = *localtime (&clock);
		printf("\tseries %s started on %s", string, asctime(&times1));
		fflush (stdout);

		// Normally s_count will be zero, unless -n option is thrown on
		// TCS command line.  So look in local DB for series and loop
		// through series for each test that is greater than first and
		// less than second (determined by arguments passed in and parsed.)

		char serieName[128];
		char testName[128];
		int ret;
		sprintf(serieName,"%s",string);
		if (s_count <= 0)
		{
			char lSerieList[128];
			// Get series local list 
			sprintf(lSerieList,"%s/series/%s.list",lBaseDir,serieName);
			FILE* lTests = fopen(lSerieList,"r");
			int order;
			if (lTests != NULL){
				while (true){
					ret = fscanf(lTests,"%s\t%d\n",testName,&order);
					if (ret!= 2)
						break;
					//
					// Try to run the test
					//
					if (testName[0] != '#'){
						if (disk_io_error)
							break;
						total++;
						s_count--;
						s_sequence = order;
						count = test_one(testName);
	
						// If quiet switch is not thrown and the test was run by test_one()
						// print the differences.
	
// 					if (count && !sw_quiet)
// 						diff (string, NULL, NULL, NULL);

					}
				}
				fclose(lTests);
			}
		}

		// If total is zero then series was not found in local DB so seach
		// global DB for series and loop through series for each test that
		// is greater than first and less than second ( determined by
		// arguments passed in and parsed )

		if (!total)
		{
			total = s_count;
			// Get series local list 
			char gSerieList[128];
			sprintf(gSerieList,"%s/series/%s.list",gBaseDir,serieName);
			FILE* gTests = fopen(gSerieList,"r");
			int order;
			if (gTests != NULL){
				while (true){
					ret = fscanf(gTests,"%s\t%d\n",testName,&order);
					if (ret!= 2)
						break;
					//
					// Try to run the test
					//
					if (testName[0] != '#'){
						if (disk_io_error)
							break;
						total++;
						s_count++;
						s_sequence = order;
						count = test_one(testName);

						// If quiet switch is not thrown and the test was run by test_one()
						// print the differences.

// 					if (count && !sw_quiet)
// 						diff (string, NULL, NULL, NULL);

					}
				}
				fclose(gTests);
			}
		}

		// If total is still zero here then series was not found in local
		// or global DBs so print error.

		if (!total)
			print_error("\tSeries %s isn't there.\n", string,0,0);
		printf("\tend Series");

		ULONG run_time = get_elapsed_time(times1);
		printf("\t%ld s", run_time);
		printf("\n");
	}

	return true;
}

ULONG get_elapsed_time(struct tm times1)
{
	const time_t clock = time(NULL);
	struct tm times2 = *localtime (&clock);
	ULONG run_time;

	//  handle test running through midnight, but bag multi day test

	if (times2.tm_mday != times1.tm_mday)
	{
		run_time = (60 - times1.tm_sec) + (60 - times1.tm_min) * 60
					+ (24 - times1.tm_hour) * 3600;
		run_time += times2.tm_sec + times2.tm_min * 60
					+ times2.tm_hour * 3600;
	}
	else
		run_time = (times2.tm_sec - times1.tm_sec) + (times2.tm_min - times1.tm_min) * 60
					+ (times2.tm_hour - times1.tm_hour) * 3600;
	return run_time;
}

#ifdef NO_STRDUP
char *strdup(text)
char	  *text;
{
	/**************************************
	 *
	 *	s t r d u p
	 *
	 **************************************
	 *
	 * Functional description
	 *	Allocates memory for a copy of text and
	 *	copys text into the new space.
	 *
	 **************************************/
	int		   len;
	char	  *ptr,
	*new;
	len = strlen(text) + 1;
	new = (char*)malloc(len * sizeof(char));
	strcpy(new, text);
	return ptr;
}
#endif
