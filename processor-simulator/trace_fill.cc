#include "processor.h"

trace_fill_unit::trace_fill_unit(unsigned int max_n, unsigned int max_m)
{
	trace_done = false;
	this->max_n = max_n;  //max instructions say 16
	this->max_m = max_m;  //max embedded blocks say 3 -> hence 2 branches allowed
	n=0; m=0;
	busy = false;
	
}

bool trace_fill_unit::is_trace_done()
{
	return trace_done;
}

bool trace_fill_unit::is_busy()
{
	return busy;
}

SS_TIME_TYPE trace_fill_unit::access_IC(CacheClass &IC, unsigned int pc, unsigned int Tid, SS_TIME_TYPE cycle)
{
	//Access ICache

	unsigned int line1;				// I$
	unsigned int line2;				// I$
	bool hit1;						// I$
	bool hit2;						// I$
	SS_TIME_TYPE resolve_cycle1;			// I$
	SS_TIME_TYPE resolve_cycle2;			// I$

	SS_TIME_TYPE next_fetch_cycle;

	/////////////////////////////
	// I-cache access.
	/////////////////////////////

	if (!PERFECT_ICACHE) {
	   if (IC_INTERLEAVED) {
	      // Access two consecutive lines.
	      line1 = (pc >> L1_IC_LINE_SIZE);
	      line2 = (pc >> L1_IC_LINE_SIZE) + 1;
	      resolve_cycle1 = IC.Access(Tid, cycle, (line1 << L1_IC_LINE_SIZE), false, &hit1);
	      resolve_cycle2 = IC.Access(Tid, cycle, (line2 << L1_IC_LINE_SIZE), false, &hit2);

	      if (!hit1) {
		 if (!hit2)
	            next_fetch_cycle = MAX(resolve_cycle1, resolve_cycle2);	// Both lines missed.
		 else
		    next_fetch_cycle = resolve_cycle1;				// Line1 missed, line2 hit.

	         return next_fetch_cycle;
	      }
	      else if (!hit2) {
		 next_fetch_cycle = resolve_cycle2;				// Line1 hit, line2 missed.
		 return next_fetch_cycle;
	      }
	   }
	   else {
	      // Access only one line.
	      line1 = (pc >> L1_IC_LINE_SIZE);
	      resolve_cycle1 = IC.Access(Tid, cycle, (line1 << L1_IC_LINE_SIZE), false, &hit1);

	      if (!hit1) {
		 next_fetch_cycle = resolve_cycle1;				// Line1 missed.
		 return next_fetch_cycle;
	      }
	   }
	}

	return cycle;
}


