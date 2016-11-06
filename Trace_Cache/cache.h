// Trace cache input config parameters:
// 1. Cache line size 
// 2. Associativity
// 3. Cache size
// 4. Number of embedded blocks (m)

	//Struct for a trace cache block.
	typedef struct {
		int validBit;		     //validBit == 1 is valid.
		
		int first_pc;
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
		int writeMisses;
		int missRate;
		int writeBacks;
		int swapRequests;
		int swaps;
		
	} trace_stats;	

class trace_cache {
	public:
	int block_size;
	int cache_size;
	int cache_assoc;
		
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

	Cache (int, int, int, int, int);    //Constructor
	void initCache();
	void Request(const char *c, unsigned int address, CacheL2 L2);
	void initLRU();
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void printState();	
	void printStats();	
	//void printLRU();

	void initVC();
	void initvcLRU();
	void updatevcLRU(int way);
	void writeVC(unsigned int, int, CacheL2);
	unsigned int getvcLRU();
	int searchVC(unsigned int);
	void printVC();
	//void freeMemory();

};   //Class Cache ends here.

/*Header file ends here*/

