/* This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin, University of Wisconsin - Madison, Computer Sciences
 * Department as a part of the Multiscalar Research Project.
 *  
 * The SimpleScalar x86 port was written by Steve Bennett.
 * 
 * The tool suite is currently maintained by Doug Burger.
 * 
 * Copyright (C) 1994, 1995, 1996 by Todd M. Austin
 *
 * This source file is distributed in the hope that it will be useful,
 * but without any warranty.  No author or distributor accepts
 * any responsibility for the consequences of its use.
 *
 * Everyone is granted permission to copy, modify and redistribute
 * this source file under the following conditions:
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#ifdef SIM_LINUX	/* 11/5/04 ERIC_CHANGE */
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <string.h>	/* 10/5/96 ERIC_CHANGE */

#include "Thread.h"

/* 10/5/96 ERIC_CHANGE: added this for debugging. (not declared anywhere!) */
#ifdef DEBUG
int debugging = 1;
#endif

/* PRT: added per email from Todd Austin */
#ifdef linux
#ifdef SIM_LINUX
#include <fpu_control.h>
#else
#include <i386/fpu_control.h>
#endif /* linux */
#endif

#include "misc.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "ss.h"
#include "_endian.h"
#include "version.h"
#include "sim.h"

/* stats signal handler */
static void
signal_sim_stats(int sigtype)
{
  sim_dump_stats = TRUE;
}

/* exit signal handler */
static void
signal_exit_now(int sigtype)
{
  sim_exit_now = TRUE;
}

static void
usage(int argc, char **argv)
{
  fprintf(stderr, "Usage: %s [-options] exec args...\n", argv[0]);
  exit(1);
}

void
exit_now(int exit_code)
{
  sim_stats(stderr);
  for (unsigned int i = 0; i < NumThreads; i++)
     THREAD[i]->mem_stats(stderr);
  exit(exit_code);
}

/* execution start time */
time_t start_time;

/* byte/word swapping required to execute target executable on this host */
int sim_swap_bytes;
int sim_swap_words;

/////////////////////////////////////////////////////////////////
// Multiple threads.
Thread *THREAD[MAX_THREADS];
unsigned int NumThreads = 0;

#define JOB_ARGS	64

void tokenize(char *job, int& argc, char **argv) {
  char delimit[4] = " \t\n";	// tokenize based on "space", "tab", eol

  argc = 0;
  while ((argc < JOB_ARGS) &&
         (argv[argc] = strtok((argc ? (char *)NULL : job), delimit))) {
     argc++;
  }

  if (argc == 0)
     fatal("No thread specified.");
}

void init_threads(FILE *fp, char **envp) {
  unsigned int i;
  char job[256];
  int argc;
  char *argv[JOB_ARGS];

  i = 0;
  while (i < MAX_THREADS && fgets(job, 256, fp)) {
     tokenize(job, argc, argv);
     THREAD[i] = new Thread(argc, argv, envp);
     i++;
  }

  NumThreads = i;
  if (NumThreads == 0)
     fatal("No threads started.");
}

// 12/26/99 ER: add fork capability.
void ss_fork(Thread *master, SS_ADDR_TYPE ThreadIdPtr, unsigned int id) {
  if (NumThreads == MAX_THREADS)
     fatal("SS_SYS_fork: no more threads available.");

  // Create a (slave) thread.
  THREAD[NumThreads] = new Thread(master, ThreadIdPtr, id);
  NumThreads++;
}

// 12/28/99 ER: wakeup threads blocked on a lock.
void ss_unlock(SS_ADDR_TYPE lockaddr) {
  for (unsigned int i = 0; i < NumThreads; i++)
     THREAD[i]->wakeup(lockaddr);
}
/////////////////////////////////////////////////////////////////