void trace_fill_unit::fill(unsigned int pc, bool &exit_flag, unsigned int br_flags, bool br_pred, 
		unsigned int Tid, bool wait_for_trap)
{
	unsigned int i = 0;
	bool stop = false;
	SS_INST_TYPE inst;
	unsigned int pred_tag;
	unsigned int next_pc;
	unsigned int direct_target;
	bool conf;				// dummy
	bool fm;				// dummy
	unsigned int history_reg;
	
	unsigned int br_flg = br_flags;
	unsigned int index;
	
	unsigned int line1;				// I$
	unsigned int line2;				// I$
	
	while((n<max_n) && (m<max_m) && (!stop))
	{
		  //////////////////////////////////////////////////////
	      // Initialize some important flags.
	      //////////////////////////////////////////////////////
	      //pred_tag = 0;
	      //history_reg = 0xFFFFFFFF;

	      //////////////////////////////////////////////////////
	      // Fetch instruction from the binary.
	      //////////////////////////////////////////////////////
	      THREAD[Tid]->fetch(pc, inst);

	      //////////////////////////////////////////////////////
	      // Set next_pc and the prediction tag.
	      //////////////////////////////////////////////////////

			switch(SS_OPCODE(inst)) 
			{
				case JUMP: case JAL:
					direct_target = ((pc & 036000000000) | (TARG << 2));
					
					//next_pc = direct_target;
					/* (PERFECT_BRANCH_PRED ?
					  (wait_for_trap ? direct_target : THREAD[Tid]->pop_pc()) :
					  BP.get_pred(history_reg, pc, inst, direct_target, &pred_tag, &conf, &fm));
					assert(next_pc == direct_target);*/

					next_pc = direct_target;	//Predict taken --- backward branch
					fill_br_flag = (1<<m) | fill_br_flag;
					m++;
					
					//Set fall through and taken target
					trace_fill_buffer.taken_addr = direct_target;
					trace_fill_buffer.fall_addr = direct_target;		
					
					stop = true;
					break;

				case JR: case JALR:
	
					/*next_pc =
					 (PERFECT_BRANCH_PRED ?
					  (wait_for_trap ? (pc + 8) : THREAD[Tid]->pop_pc()) :
					  BP.get_pred(history_reg, pc, inst, 0, &pred_tag, &conf, &fm));*/
					  
					next_pc = pc + 8;
						
					//Set fall through and taken target  ????????????????????????????????????
					trace_fill_buffer.taken_addr = pc + 8;
					trace_fill_buffer.fall_addr = pc + 8;
						
					stop = true;
					exit_flag = true;
					break;

				case BEQ : case BNE : case BLEZ: case BGTZ:
				case BLTZ: case BGEZ: case BC1F: case BC1T:
					direct_target = (pc + 8 + (OFS << 2));

					/*next_pc =
					 (PERFECT_BRANCH_PRED ?
					  (wait_for_trap ? (pc + 8) : THREAD[Tid]->pop_pc()) :
					  BP.get_pred(history_reg, pc, inst, direct_target, &pred_tag, &conf, &fm));
					assert(next_pc == direct_target || next_pc == pc + 8);*/
					
					if(br_pred == true)   //We need to take prediction from branch predictor
					{
						if(direct_target<pc)
						{
							next_pc = direct_target;	//Predict taken --- backward branch
							fill_br_flag = (1<<m) | fill_br_flag;
						}
						else
						{
							next_pc = pc + 8;			//Predict not taken --- forward branch
						}
					}
					else				//We need to take predictions from br_flags
					{
						if(((br_flg) & (1<<m)) == 0)
						{
							next_pc = pc + 8;		//Not taken path
						}
						else
						{
							next_pc = direct_target;		//Taken path
							fill_br_flag = (1<<m) | fill_br_flag;
						}
					}
					m++;
					
					//Set fall through and taken target
					trace_fill_buffer.taken_addr = direct_target;
					trace_fill_buffer.fall_addr = pc + 8;
					
					if (next_pc != pc + 8)
					   stop = true;
					break;

				case SYSCALL:	case BREAK:

					next_pc = pc + 8;
					trace_fill_buffer.taken_addr = pc + 8;
					trace_fill_buffer.fall_addr = pc + 8;

					stop = true;
					exit_flag = true;
					break;

				default:
					/*next_pc = (PERFECT_BRANCH_PRED ?
						(wait_for_trap ? (pc + 8) : THREAD[Tid]->pop_pc()) :
						(pc + 8));*/
					next_pc = pc + 8;
					trace_fill_buffer.taken_addr = pc + 8;
					trace_fill_buffer.fall_addr = pc + 8;
					break;
			}

	// This is needed for perfect branch prediction to work properly when fetching past trap instructions.
	// Specifically, wait_for_trap prevents popping PCs from the functional simulator after a trap instruction.
	if ((SS_OPCODE(inst) == SYSCALL) || (SS_OPCODE(inst) == BREAK))
		wait_for_trap = PERFECT_BRANCH_PRED;	
 
	// If not already stopped:
	// Stop if the I$ is not interleaved and if a line boundary is crossed.
	if (!stop && !IC_INTERLEAVED)
	{
		line1 = (pc >> L1_IC_LINE_SIZE);
		line2 = (next_pc >> L1_IC_LINE_SIZE);
		stop = (line1 != line2);
	}
	  
	index = n*2;   //push instruction to trace's location
	buf[index].inst.a = inst.a;
	buf[index].inst.b = inst.b;
	buf[index].pc = pc;
	buf[index].next_pc = next_pc;
	buf[index].pred_tag = 0;	//Not using branch predictor	
	
	//Set LQ/SQ defaults
	buf[index].LQ_index = 0;	
	buf[index].LQ_phase = 0;
	buf[index].SQ_index = 0;
	buf[index].SQ_phase = 0;

	//Checkpoints for LSU
	buf[index].LQ_index_cp = 0;	
	buf[index].LQ_phase_cp = 0;
	buf[index].SQ_index_cp = 0;
	buf[index].SQ_phase_cp = 0;

	buf[index].is_resolved = false;
	buf[index].is_mispredict= false;

	// Go to next PC.
	pc = next_pc;	 
	
	//increment instruction count		
	n++;			
	
	//Set exit flag if either instr are 16 or m is 3
	//This can terminate the calling while loop
	if((m==max_m)||(n==max_n))
	{
		stop = true;
		exit_flag = true;	
	}
	
	} //while ends here
	
	return_pc = next_pc;

}

