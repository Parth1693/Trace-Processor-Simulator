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

#define HOST_ONLY
#include "endian.cc"

//void
main(void)
{
  int little_bytes = 0, little_words = 0;

  switch (endian_host_byte_order()) {
  case endian_big:
    fprintf(stdout, "-DBYTES_BIG_ENDIAN ");
    break;
  case endian_little:
    fprintf(stdout, "-DBYTES_LITTLE_ENDIAN ");
    little_bytes = 1;
    break;
  case endian_unknown:
    fprintf(stderr, "\nerror: cannot deterine byte order!\n");
    exit(1);
  default:
    abort();
  }

  switch (endian_host_word_order()) {
  case endian_big:
    fprintf(stdout, "-DWORDS_BIG_ENDIAN ");
    break;
  case endian_little:
    fprintf(stdout, "-DWORDS_LITTLE_ENDIAN ");
    little_words = 1;
    break;
  case endian_unknown:
    fprintf(stderr, "\nerror: cannot deterine word order!\n");
    exit(1);
  default:
    abort();
  }

  /* check for different byte/word endian-ness */
  if (little_bytes != little_words)
    {
      fprintf(stderr,
	      "\nerror: opposite byte/word endian currently not supported!\n");
      exit(1);
    }
  exit(0);
}

