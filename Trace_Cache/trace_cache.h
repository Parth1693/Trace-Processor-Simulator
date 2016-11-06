// Trace cache input config parameters:
// 1. Cache line size 
// 2. Associativity
// 3. Cache size
// 4. Number of embedded blocks (m)

	//Struct for a trace cache block.
	typedef struct {
		int validBit;		     //validBit == 1 is valid.
		
		int start_pc;
		unsigned int tag;		 //Decimal value of tag and index.	      
		int branch_mask;
		int branch_flags;
		int taken_addr;
		int fall_addr;

		bool end_branch;
		trace_payload trace; 
	} trace_block;
			
	//Struct to hold access statistics for cache.
	typedef struct  {
		int reads;
		int readMisses;
		int writes;
		float missRate;		
	} trace_stats;	

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

	int accessCount;
	
	//block struct to hold information for each memory block in cache.
	trace_block **tr_cache;   		

	//cacheStats struct to hold run statistics.
	trace_stats tr_stats;

	//Array to hold LRU information for L1
	int **LRU;
	
	//Member functions

	trace_cache (int assoc, int block, int size, int m);    //Constructor
	void init_tr_cache();
	bool tr_access(int pc, int branchflag);		//Arg should be trace id from predictor?!
	trace_payload * get_trace(int pc, int branch_flags, int num_branches);
	void put_trace(int pc, int branch_flags, int num_branches, trace_payload filltrace);

	void initLRU();
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void tr_print_state();	
	void tr_print_stats();	

};   //Class Cache ends here.

/*Header file ends here*/

