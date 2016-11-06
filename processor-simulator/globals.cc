#include "processor.h"


/////////////
// GLOBALS
/////////////

// The processing core(s).
processor **PROC;

// Output file pointer.
FILE *fp_info;


#if !defined(__CYGWIN32__)
// 9/3/05 ER
#include <monetary.h>
#include <locale.h>
char *comma(unsigned long long x, char *buf, unsigned int length) {
        // set locale to U.S.
        setlocale(LC_ALL, "en_US");

        strfmon(buf, length, "%!.0n", (double)x);

        return(buf);
}
#endif
