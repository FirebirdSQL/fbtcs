/* modified 7/21/93, TRUE and FALSE had ; at the end of the
   macro which cased a problem when TRUE/FALSE were used
   after an SQL stement. SH.
*/

#ifdef __MSDOS__
#include <stdlib.h>
#if !defined (_Windows)
extern unsigned _stklen = 0x8000;
#endif
#endif

#ifdef VMS
#define TIME_DEFINED
#include <time.h>
#endif

#ifdef apollo
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef sun
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef hpux
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef ultrix
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef DGUX
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef sgi
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifdef mpexl
#define TIME_DEFINED
#include <sys/time.h>
#endif

#ifndef TIME_DEFINED
#include <time.h>
#endif

#define TRUE 1
#define FALSE 0

/*---------------------------------------------------------*/
