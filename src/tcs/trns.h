/*
 *	PROGRAM:	Test Control System
 *	MODULE:		trns.h
 *	DESCRIPTION:
 *
 *	These are system-specific declarations for OS/2 and for NT systems
 *	to aid in modifying command lines in tests.
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

enum language_flags {
	LANG_ADA		= 1,	// language flags
	LANG_BASIC		= 2,
	LANG_CC			= 3,
	LANG_COBOL		= 4,
	LANG_FORTRAN	= 5,
	LANG_PASCAL		= 6,
	LANG_PL1		= 7,
	LANG_LINK		= 8,
	LANG_CXX		= 9,
	LANG_JAVA		= 10,
	LANG_JAVAC		= 11
};

// hpux
#ifdef hpux
#define E_FORTRAN_EXT	".ef"
#define FORTRAN_EXT	".f"
#endif
// sun 
#ifdef sun
#ifndef SOLARIS
#undef NO_DOLLAR_SIGN
#endif
#define ADA_GPRE_EXT	".ea"
#define ADA_EXT		".a"
#define E_FORTRAN_EXT	".ef"
#define FORTRAN_EXT	".f"
#endif
//vms
#ifdef VMS
#undef NO_DOLLAR_SIGN
#define ADA_GPRE_EXT	".EADA"
#define ADA_EXT		".ADA"
#define E_FORTRAN_EXT	".efor"
#define FORTRAN_EXT	".for"
#define E_COBOL_EXT	".ecob"
#define COBOL_EXT	   ".cob"
#define E_CXX_EXT	   ".EXX"
#define CXX_EXT		".CXX"
#endif
// win_nt
#if (defined WIN_NT)
#define E_CXX_EXT ".exx"
#define CXX_EXT   ".cpp"
#define GBAK_EXT	".gbk"
#endif
// sco_unix
#ifdef SCO_UNIX
#define ADA_EXT		".a"
#define ADA_GPRE_EXT	".ea"
#endif

// default
#ifndef E_CXX_EXT
#define E_CXX_EXT	".E"
#define CXX_EXT 	".C"
#endif

#ifndef E_COBOL_EXT
#define E_COBOL_EXT	".ecbl"
#define COBOL_EXT	   ".cbl"
#endif

#ifndef E_FORTRAN_EXT
#define E_FORTRAN_EXT	".ef"
#define FORTRAN_EXT	   ".f"
#endif

#ifndef GBAK_EXT
#define GBAK_EXT	".gbak"
#endif

static char *link_ext[] = {
#ifdef WIN_NT
#	ifndef MINGW
	 ".obj",
	 " ",
	 ".obj",
	 ".obj",
	 ".obj",

	 ".obj",
	 ".obj",
	 ".obj",
	 ".obj",
	 ".obj"
#	else
	 ".o",
	 " ",
	 ".o",
	 ".o",
	 ".o",
	 ".o",
	 ".o",
	 ".o",
	 ".o",
	 ".o"
#	endif
#else
	".bin",
	" ",
	".bin",
	".o",
	".int",
	".o",
	".bin",
	".bin",
	".o",
	".o"
#endif
	 };

// PTSL Command lookup table
static struct ptl_cmd {
	SCHAR	*ptsl_cmds;
	SCHAR	*cmd_replacemt;
	int		(*fixup)(char*, char*, int);
	int		language;
	SCHAR	*ptsl_cmd_text;
} ptl_lookup[] = {
	{"ADA",			NULL,				fix_compile,	LANG_ADA, "\t\tInvokes ada compiler"},
	{"ADA_LINK",	NULL,				fix_link,		0, "\tInvokes ada linker"},
	{"ADA_MKFAM",	NULL,				fix_isc,		0, "\tInvokes ada family manager to create a family"},
	{"ADA_MKLIB",	NULL,				fix_isc,		0, "\tCreates an ada library"},
	{"ADA_RMFAM",	NULL,				fix_isc,		0, "\tInvokes ada family manager to remove a family"},
	{"ADA_RMLIB",	NULL,				fix_isc,		0, "\tRemoves an ada library"},
	{"ADA_SEARCH",	NULL,				fix_isc,		0, ""},
	{"ADA_SETLIB",	NULL,				fix_isc,		0, ""},
	{"CC",			NULL,				fix_compile,	LANG_CC, "\t\tC Compile"},
	{"CXX",			NULL,				fix_compile,	LANG_CXX, "\t\tC++ compile"},
	{"COBOL",		NULL,				fix_compile,	LANG_COBOL, "\t\tInvokes cobol compiler"},
	{"COB_LINK",	NULL,				fix_link,		0, "\tInvokes cobol linker"},
	{"COPY",		"copy_file ",		fix_isc,		0, "\t\t\"copy_file\""},
	{"CREATE",		"create_file > ",	fix_isc,		0, "\t\t\"create_file >\""},
	{"DELETE",		NULL,				fix_isc,		0, "\t\t\"rm\""},
	{"DELETE_REM",	"delete_file",		fix_isc,		0, "\t\t\"delete_file\""},
	{"DROP",		"drop_gdb",			fix_isc,		0, "\t\t\"drop_gdb\""},
	{"FORTRAN",		NULL,				fix_compile,	LANG_FORTRAN, "\t\tInvokes fortran compiler"},
	{"GBAK",		"gbak",				fix_isc,		0, "\t\t\"gbak\""},
	{"GDEF",		"gdef",				fix_isc,		0, "\t\t\"gdef\""},
	{"GFIX",		"gfix",				fix_isc,		0, "\t\t\"gfix\""},
	{"GJRN",		"gjrn",				fix_isc,		0, "\t\t\"gjrn\""},
	{"GPRE",		"gpre",				fix_isc,		0, "\t\t\"gpre\""},
	{"GSEC",		"gsec",				fix_isc,		0, "\t\t\"gsec\""},
	{"ISQL",		"isql",				fix_isc,		0, "\t\t\"isql\""},
	{"JAVA",		"java",				fix_compile,	LANG_JAVA, "\t\t\"java\""},
	{"JAVAC",		"javac",			fix_compile,	LANG_JAVAC, "\t\t\"javac\""},
	{"LINK",		NULL,				fix_link,		0, "\t\tInvokes linker"},
	{"MAKE",		NULL,				fix_isc,		0, "\t\t\"nmake\""},
	{"PASCAL",		NULL,				fix_compile,	LANG_PASCAL, "\t\tInvokes pascal compiler"},
	{"QLI",			"qli",				fix_isc,		0, "\t\t\"qli\""},
	{"RUN",			NULL,				fix_isc,		0, "\t\tExecute in the shell"},
	{"RUN_BG",		NULL,				fix_isc,		0, "\t\t\"sh\""},
	{"SH",			NULL,				fix_isc,		0, "\t\t\"sh\""},
	{"SLEEP",		"sleep_sec",		fix_isc,		0, "\t\t\"sleep_sec\""},
	{"TYPE",		"create_file <",	fix_isc,		0, "\t\t\"create_file <\""},
	{ NULL, NULL, NULL, 0, NULL}
	};

#define		EXE_EXT			".exe"
#define		PATH_SEPARATER	'/'