int	/* 11/5/04, 2/28/05: ERIC_CHANGE */
main(int argc, char **argv, char **envp)
{
  int i, exit_code, exec_index;
  char c, *all_options, *s;
#if 0
  /************************************************************************/
  /* 10/5/96 ERIC_CHANGE : Why did Todd use this?  It messes linker up... */
  /************************************************************************/
  extern char *strrchr(char *, char);
#endif

  /* opening banner */

  /* 8/20/05 ER: Add banner for 721 simulator. */
  fprintf(stderr,
	  "\n\nCopyright (c) 1999-2005 by Eric Rotenberg.  All Rights Reserved.\n");
  fprintf(stderr,
	  "Welcome to the ECE 721 Simulator.  This is a custom simulator\n");
  fprintf(stderr,
          "developed at North Carolina State University by Eric Rotenberg\n");
  fprintf(stderr,
	  "and his students.  It uses the Simplescalar ISA and only those\n");
  fprintf(stderr,
	  "files from the Simplescalar toolset needed to functionally\n");
  fprintf(stderr,
	  "simulate a Simplescalar binary, copyright below:\n");

#if 0
  fprintf(stderr,
	  "%s: Version %d.%d.%d of %s.\n"
	  "Copyright (c) 1994-1995 by Todd M. Austin.  All Rights Reserved.\n",
	  ((s = strrchr(argv[0], '/')) ? s+1 : argv[0]),
	  VER_MAJOR, VER_MINOR, VER_REVISION, VER_UPDATE);
#else
  fprintf(stderr,
	  "Copyright (c) 1994-1995 by Todd M. Austin.  All Rights Reserved.\n\n");
#endif

  /* PRT: added per email from Todd Austin */
  #ifdef linux
  /* get linux to perform 64-bit FP (not IA 80-bit) for compatibility */
  {
    int cw = (_FPU_DEFAULT & ~(3 << 8)) | _FPU_DOUBLE;
#ifdef SIM_LINUX
    _FPU_SETCW(cw);
#else
    __setfpucw(cw);
#endif
  }
  #endif /* linux */

  /* PRT: added per email from Todd Austin */
  /* FIXME: should ignore FP faults only in speculative mode */
  signal(SIGFPE, SIG_IGN);

  /* print out the program arguments */
  fprintf(stderr, "**ARGS**: ");
  for (i=0; i<argc; i++)
    fprintf(stderr, "%s ", argv[i]);
  fprintf(stderr, "\n");

  if (argc < 2)
    usage(argc, argv);

  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR1, signal_sim_stats);

  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR2, signal_exit_now);

  /* register an error handler */
  fatal_hook(sim_stats);

  /* set up a non-local exit point */
  if ((exit_code = setjmp(sim_exit_buf)) != 0)
    {
      /* special handling as longjmp cannot pass 0 */
      exit_now(exit_code-1);
    }

  /* compute legal options */
  all_options = getopt_combine_options("v", sim_optstring);
  all_options = getopt_combine_options(all_options, mem_optstring);

  /* parse global options */
  getopt_init();
  while ((c = getopt_next(argc, argv, all_options)) != EOF)
    {
      switch (c) {
      case 'v':
	verbose = TRUE;
	break;
      case '?':
	usage(argc, argv);
      }
    }
  exec_index = getopt_index;

  /* parse simulator options */
  sim_options(argc, argv);
  mem_options(argc, argv);

  if (exec_index == argc)
    usage(argc, argv);

  /* initialize the instruction decoder */
  ss_init_decoder();

  // 10/3/99 ER: Added support for multiple (funcsim) threads.
  FILE *fp = fopen(argv[exec_index], "r");
  if (fp)
     init_threads(fp, envp);
  else
     fatal("Could not open the job file '%s'.", argv[exec_index]);

  /* record start of execution time, used in rate stats */
  start_time = time((time_t *)NULL);

  /* output simulation conditions */
  sim_config(stderr);
  mem_config(stderr);

  sim_main();

  panic("returned to main");
}
