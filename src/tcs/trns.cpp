/*
 *      PROGRAM:        Test Control System
 *      MODULE:         trns.c
 *      DESCRIPTION:
 *
 *
 *      Note: Once script is translated (or as)
 *      must still apply platform dependent rules on
 *      structuring the files (scripts, pgms) to be run.
 *
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

#include <cstdio>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN_NT
#include <winsock.h>
#endif	/* WIN_NT */
#ifdef MINGW
#include<io.h>
#endif
#include "tcs.h"
#include "tcs_proto.h"
#include <gds.h>

static int process_command(char* in_line, char* result);
static int process_defn(char* in_line);
static int process_noxcmd(char* in_line, char*  result);
static int process_regtxt(char* in_line, char*  result);
static int handle_keyword(char* in_line, char* result);

static int addstr(char* wrd, char** bufr, char delim);

static int fix_isc(char* line, char* bufr, int language);
static int fix_link(char* line, char* bufr, int language);
static int fix_compile(char* line, char* result, int language);

static char *mytokget(char** position, char* delimiters);
static struct defn *lookup_defn(char* wrd);
static int handle_options(char* options, char* opt_template);
static int handle_filename(char* name);
static int handle_path(char* tok, char*  wrd, char* word2, char** result);

static int disp_upcase(char* string, char* name);

static int	language = 0;	// programming language flag
static int	defn_ofst = 0;	// offset into defintion table for next symbol

const int MAXDEFNS		= 80;	// Max number of definitions
const int MAXIDLEN		= 40;	// Max length of ident field
const int MAXRPCLN		= 256;	// Max length of replacement field

// Entry for symbol definition
struct defn
{
	char ident[MAXIDLEN];
	char replacement[MAXRPCLN];
};

// include the proper file for translation of system-specific commands

#include "trns.h"

struct defn definitions[MAXDEFNS + 1] =
{
	{"", ""}
};

#ifdef NO_STRSTR
char *strstr(char *text, char *sub)
{
	/**************************************
	 *
	 *	s t r s t r
	 *
	 **************************************
	 *
	 * Functional description
	 *   finds the first occurance of sub in text and
	 *   returns a pointer to it, otherwise returns 0
	 *
	 **************************************/
	char *ptr, *cmp1, *cmp2;
	for (ptr = text; *ptr != '\0'; ptr++) {
		for (cmp1 = sub, cmp2 = ptr; *cmp1 && (*cmp1 == *cmp2);cmp1++, cmp2++)
			;
		if (!*cmp1)
			return ptr;
	}
	return (char*)0;
}
#endif

int PTSL_symbol(char *in_line)
{
	/**************************************************************************
	 *
	 *	P T S L _  s y m b o l
	 *
	 **************************************************************************
	 *
	 * Process environment variables
	 *
	 **************************************************************************/
	USHORT linetype;
	USHORT success;

	if (*in_line == '#')
	{
		linetype = COMML;
		success = true;
	}
	else
	{
		linetype = SYMDFN;
		success = (USHORT) process_defn(in_line);	// symbol definition
	}

	return (success ? linetype : false);
}

int PTSL_2ap(char *in_line, char *result)
{
	/**************************************************************************
	 *
	 *	P T S L _  2 a p
	 *
	 **************************************************************************
	 *
	 *	Translate a script
	 *      Return the linetype for the edification of the
	 *      command file parser
	 *
	 **************************************************************************/
	USHORT linetype;
	USHORT success;

	//  If the first character in the line is a '$' then we have a
	//  verb (command) that needs translation.  Call process_command
	//  to translate.

	if (*in_line == '$')
	{
		linetype = CMD;
		success = (USHORT) process_command(in_line, result); //  command line
	}

	// If the first character is a carat and the second is a '$'
	// then we do not process the command, but mark it as a command
	// anyway and return.

	else if (*in_line == '^' && *(in_line + 1) == '$')
	{
		success = (USHORT) process_noxcmd(in_line, result);	/* no translate cmd */
		linetype = CMD;
	}

	//  What ever is left by the time we reach here must be plain old
	//  text, so call process_regtxt() to handle that, and remove
	//  dollar signs if need be.

	else
	{
		linetype = REGTXT;
		success = (USHORT) process_regtxt(in_line,  result); //  regular text
	}

	return (success ? linetype : false);
}

