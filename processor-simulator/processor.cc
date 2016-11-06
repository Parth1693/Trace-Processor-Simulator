#include "processor.h"

processor::processor(unsigned int fq_size,
		     unsigned int num_chkpts,
		     unsigned int rob_size,
		     unsigned int iq_size,
		     unsigned int lq_size,
		     unsigned int sq_size,
		     unsigned int fetch_width,
		     unsigned int dispatch_width,
		     unsigned int issue_width,
		     unsigned int retire_width,
		     unsigned int fu_lane_matrix[],
		     unsigned int Tid):
LSU(lq_size, sq_size, Tid),
IC(L1_IC_SETS, L1_IC_ASSOC, L1_IC_LINE_SIZE, L1_IC_HIT_LATENCY, L1_IC_MISS_LATENCY, L1_IC_NUM_MHSRs, L1_IC_MISS_SRV_PORTS, L1_IC_MISS_SRV_LATENCY),
TC(TC_ASSOC,TC_BLOCKSIZE,TC_CACHESIZE,MAX_M),
TP(TP_TABLE_SIZE,HASHED_IDS,MAX_M,TP_D_BITS,TP_O_BITS,TP_L_BITS,TP_C_BITS),
TFill(MAX_N,MAX_M)
{
   unsigned int i;

   // Initialize the thread id.
   this->Tid = Tid;

   // Initialize simulator time:
   cycle = 0;

   // Initialize number of retired instructions.
   num_insn = 0;
   num_traces = 0;
   num_insn_split = 0;
   global_exception = false;

	//Stats
   total_tr_size = 0;
   tc_hits = 0;
   tc_misses = 0;
   tp_hits = 0;
   tp_misses = 0;
   total_live_ins = 0;
   total_live_outs = 0;
   avg_tr_size = 0;
   avg_tc_hits = 0;
   avg_tc_misses = 0;
   avg_tp_hits = 0;
   avg_tp_misses = 0;
   num_fgci_br = 0;
   num_br = 0;
   avg_fgci_br = 0;
   fgci_br = 0;
   total_pc = 0;
   total_br_code = 0;
   t_access = 0;
   t_miss = 0;
   t_hit = 0;	

   // Skip.
   if (SKIP_AMT)
      skip(SKIP_AMT);

   /////////////////////////////////////////////////////////////
   // Pipeline widths.
   /////////////////////////////////////////////////////////////
   this->fetch_width = fetch_width;
   this->dispatch_width = dispatch_width;
   this->issue_width = issue_width;
   this->retire_width = retire_width;

   /////////////////////////////////////////////////////////////
   // Fetch unit.
   /////////////////////////////////////////////////////////////
   committed_pc = THREAD[Tid]->get_arch_PC();
   	ld_text_base = THREAD[Tid]->get_text_base();
	ld_text_size = THREAD[Tid]->get_text_size();
   pc = THREAD[Tid]->get_arch_PC();
   next_fetch_cycle = (SS_TIME_TYPE)0;
   wait_for_trap = false;

   ////////////////////////////////////////////////////////////
   // Set up the register renaming modules.
   ////////////////////////////////////////////////////////////

   REN_INT = new renamer(TOTAL_INT_REGS, (TOTAL_INT_REGS + rob_size));
   REN_FP = new renamer(TOTAL_FP_REGS, (TOTAL_FP_REGS + rob_size));

   //Active List
   TR_AL = new AList();

	/////////////////////////////////////////////////////////////
	// 	PE
	/////////////////////////////////////////////////////////////
	PE_array = new PE_t *[TOTAL_PE];
	for(int i=0; i<TOTAL_PE; i++)
	{
		PE_array[i] = new PE_t(issue_width, fu_lane_matrix);
	}
   // Initialize the physical register files from func. sim.
   copy_reg();
   
   ///////////////////////////////////////////////////
   // Set up the memory system.
   ///////////////////////////////////////////////////
   // Memory system is declared above.
   // Initialize memory from functional simulator memory.
   LSU.copy_mem(THREAD[Tid]->get_mem_table());
}


#if 0
void processor::echo_config(FILE *fp) {
   fprintf(fp, "-----------\n");
   fprintf(fp, "SUPERSCALAR\n");
   fprintf(fp, "-----------\n");

   //fprintf(fp, "FETCH QUEUE SIZE: %d\n", FQ.get_size());
   //fprintf(fp, "ROB SIZE: %d\n", rob_size);
   //fprintf(fp, "IQ SIZE: %d\n", iq_size);
   //fprintf(fp, "LSQ SIZE: %d\n", mem_i.get_size());
   fprintf(fp, "FETCH_WIDTH: %d\n", fetch_width);
   fprintf(fp, "DISPATCH_WIDTH: %d\n", dispatch_width);
   fprintf(fp, "ISSUE_WIDTH: %d\n", issue_width);
   fprintf(fp, "RETIRE_WIDTH: %d\n", retire_width);

   fflush(fp);
}
#endif


