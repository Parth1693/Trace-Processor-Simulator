//New cache file.

#include "cache.h"

using namespace std;

/******Constructor to instantiate cache object*****/

trace_cache::trace_cache (int assoc, int block, int size, int m) {
	block_size = block;
	cache_size = size;
	cache_assoc = assoc;
	num_embedded_branches = m;

	//Cache configuration; Check for FA
	if ( cache_assoc < (cache_size/block_size))
	{
		num_ways = cache_assoc;
		num_sets = (cache_size / ( block_size * cache_assoc));
	}
	else if ( cache_assoc >= (cache_size/block_size) )
	{
		num_ways = cache_assoc;
		num_sets = 1;
	}

	//Calculate number of bits in TAG, INDEX and OFFSET;check for FA first.
	
	if ( cache_assoc < (cache_size/block_size))
	{
		//OFFSET num of bits
		offset_bits = log2 (block_size);
		//INDEX num of bits
		index_bits = log2 (cache_size / ( block_size * cache_assoc));	
		//TAG num of bits
		tag_bits = 32 - offset_bits - index_bits;
	}
	else if ( cache_assoc >= (cache_size/block_size) )
	{
		offset_bits = log2 (block_size);
		index_bits = 0;
		tag_bits = 32 - offset_bits - index_bits;
	}
	
	//Initialize 2D array of structs as main cache structure
	tr_cache =  new tr_block*[num_sets];                   //Row Count
	for(int c=0; c<num_sets; ++c)                    //Row Count
	{
		tr_cache[c] = new tr_block[num_ways];           //Column Count 
	}
	
	//Initialize the cacheStatistics structure with zeros.
	tr_stats = { 0, 0, 0, 0, 0};

	//Make array of dim. num_sets * num_ways to handle LRU.
	LRU = new int*[num_sets];
	for(c=0;c<num_sets;++c)
	{
		LRU[c] = new int[num_ways];
	}
	
	init_tr_cache();
	
	initLRU();

	accessCount = 0;
}  //Constructor ends here.


//Initialize the cache; Make all block invalid.
void trace_cache::init_tr_cache()
{
		int i, j;

		for(i=0;i<num_sets;i++)
			for(j=0;j<num_ways;j++)
			{
				tr_cache[i][j].validBit = 0;
				tr_cache[i][j].branch_mask = 0;
				tr_cache[i][j].branch_flags = 0;
				tr_cache[i][j].end_branch = FALSE;
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


void trace_cache::tr_print_state()
{
	int i,j;
	
	cout << '\n' ;
	cout << "===== Trace Cache Contents =====" << endl;

	for(i=0;i<num_sets;i++)  {
		cout << "set  " << dec << i << ":\t";
		for(j=0; j<num_ways;j++) {			
			cout << hex << ( cache[i][LRU[i][j]].tag << (tag_bits + index_bits + offset_bits) );
			if (cache[i][LRU[i][j]].dirtyBit == 1 ) cout << " D\t";
			else cout << '\t';
		}
	cout << '\n' ;		
	}
}

void trace_cache::tr_print_stats()
{	
	streamsize default_prec = cout.precision();
		
	tr_stats.missRate = (float)tr_stats.readMisses/(float)tr_stats.reads; 	

	cout << '\n';
	cout << "===== Trace Cache Stats =====" << '\n' 
	<< "a. number of TC reads:\t\t\t" << dec << tr_stats.reads << '\n'
	<< "b. number of TC read misses:\t\t" << dec << tr_stats.readMisses << '\n'
	<< "c. number of TC writes:\t\t\t" << dec << tr_stats.writes << '\n'
	<< "d. TC miss rate:\t\t\t\t" << dec << tr_stats.missRate << '\n' << endl;

} 

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

}

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
	 			
	//Index
	unsigned int mask = 0xFFFFFFFF;
	unsigned int a; 	
	
	int i;
	int found_way;
	int invalidBlock;
	int hit;

	accessCount++;

	a = pc >> offset_bits;
	mask = 0xFFFFFFFF;
	mask = mask >> (tag_bits + offset_bits);
	if (cache_assoc < (cache_size/block_size) )
	{
		index = (pc >> offset_bits) & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size/block_size) )
	{
		index = 0;
	}

	//Tag
	tag = pc >> (index_bits + offset_bits);
	
	//Update read count
	tr_stats.reads++;

	//Search the cache: Search particular cache index set. Consider validbit, branch_flag and branch_mask to determine cache hit.
	for(int i=0;i<num_ways;i++)
	{
		unsigned int mask = 1;
		mask = (mask << tr_cache[index][i].branch_mask) - 1;

		//Condition for Hit
		if( (tr_cache[index][i].validBit == 1) && (tr_cache[index][i].tag == tag) && (tr_cache[i][j].branch_flags & mask == branchflag) )   
		{	hit = 1; 
			found_way = i;	
		    break;   }
		else 
			hit = 0;
	}

	//foundWay holds the way# where block was found.
	if(hit == 1)    
		return TRUE;
	else 
	{
		tr_stats.readMisses++;
		return FALSE:
	}

}

