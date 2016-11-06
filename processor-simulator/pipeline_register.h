// +Parth - ONLY the register between trace predictor and trace fetch stages is special since
// it holds information about the pc and branch flag.
// All other stages use the common trace_pipeline_register class.
// This class holds start and end indexes of the trace in the PAY.buf[].
// Inside a PE, the pipeline regs between IQ, RR and EX hold only one instruction at a time.
// Therefore, they use the 721sim pipeline_register class.

class tr_fetch_register {
public:

	bool valid;
	unsigned int pc;
	unsigned int br_flag;
  unsigned int table_index;
	
	tr_fetch_register();
  //~tr_fetch_register();
};

// Used between FE/RN, RN/DI and DI/PE stages
class trace_pipeline_register {
public:

   bool valid;	// valid instruction
   unsigned int info_index;				// Trace level index in TR_PAY.tr_buf[]

   trace_pipeline_register();	    // constructor
  ~trace_pipeline_register();
};

class pipeline_register {
public:

   bool valid;					// valid instruction
   unsigned int index;	//Index of trace in PAY.buf[]   Instruction level index

   pipeline_register();	// constructor
   ~pipeline_register();
};
