#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef  WIN32
#include <windows.h>
#endif

#if   (defined(__BORLANDC__) && defined(__WIN32__))
#define  EXPORT  __stdcall   _export
#else
#define  EXPORT
#endif
