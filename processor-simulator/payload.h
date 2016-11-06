#ifndef PAYLOAD_INCLUDED
#define PAYLOAD_INCLUDED

#define PAYLOAD_BUFFER_SIZE  2048

typedef
enum {
SEL_IQ_INT,		// Select integer IQ.
SEL_IQ_FP,		// Select floating-point IQ.
SEL_IQ_NONE,		// Skip IQ: mark completed right away.
SEL_IQ_NONE_EXCEPTION	// Skip IQ with exception: mark completed and exception right away.
} sel_iq;

union DOUBLE_WORD {
   unsigned long long dw;
   SS_WORD_TYPE w[2];
   float f[2];
   double d;
};

typedef struct {

	// +Parth
   ////////////////////////
   // Set by Slow Path Fetch Stage.
   ////////////////////////

   SS_INST_TYPE inst;           // The simplescalar instruction.
   unsigned int pc;		// The instruction's PC.
   unsigned int next_pc;	// The next instruction's PC. (I.e., the PC of
							// the instruction fetched after this inside the trace.)
   unsigned int pred_tag;       // If the instruction is a branch, this is its
				// index into the Branch Predictor's
				// branch queue.

   bool good_instruction;	// If 'true', this instruction has a
				// corresponding instruction in the
				// functional simulator. This implies the
				// instruction is on the correct control-flow
				// path.
   debug_index db_index;	// Index of corresponding instruction in the
				// functional simulator
				// (if good_instruction == 'true').
				// Having this index is useful for obtaining
				// oracle information about the instruction,
				// for various oracle modes of the simulator.
	int index_trace;	//Holds index number within its particular trace (inside trace_payload).
						//Set while line filling in slow path.

   ////////////////////////
   // Set by Trace Pre-decode Stage.
   ////////////////////////

   unsigned int flags;          // Operation flags: can be used for quickly
				// deciphering the type of instruction.
   fu_type fu;                  // Operation function unit type.
   unsigned int latency;        // Operation latency (ignore: not currently
				// used).
   bool split;			// Instruction is split into two.
   bool upper;			// If 'true': this instruction is the upper
				// half of a split instruction.
				// If 'false': this instruction is the lower
				// half of a split instruction.
   bool checkpoint;		// If 'true', this instruction is a branch  // Now we need this to check if it is branch in fill logic
					// that needs a checkpoint.
   bool split_store;		// Floating-point stores (S_S and S_D) are
				// implemented as split-stores because they
				// use both int and fp regs.

   // Source register A.
   bool A_valid;		// If 'true', the instruction has a
				// first source register.
   bool A_int;			// If 'true', the source register is an
				// integer register, else it is a
				// floating-point register.
   unsigned int A_log_reg;	// The logical register specifier of the
				// source register.

   // Source register B.
   bool B_valid;		// If 'true', the instruction has a
				// second source register.
   bool B_int;			// If 'true', the source register is an
				// integer register, else it is a
				// floating-point register.
   unsigned int B_log_reg;	// The logical register specifier of the
				// source register.

   // Destination register C.
   bool C_valid;		// If 'true', the instruction has a
				// destination register.
   bool C_int;			// If 'true', the destination register is an
				// integer register, else it is a
				// floating-point register.
   unsigned int C_log_reg;	// The logical register specifier of the
				// destination register.

   // IQ selection.
   sel_iq iq;			// The value of this enumerated type indicates
				// whether to place the instruction in the
				// integer issue queue, floating-point issue
				// queue, or neither issue queue.
				// (The 'sel_iq' enumerated type is also
				// defined in this file.)

   // Details about loads and stores.
   unsigned int size;		// Size of load or store (1, 2, 4, or 8 bytes).
   bool is_signed;		// If 'true', the loaded value is signed,
				// else it is unsigned.
   bool left;			// LWL or SWL instruction.
   bool right;			// LWR or SWR instruction.

   ////////////////////////
   // Set by Trace Pre-Rename Stage
   ////////////////////////

   // Flags to indicate type of the register operands:
   // Global live-in, global live-out and locals
   // Should we set both local and live_out flags for a register which
   // is both global live_out and local?
   // P.S: Local renaming of such registers is tricky. (Use both renamed regs)
   
   bool A_live_in;
   bool A_local;

   bool B_live_in;
   bool B_local;

	// C can be both live_out and local.
   bool C_live_out;
   bool C_local;

   unsigned int A_global_phys_reg;
   unsigned int B_global_phys_reg;
   unsigned int C_global_phys_reg;

   // Local physical registers assigned during pre-renaming.
   // Local renaming is done only for local source and destination registers.
   // If the corresponding bool flag declared above is set, these variables
   // the local renamed register numbers.
   unsigned int A_local_phys_reg;
   unsigned int B_local_phys_reg;
   unsigned int C_local_phys_reg;
   
   ////////////////////////
   // Set by Trace Fetch Stage
   ////////////////////////  
   
   //Allocate entry in TR_PAY's tr_buf
   //hence store its index here
   unsigned int trace_info_index;

   ////////////////////////
   // Set by Trace Global Rename Stage
   ////////////////////////

   //Global Physical registers.
   //These register numbers are used only if the corresponding register is a global live-in or live-out.
   unsigned int A_phys_reg;	// If there exists a first source register (A),
				// this is the physical register specifier to
				// which it is renamed.
   unsigned int B_phys_reg;	// If there exists a second source register (B),
				// this is the physical register specifier to
				// which it is renamed.
   unsigned int C_phys_reg;	// If there exists a destination register (C),
				// this is the physical register specifier to
				// which it is renamed.

				
   ////////////////////////
   // Set by PE in writeback stage.
   ////////////////////////
   bool is_resolved;    //set by PE in writeback
   bool is_mispredict;
   
   ////////////////////////
   // Set by Trace Dispatch Stage.
   ////////////////////////

   //unsigned int AL_index;	// Index into global trace Active List.
   // This info should be present in the trace_payload structure.

   //unsigned int AL_index_fp;	// Index into floating-point Active List.
   unsigned int LQ_index;	// Indices into LSU. Only used by loads, stores, and branches.
   bool LQ_phase;
   unsigned int SQ_index;
   bool SQ_phase;

   //Checkpoints for LSU
   unsigned int LQ_index_cp;	// Indices into LSU. Only used for checkpoint recovery.
   bool LQ_phase_cp;
   unsigned int SQ_index_cp;
   bool SQ_phase_cp;
	
   unsigned int lane_id;	// Execution lane chosen for the instruction.
   unsigned int iq_index;		//Index in the local PE FP or INT issue queue.

   ////////////////////////
   // Set by Reg. Read Stage. (Inside PE)
   ////////////////////////

   // Source values.
   DOUBLE_WORD A_value;		// If there exists a first source register (A),
				// this is its value. To reference the value as
				// an unsigned long long, use "A_value.dw".
   DOUBLE_WORD B_value;		// If there exists a second source register (B),
				// this is its value. To reference the value as
				// an unsigned long long, use "B_value.dw".

   ////////////////////////
   // Set by Execute Stage. (Inside PE)
   ////////////////////////

   // Load/store address calculated by AGEN unit.
   unsigned int addr;

   // Resolved branch target. (c_next_pc: computed next program counter)
   unsigned int c_next_pc;

   // Destination value.
   DOUBLE_WORD C_value;		// If there exists a destination register (C),
				// this is its value. To reference the value as
				// an unsigned long long, use "C_value.dw".

   ////////////////////////
   // Set by Writeback Stage.
   ////////////////////////
   bool completed;
   bool exception;

} payload_t;

class payload {
	public:
		////////////////////////////////////////////////////////////////////////
		//
		// The new 721sim explicitly models all processor queues and
		// pipeline registers so that it is structurally the same as
		// a real pipeline. To maintain good simulation efficiency,
		// however, the simulator holds all "payload" information about
		// an instruction in a centralized data structure and only
		// pointers (indices) into this data structure are actually
		// moved through the pipeline. It is not a real hardware
		// structure but each entry collectively represents an instruction's
		// payload bits distributed throughout the pipeline.
		//
		// Each instruction is allocated two consecutive entries,
		// even and odd, in case the instruction is split into two.
		//
		////////////////////////////////////////////////////////////////////////
		payload_t    buf[PAYLOAD_BUFFER_SIZE];
		unsigned int head;
		unsigned int tail;
		int          length;
 
		payload();		       // constructor
		unsigned int push();
		void pop();
		void clear();
		void split(unsigned int index);
		void map_to_actual(unsigned int index, unsigned int Tid);
		void rollback(unsigned int index);
};
#endif

