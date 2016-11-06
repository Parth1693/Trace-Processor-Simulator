#include "processor.h"
#include "globals.h"
#include <time.h>

/* set to non-zero when simulator should dump statistics */
int sim_dump_stats = FALSE;

void
sim_stats(FILE *stream)
{
  extern time_t start_time;
  time_t elapsed = MAX(time((time_t *)NULL) - start_time, 1);


  for (unsigned int i = 0; i < NumThreads; i++) {
     INFO("sim: simulation time: %s (%f insts/sec)",
          elapsed_time(elapsed), (double)PROC[i]->num_insn/(double)elapsed);
     INFO("sim: number of instructions: %.0f", (double)PROC[i]->num_insn);
     INFO("sim: number of cycles: %.0f", (double)PROC[i]->cycle);
     INFO("sim: IPC: %.2f", (double)PROC[i]->num_insn/(double)PROC[i]->cycle);
     INFO("sim: number of split instructions: %.0f", (double)PROC[i]->num_insn_split);
     INFO("sim: eff-IPC: %.2f", (double)(PROC[i]->num_insn - PROC[i]->num_insn_split)/(double)PROC[i]->cycle);

	 if((PROC[i]->t_access) != 0)
	  {PROC[i]->avg_tc_hits =  (double)PROC[i]->t_hit/(double)(PROC[i]->t_access);
	  PROC[i]->avg_tc_misses =	(double)PROC[i]->t_miss/(double)(PROC[i]->t_access);}
	 if(PROC[i]->num_traces != 0)
	  PROC[i]->avg_tr_size = (double)PROC[i]->total_tr_size/(double)PROC[i]->num_traces;
	 if ((PROC[i]->tp_hits + PROC[i]->tp_misses) != 0)
	  {PROC[i]->avg_tp_hits = (double)PROC[i]->tp_hits/(double)(PROC[i]->tp_hits + PROC[i]->tp_misses);
	  PROC[i]->avg_tp_misses = (double)PROC[i]->tp_misses/(double)(PROC[i]->tp_hits + PROC[i]->tp_misses);}
	  
		  if(PROC[i]->num_br != 0)
	  PROC[i]->avg_fgci_br = (double)PROC[i]->num_fgci_br/(double)PROC[i]->num_br;

	 INFO("\nStatistics:");
	 INFO("Number of PEs: %d", TOTAL_PE);	
	 INFO("\nTrace Cache Parameters:");
         INFO("Trace Cache Size: %d", TC_CACHESIZE);
	 INFO("Trace Cache Associativity: %d", TC_ASSOC);	
	 INFO("Total Trace Cache Hits : %d",PROC[i]->t_hit);
	 INFO("Total Trace Cache Misses : %d",PROC[i]->t_miss);
	 INFO("Trace Cache Hit Rate : %.2f",PROC[i]->avg_tc_hits);
	 INFO("Trace Cache Miss Rate : %.2f",PROC[i]->avg_tc_misses);
	 INFO("\nTrace Predictor Parameters:");
	 INFO("Trace Predictor Hits : %.2f",PROC[i]->avg_tp_hits);
	 INFO("Trace Predictor Mispredictions : %.2f",PROC[i]->avg_tp_misses);
	 INFO("\nGeneral Parameters:");
	 INFO("Average Trace Size : %.2f",PROC[i]->avg_tr_size);
	 INFO("Average Live-ins per trace : %.2f",(double)PROC[i]->total_live_ins/(double)PROC[i]->num_traces);
	 INFO("Average Live-outs per trace : %.2f",(double)PROC[i]->total_live_outs/(double)PROC[i]->num_traces);

	 INFO("\nFGCI Statistics:");
	INFO("Number of FGCI and CGCI branches : %d",PROC[i]->num_fgci_br);
	INFO("Number of FGCI branches : %d\n",PROC[i]->fgci_br);
	INFO("Total number of branches : %d\n",PROC[i]->num_br);
	INFO("Total static instruction count : %d\n",PROC[i]->total_pc);
	INFO("Percentage FGCI and CGCI branches : %.2f\n",PROC[i]->avg_fgci_br);
	if((PROC[i]->num_fgci_br) != 0)
		INFO("Percentage of FGCI branches among total branches in code : %.2f\n",(double)PROC[i]->fgci_br/(double)(PROC[i]->total_br_code));
     PROC[i]->stats(fp_info);
  } //end of for

}