static int addstr(char *wrd, char **bufr, char delim)
{
	/**************************************
	 *
	 *	a d d s t r
	 *
	 **************************************
	 *
	 * Functional description
	 *       add a word to a linebuffer, bumping pointers
	 *       end the line with the caller's choice of chars
	 *
	 **************************************/
	char    *bptr;
	SSHORT	nl;

	//  If we didn't get a value in wrd to put in bufr return

	if (wrd == NULL || wrd[0]=='\0')
		return 1;

	nl = false;

	//  If the wrd already has a new line following it, then do not
	//  add delimiters to the end of the line.

	if (*wrd == '\n')
		nl = true;

	//  Do the move wrd into bufr, and make sure we haven't stopped
	//  copying.

	for (bptr = *bufr; (*bptr = *wrd); bptr++, wrd++)
	{
		if (!*bptr)
			break;
	}

	//  If we have a delimiter, and the new word did not end with a
	//  new line, then put the delimiter at the end of the line.

	if ((SSHORT)delim && !nl)
		*bptr++ = delim;

	*bufr = bptr;

	return 0;
}

static int fix_compile(char *line, char *result, int lang)
{
	/**************************************
	 *
	 *	f i x _ c o m p i l e
	 *
	 **************************************
	 *
	 * Functional description
	 *	parse the rest of the line for
	 *	environment variables expected
	 *	on a compile line.
	 *
	 **************************************/
	bool	options = false;
	char	*wrd, *tok, *firstname, *posn, upc[MAXRPCLN];
	char	*bufr, options_template[MAXRPCLN];
	struct defn *dptr;

	firstname = NULL;
	posn = wrd = line;
	bufr = result;

	language = lang;

	//  loop through the line...

	while ((tok = (char *)mytokget(&posn, " \t\n\r")))
	{
		disp_upcase(tok, upc);
		dptr = lookup_defn(upc);

		//  If a valid environment variable has been found, use it.

		if (*(dptr->ident))
		{

			//  If an environment variable was found, but that variable has
			//  no value, then continue looping.

			if (!*(dptr->replacement))
				continue;

			wrd = dptr->replacement;
			options = true;

			if (handle_options(wrd, options_template))
				continue;
		}

		//  If not an environment variable, check for a filename and modify
		//  the suffix of that filename if found.

		else {
			if (!firstname) // If this is a first token
				firstname = tok;

			posn += handle_filename(firstname); 	//  Check filename suffix.
			break;
		}

		addstr(wrd, &bufr, ' ');
		wrd = posn;
	}

	//  If no filename was found print and error message.

	if (!firstname)
	{
		print_error("%s fails parse.\nFilename missing\n",line,0,0);
		return false;
	}

	//  If no environment variable was used, then use the defaults

	if (!options){
		printf("ERROR: Environment variable not defined line %s\n", line);
		exit(1);
	}

	//  If an environment variable was used, then use it.

	else if (*options_template)
		sprintf(bufr, options_template, firstname);

	//  Otherwise move the value in firstname into the buffer

	else {
		addstr(firstname, &bufr, 0);
		*bufr++ = '\n';
		*bufr = 0;
	}
	return 0;
}

