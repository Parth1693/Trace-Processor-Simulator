#include "processor.h"


void processor::trace_predict() 
{
	//Fetch stage is stalled and it hasn't taken the previous prediction yet and hence stall trace predict stage
	if(TR_FETCH.valid) 
		return;

	//Variables
	unsigned int pc,br_flag,table_index;
	
	//Access Predictor table
	TP.access_table(pc, br_flag, table_index);
	
	//Put the entries in pipeline reg between Predicto and Fetch stages
	TR_FETCH.valid = true;
	TR_FETCH.pc = pc;
	TR_FETCH.br_flag = br_flag;
	TR_FETCH.table_index = table_index;
	
	//Put new hash
	//TP.put_hash(pc, br_flag);
}

