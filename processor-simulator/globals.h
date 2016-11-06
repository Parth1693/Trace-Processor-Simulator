/////////////
// GLOBALS
/////////////

// The processing core(s).
extern processor **PROC;

// INFO macro definition.
#define INFO(fmt, args...)	\
	(fprintf(fp_info, fmt, ## args),	\
	 fprintf(fp_info, "\n"))

// Output file pointer.
extern FILE *fp_info;

#if !defined(__CYGWIN32__)
// 9/3/05 ER
extern char *comma(unsigned long long x, char *buf, unsigned int length);
#endif