static int fix_isc(char *line, char *bufr, int lang)
{
	/**************************************
	 *
	 *	f i x _ i s c
	 *
	 **************************************
	 *
	 * Functional description
	 *       replace any pathname specifiers (and
	 *       possibly flags) in an isc  command:
	 *       gbak, gfix, gdef, glitj, qli
	 *	 Also call handle_filename() to modify suffix properly
	 *
	 **************************************/
	char	*tok, *tok2, upc[MAXRPCLN], *wrd, *posn;
	struct defn *dptr;

	wrd = posn = line;

	//  Get the first token.

	while ((tok = (char *)mytokget(&posn, " \t\n\r")))
	{

		//  Get the second token

		if ((tok2 = (char*) strchr(tok, ':')))
		{
			*tok2++ = 0; 			  /* Remove colon */

			//  posn is supposed to point to the next token following tok2, but if .a
			//  changes to .ada posn must be incremented appropiately by handle_path.

			posn += handle_path(tok, wrd, tok2, &bufr);
			/*   	*bufr++ = ' '; */
			wrd = posn;
			continue;
		}

		disp_upcase(tok, upc);
		dptr = lookup_defn(upc);

		//  If we have a valid environment variable then use it.

		if (*(dptr->ident))
		{

			//  If the variable has no value, then do not use.

			if (!*(dptr->replacement))
				continue;

			tok = dptr->replacement;
		}

		//  Evaluate anything else, checking to make sure it has the proper
		//  suffix by calling handle_filename().

		else
			//** Again posn must be incremented appropriately.
			posn += handle_filename(tok);

		addstr(tok, &bufr, ' ');		// Insert value & trailing space
		wrd = posn;
	}

	*bufr = 0;
	*--bufr = '\n';

	return 0;
}

static int fix_link(char *line, char *bufr, int lang)
{
	/**************************************
	 *
	 *	f i x _ l i n k
	 *
	 **************************************
	 *
	 * Functional description
	 *    link command syntax:
	 *         LINK FLAG_LINK filename[ filename...]
	 *         where FLAG_LINK may have the form
	 *         -ch [-ch...] %s [-ch...]] %s
	 *    translation form:
	 *    command [switches] objfile(s) [switches] exefile
	 *    or vms: command /exe=file file[, file] [switches]
	 *
	 **************************************/
	char	*firstname, *posn, *tok;
	char	options_template[512], *fptr, *tfsp;
	char	files[MAXRPCLN], upc[MAXRPCLN], tfs[MAXRPCLN];
	static struct defn *dptr;
	bool	options = false;

	firstname  = fptr = (char*) NULL;
	dptr = NULL;
	posn = line;

	//  Loop through each token on the line.

	while ((tok = (char *)mytokget(&posn, " \t\n\r")))
	{
		disp_upcase(tok, upc);
		dptr = lookup_defn(upc);

		//  Check to see if it is in the environment variable table but
		//  does not have a definition.  If this is the case, then continue
		//  looping.

		if (*(dptr->ident) && !*(dptr->replacement))
			continue;

		//  Check to see if it is in the environment variable table, if it
		//  is, then use it.

		if (*(dptr->ident))
		{
			tok = dptr->replacement;

			//  Move the environment variable value, into options template.
			options = true;
			handle_options(tok, options_template);
			continue;
		}

		//  If we are not processing an environment variable(symbol), then
		//  we must be processing filenames.  So process them and their
		//  possible suffixes.

		else
		{

			//  Only save the first filename.

			if (!firstname)
			{
				fptr = files;
				firstname = tok;
			}

			tfsp = tfs;
			addstr(tok, &tfsp, 0);
			addstr(link_ext[language], &tfsp, 0);
			handle_filename(tfs);
			addstr(tfs, &fptr, ' ');
			continue;
		}
		addstr(tok, &bufr, ' ');
	}

	//  If did not get a firstname then, print error message.

	if (!firstname)
	{
		print_error("%s fails parse.\nFilename missing\n",line,0,0);
		return false;
	}

	*fptr = 0;

	//  Use the default link info. to run the filename through if we
	//  did not get an environment variable with the link info. in it.

	if (!options){
		printf("ERROR: Environment variable not defined line %s\n", line);
		exit(1);
	}
	else if (*options_template)
		sprintf(bufr, options_template, files, firstname);

	return 0;
}

