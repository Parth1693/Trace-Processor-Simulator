#include <stdlib.h>

#include "Thread.h"
#include "parameters.h"
#include "histogram.h"
#include "cache.h"
#include "dcache.h"
#include "memory_macros.h"
#include "lsu.h"

bool lsu::disambiguate(unsigned int lq_index,
		       unsigned int sq_index, bool sq_index_phase,
		       bool& forward,
		       unsigned int& store_entry, bool &cheat) {
   bool stall;		// return value
   unsigned int max_size;
   unsigned int mask;
   
   cheat = false;

   // Check if the load is logically at the head of the SQ, i.e., no prior stores.
   if ((sq_index == sq_head) && (sq_index_phase == sq_head_phase)) {
      // There are no stores prior to the load.
      stall = false;
      forward = false;
   }
   else {
      stall = false;
      forward = false;

      // Because the load is not logically at the head of the SQ,
      // it must be true that the SQ has at least one store.
      assert(sq_length > 0);

      store_entry = sq_index;
      do {
         store_entry = MOD_S((store_entry + sq_size - 1), sq_size);

         max_size = MAX(SQ[store_entry].size, LQ[lq_index].size);
         mask = (~(max_size - 1));

	 if (!SQ[store_entry].addr_avail) {
	    stall = true;	// stall: possible conflict
	 }
	 else if ((SQ[store_entry].addr & mask) ==
		  (LQ[lq_index].addr    & mask)) {
	    // There is a conflict.
	    if (SQ[store_entry].size != LQ[lq_index].size)
		{
			stall = false;
			forward = false;
			cheat = true;
		}
	    	// Dont stall!
	       //stall = true;	// stall: partial conflict scenarios are hard
	    else if (!SQ[store_entry].value_avail)
	       stall = true;	// stall: must wait for value to be available
	    else
	       forward = true;	// forward: sizes match and value is available
	 }
      } while ((store_entry != sq_head) && !stall && !forward && !cheat);
   }

   return(stall);
}	// disambiguate()


void lsu::next_replay_index(bool restart) {
   unsigned int scan;

   if (restart) {

      scan = lq_head;
      replay_index = -1;

      for (unsigned int i = 0; i < lq_length; i++) {
         assert(LQ[scan].valid);
         if (LQ[scan].addr_avail && !LQ[scan].value_avail) {
            replay_index = scan;
            break;
         }
         scan = MOD_S((scan + 1), lq_size);
      }
   }
   else {
      assert(lq_length > 0);
      assert(replay_index >= 0);

      scan = MOD_S((replay_index + 1), lq_size);
      if (scan == lq_tail)
	 scan = lq_head;
      replay_index = -1;

      for (unsigned int i = 0; i < lq_length; i++) {
         assert(LQ[scan].valid);
         if (LQ[scan].addr_avail && !LQ[scan].value_avail) {
            replay_index = scan;
            break;
         }
         scan = MOD_S((scan + 1), lq_size);
	 if (scan == lq_tail)
	    scan = lq_head;
      }
   }
}


lsu::lsu(unsigned int lq_size, unsigned int sq_size, unsigned int Tid):
			DC(L1_DC_SETS,
			   L1_DC_ASSOC,
			   L1_DC_LINE_SIZE,
			   L1_DC_HIT_LATENCY,
			   L1_DC_MISS_LATENCY,
			   L1_DC_NUM_MHSRs,
			   L1_DC_MISS_SRV_PORTS,
			   L1_DC_MISS_SRV_LATENCY) {

	   this->Tid = Tid;

	   // LQ initialization.
	   this->lq_size = lq_size;
	   lq_head = 0;
	   lq_head_phase = false;
	   lq_tail = 0;
	   lq_tail_phase = false;
	   lq_length = 0;
	   replay_index = -1;

	   LQ = new lsq_entry[lq_size];
	   for (unsigned int i = 0; i < lq_size; i++)
	      LQ[i].valid = false;

	   // SQ initialization.
	   this->sq_size = sq_size;
	   sq_head = 0;
	   sq_head_phase = false;
	   sq_tail = 0;
	   sq_tail_phase = false;
	   sq_length = 0;

	   SQ = new lsq_entry[sq_size];
	   for (unsigned int i = 0; i < sq_size; i++)
	      SQ[i].valid = false;

	   // Architectural memory state initialization.
	   for (unsigned int i = 0; i < MEMORY_TABLE_SIZE; i++)
	      mem_table[i] = (char *)NULL;

	   // STATS
	   n_stall_disambig = 0;
	   n_forward = 0;
	   n_stall_miss_l = 0;
	   n_stall_miss_s = 0;
	   n_load = 0;
	   n_store = 0;
}

