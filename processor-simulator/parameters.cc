#include "fu.h"


// Benchmark control.
unsigned int SKIP_AMT		= 0;

// Oracle controls.
bool PERFECT_BRANCH_PRED	= false;
bool PERFECT_FETCH		= false;
bool ORACLE_DISAMBIG		= false;
//+Radhika
bool PERFECT_TRACECACHE = false;
bool PERFECT_ICACHE		= false;
bool PERFECT_DCACHE		= false;

// Core.
unsigned int FETCH_QUEUE_SIZE	= 32;
unsigned int NUM_CHECKPOINTS	= 32;
unsigned int ACTIVE_LIST_SIZE	= 128;
unsigned int ISSUE_QUEUE_SIZE	= 32;
unsigned int LQ_SIZE		= 32;
unsigned int SQ_SIZE		= 32;
unsigned int FETCH_WIDTH	= 4;
unsigned int DISPATCH_WIDTH	= 4;
unsigned int ISSUE_WIDTH	= 8;
unsigned int RETIRE_WIDTH	= 4;
bool IC_INTERLEAVED		= true;
bool IC_SINGLE_BB		= false;	// not used currently
bool IN_ORDER_ISSUE		= false;	// not used currently
unsigned int FU_LANE_MATRIX[(unsigned int)NUMBER_FU_TYPES] = {0x04 /*     BR: 0000 0100 */ ,
							      0x18 /*     LS: 0001 1000 */ ,
							      0x07 /*  ALU_S: 0000 0111 */ ,
							      0x02 /*  ALU_C: 0000 0010 */ ,
							      0x10 /*  LS_FP: 0001 0000 */ ,
							      0xE0 /* ALU_FP: 1110 0000 */ ,
							      0x10 /*    MTF: 0001 0000 */ };

// L1 Data Cache.
unsigned int L1_DC_SETS = 256;
unsigned int L1_DC_ASSOC = 4;
unsigned int L1_DC_LINE_SIZE = 6;
unsigned int L1_DC_HIT_LATENCY = 1;
unsigned int L1_DC_MISS_LATENCY = 10;
unsigned int L1_DC_NUM_MHSRs = 1024; //512;
unsigned int L1_DC_MISS_SRV_PORTS = 1;
unsigned int L1_DC_MISS_SRV_LATENCY = 1;

// L1 Instruction Cache.
unsigned int L1_IC_SETS = 256;
unsigned int L1_IC_ASSOC = 4;
unsigned int L1_IC_LINE_SIZE = 7;	// 2x D$ line size because 8B/instr
unsigned int L1_IC_HIT_LATENCY = 1;
unsigned int L1_IC_MISS_LATENCY = 10;
unsigned int L1_IC_NUM_MHSRs = 32;
unsigned int L1_IC_MISS_SRV_PORTS = 1;
unsigned int L1_IC_MISS_SRV_LATENCY = 1;

// Branch predictor confidence.
bool CONF_RESET = true;
unsigned int CONF_THRESHOLD = 14;
unsigned int CONF_MAX = 15;

bool FM_RESET = true;
unsigned int FM_THRESHOLD = 14;
unsigned int FM_MAX = 15;
