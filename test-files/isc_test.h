/* Header file for ISC tests */

#include <string.h>
#ifndef IMP
#include <stdlib.h>
#endif

#ifndef DARWIN
#include <malloc.h>
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef _Windows
#ifndef DARWIN
#include <ibase.h>
#else
#include <Firebird/ibase.h>
#endif
#define LogPrintf printf
extern void fill_db_parameter_buffer();
#else

#include "globals.h"

/*
#include <ibase.h>
*/







extern void fill_db_parameter_buffer(void);       
#endif

#define ERREXIT(status, rc, err_buf)     {LogPrintf("RC=%ld\n", rc); isc_sql_interprete(isc_sqlcode(status), err_buf, sizeof(err_buf)); \
				LogPrintf("ERROR: %s\n",err_buf); return;}

#define ERRCONTINUE(status, rc, err_buf)     {isc_sql_interprete(isc_sqlcode(status), err_buf, sizeof(err_buf)); \
				LogPrintf("RC=%ld\n",rc); LogPrintf("ERROR: %s\n",err_buf);}

typedef struct vary20 {
	short vary_length;
	char vary_string[21];
	} VARY20;

typedef struct vary25 {
	short vary_length;
	char vary_string[26];
	} VARY25;

