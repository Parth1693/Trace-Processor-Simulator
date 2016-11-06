
#include "processor.h"

// Access trace predictor to get predicted TID
// Output is predicted traceID -> branch_flag + complete PC (say 32 bits)

//Now time for trace fetch

void processor::trace_fetch() 
{
	unsigned int i;
	bool stop;
	SS_INST_TYPE inst;
	unsigned int pred_tag;
	unsigned int next_pc;
	unsigned int direct_target;
	bool conf;				// dummy
	bool fm;				// dummy
	unsigned int history_reg;

	unsigned int line1;				// I$
	unsigned int line2;				// I$
	bool hit1;					// I$
	bool hit2;					// I$
	SS_TIME_TYPE resolve_cycle1;			// I$
	SS_TIME_TYPE resolve_cycle2;			// I$
	
	unsigned int slow_path_fetch_pc;
	bool slow_path_fetch = false;
	bool fast_path_fetch = true;
	bool recovery = false;
	unsigned int mode4_n, mode4_br_flag, mode4_m;
	bool exptn = false;

	// These are fetch modes
	// 0. Fast path mode
	// 1. Exception at head of Active List: use committed PC and branch predictor outcomes in slow path
	//		Need to set committed PC!!!!
	// 2. TID mispredict (immediate): use previous trace's FT ot T addr from OTB and use branch predictor in slow path 
	//     needs slow path because we dont have branch flag info
	// 3. T$ miss: use slow path with br_flags from TracePredictor
	// 4. Branch mispredict by PE in exec stage: squash OTB etc. Use prefix from OTB and then fill suffix using BP in slow path 
	//	 check number of branches in prefix of branch in the trace.
	// 5. Trace mispredict mode : Previous PE (which ends in branch) has sent the taken / fall through address 
	int fetch_mode = 0;
	

	//Steps:-
	//1.  If trace at head of active list signals exception, we need to squash OTB, free all PEs and make AL empty.
	//2.  Check for misprediction recovery from PE - check every cycle
	//	- Branch misprediction -> squash next entries in OTB, free next PEs, rollback AL; Copy instructions till the branch (i.e.suffix) to the line-fill buffer.
	//	- Ends in branch of previous trace mismatch - Compare in OTB with address from previous trace execution
	//	- This recovery overrides any outstanding fill operations 
	//3.  Check for immmediate misprediction recovery - 
	//	- It all OTB's are empty, check the commited PC.
	//	- Check the fall through from previous trace if previous trace does not end in branch
	// In immed. recovery, start fetching from the correct addr. (check T$ hit first and reconstruct if needed).

	if( global_exception == true )
	{
		//Squash all TR_PAY and AL. Squashing and all done in retire stage.
		
		recovery = true;
		fetch_mode = 1;
		slow_path_fetch_pc = committed_pc;  //can use committed PC directly//offending pc //Next fetch pc is offending_pc + 8.
		slow_path_fetch = true;
		fast_path_fetch = false;
		global_exception = false;
		TR_PAY.tr_buf[TR_PAY.head].trace_mispredict = false;
		TR_PAY.tr_buf[TR_PAY.head].br_mispredict = false;
		exptn = true;
		
	}
	else //no exception
	{
		//Check for branch mispredict/prev trace ends_in_branch mispredict in active OTB entries.
		//fetch mode 4 and 5
		//The br_mispredict are set by the PE execute stage.
		//trace_mispredict -> set by PE when ends in branch instruction is mispredicted. So this trace isnt affected but next traces are
		//Hence, we just squash from next traces without re-executing this trace

		unsigned int next = TR_PAY.head;
		unsigned int len = TR_PAY.length;
		//if(TR_PAY.is_empty())
		//	len = 1;
		if(!TR_PAY.is_full())
			len = len+1;
		for(int iter=0; iter<len; iter++)
		{
			i=next;
			if(TR_PAY.tr_buf[i].br_mispredict == true || TR_PAY.tr_buf[i].trace_mispredict == true)
			{
				//Squash OTBs and PEs which are after this trace.
				//Rollback AL.
				if (TR_PAY.tr_buf[i].trace_mispredict)
				{
					//Rollback the AL and TR_PAY in writeback.
				
					//Construct trace from the addr got from previous trace PE.
					//Addr is stored in previous PE object.
					slow_path_fetch = true;
					slow_path_fetch_pc = TR_PAY.tr_buf[i].mode5_pc;
					fast_path_fetch = false;
					fetch_mode = 5;
					recovery = true;
					TR_PAY.tr_buf[i].trace_mispredict = false;
					TR_PAY.tr_buf[i].br_mispredict = false;
				}
				
				//Branch mispredicted trace is good till the branch. Construct only the suffix portion.
				else if (TR_PAY.tr_buf[i].br_mispredict)
					{
					//Rollback the AL and TR_PAY in writeback
					
					//Reconstruct the trace only after the mispredicted branch.
					//Copy trace including branch directly to the line-fill buffer.
					unsigned int start_index, end_index;
					start_index = TR_PAY.tr_buf[i].start_index;
					end_index = TR_PAY.tr_buf[i].index_mispredict_inst;
					unsigned int m=0, branch_flag=0;

					unsigned int next1 = start_index;
					unsigned int len1 = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
					len1 = (len1) + 2;
					for(int iter1 = 0; iter1 < len1; iter1++)
					{	
						unsigned int j = next1;
						unsigned int index = iter1;
						TFill.buf[index] = PAY.buf[j];
						if(PAY.buf[j].checkpoint == true && PAY.buf[j].A_valid == true)
						{
							m++;
						}	
						TFill.buf[index].completed = false;
						TFill.buf[index].exception = false;
						if(j == end_index)
						{
							TFill.buf[index].next_pc = TFill.buf[index].c_next_pc;
						}

						TFill.buf[index].LQ_index = 0;	
						TFill.buf[index].LQ_phase = 0;
						TFill.buf[index].SQ_index = 0;
						TFill.buf[index].SQ_phase = 0;

						//Checkpoints for LSU
						TFill.buf[index].LQ_index_cp = 0;	
						TFill.buf[index].LQ_phase_cp = 0;
						TFill.buf[index].SQ_index_cp = 0;
						TFill.buf[index].SQ_phase_cp = 0;
						
						next1 = MOD((next1+1),PAYLOAD_BUFFER_SIZE);
					} //for loop ends here
					
					branch_flag = ((1<<(m-1)) - 1) & TR_PAY.tr_buf[i].br_flag;
					if(1<<(m-1) & TR_PAY.tr_buf[i].br_flag)  //branch was taken and now it is not taken
					{
						branch_flag = branch_flag;
					}
					else
					{
						branch_flag = branch_flag | (1<<(m-1));
					}
					slow_path_fetch = true;		//Partial slow path fetching to fill the branch suffix portion in trace.
					slow_path_fetch_pc = PAY.buf[end_index].c_next_pc;		//PC of the mispredicted branch;
					fast_path_fetch = false;
					fetch_mode = 4;
					recovery = true;
					mode4_n = len1 >> 1;
					mode4_br_flag = branch_flag;
					mode4_m = m;
					TR_PAY.tr_buf[i].br_mispredict = false;
				}
				break;
			}	
			next = MOD((next+1),TOTAL_PE);
		}
	}
	//}  //End of TR_PAY empty check.

	if (!recovery)
	{
	//Reaching here means we do not have to recover from exception or branch mispredict from PE execute stage.
	//We will use the predicted trace ID from the trace predictor, since we do not have to handle 
	//exception or misprediction from AL or PE.
		if((TR_RENAME.valid)  ||  //Rename stage is stalled
			(cycle < next_fetch_cycle))    // Slow path is busy fetching so might be I$ miss or multiple block fetch
			return;
		
		//If there is no space in TR_PAY then all PEs are busy and hence return
		if(TR_PAY.is_full())
			return;

		//Check for immediate misprediction: Either from committed PC or from fall through/taken addr of previous trace if it ends in a branch.
		//Check if OTBs are empty
		//This code loop will be used only when we start the TP for the first time.
		if (TFill.busy == false)		
		{
			if (TR_PAY.is_empty())
			{
				//start fetch from committed pc
				slow_path_fetch_pc = committed_pc;  //from where do we get this?????
				fetch_mode = 1;
				slow_path_fetch = true;
				fast_path_fetch = false;
				
			}

			//Consistency check failure fetch mode 2
			//Immed misprediction from fall through/taken addr of previous trace.
			else if (!TR_PAY.is_empty() && !TR_PAY.is_full())
			{
				//Get the TR_PAY index of most recently dispatched instruction.
				unsigned int ind;
				if (TR_PAY.tail != 0 )
					ind = TR_PAY.tail - 1;
				else
					ind = TR_PAY.length - 1;
				
				// If previous trace ends in a branch, check against both the fall through and the taken addr.
				// The start_pc should match one of those.
				// ends_in_br is true for branches and direct jumps.
				if(TR_PAY.tr_buf[ind].ends_in_br == true)
				{
					if((TR_PAY.tr_buf[ind].fall_addr != TR_FETCH.pc) && (TR_PAY.tr_buf[ind].taken_addr != TR_FETCH.pc))
					{
						//Use the predicted branch outcome for the last branch from the predicted TID for this trace.
						//int pay_index = TOTB.OTB_array[ind].index;
						unsigned int mask = 1;
						mask = mask << (TR_PAY.tr_buf[ind].br_mask);
					
						if ((mask & TR_PAY.tr_buf[ind].br_flag) == 0) //Not taken
						{
							slow_path_fetch_pc = TR_PAY.tr_buf[ind].fall_addr;
						}

						else if ((mask & TR_PAY.tr_buf[ind].br_flag) != 0)	//Taken
						{
							slow_path_fetch_pc = TR_PAY.tr_buf[ind].taken_addr;	
						}

						fetch_mode = 2;
						slow_path_fetch = true;
						fast_path_fetch = false;								
					}
					else if (TR_PAY.tr_buf[ind].fall_addr == TR_FETCH.pc)
					{
						//int pay_index = TOTB.OTB_array[ind].index;
						unsigned int mask = 1;
						mask = mask << (TR_PAY.tr_buf[ind].br_mask);

						if (mask & TR_PAY.tr_buf[ind].br_flag != 0) 	//Taken
						{
							slow_path_fetch_pc = TR_PAY.tr_buf[ind].taken_addr;
							fetch_mode = 2;
							slow_path_fetch = true;
							fast_path_fetch = false;
						}	

					}
					else if (TR_PAY.tr_buf[ind].taken_addr == TR_FETCH.pc)
					{
						unsigned int mask = 1;
						mask = mask << (TR_PAY.tr_buf[ind].br_mask);

						if (mask & TR_PAY.tr_buf[ind].br_flag == 0) 	//Not taken
						{
							slow_path_fetch_pc = TR_PAY.tr_buf[ind].fall_addr;
							fetch_mode = 2;
							slow_path_fetch = true;
							fast_path_fetch = false;
						}
					}
				}

				else  if(TR_PAY.tr_buf[ind].ends_in_br == false)
				{
					// If last trace ends in JR or JALR, then dont do consistency check.
					if( (SS_OPCODE(PAY.buf[TR_PAY.tr_buf[ind].end_index].inst) == JR) || (SS_OPCODE(PAY.buf[TR_PAY.tr_buf[ind].end_index].inst) == JALR) )
					{
						//Dont do consistency check.
						slow_path_fetch = false;
						fast_path_fetch = true;					
					}
				
					else if(TR_PAY.tr_buf[ind].fall_addr != TR_FETCH.pc)
					{
						//Start fetching from fall_addr
						slow_path_fetch_pc = TR_PAY.tr_buf[ind].fall_addr;
						slow_path_fetch = true;
						fast_path_fetch = false;
						fetch_mode = 2;				
					}
				}
			}
	#ifdef DEBUG
			//printf("Fetch mode and no-recovery: %d in cycle %d\n",fetch_mode, cycle);
	#endif
		}  // TFill busy ends
	} //end of !recovery


	if(recovery)
	{
		//Unset Fill Unit's busy flag
		TFill.busy = false;
	#ifdef DEBUG
		//printf("Fetch mode and in recovery: %d in cycle %d\n",fetch_mode, cycle);
	#endif
	}

	//When Fill Unit was busy and now cycle has reached next fetch cycle unstall the Fill Unit's busy flag
	if((TFill.is_busy() == true) && (cycle == next_fetch_cycle))
	{
		TFill.busy = false;
		
		//Put trace from fill unit into T$
		trace_block new_block;
		new_block.tr_info = TFill.trace_fill_buffer;
		for(int i=0; i<TRACE_SIZE*2; i++)
		{
			new_block.buf[i] = TFill.buf[i];
		}
		//Generate new_block tag.
		unsigned int mask, pc, tag, br;
		mask = ~((1 << TC.num_embedded_branches) - 1);
		pc = TFill.trace_fill_buffer.start_pc & mask;
		mask = (1 << TC.num_embedded_branches) - 1;
		br = TFill.trace_fill_buffer.br_flag & mask;		//Take lower 3 bits.
		tag = br | pc;
		
		new_block.tag = tag;
		
		TC.put_trace(new_block);
		
		//put trace is in TR_PAY.tr_buf, instructions are in PAY.buf
		unsigned int tr_index;
		tr_index = TR_PAY.push();
		TR_PAY.tr_buf[tr_index] = TFill.trace_fill_buffer;
		TR_PAY.tr_buf[tr_index].valid = true;
		for(int i=0;i<TFill.trace_fill_buffer.tr_size;i++)
		{
			int index;
			index = PAY.push();
			if(i==0)  //First instruction in trace
			{
				TR_PAY.tr_buf[tr_index].start_index = index;
			}
			if(i == TFill.trace_fill_buffer.tr_size-1)  //Last instruction in trace
			{
				TR_PAY.tr_buf[tr_index].end_index = index;
			}
			PAY.buf[index] = TFill.buf[i*2];
			PAY.buf[index].trace_info_index = tr_index;  //Remember to do this in slow path

			PAY.buf[index+1] = TFill.buf[i*2+1];
			PAY.buf[index+1].trace_info_index = tr_index;  //Remember to do this in slow path

			// Link the instruction to corresponding instruction in functional simulator.
			PAY.map_to_actual(index, Tid);

			PAY.buf[index+1].db_index = PAY.buf[index].db_index;
			PAY.buf[index+1].good_instruction = PAY.buf[index].good_instruction;		
		}	

		TR_PAY.tr_buf[tr_index].pred_index = TR_FETCH.table_index;
		TR_PAY.tr_buf[tr_index].pred_pc = TR_FETCH.pc;
		TR_PAY.tr_buf[tr_index].pred_br_flag = TR_FETCH.br_flag;
		
		//put the trace level index in pipeline register
		TR_RENAME.info_index = tr_index;
		
		//Set valid bit of it
		TR_RENAME.valid = true;

		// Make invalid entries in TR_FETCH pipeline register
		TR_FETCH.valid = false;		
		return;
	}

	//Check if any kind of misprediction has been detected.
	//If not, continue with access to T$ using the predicted trace ID from TP.
	//Stall if trace_rename stage is full.
	if(fast_path_fetch)
	{
		//Access T$ using the prediction from trace predictor.
		bool hit;
		hit = TC.tr_access(TR_FETCH.pc, TR_FETCH.br_flag);
		t_access++;

		if (hit)
		{
			fetch_mode = 0;
			t_hit++;
		}
			
		else  //TC miss
		{
			fetch_mode = 3;	
			t_miss++;
		}
	}

	switch(fetch_mode)
	{
		case 0:
			// We know that it is a trace cache hit. So we fetch from trace cache and send to global rename stage.
			//Fast-Path fetching					
			//Get the trace from trace cache
			trace_block new_block;
	
			new_block = TC.get_trace(TR_FETCH.pc, TR_FETCH.br_flag);
			//push the new trace into TR_PAY.tr_buf[]
			unsigned int tr_index;
			tr_index = TR_PAY.push();
			TR_PAY.tr_buf[tr_index] = new_block.tr_info;
			TR_PAY.tr_buf[tr_index].valid = true;

			TR_PAY.tr_buf[tr_index].pred_index = TR_FETCH.table_index;
			TR_PAY.tr_buf[tr_index].pred_pc = TR_FETCH.pc;
			TR_PAY.tr_buf[tr_index].pred_br_flag = TR_FETCH.br_flag;
			
			for(int i=0;i<new_block.tr_info.tr_size;i++)
			{
				int index;
				index = PAY.push();
				if(i==0)  //First instruction in trace
				{
					TR_PAY.tr_buf[tr_index].start_index = index;
				}
				if(i == new_block.tr_info.tr_size-1)  //Last instruction in trace
				{
					TR_PAY.tr_buf[tr_index].end_index = index;
				}
				PAY.buf[index] = new_block.buf[i*2];
				PAY.buf[index].trace_info_index = tr_index;  //Remember to do this in slow path

				PAY.buf[index+1] = new_block.buf[i*2+1];
				PAY.buf[index+1].trace_info_index = tr_index;  //Remember to do this in slow path

									
				// Link the instruction to corresponding instruction in functional simulator.
				PAY.map_to_actual(index, Tid);

				PAY.buf[index+1].db_index = PAY.buf[index].db_index;
				PAY.buf[index+1].good_instruction = PAY.buf[index].good_instruction;
				//PAY.split(index);
			}

			//Send the trace to global rename stage
			TR_RENAME.valid = true;
			TR_RENAME.info_index = tr_index;

			// Make invalid entries in TR_FETCH pipeline register
			TR_FETCH.valid = false;

			//Update speculatively TP pattern table
			TP.PT[TR_FETCH.table_index].pc = TR_FETCH.pc;
			TP.PT[TR_FETCH.table_index].br_flag =  TR_FETCH.br_flag;

			//Speculatively update trace predictor history
			TP.put_hash(TR_FETCH.pc, TR_FETCH.br_flag);
			break;
			
		case 1:
			// Slow path mode. Use slow_path_PC (offending_pc + 8) and branch predictor outcomes in slow path.
			next_fetch_cycle = TFill.trace_fill(IC,slow_path_fetch_pc,TR_FETCH.br_flag,true,false,0,0,0,Tid,cycle,wait_for_trap);

			if(exptn == false)
				{
					//Update speculatively TP pattern table
					TP.PT[TR_FETCH.table_index].pc = TFill.trace_fill_buffer.start_pc;
					TP.PT[TR_FETCH.table_index].br_flag = TFill.trace_fill_buffer.br_flag ;
					
					//Update speculatively TP history
					TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);	
				}
			else if(exptn == true)
				{
					//Update speculatively TP pattern table
					TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].pc = TFill.trace_fill_buffer.start_pc;
					TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].br_flag = TFill.trace_fill_buffer.br_flag ;
					
					//Update speculatively TP history
					TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);	
					exptn = false;
				}
			break;
			
		case 2:
			// Slow path mode. Use slow_path_PC (previous trace fall through pc) and branch predictor outcomes in slow path.
			next_fetch_cycle = TFill.trace_fill(IC,slow_path_fetch_pc,TR_FETCH.br_flag,true,false,0,0,0,Tid,cycle,wait_for_trap);

			//Update speculatively TP pattern table
			TP.PT[TR_FETCH.table_index].pc = TFill.trace_fill_buffer.start_pc;
			TP.PT[TR_FETCH.table_index].br_flag = TFill.trace_fill_buffer.br_flag ;

			//Update speculatively TP history
			TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);


			break;
			
		case 3:
			// Slow path mode. Use start_pc and branch_flags from trace predictor to access slow path.
			next_fetch_cycle = TFill.trace_fill(IC,TR_FETCH.pc,TR_FETCH.br_flag,false,false,0,0,0,Tid,cycle,wait_for_trap);

			//Update speculatively TP pattern table
			TP.PT[TR_FETCH.table_index].pc = TFill.trace_fill_buffer.start_pc;
			TP.PT[TR_FETCH.table_index].br_flag = TFill.trace_fill_buffer.br_flag ;

			//Update speculatively TP history
			TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);


			break;
			
		case 4:
			// Partial slow path mode. Keep prefix as it is, fetch the suffix using the slow path and branch predictor.
			next_fetch_cycle = TFill.trace_fill(IC,slow_path_fetch_pc,TR_FETCH.br_flag,true,true,mode4_n,mode4_m,mode4_br_flag,Tid,cycle,wait_for_trap);

			//8b. Update pattern table
			TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].pc = TFill.trace_fill_buffer.start_pc;
			TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].br_flag = TFill.trace_fill_buffer.br_flag ;
			
			//8c. Update speculatively TP history
			TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);
			break;
			
		case 5:
			// Slow path mode. Access slow path using the addr given by previous PE taken or fall through addr. Use branch predictor.
			next_fetch_cycle = TFill.trace_fill(IC,slow_path_fetch_pc,TR_FETCH.br_flag,true,false,0,0,0,Tid,cycle,wait_for_trap);

			//8b. Update pattern table
			TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].pc = TFill.trace_fill_buffer.start_pc;
			TP.PT[TR_PAY.tr_buf[TR_PAY.head].pred_index].br_flag = TFill.trace_fill_buffer.br_flag ;
			
			//8c. Update speculatively TP history
			TP.put_hash(TFill.trace_fill_buffer.start_pc, TFill.trace_fill_buffer.br_flag);


			break;
	}


}  //final end of trace_fetch