int set_ptl_lookup(char *verb, char *def)
{
	/**************************************
	 *
	 *	s e t _ p t l _ l o o k u p
	 *
	 **************************************
	 *
	 * Functional description
	 * 	If the '-m' command line switch 
	 *	has been thrown on NT then this
	 *	function has been called and it
	 *	will change the ptl_lookup table
	 *
	 **************************************/
	struct ptl_cmd *lookup;

	/*   	Loop through the '$' verb table... 				*/

	for (lookup = ptl_lookup; lookup->ptsl_cmds; lookup++)
		if (!strcmp(lookup->ptsl_cmds, verb)) {
			lookup->cmd_replacemt = def;
			return true;
		}

	return false;
}

static int handle_filename(char *name)
{
	/**************************************
	 *
	 *	h a n d l e _ f i l e n a m e
	 *
	 **************************************
	 *
	 * Functional description
	 *        squish filename for picky systems
	 *        a filename (the part before the extension
	 *        is SSHORTened to 8 char with no '_'
	 *
	 *        On VMS, convert .eftn suffixes to .efor
	 *        and .ftn to .for
	 *
	 *	  On OS2 emasculate the extension
	 *        which should be handled by a nice table but
	 *        for only 2 instances is it worth it?
	 *
	 **************************************/
	char	temp[80];
	char*	dot;
	int		longer = 0;

#ifndef USHORT_NAMES

	//  If name has a suffix, then try to convert...

	if (dot = (char*) strchr(name, '.'))
	{
		//  Convert for a gbak extension...
		//  The only case for now is NT extension .gbk

		if (!strcmp(dot, ".gbak") && GBAK_EXT && strcmp(dot, GBAK_EXT))
		{
			*dot = '\0';
			strcat(name, GBAK_EXT);
		}

		//  Fix ada extensions...We do complex calculations here
		//  because ada extensions may get longer than in the TCS
		//  test.  If this is the case we have to move the rest of
		//  the line over, else it will get garbled/written over.

#ifdef ADA_EXT
		else if (!strcmp(dot, ".a") && ADA_EXT && strcmp(ADA_EXT, ".a"))
		{

			//  Modify .a to appropriate suffix for ada files and save length
			//  difference in longer.
			*temp = '\0';
			strcpy(temp, dot+3);					// Save next token.
			memcpy(dot, ADA_EXT, strlen(ADA_EXT));	// Add new suffix.
			dot += strlen(ADA_EXT)+1;				// Move dot past new suffix.
			*(dot-1) = '\0';						// Insert delimiter.

			//  If there was anything else on the line, reattach...

			if(*temp) {
				addstr(temp, &dot, 0);		// Reattach following token/line.
				longer = strlen(ADA_EXT)-2;	// Calculate difference in length.
			} else
				longer = strlen(ADA_EXT)-3;
		}
#endif // ADA_EXT

		//  Another fix for an ada gpre file.

#ifdef ADA_GPRE_EXT
		else if (!strcmp(dot, ".ea") && ADA_GPRE_EXT && strcmp(ADA_GPRE_EXT, ".ea"))
		{
			//  Modify .ea to appropriate suffix for ada files and save length
			//  difference in longer.
			*temp = '\0';
			strcpy(temp, dot+4);								// Save next token.
			memcpy(dot, ADA_GPRE_EXT, strlen(ADA_GPRE_EXT));	// Add new suffix.
			dot += strlen(ADA_GPRE_EXT)+1;						// Move dot past new suffix.
			*(dot-1) = '\0';									// Insert delimiter.

			//  If there was anything else on the line, reattach...

			if(*temp) {
				//  Reattach following token/line.
				addstr(temp, &dot, 0);
				//  Calculate difference in length.
				longer = strlen(ADA_GPRE_EXT)-3;
			}
			else
				longer = strlen(ADA_GPRE_EXT)-4;
		}
#endif // ADA_GPRE_EXT

		//  Handle Fortran extensions...
		//  We don't do complicated ada calculations here because extension
		//  length for fortran stays the same or gets smaller that the
		//  extension in the TCS test--It does not get longer.

#ifdef E_FORTRAN_EXT
		else if (strcmp(dot, ".eftn") == 0 && strcmp(E_FORTRAN_EXT, dot) != 0)
		{
			*dot = '\0';
			strcat(name, E_FORTRAN_EXT);
		}
#endif

#ifdef FORTRAN_EXT
		else if (strcmp(dot, ".ftn") == 0 && strcmp(FORTRAN_EXT, dot) != 0)
		{
			*dot = '\0';
			strcat(name, FORTRAN_EXT);
		}
#endif

		//  Handle Cobol extensions...
#ifdef E_COBOL_EXT
		else if (strcmp(dot, ".ecbl") == 0 && strcmp(E_COBOL_EXT, dot))
		{
			*dot = '\0';
			strcat(name, E_COBOL_EXT);
		}
#endif

#ifdef COBOL_EXT
		else if (strcmp(dot, ".cbl") == 0 && strcmp(COBOL_EXT, dot))
		{
			*dot = '\0';
			strcat(name, COBOL_EXT);
		}
#endif

#ifdef E_CXX_EXT
		//  Handle GPRE C++ extensions... The default is .exx according to the docs
		else if (strcmp(dot, ".E") == 0 && strcmp(E_CXX_EXT, dot))
		{
			//  Modify .a to appropriate suffix for cxx files and save length 
			//  difference in longer.

			*temp = '\0';
			strcpy(temp, dot+3);						//  Save next token.
			memcpy(dot, E_CXX_EXT, strlen(E_CXX_EXT));	//  Add new suffix.
			dot += strlen(E_CXX_EXT)+1;					//  Move dot past new suffix.
			*(dot-1) = '\0';							//  Insert delimiter.

			//  If there was anything else on the line, reattach...

			if(*temp)
			{
				//  Reattach following token/line.
				addstr(temp, &dot, 0);
				//  Calculate difference in length.
				longer = strlen(E_CXX_EXT)-2;
			}
			else
				longer = strlen(E_CXX_EXT)-3;
		}
#endif

#ifdef CXX_EXT
		else if ((strcmp(dot, ".cxx") == 0 || strcmp(dot, ".C") == 0) &&
		         strcmp(CXX_EXT, dot) != 0)
		{
			//  Modify .C to appropriate suffix for cxx files and save length
			//  difference in longer.
			*temp = '\0';
			strcpy(temp, dot+3);					// Save next token.
			memcpy(dot, CXX_EXT, strlen(CXX_EXT));	// Add new suffix.
			dot += strlen(CXX_EXT)+1;				// Move dot past new suffix.
			*(dot-1) = '\0';						// Insert delimiter.

			//  If there was anything else on the line, reattach...

			if(*temp)
			{
				addstr(temp, &dot, 0);			// Reattach following token/line.
				longer = strlen(CXX_EXT)-2;		// Calculate difference in length.
			}
			else
				longer = strlen(CXX_EXT)-3;
		}
	}

#endif // CXX_EXT

	//  Return the length change for a file suffix modifications.
	if (longer < 0)
		return 0;

	return longer;

#else

	strcpy(temp, name);
	j = 8;
	if ((temp[0] == '\"') || (temp[0] == '\''))
		j = 9;

	for (t = temp, n = name, i = 0; i < j; t++) {
		if ((!*t) || (*t == '.'))
			break;

		if (*t == '_' )
			continue;

		*n++ = *t;
		i++;
	}

	dot = (char*) strchr(temp, '.');

	*n =0;
	strcat(name, dot);

	return 0;
#endif
}

