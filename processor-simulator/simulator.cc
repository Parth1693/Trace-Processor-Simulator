#include "processor.h"
#include "globals.h"
#include <time.h>

// Change function names to be more meaningful:
//	     NEW	       OLD
#define simulator_init	trace_consume_init
#define simulator	trace_consume


void simulator_init() {
  ///////////////////////////////////////////
  // Create the processor(s).
  ///////////////////////////////////////////
  PROC = new processor *[NumThreads];
  for (unsigned int i = 0; i < NumThreads; i++) {
     PROC[i] = new processor(FETCH_QUEUE_SIZE,
			     NUM_CHECKPOINTS,
			     ACTIVE_LIST_SIZE,
			     ISSUE_QUEUE_SIZE,
			     LQ_SIZE,
			     SQ_SIZE,
			     FETCH_WIDTH,
			     DISPATCH_WIDTH,
			     ISSUE_WIDTH,
			     RETIRE_WIDTH,
			     FU_LANE_MATRIX,
			     i);
  }

  ///////////////////////////////////////////
  // Fill debug buffers for all threads.
  ///////////////////////////////////////////
  for (unsigned int i = 0; i < NumThreads; i++)
     THREAD[i]->run_ahead();
}	// simulator_init()


void simulator() {
   unsigned int i;
   unsigned int lane_number;
   unsigned int pe_number; unsigned int iter;
   unsigned int start;

   unsigned int plateaus[4] = {1000, 10000, 100000, MAXINT};
   unsigned int grades[4] = {60, 70, 80, 90};
   unsigned int next_plateau = 0;

#ifdef DEBUG
	unsigned int start;
	clock_t begin,end;
	double time_spent;
	clock_t begin1,end1,begin2,end2,begin3,end3,begin4,end4,begin5,end5,begin6,end6,begin7,end7,begin8,end8,begin9,end9;
	double time_spent1 = 0, time_spent2 = 0,time_spent3 = 0, time_spent4 = 0, time_spent5 = 0, time_spent6 = 0, time_spent7 = 0, time_spent8 = 0, time_spent9 = 0;
#endif


	i=0;
	PROC[i]->update_bit();



#if 1
   ////////////////////////////////
   // Main simulator loop.
   ////////////////////////////////
   while (1) {
      for (i = 0; i < NumThreads; i++) {
	 /////////////////////////////////////////////////////////////
	 // Pipeline.
	 /////////////////////////////////////////////////////////////

	 // NOTE:  We need to run loop from oldest active trace to newest.

		
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PROC[i]->trace_retire();	 // Retire Stage

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	start = PROC[i]->TR_AL->al_head;
	pe_number = start;
	for(iter = 0; iter<TOTAL_PE; iter++) 
		{
			for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++)
				PROC[i]->trace_writeback(pe_number, lane_number);				// Writeback Stage
			pe_number = MOD((pe_number+1),TOTAL_PE);
		}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	start = PROC[i]->TR_AL->al_head;	
	pe_number = start;
	for(iter = 0; iter<TOTAL_PE; iter++) 
	   {
		   for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++)
			   PROC[i]->trace_execute(pe_number, lane_number);			   // Execute Stage
		   pe_number = MOD((pe_number+1),TOTAL_PE); 	   
	   }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	start = PROC[i]->TR_AL->al_head;	
	pe_number = start;
	for(iter = 0; iter<TOTAL_PE; iter++) 
	   {
		   for (lane_number = 0; lane_number < ISSUE_WIDTH; lane_number++)
			   PROC[i]->trace_register_read(pe_number, lane_number);		   // Register Read Stage
		   pe_number = MOD((pe_number+1),TOTAL_PE); 	   
	   }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	start = PROC[i]->TR_AL->al_head;	
	pe_number = start;
	for(iter = 0; iter<TOTAL_PE; iter++) 
		{
			PROC[i]->trace_schedule(pe_number); 		// Schedule Stage
			pe_number = MOD((pe_number+1),TOTAL_PE);		
		}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PROC[i]->trace_dispatch();						// Dispatch Stage

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PROC[i]->trace_rename();						// Rename Stage

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

	PROC[i]->trace_fetch();						// Fetch Stage

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	PROC[i]->trace_predict();						// Predict Stage


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#ifdef DEBUG
	if (PROC[i]->cycle % 5000000 == 0 )
	{
		std::cout << "------------------ Cycle " << PROC[0]->cycle << "---------------" << std::endl;
		std::cout << " num_insn = " << PROC[0]->num_insn << "  num_traces = "<< PROC[0]->num_traces << std::endl;
			 //Print stats to file
	//if((PROC[i]->TC.tc_hits + PROC[i]->TC.tc_misses) != 0)
	if((PROC[i]->t_access) != 0)
	 {PROC[i]->avg_tc_hits =  (double)PROC[i]->t_hit/(double)(PROC[i]->t_access);
	 PROC[i]->avg_tc_misses =  (double)PROC[i]->t_miss/(double)(PROC[i]->t_access);}
	if(PROC[i]->num_traces != 0)
	 PROC[i]->avg_tr_size = (double)PROC[i]->total_tr_size/(double)PROC[i]->num_traces;
	if ((PROC[i]->tp_hits + PROC[i]->tp_misses) != 0)
	 {PROC[i]->avg_tp_hits = (double)PROC[i]->tp_hits/(double)(PROC[i]->tp_hits + PROC[i]->tp_misses);
	 PROC[i]->avg_tp_misses = (double)PROC[i]->tp_misses/(double)(PROC[i]->tp_hits + PROC[i]->tp_misses);}

	 	 	if(PROC[i]->num_br != 0)
		PROC[i]->avg_fgci_br = (double)PROC[i]->num_fgci_br/(double)PROC[i]->num_br;
	 
	 printf("Statistics:\n");
	 printf("Trace Cache Parameters:\n");
	 printf("Total Trace Cache Hits : %d\n",PROC[i]->TC.tc_hits);
	 printf("Total Trace Cache Misses : %d\n",PROC[i]->TC.tc_misses);
	 printf("Average Trace Cache Hits : %.6f\n",PROC[i]->avg_tc_hits);
	 printf("Average Trace Cache Misses : %.6f\n",PROC[i]->avg_tc_misses);
	 printf("Trace Predictor Parameters:\n");
	 printf("Average Trace Predictor Hits : %.2f\n",PROC[i]->avg_tp_hits);
	 printf("Average Trace Predictor Misses : %.2f\n",PROC[i]->avg_tp_misses);
	 printf("General Parameters:\n");
	 printf("Average Trace Size : %.2f\n",PROC[i]->avg_tr_size);
	 printf("Total Live-ins : %d\n",PROC[i]->total_live_ins);
	 printf("Total Live-outs : %d\n",PROC[i]->total_live_outs);

	printf("num fgci and cgci branches : %d\n",PROC[i]->num_fgci_br);
	printf("num fgci branches : %d\n",PROC[i]->fgci_br);
	printf("num branches : %d\n",PROC[i]->num_br);
	printf("total pc : %d\n",PROC[i]->total_pc);
	printf("Average fgci cgci branches : %.2f\n",PROC[i]->avg_fgci_br);
	if((PROC[i]->num_fgci_br) != (unsigned int)0)
		printf("Average fgci branches among total branches in code : %.2f\n",(double)PROC[i]->fgci_br/(double)(PROC[i]->total_br_code));
	}
//#endif
	 /////////////////////////////////////////////////////////////
	 // Miscellaneous stuff that must be processed every cycle.
	 /////////////////////////////////////////////////////////////

	 // Go to the next simulator cycle.
	 PROC[i]->next_cycle();

	 // For detecting deadlock, and monitoring progress.
	 if (MOD(PROC[i]->cycle, 0x400000) == 0) {
	    INFO("Thread %d: (cycle = %x) num_insn = %.0f\tIPC = %.2f",
		 i,
		 (unsigned int)PROC[i]->cycle,
		 (double)PROC[i]->num_insn,
		 (double)PROC[i]->num_insn/(double)PROC[i]->cycle);
	    fflush(fp_info);
	 }

         // Print grading plateaus.
	 if (PROC[i]->num_insn >= plateaus[next_plateau]) {
	    INFO("GRADING_PLATEAU_%d (%d%%)", plateaus[next_plateau], grades[next_plateau]);
	    next_plateau++;
	 }

      }
   }
#endif
   
	 
}	// simulator()
