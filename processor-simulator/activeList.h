#ifndef AL_INCLUDED
#define AL_INCLUDED
//Global active list class.
#include "processor.h"

typedef struct 
{
	bool valid;
	unsigned int info_index;
} alist_t;

//Active list class
class AList
{
public:
	alist_t AL[TOTAL_PE];
	unsigned int AL_size;		// == Num of PEs (TOTAL_PE)
	int al_head;
	int al_tail;
	int al_length;

	//Constructor
	AList();
	~AList();

	//Functions
	void initialize();
	unsigned int dequeue();					//Pop head of AL
	unsigned int enqueue(unsigned int index);	//Push at tail of AL, return the active list index.
	bool al_is_empty();		//True means AL is full
	int al_get_entries();		//Get number of currently active traces.
	void al_rollback(unsigned int index);
	void commit_inst(unsigned int i, bool &load, bool &store,			//Used to commit each instruction inside the trace.
		    bool &exception, unsigned int &offending_PC, payload &PAY);
	void commit_trace(bool &completed, bool &exception, trace_payload &TR_PAY, payload &PAY);	//Used to check if head trace is ready to be committed.
	void al_squash();
	void al_clear();
	bool is_empty();
	bool is_full();
};

#endif