static int handle_options(char *options, char *opt_template)
{
	/**************************************
	 *
	 *	h a n d l e _ o p t i o n s
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 *        if this is an options template put it in
	 *        the template buffer with a trailing \n and 
	 *	  NULL, otherwise, copy it into
	 *        the command result buffer.
	 *        a template contains %s
	 *
	 **************************************/
	addstr(options, &opt_template, '\n');

	*opt_template = 0;
	return true;
}

static int handle_path(char *tok, char *wrd, char *word2, char **result)
{
	/**************************************
	 *
	 *	h a n d l e _ p a t h
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 *        This assumes that a string containing
	 *        a ':' has been found and that the word
	 *        preceding the colon is likely to be a 
	 *        path definition keyword.
	 *
	 **************************************/
	char 	upc[MAXRPCLN], *resultptr;
	struct defn *dptr;
	int longer = 0;

	resultptr = *result;

	//  skip char common in #include statements

	while ((*tok == '"') || (*tok == '<') || (*tok == '\'') || (*tok == '\\'))
		tok++;

	disp_upcase(tok, upc);
	dptr = lookup_defn(upc);

	//  preserve the leading white space in the result string

	while (wrd < tok)
		*resultptr++ = *wrd++;

	//  If we found a symbol (environment variable) -- use it.

	if (*(dptr->ident))
	{
		if (*(dptr->replacement)) {
			addstr(dptr->replacement, &resultptr, PATH_SEPARATER);
		}

		tok = word2;
	}

	//  If we didn't find a symbol, then reconstruct the line the way
	//  it used to be and return.

	else
	{
		addstr(tok, &resultptr, 0);
		*resultptr++ = ':';
		addstr(word2, &resultptr, ' ');
		*result = resultptr;
		return longer;
	}

	longer = handle_filename(tok);			//  Save length to be passed up
	addstr(tok, &resultptr, ' ');

	*result = resultptr;

	//  Pass up the change in length if .a goes to .ada
	return longer;
}