bool lsu::stall(unsigned int bundle_load, unsigned int bundle_store) {
	return( ((lq_length + bundle_load)  > lq_size) ||
	        ((sq_length + bundle_store) > sq_size) );
}


void lsu::dispatch(bool load,
		   unsigned int size,
		   bool left,
		   bool right,
		   bool is_signed,
		   unsigned int pay_index,
		   unsigned int& lq_index, bool& lq_index_phase,
		   unsigned int& sq_index, bool& sq_index_phase) {
	// Assign indices to the load or store.
	lq_index = lq_tail;
	lq_index_phase = lq_tail_phase;
	sq_index = sq_tail;
	sq_index_phase = sq_tail_phase;

	if (load) {
	   // Assert that the LQ isn't full.
	   assert(lq_length < lq_size);

	   // Allocate entry in the LQ.
	   LQ[lq_tail].valid = true;
	   LQ[lq_tail].is_signed = is_signed;
	   LQ[lq_tail].left = left;
           LQ[lq_tail].right = right;
	   LQ[lq_tail].size = size;
	   LQ[lq_tail].addr_avail = false;
	   LQ[lq_tail].value_avail = false;
	   LQ[lq_tail].missed = false;

	   LQ[lq_tail].pay_index = pay_index;
	   LQ[lq_tail].sq_index = sq_index;
	   LQ[lq_tail].sq_index_phase = sq_index_phase;

	   // STATS
	   LQ[lq_tail].stat_load_stall_disambig = false;
	   LQ[lq_tail].stat_load_stall_miss = false;
	   LQ[lq_tail].stat_store_stall_miss = false;
	   LQ[lq_tail].stat_forward = false;

	   // Advance LQ tail pointer and increment LQ length.
	   lq_tail = MOD_S((lq_tail + 1), lq_size);
	   lq_length++;

	   // Detect wrap-around of lq_tail and toggle its phase bit accordingly.
	   if (lq_tail == 0)
	      lq_tail_phase = !lq_tail_phase;
	}
	else {
	   // Assert that the SQ isn't full.
	   assert(sq_length < sq_size);

	   // Allocate entry in the SQ.
	   SQ[sq_tail].valid = true;
	   SQ[sq_tail].is_signed = is_signed;
	   SQ[sq_tail].left = left;
           SQ[sq_tail].right = right;
	   SQ[sq_tail].size = size;
	   SQ[sq_tail].addr_avail = false;
	   SQ[sq_tail].value_avail = false;
	   SQ[sq_tail].missed = false;	  
	   SQ[sq_tail].pay_index = pay_index;

	   // STATS
	   SQ[sq_tail].stat_load_stall_disambig = false;
	   SQ[sq_tail].stat_load_stall_miss = false;
	   SQ[sq_tail].stat_store_stall_miss = false;
	   SQ[sq_tail].stat_forward = false;

	   // Advance SQ tail pointer and increment SQ length.
	   sq_tail = MOD_S((sq_tail + 1), sq_size);
	   sq_length++;

	   // Detect wrap-around of sq_tail and toggle its phase bit accordingly.
	   if (sq_tail == 0)
	      sq_tail_phase = !sq_tail_phase;
	}
}


