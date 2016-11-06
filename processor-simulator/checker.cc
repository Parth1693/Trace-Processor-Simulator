#include "processor.h"


void processor::check_single(SS_WORD_TYPE proc, SS_WORD_TYPE func, char *desc) {
   if (proc != func) {
      printf("Instruction %.0f, Cycle %.0f: %s Pipeline:%x vs. FuncSim:%x.", (double)num_insn, (double)cycle, desc, proc, func);
      assert(0);
   }
}

void processor::check_double(SS_WORD_TYPE proc0, SS_WORD_TYPE proc1, SS_WORD_TYPE func0, SS_WORD_TYPE func1, char *desc) {
   if ((proc0 != func0) || (proc1 != func1)) {
      printf("Instruction %.0f, Cycle %.0f: %s Pipeline:%x,%x vs. FuncSim:%x,%x.", (double)num_insn, (double)cycle, desc, proc0, proc1, func0, func1);
      assert(0);
   }
}

void processor::checker() {
	 unsigned int head;	// Index of the head instruction in PAY.
	 db_t *actual;		// Pointer to corresponding instruction in the functional simulator.

	 // Get the index of the head instruction in PAY.
	 head = PAY.head;

      // Get pointer to the corresponding instruction in the functional simulator.
	 // This enables checking results of the pipeline simulator.
	 //assert(PAY.buf[head].good_instruction && (PAY.buf[head].db_index != DEBUG_INDEX_INVALID));
	 assert(PAY.buf[head].good_instruction);
	 assert((PAY.buf[head].db_index != DEBUG_INDEX_INVALID));
	 if (PAY.buf[head].split && PAY.buf[head].upper)
	    actual = THREAD[Tid]->peek(PAY.buf[head].db_index);
	 else
	    actual = THREAD[Tid]->pop(PAY.buf[head].db_index);

	 // Validate the instruction PC.
	 check_single((SS_WORD_TYPE)PAY.buf[head].pc, (SS_WORD_TYPE)actual->a_pc, "PC mismatch.");

	 if (PAY.buf[head].split) {
	    switch (SS_OPCODE(PAY.buf[head].inst)) {
	       case DLW:
	          // Validate address.
	          check_single((SS_WORD_TYPE)PAY.buf[head].addr, (SS_WORD_TYPE)(actual->a_addr + (PAY.buf[head].upper ? 0 : 4)), "DLW address mismatch.");
	          // Validate source register.
	          check_single(PAY.buf[head].A_value.w[0], actual->a_rsrcA[0].value, "DLW source mismatch.");
	          // Validate destination register.
	          check_single(PAY.buf[head].C_value.w[0], (PAY.buf[head].upper ? actual->a_rdst[0].value : actual->a_rdst[1].value), "DLW destination mismatch.");
	          break;
	          
	       case DSW:
	          // Validate address.
	          check_single((SS_WORD_TYPE)PAY.buf[head].addr, (SS_WORD_TYPE)(actual->a_addr + (PAY.buf[head].upper ? 0 : 4)), "DSW address mismatch.");
	          // Validate first source register.
	          check_single(PAY.buf[head].A_value.w[0], actual->a_rsrcA[0].value, "DSW first source mismatch.");
	          // Validate second source register.
	          check_single(PAY.buf[head].B_value.w[0], (PAY.buf[head].upper ? actual->a_rsrc[0].value : actual->a_rsrc[1].value), "DSW second source mismatch.");
	          break;
	          
	       case S_S:
	          if (PAY.buf[head].upper) { // address op
	             // Validate address.
	             check_single((SS_WORD_TYPE)PAY.buf[head].addr, (SS_WORD_TYPE)actual->a_addr, "S_S address mismatch.");
	             // Validate first source register.
	             check_single(PAY.buf[head].A_value.w[0], actual->a_rsrcA[0].value, "S_S first source mismatch.");
	          }
	          else {  // value op
	             // Validate second source register.
	             check_single(PAY.buf[head].A_value.w[0], actual->a_rsrc[0].value, "S_S second source mismatch.");
	          }
	          break;

	       case S_D:
	          if (PAY.buf[head].upper) { // address op
	             // Validate address.
	             check_single((SS_WORD_TYPE)PAY.buf[head].addr, (SS_WORD_TYPE)actual->a_addr, "S_D address mismatch.");
	             // Validate first source register.
	             check_single(PAY.buf[head].A_value.w[0], actual->a_rsrcA[0].value, "S_D first source mismatch.");
	          }
	          else {  // value op
	             // Validate second source register.
		     check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "S_D second source mismatch.");
	          }
	          break;

	       case MULT: case MULTU: case DIV: case DIVU:
	          // Validate first source register.
	          check_single(PAY.buf[head].A_value.w[0], actual->a_rsrc[0].value, "MULT/DIV first source mismatch.");
	          // Validate second source register.
	          check_single(PAY.buf[head].B_value.w[0], actual->a_rsrc[1].value, "MULT/DIV second source mismatch.");
	          // Validate destination register.
	          check_single(PAY.buf[head].C_value.w[0], (PAY.buf[head].upper ? actual->a_rdst[0].value : actual->a_rdst[1].value), "MULT/DIV destination mismatch.");
	          break;

	       case DMFC1:
	          // Validate source register.
	          check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "DMFC1 source mismatch.");
	          // Validate destination register.
	          check_single(PAY.buf[head].C_value.w[0], (PAY.buf[head].upper ? actual->a_rdst[0].value : actual->a_rdst[1].value), "DMFC1 destination mismatch.");
	          break;

	       default:
	          assert(0);
	          break;
	    }
	 }
	 else if (IS_MEM_OP(PAY.buf[head].flags)) {
	    // Validate address.
	    check_single((SS_WORD_TYPE)PAY.buf[head].addr, (SS_WORD_TYPE)actual->a_addr, "Load/store address mismatch.");

	    // Validate first source register.
	    check_single(PAY.buf[head].A_value.w[0], actual->a_rsrcA[0].value, "Load/store first source mismatch.");

	    if ((IS_STORE(PAY.buf[head].flags) && (SS_OPCODE(PAY.buf[head].inst) != DSZ)) ||
		(IS_LOAD(PAY.buf[head].flags) && (PAY.buf[head].left || PAY.buf[head].right))) {
	       // Validate second source register.
	       check_single(PAY.buf[head].B_value.w[0], actual->a_rsrc[0].value, "Store/LWL/LWR second source mismatch.");
	    }

	    if (IS_LOAD(PAY.buf[head].flags)) {
	       // Validate destination register.
	       if (SS_OPCODE(PAY.buf[head].inst) == L_D)
	          check_double(PAY.buf[head].C_value.w[0], PAY.buf[head].C_value.w[1], actual->a_rdst[0].value, actual->a_rdst[1].value, "L_D destination mismatch.");
	       else
	          check_single(PAY.buf[head].C_value.w[0], actual->a_rdst[0].value, "Load destination mismatch.");
	    }
	 }
	 else {
	    assert(actual->a_num_rsrcA == 0);	// already checked all loads/stores above (whether split or regular)

	    switch (SS_OPCODE(PAY.buf[head].inst)) {
	       case DMTC1:
	          check_single(PAY.buf[head].A_value.w[0], actual->a_rsrc[0].value, "First source mismatch.");
	          check_single(PAY.buf[head].B_value.w[0], actual->a_rsrc[1].value, "Second source mismatch.");
	          check_double(PAY.buf[head].C_value.w[0], PAY.buf[head].C_value.w[1], actual->a_rdst[0].value, actual->a_rdst[1].value, "Destination mismatch.");
	          break;

	       case FADD_D: case FSUB_D: case FMUL_D: case FDIV_D:
	          check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "First source mismatch.");
	          check_double(PAY.buf[head].B_value.w[0], PAY.buf[head].B_value.w[1], actual->a_rsrc[2].value, actual->a_rsrc[3].value, "Second source mismatch.");
	          check_double(PAY.buf[head].C_value.w[0], PAY.buf[head].C_value.w[1], actual->a_rdst[0].value, actual->a_rdst[1].value, "Destination mismatch.");
	          break;

	       case FABS_D: case FNEG_D: case FMOV_D: case FSQRT_D:
	          check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "Source mismatch.");
	          check_double(PAY.buf[head].C_value.w[0], PAY.buf[head].C_value.w[1], actual->a_rdst[0].value, actual->a_rdst[1].value, "Destination mismatch.");
	          break;

	       case CVT_S_D: case CVT_W_D:
	          check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "Source mismatch.");
	          check_single(PAY.buf[head].C_value.w[0], actual->a_rdst[0].value, "Destination mismatch.");
	          break;

	       case CVT_D_S: case CVT_D_W:
	          check_single(PAY.buf[head].A_value.w[0], actual->a_rsrc[0].value, "Source mismatch.");
	          check_double(PAY.buf[head].C_value.w[0], PAY.buf[head].C_value.w[1], actual->a_rdst[0].value, actual->a_rdst[1].value, "Destination mismatch.");
	          break;

	       case C_EQ_D: case C_LT_D: case C_LE_D:
	          check_double(PAY.buf[head].A_value.w[0], PAY.buf[head].A_value.w[1], actual->a_rsrc[0].value, actual->a_rsrc[1].value, "First source mismatch.");
	          check_double(PAY.buf[head].B_value.w[0], PAY.buf[head].B_value.w[1], actual->a_rsrc[2].value, actual->a_rsrc[3].value, "Second source mismatch.");
	          check_single(PAY.buf[head].C_value.w[0], actual->a_rdst[0].value, "Destination mismatch.");
	          break;

	       default:
	          // Remaining instructions handled here:
	          // All source and destination operands are either integer or single-precision float.
	          assert(actual->a_num_rsrc <= 2);

	          assert(actual->a_num_rdst <= 1);

	          if (actual->a_num_rsrc > 0) {
	             // Validate first source register.
	             check_single(PAY.buf[head].A_value.w[0], actual->a_rsrc[0].value, "First source mismatch.");
	          }
	          if (actual->a_num_rsrc > 1) {
	             // Validate second source register.
	             check_single(PAY.buf[head].B_value.w[0], actual->a_rsrc[1].value, "Second source mismatch.");
	          }
	          if (actual->a_num_rdst > 0) {
	             // Validate destination register.
	             check_single(PAY.buf[head].C_value.w[0], actual->a_rdst[0].value, "Destination mismatch.");
	          }
	          break;
	    }
	 }
}

