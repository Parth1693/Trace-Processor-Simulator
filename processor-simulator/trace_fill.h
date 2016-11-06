#ifndef TFILL_INCLUDED
#define TFILL_INCLUDED

#include "processor.h"

//This class has slow path fetching
// what all functions should it do?
// 1. access the ICache 2 way interleaved and get the next fetch cycle
// 2. form a trace and hence I need line fill buffer here
// 3. check if there are any branches in the fill when I reach last cycle of next fetch 
// 	and my trace_done flag is not set.
// 4. trace_done flag is set when i have at max 2 embedded branches and instruction count is 16
//    OR i have 2 embedded branches and now again I encounter a branch

class trace_fill_unit
{
	public:	
	payload_t    	buf[TRACE_SIZE*2];
	trace_info 	trace_fill_buffer;
	
	unsigned int max_n;
	unsigned int max_m;

	bool busy;
	bool trace_done;	
	unsigned int n;
	unsigned int m;
	unsigned int fill_br_flag;
	bool fill_ends_in_br;
	unsigned int return_pc;
	unsigned int now_servicing_pc;

	trace_fill_unit(unsigned int max_n, unsigned int max_m);
	
	void fill(unsigned int pc, bool &exit_flag, unsigned int br_flags, bool br_pred,
			unsigned int Tid, bool wait_for_trap);
	
	// Access Trace fill
	// All blocks of trace are filled as per the br_flags or branch predictor
	// This is the main function which fills the line fill buffer for entire trace with start_pc
	// Flag br_pred tells if we need to take predictions from br_flags of TPredictor or from branch predictor 
	// br_pred : 
	//		true = br_flags of Predictor
	//		false = branch predictor
	unsigned int trace_fill(CacheClass &IC, unsigned int start_pc, unsigned int br_flags, bool br_pred,
			bool partial, unsigned int mode4_n, unsigned int mode4_m, unsigned int mode4_br_flag,
			unsigned int Tid, SS_TIME_TYPE cycle, bool wait_for_trap);
	//unsigned int trace_fill_partial(CacheClass &IC, unsigned int start_pc, unsigned int br_flags, bool br_pred);
	// Access I$ for that block
	// Returns number of cycles it takes to get 
	SS_TIME_TYPE access_IC(CacheClass &IC, unsigned int pc, unsigned int Tid, SS_TIME_TYPE cycle);
	
	bool is_trace_done();
	//trace_info get_trace();
	void reset_trace_done();
	
	void pre_rename();
	void pre_decode();
	void split(unsigned int index);
	bool is_busy();
		
};

#endif

