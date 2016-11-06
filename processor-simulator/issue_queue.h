#ifndef ISSUE_QUEUE_DEFINED
#define ISSUE_QUEUE_DEFINED

// +Parth
// Issue Queue which is present inside each PE.
// Modified to contain the instructions within a trace i.e. within PAY.buf[].
// Iterate over all the instruction in the particular
// trace_payload to implement issue_queue functions.

typedef struct {

// If true, it means an instruction is present in this issue queue entry.
bool valid;

// Index of instruction into the PAY.buf[].
unsigned int index;

// Branches that this instruction depends on.
//unsigned long long branch_mask;

// Execution lane that this instruction wants.
unsigned int lane_id;

// Valid bit, ready bit, and tag of first operand (A).
bool A_valid;		// valid bit (operand exists)
bool A_ready;		// ready bit (operand is ready)
unsigned int A_tag;	// Global or local physical register name
bool A_live_in;
bool A_local;

// Valid bit, ready bit, and tag of second operand (B).
bool B_valid;		// valid bit (operand exists)
bool B_ready;		// ready bit (operand is ready)
unsigned int B_tag;	// Global or local physical register name
bool B_live_in;
bool B_local;

bool completed;

} issue_queue_entry_t;


class issue_queue {
	private:
		issue_queue_entry_t *q;
		bool IQ_valid; 		//Valid bit for the IQ as a whole.
							//Info whether a trace is present in this IQ.

		// Index into the trace payload buffer.
		unsigned int index;

		unsigned int trace_size;		// Issue queue size == Max number of instructions in a trace.
		int length;						// Number of instructions in the trace kept in the issue queue.

		//Counter to keep the number of isntructions that have finished execution.
		int num_completed;

		unsigned int *fl;			// This is the list of free issue queue entries, i.e., the "free list".
		unsigned int fl_head;		// Head of issue queue's free list.
		unsigned int fl_tail;		// Tail of issue queue's free list.
		int fl_length;				// Length of issue queue's free list.

		void remove(unsigned int i);		// Remove the instruction in issue queue entry 'i' from the issue queue.
		
	public:
		issue_queue(unsigned int size);		// constructor
		bool stall(unsigned int bundle_inst);
		unsigned int dispatch(unsigned int index, unsigned int lane_id,
			      bool A_valid, bool A_ready, unsigned int A_tag, bool A_live_in, bool A_local,
			      bool B_valid, bool B_ready, unsigned int B_tag, bool B_live_in, bool B_local);
		void global_wakeup(unsigned int tag);
		void local_wakeup(unsigned int tag);
		void select_and_issue(unsigned int num_lanes, PE_lane *Execution_Lanes);
		void flush();		//Flush the IQ in case of a branch mispredict or exception in any previous trace.
		void set_complete(unsigned int i);	//Set the completed bit for the instruction in the issue queue.

		//void clear_branch_bit(unsigned int branch_ID);
		//void squash(unsigned int branch_ID);
};

#endif