void lsu::store_addr(SS_TIME_TYPE cycle,
		     unsigned int addr,
		     unsigned int sq_index,
		     unsigned int lq_index, bool lq_index_phase) {	// Future functionality: store's LQ index will be needed for detecting mispredicted loads.
	assert(SQ[sq_index].valid);

	// Oracle memory disambiguation results in the store address being recorded twice:
	// the oracle store address at dispatch time and the computed store address at
	// execute time. If the address was already recorded at dispatch time, confirm that
	// it matches the one recorded at execute time (now). Then exit the function since
	// we do not want to access the Data Cache for a second time.
	if (SQ[sq_index].addr_avail) {
	   assert(SQ[sq_index].addr == addr);
	   return;
	}

	SQ[sq_index].addr_avail = true;
	SQ[sq_index].addr = addr;

	if (!PERFECT_DCACHE) {
	   bool hit;
	   SQ[sq_index].miss_resolve_cycle = DC.Access(Tid, cycle, addr, true, &hit);
	   SQ[sq_index].missed = !hit;
	}
}


void lsu::store_value(unsigned int sq_index, SS_WORD_TYPE value0, SS_WORD_TYPE value1) {
	assert(SQ[sq_index].valid);

	SQ[sq_index].value_avail = true;
	SQ[sq_index].value[0] = value0;
	SQ[sq_index].value[1] = value1;
}


bool lsu::load_addr(SS_TIME_TYPE cycle,
		    unsigned int addr,
		    unsigned int lq_index,
		    unsigned int sq_index, bool sq_index_phase,
		    SS_WORD_TYPE back_data, // LWL/LWR
		    SS_WORD_TYPE& value0,
		    SS_WORD_TYPE& value1,
			unsigned int load_low_data,
			unsigned int load_up_data) {
	assert(LQ[lq_index].valid);

	// Set up information for executing the load.
	LQ[lq_index].addr_avail = true;
	LQ[lq_index].addr = addr;
	LQ[lq_index].back_data = back_data;

	if (!PERFECT_DCACHE) {
	   bool hit;
	   LQ[lq_index].miss_resolve_cycle = DC.Access(Tid, cycle, addr, true, &hit);
	   LQ[lq_index].missed = !hit;
	}

	// Run the load through the load execution datapath.
	execute_load(cycle, lq_index, sq_index, sq_index_phase, load_low_data, load_up_data);

	// If this load stalled and there are no other stalled loads,
	// set the replay_index to point to this load.
	if (!LQ[lq_index].value_avail && (replay_index < 0))
	   replay_index = lq_index;

	// Result of running the load through the load execution datapath.
	value0 = LQ[lq_index].value[0];
	value1 = LQ[lq_index].value[1];
	return(LQ[lq_index].value_avail);
}


bool lsu::load_replay(SS_TIME_TYPE cycle, unsigned int& pay_index, SS_WORD_TYPE& value0, SS_WORD_TYPE& value1) {
   bool unstalled;

   if (replay_index < 0) {
      // There aren't any stalled loads to replay.
      return(false);
   }
   else {
      // Replay the load pointed to by replay_index.
      execute_load(cycle, replay_index, LQ[replay_index].sq_index, LQ[replay_index].sq_index_phase, 0, 0);

      // Result of replaying the load.
      unstalled = LQ[replay_index].value_avail;
      pay_index = LQ[replay_index].pay_index;
      value0 = LQ[replay_index].value[0];
      value1 = LQ[replay_index].value[1];

      // Generate the next replay index.
      next_replay_index(false);

      // Return indication of whether or not a load was unstalled.
      return(unstalled);
   }
}


