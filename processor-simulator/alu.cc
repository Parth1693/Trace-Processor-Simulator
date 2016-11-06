#include <math.h>
#include "processor.h"


// Helper functions at the bottom of this file.
static void ExecMULT(SS_WORD_TYPE, SS_WORD_TYPE,
		     SS_WORD_TYPE *, SS_WORD_TYPE *);
static void ExecMULTU(SS_WORD_TYPE, SS_WORD_TYPE,
		      SS_WORD_TYPE *, SS_WORD_TYPE *);
static void ExecSRA(SS_WORD_TYPE, unsigned int, SS_WORD_TYPE *);
static void ExecSRAV(SS_WORD_TYPE, SS_WORD_TYPE, SS_WORD_TYPE *);

// +Parth
// We now need to pass info about the trace_index as well as inst_index
void processor::agen(unsigned int index)
{
   SS_INST_TYPE inst;
   SS_WORD_TYPE addr;

   // Only loads and stores should do AGEN.
   assert(IS_MEM_OP(PAY.buf[index].flags));

   // Need "inst" for SS macros to work.
   inst.a = PAY.buf[index].inst.a;
   inst.b = PAY.buf[index].inst.b;

   // Execute the AGEN.
   addr = PAY.buf[index].A_value.w[0] + (OFS);
   PAY.buf[index].addr = (unsigned int) addr;

   // Adjust address of the lower half of DLW and DSW.
   if ((SS_OPCODE(inst) == DLW) || (SS_OPCODE(inst) == DSW)) {
      assert(PAY.buf[index].split);
      if (!PAY.buf[index].upper)
         PAY.buf[index].addr += 4;
   }
}	// agen()


void processor::alu(unsigned int index)
{
   SS_INST_TYPE inst;
   SS_WORD_TYPE hi, lo;

   unsigned int x,y;
   DOUBLE_WORD a, b, r;

   // Need "inst" for SS macros to work.
   inst.a = PAY.buf[index].inst.a;
   inst.b = PAY.buf[index].inst.b;

   switch (SS_OPCODE(inst)) {

//
// LOAD instructions
//
      case LB: case LBU: case LH: case LHU:
      case LW: case DLW: case L_S: case L_D:
      case LB_RR: case LBU_RR: case LH_RR: case LHU_RR:
      case LW_RR: case DLW_RR: case L_S_RR: case L_D_RR:
      case LWL: case LWR:
	 // Loads are executed by memory interface.
	 assert(0);
	 break;

//
// STORE instructions
//
      case SB: case SH: case SW: case DSW: case S_S: case S_D:
      case SB_RR: case SH_RR: case SW_RR: case DSW_RR: case S_S_RR: case S_D_RR:
      case SWL: case SWR:
      case DSZ: case DSZ_RR:
	 // Stores are executed by memory interface.
	 assert(0);
	 break;

//
// explicit NOP
//
      case NOP:
	 break;

//
// control transfer operations
//
      case JUMP:
	 //PAY.buf[index].c_next_pc = ((PAY.buf[index].pc & 036000000000) | (TARG << 2));
	 // This skips the IQ and execution lanes (completed in Dispatch Stage).
	 assert(0);
	 break;

      case JAL:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].pc + 8 ;
	 PAY.buf[index].c_next_pc = ((PAY.buf[index].pc & 036000000000) | (TARG << 2));
	 break;

      case JR:
	 PAY.buf[index].c_next_pc = PAY.buf[index].A_value.w[0];
	 break;

      case JALR:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].pc + 8 ;
	 PAY.buf[index].c_next_pc = PAY.buf[index].A_value.w[0];
	 break;

      case BEQ:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] == PAY.buf[index].B_value.w[0]) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BNE:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] != PAY.buf[index].B_value.w[0]) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BLEZ:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] <= 0) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BGTZ:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] > 0) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BLTZ:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] < 0) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BGEZ:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0] >= 0) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BC1F:
	 PAY.buf[index].c_next_pc =
	 (!PAY.buf[index].A_value.w[0]) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

      case BC1T:
	 PAY.buf[index].c_next_pc =
	 (PAY.buf[index].A_value.w[0]) ?
	 (PAY.buf[index].pc + 8 + (OFS << 2)) : (PAY.buf[index].pc + 8);
	 break;