unsigned int trace_fill_unit::trace_fill(CacheClass &IC, unsigned int start_pc, 
	unsigned int br_flags, bool br_pred, bool partial, unsigned int mode4_n, unsigned int mode4_m, 
		unsigned int mode4_br_flag, unsigned int Tid, SS_TIME_TYPE cycle, bool wait_for_trap)
{
	unsigned int total_next_fetch_cycle = cycle;
	bool exit_flag = false;
	unsigned int trace_index;
	unsigned int pc;
	
	if(partial)
	{
		n = mode4_n;
		m = mode4_m;
		fill_br_flag = mode4_br_flag;
		trace_fill_buffer.start_pc = buf[0].pc;
		now_servicing_pc = buf[0].pc;
	}
	else
	{
		n=0;
		m=0;
		fill_br_flag = 0;
		trace_fill_buffer.start_pc = start_pc;
		now_servicing_pc = start_pc;
	}
	fill_ends_in_br = false;
	busy = true;
	pc = start_pc;
	
	//Push trace on TR_PAY.tr_buf
	//trace_info_index = TR_PAY.push();
	
	trace_fill_buffer.valid = true;
	trace_fill_buffer.trace_mispredict = false;
	trace_fill_buffer.completed = false;
	trace_fill_buffer.exception = false;
	trace_fill_buffer.br_mispredict = false;
	trace_fill_buffer.mode5_pc = 0;
	trace_fill_buffer.checkpoint_ID = TOTAL_PE + 1;

	unsigned int temp_cycles = 0;
	
	while(!exit_flag)
		{
			//Access I$ with the pc
			temp_cycles  += access_IC(IC, pc, Tid, cycle) - cycle;

			//Check if the instrs has branch and accordingly fill the fill buffer
			fill(pc, exit_flag, br_flags, br_pred, Tid, wait_for_trap);
			
			pc = return_pc;
			
		}
	
	total_next_fetch_cycle += temp_cycles + 1;

	//Set trace_info flags
	
	trace_fill_buffer.tr_size = n;
	trace_fill_buffer.br_mask = m;
	trace_fill_buffer.br_flag = fill_br_flag;
	
	//trace_fill_buffer.taken_addr = return_pc;     Already set earlier
	//trace_fill_buffer.fall_addr = return_pc;
	
	
	//Call pre decoding logic
	pre_decode();
	//total_next_fetch_cycle +=1;
	
	//Call pre renaming logic
	pre_rename();
	//total_next_fetch_cycle +=1;

#if 0

	for(int i=0; i<2*n; i++)
	{
		printf("C:<%d> %d		A:<%d> %d		B:<%d> %d\n",buf[i].C_valid,buf[i].C_log_reg,buf[i].A_valid,buf[i].A_log_reg,buf[i].B_valid,buf[i].B_log_reg);
	}

	for(int i=0; i<2*n; i++)
	{
		printf("C:<%d> <%d> %d		A:<%d> <%d> %d		B:<%d> <%d> %d\n",buf[i].C_live_out,buf[i].C_local,buf[i].C_local_phys_reg,buf[i].A_live_in,buf[i].A_local,buf[i].A_local_phys_reg,buf[i].B_live_in,buf[i].B_local,buf[i].B_local_phys_reg);
	}
#endif
	//Set ends in branch flag
	if((buf[(n-1)*2].checkpoint == true) && (SS_OPCODE(buf[(n-1)*2].inst) != JR) && (SS_OPCODE(buf[(n-1)*2].inst) != JALR))
	{
		fill_ends_in_br = true;
	}
	if((SS_OPCODE(buf[(n-1)*2].inst) == JUMP) || (SS_OPCODE(buf[(n-1)*2].inst) == JAL))
	{
		fill_ends_in_br = true;
	}

	trace_fill_buffer.ends_in_br = fill_ends_in_br;

	return total_next_fetch_cycle;
}

