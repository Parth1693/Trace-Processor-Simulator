
#include "processor.h"

void processor::trace_retire() 
{
	unsigned int i;
	bool committed1, committed2;
	bool load, load2;
	bool store, store2;
	bool branch1, branch2;	// ERIC_FIX_ME
	bool exception, exception2;
	unsigned int offending_PC, offending_PC2;
	unsigned int next, len, iter;
	unsigned int info_index ;
	unsigned int start_index, end_index;
	bool trace_completed, trace_exception;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Step 1. AL is empty
	//			a) TR_PAY is non-empty and (committed PC != TR_PAY head's start_pc)
	//			b) TR_PAY is empty. Hence check slow path's now servicing PC
	//Step 2. AL is non-empty. Check AL's head
	//			a) Check if head of AL is mispredicted
	//			b) Check branch mispredict
	//Step 3. Check if head of AL trace can be committed 
	//			If yes,
	//			a) If no-exception
	//			b) If exception
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////
	// Replay stalled loads.
	///////////////////////////////
	
	SS_WORD_TYPE value0, value1;
	unsigned int index;
	if (LSU.load_replay(cycle, index, value0, value1)) 
	{
		// Load has resolved.
		assert(IS_LOAD(PAY.buf[index].flags));
		assert(PAY.buf[index].C_valid);
		PAY.buf[index].C_value.w[0] = value0;
		PAY.buf[index].C_value.w[1] = value1;

		// +Parth
		// Tips:
		// 1. At this point of the code, 'index' is the instruction's index into PAY.buf[] (payload).
		// 2. See #13 (in execute.cc), and implement steps 3a,3b,3c.

		//Here we have to get mapping of global index to PAY.buf[] index and index inside trace_buf[].
		if (PAY.buf[index].C_valid == true )            
		{
			if(PAY.buf[index].C_int == true)
			{
				if (PAY.buf[index].C_live_out == true)
				{
					//Global wakeup and GRF ready bit set
					for (int i=0; i<TOTAL_PE; i++)
					{
						if (PE_array[i]->valid == true && i!= PAY.buf[index].trace_info_index)
						{
							PE_array[i]->IQ_INT.global_wakeup(PAY.buf[index].C_global_phys_reg);
						}
					}
					REN_INT->set_ready(PAY.buf[index].C_global_phys_reg);
					REN_INT->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
				}
				if (PAY.buf[index].C_local == true)
				{
					PE_array[PAY.buf[index].trace_info_index]->IQ_INT.local_wakeup(PAY.buf[index].C_local_phys_reg);
					PE_array[PAY.buf[index].trace_info_index]->set_ready_int(PAY.buf[index].C_local_phys_reg);
					PE_array[PAY.buf[index].trace_info_index]->write_int(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
				}
			}
			else if (PAY.buf[index].C_int == false)
			{
				if (PAY.buf[index].C_live_out == true)
				{
					for (int i=0; i<TOTAL_PE; i++)
					{
						if (PE_array[i]->valid == true && i!= PAY.buf[index].trace_info_index)
						{
							PE_array[i]->IQ_FP.global_wakeup(PAY.buf[index].C_global_phys_reg);
						}
					}		
					REN_FP->set_ready(PAY.buf[index].C_global_phys_reg);
					REN_FP->write(PAY.buf[index].C_global_phys_reg, PAY.buf[index].C_value.dw);
				}
				if (PAY.buf[index].C_local == true)
				{
					PE_array[PAY.buf[index].trace_info_index]->IQ_FP.local_wakeup(PAY.buf[index].C_local_phys_reg);
					PE_array[PAY.buf[index].trace_info_index]->set_ready_fp(PAY.buf[index].C_local_phys_reg);
					PE_array[PAY.buf[index].trace_info_index]->write_fp(PAY.buf[index].C_local_phys_reg, PAY.buf[index].C_value.dw);
				}
			}
		}

		// +Parth
		// Set completed bits in the PAY.buf[] structure.
		// Set c_next_pc for replayed loads.
		PAY.buf[index].c_next_pc =  PAY.buf[index].pc + 8;

		PAY.buf[index].completed = true;

	} //Load replay ends.


	//Step 1
	// For empty TR_AL, check for start_pc of prepared head trace in TR_PAY() OR now_servicing_pc mismatch with committed_pc.
	if (TR_AL->is_empty()) //AL is empty
	{
		if (!TR_PAY.is_empty()) //TR_PAY is not empty
		{
			//Check head trace in TR_PAY with committed_pc.
			if (TR_PAY.tr_buf[TR_PAY.head].start_pc != committed_pc)
			{
				//1. Recover fetch unit
				TR_PAY.tr_buf[TR_PAY.head].trace_mispredict = true;
				TR_PAY.tr_buf[TR_PAY.head].mode5_pc = committed_pc;

				// 2. Recover Renamer module
				//Copy AMT to RMT: Separate for integer and floating point since different number of logical registers.
			
				for(int i=0; i<REN_INT->logRegs; i++)
				{
					REN_INT->RMT[i] = REN_INT->AMT[i];
				}

				for(int i=0; i<REN_FP->logRegs; i++)
				{
					REN_FP->RMT[i] = REN_FP->AMT[i];
				}

				//Set ready bit in GRF.
				for(int i=0; i<REN_INT->phyRegs; i++)
				{
					REN_INT->GRF_ready[i] = 0;
				}

				for(int i=0; i<REN_FP->phyRegs; i++)
				{
					REN_FP->GRF_ready[i] = 0;
				}

				for(int i=0; i<REN_INT->logRegs; i++)
				{
					REN_INT->GRF_ready[REN_INT->AMT[i]] = 1;
				}

				for(int i=0; i<REN_FP->logRegs; i++)
				{
					REN_FP->GRF_ready[REN_INT->AMT[i]] = 1;
				}

				//FreeList head rollback
				REN_INT->freeList->restoreFL();
				REN_FP->freeList->restoreFL();

				//Active list rollback?? Here it is empty

				// 3. Squash LSU
				LSU.flush();

				// 4. Squash PE  --- Free all PEs (IQ_INT, IQ_FP, Execution lanes RR EX WB)
				for (int i=0; i<TOTAL_PE; i++)
				{
					PE_array[i]->valid = false;
					PE_array[i]->pe_free();
				}

				// 5. Squash pipeline registers from Fetch, Rename, Dispatch.
				TR_FETCH.valid = false;
				TR_RENAME.valid = false;
				TR_DISPATCH.valid = false;

				// 6. Rollback TR_PAY
				TR_PAY.rollback(TR_PAY.head);
				
				// 7. Clear PAY or rollback???
				PAY.clear();
			}
			return;
		}
		else // TR_PAY is empty
		{
			//Check slow path fill pc.
			if(TFill.busy == true)
			{			
				//printf("Here with committed_pc = %d\n", committed_pc);				
				if (TFill.now_servicing_pc != committed_pc)
				{
					TR_PAY.tr_buf[TR_PAY.tail].trace_mispredict = true;
					TR_PAY.tr_buf[TR_PAY.tail].mode5_pc = committed_pc;
				}
				//printf("Here with now_servicing_pc = %d\n", TFill.now_servicing_pc);			
			}
			return;
		}
	} // TR_AL empty ends.


	//Step 2
	//Reach here only if AL is non-empty


	//a) Check head trace in TR_AL with committed_pc. Head of AL same as head of TR_PAY
	if (TR_PAY.tr_buf[TR_PAY.head].start_pc != committed_pc)
	{
		//1. Recover fetch unit
		TR_PAY.tr_buf[TR_PAY.head].trace_mispredict = true;
		TR_PAY.tr_buf[TR_PAY.head].mode5_pc = committed_pc;

		// 2. Recover Renamer module
		//Copy AMT to RMT: Separate for integer and floating point since different number of logical registers.
		for(int i=0; i<REN_INT->logRegs; i++)
		{
			REN_INT->RMT[i] = REN_INT->AMT[i];
		}
		for(int i=0; i<REN_FP->logRegs; i++)
		{
			REN_FP->RMT[i] = REN_FP->AMT[i];
		}

		//Set ready bit in GRF.
		for(int i=0; i<REN_INT->phyRegs; i++)
		{
			REN_INT->GRF_ready[i] = 0;
		}

		for(int i=0; i<REN_FP->phyRegs; i++)
		{
			REN_FP->GRF_ready[i] = 0;
		}

		for(int i=0; i<REN_INT->logRegs; i++)
		{
			REN_INT->GRF_ready[REN_INT->AMT[i]] = 1;
		}

		for(int i=0; i<REN_FP->logRegs; i++)
		{
			REN_FP->GRF_ready[REN_FP->AMT[i]] = 1;
		}

		//FreeList head rollback??
		REN_INT->freeList->restoreFL();
		REN_FP->freeList->restoreFL();

		// Rollback active list
		TR_AL->al_rollback(TR_PAY.head);

		// 3. Squash LSU
		LSU.flush();

		// 4. Squash PE  --- Free all PEs (IQ_INT, IQ_FP, Execution lanes RR EX WB)
		for (int i=0; i<TOTAL_PE; i++)
		{
			PE_array[i]->valid = false;
			PE_array[i]->pe_free();
		}

		// 5. Squash pipeline registers from Fetch, Rename, Dispatch.
		TR_FETCH.valid = false;
		TR_RENAME.valid = false;
		TR_DISPATCH.valid = false;

		// 6. Rollback TR_PAY
		TR_PAY.rollback(TR_PAY.head);
		
		// 7. Clear PAY or rollback???
		PAY.clear();

		//8. TP rollback history
		if(TR_PAY.tr_buf[TR_PAY.head].checkpoint_ID == TR_PAY.head)
			{
				if (TR_PAY.tr_buf[TR_PAY.head].valid)
				{
					// Copy from checkpoint.
					for (int i=0; i<TP.hist_reg_entries; i++)
					{
						TP.hist_reg[i] = REN_INT->CP[TR_PAY.head].tp_hist[i];	
					}				
				} 
			}

		return;
	}
	
	// b)Check branch misprediction in the trace at AL head.
	info_index = TR_AL->AL[TR_AL->al_head].info_index; //index is TR_PAY index
	start_index = TR_PAY.tr_buf[info_index].start_index; //PAY's start
	end_index = TR_PAY.tr_buf[info_index].end_index; //PAY's end
	if (!TR_AL->is_empty())
	{
		next = start_index;
		len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
		len = (len >> 1) + 1;
		len = len << 1;
		iter = 0;
		while (iter < len)
		{
			i = next;
			if( i != end_index ) // We shouldn't check for ends_in_br -- ie last branch in the trace. It is trace mispredict and not branch mispredict
			{
				if (PAY.buf[i].checkpoint && (PAY.buf[i].completed == true) && (SS_OPCODE(PAY.buf[i].inst) != JALR) && (SS_OPCODE(PAY.buf[i].inst) != JR))  //JUMP is not disptched in IQ --so we check all br
			      	{
						if (PERFECT_BRANCH_PRED) 
						{
				  	    	if (!wait_for_trap)
				  	       		assert(PAY.buf[i].next_pc == PAY.buf[i].c_next_pc);

								PAY.buf[i].is_resolved = true;
								PAY.buf[i].is_mispredict = false;
						}

					 	else if (PAY.buf[i].next_pc == PAY.buf[i].c_next_pc) 
					 	{
							// Branch was predicted correctly.
							PAY.buf[i].is_resolved = true;
							PAY.buf[i].is_mispredict = false;

					 	}

			    	 	// Branch was mispredicted.
			    	 	else 
			    	 	{  	    	

							PAY.buf[i].is_resolved = true;
							PAY.buf[i].is_mispredict = true;
				
							//1. Recover fetch unit. Set info for trace reconstruction in the TR_PAY.tr_buf[]
							TR_PAY.tr_buf[info_index].br_mispredict = true;
							TR_PAY.tr_buf[info_index].index_mispredict_inst = i;
							

							REN_INT->resolve(info_index, TP);
							REN_FP->resolve(info_index, TP);
/*
							// 2. Recover Renamer module. Copy AMT to RMT.
							for(int j=0; j<REN_INT->logRegs; j++)
							{
								REN_INT->RMT[j] = REN_INT->AMT[j];
							}
					
							for(int j=0; j<REN_FP->logRegs; j++)
							{
								REN_FP->RMT[j] = REN_FP->AMT[j];
							}

							//Set ready bit in GRF.

							for(int i=0; i<REN_INT->phyRegs; i++)
							{
								REN_INT->GRF_ready[i] = 0;
							}

							for(int i=0; i<REN_FP->phyRegs; i++)
							{
								REN_FP->GRF_ready[i] = 0;
							}

							for(int i=0; i<REN_INT->logRegs; i++)
							{
								REN_INT->GRF_ready[REN_INT->AMT[i]] = 1;
							}

							for(int i=0; i<REN_FP->logRegs; i++)
							{
								REN_FP->GRF_ready[REN_FP->AMT[i]] = 1;
							}

							//FreeList head rollback??
							REN_INT->freeList->restoreFL();
							REN_FP->freeList->restoreFL();
*/
							//Active list rollback?? 	Rollback AL 
							TR_AL->al_rollback(info_index);

			  				// 3. Squash LSU  // Squash the LQ/SQ. 
							LSU.flush();

							// 4. Squash PE  --- Free all PEs (IQ_INT, IQ_FP, Execution lanes RR EX WB) // Make all PE free.	  
							for(int iter = 0; iter<TOTAL_PE; iter++)
							{
								PE_array[iter]->valid = false;
								PE_array[iter]->pe_free();
							}
							
							// 5. Squash pipeline registers from Fetch, Rename, Dispatch.// Squash pipeline registers
							TR_FETCH.valid = false;	
							TR_RENAME.valid = false;
							TR_DISPATCH.valid = false;

							// 6. Rollback TR_PAY  //Rollback the TR_PAY.buf[] and PE valid bits.
							TR_PAY.rollback(info_index);

							// 7. Clear PAY.
							PAY.clear();

							//8a. TP rollback history
							if(TR_PAY.tr_buf[TR_PAY.head].checkpoint_ID == TR_PAY.head)
								{
									if (TR_PAY.tr_buf[TR_PAY.head].valid)
									{
										// Copy from checkpoint.
										for (int i=0; i<TP.hist_reg_entries; i++)
										{
									  		TP.hist_reg[i] = REN_INT->CP[TR_PAY.head].tp_hist[i];   
										}               
									} 
								}

							return;
					} //end of Branch misprediction
				} // Check for BR instruction.	
			} //Not last instruction.

			next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
			iter = iter + 2;
		} //while ends.
	} //TR_AL is not empty.


	//Step 3
	
	//Reaching here means there is no self trace mispredict nor branch mispredict at head
	// Retire completed trace from AL head.
	// Compute the completed bit for the head trace in the AL every cycle.
	// If completed and no exception, commit the entire trace. Update mappings for all live_out registers.
	// Call LSU to commit stores and loads from the LSQ.
	// Also free the PE and OTB entry for that trace.

	
	start_index = TR_PAY.tr_buf[TR_AL->AL[TR_AL->al_head].info_index].start_index;
	end_index = TR_PAY.tr_buf[TR_AL->AL[TR_AL->al_head].info_index].end_index;

	trace_completed = false;
	trace_exception = false;

	TR_AL->commit_trace(trace_completed, trace_exception, TR_PAY, PAY);

  	if (trace_completed)
  	{
		//DEBUG
		last_info = TR_AL->AL[TR_AL->al_head].info_index;
		last_inst = end_index;
	#ifdef DEBUG
		printf("Trace retired : %d\n", last_info);
	#endif
    	if (!trace_exception)      //There is no exception in trace at AL head.
     	{

    	//Commit all instruction in the trace.
		next = start_index;
		len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
		len = (len >> 1) + 1;
		len =  len << 1;
		int iter = 0;
		while (iter < len)
        {	
			unsigned int i = next;
           	TR_AL->commit_inst(i, load, store, exception, offending_PC, PAY);
   				assert (exception == false);  //There should not be an exception in the trace.

       		if (load || store) 
	       	{
		  		LSU.commit(load);
	       	}

	       	// Update AMT for live-out regs.
	       	if (PAY.buf[i].C_valid == true)
	       	{
			  	if (PAY.buf[i].C_int == true)
			  	{
			     		if (PAY.buf[i].C_live_out == true)
			     		{
						unsigned int freeReg = REN_INT->AMT[PAY.buf[i].C_log_reg];
						REN_INT->AMT[PAY.buf[i].C_log_reg] = PAY.buf[i].C_global_phys_reg;
						//Push evicted entry onto free list.
						REN_INT->freeList->pushFL(freeReg);

						//stats
						total_live_outs++;
			     		}
			  	}
				else if (PAY.buf[i].C_int == false)
				{
					if (PAY.buf[i].C_live_out == true)
					{
						unsigned int freeReg = REN_FP->AMT[PAY.buf[i].C_log_reg];
						REN_FP->AMT[PAY.buf[i].C_log_reg] = PAY.buf[i].C_global_phys_reg;
						//Push evicted entry onto free list.
						REN_FP->freeList->pushFL(freeReg);

						//stats
						total_live_outs++;
					}
				}
		    }

			//collect stats for live ins
			if((PAY.buf[i].A_live_in == true) || (PAY.buf[i].B_live_in == true))
				total_live_ins++;

			
	       	// Check results.
	       	checker();
	       	// Keep track of the number of retired instructions.
	       	num_insn++;

			//Check for split instructions.
			if (PAY.buf[i].split == true)
			{
				PAY.pop();
				// Keep track of the number of split instructions.
	    		if (PAY.buf[PAY.head].upper)
	       			num_insn_split++;	// ERIC_FIX_ME
				next = MOD((next+1), PAYLOAD_BUFFER_SIZE);
				iter = iter + 1;
			}
			else 
			{	
				PAY.pop();
				PAY.pop();
				next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
				iter = iter + 2;
			}

			//FGCI thing
			 std::map <unsigned int, info_bit>::iterator it;
			info_bit val_got;
			it = BIT.find(PAY.buf[i].pc);
			if(it != BIT.end())
				{
					val_got = it->second;
					if(val_got.pc == PAY.buf[i].pc )
						if((iter + val_got.tot_insn) <= 16)
							fgci_br++;
				}
			switch(SS_OPCODE(PAY.buf[i].inst))
				{
						case BEQ : case BNE : case BLEZ: case BGTZ:
						case BLTZ: case BGEZ: case BC1F: case BC1T:
							total_br_code++;
							break;

						default:
							break;
				}
        }  //while loop ends.

		//Free the PE.
		PE_array[TR_AL->AL[TR_AL->al_head].info_index]->pe_free();
		PE_array[TR_AL->AL[TR_AL->al_head].info_index]->valid = false;

		assert(TR_PAY.head == TR_AL->AL[TR_AL->al_head].info_index);

		// Update trace predictor pattern table.
		//Non-speculatively update TP table
		if (committed_pc != TR_PAY.tr_buf[TR_PAY.head].pred_pc)	// Update TP
		{
			
			TP.mispredict(TR_PAY.tr_buf[TR_PAY.head].pred_index, TR_PAY.tr_buf[TR_PAY.head].start_pc, TR_PAY.tr_buf[TR_PAY.head].br_flag);
			tp_misses++;
		}
		else		//TP is correct
		{
			
			tp_hits++;
			TP.incr_cntr(TR_PAY.tr_buf[TR_PAY.head].start_pc,TR_PAY.tr_buf[TR_PAY.head].br_flag, TR_PAY.tr_buf[TR_PAY.head].pred_index);
		}

		//set committed_pc
		if ( (SS_OPCODE(PAY.buf[TR_PAY.tr_buf[TR_PAY.head].end_index].inst) == JUMP) || (SS_OPCODE(PAY.buf[TR_PAY.tr_buf[TR_PAY.head].end_index].inst) == JAL) )
		{
			committed_pc = TR_PAY.tr_buf[TR_PAY.head].fall_addr;
			pc = committed_pc;
		}
		else
		{
			committed_pc = PAY.buf[TR_PAY.tr_buf[TR_PAY.head].end_index].c_next_pc;
			pc = committed_pc;
		}

		num_traces++;

		//Remove AL head trace.
		TR_AL->dequeue();

		//Collect stats before TR_PAY pop
		total_tr_size += TR_PAY.tr_buf[TR_PAY.head].tr_size;
			
		TR_PAY.pop();

		// Check for misprediction.
		if (!TR_PAY.is_empty()) //TR_PAY is not empty
		{
			//Check head trace in TR_PAY with committed_pc.
			if (TR_PAY.tr_buf[TR_PAY.head].start_pc != committed_pc)
			{
#ifdef DEBUG
				printf("trace_retire:529 mispredict   committed_pc = %d\n",committed_pc);
#endif
				//1. Recover fetch unit
				TR_PAY.tr_buf[TR_PAY.head].trace_mispredict = true;
				TR_PAY.tr_buf[TR_PAY.head].mode5_pc = committed_pc;

				// 2. Recover Renamer module
				//Copy AMT to RMT: Separate for integer and floating point since different number of logical registers.
				for(int i=0; i<REN_INT->logRegs; i++)
				{
					REN_INT->RMT[i] = REN_INT->AMT[i];
				}
				for(int i=0; i<REN_FP->logRegs; i++)
				{
					REN_FP->RMT[i] = REN_FP->AMT[i];
				}

				//Set ready bit in GRF.
				for(int i=0; i<REN_INT->phyRegs; i++)
				{
					REN_INT->GRF_ready[i] = 0;
				}

				for(int i=0; i<REN_FP->phyRegs; i++)
				{
					REN_FP->GRF_ready[i] = 0;
				}

				for(int i=0; i<REN_INT->logRegs; i++)
				{
					REN_INT->GRF_ready[REN_INT->AMT[i]] = 1;
				}

				for(int i=0; i<REN_FP->logRegs; i++)
				{
					REN_FP->GRF_ready[REN_FP->AMT[i]] = 1;
				}

				//FreeList head rollback??
				REN_INT->freeList->restoreFL();
				REN_FP->freeList->restoreFL();

#ifdef DEBUG
				printf("trace_retire:529 mispredict   FL restore   FL head = %d   FL tail = %d\n",REN_INT->freeList->head, REN_INT->freeList->tail);
#endif

				//Active list rollback??
				TR_AL->al_rollback(TR_PAY.head);

				// 3. Squash LSU
				LSU.flush();

				// 4. Squash PE  --- Free all PEs (IQ_INT, IQ_FP, Execution lanes RR EX WB)
				for (int i=0; i<TOTAL_PE; i++)
				{
					PE_array[i]->valid = false;
					PE_array[i]->pe_free();
				}

				// 5. Squash pipeline registers from Fetch, Rename, Dispatch.
				TR_FETCH.valid = false;
				TR_RENAME.valid = false;
				TR_DISPATCH.valid = false;

				// 6. Rollback TR_PAY
				TR_PAY.rollback(TR_PAY.head);
				
				// 7. Clear PAY or rollback???
				PAY.clear();

				//8. TP rollback history
				if(TR_PAY.tr_buf[TR_PAY.head].checkpoint_ID == TR_PAY.head)
					{
						if (TR_PAY.tr_buf[TR_PAY.head].valid)
						{
							// Copy from checkpoint.
							for (int i=0; i<TP.hist_reg_entries; i++)
							{
						  		TP.hist_reg[i] = REN_INT->CP[TR_PAY.head].tp_hist[i];   
							}               
						} 
					}

			}
			return;
		}
		else // TR_PAY is empty
		{
			//Check slow path fill pc.
			if(TFill.busy == true)
			{			
				//printf("Here with committed_pc = %d\n", committed_pc);				
				if (TFill.now_servicing_pc != committed_pc)
				{
					TR_PAY.tr_buf[TR_PAY.tail].trace_mispredict = true;
					TR_PAY.tr_buf[TR_PAY.tail].mode5_pc = committed_pc;
				}
				//printf("Here with now_servicing_pc = %d\n", TFill.now_servicing_pc);			
			}
		return;
		}
		

     	} //No exception case ends.
         	
    	else if (trace_exception)
     	{
			next = start_index;
			len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
			len = (len >> 1) + 1;
			len = len << 1;
			iter = 0;
			while(iter < len)
	        {
				i = next;
				TR_AL->commit_inst(i, load, store, exception, offending_PC, PAY);
	           	
			    // Update AMT for live-out regs.
			    // DONT do this for last instr which is exception.
				if (i != end_index)
				{
					if (load || store)
					{
						LSU.commit(load);
					}
					if (PAY.buf[i].C_valid == true)
					{
						if (PAY.buf[i].C_int == true)
						{
							if (PAY.buf[i].C_live_out == true)
							{
								unsigned int freeReg = REN_INT->AMT[PAY.buf[i].C_log_reg];
								REN_INT->AMT[PAY.buf[i].C_log_reg] = PAY.buf[i].C_global_phys_reg;
								//Push evicted entry onto free list.
								REN_INT->freeList->pushFL(freeReg);
							}
						}
						else if (PAY.buf[i].C_int == false)
						{
							if (PAY.buf[i].C_live_out == true)
							{
								unsigned int freeReg = REN_FP->AMT[PAY.buf[i].C_log_reg];
								REN_FP->AMT[PAY.buf[i].C_log_reg] = PAY.buf[i].C_global_phys_reg;
								//Push evicted entry onto free list.
								REN_FP->freeList->pushFL(freeReg);
							}
						}
					}
				}

		       // Check results.
		       checker();
		       // Keep track of the number of retired instructions.
		       num_insn++;

				//Check for split instructions.
				if (PAY.buf[i].split == true)
				{
					next = MOD((next+1), PAYLOAD_BUFFER_SIZE);
					PAY.pop();
					iter = iter + 1;
				}
				else 
				{	
					next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
					iter = iter + 2;
					PAY.pop();
					PAY.pop();
				}

				//FGCI thing
				std::map <unsigned int, info_bit>::iterator it;
				info_bit val_got;
				it = BIT.find(PAY.buf[i].pc);
				if(it != BIT.end())
					{
						val_got = it->second;
						if(val_got.pc == PAY.buf[i].pc )
							if((iter + val_got.tot_insn) <= 16)
								fgci_br++;
					}
				switch(SS_OPCODE(PAY.buf[i].inst))
					{
							case BEQ : case BNE : case BLEZ: case BGTZ:
							case BLTZ: case BGEZ: case BC1F: case BC1T:
								total_br_code++;
								break;

							default:
								break;
					}
	        }  //while loop ends.



			assert((SS_OPCODE(PAY.buf[end_index].inst) == SYSCALL) || (SS_OPCODE(PAY.buf[end_index].inst) == BREAK));
			
			//Handle the exception. // Squash the pipeline and set the committed PC.
			//Copy AMT to RMT: Separate for integer and floating point since different number of logical registers.
			for(int i=0; i<REN_INT->logRegs; i++)
			{
				REN_INT->RMT[i] = REN_INT->AMT[i];
			}

			for(int i=0; i<REN_FP->logRegs; i++)
			{
				REN_FP->RMT[i] = REN_FP->AMT[i];
			}

			for(int i=0; i<REN_INT->phyRegs; i++)
			{
				REN_INT->GRF_ready[i] = 0;
			}

			for(int i=0; i<REN_FP->phyRegs; i++)
			{
				REN_FP->GRF_ready[i] = 0;
			}

			for(int i=0; i<REN_INT->logRegs; i++)
			{
				REN_INT->GRF_ready[REN_INT->AMT[i]] = 1;
			}

			for(int i=0; i<REN_FP->logRegs; i++)
			{
				REN_FP->GRF_ready[REN_FP->AMT[i]] = 1;
			}

			// assertions.
			assert (PAY.buf[start_index].pc == PAY.buf[TR_PAY.tr_buf[TR_PAY.head].start_index].pc);
			assert (committed_pc == PAY.buf[start_index].pc);
			assert (committed_pc == TR_PAY.tr_buf[TR_PAY.head].start_pc);

			if (committed_pc != TR_PAY.tr_buf[TR_PAY.head].pred_pc)
			{
				// Update TP
				TP.mispredict(TR_PAY.tr_buf[TR_PAY.head].pred_index, TR_PAY.tr_buf[TR_PAY.head].start_pc, TR_PAY.tr_buf[TR_PAY.head].br_flag);
				tp_misses++;
			}
			else
			{
				//TP is correct
				tp_hits++;
				TP.incr_cntr(TR_PAY.tr_buf[TR_PAY.head].start_pc,TR_PAY.tr_buf[TR_PAY.head].br_flag, TR_PAY.tr_buf[TR_PAY.head].pred_index);
			}

			//Set committed PC, pc, next_fetch_cycle and wait_for_trap
			committed_pc = PAY.buf[end_index].pc + SS_INST_SIZE;
			pc = PAY.buf[end_index].pc + SS_INST_SIZE;
			next_fetch_cycle = (SS_TIME_TYPE)0;
			wait_for_trap = false;
			global_exception = true;

			// Squash pipeline registers
			TR_FETCH.valid = false;
			TR_RENAME.valid = false;
			TR_DISPATCH.valid = false;            

			//Free all PEs
			for (int i=0; i<TOTAL_PE; i++)
			{
				PE_array[i]->valid = false;
				PE_array[i]->pe_free();
			}

			// Copy from checkpoint of the following trace. Rollback of Trace Predictor history
			if (TR_PAY.tr_buf[MOD((TR_PAY.head + 1), TOTAL_PE)].valid)
			{
				// Copy from checkpoint.
				for (int i=0; i<TP.hist_reg_entries; i++)
				{
			  		TP.hist_reg[i] = REN_INT->CP[MOD((TR_PAY.head + 1), TOTAL_PE)].tp_hist[i];   
				}               
			}           

			//Restore the free lists for both INT and FP renamers.
			REN_INT->freeList->restoreFL();
			REN_FP->freeList->restoreFL();

			//Flush the LSU
			LSU.flush();

			// Step 2:
			// Func. sim. is stalled waiting to execute the trap.
			// Signal it to proceed with the trap.
			THREAD[Tid]->trap_now(PAY.buf[TR_PAY.tr_buf[TR_AL->AL[TR_AL->al_head].info_index].end_index].inst);

			// Step 3:
			// Registers of the timing simulator are now stale.
			// Copy values from the functional simulator.
			copy_reg();

			// Step 4:
			// The memory state of the timing simulator is now stale.
			// Copy values from the functional simulator.
			LSU.copy_mem(THREAD[Tid]->get_mem_table());

			// Step 5:
			// Func. sim. is waiting to resume after the trap.
			// Signal it to resume.
			THREAD[Tid]->trap_resume();
			
			// Pop TR_PAY
			TR_PAY.pop();
			TR_AL->dequeue();

			//Squash the AL
			TR_AL->al_rollback(TR_PAY.head);
			num_traces++;

			//Squash PAY
			PAY.clear();
			//Squash OTB
			TR_PAY.rollback(TR_PAY.head);

			return; //no replay --- stall
		} // exception ends.
         
    }  //Trace completed ends.

}