//
// Integer ALU operations
//

      case ADD: case ADDU:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] + PAY.buf[index].B_value.w[0] ;
	 break;


      case ADDI: case ADDIU:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] + (IMM) ;
	 break;

      case SUB: case SUBU:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] - PAY.buf[index].B_value.w[0] ;
	 break;

      case MULT:
	 ExecMULT(PAY.buf[index].A_value.w[0], PAY.buf[index].B_value.w[0], &hi, &lo);
	 assert(PAY.buf[index].split);
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].upper ? hi : lo);
	 break;

      case MULTU:
	 ExecMULTU(PAY.buf[index].A_value.w[0], PAY.buf[index].B_value.w[0], &hi, &lo);
	 assert(PAY.buf[index].split);
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].upper ? hi : lo);
	 break;

      case DIV:
	 assert(PAY.buf[index].split);
	 if (PAY.buf[index].B_value.w[0] != 0) {	// check for div by 0
	    PAY.buf[index].C_value.w[0] = (PAY.buf[index].upper ?
	    				   (PAY.buf[index].A_value.w[0] % PAY.buf[index].B_value.w[0]) :	// set the HI value: RS % RT
					   (PAY.buf[index].A_value.w[0] / PAY.buf[index].B_value.w[0]));	// set the LO value: RS / RT
	 }
	 else {						// div by 0: output bad values
	    PAY.buf[index].C_value.w[0] = 0xDEADBEEF;
	 }
	 break;

      case DIVU:
	 assert(PAY.buf[index].split);
	 if (PAY.buf[index].B_value.w[0] != 0) {	// check for div by 0
	    PAY.buf[index].C_value.w[0] = (PAY.buf[index].upper ?
					   ((unsigned int)PAY.buf[index].A_value.w[0] % (unsigned int)PAY.buf[index].B_value.w[0]) :	// set the HI value: RS % RT
					   ((unsigned int)PAY.buf[index].A_value.w[0] / (unsigned int)PAY.buf[index].B_value.w[0]));	// set the LO value: RS / RT
	 }
	 else {						// div by 0: output bad values
	    PAY.buf[index].C_value.w[0] = 0xDEADBEEF;
	 }
	 break;

      case MFHI: case MTHI: case MFLO: case MTLO:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0];
	 break;

      case AND_:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] & PAY.buf[index].B_value.w[0];
	 break;

      case ANDI:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] & UIMM ;
	 break;

      case OR:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] | PAY.buf[index].B_value.w[0];
	 break;

      case ORI:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] | UIMM ;
	 break;

      case XOR:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] ^ PAY.buf[index].B_value.w[0];
	 break;

      case XORI:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] ^ UIMM ;
	 break;

      case NOR:
	 PAY.buf[index].C_value.w[0] = ~(PAY.buf[index].A_value.w[0] | PAY.buf[index].B_value.w[0]);
	 break;

      case SLL:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0] << SHAMT;
	 break;

      case SLLV:
	 // RD = RT << (RS & 037)
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].B_value.w[0] << (PAY.buf[index].A_value.w[0] & 037);
	 break;

      case SRL:
	 PAY.buf[index].C_value.w[0] = ((unsigned int)PAY.buf[index].A_value.w[0]) >> SHAMT;
	 break;

      case SRLV:
	 // RD = ((unsigned int)RT) >> (RS & 037)
	 PAY.buf[index].C_value.w[0] = ((unsigned int)PAY.buf[index].B_value.w[0]) >> (PAY.buf[index].A_value.w[0] & 037);
	 break;

      case SRA:
	 ExecSRA(PAY.buf[index].A_value.w[0], SHAMT, &(PAY.buf[index].C_value.w[0]));
	 break;

      case SRAV:
	 ExecSRAV(PAY.buf[index].B_value.w[0], PAY.buf[index].A_value.w[0], &(PAY.buf[index].C_value.w[0]));
	 break;

      case SLT:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.w[0] < PAY.buf[index].B_value.w[0]) ? 1 : 0 ;
	 break;

      case SLTI:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.w[0] < IMM) ? 1 : 0 ;
	 break;

      case SLTU:
	 PAY.buf[index].C_value.w[0] = ( ((unsigned int)PAY.buf[index].A_value.w[0]) < ((unsigned int)PAY.buf[index].B_value.w[0]) ) ? 1 : 0 ;
	 break;

      case SLTIU:
	 PAY.buf[index].C_value.w[0] = ( ((unsigned int)PAY.buf[index].A_value.w[0]) < ((unsigned int)IMM) ) ? 1 : 0 ;
	 break;