void trace_fill_unit::pre_decode()
{
	unsigned int index;
	SS_INST_TYPE inst;
	
	for(int index=0; index<TRACE_SIZE*2; index++)
	{
		buf[index].split = false;
		buf[index].split_store = false;
		buf[index].A_valid = false;
		buf[index].B_valid = false;
		buf[index].C_valid = false;
		buf[index].A_live_in = false;
		buf[index].A_local = false;
		buf[index].B_live_in = false;
		buf[index].B_local = false;
		buf[index].C_live_out = false;
		buf[index].C_local = false;
	}
		  
	for(int i=0; i<n; i++)
	{
		index = i*2;
		
		// Get instruction from payload buffer.
		inst.a = buf[index].inst.a;
		inst.b = buf[index].inst.b;

		// Set checkpoint flag.
		switch (SS_OPCODE(inst)) 
		{
			case JR: case JALR:
			case BEQ: case BNE: case BLEZ: case BGTZ: case BLTZ: case BGEZ: case BC1F: case BC1T:
				buf[index].checkpoint = true;
				break;

			default:
				buf[index].checkpoint = false;
				break;
		}
		
		// Set flags
		switch (SS_OPCODE(inst)) 
		{
			 case JUMP: case JAL: case JR: case JALR:
				buf[index].flags = (F_CTRL|F_UNCOND);
				break;

			 case BEQ: case BNE: case BLEZ: case BGTZ: case BLTZ: case BGEZ: case BC1F: case BC1T:
				buf[index].flags = (F_CTRL|F_COND);
				break;

			 case LB: case LBU: case LH: case LHU: case LW: case DLW: case L_S: case L_D: case LWL: case LWR:
				buf[index].flags = (F_MEM|F_LOAD|F_DISP);
				break;

			 case SB: case SH: case SW: case DSW: case DSZ: case S_S: case S_D: case SWL: case SWR:
				buf[index].flags = (F_MEM|F_STORE|F_DISP);
				break;

			 case ADD: case ADDI: case ADDU: case ADDIU: case SUB: case SUBU:
			 case MFHI: case MTHI: case MFLO: case MTLO:
			 case AND_: case ANDI: case OR: case ORI: case XOR: case XORI: case NOR:
			 case SLL: case SLLV: case SRL: case SRLV: case SRA: case SRAV:
			 case SLT: case SLTI: case SLTU: case SLTIU:
			 case LUI:
			 case MFC1: case DMFC1: case MTC1: case DMTC1:
			 case NOP:
				buf[index].flags = (F_ICOMP);
				break;

			 case MULT: case MULTU: case DIV: case DIVU:
				buf[index].flags = (F_ICOMP|F_LONGLAT);
				break;

			 case FADD_S: case FADD_D: case FSUB_S: case FSUB_D:
			 case FABS_S: case FABS_D: case FNEG_S: case FNEG_D: case FMOV_S: case FMOV_D:
			 case CVT_S_D: case CVT_S_W: case CVT_D_S: case CVT_D_W: case CVT_W_S: case CVT_W_D:
			 case C_EQ_S: case C_EQ_D: case C_LT_S: case C_LT_D: case C_LE_S: case C_LE_D:
				buf[index].flags = (F_FCOMP);
				break;

			 case FMUL_S: case FMUL_D: case FDIV_S: case FDIV_D: case FSQRT_S: case FSQRT_D:
				buf[index].flags = (F_FCOMP|F_LONGLAT);
				break;

			 case SYSCALL: case BREAK:
				buf[index].flags = (F_TRAP);
				break;

			 default:
				assert(0);
				break;
		}
		
		// Set function unit type
		switch (SS_OPCODE(inst)) 
		{
			case JUMP: case JAL: case JR: case JALR:
			 case BEQ: case BNE: case BLEZ: case BGTZ: case BLTZ: case BGEZ: case BC1F: case BC1T:
			 case SYSCALL: case BREAK:
				buf[index].fu = FU_BR;
				break;

			 case LB: case LBU: case LH: case LHU: case LW: case DLW: case LWL: case LWR:
			 case SB: case SH: case SW: case DSW: case DSZ: case SWL: case SWR:
				buf[index].fu = FU_LS;
				break;

			 case ADD: case ADDI: case ADDU: case ADDIU: case SUB: case SUBU:
			 case MFHI: case MTHI: case MFLO: case MTLO:
			 case AND_: case ANDI: case OR: case ORI: case XOR: case XORI: case NOR:
			 case SLL: case SLLV: case SRL: case SRLV: case SRA: case SRAV:
			 case SLT: case SLTI: case SLTU: case SLTIU:
			 case LUI:
			 case NOP:
				buf[index].fu = FU_ALU_S;
				break;

			 case MULT: case MULTU: case DIV: case DIVU:
				buf[index].fu = FU_ALU_C;
				break;

			 case L_S: case L_D:
			 case S_S: case S_D: 
				buf[index].fu = FU_LS_FP;
				break;

			 case FADD_S: case FADD_D: case FSUB_S: case FSUB_D:
			 case FABS_S: case FABS_D: case FNEG_S: case FNEG_D: case FMOV_S: case FMOV_D:
			 case CVT_S_D: case CVT_S_W: case CVT_D_S: case CVT_D_W: case CVT_W_S: case CVT_W_D:
			 case C_EQ_S: case C_EQ_D: case C_LT_S: case C_LT_D: case C_LE_S: case C_LE_D:
			 case FMUL_S: case FMUL_D: case FDIV_S: case FDIV_D: case FSQRT_S: case FSQRT_D:
				buf[index].fu = FU_ALU_FP;
				break;

			 case MFC1: case DMFC1: case MTC1: case DMTC1:
				buf[index].fu = FU_MTF;
				break;

			 default:
				assert(0);
				break;
		}

		  // Set register operands and split instructions.
		  // Select IQ.

		  // Default values.
		  buf[index].split = false;
		  buf[index].split_store = false;
		  buf[index].A_valid = false;
		  buf[index].B_valid = false;
		  buf[index].C_valid = false;	

		switch (SS_OPCODE(inst)) 
		{
			case JUMP: case NOP:
				// Select IQ.
				buf[index].iq = SEL_IQ_NONE;
				break;

			case JAL:
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = 31;
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			case JR:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			case JALR:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RD);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case BEQ: case BNE:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case BLEZ: case BGTZ: case BLTZ: case BGEZ:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case BC1F: case BC1T:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (FCC_ID_NEW);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case LB: case LBU: case LH: case LHU: case LW:
				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case DLW:
				// Split the instruction.
				split(index);

				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);

				buf[index+1].A_valid = true;
				buf[index+1].A_int = true;
				buf[index+1].A_log_reg = (BS);

				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);

				buf[index+1].C_valid = true;
				buf[index+1].C_int = true;
				buf[index+1].C_log_reg = ((RT)+1);

				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				buf[index+1].iq = SEL_IQ_INT;
					break;

			 case L_S: case L_D:
				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = false;
				buf[index].C_log_reg = (FT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case LWL: case LWR:
				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);
				// source register (same as dest register)
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case SB: case SH: case SW: case SWL: case SWR:
				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);
				// source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case DSW:
				// Split the instruction.
				split(index);

				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);

				buf[index+1].A_valid = true;
				buf[index+1].A_int = true;
				buf[index+1].A_log_reg = (BS);

				// source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);

				buf[index+1].B_valid = true;
				buf[index+1].B_int = true;
				buf[index+1].B_log_reg = ((RT)+1);

				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				buf[index+1].iq = SEL_IQ_INT;
				break;

			 case DSZ:
				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case S_S: case S_D:
				// Implement split-stores for these, due to referencing both int and fp source regs.
				buf[index].split_store = true;

				// Split the instruction.
				split(index);

				// base register for AGEN
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (BS);

				// source register
				buf[index+1].A_valid = true;
				buf[index+1].A_int = false;
				buf[index+1].A_log_reg = (FT);

				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				buf[index+1].iq = SEL_IQ_FP;
					break;

			 case ADD: case ADDU: case SUB: case SUBU:
			 case AND_: case OR: case XOR: case NOR:
			 case SLLV: case SRLV: case SRAV:
			 case SLT: case SLTU:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RD);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case SLL: case SRL: case SRA:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RD);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case ADDI: case ADDIU:
			 case ANDI: case ORI: case XORI:
			 case SLTI: case SLTIU:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case MULT: case MULTU: case DIV: case DIVU:
				// Split the instruction.
				split(index);

				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);

				buf[index+1].A_valid = true;
				buf[index+1].A_int = true;
				buf[index+1].A_log_reg = (RS);

				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = (RT);

				buf[index+1].B_valid = true;
				buf[index+1].B_int = true;
				buf[index+1].B_log_reg = (RT);

				// destination register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (HI_ID_NEW);

				buf[index+1].C_valid = true;
				buf[index+1].C_int = true;
				buf[index+1].C_log_reg = (LO_ID_NEW);

				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				buf[index+1].iq = SEL_IQ_INT;
				break;

			 case MFHI:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (HI_ID_NEW);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RD);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case MTHI:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (HI_ID_NEW);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case MFLO:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (LO_ID_NEW);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RD);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case MTLO:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (LO_ID_NEW);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case FADD_S: case FADD_D: case FSUB_S: case FSUB_D: case FMUL_S: case FMUL_D: case FDIV_S: case FDIV_D:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = false;
				buf[index].A_log_reg = (FS);
				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = false;
				buf[index].B_log_reg = (FT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = false;
				buf[index].C_log_reg = (FD);
				// Select IQ.
				buf[index].iq = SEL_IQ_FP;
					break;

			 case FABS_S: case FABS_D: case FMOV_S: case FMOV_D: case FNEG_S: case FNEG_D:
			 case CVT_S_D: case CVT_S_W: case CVT_D_S: case CVT_D_W: case CVT_W_S: case CVT_W_D:
			 case FSQRT_S: case FSQRT_D:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = false;
				buf[index].A_log_reg = (FS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = false;
				buf[index].C_log_reg = (FD);
				// Select IQ.
				buf[index].iq = SEL_IQ_FP;
					break;

			 case C_EQ_S: case C_EQ_D: case C_LT_S: case C_LT_D: case C_LE_S: case C_LE_D:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = false;
				buf[index].A_log_reg = (FS);
				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = false;
				buf[index].B_log_reg = (FT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (FCC_ID_NEW);
				// Select IQ.
				buf[index].iq = SEL_IQ_FP;
					break;

			 case MFC1:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = false;
				buf[index].A_log_reg = (FS);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_FP;
				break;

			 case DMFC1:
				// Split the instruction.
				split(index);

				// source register
				buf[index].A_valid = true;
				buf[index].A_int = false;
				buf[index].A_log_reg = (FS);

				buf[index+1].A_valid = true;
				buf[index+1].A_int = false;
				buf[index+1].A_log_reg = (FS);

				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);

				buf[index+1].C_valid = true;
				buf[index+1].C_int = true;
				buf[index+1].C_log_reg = ((RT)+1);

				// Select IQ.
				buf[index].iq = SEL_IQ_FP;
				buf[index+1].iq = SEL_IQ_FP;
					break;

			 case MTC1:
				// source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RT);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = false;
				buf[index].C_log_reg = (FS);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
				break;

			 case DMTC1:
				// first source register
				buf[index].A_valid = true;
				buf[index].A_int = true;
				buf[index].A_log_reg = (RT);
				// second source register
				buf[index].B_valid = true;
				buf[index].B_int = true;
				buf[index].B_log_reg = ((RT)+1);
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = false;
				buf[index].C_log_reg = (FS);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			 case SYSCALL: case BREAK:
				// Select IQ.
				buf[index].iq = SEL_IQ_NONE_EXCEPTION;
					break;

			 case LUI:
				// dest register
				buf[index].C_valid = true;
				buf[index].C_int = true;
				buf[index].C_log_reg = (RT);
				// Select IQ.
				buf[index].iq = SEL_IQ_INT;
					break;

			default:
				assert(0);
				break;
		}

		// Decode some details about loads and stores:
		// size and sign of data, and left/right info.
		switch (SS_OPCODE(inst)) 
		{
			 case DLW: case DSW:
				assert(buf[index].split && buf[index].upper);
				buf[index].size = 4;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = false;

				assert(buf[index+1].split && !buf[index+1].upper);
				buf[index+1].size = 4;
				buf[index+1].is_signed = false;
				buf[index+1].left = false;
				buf[index+1].right = false;
				break;

			 case L_D: case S_D: case DSZ:
					buf[index].size = 8;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = false;
					break;

				 case LWL: case SWL:
				buf[index].size = 4;
				buf[index].is_signed = false;
				buf[index].left = true;
				buf[index].right = false;
				break;

				 case LWR: case SWR:
				buf[index].size = 4;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = true;
				break;

				 case LB:
				buf[index].size = 1;
				buf[index].is_signed = true;
				buf[index].left = false;
				buf[index].right = false;
				break;

				 case LH:
				buf[index].size = 2;
				buf[index].is_signed = true;
				buf[index].left = false;
				buf[index].right = false;
				break;

				 case LBU: case SB:
				buf[index].size = 1;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = false;
				break;

				 case LHU: case SH:
				buf[index].size = 2;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = false;
				break;

				 case LW: case L_S:
				 case SW: case S_S:
				buf[index].size = 4;
				buf[index].is_signed = false;
				buf[index].left = false;
				buf[index].right = false;
				break;

			default:
					break;
		}
	
		//Set c_next_pc for JUMP
		if(SS_OPCODE(inst) == JUMP)
		{
			buf[index].c_next_pc = buf[index].next_pc;
		}


	//Push everything to next stage including split stores
	  
		
	} //end of for loop for index
}