static struct defn *lookup_defn(char *wrd)
{
	/**************************************
	 *
	 *	l o o k u p _ d e f n
	 *
	 **************************************
	 *
	 * Functional description
	 *      Look up word in symbol list and return pointer to the
	 *      entry in the lookup table.  If no matching entry return
	 *      the address of the first empty space.  
	 *      If table is empty, return a point to the first empty space.
	 *
	 **************************************/
	struct defn *dptr;

	//  If the table exists, lookup the word.

	if (defn_ofst)
	{
		for (dptr = definitions; *(dptr->ident);  dptr++)
			if (!strcmp(wrd, dptr->ident))
				break;
	}
	else
		dptr = &definitions [0];

	return dptr;
}

static char *mytokget(char **position, char *delimiters)
{
	/**************************************
	 *
	 *	m y t o k g e t
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 *         return a pointer to the next interesting
	 *         section of a string. 
	 *
	 **************************************/
	char	*ch, *delim, *str;

	if (!(**position))
		return (NULL);

	//  skip delimiters until the string starts

	for (ch = *position; *ch ; ch++)
	{
		for (delim = delimiters; *delim && (*ch != *delim); )
			delim++;

		if (!*delim)
			break;
	}

	if (!*ch)
		return (NULL);

	str = ch;

	//  find the delimiter at the end of the string
	while (*ch)
	{
		for (delim = delimiters; *delim && (*ch != *delim); )
			delim++;

		if (*delim)
			break;

		ch++;
	}

	if (*ch)
		*ch++ = 0;

	*position = ch;

	return (str);
}

