#ifndef PE_INCLUDED
#define PE_INCLUDED
//This is the main PE class. Queue of PE's will be used.
//Structures inside the PE include issue queue, local reg file, local ready bit array
//and execution lanes.
#include "processor.h"
#include "defs.h"

class PE_t
{
public:
	issue_queue IQ_INT;
	issue_queue IQ_FP;

	bool valid;
	unsigned int info_index;
	int issue_width;
	
	//Local register files
	unsigned long long *LRF_int;
	unsigned long long *LRF_fp;
	
	//Local register files ready bit arrays
	int *LRF_int_ready;
	int *LRF_fp_ready;

	PE_lane *Execution_Lanes;		//Issue width number of lanes
	unsigned int end_addr;			//Set to the taken or fall through addr of the last instruction in the trace.
									// fall through addr by default if trace does not end in a branch.
	
	unsigned int fu_lane_matrix[(unsigned int)NUMBER_FU_TYPES];	// Indexed by FU type: bit vector indicating which lanes have that FU type.		
	unsigned int fu_lane_ptr[(unsigned int)NUMBER_FU_TYPES];	// Indexed by FU type: lane to which the last instruction of that FU type was steered.


	// Issue width = 8.
	PE_t(int issue_width, unsigned int fu_lane_matrix[]);
	~PE_t();
	bool is_ready_int(unsigned int reg);
	void clear_ready_int(unsigned int reg);
	void set_ready_int(unsigned int reg);
	unsigned long long read_int(unsigned int reg);
	void write_int(unsigned int reg, unsigned long long value);

	bool is_ready_fp(unsigned int reg);
	void clear_ready_fp(unsigned int reg);
	void set_ready_fp(unsigned int reg);
	unsigned long long read_fp(unsigned int reg);
	void write_fp(unsigned int reg, unsigned long long value);
	void pe_free();
};
#endif