//
// miscellaneous
//

      case SYSCALL: case BREAK:
	 // These skip the IQ and execution lanes (completed in Dispatch Stage).
	 assert(0);
	 break;

      case LUI:
         PAY.buf[index].C_value.w[0] = (UIMM << 16);
	 break;

      case MFC1: case MTC1:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0];
	 PAY.buf[index].C_value.w[1] = (SS_WORD_TYPE)0;			// KLUGE: Fix sequence MTC1 r0->f0, MTC1 r0->f1, followed by reference to f0 as double-precision float. (twolf)
	 break;

      case DMFC1:
	 assert(PAY.buf[index].split);
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].upper ? PAY.buf[index].A_value.w[0] : PAY.buf[index].A_value.w[1]);
	 break;

      case DMTC1:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0];
	 PAY.buf[index].C_value.w[1] = PAY.buf[index].B_value.w[0];
	 break;

      case CFC1: case CTC1:
	 assert(0);		// not supported
	 break;

//
// Floating Point ALU operations
//

      case FADD_S:
	 PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] + PAY.buf[index].B_value.f[0];
	 break;

      case FADD_D:
	 PAY.buf[index].C_value.d = PAY.buf[index].A_value.d + PAY.buf[index].B_value.d;
	 break;

      case FSUB_S:
	 PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] - PAY.buf[index].B_value.f[0];
	 break;

      case FSUB_D:
	 PAY.buf[index].C_value.d = PAY.buf[index].A_value.d - PAY.buf[index].B_value.d;
	 break;

      case FMUL_S:
	 PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] * PAY.buf[index].B_value.f[0];
	 break;

      case FMUL_D:
	 PAY.buf[index].C_value.d = PAY.buf[index].A_value.d * PAY.buf[index].B_value.d;
	 break;

      case FDIV_S:
	 PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] / PAY.buf[index].B_value.f[0];
	 break;

      case FDIV_D:
	 PAY.buf[index].C_value.d = PAY.buf[index].A_value.d / PAY.buf[index].B_value.d;
	 break;

      case FABS_S:
	 PAY.buf[index].C_value.f[0] = (float) fabs((double)PAY.buf[index].A_value.f[0]);
	 break;

      case FABS_D:
	 PAY.buf[index].C_value.d = fabs(PAY.buf[index].A_value.d);
	 break;

      case FMOV_S:
	 PAY.buf[index].C_value.w[0] = PAY.buf[index].A_value.w[0];
	 break;

      case FMOV_D:
	 PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw;
	 break;

      case FNEG_S:
	 PAY.buf[index].C_value.f[0] = -PAY.buf[index].A_value.f[0];
	 break;

      case FNEG_D:
	 PAY.buf[index].C_value.d = -PAY.buf[index].A_value.d;
	 break;

      case CVT_S_D:
	 PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.d;
	 break;

      case CVT_S_W:
	 PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.w[0];
	 break;

      case CVT_D_S:
	 PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.f[0];
	 break;

      case CVT_D_W:
	 PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.w[0];
	 break;

      case CVT_W_S:
	 PAY.buf[index].C_value.w[0] = (long)PAY.buf[index].A_value.f[0];
	 break;

      case CVT_W_D:
	 PAY.buf[index].C_value.w[0] = (long)PAY.buf[index].A_value.d;
	 break;

      case C_EQ_S:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] == PAY.buf[index].B_value.f[0]);
         break;

      case C_EQ_D:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.d == PAY.buf[index].B_value.d);
	 break;

      case C_LT_S:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] < PAY.buf[index].B_value.f[0]);
         break;

      case C_LT_D:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.d < PAY.buf[index].B_value.d);
	 break;

      case C_LE_S:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] <= PAY.buf[index].B_value.f[0]);
         break;

      case C_LE_D:
	 PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.d <= PAY.buf[index].B_value.d);
	 break;

      case FSQRT_S:
	 PAY.buf[index].C_value.f[0] = sqrt((double)PAY.buf[index].A_value.f[0]);
	 break;

      case FSQRT_D:
	 PAY.buf[index].C_value.d = sqrt(PAY.buf[index].A_value.d);
	 break;

      default:
	 fatal("execute(): unknown operation (%d)", SS_OPCODE(inst));
	 break;
   }

}



