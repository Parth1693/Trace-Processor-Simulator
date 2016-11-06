//New cache file.

#include "processor.h"
#include <math.h>

using namespace std;

/******Constructor to instantiate cache object*****/

trace_cache::trace_cache (int assoc, int block, int size, int m) 
{
	block_size = block;
	cache_size = size;
	cache_assoc = assoc;
	num_embedded_branches = m;

	//Cache configuration; Check for FA
	if ( cache_assoc < (cache_size))
	{
		num_ways = cache_assoc;
		num_sets = (cache_size / cache_assoc);
	}
	else if ( cache_assoc >= (cache_size) )
	{
		num_ways = cache_assoc;
		num_sets = 1;
	}

	//Calculate number of bits in TAG, INDEX and OFFSET; check for FA first.
	
	if ( cache_assoc < (cache_size))
	{
		//OFFSET num of bits
		offset_bits = log2(block_size);		//Keep = 3.
		//INDEX num of bits
		index_bits = log2(cache_size / cache_assoc);	
		//TAG num of bits
		tag_bits = 32 - offset_bits - index_bits;
	}
	else if ( cache_assoc >= (cache_size) )
	{
		offset_bits = log2(block_size);
		index_bits = 0;
		tag_bits = 32 - offset_bits - index_bits;
	}
	
	//Initialize 2D array of structs as main cache structure
	tr_cache =  new trace_block*[num_sets];                   //Row Count
	for(int c=0; c<num_sets; c++)                    //Row Count
	{
		tr_cache[c] = new trace_block[num_ways];           //Column Count 
	}
	
	//Make array of dim. num_sets * num_ways to handle LRU.
	LRU = new int*[num_sets];
	for(int c=0;c<num_sets;++c)
	{
		LRU[c] = new int[num_ways];
	}
	
	init_tr_cache();
	
	initLRU();

	accessCount = 0;

	//init stats
	tc_hits = 0;
	tc_misses = 0;
	reads = 0;
	writes = 0;

}  //Constructor ends here.


//Initialize the cache; Make all block invalid.
void trace_cache::init_tr_cache()
{
		int i, j;

		for(i=0;i<num_sets;i++)
			for(j=0;j<num_ways;j++)
			{
				tr_cache[i][j].validBit = 0;
			}
} //Init Cache ends here.


void trace_cache::initLRU()
{
	int i, j, k;
	
	for(i=0;i<num_sets;i++)
	{
		k = num_ways - 1;
		for(j=0;j<num_ways;j++)
		{
			LRU[i][j] = k;
			k--;
		}
	}

} //initLRU ends here.


void trace_cache::printState()
{
	int i,j;
	
	cout << '\n' ;
	cout << "===== L1 Contents =====" << endl;

	for(i=0;i<num_sets;i++)  {
		cout << "set  " << dec << i << ":\t";
		for(j=0; j<num_ways;j++) {			
			cout << hex << ( tr_cache[i][LRU[i][j]].tag << (tag_bits + index_bits + offset_bits) );
            cout << '\t';
		}
	cout << '\n' ;		
	}
}  //printState ends here

void trace_cache::printStats()
{	
	/*
	double swapRequestRate = double (tr_stats.swapRequests)/(tr_stats.reads + tr_stats.writes);
	double L1VCMissRate = double(tr_stats.readMisses + tr_stats.writeMisses - tr_stats.swaps)/(tr_stats.reads + tr_stats.writes);

	streamsize default_prec = cout.precision();
	
	cout << '\n';
	cout << "===== Simulation Results =====" << '\n' 
	<< "a. number of TC reads:\t\t\t" << dec << tr_stats.reads << '\n'
	<< "b. number of TC read misses:\t\t" << dec << tr_stats.readMisses << '\n'
	<< "c. number of TC writes:\t\t\t" << dec << tr_stats.writes << '\n'
	<< "d. number of TC write misses:\t\t" << dec << tr_stats.writeMisses << '\n'
	cout << "h. combined L1+VC miss rate:\t\t" << L1VCMissRate << '\n';
	cout.unsetf(ios::floatfield);
	//<< "h. combined L1+VC miss rate:\t\t" << setprecision(4) << L1VCMissRate << '\n'
	cout << "i. number writebacks from L1/VC:\t" << dec << stats.writeBacks << endl;

	cout.precision(default_prec);
	*/
} //printStats ends here

void trace_cache::updateLRU(unsigned int index, int way)
{
	int i, k, temp;

	//Search LRU[][]

	for(i=0;i<num_ways;i++)
	{
		if(LRU[index][i] == way)
		{
			k = i; 
			break;
		}
	}

	temp = LRU[index][k];

	for(i=k;i>0;i--)
	{
		//Shift the array elements upto the way#
		LRU[index][i] = LRU[index][i-1]; 
	}

	LRU[index][0] = temp;         //Make MRU

} //Update LRU ends here

unsigned int trace_cache::getLRU(unsigned int index)
{
	unsigned int a;

	a = LRU[index][num_ways-1];

	return a;  //Returns the way# which is the current LRU way.
} //getLRU ends here

//Cache functionality implementation

