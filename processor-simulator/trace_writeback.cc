#include "processor.h"

using namespace std;

void processor::trace_writeback(unsigned int pe_number, unsigned int lane_number)
{
   unsigned int trace_index;
   unsigned int inst_index;
   unsigned int index;
   unsigned int pe = pe_number;

   	if(TR_PAY.is_empty())
		return;
	
   // Check if there is an instruction in the Writeback Stage of the specified Execution Lane.
   if (PE_array[pe]->Execution_Lanes[lane_number].wb.valid) 
   {

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Get the trace's index into PAY.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      index = PE_array[pe]->Execution_Lanes[lane_number].wb.index;

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Resolve branches.	
      // --- Branch resolving is now moved 
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////

	    // Execute doesnt set c_next_pc for non-control transfer instructions so we set here
	    //set c_next_pc for ALU,  SYSCALL
	    // for JUMP already set in FIll and it doesnt come here
	if ((PAY.buf[index].checkpoint == false) && (SS_OPCODE(PAY.buf[index].inst)) != JAL ) 
	{          
		PAY.buf[index].c_next_pc = PAY.buf[index].pc + 8;
	} 

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Set completed bit for the instruction in TR_PAY.buf[]
      // The active list checks all the instructions in a particular trace to set the overall trace completed bit.
      // Also set the completed bit for the instruction in the issue queue.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      PAY.buf[index].completed = true;

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Remove the instruction from the Execution Lane.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      PE_array[pe]->Execution_Lanes[lane_number].wb.valid = false;
	}
}