void lsu::execute_load(SS_TIME_TYPE cycle,
		       unsigned int lq_index,
		       unsigned int sq_index, bool sq_index_phase,
			   unsigned int load_low_data, unsigned int load_up_data) {
   bool stall_disambig;
   bool forward;
   unsigned int store_entry;
   bool cheat;
   
   assert(LQ[lq_index].valid);
   assert(LQ[lq_index].addr_avail);
   assert(!LQ[lq_index].value_avail);

   stall_disambig = disambiguate(lq_index, sq_index, sq_index_phase, forward, store_entry, cheat);

   if (stall_disambig) {
      // STATS
      LQ[lq_index].stat_load_stall_disambig = true;
   }
   else if (forward) {
      // STATS
      LQ[lq_index].stat_forward = true;

      // Forward the data from the store entry.
      switch (LQ[lq_index].size) {
	 case 1:
	    if (LQ[lq_index].is_signed)
	       LQ[lq_index].value[0] = (signed long)((signed char)(SQ[store_entry].value[0]));
	    else
	       LQ[lq_index].value[0] = (unsigned long)((unsigned char)(SQ[store_entry].value[0]));
	    break;
     
	 case 2:
	    if (LQ[lq_index].is_signed)
	       LQ[lq_index].value[0] = (signed long)((signed short)(SQ[store_entry].value[0]));
	    else
	       LQ[lq_index].value[0] = (unsigned long)((unsigned short)(SQ[store_entry].value[0]));	
	    break;

	 case 4:
	    LQ[lq_index].value[0] = SQ[store_entry].value[0];
	    break;
    
	 case 8:
	    LQ[lq_index].value[0] = SQ[store_entry].value[0];
	    LQ[lq_index].value[1] = SQ[store_entry].value[1];
	    break;

	 default:
	    assert(0);
	    break;
      }

      // The load value is now available.
      LQ[lq_index].value_avail = true;
   }
   else if (cheat == true)
   {
	   LQ[lq_index].value_avail = true;
	   
	   switch(LQ[lq_index].size)
	   {
		 case 1:
			if (LQ[lq_index].is_signed)
			   LQ[lq_index].value[0] = load_up_data;//(signed long)((signed char)(SQ[store_entry].value[0]));
			else
			   LQ[lq_index].value[0] = load_up_data; //(unsigned long)((unsigned char)(SQ[store_entry].value[0]));
			break;
		 
		 case 2:
			if (LQ[lq_index].is_signed)
			   LQ[lq_index].value[0] = load_up_data; //(signed long)((signed short)(SQ[store_entry].value[0]));
			else
			   LQ[lq_index].value[0] = load_up_data; //(unsigned long)((unsigned short)(SQ[store_entry].value[0]));	
			break;

		 case 4:
			LQ[lq_index].value[0] = load_up_data;//SQ[store_entry].value[0];
			break;
		
		 case 8:
			LQ[lq_index].value[0] = load_up_data; //SQ[store_entry].value[0];
			LQ[lq_index].value[1] = load_low_data; //SQ[store_entry].value[1];
			break;

		 default:
			assert(0);
			break;		   
	   }
	   cheat = false;
   }
   else if (!(LQ[lq_index].missed && (cycle < LQ[lq_index].miss_resolve_cycle))) {
      // Load data from memory.
      if (MEMORY_IN_BOUNDS(LQ[lq_index].addr)) {
	 switch (LQ[lq_index].size) {
	    case 1:
	       if (LQ[lq_index].is_signed)
	          LQ[lq_index].value[0] = MEM_READ_SIGNED_BYTE(LQ[lq_index].addr);
	       else
	          LQ[lq_index].value[0] = MEM_READ_UNSIGNED_BYTE(LQ[lq_index].addr);
	       break;
     
	    case 2:
	       if (LQ[lq_index].is_signed)
	          LQ[lq_index].value[0] = MEM_READ_SIGNED_HALF(LQ[lq_index].addr & (~(1)));
	       else
	          LQ[lq_index].value[0] = MEM_READ_UNSIGNED_HALF(LQ[lq_index].addr & (~(1)));
	       break;

	    case 4:
	       SS_WORD_TYPE load_data;
	       load_data = MEM_READ_WORD(LQ[lq_index].addr & (~(3)));

	       if (LQ[lq_index].left)
	          LQ[lq_index].value[0] = LWL_MACRO(LQ[lq_index].addr, load_data, LQ[lq_index].back_data);
	       else if (LQ[lq_index].right)
	          LQ[lq_index].value[0] = LWR_MACRO(LQ[lq_index].addr, load_data, LQ[lq_index].back_data);
	       else
	          LQ[lq_index].value[0] = load_data;
	       break;
    
	    case 8:
	       LQ[lq_index].value[0] = MEM_READ_WORD(LQ[lq_index].addr & (~(3)));
	       LQ[lq_index].value[1] = MEM_READ_WORD((LQ[lq_index].addr & (~(3))) + 4);
	       break;

	    default:
	       assert(0);
	       break;
	 }
      }

      // The load value is now available.
      LQ[lq_index].value_avail = true;
   }
   else {
      // STATS
      LQ[lq_index].stat_load_stall_miss = true;
   }
}


