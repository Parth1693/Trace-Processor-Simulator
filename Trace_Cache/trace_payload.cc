
class trace_payload {
	public:
		////////////////////////////////////////////////////////////////////////
	  // This class holds the instructions in each trace cache line. For example: 16 instructions.
    //  payload_t holds info for one instruction.
    //  16 (trace_size) instructions are held in *trace_buf.
		////////////////////////////////////////////////////////////////////////
		
    payload_t 	*trace_buf;
		unsigned int trace_size;
		unsigned int head;    
		unsigned int tail;
		int          length;
 
		trace_payload();		// constructor
		unsigned int push();
		void pop();
		void clear();
		void split(unsigned int index);
		void map_to_actual(unsigned int index, unsigned int Tid);
		void rollback(unsigned int index);
};

trace_payload::trace_payload( unsigned int size) {
   clear();
   trace_size = size;
   trace_buf = new payload_t[size];
}

trace_payload::~trace_payload()
{
	delete [] trace_buf;
}

unsigned int trace_payload::push() {
   unsigned int index;
		   
   index = tail;

   // Increment tail by two, since each instruction is pre-allocated
   // two entries, even and odd, to accommodate instruction splitting.
   tail = MOD((tail + 2), trace_size);
   length += 2;

   // Check for overflowing buf.
   assert(length <= trace_size);

   return(index);
}

void trace_payload::pop() {
   // Increment head by one.
   head = MOD((head + 1), trace_size);
   length -= 1;

   // Check for underflowing instr_buf.
   assert(length >= 0);
}

void trace_payload::clear() {
   head = 0;
   tail = 0;
   length = 0;
}

void trace_payload::split(unsigned int index) {
   assert((index+1) < trace_size);

   trace_buf[index+1].inst.a           = trace_buf[index].inst.a;
   trace_buf[index+1].inst.b           = trace_buf[index].inst.b;
   trace_buf[index+1].pc               = trace_buf[index].pc;
   trace_buf[index+1].next_pc          = trace_buf[index].next_pc;
   trace_buf[index+1].pred_tag         = trace_buf[index].pred_tag;
   trace_buf[index+1].good_instruction = trace_buf[index].good_instruction;
   trace_buf[index+1].db_index	 = trace_buf[index].db_index;

   trace_buf[index+1].flags            = trace_buf[index].flags;
   trace_buf[index+1].fu               = trace_buf[index].fu;
   trace_buf[index+1].latency          = trace_buf[index].latency;
   trace_buf[index+1].checkpoint       = trace_buf[index].checkpoint;
   trace_buf[index+1].split_store      = trace_buf[index].split_store;

   trace_buf[index+1].A_valid          = false;
   trace_buf[index+1].B_valid          = false;
   trace_buf[index+1].C_valid          = false;

   ////////////////////////

   trace_buf[index].split = true;
   trace_buf[index].upper = true;

   trace_buf[index+1].split = true;
   trace_buf[index+1].upper = false;
}

// Mapping of instructions to actual (functional simulation) instructions.
void trace_payload::map_to_actual(unsigned int index, unsigned int Tid) {
           unsigned int prev_index;
           bool         first;
           debug_index  db_index;
 
           //////////////////////////////
           // Find previous instruction.
           //////////////////////////////
           prev_index = MOD((index + trace_size - 2), trace_size);
	   first = (index == head);

           ////////////////////////////
           // Calculate and set state.
           ////////////////////////////
	   if (first) {                           // FIRST INSTRUCTION
	      trace_buf[index].good_instruction = true;
	      trace_buf[index].db_index = THREAD[Tid]->first(trace_buf[index].pc);
           }
	   else if (trace_buf[prev_index].good_instruction) {         // GOOD MODE
	      db_index = THREAD[Tid]->check_next(trace_buf[prev_index].db_index, trace_buf[index].pc);

	      if (db_index == DEBUG_INDEX_INVALID) {
		 // Transition to bad mode.
		 trace_buf[index].good_instruction = false;
		 trace_buf[index].db_index = DEBUG_INDEX_INVALID;
	      }
	      else {
		 // Stay in good mode.
		 trace_buf[index].good_instruction = true;
		 trace_buf[index].db_index = db_index;
	      }
           }
           else {                                               // BAD MODE
	      // Stay in bad mode.
	      trace_buf[index].good_instruction = false;
              trace_buf[index].db_index = DEBUG_INDEX_INVALID;
           }
}

void trace_payload::rollback(unsigned int index) {
   // Rollback the tail to the instruction after the instruction at 'index'.
   tail = MOD((index + 2), trace_size);

   // Recompute the length.
   length = MOD((trace_size + tail - head), trace_size);
}




//This class holds traces which are active in the pipeline.

class payload {
	public:
		//Define PAYLOAD_BUFFER_SIZE = 256 in payload.h
		trace_payload buf[PAYLOAD_BUFFER_SIZE];
		unsigned int head;
		unsigned int tail;
		int          length;
 
		payload();		// constructor
		unsigned int push();
		void pop();
		void clear();
		void map_to_actual(unsigned int index, unsigned int Tid);		//trace level mapping??

		void rollback(unsigned int index);
};

payload::payload() {
   clear();
}

unsigned int payload::push() {
   unsigned int index;		   
   index = tail;

   // Increment tail by two, since each instruction is pre-allocated
   // two entries, even and odd, to accommodate instruction splitting.
   tail = MOD((tail + 1), PAYLOAD_BUFFER_SIZE);
   length += 1;

   // Check for overflowing buf.
   assert(length <= PAYLOAD_BUFFER_SIZE);

   return(index);
}

void payload::pop() {
   // Increment head by one.
   head = MOD((head + 1), PAYLOAD_BUFFER_SIZE);
   length -= 1;

   // Check for underflowing instr_buf.
   assert(length >= 0);
}

void payload::clear() {
   head = 0;
   tail = 0;
   length = 0;
}

// Mapping of instructions to actual (functional simulation) instructions.
void payload::map_to_actual(unsigned int index, unsigned int Tid) {
           unsigned int prev_index;
           bool         first;
           debug_index  db_index;
 
           //////////////////////////////
           // Find previous instruction.
           //////////////////////////////
           prev_index = MOD((index + PAYLOAD_BUFFER_SIZE - 2), PAYLOAD_BUFFER_SIZE);
	   first = (index == head);

           ////////////////////////////
           // Calculate and set state.
           ////////////////////////////
	   if (first) {                           // FIRST INSTRUCTION
	      buf[index].good_instruction = true;
	      buf[index].db_index = THREAD[Tid]->first(buf[index].pc);
           }
	   else if (buf[prev_index].good_instruction) {         // GOOD MODE
	      db_index = THREAD[Tid]->check_next(buf[prev_index].db_index, buf[index].pc);

	      if (db_index == DEBUG_INDEX_INVALID) {
		 // Transition to bad mode.
		 buf[index].good_instruction = false;
		 buf[index].db_index = DEBUG_INDEX_INVALID;
	      }
	      else {
		 // Stay in good mode.
		 buf[index].good_instruction = true;
		 buf[index].db_index = db_index;
	      }
           }
           else {                                               // BAD MODE
	      // Stay in bad mode.
	      buf[index].good_instruction = false;
              buf[index].db_index = DEBUG_INDEX_INVALID;
           }
}

void payload::rollback(unsigned int index) {
   // Rollback the tail to the instruction after the instruction at 'index'.
   tail = MOD((index + 1), PAYLOAD_BUFFER_SIZE);

   // Recompute the length.
   length = MOD((PAYLOAD_BUFFER_SIZE + tail - head), PAYLOAD_BUFFER_SIZE);
}

