#if defined __STDC__ || defined __BORLANDC__ || defined _MSC_VER
#define PROTO(args)		args
#define ARG(type, arg)		type arg
#define ARGLIST(arg)
#else
#define PROTO(args)		()
#define ARG(type, arg)		arg
#define ARGLIST(arg)	arg;
#endif

#if defined __BORLANDC__ && defined __WIN32__
#define EXPORT _export
#else
#define EXPORT
#endif

#define ERREXIT(status, rc)	{isc_print_status(status); return rc;}