void lsu::checkpoint(unsigned int& chkpt_lq_tail, bool& chkpt_lq_tail_phase,
		     unsigned int& chkpt_sq_tail, bool& chkpt_sq_tail_phase) {
   chkpt_lq_tail = lq_tail;
   chkpt_lq_tail_phase = lq_tail_phase;
   chkpt_sq_tail = sq_tail;
   chkpt_sq_tail_phase = sq_tail_phase;
}


void lsu::restore(unsigned int recover_lq_tail, bool recover_lq_tail_phase,
		  unsigned int recover_sq_tail, bool recover_sq_tail_phase) {
   /////////////////////////////
   // Restore LQ.
   /////////////////////////////

   // Restore tail state.
   lq_tail = recover_lq_tail;
   lq_tail_phase = recover_lq_tail_phase;

   // Recompute the length.
   lq_length = MOD_S((lq_size + lq_tail - lq_head), lq_size);
   if ((lq_length == 0) && (lq_tail_phase != lq_head_phase))
      lq_length = lq_size;

   // Restore valid bits in two steps:
   // (1) Clear all valid bits.
   // (2) Set valid bits between head and tail.

   for (unsigned int i = 0; i < lq_size; i++)
      LQ[i].valid = false;

   for (unsigned int i = 0, j = lq_head; i < lq_length; i++, j = MOD_S((j+1), lq_size))
      LQ[j].valid = true;

   // Reset the replay index.
   next_replay_index(true);

   /////////////////////////////
   // Restore SQ.
   /////////////////////////////

   // Restore tail state.
   sq_tail = recover_sq_tail;
   sq_tail_phase = recover_sq_tail_phase;

   // Recompute the length.
   sq_length = MOD_S((sq_size + sq_tail - sq_head), sq_size);
   if ((sq_length == 0) && (sq_tail_phase != sq_head_phase))
      sq_length = sq_size;

   // Restore valid bits in two steps:
   // (1) Clear all valid bits.
   // (2) Set valid bits between head and tail.

   for (unsigned int i = 0; i < sq_size; i++)
      SQ[i].valid = false;

   for (unsigned int i = 0, j = sq_head; i < sq_length; i++, j = MOD_S((j+1), sq_size))
      SQ[j].valid = true;
}


void lsu::commit(bool load) {
	if (load) {
	   // LQ should not be empty.
	   assert(lq_length > 0);

	   // STATS
	   n_load++;
	   if (LQ[lq_head].stat_load_stall_disambig) n_stall_disambig++;
	   if (LQ[lq_head].stat_load_stall_miss) n_stall_miss_l++;
	   if (LQ[lq_head].stat_forward) n_forward++;

	   // Invalidate the entry.
           LQ[lq_head].valid = false;

	   // Advance the head pointer and decrement the queue length.
	   lq_head = MOD_S((lq_head + 1), lq_size);
	   lq_length--;

	   // Detect wrap-around of lq_head and toggle its phase bit accordingly.
	   if (lq_head == 0)
	      lq_head_phase = !lq_head_phase;
	}
	else {
	   // SQ should not be empty.
	   assert(sq_length > 0);

	   // STATS
	   n_store++;

	   // Commit store data.
	   switch (SQ[sq_head].size) {
     	      case 1:
	         MEM_WRITE_BYTE(SQ[sq_head].value[0], SQ[sq_head].addr);
                 break;

              case 2:
	         MEM_WRITE_HALF(SQ[sq_head].value[0], SQ[sq_head].addr);
		 break;

              case 4:
		 SS_WORD_TYPE value;

	         if (SQ[sq_head].left)
	   	    value = SWL_MACRO(SQ[sq_head].addr, SQ[sq_head].value[0], (MEM_READ_WORD(SQ[sq_head].addr & (~(3)))));
		 else if (SQ[sq_head].right)
		    value = SWR_MACRO(SQ[sq_head].addr, SQ[sq_head].value[0], (MEM_READ_WORD(SQ[sq_head].addr & (~(3)))));
		 else
		    value = SQ[sq_head].value[0];

		 MEM_WRITE_WORD(value, (SQ[sq_head].addr & (~(3))));
	 	 break;

	      case 8:
	         MEM_WRITE_WORD(SQ[sq_head].value[0], SQ[sq_head].addr);
		 MEM_WRITE_WORD(SQ[sq_head].value[1], SQ[sq_head].addr + 4);
       		 break;

	      default:
		 assert(0);
	         break;
	   }

	   // Invalidate the entry.
           SQ[sq_head].valid = false;

	   // Advance the head pointer and decrement the queue length.
	   sq_head = MOD_S((sq_head + 1), sq_size);
	   sq_length--;

	   // Detect wrap-around of sq_head and toggle its phase bit accordingly.
	   if (sq_head == 0)
	      sq_head_phase = !sq_head_phase;
        }
}

