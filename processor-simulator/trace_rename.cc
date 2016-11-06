
#include "processor.h"

// Global rename stage.
void processor::trace_rename()
{
	unsigned int i;
	unsigned int next, len;
	unsigned int iter;

	////////////////////////////////////////////////////////////////////////////////////
	// Try to get the next renaming trace.
	// Two conditions might prevent getting the next rename trace, either:
	// (1) The current dispatch trace is stalled in trace_dispatch.
	// (2) There is no new trace to rename in the TR_RENAME pipeline.
	////////////////////////////////////////////////////////////////////////////////////

	// Check whether or not the pipeline register
	// between rename and dispatch still has a rename trace.


#ifdef DEBUG
	if(cycle >= 65)
	{
		printf("AMT : \n");
		for(int i=0 ;i<REN_INT->logRegs; i++)
		{
			printf(" AMT[%d] = %d\n",i,REN_INT->AMT[i]);
		}

		printf("RMT : \n");
		for(int i=0 ;i<REN_INT->logRegs; i++)
		{
			printf(" RMT[%d] = %d\n",i,REN_INT->RMT[i]);
		}

		printf(" Free List	head = %d  		tail = %d\n", REN_INT->freeList->head, REN_INT->freeList->tail);
		for(int i=0 ;i<REN_INT->freeList->FL_size; i++)
		{
			printf(" FL[%d] = %d\n",i,REN_INT->freeList->FL[i]);
		}
	}
	
#endif

	// First stall condition.
	if (TR_DISPATCH.valid)	// The current dispatch trace is stalled.
		return;

	// Check the second condition. Does the FE/RN pipeline have a new trace to rename.
	if (!TR_RENAME.valid)
		return;

	if(TR_PAY.is_empty())
		return;



	// Get the next rename trace.
	unsigned int start_index = TR_PAY.tr_buf[TR_RENAME.info_index].start_index;
	unsigned int end_index = TR_PAY.tr_buf[TR_RENAME.info_index].end_index;

	unsigned int chckpts_needed = 0, int_dest_needed = 0, fltpt_dest_needed = 0;

	// Third stall condition: There aren't enough rename resources for the current rename bundle.
	next = start_index;
	len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
	len = (len >> 1) + 1;
	len = len << 1;
	iter = 0;
	while(iter < len)
	{	
		unsigned int i = next;
      		assert(TR_RENAME.valid);

		// +Parth
		// Count the number of instructions in the rename bundle that have an integer destination register.
		// Count the number of instructions in the rename bundle that have a floating-point destination register.
		// With these counts, you will be able to query the integer and floating-point renamers for resource availability

		if(PAY.buf[i].checkpoint == true)  //not needed
		 	chckpts_needed++;

		if(PAY.buf[i].C_valid == true)
		{
			//INT
			if(PAY.buf[i].C_int == true && PAY.buf[i].C_live_out == true)
				int_dest_needed++;
			//FP
			else if(PAY.buf[i].C_int == false && PAY.buf[i].C_live_out == true) 
				fltpt_dest_needed++;
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
	} //while loop ends

	// +Parth
	// Check if the trace_rename Stage must stall due to any of the following conditions:
	// * Not enough free integer physical registers.
	// * Not enough free floating-point physical registers.
	//
	if(REN_INT->stall_reg(int_dest_needed) == true)
		return;

	if(REN_FP->stall_reg(fltpt_dest_needed) == true)
		return;

	// Sufficient resources are available to rename the trace.
	// First, checkpoint the arch state. So checkpoints point to the 
	// state before our trace itself is renamed.

	assert(TR_RENAME.valid);
	
	// +Parth
	// Checkpoint every trace before global renaming.
	//
	unsigned int int_chckpt = REN_INT->checkpoint(TR_PAY, TP);
	unsigned int flt_chckpt = REN_FP->checkpoint(TR_PAY, TP);
	assert(int_chckpt == flt_chckpt);
	TR_PAY.tr_buf[TR_RENAME.info_index].checkpoint_ID = int_chckpt;

	next = start_index;
	len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
	len = (len >> 1) + 1;
	len = len << 1;
	iter = 0;
	while(iter < len)
	{	
		unsigned int i = next;

		// Rename source registers (first) and destination register (second).
		//
		if(PAY.buf[i].A_valid == true)
		{
			if(PAY.buf[i].A_int == true && PAY.buf[i].A_live_in == true)
				PAY.buf[i].A_global_phys_reg = REN_INT->rename_rsrc(PAY.buf[i].A_log_reg);
			else if(PAY.buf[i].A_int == false && PAY.buf[i].A_live_in == true)
				PAY.buf[i].A_global_phys_reg = REN_FP->rename_rsrc(PAY.buf[i].A_log_reg);
		}

		if(PAY.buf[i].B_valid == true)
		{
			if(PAY.buf[i].B_int == true && PAY.buf[i].B_live_in == true)
				PAY.buf[i].B_global_phys_reg = REN_INT->rename_rsrc(PAY.buf[i].B_log_reg);
			else if(PAY.buf[i].B_int == false && PAY.buf[i].B_live_in == true)
				PAY.buf[i].B_global_phys_reg = REN_FP->rename_rsrc(PAY.buf[i].B_log_reg);
		}

		if(PAY.buf[i].C_valid == true)
		{
			if(PAY.buf[i].C_int == true && PAY.buf[i].C_live_out == true)
				PAY.buf[i].C_global_phys_reg = REN_INT->rename_rdst(PAY.buf[i].C_log_reg);
			else if(PAY.buf[i].C_int == false && PAY.buf[i].C_live_out == true)
				PAY.buf[i].C_global_phys_reg = REN_FP->rename_rdst(PAY.buf[i].C_log_reg);
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
	}//while loop ends
	
	// Transfer the rename trace from the Rename Stage to the trace dispatch Stage.
	
	TR_DISPATCH.info_index = TR_RENAME.info_index;
	TR_RENAME.valid = false;
	TR_DISPATCH.valid = true; 
}

