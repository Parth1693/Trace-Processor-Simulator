#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "mt.h"
#include "mt_trace_consume.h"
#include "info.h"
#include "common.h"
#include "macros.h"
#include "Thread.h"
//#include "parameters.h" // 08/27/04: ER


// LibSS
#define NO_ICHECKS
#include "misc.h"
#include "ss.h"
#include "regs.h"
#define mem_table local_mem_table
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "sim.h"


// 08/27/04: ER
bool USE_INSTR_LIMIT = false;
SS_TIME_TYPE INSTR_LIMIT = 100000000;


/////////////////////////////////////////////////////////////////////

/* configure the execution engine */
#define SET_NPC(EXPR)		(next_PC = (EXPR))
#define CPC			(regs_PC)

#define GPR(N)			(local_regs_R[N])
#define GPR_D(N)		(local_regs_R[N >> 1])
#define SET_GPR(N,EXPR)		(local_regs_R[N] = (EXPR))

#define FPR_L(N)		(regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs_F.d[(N) >> 1] = (EXPR))

#define SET_HI(EXPR)		(regs_HI = (EXPR))
#define HI			(regs_HI)
#define SET_LO(EXPR)		(regs_LO = (EXPR))
#define LO			(regs_LO)
#define FCC			(regs_FCC)
#define SET_FCC(EXPR)		(regs_FCC = (EXPR))

#define READ_WORD(SRC)							\
  (/* num_refs++, */MEM_WORD(SRC))
#define READ_UNSIGNED_HALF(SRC)						\
  (/* num_refs++, */(unsigned long)((unsigned short)MEM_HALF(SRC)))
#define READ_SIGNED_HALF(SRC)						\
  (/* num_refs++, */(signed long)((signed short)MEM_HALF(SRC)))
#define READ_UNSIGNED_BYTE(SRC)						\
  (/* num_refs++, */(unsigned long)((unsigned char)MEM_BYTE(SRC)))
#define READ_SIGNED_BYTE(SRC)						\
  (/* num_refs++, */(unsigned long)((signed long)((signed char)MEM_BYTE(SRC))))

#define WRITE_WORD(SRC, DST)						\
  (/* num_refs++, */MEM_WORD(DST) = (unsigned long)(SRC))
#define WRITE_HALF(SRC, DST)						\
  (/* num_refs++, */MEM_HALF(DST) = (unsigned short)((unsigned long)(SRC)))
#define WRITE_BYTE(SRC, DST)						\
  (/* num_refs++, */MEM_BYTE(DST) = (unsigned char)((unsigned long)(SRC)))

// 06/09/01 ER
#if 0
#define SYSCALL(INST)		(ss_syscall(mem_access, INST))
#else
#define SYSCALL(INST)		(trap_stop(INST))
#endif


#define DEFFU(FU,DESC)
#define DEFICLASS(ICLASS,DESC)
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,O2,I1,I2,I3,EXPR,L,TEXPR1,TEXPR2)
#define DEFLDST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,O2,I1,I2,I3,EXPR,DIRECT)
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)
#define CONNECT(OP)
#define IMPL
#include "ss.def"
#undef DEFFU
#undef DEFICLASS
#undef DEFINST
#undef DEFLDST
#undef DEFLINK
#undef CONNECT
#undef IMPL


void Thread::func_sim() {
  register SS_INST_TYPE inst;
#undef mem_table
  register char **local_mem_table = mem_table;
#define mem_table local_mem_table

	/////////////////////////////
	// SIMULATE ONE INSTRUCTION
	/////////////////////////////

        /* maintain $r0 semantics */
        local_regs_R[0] = 0;

        /* keep an instruction count */
        num_insn++;

        inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, regs_PC);


	// Start a new debug entry.
	DB->start();

        switch (SS_OPCODE(inst)) {
#define DEFFU(FU,DESC)
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR,L,TEXPR1,TEXPR2) \
        case OP:							\
	  TEXPR1;							\
	  EXPR;								\
	  TEXPR2;							\
	  /**** Pass instruction to the debug buffer. ****/		\
	  if (FLAGS & F_STORE)						\
	     get_arch_mem_value(store_addr, &real_upper, &real_lower);	\
	  DB->push_instr_actual(inst, FLAGS, L, regs_PC, next_PC,	\
				real_upper, real_lower);		\
	  break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
	  panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "ss.def"
#undef DEFFU
#undef DEFINST
#undef DEFLINK
#undef CONNECT
        }

	/////////////////////////////////////////////////////////////////////

        /* execute next instruction */
        regs_PC = next_PC;
        next_PC += 8;

        /////////////////////////////////////////////////////////
        // Exit simulator if we exceeded instruction limit.
        /////////////////////////////////////////////////////////
        if (USE_INSTR_LIMIT && num_insn > INSTR_LIMIT)
           longjmp(sim_exit_buf, 0);

}	// func_sim()
