#ifndef TRACE_PREDICTOR_INCLUDED
#define TRACE_PREDICTOR_INCLUDED

typedef struct
{
	unsigned int pc;   //start pc
	unsigned int br_flag;  //embedded blocks
	unsigned int cntr;  //confidence counter
	bool valid;  //if the entry is valid or not
}table;

class predictor
{

public:
	unsigned int table_entries; //number of prediction table entries
	unsigned int hist_reg_entries;  //Nunber of history hashed ID
	table *PT;
	unsigned int *hist_reg; 	//by default 16 bit each hashed ID
	unsigned int max_m;			//embedded blocks
	unsigned int d;
	unsigned int o;
	unsigned int l;
	unsigned int c;

	predictor(unsigned int table_size, unsigned int hashed_ids, unsigned int max_m, 
					unsigned int d, unsigned int o, unsigned int l, unsigned int c);
	~predictor();
	void put_hash(unsigned int pc, unsigned int br_flag); //shift and put the new hash value
	void incr_cntr(unsigned int pc, unsigned int br_flag, unsigned int index); 	// This is called at retire time
																					// when prediction is correct.
	unsigned int index_gen(); 	//get index from hashed IDs
	void access_table(unsigned int &pc, unsigned int &br_flag, unsigned int &table_index); 	//get predicted TID from the predictor table
	void mispredict(unsigned int index, unsigned int pc, unsigned int br_flag); 		//called at retire time
	void decr_cntr(unsigned int pc, unsigned int br_flag, unsigned int index);			//Called inside mispredict function
		
};
#endif


