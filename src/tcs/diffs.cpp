/*
 *	PROGRAM:	isc_diffs
 *	MODULE:		diffs.c
 *	DESCRIPTION:	our modification of a diff utility
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
#include <gds.h>
#include "do_diffs_proto.h"

#define UPPER(c)	(((c) >= 'a' && (c)<= 'z') ? (c) - 'a' + 'A' : (c))

// Defined as needed in do_diffs.c
bool	disk_io_error;

int main (int argc, char *argv[])
{
	/**************************************
	 *
	 *	m a i n
	 *
	 **************************************
	 *
	 * Functional description
	 *
	 **************************************/
	char	*p, *input_1, *input_2, *diff_file;
	long	window, minmatch, ignore_whtsp;

	if ( argc < 4 ) {
		fprintf (stderr, "Usage: isc_diff from_file to_file output_file [options]\n" );
		exit(1);
	}

	argv++;
	argc--;
	input_1 = *argv++;
	argc--;
	input_2 = *argv++;
	argc--;
	diff_file = *argv++;
	argc--;

	window = 75;
	minmatch = 6;
	ignore_whtsp = false;

	while (argc--) {
		p = *argv++;
		if (*p++ != '-')
			continue;
		switch (UPPER (*p)) {
		case 'W':
			argc--;
			window = atoi (*argv);
			argv++;
			break;
		case 'M':
			argc--;
			minmatch = atoi (*argv);
			argv++;
			break;
		case 'I':
			ignore_whtsp = true;
			break;
		}
	}
	if (do_diffs (input_1, input_2, diff_file, window, minmatch, ignore_whtsp))
		exit (0);
	else
		exit (1);
	// make compiler happy
	return 0;
}