//Fetch a trace from trace cache to dispatch to the PEs
//We already know it will be a trace cache hit, since we have called the tr_access function to determine hit/miss.
trace_payload * trace_cache::get_trace(int pc, int branchflag)
{	
	//Index
	unsigned int mask = 0xFFFFFFFF;
	unsigned int a; 	
	
	int i;
	int found_way;
	int invalidBlock;
	int hit;

	a = pc >> offset_bits;
	mask = 0xFFFFFFFF;
	mask = mask >> (tag_bits + offset_bits);
	if (cache_assoc < (cache_size/block_size) )
	{
		index = (pc >> offset_bits) & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size/block_size) )
	{
		index = 0;
	}

	//Tag
	tag = pc >> (index_bits + offset_bits);

	//Search the cache: Search particular cache index set.
	//Assert that it will be a trace cache hit.
	for(int i=0;i<num_ways;i++)
	{
		unsigned int mask = 1;
		mask = (mask << tr_cache[index][i].branch_mask) - 1;

		//Condition for Hit
		if( (tr_cache[index][i].validBit == 1) && (tr_cache[index][i].tag == tag) && (tr_cache[i][j].branch_flags & mask == branchflag) )   
		{	hit = 1; 
			found_way = i;	
		    break;   }
		else hit = 0;
	}

	assert(hit == 1);
	//foundWay holds the way# where block was found.
	//Make that way# as the MRU.

	updateLRU(index, found_way);

	return &tr_cache[index][found_way].trace;
}

/*Insert a trace into the trace cache from line-fill buffer*/
void trace_cache::put_trace(int pc, int branchflag, int num_branches, trace_payload filltrace)
{
	//Index
	unsigned int mask = 0xFFFFFFFF;
	unsigned int a; 	
	
	int i;
	int found_way;
	int invalidBlock;
	int empty = 0;

	mask = 0xFFFFFFFF;
	mask = mask >> (tag_bits + offset_bits);
	if (cache_assoc < (cache_size/block_size) )
	{
		index = (pc >> offset_bits) & mask;		//CHANGE THIS FOR PATH ASSOCIATIVITY!
	}
	else if (cache_assoc >= (cache_size/block_size) )
	{
		index = 0;
	}

	//Tag
	tag = pc >> (index_bits + offset_bits);

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

	tr_stats.writes++;
	if ( empty == 1)
	{
		//Put the trace in the empty way.
		tr_cache[index][found_way].trace = filltrace;
		tr_cache[index][found_way].validBit = 1;
		tr_cache[index][found_way].tag = tag;
		tr_cache[index][found_way].branch_flags = filltrace.branchflag;
		tr_cache[index][found_way].branch_mask = filltrace.branchmask;
		tr_cache[index][found_way].taken_addr = filltrace.takenaddr;
		tr_cache[index][found_way].fall_addr = filltrace.falladdr;
		tr_cache[index][found_way].end_branch = filltrace.endbranch;

		//Update LRU information.
		updateLRU(index, found_way);
	}
	else
	{
		//Evict the LRU way, and put new trace in that way.
		int lru_way = getLRU(index);
		tr_cache[index][lru_way].trace = filltrace;
		tr_cache[index][lru_way].validBit = 1;
		tr_cache[index][lru_way].tag = tag;
		tr_cache[index][lru_way].branch_flags = filltrace.branchflag;
		tr_cache[index][lru_way].branch_mask = filltrace.branchmask;
		tr_cache[index][lru_way].taken_addr = filltrace.takenaddr;
		tr_cache[index][lru_way].fall_addr = filltrace.falladdr;
		tr_cache[index][lru_way].end_branch = filltrace.endbranch;

		//Update LRU information.
		updateLRU(index, lru_way);

	}

}
