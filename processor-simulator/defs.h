#ifndef DEFS_INCLUDED
#define DEFS_INCLUDED

#define TOTAL_FP_REGS		32
#define TOTAL_GP_INT_REGS	32
#define TOTAL_INT_REGS		35

#define TOTAL_PE		2
#define TRACE_SIZE		16

#define HI_ID_NEW		32
#define LO_ID_NEW		33
#define FCC_ID_NEW		34

//#define DEBUG

//TP is trace predictor here
#define 	TP_TABLE_SIZE		65536		//our trace predictor index is 16 bit
#define 	TP_D_BITS			5 //9 //5
#define 	TP_O_BITS			5 //4 //5
#define		TP_L_BITS			5 //7 //5
#define		TP_C_BITS			7 //9 //7
#define 	HASHED_IDS			(TP_D_BITS+1)  //16

//TC parameters
#define		TC_ASSOC	2048
#define		TC_BLOCKSIZE	8
#define		TC_CACHESIZE	2048

#define		MAX_M		3		//Number of embedded blocks
#define 	MAX_N		16 		//Number of instructions in a trace

#endif

