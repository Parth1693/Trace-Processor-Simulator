#include "processor.h"

trace_payload::trace_payload() {
   head = 0;
   tail = 0;
   length = 0;
   for(int i=0;i<TOTAL_PE;i++)
   {
	   tr_buf[i].valid = false;
	   tr_buf[i].ends_in_br = false;
	   tr_buf[i].trace_mispredict = false;
	   tr_buf[i].completed = false;
	   tr_buf[i].exception = false;
	   tr_buf[i].br_mispredict = false;
   }
}

unsigned int trace_payload::push() {
   unsigned int index;
		   
   index = tail;
   tr_buf[index].valid = true; 	
   // Increment tail by two, since each instruction is pre-allocated
   // two entries, even and odd, to accommodate instruction splitting.
   tail = MOD((tail + 1), TOTAL_PE);
   length += 1;

   // Check for overflowing buf.
   assert(length <= TOTAL_PE);

   return(index);
}

void trace_payload::pop() {
   // Increment head by one.
   tr_buf[head].valid = false;
   head = MOD((head + 1), TOTAL_PE);
   length -= 1;

   // Check for underflowing instr_buf.
   assert(length >= 0);
}

//Called NEVER
void trace_payload::clear() {
   head = 0;
   tail = 0;
   length = 0;
   for(int i=0;i<TOTAL_PE;i++)
   {
	   tr_buf[i].valid = false;
	   tr_buf[i].ends_in_br = false;
	   tr_buf[i].trace_mispredict = false;
	   tr_buf[i].completed = false;
	   //tr_buf[i].exception = false;
	   tr_buf[i].br_mispredict = false;
   }
}

//Called for all cases -- trace mispredict, br mispredict and exception!!!
void trace_payload::rollback(unsigned int index) {
   // Rollback the tail to the instruction after the instruction at 'index'.
   tail = index;

   // Recompute the length.
   length = MOD((TOTAL_PE + tail - head), TOTAL_PE);

   // Invalidate tr_buf from tail to head
   unsigned int next = tail;
   for (int i=0; i<TOTAL_PE-length; i++)
   {
	tr_buf[next].valid = false;
	 tr_buf[i].completed = false;
	next = MOD((next + 1), TOTAL_PE);
   }

}

bool trace_payload::is_empty()
{
	if (length == 0)
	{
		assert (head == tail);
		return true;
	}
	else
		return false;
}

bool trace_payload::is_full()
{
	if (length == TOTAL_PE)
	{
		return true;
	}
	else
		return false;
}
//Not needed now below functions
// Mapping of traces to actual (functional simulation) traces.
//Not implemented now

