//Renamer class
#include "processor.h"
#include "defs.h"
#include "renamer.h"

int c = 0;

//Constructor
renamer::renamer(unsigned int n_log_regs,
		unsigned int n_phys_regs)
{
	logRegs = n_log_regs;
	phyRegs = n_phys_regs;

	unsigned int listSize = phyRegs - logRegs;

	assert (phyRegs > logRegs);

	RMT = new unsigned int[logRegs];
	AMT = new unsigned int[logRegs];

	freeList = new FList(listSize, logRegs);
	//activeList = new AList(listSize);

	GRF = new unsigned long long[phyRegs];
	GRF_ready = new int[phyRegs];

	CP = new checkpoints[TOTAL_PE];
	for (int i=0; i<TOTAL_PE; i++)
	{
		CP[i].mapTable = new unsigned int[logRegs];
		CP[i].tp_hist = new unsigned int[HASHED_IDS];
	}

	//Initialize structures

	for (int i=0; i<logRegs; i++)
	{
		RMT[i] = i;
	}

	for (int i=0; i<logRegs; i++)
	{
		AMT[i] = i;
	}

	for (int i=0; i<phyRegs; i++)
	{
		GRF[i] = 0;
	}

	for (int i=0; i<phyRegs; i++)
	{
		GRF_ready[i] = 1;
	}

	freeList->initialize();
}	

renamer::~renamer()
{
	delete [] RMT;
	delete [] AMT;
	delete [] GRF;
	delete [] GRF_ready;

	for (int i=0; i<TOTAL_PE; i++)
	{
		delete [] CP[i].mapTable;
		delete [] CP[i].tp_hist;
	}

	delete [] CP;
	
}

//Rename Stage function

bool renamer::stall_reg(unsigned int bundle_dst)
{
	int freeRegs;
	freeRegs = freeList->getFLentries();

	if ( freeRegs >= bundle_dst)
		return false;	//No need to stall.
	else	
		return true;	//Stall required.
}

unsigned int renamer::rename_rsrc(unsigned int log_reg)
{
 	return RMT[log_reg];
}

unsigned int renamer::rename_rdst(unsigned int log_reg)
{
	//Pop head of free list.
	unsigned int popReg;
	popReg = freeList->popFL();
	//assert( (freeList->head >= 0) && (freeList->tail >= 0) );

	//Update the RMT
	RMT[log_reg] = popReg;

	return popReg;
}

// This function is used create a checkpoint after every trace renaming.
unsigned int renamer::checkpoint(trace_payload &TR_PAY, predictor &TP)
{
	unsigned int ind;
	if (TR_PAY.tail != 0)
		ind = (TR_PAY.tail - 1);
	else ind = TOTAL_PE - 1;

	//Copy the RMT to shadow map table.
	for (int i=0; i<logRegs; i++)
	{
		CP[ind].mapTable[i] = RMT[i];
	}

	//Copy the trace predictor branch history
	for(int i=0; i<HASHED_IDS; i++)
	{
		CP[ind].tp_hist[i] = TP.hist_reg[i];
	}

	//Copy Free List head
	CP[ind].FLhead = freeList->head;

	return ind;
}

//Resolve mispredicted branch
void renamer::resolve(unsigned int ID, predictor &TP)
{
#ifdef DEBUG
	/*if ( cycle >= 14962000 )
	{
		printf("Resolve called in cycle = %d with chkpoint ID = %d\n", cycle, ID);
		printf("FL rolled back to %d \n", CP[ID].FLhead);
	}*/
#endif
	//Restore FL
	freeList->head = CP[ID].FLhead;
	freeList->setFLValid();

	//Restore RMT
	for (int i=0; i<logRegs; i++)
	{
		RMT[i] = CP[ID].mapTable[i];
	}

	//Restore trace predictor history
	for(int i=0; i<HASHED_IDS; i++)
	{
		TP.hist_reg[i] = CP[ID].tp_hist[i];
	}
}
/**************   Schedule Stage functions  ***********************/

/* Test if the physical register is ready */
bool renamer::is_ready(unsigned int phys_reg)
{
	return (GRF_ready[phys_reg] == 1);
}

/* Clear ready bit for physical register */
void renamer::clear_ready(unsigned int phys_reg)
{
	GRF_ready[phys_reg] = 0;
}

