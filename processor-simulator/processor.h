#ifndef PROCESSOR_IS_INCLUDED
#define PROCESSOR_IS_INCLUDED

#include "Thread.h"		// simplescalar + thread functional simulator(s)
#include "parameters.h"		// simulator configuration
#include <map>
#include <vector>
#include <string>


//////////////////////////////////////////////////////////////////////////////

#include "bpred_interface.h"	// referenced by fetch unit
#include "memory_macros.h"	// referenced by memory system
#include "histogram.h"		// referenced by fetcn unit and memory system
#include "cache.h"		//	"
#include "dcache.h"		//	"
#include "fu.h"			// function unit types

//////////////////////////////////////////////////////////////////////////////

#include "payload.h"		// instruction payload buffer

#include "pipeline_register.h"	// PIPELINE REGISTERS

#include "renamer.h"		// REGISTER RENAMER + REGISTER FILE

#include "lane.h"		// EXECUTION LANES

#include "issue_queue.h"	// ISSUE QUEUE

#include "lsu.h"		// LOAD/STORE UNIT

//////////////////////////////////////////////////////////////////////////////

//Our new added files

#include "trace_cache.h"
#include "PE.h"
#include "activeList.h"
#include "trace_fill.h"
#include "trace_payload.h"
#include "trace_predictor.h"
#include "defs.h"

///////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////

#define IS_BRANCH(flags)	((flags) & (F_CTRL))
#define IS_LOAD(flags)          ((flags) & (F_LOAD))
#define IS_STORE(flags)         ((flags) & (F_STORE))
#define IS_MEM_OP(flags)        ((flags) & (F_MEM))

//////////////////////////////////////////////////////////////////////////////

#define BIT_IS_ZERO(x,i)	(((x) & (((unsigned long long)1) << i)) == 0)
#define BIT_IS_ONE(x,i)		(((x) & (((unsigned long long)1) << i)) != 0)
#define SET_BIT(x,i)		(x |= (((unsigned long long)1) << i))
#define CLEAR_BIT(x,i)		(x &= ~(((unsigned long long)1) << i))

//////////////////////////////////////////////////////////////////////////////

struct info_bit
{   unsigned int pc;
	unsigned int reconv_pt;   
	unsigned int tot_insn;
};


class processor {
	private:
		/////////////////////////////////////////////////////////////
		// Instruction payload buffer.
		// +Rad
		// Now this is main payload of all traces info
		/////////////////////////////////////////////////////////////
		payload	PAY;
		//trace_payload TR_PAY;		//It is our OTB!!!!! Centralized!!!!

		/////////////////////////////////////////////////////////////
		// Pipeline widths.
		/////////////////////////////////////////////////////////////
		unsigned int fetch_width;	// fetch, decode width
		unsigned int dispatch_width;	// rename, dispatch width
		unsigned int issue_width;	// issue width
		unsigned int retire_width;	// retire width

		/////////////////////////////////////////////////////////////
		// Trace Fetch unit.
		/////////////////////////////////////////////////////////////
		unsigned int pc;		// Speculative program counter.
		SS_TIME_TYPE next_fetch_cycle;	// Support for I$ miss stalls.
		bool slow_path_busy;   //+Rad -- It checks if my slow path sequencing is still going on
		unsigned int m;  //number of max blocks allowed
		unsigned int n;  //number of instructions allowed
		bool wait_for_trap;		// Needed to override perfect branch prediction after fetching a syscall.
		bpred_interface BP;		// Branch predictor.
		CacheClass IC;			// Instruction cache.

		// Global variable for Exception detection.
		bool global_exception;



		/////////////////////////////////////////////////////////////
		// +Rad
		// Pipeline register between the Trace Predictor and Trace Fetch Stages.
		/////////////////////////////////////////////////////////////	
		tr_fetch_register TR_FETCH;

		/////////////////////////////////////////////////////////////
		// +Rad
		// Pipeline register between the Trace Fetch and Trace Rename Stages.
		/////////////////////////////////////////////////////////////	
		trace_pipeline_register TR_RENAME;

