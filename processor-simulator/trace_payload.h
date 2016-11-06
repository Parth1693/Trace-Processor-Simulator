#ifndef TR_PAYLOAD_INCLUDED
#define TR_PAYLOAD_INCLUDED

#include "processor.h"
#include "defs.h"

typedef struct
{
	bool valid;
	unsigned int start_index;
	unsigned int end_index;
	unsigned int tr_size;

	//Trace information
	unsigned int br_mask; //number of embeded branches
	unsigned int br_flag; //Taken or not Taken for m branches
	bool ends_in_br;
	unsigned int taken_addr;
	unsigned int fall_addr;
	unsigned int start_pc;
	
	//unsigned int OTB_index;
	//unsigned int PE_index;	//Assert that these two are always equal.
	unsigned int AL_index;	// Index into global trace active List.
	
	unsigned int checkpoint_ID;	// Set to TOTAL_PE + 1 initially.

	//Trace predictor needed flags
	//To be checked at retire time with previous head trace.
	//Update TP using this info.
	unsigned int pred_index;	// Trace predictor's index in table
	unsigned int pred_pc;		//Set while dispatching instruction from T$
	unsigned int pred_br_flag;	//Taken or not taken for m branches
	
	bool trace_mispredict;		// Need to update TP. Call misprepdict() of TP.
	int mode5_pc;				// It is set by previous trace's PE. Used to fetch from new pc in fetch stage.
	
	bool completed;				
	bool exception;				// To signal that some inst in the trace encountered exception.
								// This is checked at head of AL at retire time.
							
	//Information to store after execution
	bool br_mispredict;			//Set by PE after branch resolve.
								//This is checked in trace_fetch in the OTB.
	int index_mispredict_inst;		//holds index of branch within trace that was mispredicted.
	int index_exception_inst;		//holds index of inst within trace that caused exception. Not needed in 721sim
	
} trace_info;

class trace_payload {
	public:

		trace_info  tr_buf[TOTAL_PE];
		unsigned int head;
		unsigned int tail;
		int          length;
		
		trace_payload();		// constructor
		unsigned int push();
		void pop();
		void clear();
		bool is_empty();
		bool is_full();
		//void split(unsigned int index);
		//void map_to_actual(unsigned int index, unsigned int Tid);
		void rollback(unsigned int index);
};
#endif


