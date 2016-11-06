#include "processor.h"


void processor::squash_complete(unsigned int offending_PC) {
  /* unsigned int i;

   //////////////////////////
   // Fetch Stage
   //////////////////////////

   pc = offending_PC + SS_INST_SIZE;
   next_fetch_cycle = (SS_TIME_TYPE)0;
   wait_for_trap = false;
   BP.flush();

   //////////////////////////
   // Decode Stage
   //////////////////////////

   for (i = 0; i < fetch_width; i++)
      DECODE[i].valid = false;

   //////////////////////////
   // Rename1 Stage
   //////////////////////////

   FQ.flush();

   //////////////////////////
   // Rename2 Stage
   //////////////////////////

   for (i = 0; i < dispatch_width; i++)
      RENAME2[i].valid = false;

   //////////////////////////
   // Dispatch Stage
   //////////////////////////

   for (i = 0; i < dispatch_width; i++)
      DISPATCH[i].valid = false;

   //////////////////////////
   // Schedule Stage
   //////////////////////////

   IQ_INT.flush();
   IQ_FP.flush();

   //////////////////////////
   // Register Read Stage
   // Execute Stage
   // Writeback Stage
   //////////////////////////

   for (i = 0; i < issue_width; i++) {
      Execution_Lanes[i].rr.valid = false;
      Execution_Lanes[i].ex.valid = false;
      Execution_Lanes[i].wb.valid = false;
   }

   LSU.flush();
   */
}


void processor::resolve(unsigned int branch_ID, bool correct) {
 /*  unsigned int i;

   if (correct) {
      // Instructions in the Rename2 through Writeback Stages have branch masks.
      // The correctly-resolved branch's bit must be cleared in all branch masks.

      for (i = 0; i < dispatch_width; i++) {
         // Rename2 Stage:
         CLEAR_BIT(RENAME2[i].branch_mask, branch_ID);

	 // Dispatch Stage:
         CLEAR_BIT(DISPATCH[i].branch_mask, branch_ID);
      }

      // Schedule Stage:
      IQ_INT.clear_branch_bit(branch_ID);
      IQ_FP.clear_branch_bit(branch_ID);

      for (i = 0; i < issue_width; i++) {
	 // Register Read Stage:
	 CLEAR_BIT(Execution_Lanes[i].rr.branch_mask, branch_ID);

	 // Execute Stage:
	 CLEAR_BIT(Execution_Lanes[i].ex.branch_mask, branch_ID);

	 // Writeback Stage:
	 CLEAR_BIT(Execution_Lanes[i].wb.branch_mask, branch_ID);
      }
   }
   else {
      // Squash all instructions in the Decode through Dispatch Stages.

      // Decode Stage:
      for (i = 0; i < fetch_width; i++)
	 DECODE[i].valid = false;

      // Rename1 Stage:
      FQ.flush();

      // Rename2 Stage:
      for (i = 0; i < dispatch_width; i++)
         RENAME2[i].valid = false;

      // Dispatch Stage:
      for (i = 0; i < dispatch_width; i++)
         DISPATCH[i].valid = false;

      // Selectively squash instructions after the branch, in the Schedule through Writeback Stages.

      // Schedule Stage:
      IQ_INT.squash(branch_ID);
      IQ_FP.squash(branch_ID);

      for (i = 0; i < issue_width; i++) {
	 // Register Read Stage:
	 if (Execution_Lanes[i].rr.valid && BIT_IS_ONE(Execution_Lanes[i].rr.branch_mask, branch_ID))
	    Execution_Lanes[i].rr.valid = false;

	 // Execute Stage:
	 if (Execution_Lanes[i].ex.valid && BIT_IS_ONE(Execution_Lanes[i].ex.branch_mask, branch_ID))
	    Execution_Lanes[i].ex.valid = false;

	 // Writeback Stage:
	 if (Execution_Lanes[i].wb.valid && BIT_IS_ONE(Execution_Lanes[i].wb.branch_mask, branch_ID))
	    Execution_Lanes[i].wb.valid = false;
      }
   }
   */
}
