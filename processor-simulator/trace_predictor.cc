#include "processor.h"
#include <math.h>

predictor::predictor(unsigned int table_size, unsigned int hashed_ids, unsigned int max_m, 
					unsigned int d, unsigned int o, unsigned int l, unsigned int c)
{
	table_entries = table_size;
	hist_reg_entries = hashed_ids;
	this->max_m = max_m;
	this->d = d;
	this->o = o;
	this->l = l;
	this->c = c;

	PT = new table[table_size];
	//Make all entries invlaid and cntr = 0;
	for(int i=0; i<table_size; i++)
	{
		PT[i].valid = false;		//Not used anywhere!!!
		PT[i].cntr = 0;
		PT[i].pc = 0;
		PT[i].br_flag = 0;
	}
	
	hist_reg = new unsigned int[hashed_ids];
	for (int i=0; i<hist_reg_entries; i++)
	{
		hist_reg[i] = 0;
	}
}
predictor::~predictor()
{
	delete[] PT;
	delete[] hist_reg;
}

void predictor::put_hash(unsigned int pc, unsigned int br_flag) 		//shift and put the new hash value
{
	// In history register, 0th one is the newest and then second is later one and so on...
	//So we need to shift each entry by one position and then put the new one at 0th position
	int i;
	for(i=hist_reg_entries-1; i>=1; i--)
	{
		hist_reg[i] = hist_reg[i-1];
	}
	
	unsigned int temp;
	unsigned int calc = 0x3;
	pc = pc >> 3;
	calc &= br_flag; 		//least significant 2 bits of br_flag
	temp = 0x3 & pc;		//least significant 2 bits of pc
	temp = temp << 2;
	calc |= temp;  			//Least significant 4 bits of hashed ID

	unsigned int pc_temp = pc>>2;
	unsigned int br_flg_temp = br_flag>>2;
	temp = pc_temp ^ br_flg_temp; //xor 
	temp = temp << 4;
	calc |= temp;
	
	//put in 0th history buffer
	hist_reg[0] = calc;
	
}

void predictor::incr_cntr(unsigned int pc, unsigned int br_flag, unsigned int index)
{
	PT[index].cntr++;
	if (PT[index].cntr > 3)
		PT[index].cntr = 3;
}
																			
unsigned int predictor::index_gen()
{
	//D = 5, O = 5, L=5, C=7
	
	//C bits from hist_reg[0]
	unsigned int temp = (1 << this->c) - 1;
	unsigned int c_bits = hist_reg[0] & temp;
	
	temp = (1 << l) - 1;
	unsigned int l_bits = hist_reg[1] & temp;
	temp = (1 << o) - 1;
	unsigned int o_bits = hist_reg[2] & temp;
	
	//OLC concat bits
	unsigned int olc_bits = (o_bits << (l+c) ) | (l_bits << c) | c_bits;
	
	temp = (1 << d) - 1;
	unsigned int d_array = 0;
	for(int i=3; i<this->d + 1; i++)
	{
		d_array = d_array | (hist_reg[i] & temp);
		d_array = d_array << o;
	}
	
	unsigned int ind = d_array ^ olc_bits;
	
	return (ind & 0xFFFF);
}

void predictor::access_table(unsigned int &pc, unsigned int &br_flag, unsigned int &table_index) 	//get predicted TID from the predictor table
{
	//Call index_gen to get the predictor index.
	
	unsigned int index;
	index = this->index_gen();

	unsigned int mask = log2(table_entries);

	index = index & ((1<<mask)-1);
	
	pc = PT[index].pc;
	br_flag = PT[index].br_flag;
	table_index = index;

}

void predictor::mispredict(unsigned int index, unsigned int pc, unsigned int br_flag) 		//called at retire time
{
	//Call decr_cntr to decrement counter value.
	this->decr_cntr(pc, br_flag, index);
	
	//Check for counter value; if cntr == 0, fill in the new value in PT.
	if (PT[index].cntr == 0)
	{
		PT[index].pc = pc;
		PT[index].br_flag = br_flag;
	}
}

void predictor::decr_cntr(unsigned int pc, unsigned int br_flag, unsigned int index)			//Called inside mispredict function
{
	if (PT[index].cntr >= 2)
		PT[index].cntr = PT[index].cntr - 2;
	else
		PT[index].cntr = 0;
}