static int process_command(char *in_line, char *result)
{
	/**************************************
	 *
	 *	p r o c e s s _ c o m m a n d
	 *
	 **************************************
	 *
	 * Functional description
	 *      Find the command. Move it to the result string.
	 *      Pass the rest of the in_line and the result to
	 *      an appropriate fix routine
	 *
	 **************************************/
	char	*resultptr, *word1, *word2, *posn;
	bool	found;
	char	upc[MAXRPCLN];
	struct	defn *dptr;
	struct	ptl_cmd *lookup;

	found = 0;
	posn = in_line;

	//  handle empty commands
	if (!(word1 = mytokget (&posn, " $\t\n\r")) || (!*word1))
	{
		strcpy(result, in_line);
		return true;
	}

	for (resultptr = result; in_line < word1; )
		*resultptr++ = *in_line++;

	//  look for a command

	while (!found) {
		disp_upcase(word1,  upc);

		//  is this in the command table?

		for (lookup = ptl_lookup; lookup->ptsl_cmds; lookup++)
			if (!strcmp(lookup->ptsl_cmds,  upc)) {
				found = true;
				break;
			}
		//  yes - copy it and leave the loop
		if (found) {
			if (lookup->cmd_replacemt==NULL)
			{
				printf("Dollar verb %s not defined\n", lookup->ptsl_cmds);
				exit(1);
			}
			else {
				addstr(lookup->cmd_replacemt, &resultptr, ' ');
				break;
			}
		}

		//  no - so if its a path we need to look up the definition

		if ((word2 = (char*) strchr (upc, ':')))
			*word2++ = 0;

		dptr = lookup_defn(upc);

		//  If we found the definition, move it in.

		if (*(dptr->ident))
		{
			addstr(dptr->replacement, &resultptr, PATH_SEPARATER);
			word1 = word2;

			//  If there is no token following path, then we have a syntax
			//  error.

			if (!word2)
			{
				print_error("Unexpected end of command: %s\n",in_line,0,0);
				return false;
			}

			continue;
		}

		//  Otherwise we could not translate...print error

		else
		{
			print_error("Can't translate: %s. Giving up.\n",word1,0,0);
			return false;
		}
	}

	//  If the $ verb was not found then print an error.

	if (!found) {
		print_error("Untranslatable command: %s \n",  in_line,0,0);
		return false;
	}

	//  Call the function associated with this $ verb if it exists

	if (lookup->fixup)
		(*lookup->fixup)(posn, resultptr, lookup->language);

	return true;
}

static int process_defn(char *in_line)
{
	/**************************************
	 *
	 *      p r o c e s s _ d e f n
	 *
	 **************************************
	 *
	 * Functional description
	 *      This routine handles symbol definition.  If a new symbol is
	 *      received it is added to the table.  If the symbol is already
	 *      defined the definition is replaced with the new one.
	 *
	 **************************************/
	char	*word1, *word2, *posn;
	static char	w2 = 0;
	struct defn *dptr;

	posn = in_line;

	//  Get the environment variable name...

	if (!(word1 = mytokget(&posn, " :\t\n\r")))
		return true;

	//  Get the value...

	if (!(word2 = mytokget(&posn, "\n\r")))
		word2 = &w2;

	//  find a definition and replace it if we can.
	//  otherwise, point at 1st empty slot 

	//  If there are any entries in the symbol definition table, then
	//  check to see if our symbol exists, and if it does replace it
	//  quickly.
	printf("\tEnvironment template: %s = %s\n", word1, word2);
	fflush(stdout);
	if (defn_ofst)				//  This is a counter of symbols
	{
		dptr = lookup_defn(word1);

		//  If we found the symbol in the table, replace with the new one.

		if (*(dptr->ident))

		{
			strcpy(dptr->replacement, word2);
			return true;
		}
	}

	//  The symbol definition/replacement table has zero entries

	else
		dptr = definitions;

	//  store a new definition and mark the end of the table

	if (defn_ofst < MAXDEFNS)
	{
		strcpy(dptr->ident, word1);
		strcpy(dptr->replacement, word2);
		defn_ofst++;
		dptr++;
		*(dptr->ident) = 0;
		*(dptr->replacement) = 0;
	}
	else
		print_error("Maximum number of symbol definitions exceeded\n",0,0,0);

	return true;
}

static int process_noxcmd(char *in_line, char *result)
{
	/**************************************
	 *
	 *	p r o c e s s _ n o x c m d _ l i n e
	 *
	 **************************************
	 *
	 * Functional description
	 *      This routine removes the '^" from the front of a command line
	 *      that is not to be translated.
	 *      
	 **************************************/

	strcpy(result, ++in_line);
	return true;
}

