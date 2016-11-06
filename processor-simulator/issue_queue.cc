#include "processor.h"
#include "issue_queue.h"
#include <assert.h>

// constructor
issue_queue::issue_queue(unsigned int size)
{
   // Initialize the issue queue.
   q = new issue_queue_entry_t[size];
   this->trace_size = size;			//Size of IQ = Number of instructions in a single trace.
   length = 0;
   num_completed = 0;

   for (unsigned int i = 0; i < trace_size; i++)
   {
      q[i].valid = false;
      q[i].completed = false;
   }

   // Initialize the issue queue's free list.
   fl = new unsigned int[size];
   fl_head = 0;
   fl_tail = 0;
   fl_length = size;
   for (unsigned int i = 0; i < trace_size; i++)
      fl[i] = i;
}

bool issue_queue::stall(unsigned int bundle_inst) 
{
   assert((length + fl_length) == trace_size);
   return(fl_length < bundle_inst);
}

unsigned int issue_queue::dispatch(unsigned int index, unsigned int lane_id,
			   bool A_valid, bool A_ready, unsigned int A_tag, bool A_live_in, bool A_local,
			   bool B_valid, bool B_ready, unsigned int B_tag, bool B_live_in, bool B_local)
{
   unsigned int free;

   // Assert there is a free issue queue entry.
   assert(fl_length > 0);
   assert(length < trace_size);

   // Pop a free issue queue entry.
   free = fl[fl_head];
   fl_head = MOD_S((fl_head + 1), trace_size);
   fl_length--;

   // Dispatch the instruction into the free issue queue entry.
   length++;
   assert(!q[free].valid);

   q[free].valid = true;
   q[free].index = index;
   q[free].lane_id = lane_id;
   q[free].A_valid = A_valid;
   q[free].A_ready = A_ready;
   q[free].A_tag = A_tag;
   q[free].A_live_in = A_live_in;
   q[free].A_local = A_local;
   q[free].B_valid = B_valid;
   q[free].B_ready = B_ready;
   q[free].B_tag = B_tag;
   q[free].B_live_in = B_live_in;
   q[free].B_local = B_local;

   return free;
}

void issue_queue::global_wakeup(unsigned int tag)
{
    // +Parth
	// This function is used to wakeup the global live-in registers.
	// The incoming tag in a global PRF renamed register.
	// Broadcast the tag to every entry in the issue queue.
   // If the broadcasted tag matches a valid tag, and the register is a live-in:
   // (1) Assert that the ready bit is initially false ???  Why ??
   // (2) Set the ready bit.

   for (unsigned int i = 0; i < trace_size; i++)
   {
      if (q[i].valid)	// Only consider valid global live-in issue queue entries.
      {
         if (q[i].A_valid && (tag == q[i].A_tag) && q[i].A_live_in)
        {	// Check first source operand.
        	 assert(!q[i].A_ready);
        	 q[i].A_ready = true;
        }
         if (q[i].B_valid && (tag == q[i].B_tag) && q[i].B_live_in)
         {	// Check second source operand.
        	 assert(!q[i].B_ready);
        	 q[i].B_ready = true;
         }
      }
   }
}

void issue_queue::local_wakeup(unsigned int tag)
{
    // +Parth
	// This function is used to wakeup the local registers.
	// The incoming tag in a local LRF renamed register.
	// Broadcast the tag to every entry in the issue queue.
   // If the broadcasted tag matches a valid tag, and the register is a local:
   // (1) Assert that the ready bit is initially false ???  Why ??
   // (2) Set the ready bit.

for (unsigned int i = 0; i < trace_size; i++)
{
  if (q[i].valid)	// Only consider valid local issue queue entries.
  {
    	if (q[i].A_valid && (tag == q[i].A_tag) && q[i].A_local)
    	{	// Check first source operand.
    			 assert(!q[i].A_ready);
    			 q[i].A_ready = true;
    	 }
	if (q[i].B_valid && (tag == q[i].B_tag) && q[i].B_local)
	{	// Check second source operand.
		 assert(!q[i].B_ready);
		 q[i].B_ready = true;
	}
   }
}

}

void issue_queue::select_and_issue(unsigned int num_lanes, PE_lane *Execution_Lanes)
{
   unsigned int i;

   for (i = 0; i < trace_size; i++)
   {
      // Check if the instruction is valid and ready.
      if (q[i].valid && (!q[i].A_valid || q[i].A_ready) && (!q[i].B_valid || q[i].B_ready))
      {
    	  // Check if the instruction's desired Execution Lane is free.
    	  assert(q[i].lane_id < num_lanes);
    	  if (!Execution_Lanes[q[i].lane_id].rr.valid)
    	  {
    		  // Issue the instruction to the Register Read Stage within the Execution Lane.
    		  Execution_Lanes[q[i].lane_id].rr.valid = true;
    		  Execution_Lanes[q[i].lane_id].rr.index = q[i].index;  //PAY index
		  remove(i);
		  //std::cout << "Issued PAY.buf index " << q[i].index << " to lane num " << q[i].lane_id << std::endl; 
    	  }
      }
   }
}

// +Parth
// We will be setting the completed bit for instructions that are executed.
void issue_queue::set_complete(unsigned int i)
{
	assert(length > 0);
	assert(fl_length < trace_size);

	//Set the completed bit for the instruction.
	q[i].completed = true;
	num_completed++;
}

// DO NOT call issue_queue::remove until the trace is retired.
void issue_queue::remove(unsigned int i)
{
	 assert(length > 0);
	 assert(fl_length < trace_size);

     // Remove the instruction from the issue queue.
	 q[i].valid = false;
	 length--;

     // Push the issue queue entry back onto the free list.
	 fl[fl_tail] = i;
	 fl_tail = MOD_S((fl_tail + 1), trace_size);
	 fl_length++;
}

void issue_queue::flush()
{
   length = 0;
   for (unsigned int i = 0; i < trace_size; i++)
   {
      q[i].valid = false;
      q[i].completed = false;
   }

   num_completed = 0;
   fl_head = 0;
   fl_tail = 0;
   fl_length = trace_size;
   for (unsigned int i = 0; i < trace_size; i++)
      fl[i] = i;
}


