#ifndef _PARAMETERS_H
#define _PARAMETERS_H

// Benchmark control.
extern bool USE_INSTR_LIMIT;
extern SS_TIME_TYPE INSTR_LIMIT;
extern unsigned int SKIP_AMT;

// Oracle controls.
extern bool PERFECT_BRANCH_PRED;
extern bool PERFECT_FETCH;
extern bool ORACLE_DISAMBIG;
extern bool PERFECT_ICACHE;
extern bool PERFECT_DCACHE;

// Core.
extern unsigned int FETCH_QUEUE_SIZE;
extern unsigned int NUM_CHECKPOINTS;
extern unsigned int ACTIVE_LIST_SIZE;
extern unsigned int ISSUE_QUEUE_SIZE;
extern unsigned int LQ_SIZE;
extern unsigned int SQ_SIZE;
extern unsigned int FETCH_WIDTH;
extern unsigned int DISPATCH_WIDTH;
extern unsigned int ISSUE_WIDTH;
extern unsigned int RETIRE_WIDTH;
extern bool IC_INTERLEAVED;
extern bool IC_SINGLE_BB;		// not used currently
extern bool IN_ORDER_ISSUE;		// not used currently
extern unsigned int FU_LANE_MATRIX[];

// L1 Data Cache.
extern unsigned int L1_DC_SETS;
extern unsigned int L1_DC_ASSOC;
extern unsigned int L1_DC_LINE_SIZE;
extern unsigned int L1_DC_HIT_LATENCY;
extern unsigned int L1_DC_MISS_LATENCY;
extern unsigned int L1_DC_NUM_MHSRs;
extern unsigned int L1_DC_MISS_SRV_PORTS;
extern unsigned int L1_DC_MISS_SRV_LATENCY;

// L1 Instruction Cache.
extern unsigned int L1_IC_SETS;
extern unsigned int L1_IC_ASSOC;
extern unsigned int L1_IC_LINE_SIZE;
extern unsigned int L1_IC_HIT_LATENCY;
extern unsigned int L1_IC_MISS_LATENCY;
extern unsigned int L1_IC_NUM_MHSRs;
extern unsigned int L1_IC_MISS_SRV_PORTS;
extern unsigned int L1_IC_MISS_SRV_LATENCY;

// Branch predictor confidence.
extern bool CONF_RESET;
extern unsigned int CONF_THRESHOLD;
extern unsigned int CONF_MAX;

extern bool FM_RESET;
extern unsigned int FM_THRESHOLD;
extern unsigned int FM_MAX;

#endif