		/////////////////////////////////////////////////////////////
		// Pipeline register between the trace rename and trace dispatch stages.
		/////////////////////////////////////////////////////////////
		trace_pipeline_register TR_DISPATCH;

		/////////////////////////////////////////////////////////////
		// Register renaming modules.
		/////////////////////////////////////////////////////////////
		renamer *REN_INT;
		renamer *REN_FP;

		/////////////////////////////////////////////////////////////
		// 	PE
		/////////////////////////////////////////////////////////////
		PE_t ** PE_array;
		
		/////////////////////////////////////////////////////////////
		// Load and Store Unit.
		/////////////////////////////////////////////////////////////
		lsu LSU;				

		//////////////////////
		// PRIVATE FUNCTIONS
		//////////////////////

		void copy_reg();
		unsigned int steer(fu_type fu, unsigned int pe_index);  //Modified
		void agen(unsigned int index);
		void alu(unsigned int index);
		void squash_complete(unsigned int offending_PC);
		void resolve(unsigned int branch_ID, bool correct);
		void checker();
		void check_single(SS_WORD_TYPE proc, SS_WORD_TYPE func, char *desc);
		void check_double(SS_WORD_TYPE proc0, SS_WORD_TYPE proc1, SS_WORD_TYPE func0, SS_WORD_TYPE func1, char *desc);

	public:
		/////////////////////////////////////////////////////////////		
		// BIT		
		/////////////////////////////////////////////////////////////		
		std::map < unsigned int, info_bit > BIT;
		
		SS_ADDR_TYPE ld_text_base;
		SS_ADDR_TYPE ld_text_size;

		//+Rad +Adding trace cache object
		trace_cache TC;
		predictor TP;
		trace_fill_unit TFill;
		unsigned int t_access;
		unsigned int t_hit;
		unsigned int t_miss;

		//DEBUG
		unsigned int last_info;
		unsigned int last_inst;

		/////////////////////////////////////////////////////////////
		// Active List
		/////////////////////////////////////////////////////////////
		AList *TR_AL;

		trace_payload TR_PAY;	
		// Committed PC
		unsigned int committed_pc;
		
		// The thread id.
		unsigned int Tid;

		// The simulator cycle.
		SS_TIME_TYPE cycle;

		/////////////////////////////////////////////////////////////
		// Stats
		/////////////////////////////////////////////////////////////

		
		unsigned int num_insn; // Number of instructions retired.
		unsigned int num_insn_split;
		unsigned int num_traces;	// Number of traces retired.
		unsigned int total_tr_size;
		double avg_tr_size;
		
		unsigned int tc_hits;
		unsigned int tc_misses;
		unsigned int tp_hits;
		unsigned int tp_misses;
		double avg_tc_hits;
		double avg_tc_misses;
		double avg_tp_hits;
		double avg_tp_misses;

		unsigned int total_live_ins;
		unsigned int total_live_outs;

		unsigned int num_fgci_br;
		unsigned int fgci_br;
		unsigned int num_br;
		double avg_fgci_br;
		unsigned int total_pc;
		unsigned int total_br_code;


		processor(unsigned int fq_size,
			  unsigned int num_chkpts,
			  unsigned int rob_size,
			  unsigned int iq_size,
			  unsigned int lq_size,
			  unsigned int sq_size,
			  unsigned int fetch_width,
			  unsigned int dispatch_width,
			  unsigned int issue_width,
			  unsigned int retire_width,
			  unsigned int fu_lane_matrix[],
			  unsigned int Tid);
		
		// Functions for pipeline stages.
		void trace_predict();
		void trace_fetch();
		void trace_rename();
		void trace_dispatch();
		void trace_schedule(unsigned int pe_num);
		void trace_register_read(unsigned int pe_num,unsigned int lane_number);
		void trace_execute(unsigned int pe_num,unsigned int lane_number);
		void trace_writeback(unsigned int pe_num,unsigned int lane_number);
		void trace_retire();

		// Miscellaneous other functions.
		void next_cycle();
		void stats(FILE *fp);
		void skip(unsigned int skip_amt);
		void update_bit();
};
#endif

