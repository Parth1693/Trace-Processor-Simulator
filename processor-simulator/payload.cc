//#include <assert.h>

//#include "ss.h"
//#include "common.h"

//#include "payload.h"

#include "processor.h"

payload::payload() {
   clear();
}

unsigned int payload::push() {
   unsigned int index;
		   
   index = tail;

   // Increment tail by two, since each instruction is pre-allocated
   // two entries, even and odd, to accommodate instruction splitting.
   tail = MOD((tail + 2), PAYLOAD_BUFFER_SIZE);
   length += 2;

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
   for(int i=0; i<PAYLOAD_BUFFER_SIZE; i++ )
   {
	   buf[i].completed = false;
	   buf[i].exception  = false;
   }
}

void payload::split(unsigned int index) {
   assert((index+1) < PAYLOAD_BUFFER_SIZE);

   buf[index+1].inst.a           = buf[index].inst.a;
   buf[index+1].inst.b           = buf[index].inst.b;
   buf[index+1].pc               = buf[index].pc;
   buf[index+1].next_pc          = buf[index].next_pc;
   buf[index+1].pred_tag         = buf[index].pred_tag;
   buf[index+1].good_instruction = buf[index].good_instruction;
   buf[index+1].db_index	 = buf[index].db_index;

   buf[index+1].flags            = buf[index].flags;
   buf[index+1].fu               = buf[index].fu;
   buf[index+1].latency          = buf[index].latency;
   buf[index+1].checkpoint       = buf[index].checkpoint;
   buf[index+1].split_store      = buf[index].split_store;

   buf[index+1].A_valid          = false;
   buf[index+1].B_valid          = false;
   buf[index+1].C_valid          = false;

   ////////////////////////

   buf[index].split = true;
   buf[index].upper = true;

   buf[index+1].split = true;
   buf[index+1].upper = false;
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
   tail = MOD((index + 2), PAYLOAD_BUFFER_SIZE);

   // Recompute the length.
   length = MOD((PAYLOAD_BUFFER_SIZE + tail - head), PAYLOAD_BUFFER_SIZE);

	unsigned int next = tail;
   for(int i=0; i<PAYLOAD_BUFFER_SIZE-length; i++ )
   {
	   buf[next].completed = false;
	   buf[next].exception  = false;
	   next = MOD((next+1), PAYLOAD_BUFFER_SIZE);
   }
}



