// Trace cache input config parameters:
// 1. Cache line size 
// 2. Associativity
// 3. Cache size
// 4. Number of embedded blocks (m)
#ifndef TR_CACHE_INCLUDED
#define TR_CACHE_INCLUDED

#include "trace_payload.h"
#include "payload.h"
//Struct for a trace cache block.
typedef struct {
	int 		validBit;		     //validBit == 1 is valid.
	payload_t 	buf[TRACE_SIZE*2];
	unsigned int tag;
	trace_info	tr_info;	
} trace_block;

class trace_cache {
	public:
	int block_size;
	int cache_size;
	int cache_assoc;
	int num_embedded_branches;
		
	int num_sets;
	int num_ways;
	unsigned int offset_bits;
	unsigned int index_bits;
	unsigned int tag_bits;
	unsigned int address;
	unsigned int index;
	unsigned int tag;

	//TC stats
	unsigned int tc_hits;
	unsigned int tc_misses;
	unsigned int reads;
	unsigned int writes;

	int accessCount;
	//block struct to hold information for each memory block in cache.
	trace_block **tr_cache;   		

	//Array to hold LRU information for L1
	int **LRU;
	
	//Member functions

	trace_cache (int assoc, int block, int size, int m);    //Constructor
	void init_tr_cache();
	bool tr_access(int pc, int branchflag);		//Arg should be trace id from predictor?!
	trace_block get_trace(int pc, int branchflag);
	void put_trace(trace_block filltrace); //set valid here!!!

	void initLRU();
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void printState();	
	void printStats();	

};   //Class Cache ends here.

/*Header file ends here*/
#endif