/* Set ready bit for physical register */
void renamer::set_ready(unsigned int phys_reg)
{
	GRF_ready[phys_reg] = 1;
}

/**************   Reg Read Stage functions  ***********************/

/* Read physical register value from PRF */
unsigned long long renamer::read(unsigned int phys_reg)
{
	return GRF[phys_reg];
}

/**************   Writeback Stage functions  ***********************/

/* Write physical register value in PRF */
void renamer::write(unsigned int phys_reg, unsigned long long value)
{
	GRF[phys_reg] = value;
}


void renamer::restoreRMT()
{
	//Copy AMT to RMT
	for (int i=0; i<logRegs; i++)
	{
		RMT[i] = AMT[i];
	}

	return;
}

//Free list class functions
FList::FList(unsigned int size, unsigned int logs)
{
	FL_size = size - 1;
	logical = logs;
	FL = new unsigned int[size];
	valid = new int[size];
}

void FList::initialize()
{
	head = 0;
	tail = -1;

	//Write initial values into the free list
	for ( int i=0; i<=FL_size; i++)
	{
		FL[i] = i + logical;
		valid[i] = 1;
	}
}

unsigned int FList::popFL()
{
	int pos = head;

	if ( head == FL_size )
	{
		head = 0;
		valid[pos] = 0;
		/*if ( c == 1 )
		{
			assert (tail >= 0);
		}*/
		return FL[pos];
	}
	else
	{
		head++;
		valid[pos] = 0;
		/*if ( c == 1 )
		{
			assert (tail >= 0);
		}*/
		return FL[pos];
	}

}

void FList::pushFL(unsigned int reg)
{
	c = 1;

	if ( tail == FL_size)
	{
		tail = 0;
		FL[tail] = reg;
		valid[tail] = 1;
		if ( c == 1 )
		{
			assert (tail >= 0);
		}
		return;
	}
	else
	{
		tail++;
		FL[tail] = reg;
		valid[tail] = 1;
		if ( c == 1 )
		{
			assert (tail >= 0);
		}	
		return;
	}
}

bool FList::getFLStatus()
{
	int cnt = 0;

	for ( int i=0; i<=FL_size; i++ )
	{
		if ( valid[i] == 1)
			cnt++;
	}

	if ( cnt == FL_size + 1)
		return true;	//Full
	else 
		return false;	//Not full
}

int FList::getFLentries()
{
	int a,b;

    if ( (head == 0) && (tail == FL_size) )
    {
    	return FL_size + 1;
    }

    else if ( head == tail + 1) 
    {
        return FL_size + 1;
    }

    else if ( tail == head )
    {
    	if ( (valid[head+1] == 1) || (valid[head-1] == 1) )
    	return FL_size+1;
    	else return 1;
    }

	else if ( tail > head)
    {
    	a = tail - head + 1;
    	return a;
    }

    else if ( tail < head)
    {
    	b = FL_size - head;
    	a = b + tail + 2;
    	return a;
   	}
}

void FList::restoreFL()
{
	//Roll back the head to the tail, make the entries in the FL valid.
	if ( tail >= 0)
	{
		head = tail;
	}

	if ( tail == -1 )
	{
		tail = -1;
	}

	else if ( tail == 0 )
	{
		tail = FL_size;
	}
	else if ( tail != 0 )
	{
		tail = tail - 1;
	}

	if ( c == 1 )
	{
		//assert (tail >= 0);
		//assert (head >= 0);
	}

	for ( int i=0; i<=FL_size; i++)
	{
		valid[i] = 1;
	}
	return;
}

void FList::setFLValid()
{
	//Make all entries invalid initially.
	for ( int i=0; i<=FL_size; i++ )
	{
		valid[i] = 0;
	}
	
	if ( head > tail )
	{
		for ( int i=head; i<=FL_size; i++ )
		{
			valid[i] = 1;
		}

		for ( int i=0; i<=tail; i++ )
		{
			valid[i] = 1;
		}
	}

	else if ( head < tail )
	{
		for ( int i=head; i<=tail; i++ )
		{
			valid[i] = 1;
		}
	}

	else if ( head == tail )
	{
		valid[head] = 1;
	}
}