static int process_regtxt(char *in_line, char *result)
{
	/**************************************
	 *
	 *	p r o c e s s _ r e g t x t 
	 *
	 **************************************
	 *
	 * Functional description
	 *      This routine processes  the regular text portion of the script.
	 *      one string at a time.
	 *      It will do symbol substitution for paths if used in a #include,
	 *      ready or database statement.  
	 *
	 *      a DATABASE statement can extend over 2 lines. To process this 
	 *      correctly we set keyword and unset it as soon as we find another token
	 *      
	 **************************************/
	char	*wrd, *posn, *rsltptr;
//	static char parse_database = 0;

	rsltptr = result;
	wrd = posn = in_line;

	//  If the line contains 'database' then set parse_database to true
//	if (strstr(wrd,"DATABASE")	|| strstr(wrd,"database") || parse_database) {
//		parse_database = 1;
//	}

	//  If the line contains a ';' and parse_database is true then set
	//  parse_database to false.
//	if (strchr(wrd, ';'))
//		parse_database = 0;

	//  If the line is blank or does not contain a colon or a dot then
	//  move the line to result and return.
	if ((*wrd == '\n') || (!strchr(wrd, ':'))){ // && !strchr(wrd, '.'))) {
		strcpy(result, in_line);
		return true;
	}

	//  If the line does not contain '#include', 'ready' or 'connect'
	//  then move the line to result and return.
/*
	if ((!strstr(wrd,"#include"))	&&
	        (!strstr(wrd,"#define"))	&&
	        (!strstr(wrd,"CONNECT"))	&&
	        (!strstr(wrd,"connect"))	&&
	        (!strstr(wrd,"READY"))	&&
	        (!strstr(wrd,"ready"))	&&
	        (!strstr(wrd,"DATABASE"))	&&
	        (!strstr(wrd,"database"))	&&
	        (!strstr(wrd,"SCHEMA"))	&&
	        (!strstr(wrd,"schema"))	&&
	        (!parse_database)) {
		strcpy(result, in_line);
		return true;
	}
*/
	//
	//  Processes all the tokens and replaces the env variables
	//  Adds \n at the end of the line and zero terminates the string
	//
	handle_keyword(posn, rsltptr);

	return true;
}

static int handle_keyword (char *in_line, char *result)
{
	/**************************************
	 *
	 *	h a n d l e _ k e y w o r d 
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 *         handle a line or a partial line with
	 *         a filename spec in it.  If this line is
	 *         empty then return true.  
	 *
	 **************************************/
	char	*wrd, *posn, *tok, *tok2;

	wrd = posn = in_line;

	/*	Loop through each token on the line.				*/

	while ((tok = mytokget(&posn, " \t\n\r")))
	{

		/*	Check for an environment variable path specifier.  If found	*
		 *	call handle_path to do the work.				*/

		if ((tok2 = (char*) strchr (tok, ':')) && ( *tok != ':' ))
		{
			/*      If there was a space following the colon, then this can't be 	*
			 *      a TCS environment variable, so move word over and continue.	*/

			if ( *(tok2+1) == '\0' ) {
				addstr (wrd, &result, ' ');
				wrd = posn;
				continue;
			}

			*tok2++ = 0; 		/* Get rid of the colon. */
			handle_path (tok, wrd, tok2, &result);
			wrd = posn;
			continue;
		}


		/*	If we have a dot in our token and we are not in the middle of	*
		 *	an #include statement then we have a filename, so call  	*
		 *	handle_filename to handle the extension.			*/

		if (strchr (tok, '.') && (!strchr (tok,'<')))
			handle_filename (tok);

		addstr (wrd, &result, ' ');
		wrd = posn;
	}

	*result = 0;
	*--result = '\n';  /* Replace the delimeter with a new line */
	return 0;
}

int disp_upcase(char *string, char *name)
{
	/**************************************
	 *
	 *	d i s p _ u p c a s e
	 *
	 **************************************
	 *
	 * Functional description
	 *	Translate a string into a name.  The string may be either
	 *	blank or null terminated.  The output name is null terminated.
	 *
	 **************************************/
	char	c;

	while ((c = *string++) && c != ' ')
		*name++ = UPPER(c);

	*name = 0;

	return 0;
}
