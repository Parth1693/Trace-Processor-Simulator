#include "processor.h"

// PE_t class functions.
// PE_t defines each processing element along with its internal structures.

PE_t::PE_t(int issue_width, unsigned int fu_lane_matrix[]):IQ_INT(TRACE_SIZE*2),IQ_FP(TRACE_SIZE*2)
{
	this->issue_width = issue_width;
	Execution_Lanes = new PE_lane[issue_width];

   for (int i = 0; i < (unsigned int)NUMBER_FU_TYPES; i++)
   {
      this->fu_lane_matrix[i] = fu_lane_matrix[i];
      this->fu_lane_ptr[i] = 0;
   }

	LRF_int = new unsigned long long[TRACE_SIZE*2];
	LRF_fp = new unsigned long long[TRACE_SIZE*2];

	LRF_int_ready = new int[TRACE_SIZE*2];
	LRF_fp_ready = new int[TRACE_SIZE*2];

	for(int i=0;i<TRACE_SIZE*2; i++)
	{
		LRF_int_ready[i] = 0;
		LRF_fp_ready[i] = 0;
	}
	end_addr = 0;

	this->valid = false;
	for (int i=0; i<this->issue_width; i++)
	   {
	   		Execution_Lanes[i].rr.valid = false;
	   		Execution_Lanes[i].ex.valid = false;
	   		Execution_Lanes[i].wb.valid = false;
	   }
}

PE_t::~PE_t()
{
	delete [] Execution_Lanes;
	delete [] LRF_int;
	delete [] LRF_fp;
	delete [] LRF_int_ready;
	delete [] LRF_fp_ready;
}

//INT local reg file funcions
bool PE_t::is_ready_int(unsigned int reg)
{
	return(LRF_int_ready[reg] == 1);
}

void PE_t::clear_ready_int(unsigned int reg)
{
	LRF_int_ready[reg] = 0;
}

void PE_t::set_ready_int(unsigned int reg)
{
	LRF_int_ready[reg] = 1;
}

//FP local reg file functions
bool PE_t::is_ready_fp(unsigned int reg)
{
	return(LRF_fp_ready[reg] == 1);
}

void PE_t::clear_ready_fp(unsigned int reg)
{
	LRF_fp_ready[reg] = 0;
}

void PE_t::set_ready_fp(unsigned int reg)
{
	LRF_fp_ready[reg] = 1;
}

unsigned long long PE_t::read_int(unsigned int reg)
{
	return LRF_int[reg];
}

void PE_t::write_int(unsigned int reg, unsigned long long value)
{
	LRF_int[reg] = value;
}

unsigned long long PE_t::read_fp(unsigned int reg)
{
	return LRF_fp[reg];
}

void PE_t::write_fp(unsigned int reg, unsigned long long value)
{
	LRF_fp[reg] = value;
}

//This function frees the PE by making all PE state as empty.
void PE_t::pe_free()
{
	this->valid = false;
	//Make IQ's empty.
	this->IQ_INT.flush();
	this->IQ_FP.flush();
	
   for (int i = 0; i < (unsigned int)NUMBER_FU_TYPES; i++)
   {
      this->fu_lane_ptr[i] = 0;
   }

   for (int i=0; i<this->issue_width; i++)
   {
   		Execution_Lanes[i].rr.valid = false;
   		Execution_Lanes[i].ex.valid = false;
   		Execution_Lanes[i].wb.valid = false;
   }

	for(int i=0;i<TRACE_SIZE*2; i++)
	{
		LRF_int_ready[i] = 0;
		LRF_fp_ready[i] = 0;
	}   
}