/*Call tr_access to check for hit or miss in case of read from trace cache*/
bool trace_cache::tr_access(int pc, int branchflag)
{
	//Parse address into tag, index. Remove offset.
	unsigned int mask = 1;
	unsigned int a;
	unsigned int index; 
	unsigned int tag;	
	
	int i;
	int found_way;
	int invalidBlock;
	int hit = 0;

	accessCount++;

	mask = ~((1 << num_embedded_branches) - 1);
	a = pc & mask;		//Mask lower 3 bits.

	mask = (1 << num_embedded_branches) - 1;
	branchflag = branchflag & mask;		//Take lower 3 bits.

	a = a | branchflag;

	mask = (1 << index_bits) - 1;

	if (cache_assoc < (cache_size) )
	{
		index = a & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size) )
	{
		index = 0;
	}

	//Tag
	tag = a;

	//Search the cache: Search particular cache index set. Consider validbit, branch_flag and branch_mask to determine cache hit.
	for(int i=0;i<num_ways;i++)
	{
		//Condition for Hit
		if( (tr_cache[index][i].validBit == 1) && (tr_cache[index][i].tag == tag) )   
		{	
			hit = 1; tc_hits++;
			found_way = i;	
		    	break;   
		}
		else 
		{
			hit = 0;  tc_misses++;
		}
	}

	//foundWay holds the way# where block was found.
	if(hit == 1)    
		return TRUE;
	else 
		return FALSE;
}

//Fetch a trace from trace cache to dispatch to the PEs
//We already know it will be a trace cache hit, since we have called the tr_access function to determine hit/miss.
trace_block trace_cache::get_trace(int pc, int branchflag)
{	
	//Parse address into tag, index. Remove offset.
	unsigned int mask = 1;
	unsigned int a;
	unsigned int index; 
	unsigned int tag;	
	
	int i;
	int found_way;
	int invalidBlock;
	int hit = 0;

	accessCount++;

	mask = ~((1 << num_embedded_branches) - 1);
	a = pc & mask;		//Mask lower 3 bits.

	mask = (1 << num_embedded_branches) - 1;
	branchflag = branchflag & mask;		//Take lower 3 bits.

	a = a | branchflag;

	mask = (1 << index_bits) - 1;

	if (cache_assoc < (cache_size) )
	{
		index = a & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size) )
	{
		index = 0;
	}

	//Tag
	tag = a;

	//Update read count stat
	reads ++;

	//Search the cache: Search particular cache index set.
	//Assert that it will be a trace cache hit.
	for(int i=0;i<num_ways;i++)
	{
		//Condition for Hit
		if( (tr_cache[index][i].validBit == 1) && (tr_cache[index][i].tag == tag) )   
		{	
			hit = 1;  
			found_way = i;	
		    break;   
		}
		else 
		{
			hit = 0;  
		}
	}

	assert(hit == 1);
	//foundWay holds the way# where block was found.
	//Make that way# as the MRU.
	updateLRU(index, found_way);

	return tr_cache[index][found_way];
}

/*Insert a trace into the trace cache from line-fill buffer*/
void trace_cache::put_trace(trace_block filltrace)
{
	//Parse address into tag, index. Remove offset.
	unsigned int mask = 1;
	unsigned int a;
	unsigned int index; 
	unsigned int tag;
	unsigned int branchflag;	
	
	int i;
	int found_way;
	int invalidBlock;
	int empty = 0;

	mask = ~((1 << num_embedded_branches) - 1);
	a = filltrace.tr_info.start_pc & mask;		//Mask lower 3 bits.

	mask = (1 << num_embedded_branches) - 1;
	branchflag = filltrace.tr_info.br_flag & mask;		//Take lower 3 bits.

	a = a | branchflag;

	mask = (1 << index_bits) - 1;

	if (cache_assoc < (cache_size) )
	{
		index = a & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size) )
	{
		index = 0;
	}

	//Tag
	tag = a;

	//Update write count stat
	writes++;
	
	//Check if there is space in the particular trace cache index set.
	for (int i=0; i<num_ways; i++)
	{
		if (tr_cache[index][i].validBit == 0)
		{	
			empty = 1; 
			found_way = i;
			break;
		}
	}

	if ( empty == 1)
	{
		//Put the trace in the empty way.
		for(int i=0; i<TRACE_SIZE*2; i++)
		{
			tr_cache[index][found_way].buf[i] = filltrace.buf[i];
		}
		tr_cache[index][found_way].validBit = 1;
		tr_cache[index][found_way].tag = tag;
		tr_cache[index][found_way].tr_info = filltrace.tr_info; 

		//Update LRU information.
		updateLRU(index, found_way);
	}
	else
	{
		//Evict the LRU way, and put new trace in that way.
		int lru_way = getLRU(index);
		tr_cache[index][lru_way].tr_info = filltrace.tr_info;
		tr_cache[index][lru_way].validBit = 1;
		tr_cache[index][lru_way].tag = tag;
		for(int i=0; i<TRACE_SIZE*2; i++)
		{
			tr_cache[index][lru_way].buf[i] = filltrace.buf[i];
		}

		//Update LRU information.
		updateLRU(index, lru_way);
		
	}
}