//////////////////////////////////////////////////
// non-expression instruction implementations
//////////////////////////////////////////////////

//
// rd <- ([rt] >> shamt)
//
static void
ExecSRA(SS_WORD_TYPE rt, unsigned int shamt, SS_WORD_TYPE *rd)
{
  unsigned int i;
  SS_WORD_TYPE temp;

  /* Although SRA could be implemented with the >> operator in the
     MIPS machine, there are other machines that perform a logical
     right shift with the >> operator. */
  if (rt & 020000000000) {
    temp = rt;
    for (i = 0; i < shamt; i++)
      temp = (temp >> 1) | 020000000000 ;

    *rd = temp;
  }
  else {
    *rd = (rt >> shamt);
  }
}

//
// rd <- [rt] >> [5 LSBs of rs])
//
static void
ExecSRAV(SS_WORD_TYPE rt, SS_WORD_TYPE rs, SS_WORD_TYPE *rd)
{
  unsigned int i;
  SS_WORD_TYPE temp;
  unsigned int shamt = (rs & 037);

  if (rt & 020000000000) {
    temp = rt;
    for (i = 0; i < shamt; i++)
      temp = (temp >> 1) | 020000000000 ;

    *rd = temp;
  }
  else {
    *rd = (rt >> shamt);
  }
}

//
// HI,LO <- [rs] * [rt]    Integer product of [rs] & [rt] to HI/LO
//
// op1 = RS
// op2 = RT
//
static void
ExecMULT(SS_WORD_TYPE op1, SS_WORD_TYPE op2,
	 SS_WORD_TYPE *hi, SS_WORD_TYPE *lo)
{
  int sign1, sign2;
  SS_WORD_TYPE temp_hi, temp_lo;
  long i;

  sign1 = sign2 = 0;
  temp_hi = 0;
  temp_lo = 0;

  /* For multiplication, treat -ve numbers as +ve numbers by
     converting 2's complement -ve numbers to ordinary notation */
  if (op1 & 020000000000) {
    sign1 = 1;
    op1 = (~op1) + 1;
  }
  if (op2 & 020000000000) {
    sign2 = 1;
    op2 = (~op2) + 1;
  }
  if (op1 & 020000000000)
    temp_lo = op2;
  for (i = 0; i < 31; i++) {
    temp_hi = (temp_hi << 1);
    temp_hi = (temp_hi + extractl(temp_lo, 31, 1));
    temp_lo = (temp_lo << 1);
    if ((extractl(op1, 30-i, 1)) == 1) {
      if (((unsigned)037777777777 - (unsigned)temp_lo) < (unsigned)op2) {
	temp_hi = (temp_hi + 1);
      }
      temp_lo = (temp_lo + op2);
    }
  }

  /* Take 2's complement of the result if the result is negative */
  if (sign1 ^ sign2) {
    temp_lo = (~temp_lo);
    temp_hi = (~temp_hi);
    if ((unsigned)temp_lo == 037777777777) {
      temp_hi = (temp_hi + 1);
    }
    temp_lo = (temp_lo + 1);
  }

  *hi = temp_hi;
  *lo = temp_lo;
}

//
// HI,LO <- [rs] * [rt]    Integer product of [rs] & [rt] to HI/LO
//
static void
ExecMULTU(SS_WORD_TYPE rs, SS_WORD_TYPE rt,
	  SS_WORD_TYPE *hi, SS_WORD_TYPE *lo)
{
  long i;
  SS_WORD_TYPE temp_hi, temp_lo;

  temp_hi = 0;
  temp_lo = 0;
  if (rs & 020000000000)
    temp_lo = rt;
  for (i = 0; i < 31; i++) {
    temp_hi = (temp_hi << 1);
    temp_hi = (temp_hi + extractl(temp_lo, 31, 1));
    temp_lo = (temp_lo << 1);
    if ((extractl(rs, 30-i, 1)) == 1) {
      if (((unsigned)037777777777 - (unsigned)temp_lo) < (unsigned)rt) {
	temp_hi = (temp_hi + 1);
      }
      temp_lo = (temp_lo + rt);
    }
  }

  *hi = temp_hi;
  *lo = temp_lo;
}
