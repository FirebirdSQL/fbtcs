/*
 *	PROGRAM:	isc_diffs
 *	MODULE:		diffs.h
 *	DESCRIPTION:	defines, includes and typedefs
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

const int PAGESIZE	= 4096;		// default line page size
const int MASK		= 0x0FFF;	// 12 bit hash value
const int MAXLINE	= 256;		// maximum line length


const int CHANGE	= 0;	// change a range of lines
const int DELETE	= 1;	// delete a range of lines
const int ADD		= 2;	// add a range of lines

#define MIN(a,b)	((a < b) ? a : b)

const char* OUTFILE		= "dif.out";
const int BUFSIZE		= 4096;
const int BUF_FULL		= 4080;

typedef struct 
{
	char	*addr;		// address of string in buffer
	ULONG	hash;		// hash value for string
	SLONG	linenum;	// line number in file
}
LINE;


typedef struct file_block
{
	char	*buffer;
	SLONG	line_count;
	LINE		line_ptr [1];
} *FILE_BLK_PTR, FILE_BLK;


static SLONG window;
static SLONG minmatch;

static SLONG ignore_whtsp;

