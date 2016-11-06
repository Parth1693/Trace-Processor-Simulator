#include "processor.h"

void processor::trace_schedule(unsigned int pe_number)
{
	if(TR_PAY.is_empty())
		return;	
	
   PE_array[pe_number]->IQ_INT.select_and_issue(issue_width, PE_array[pe_number]->Execution_Lanes);	// Issue instructions from integer IQ to the Execution Lanes.
   PE_array[pe_number]->IQ_FP.select_and_issue(issue_width, PE_array[pe_number]->Execution_Lanes);	// Issue instructions from floating-point IQ to the Execution Lanes.
}
