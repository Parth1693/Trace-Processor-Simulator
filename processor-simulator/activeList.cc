#include "processor.h"

AList::AList()
{
	this->AL_size = TOTAL_PE;
	al_head = 0;
	al_tail = 0;
	al_length = 0;
	this->initialize();
}

AList::~AList()
{
	
}

void AList::initialize()
{
	for(int i=0; i<AL_size; i++)
	{
		AL[i].valid = false;
	}
}

unsigned int AList::enqueue(unsigned int index)
{
   unsigned int ind;

   ind = al_tail;
   AL[ind].info_index = index;
   AL[ind].valid = true;
	
   al_tail = MOD((al_tail + 1), AL_size);
   al_length += 1;

   // Check for overflowing buf.
   assert(al_length <= AL_size);

   return(ind);
}

unsigned int AList::dequeue()
{
   // Increment al_head by one.
   int index = al_head;
   al_head = MOD((al_head + 1), AL_size);
   al_length -= 1;

   // Check for underflowing instr_buf.
   assert(al_length >= 0);
	
	AL[index].valid = false;
   return AL[index].info_index;
}

//Used to check if head trace is ready to be committed.
void AList::commit_trace(bool &completed, bool &exception, trace_payload &TR_PAY, payload &PAY)
{
	//if(AL[al_head].valid == true)
	//{
		int cflag = 1;
		int eflag = 1;
		unsigned int start_index = TR_PAY.tr_buf[AL[al_head].info_index].start_index;
		unsigned int end_index = TR_PAY.tr_buf[AL[al_head].info_index].end_index;	
		unsigned int next = start_index;
		unsigned int len = MOD((PAYLOAD_BUFFER_SIZE - start_index + end_index),PAYLOAD_BUFFER_SIZE);
		len = (len >> 1) + 1;
	
		if(PAY.head == PAY.tail) //Pay buff is empty
		{
			completed = false;
			exception = false;
			return;
		}
	
		for(int iter=0; iter < len; iter++)
		{
			unsigned int i = next;
			if (!PAY.buf[i].completed)
				cflag = 0;
			if (PAY.buf[i].exception)
				eflag = 0;
		
			if (PAY.buf[i].split == true)
			{
				if (!PAY.buf[i+1].completed)
					cflag = 0;
				if (PAY.buf[i+1].exception)
					eflag = 0;
			}

			next = MOD((next+2),PAYLOAD_BUFFER_SIZE);
		}

		if (!cflag)
			completed = false;
		else completed = true;

		if (!eflag)
			exception = true;
		else exception = false;		
	//}
	//else
	//{
	//	completed = false;
	//	exception = false;
	//}

}

//Used to commit each instruction inside the trace.
void AList::commit_inst(unsigned int i, bool &load, bool &store,			
	    bool &exception, unsigned int &offending_PC, payload &PAY)
{
	bool ld_flag = IS_LOAD(PAY.buf[i].flags);
	bool st_flag = IS_STORE(PAY.buf[i].flags) && !(PAY.buf[i].split_store && PAY.buf[i].upper);

	assert(PAY.buf[i].completed == true);
	exception = PAY.buf[i].exception;
	offending_PC = PAY.buf[i].pc;
	load = ld_flag;
	store = st_flag;
}

/*
// Index is trace level index. 'i' is index within the trace.
void AList::set_inst_complete(unsigned int index, int i)
{
	PAY.buf[AL[index].index].completed = true;
}

void AList::set_inst_exception(unsigned int index, int i)
{
	PAY.buf[AL[index].index].trace_buf[i].exception = true;
}

//Entire trace has completed execution
void AList::set_trace_complete(unsigned int index)
{
	AL[index].completed = true;
}

//Some instruction in the trace has encountered an exception.
void AList::set_trace_exception(unsigned int index)
{
	AL[index].exception = true;
}
*/

void AList::al_squash()
{
	// Rollback the tail to the head.
	al_tail = 0;
	al_head = 0;
	al_length = 0;
	this->initialize();
}

//Called NEVER
void AList::al_clear()
{
   al_head = 0;
   al_tail = 0;
   al_length = 0;
   this->initialize();
}

void AList::al_rollback(unsigned int index)
{
   // Rollback the tail to the instruction after the instruction at 'index'.
   //al_tail = MOD((index + 1), AL_size);
	al_tail = index;		//Similar implementation to OTB and PE.

   // Recompute the length.
   al_length = MOD((AL_size + al_tail - al_head), AL_size);

	unsigned int next = al_tail;
	unsigned int ind;
   for(int i=0; i< AL_size-al_length ;i++)
   	{
   		ind = next;
   		AL[ind].valid = false;
		next = MOD((next+1),AL_size);
   	}
}

bool AList::al_is_empty()
{
	if (al_length <= AL_size)
	{
		assert(al_head != al_tail);
		return true;
	}
	else
		return false;
}

bool AList::is_empty()
{
	if(al_length == 0)
		return true;
	else
		return false;
}

bool AList::is_full()
{
	if(al_length == TOTAL_PE)
		return true;
	else
		return false;
}
