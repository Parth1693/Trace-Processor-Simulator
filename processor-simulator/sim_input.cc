#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include "processor.h"
#include "globals.h"
#include "common.h"

extern "C" {
#include "misc.h"
}

/////////////////////////////////////////////////////////
// For use by qsort.
// This function is used for sorting simulation args,
// a necessary step in generating the output filename.
/////////////////////////////////////////////////////////
int compare_args(const void *fa, const void *fb) {
	char **a;
	char **b;
	char A;
	char B;

	a = (char **)fa;
	b = (char **)fb;

	A = (*a)[1];
	B = (*b)[1];

	return( A - B );
}

//////////////////////////////////////////////////////////////////////////
// This function overrides the defaults set in the library.
// The library is common to all simulators; this function
// allows the simulators to use their own defaults.
//////////////////////////////////////////////////////////////////////////
void set_defaults() {
}	// set_defaults()


/* simulator-specific options */
char *sim_optstring = "A:D:ef:iI:kK:oPS:w:z:Z:";
 
void
sim_options(int argc, char **argv)
{
  char c;
  unsigned int i;
 
  bool user_specified_output = false;
  char output_filename[1024];

  time_t t;
  char timestring[32];

  //////////////////////////////////////////////////////////////////////////
  // Set up defaults that I tend to use often in this simulator.
  //////////////////////////////////////////////////////////////////////////
  set_defaults();
 
  /* parse options */
  getopt_init();
  while ((c = getopt_next(argc, argv, sim_optstring)) != EOF)
    {
      switch (c) {
	 case 'A':
	    L1_DC_MISS_LATENCY = L1_IC_MISS_LATENCY = atoi(getopt_arg);
	    break;

	 case 'D':
            sscanf(getopt_arg, "%d,%d,%d,%d,%d,%d,%d,%d",
		&L1_DC_SETS, &L1_DC_ASSOC, &L1_DC_LINE_SIZE,
		&L1_DC_HIT_LATENCY, &L1_DC_MISS_LATENCY,
                &L1_DC_NUM_MHSRs,
		&L1_DC_MISS_SRV_PORTS, &L1_DC_MISS_SRV_LATENCY);
	    break;

	 case 'e':
	    PERFECT_FETCH = true;
	    break;

	 case 'f':
	    user_specified_output = true;
	    strcpy(output_filename, "mt.");
	    strcat(output_filename, getopt_arg);
	    break;

	 case 'i':
	    IN_ORDER_ISSUE = true;
	    break;

	 case 'I':
            sscanf(getopt_arg, "%d,%d,%d,%d,%d,%d,%d,%d",
		&L1_IC_SETS, &L1_IC_ASSOC, &L1_IC_LINE_SIZE,
		&L1_IC_HIT_LATENCY, &L1_IC_MISS_LATENCY,
                &L1_IC_NUM_MHSRs,
		&L1_IC_MISS_SRV_PORTS, &L1_IC_MISS_SRV_LATENCY);
	    break;

	 case 'k':
	    PERFECT_DCACHE = true;
	    break;

	 case 'K':
	    unsigned int temp1, temp2, temp3;
	    sscanf(getopt_arg, "%d,%d,%d", &temp1, &temp2, &temp3);
	    PERFECT_ICACHE = (bool) temp1;
	    IC_SINGLE_BB = (bool) temp2;
	    IC_INTERLEAVED = (bool) temp3;
	    break;

	 case 'o':
	    ORACLE_DISAMBIG = true;
	    break;

         case 'P':
            PERFECT_BRANCH_PRED = true;
            break;

	 case 'S':
	    sscanf(getopt_arg, "%d,%d,%d,%d,%d,%d", &FETCH_QUEUE_SIZE, &NUM_CHECKPOINTS, &ACTIVE_LIST_SIZE, &ISSUE_QUEUE_SIZE, &LQ_SIZE, &SQ_SIZE);
	    break;

         case 'w':
	    sscanf(getopt_arg, "%d,%d,%d,%d", &FETCH_WIDTH, &DISPATCH_WIDTH, &ISSUE_WIDTH, &RETIRE_WIDTH);
            break;

	 case 'z':
	    USE_INSTR_LIMIT = true;
	    INSTR_LIMIT = atoi(getopt_arg);
	    break;

	 case 'Z':
	    SKIP_AMT = atoi(getopt_arg);
	    break;

         default:
            break;
      }
    }

    // 9/3/05 ER: User now specifies *delta* to run after skip amount.
    if (USE_INSTR_LIMIT)
       INSTR_LIMIT = (SKIP_AMT + INSTR_LIMIT);

  ///////////////////////////////////////////////
  // Compute the output filename.
  // The filename is derived from non-default
  // simulation settings (i.e. sorted args).
  ///////////////////////////////////////////////
  int sorted_argc;
  char **sorted_argv;

  if (!user_specified_output) {
     sorted_argc = 0;
     sorted_argv = new char *[argc];

     i = 1;
     while ( (i < argc) && (argv[i][0] == '-') ) {
        sorted_argv[sorted_argc] = argv[i];
        sorted_argc += 1;

        i += 1;
     }

     qsort(sorted_argv, sorted_argc, sizeof(char *), compare_args);

     // Now create the output filename from sorted argument list.

     strcpy(output_filename, "mt.");
     if (sorted_argc == 0) {
	strcat(output_filename, "base");
     }
     else {
        for (i = 0; i < sorted_argc; i++)
	   strcat(output_filename, sorted_argv[i]);
     }
  }

  fp_info = fopen(output_filename, "r");
  if (fp_info) {
     // fatal("Output file '%s' already exists.", output_filename);
     fclose(fp_info);

     //time(&t);
     //cftime(timestring, ".%b%d.%T", &t);

     time(&t);
#if 0
     strftime(timestring, 32, ".%b%d.%T", localtime(&t));
#else
     strftime(timestring, 32, ".%b%d.%H.%M.%S", localtime(&t));
#endif
     strcat(output_filename, timestring);

     fp_info = fopen(output_filename, "r");
     assert(!fp_info);
  }
  fp_info = fopen(output_filename, "w");

  ///////////////////////////////////////////////
  // Print out Simulation Parameters:
  ///////////////////////////////////////////////

  INFO("************************ PARAMETERS *************************");
 
  INFO("**ARGS**");
  for (i = 0; i < argc; i++)
     INFO("%s", argv[i]);

  INFO("--------Benchmark Control.--------------");

  // 9/3/05 ER: Better display of simulation range.
  char from[100], to[100];
#if !defined(__CYGWIN32__)
  INFO("FROM -> TO = %s -> %s",
	(SKIP_AMT ? comma(SKIP_AMT, from, 100) : "begin"),
        (USE_INSTR_LIMIT ? comma(INSTR_LIMIT, to, 100) : "end"));
#else
  INFO("FROM -> TO = %s -> %s",
	(SKIP_AMT ? (snprintf(from, 100, "%d", SKIP_AMT), from) : "begin"),
        (USE_INSTR_LIMIT ? (snprintf(to, 100, "%d", INSTR_LIMIT), to) : "end"));
#endif
  //INFO("USE_INSTR_LIMIT = %d", USE_INSTR_LIMIT ? 1 : 0);
  //INFO("SKIP_AMT        = %d", SKIP_AMT);
  //INFO("INSTR_LIMIT     = %d", INSTR_LIMIT);

  INFO("--------Oracle Controls.----------------");

  INFO("PERFECT_BRANCH_PRED = %d", (PERFECT_BRANCH_PRED ? 1 : 0));
  INFO("PERFECT_FETCH = %d", (PERFECT_FETCH ? 1 : 0));
  INFO("ORACLE_DISAMBIG = %d", (ORACLE_DISAMBIG ? 1 : 0));
  INFO("PERFECT_ICACHE = %d", (PERFECT_ICACHE ? 1 : 0));
  INFO("PERFECT_DCACHE = %d", (PERFECT_DCACHE ? 1 : 0));

  INFO("--------Core.---------------------------");

  INFO("FETCH_QUEUE_SIZE = %d", FETCH_QUEUE_SIZE);
  INFO("NUM_CHECKPOINTS = %d", NUM_CHECKPOINTS);
  INFO("ACTIVE_LIST_SIZE = %d", ACTIVE_LIST_SIZE);
  INFO("ISSUE_QUEUE_SIZE = %d", ISSUE_QUEUE_SIZE);
  INFO("LQ_SIZE = %d", LQ_SIZE);
  INFO("SQ_SIZE = %d", SQ_SIZE);
  INFO("FETCH_WIDTH = %d", FETCH_WIDTH);
  INFO("DISPATCH_WIDTH = %d", DISPATCH_WIDTH);
  INFO("ISSUE_WIDTH = %d", ISSUE_WIDTH);
  INFO("RETIRE_WIDTH = %d", RETIRE_WIDTH);
  INFO("IC_INTERLEAVED = %d", (IC_INTERLEAVED ? 1 : 0));
  //INFO("IC_SINGLE_BB   = %d", (IC_SINGLE_BB ? 1 : 0));
  //INFO("IN_ORDER_ISSUE = %d", (IN_ORDER_ISSUE ? 1 : 0));

  INFO("--------L1 Data Cache.------------------");

  if (L1_DC_ASSOC > 1) {
     INFO("*** D$: %dKB, %d-way set-associative, %dB line size",
		(L1_DC_SETS*L1_DC_ASSOC*(1 << L1_DC_LINE_SIZE))/1024,
		L1_DC_ASSOC,
		(1 << L1_DC_LINE_SIZE));
  }
  else {
     INFO("*** D$: %dKB, direct-mapped, %dB line size",
		(L1_DC_SETS*L1_DC_ASSOC*(1 << L1_DC_LINE_SIZE))/1024,
		(1 << L1_DC_LINE_SIZE));
  }
  INFO("\tL1_DC_SETS             = %d", L1_DC_SETS);
  INFO("\tL1_DC_ASSOC            = %d", L1_DC_ASSOC);
  INFO("\tL1_DC_LINE_SIZE        = %d", L1_DC_LINE_SIZE);
  INFO("\tL1_DC_HIT_LATENCY      = %d", L1_DC_HIT_LATENCY);
  INFO("\tL1_DC_MISS_LATENCY     = %d", L1_DC_MISS_LATENCY);
  INFO("\tL1_DC_NUM_MHSRs        = %d", L1_DC_NUM_MHSRs);
  INFO("\tL1_DC_MISS_SRV_PORTS   = %d", L1_DC_MISS_SRV_PORTS);
  INFO("\tL1_DC_MISS_SRV_LATENCY = %d", L1_DC_MISS_SRV_LATENCY);

  INFO("--------L1 Instruction Cache.-----------");

  if (L1_IC_ASSOC > 1) {
     INFO("*** I$: %dKB, %d-way set-associative, %dB line size",
		(L1_IC_SETS*L1_IC_ASSOC*(1 << L1_IC_LINE_SIZE))/1024,
		L1_IC_ASSOC,
		(1 << L1_IC_LINE_SIZE));
  }
  else {
     INFO("*** I$: %dKB, direct-mapped, %dB line size",
		(L1_IC_SETS*L1_IC_ASSOC*(1 << L1_IC_LINE_SIZE))/1024,
		(1 << L1_IC_LINE_SIZE));
  }
  INFO("\tL1_IC_SETS             = %d", L1_IC_SETS);
  INFO("\tL1_IC_ASSOC            = %d", L1_IC_ASSOC);
  INFO("\tL1_IC_LINE_SIZE        = %d", L1_IC_LINE_SIZE);
  INFO("\tL1_IC_HIT_LATENCY      = %d", L1_IC_HIT_LATENCY);
  INFO("\tL1_IC_MISS_LATENCY     = %d", L1_IC_MISS_LATENCY);
  INFO("\tL1_IC_NUM_MHSRs        = %d", L1_IC_NUM_MHSRs);
  INFO("\tL1_IC_MISS_SRV_PORTS   = %d", L1_IC_MISS_SRV_PORTS);
  INFO("\tL1_IC_MISS_SRV_LATENCY = %d", L1_IC_MISS_SRV_LATENCY);

  INFO("----------------------------------------");

  fflush(fp_info);
}
