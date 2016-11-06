
#include "processor.h"

void processor::trace_dispatch() 
{
	unsigned int i;
	unsigned int bundle_inst_int, bundle_inst_fp, bundle_load, bundle_store;
	unsigned int index;
	bool load_flag;
	bool store_flag;
	bool A_ready;
	bool B_ready;
	db_t *actual;
	unsigned int oracle_address;

	unsigned int next, len, iter;
	unsigned int pe_index;
	unsigned int Atag;
	unsigned int Btag;
	unsigned int iq_pos;

	// +Parth : Dispatch conditions
	// Stall the Dispatch Stage if either:
	// (1) There isn't a trace to dispatch.
	// (2) There is no free PE.
	// (3) There aren't enough LQ/SQ entries for the all memory ops in the trace.

	// First stall condition: There isn't a dispatch trace.
	// +Parth
	if (!TR_DISPATCH.valid)
		return;

	if(TR_PAY.is_empty())
		return;
	
	if(TR_AL->is_full() == true)
		return;

	// +Parth
	// Trace level active list cannot be full in a TP, since active list only holds traces.

	// Second and third stall conditions:
	// - There aren't enough LQ/SQ entries for the dispatch bundle.
	// - There is no free PE to dispatch the trace
	bundle_inst_int = 0;
	bundle_inst_fp = 0;
	bundle_load = 0;
	bundle_store = 0;

	assert(TR_DISPATCH.valid);	//Trace exists in RN/DI pipeline register.

	unsigned int start_index = TR_PAY.tr_buf[TR_DISPATCH.info_index].start_index;
	unsigned int end_index = TR_PAY.tr_buf[TR_DISPATCH.info_index].end_index;

	next = start_index;
	len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
	len = (len >> 1) + 1;
	len = len << 1;
	iter = 0;
	while (iter < len)
	{	
		unsigned int i = next;

		// No need to check for IQ availability - Each PE has both INT and FP issue queues.
		// Count number of instructions of each type.
		// Generate instruction type info to store in trace_paylaod.

		switch (PAY.buf[i].iq) 
		{
			case SEL_IQ_INT:
			// Increment number of instructions to be dispatched to integer IQ.
			bundle_inst_int++;
			break;

			case SEL_IQ_FP:
			// Increment number of instructions to be dispatched to flt. pt. IQ.
			bundle_inst_fp++;
			break;

			case SEL_IQ_NONE: case SEL_IQ_NONE_EXCEPTION:
			// Skip IQ altogether.
			break;

			default:
			assert(0);
			break;
		}

		// Check LQ/SQ requirement.
		if (IS_LOAD(PAY.buf[i].flags))
		{
			bundle_load++;
		}

		else if (IS_STORE(PAY.buf[i].flags)) 
		{
			// Special cases:
			// S_S and S_D are split-stores, i.e., they are split into an addr-op and a value-op.
			// The two ops share a SQ entry to "rejoin". Therefore, only the first op should check
			// for and allocate a SQ entry; the second op should inherit the same entry.
			if (!PAY.buf[i].split_store || PAY.buf[i].upper)
			bundle_store++;
		}

		//Check for split instructions.
		if (PAY.buf[i].split == true)
		{
			next = MOD((next+1), PAYLOAD_BUFFER_SIZE);
			iter = iter + 1;
		}
		else 
		{	
			next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
			iter = iter + 2;
		}
   	}  //for loop ends here.

	// Check available entries only in the LSQ.
	if (LSU.stall(bundle_load, bundle_store))
		return;

	// Check if there is a free PE to dispatch the trace. This condition would not arise since we are mapping it to indexes in TR_PAY.
	// Making it this far means we have all the required resources to dispatch the trace.
	// OTB entry corresponds to pe_index
	pe_index = TR_DISPATCH.info_index;
	PE_array[pe_index]->valid = true;
	PE_array[pe_index]->info_index = TR_DISPATCH.info_index;
	
	// Dispatch the instruction into the trace level active list.  
	// 1. When you dispatch the instruction into the Active List, remember to *update* the trace's
	//    payload with its active list index.
	TR_PAY.tr_buf[pe_index].AL_index = TR_AL->enqueue(pe_index);
	assert(TR_DISPATCH.info_index == TR_PAY.tr_buf[TR_DISPATCH.info_index].AL_index);
	assert(pe_index == TR_DISPATCH.info_index);
	assert(pe_index == TR_PAY.tr_buf[TR_DISPATCH.info_index].AL_index);
	
	// Need to restore the LQ/SQ when we squash traces because of branch mispredict/exception
	// We ALWAYS checkpoint.
	// Checkpointed traces must record information for restoring the LQ/SQ when a branch misprediction resolves.
	// We checkpoint at the start instruction in the trace. (start_index)
	// This is because we take a checkpoint of the RMT also before the trace is renamed.
	LSU.checkpoint(PAY.buf[start_index].LQ_index_cp, PAY.buf[start_index].LQ_phase_cp, PAY.buf[start_index].SQ_index_cp, PAY.buf[start_index].SQ_phase_cp);
	
	// Determine initial ready bits for all the instructions two source operands within the trace.
	// These will be used to initialize each instruction's ready bits in the Issue Queue.

	next = start_index;
	len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
	len = (len >> 1) + 1;
	len = len << 1;
	iter = 0;
	while(iter < len)
	{
		unsigned int i = next;
		assert(TR_DISPATCH.valid);

		//Choose execution lane for instruction
		PAY.buf[i].lane_id = steer(PAY.buf[i].fu, pe_index); 

		if(PAY.buf[i].A_valid == false)
		{
			A_ready = true;
		}
		else
		{
			if(PAY.buf[i].A_int == true)
			{
				if(PAY.buf[i].A_live_in == true)
					{
						A_ready = REN_INT->is_ready(PAY.buf[i].A_global_phys_reg);
						Atag = PAY.buf[i].A_global_phys_reg;
					}
				else if(PAY.buf[i].A_local == true)
					{
						A_ready = false;
						Atag = PAY.buf[i].A_local_phys_reg;
					}
			}
			else if(PAY.buf[i].A_int == false)
			{
				if(PAY.buf[i].A_live_in == true)
					{
						A_ready = REN_FP->is_ready(PAY.buf[i].A_global_phys_reg);
						Atag = PAY.buf[i].A_global_phys_reg;
					}
				else if(PAY.buf[i].A_local == true)
					{
						A_ready = false;
						Atag = PAY.buf[i].A_local_phys_reg;
					}
			}
		}

		if(PAY.buf[i].B_valid == false)
		{
			B_ready = true;
		}
		else
		{
			if(PAY.buf[i].B_int == true)
			{
				if(PAY.buf[i].B_live_in == true)
					{
						B_ready = REN_INT->is_ready(PAY.buf[i].B_global_phys_reg);
						Btag = PAY.buf[i].B_global_phys_reg;
					}
				else if(PAY.buf[i].B_local == true)
					{
						B_ready = false;
						Btag = PAY.buf[i].B_local_phys_reg;
					}
			}

			else if(PAY.buf[i].B_int == false)
			{
				if(PAY.buf[i].B_live_in == true)
					{
						B_ready = REN_FP->is_ready(PAY.buf[i].B_global_phys_reg);
						Btag = PAY.buf[i].B_global_phys_reg;
					}
				else if(PAY.buf[i].B_local == true)
					{
						B_ready = false;
						Btag = PAY.buf[i].B_local_phys_reg;
					}
			}
		}

		// Clear the ready bit of the instruction's destination register.
		// This is needed to synchronize future consumers.
		// Only the ready bits of global live-out regs need to be cleared.
		//
		if(PAY.buf[i].C_valid == true && PAY.buf[i].C_live_out == true)
		{
			if(PAY.buf[i].C_int == true)
				REN_INT->clear_ready(PAY.buf[i].C_global_phys_reg);
			else
				REN_FP->clear_ready(PAY.buf[i].C_global_phys_reg);
		}

		if(PAY.buf[i].C_valid == true && PAY.buf[i].C_local == true)
		{
			if(PAY.buf[i].C_int == true)
				PE_array[pe_index]->clear_ready_int(PAY.buf[i].C_local_phys_reg);
			else
				PE_array[pe_index]->clear_ready_fp(PAY.buf[i].C_local_phys_reg);
		}

		// Dispatch the instruction into Issue Queue.



		switch (PAY.buf[i].iq)
		{
          
			 case SEL_IQ_INT:
		     // Dispatch the instruction into the integer IQ.

	   			iq_pos = PE_array[pe_index]->IQ_INT.dispatch(i, PAY.buf[i].lane_id,
		        PAY.buf[i].A_valid, A_ready, Atag, PAY.buf[i].A_live_in, PAY.buf[i].A_local,
	   		       PAY.buf[i].B_valid, B_ready, Btag, PAY.buf[i].B_live_in, PAY.buf[i].B_local);

		    //Set the iq_index
		    PAY.buf[i].iq_index = iq_pos;

		      break;

		      case SEL_IQ_FP:
	      	    // FIX_ME #10b
	      	    // Dispatch the instruction into the flt. pt. IQ.

	      		 iq_pos = PE_array[pe_index]->IQ_FP.dispatch(i, PAY.buf[i].lane_id,
		          PAY.buf[i].A_valid, A_ready, Atag, PAY.buf[i].A_live_in, PAY.buf[i].A_local,
	      		      PAY.buf[i].B_valid, B_ready, Btag, PAY.buf[i].B_live_in, PAY.buf[i].B_local);

		    //Set the iq_index
		    PAY.buf[i].iq_index = iq_pos;

		      break;

		      case SEL_IQ_NONE:
	      	    // FIX_ME #10c
	      	    // Skip IQ altogether.
		       PAY.buf[i].completed = true;
		      break;

		      case SEL_IQ_NONE_EXCEPTION:

		    	PAY.buf[i].completed = true;
		    	PAY.buf[i].exception = true;
		    	TR_PAY.tr_buf[TR_DISPATCH.info_index].exception = true;

		      break;

		      default:
			   assert(0);
		    break;
		}
		
		// Dispatch loads and stores into the LQ/SQ and record their LQ/SQ indices.
		if (IS_MEM_OP(PAY.buf[i].flags))
		{
			if (!PAY.buf[i].split_store || PAY.buf[i].upper) 
			{
				LSU.dispatch(IS_LOAD(PAY.buf[i].flags),
				PAY.buf[i].size,
				PAY.buf[i].left,
				PAY.buf[i].right,
				PAY.buf[i].is_signed,
				i,
				PAY.buf[i].LQ_index, PAY.buf[i].LQ_phase,
				PAY.buf[i].SQ_index, PAY.buf[i].SQ_phase);
				// The lower part of a split-store should inherit the same LSU indices.
				if (PAY.buf[i].split_store)
				{
					   assert(PAY.buf[i+1].split && !PAY.buf[i+1].upper);
					   PAY.buf[i+1].LQ_index = PAY.buf[i].LQ_index;
					   PAY.buf[i+1].LQ_phase = PAY.buf[i].LQ_phase;
					   PAY.buf[i+1].SQ_index = PAY.buf[i].SQ_index;
					   PAY.buf[i+1].SQ_phase = PAY.buf[i].SQ_phase;
				}

				// Oracle memory disambiguation support.
				if (ORACLE_DISAMBIG && PAY.buf[i].good_instruction && IS_STORE(PAY.buf[i].flags))
				{
				   // Get pointer to the corresponding instruction in the functional simulator.
				   actual = THREAD[Tid]->peek(PAY.buf[i].db_index);

				   // Determine the oracle store address.
					if (SS_OPCODE(PAY.buf[i].inst) == DSW) 
					{
					   assert(PAY.buf[i].split);
					   oracle_address = (PAY.buf[i].upper ?  actual->a_addr : actual->a_addr + 4);
					}
					else 
					{
					  oracle_address = actual->a_addr;
					}

					   // Place oracle store address into SQ before all subsequent loads are dispatched.
					   // This policy ensures loads only stall on truly-dependent stores.
					   LSU.store_addr(cycle, oracle_address, PAY.buf[i].SQ_index, PAY.buf[i].LQ_index, PAY.buf[i].LQ_phase);
				}
			}
		}

		//Check for split instructions.
		if (PAY.buf[i].split == true)
		{
			next = MOD((next+1), PAYLOAD_BUFFER_SIZE);
			iter = iter + 1;
		}
		else 
		{	
			next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
			iter = iter + 2;
		}
   } //while loop ends here.

	// Remove the dispatch trace from the Dispatch pipeline register.
	// Move the info to the PE where this is dispatched.
	TR_DISPATCH.valid = false;
	//assert(PE_array[pe_index]->valid == false);
	PE_array[pe_index]->info_index = TR_DISPATCH.info_index;
	PE_array[pe_index]->valid = true;
}

unsigned int processor::steer(fu_type fu, unsigned int pe_index) 
{
   unsigned int fu_lane_vector;
   unsigned int lane_id;
   unsigned int i;
   bool found;

   // Choose an execution lane for the instruction based on:
   // (1) its FU type,
   // (2) the FU/lane matrix, and
   // (3) a load balancing policy.

   assert((unsigned int)fu < (unsigned int)NUMBER_FU_TYPES);

   fu_lane_vector = PE_array[pe_index]->fu_lane_matrix[(unsigned int)fu];
   lane_id = PE_array[pe_index]->fu_lane_ptr[(unsigned int)fu];

   assert(lane_id < issue_width);
   found = false;
   for (i = 0; i < issue_width; i++) 
   {
      lane_id++;
      if (lane_id == issue_width)
         lane_id = 0;

      if (fu_lane_vector & (1 << lane_id)) 
      {
         found = true;
	 break;
      }
   }

   assert(found);
   PE_array[pe_index]->fu_lane_ptr[(unsigned int)fu] = lane_id;
   return(lane_id);
}