void lsu::flush() {
	   // Flush LQ.
	   lq_head = 0;
	   lq_head_phase = false;
	   lq_tail = 0;
	   lq_tail_phase = false;
	   lq_length = 0;
	   replay_index = -1;

	   for (unsigned int i = 0; i < lq_size; i++)
	      LQ[i].valid = false;

	   // Flush SQ.
	   sq_head = 0;
	   sq_head_phase = false;
	   sq_tail = 0;
	   sq_tail_phase = false;
	   sq_length = 0;

	   for (unsigned int i = 0; i < sq_size; i++)
	      SQ[i].valid = false;
}


void lsu::copy_mem(char **master_mem_table) {
   for (unsigned int i = 0; i < MEMORY_TABLE_SIZE; i++) {
      if (master_mem_table[i]) {
	 if (!mem_table[i])
	    mem_table[i] = mem_newblock();
	 for (unsigned int j = 0; j < MEMORY_BLOCK_SIZE; j++)
	    mem_table[i][j] = master_mem_table[i][j];
      }
      else {
	 if (mem_table[i]) {
	    free(mem_table[i]);
	    mem_table[i] = (char *)NULL;
	 }
      }
   }
}


// STATS
void lsu::stats(FILE *fp) {
   fprintf(fp, "----memory system results----\n");

   fprintf(fp, "LOADS (retired)\n");
   fprintf(fp, "  loads            = %d\n", n_load);
   fprintf(fp, "  disambig. stall  = %d (%.2f%%)\n",
		n_stall_disambig,
		100.0*(double)n_stall_disambig/(double)n_load);
   fprintf(fp, "  forward          = %d (%.2f%%)\n",
		n_forward,
		100.0*(double)n_forward/(double)n_load);
   fprintf(fp, "  miss stall       = %d (%.2f%%)\n",
		n_stall_miss_l,
		100.0*(double)n_stall_miss_l/(double)n_load);

   fprintf(fp, "STORES (retired)\n");
   fprintf(fp, "  stores           = %d\n", n_store);
   fprintf(fp, "  miss stall       = %d (%.2f%%)\n",
		n_stall_miss_s,
		100.0*(double)n_stall_miss_s/(double)n_store);

   fprintf(fp, "----memory system results----\n");
}


///////////////////////////////////////////////////////////////////////////

char *lsu::mem_newblock(void) {
   char *p = new char[MEMORY_BLOCK_SIZE];

	 for (unsigned int i = 0; i < MEMORY_BLOCK_SIZE; i++) {
	 	  p[i] = (char)0;
	 }
 	 return(p);
   //return(new char[MEMORY_BLOCK_SIZE]);
   //return((char *) getcore(MEMORY_BLOCK_SIZE));
}

///////////////////////////////////////////////////////////////////////////
