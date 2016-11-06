#include "processor.h"

tr_fetch_register::tr_fetch_register()
{
	valid = false;
	pc = 0;
	br_flag = 0;
}

trace_pipeline_register::trace_pipeline_register()
{
	valid = false;
}

trace_pipeline_register::~trace_pipeline_register()
{
	
}

pipeline_register::pipeline_register() 
{
   valid = false;
}

pipeline_register::~pipeline_register() 
{
	
}

