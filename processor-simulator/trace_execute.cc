#include "processor.h"

using namespace std;

void processor::trace_execute(unsigned int pe_number, unsigned int lane_number)
{
   unsigned int index;
   unsigned int inst_index;
   bool hit;		// load value available
   unsigned int pe = pe_number;
   db_t *actual_dbind;
   unsigned int lower, upper;
   
	if(TR_PAY.is_empty())
		return;

   // Check if there is an instruction in the Execute Stage of the specified Execution Lane.
if (PE_array[pe]->Execution_Lanes[lane_number].ex.valid)
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Get the instructions's index into PAY.
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  index = PE_array[pe]->Execution_Lanes[lane_number].ex.index;   	

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Execute the instruction.
  // * Load and store instructions use the AGEN and Load/Store Units.
  // * All other instructions use the ALU.
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  	if (IS_MEM_OP(PAY.buf[index].flags))
  	{
     	// Perform AGEN to generate address.
	  	if (!PAY.buf[index].split_store || PAY.buf[index].upper)
		  agen(index);

		if(PAY.buf[index].good_instruction == true)
		{
			actual_dbind = THREAD[Tid]->peek(PAY.buf[index].db_index);
			lower = actual_dbind->a_rdst[1].value;
			upper = actual_dbind->a_rdst[0].value;
		}
		else
		{
			lower = 0;
			upper = 0;
		}
	 	// Execute the load or store in the LSU.
	 	if (IS_LOAD(PAY.buf[index].flags))
	 	{
	    		hit = LSU.load_addr(cycle,
				PAY.buf[index].addr,
				PAY.buf[index].LQ_index,
				PAY.buf[index].SQ_index, PAY.buf[index].SQ_phase,
				PAY.buf[index].B_value.w[0],	// LWL/LWR
				PAY.buf[index].C_value.w[0],
				PAY.buf[index].C_value.w[1],
				lower,
				upper);

		    // Write the load's value into either the GRF or LRF (INT/FP)
		    if(hit == true)
		    {
				if (PAY.buf[index].C_valid == true)
				{
					if(PAY.buf[index].C_int == true)
					{
						// A destination operand can be both local and live-out at the same time.
						// In this case, it will write its value into both LRF and the GRF.
						if (PAY.buf[index].C_local == true)
						{
							PE_array[pe]->IQ_INT.local_wakeup(PAY.buf[index].C_local_phys_reg);
							PE_array[pe]->set_ready_int(PAY.buf[index].C_local_phys_reg);
							PE_array[pe]->write_int(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
						}

						if (PAY.buf[index].C_live_out == true)
						{
							 //Global wakeup and GRF ready bit set
							 for (int i=0; i<TOTAL_PE; i++)
							 {
								if (PE_array[i]->valid == true /*&& i!=pe*/)
								{
								   PE_array[i]->IQ_INT.global_wakeup(PAY.buf[index].C_global_phys_reg);
								}
							 }
							REN_INT->set_ready(PAY.buf[index].C_global_phys_reg);
							REN_INT->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
						}
					}

					else  // Dest register C is a floating point register.
					{
						if (PAY.buf[index].C_local == true)
						{
							PE_array[pe]->IQ_FP.local_wakeup(PAY.buf[index].C_local_phys_reg);
							PE_array[pe]->set_ready_fp(PAY.buf[index].C_local_phys_reg);
							PE_array[pe]->write_fp(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
						}

						if (PAY.buf[index].C_live_out == true)
						{
							 //Global wakeup and GRF ready bit set
							 for (int i=0; i<TOTAL_PE; i++)
							 {
								if (PE_array[i]->valid == true /*&& i!=pe*/)
								{
								   PE_array[i]->IQ_FP.global_wakeup(PAY.buf[index].C_global_phys_reg);
								}
							 }
							REN_FP->set_ready(PAY.buf[index].C_global_phys_reg);
							REN_FP->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
						}
						
					}
				}
	    	}	//Hit loop ends.

		//set c_next_pc of loads
		PAY.buf[index].c_next_pc = PAY.buf[index].pc + 8;

	 	}  //LOAD instr loop ends.

	 else   //STORE instr loop.
	 {
	    assert(IS_STORE(PAY.buf[index].flags));
	    // Set c_next_pc for store instructions.
	      PAY.buf[index].c_next_pc = PAY.buf[index].pc + 8;

	    if (PAY.buf[index].split_store)
	    {
	       assert(PAY.buf[index].split);
	       if (PAY.buf[index].upper)
	          LSU.store_addr(cycle, PAY.buf[index].addr, PAY.buf[index].SQ_index, PAY.buf[index].LQ_index, PAY.buf[index].LQ_phase); // upper op: address
	       else
	    	   LSU.store_value(PAY.buf[index].SQ_index, PAY.buf[index].A_value.w[0], PAY.buf[index].A_value.w[1]);		// lower op: float.pt. value
	    }
	    else
	    {
	       // If not a split-store, then the store has both the address and the value.
	       LSU.store_addr(cycle, PAY.buf[index].addr, PAY.buf[index].SQ_index, PAY.buf[index].LQ_index, PAY.buf[index].LQ_phase);
	       if (SS_OPCODE(PAY.buf[index].inst) == DSZ)
	          LSU.store_value(PAY.buf[index].SQ_index, (SS_WORD_TYPE)0, (SS_WORD_TYPE)0);
	       else
	          LSU.store_value(PAY.buf[index].SQ_index, PAY.buf[index].B_value.w[0], PAY.buf[index].B_value.w[1]);
	    }
	 }
   
    }	//MEM op loop ends.

   else		// ALU type instructions.
   {
	 // Execute the ALU type instruction on the ALU.
	 alu(index);

	 // If the ALU type instruction has a destination register (not a branch),
	 // then write its result value into the appropriate Physical Register File.
	 // Doing this here, instead of in WB, properly simulates the bypass network.
	 // Here we need to write the value into either the LRF or GRF (INT or FP)
	if (PAY.buf[index].C_valid == true)
	{
	 	if(PAY.buf[index].C_int == true)
	 	{
	 		// A destination operand can be both local and live-out at the same time.
	 		// In this case, it will write its value into both LRF and the GRF.
	 		if (PAY.buf[index].C_local == true)
	 		{
	 			PE_array[pe]->write_int(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
	 		}

	 		if (PAY.buf[index].C_live_out == true)
	 		{
	 			REN_INT->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
	 		}
	 	}

	 	else  // Dest register C is a floating point register.
	 	{
	 		if (PAY.buf[index].C_local == true)
	 		{
	 			PE_array[pe]->write_fp(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
	 		}

	 		if (PAY.buf[index].C_live_out == true)
	 		{
	 			REN_FP->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
	 		}
	 	}
	}
   }	//ALU instruction loop ends.

      ////////////////////////////////////////////////////////////////////////////////////////////////////
      // Setting the completed bit in the Active List and also in the particular issue queue entry
      // is deferred to the Writeback Stage.
      // Resolving branches is deferred to the Writeback Stage.
      ////////////////////////////////////////////////////////////////////////////////////////////////////

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Advance the instruction to the Writeback Stage.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      // There must be space in the Writeback Stage because Execution Lanes are free-flowing.
      assert(!PE_array[pe]->Execution_Lanes[lane_number].wb.valid);

      // Copy instruction to Writeback Stage.
      // BUT: Stalled loads should not advance to the Writeback Stage.
      if (!IS_LOAD(PAY.buf[index].flags) || hit)
      {
    	  PE_array[pe]->Execution_Lanes[lane_number].wb.valid = true;
    	  PE_array[pe]->Execution_Lanes[lane_number].wb.index = PE_array[pe]->Execution_Lanes[lane_number].ex.index;
      }

      // Remove instruction from Execute Stage.
      PE_array[pe]->Execution_Lanes[lane_number].ex.valid = false;
}	//EX.valid loop ends.

}