void processor::copy_reg() {
   DOUBLE_WORD x;

   // Integer RF: general registers 0-31.
   for (unsigned int i = 0; i < TOTAL_GP_INT_REGS; i++)
      REN_INT->write(REN_INT->rename_rsrc(i), (unsigned long long)THREAD[Tid]->get_arch_reg_value(i));

   // Integer RF: HI, LO, FCC.
   REN_INT->write(REN_INT->rename_rsrc(HI_ID_NEW), (unsigned long long)THREAD[Tid]->get_arch_reg_value(HI_ID));
   REN_INT->write(REN_INT->rename_rsrc(LO_ID_NEW), (unsigned long long)THREAD[Tid]->get_arch_reg_value(LO_ID));
   REN_INT->write(REN_INT->rename_rsrc(FCC_ID_NEW), (unsigned long long)THREAD[Tid]->get_arch_reg_value(FCC_ID));

   // Floating-point RF.
   for (unsigned int i = 0; i < TOTAL_FP_REGS; i+=2) {
      x.w[0] = THREAD[Tid]->get_arch_reg_value(i + FPR_BASE);
      x.w[1] = THREAD[Tid]->get_arch_reg_value((i+1) + FPR_BASE);

      // single precision value or double precision value for F0
      REN_FP->write(REN_FP->rename_rsrc(i), x.dw);

      // single precision value for F1
      REN_FP->write(REN_FP->rename_rsrc(i+1), (unsigned long long)x.w[1]);
   }
}


void processor::next_cycle() {
   // Update the simulator cycle.
   cycle += 1;

#if 0
   if (cycle > BREAKPOINT_TIME) {
      printf("cycle = %.0f\n", (double)cycle);
      printf("Enter next cycle to stop at: ");
      scanf("%d", &BREAKPOINT_TIME);
      printf("next time: %d\n\n", BREAKPOINT_TIME);
   }
#endif
}


void processor::stats(FILE *fp) {
   BP.dump_stats(fp);
   LSU.stats(fp);
}


void processor::skip(unsigned int skip_amt) {
   debug_index i;
   db_t * db_ptr;
   unsigned int j;
   unsigned int pc;

   THREAD[Tid]->run_ahead();
   pc = THREAD[Tid]->get_arch_PC();

   j = 0;
   while (j < skip_amt) {
      i = THREAD[Tid]->first(pc);
      db_ptr = THREAD[Tid]->pop(i);
      THREAD[Tid]->pop_pc(); // Keep perf. branch pred. in sync
      pc = db_ptr->a_next_pc;
      j++;
      if (db_ptr->a_flags & F_TRAP) {
         THREAD[Tid]->trap_now(db_ptr->a_inst);
         THREAD[Tid]->trap_resume();
      }
   }

   while (THREAD[Tid]->cleanup_get_instr())
      THREAD[Tid]->pop_pc(); // Keep perf. branch pred. in sync

   fprintf(stderr, "Starting with Timing Simulator.\n");
}


//FGCI code
void processor::update_bit()
{
	unsigned int pc = this->pc; //ld_text_base;
	bool is_branch, is_break;
	SS_INST_TYPE inst, inst_in;
	unsigned int direct_target;
	unsigned int reconv_pt;
	unsigned int tot_insn;   // Including everything BR a b c d CON
	info_bit 	entry;
	
	is_branch = false;
	is_break = false;
	tot_insn = 0;

	for(int i=0; i<ld_text_size; i++)
	{
		//Fetch from that pc
		THREAD[Tid]->fetch(pc, inst);
		
		//Check if it is a branch
		switch(SS_OPCODE(inst))
		{
			case BEQ : case BNE : case BLEZ: case BGTZ:
			case BLTZ: case BGEZ: case BC1F: case BC1T:
				is_branch = true;
				tot_insn++;   //Incr for BR
				num_br++;   //Stat for branch
				direct_target = (pc + 8 + (OFS << 2));
				reconv_pt = direct_target;
				if(direct_target > pc)    //Forward branch
				{
					for(unsigned int pc_in=pc+8; pc_in<direct_target; pc_in=pc_in+8)
					{
						tot_insn++;
						THREAD[Tid]->fetch(pc_in, inst_in);
						switch(SS_OPCODE(inst_in))
						{
							case BEQ : case BNE : case BLEZ: case BGTZ:
							case BLTZ: case BGEZ: case BC1F: case BC1T:
							case JR: case JALR: case JUMP: case JAL:
							case SYSCALL:	case BREAK:
								is_break = true;
								break;
								
							default:
								break;
						}
						if(is_break) // I need to break from this loop as it is not simple hammock
							break;
					}
				}
				tot_insn++;   //Incr for CON
				if(is_break == false)  //It is a simple hammock
				{
					//Put the entry in BIT
					//BIT is hash table. Put following entries in it
					//1. pc
					//2. reconv_pt
					//3. number of entries between & including BR and reconvergent point (tot_insn)
					entry.reconv_pt = reconv_pt;
					entry.tot_insn = tot_insn;
					entry.pc = pc;
					BIT[pc] = entry;
					num_fgci_br++;
					
				}
				break;
				
			default:
				break;
		}
		is_branch = false;
		is_break = false;
		tot_insn = 0;
		//Update pc
		pc = pc + 8;
		total_pc++;
	}
	
}

