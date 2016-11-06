
///////////////////////////////////////////////////////////////
// Global Load/Store Unit including:
// 1. Disambiguation unit in the form of a load/store queue.
// 2. Data cache (only tags, for determining hit or miss).
// 3. Committed memory state.
///////////////////////////////////////////////////////////////

// Single entry in the load-store queue.
typedef struct {
	bool valid;		// this entry holds an active load or store

	bool is_signed;		// signed (true) or unsigned (false)
	bool left;		// LWL or SWL
    bool right;		// LWR or SWR
	int size;		// 1, 2, 4, or 8 bytes

	bool addr_avail;	// address is available
	SS_ADDR_TYPE addr;	// address
	SS_WORD_TYPE back_data; // background data for LWL/LWR

	bool value_avail;		// value is available
	SS_WORD_TYPE value[2];	// value (up to two words)

	bool missed;				// The memory block referenced by load or store is not in cache.
	SS_TIME_TYPE miss_resolve_cycle;	// Cycle when referenced memory block will be in cache.

	// These three fields are needed for replaying stalled loads.
	unsigned int pay_index;	// Index into PAY buffer.
	unsigned int sq_index;	// SQ index of stalled load.
	bool sq_index_phase;

	// STATS
	bool stat_load_stall_disambig;	// Load stalled due to unknown store address and/or value.
	bool stat_load_stall_miss;	// Load stalled due to a cache miss.
	bool stat_store_stall_miss;	// Store commit stalled due to a cache miss.
	bool stat_forward;		// Load received value from store in LSQ.
} lsq_entry;


class lsu {

private:

	//////////////////////////
	// Load Queue
	////////////////////////// 
	lsq_entry *LQ;		// Loads reside in this data structure between dispatch and retirement.
	unsigned int lq_size;
	//unsigned int lq_head;
	//unsigned int lq_tail;
	int lq_length;

	// These extra bits enable distinguishing between a full queue versus an empty queue when head==tail.
	bool lq_head_phase;
	bool lq_tail_phase;

	// Stalled loads are replayed one per cycle.
	// This is the index of the next stalled load to replay.
	// A -1 replay index indicates there are no stalled loads.
	int replay_index;

	//////////////////////////
	// Store Queue
	////////////////////////// 
	lsq_entry *SQ;		// Stores reside in this data structure between dispatch and retirement.
	unsigned int sq_size;
	unsigned int sq_head;
	unsigned int sq_tail;
	int sq_length;

	// These extra bits enable distinguishing between a full queue versus an empty queue when head==tail.
	bool sq_head_phase;
	bool sq_tail_phase;

	//////////////////////////
	// Data Cache
	////////////////////////// 
	CacheClass DC;
	unsigned int Tid;

	//////////////////////////
	// Memory
	////////////////////////// 
	char *mem_table[MEMORY_TABLE_SIZE];

	//////////////////////////
	// STATS
	//////////////////////////
	// Number of retired loads that stalled for disambiguation.
	unsigned int n_stall_disambig;

	// Number of retired loads that received values from stores in LSQ.
	unsigned int n_forward;

	// Number of retired loads (l) or stores (s) that cache missed.
	unsigned int n_stall_miss_l;
	unsigned int n_stall_miss_s;

	// Number of loads and stores.
	unsigned int n_load;
	unsigned int n_store;

	//////////////////////////
	//  Private functions
	//////////////////////////

	// The core disambiguation algorithm.
	//
	// Inputs:
	// 1. LQ/SQ indices of load being checked (four pieces of info)
	//
	// Outputs:
	// 1. (return value): stall load if function returns true
	// 2. forward: forward value from prior dependent store
	// 3. store_entry: if forwarding is indicated, this identifies the store
	//
	bool disambiguate(unsigned int lq_index,
			  unsigned int sq_index, bool sq_index_phase,
			  bool& forward,
			  unsigned int& store_entry, bool &cheat);

	// The load execution datapath.
	void execute_load(SS_TIME_TYPE cycle,
			  unsigned int lq_index,
			  unsigned int sq_index, bool sq_index_phase,
			  unsigned int load_low_data, unsigned int load_up_data);

	// Determine which stalled load should be replayed in the next cycle.
	void next_replay_index(bool restart);

	// Allocate a chunk of memory.
	char *mem_newblock(void);


public:
	unsigned int lq_head;
	unsigned int lq_tail;
	lsu(unsigned int lq_size, unsigned int sq_size, unsigned int Tid);		// constructor
  
	bool stall(unsigned int bundle_load, unsigned int bundle_store);

	void dispatch(bool load, unsigned int size, bool left, bool right, bool is_signed,
		      unsigned int pay_index,
                      unsigned int& lq_index, bool& lq_index_phase,
		      unsigned int& sq_index, bool& sq_index_phase);

	void store_addr(SS_TIME_TYPE cycle,
			unsigned int addr,
			unsigned int sq_index,
			unsigned int lq_index, bool lq_index_phase);
	void store_value(unsigned int sq_index, SS_WORD_TYPE value0, SS_WORD_TYPE value1);

	bool load_addr(SS_TIME_TYPE cycle,
		       unsigned int addr,
		       unsigned int lq_index,
		       unsigned int sq_index, bool sq_index_phase,
		       SS_WORD_TYPE back_data, // LWL/LWR
		       SS_WORD_TYPE& value0,
		       SS_WORD_TYPE& value1,
			   unsigned int load_low_data,
			   unsigned int load_up_data);
	bool load_replay(SS_TIME_TYPE cycle, unsigned int& pay_index, SS_WORD_TYPE& value0, SS_WORD_TYPE& value1);

	void checkpoint(unsigned int& chkpt_lq_tail, bool& chkpt_lq_tail_phase,
			unsigned int& chkpt_sq_tail, bool& chkpt_sq_tail_phase);
	void restore(unsigned int recover_lq_tail, bool recover_lq_tail_phase,
		     unsigned int recover_sq_tail, bool recover_sq_tail_phase);
  
	void commit(bool load);

	void flush();

	void copy_mem(char **master_mem_table);

	// STATS
	void stats(FILE *fp);
}; 
