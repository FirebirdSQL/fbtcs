/* Header file for ISC tests */

#include <string.h>
#include <stdlib.h>

#ifndef DARWIN
#include <ibase.h>
#else
#include <Firebird/ibase.h>
#endif

#define ERREXIT(status, rc, err_buf)     {printf("RC=%ld\n", rc); isc_sql_interprete(isc_sqlcode(status), err_buf, sizeof(err_buf)); \
				printf("ERROR: %s\n",err_buf); return;}

#define ERRCONTINUE(status, rc, err_buf)     {isc_sql_interprete(isc_sqlcode(status), err_buf, sizeof(err_buf)); \
				printf("RC=%ld\n",rc); printf("ERROR: %s\n",err_buf);}