void trace_fill_unit::pre_rename()
{
	//		Set bits:
	//		true: someone in this trace has written to this logical reg -- so its a local
	//		false: none has written -- so its a live-in
	int local_reg_val_int[TOTAL_INT_REGS];	//used to store last local values
	bool set_bits_int[TOTAL_INT_REGS];  //used to identify live-ins or locals
	int local_index_map_int[TOTAL_INT_REGS];
	
	int local_reg_val_fp[TOTAL_FP_REGS];
	bool set_bits_fp[TOTAL_FP_REGS];
	int local_index_map_fp[TOTAL_FP_REGS];
	
	//Init read_bit and write_bit to 0.
	for (int i=0; i<TOTAL_INT_REGS; i++)
	{
		local_reg_val_int[i] = -1;
		set_bits_int[i] = false;
		local_index_map_int[i] = -1;
	}

	for (int i=0; i<TOTAL_FP_REGS; i++)
	{
		local_reg_val_fp[i] = -1;
		set_bits_fp[i] = false;	
		local_index_map_fp[i] = -1;
	}	
	
	unsigned int local_reg_int = 0;
	unsigned int local_reg_fp = 0;
	
	for(int i=0; i<n*2; i++)
	{
		//check source A for live-in or local
		if (buf[i].A_valid)
		{
			if(buf[i].A_int)  // A is of type int
			{
				if(!set_bits_int[buf[i].A_log_reg]) 
				{
					buf[i].A_live_in = true;
					buf[i].A_local = false;
				}
				else		//It is local 
				{
					buf[i].A_live_in = false;
					buf[i].A_local = true;
					buf[i].A_local_phys_reg = local_reg_val_int[buf[i].A_log_reg];
					buf[local_index_map_int[buf[i].A_log_reg]].C_local = true;
				}
			}
			else		// A is of type float
			{
				if(!set_bits_fp[buf[i].A_log_reg]) 
				{
					buf[i].A_live_in = true;
					buf[i].A_local = false;
				}
				else		//It is local 
				{
					buf[i].A_live_in = false;
					buf[i].A_local = true;
					buf[i].A_local_phys_reg = local_reg_val_fp[buf[i].A_log_reg];
					buf[local_index_map_fp[buf[i].A_log_reg]].C_local = true;
				}	
			}
		}
		//Check source B for live-in or local
		if (buf[i].B_valid)
		{
			if(buf[i].B_int)   //Int
			{
				if(!set_bits_int[buf[i].B_log_reg])   // Its is a global INT live-in
				{
					buf[i].B_live_in = true;
					buf[i].B_local = false;
				}
				else 		//It is local
				{
					buf[i].B_live_in = false;
					buf[i].B_local = true;
					buf[i].B_local_phys_reg = local_reg_val_int[buf[i].B_log_reg];
					buf[local_index_map_int[buf[i].B_log_reg]].C_local = true;
				}				
			}
			else    //Float
			{
				if(!set_bits_fp[buf[i].B_log_reg])   // Its is a global INT live-in
				{
					buf[i].B_live_in = true;
					buf[i].B_local = false;
				}
				else 		//It is local
				{
					buf[i].B_live_in = false;
					buf[i].B_local = true;
					buf[i].B_local_phys_reg = local_reg_val_fp[buf[i].B_log_reg];
					buf[local_index_map_fp[buf[i].B_log_reg]].C_local = true;
				}				
			}

		}
			
		//set write bit of C
		if (buf[i].C_valid)
		{
			if (buf[i].C_int)   //Int
			{
				set_bits_int[buf[i].C_log_reg] = true;
				local_reg_val_int[buf[i].C_log_reg] = local_reg_int;
				buf[i].C_local_phys_reg = local_reg_int;
				local_index_map_int[buf[i].C_log_reg] = i;
				local_reg_int++;
				
			}
			else    //Float
			{
				set_bits_fp[buf[i].C_log_reg] == true;
				local_reg_val_fp[buf[i].C_log_reg] = local_reg_fp;
				buf[i].C_local_phys_reg = local_reg_fp;
				local_index_map_fp[buf[i].C_log_reg] = i;
				local_reg_fp++;
			}
			buf[i].C_live_out = false;
			buf[i].C_local = false;
		}
	}	
		for(int i=0; i<TOTAL_INT_REGS; i++)
		{
			if(local_index_map_int[i] != -1)
			{
				buf[local_index_map_int[i]].C_live_out = true;
			}
		}
		for(int i=0; i<TOTAL_FP_REGS; i++)
		{
			if(local_index_map_fp[i] != -1)
			{
				buf[local_index_map_fp[i]].C_live_out = true;
			}
		}
}

void trace_fill_unit::split(unsigned int index) 
{
   //assert((index+1) < PAYLOAD_BUFFER_SIZE);

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

