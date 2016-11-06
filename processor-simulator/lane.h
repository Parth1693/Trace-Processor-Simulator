class PE_lane {
	public:
		pipeline_register rr;	// pipeline register of Register Read Stage
		pipeline_register ex;	// pipeline register of Execute Stage
		pipeline_register wb;	// pipeline register of Writeback Stage

		PE_lane();	// constructor
};
