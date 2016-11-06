#ifndef RENAMER_INCLUDED
#define RENAMER_INCLUDED

#include "trace_payload.h"
#include "trace_predictor.h"
#include <iostream>
#include <assert.h>

//Global free list class
class FList
{
public:
	unsigned int *FL;
	int *valid;
	int head;
	int tail;
	unsigned int FL_size;
	unsigned int logical;

	//Constructor
	FList(unsigned int size, unsigned int logs);

	//Functions
	void initialize();
	unsigned int popFL();	//Pop the FL head.
	void pushFL(unsigned int reg);	//Push at tail of FL.
	bool getFLStatus();	//True means FL is full.
	int getFLentries();	//Get number of currently free registers.
	void restoreFL();
	void setFLValid();
};

// Trace checkpoints
// Checkpointed after renaming every trace.
typedef struct
{
	unsigned int *mapTable;
	unsigned int FLhead;
	//Trace predictor history
	unsigned int *tp_hist;
} checkpoints;

class renamer {
public:
	/////////////////////////////////////////////////////////////////////
	// Put private class variables here.
	/////////////////////////////////////////////////////////////////////
	unsigned int logRegs;	
	unsigned int phyRegs;

	/////////////////////////////////////////////////////////////////////
	// Structure 1: Rename Map Table
	// Entry contains: physical register mapping
	/////////////////////////////////////////////////////////////////////
	unsigned int *RMT;

	/////////////////////////////////////////////////////////////////////
	// Structure 2: Architectural Map Table
	// Entry contains: physical register mapping
	/////////////////////////////////////////////////////////////////////
	unsigned int *AMT;

	/////////////////////////////////////////////////////////////////////
	// Structure 3: Free List
	//
	// Entry contains: physical register number
	//
	// Notes:
	// * Structure includes head, tail, and possibly other variables
	//   depending on your implementation.
	/////////////////////////////////////////////////////////////////////
	FList *freeList;

	// +Parth
	// Active list is now moved outside the renamer class, since we need only global active list.
	// Active list is created in the main processor class.
	//AList *activeList;

	/////////////////////////////////////////////////////////////////////
	// Structure 5: Global Physical Register File
	// Entry contains: value
	//
	// Notes:
	// * The value must be of the following type: unsigned long long
	/////////////////////////////////////////////////////////////////////
	unsigned long long *GRF;

	/////////////////////////////////////////////////////////////////////
	// Structure 6: Global Physical Register File Ready Bit Array
	// Entry contains: ready bit
	/////////////////////////////////////////////////////////////////////
	int *GRF_ready;

	// +Parth
	//Branch mask is no longer required, since branches are handled differently.
	//unsigned long long GBM;

	/////////////////////////////////////////////////////////////////////
	// Structure 8: Checkpoints
	// Note: In TP, checkpoints are created after renaming every trace (after trace_rename stage that is).
	//
	// Each checkpoint contains the following:
	// 1. Shadow Map Table (checkpointed Rename Map Table)
	// 2. checkpointed Free List head index
	// 3. checkpointed trace predictor history state
	/////////////////////////////////////////////////////////////////////
	checkpoints *CP;

	/////////////////////////////////////////////////////////////////////
	// Private functions.
	// e.g., a generic function to copy state from one map to another.
	/////////////////////////////////////////////////////////////////////

	//Restore RMT by copying the AMT to the RMT
	void restoreRMT();

public:
	////////////////////////////////////////
	// Public functions.
	////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// This is the constructor function.
	// When a renamer object is instantiated, the caller indicates:
	// 1. The number of logical registers (e.g., 32).
	// 2. The number of physical registers (e.g., 128).
	//
	//
	// Tips:
	//
	// Assert the number of physical registers > number logical registers.
	// Then, allocate space for the primary data structures.
	// Then, initialize the data structures based on the knowledge
	// that the pipeline is intially empty (no in-flight instructions yet).
	/////////////////////////////////////////////////////////////////////
	renamer(unsigned int n_log_regs,
		unsigned int n_phys_regs);

	/////////////////////////////////////////////////////////////////////
	// This is the destructor, used to clean up memory space and
	// other things when simulation is done.
	// I typically don't use a destructor; you have the option to keep
	// this function empty.
	/////////////////////////////////////////////////////////////////////
	~renamer();

	//////////////////////////////////////////
	// Functions related to Rename Stage.   //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// The Rename Stage must stall if there aren't enough free physical
	// registers available for renaming all logical live-out destination registers
	// in the current rename trace.
	//
	// Inputs:
	// 1. bundle_dst: number of logical live-out destination registers in
	//    current rename bundle
	//
	// Return value:
	// Return "true" (stall) if there aren't enough free physical
	// registers to allocate to all of the logical live-out destination registers
	// in the current rename bundle.
	/////////////////////////////////////////////////////////////////////
	bool stall_reg(unsigned int bundle_dst);

	/////////////////////////////////////////////////////////////////////
	// NOT REQUIRED.
	/////////////////////////////////////////////////////////////////////
	//bool stall_branch(unsigned int bundle_branch);

	/////////////////////////////////////////////////////////////////////
	// This function is used to get the branch mask for an instruction.
	/////////////////////////////////////////////////////////////////////
	//unsigned long long get_branch_mask();

	/////////////////////////////////////////////////////////////////////
	// This function is used to rename a single source register.
	//
	// Inputs:
	// 1. log_reg: the logical register to rename
	//
	// Return value: physical register name
	/////////////////////////////////////////////////////////////////////
	unsigned int rename_rsrc(unsigned int log_reg);

	/////////////////////////////////////////////////////////////////////
	// This function is used to rename a single destination register.
	//
	// Inputs:
	// 1. log_reg: the logical register to rename
	//
	// Return value: physical register name
	/////////////////////////////////////////////////////////////////////
	unsigned int rename_rdst(unsigned int log_reg);

	/////////////////////////////////////////////////////////////////////
	// This function creates a new checkpoint.
	//
	// Inputs: none.
	//
	// Output:
	// 1. The function returns the checkpoint ID for this trace. The checkpoint
	// is freed when the trace is retired from the active list.
	//
	// Tips:
	// 
	// The checkpoint should contain the following:
	// 1. Shadow Map Table (checkpointed Rename Map Table)
	// 2. checkpointed Free List head index
	// 3. checkpointed trace predictor history
	/////////////////////////////////////////////////////////////////////
	unsigned int checkpoint(trace_payload &TR_PAY, predictor &TP);

	//////////////////////////////////////////
	// Functions related to Dispatch Stage. //
	//////////////////////////////////////////

	// NOT REQUIRED.
	/////////////////////////////////////////////////////////////////////
	// bool stall_dispatch(unsigned int bundle_inst);

	//////////////////////////////////////////
	// Functions related to Schedule Stage. //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Test the ready bit of the indicated physical register.
	// Returns 'true' if ready.
	/////////////////////////////////////////////////////////////////////
	bool is_ready(unsigned int phys_reg);

	/////////////////////////////////////////////////////////////////////
	// Clear the ready bit of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void clear_ready(unsigned int phys_reg);

	/////////////////////////////////////////////////////////////////////
	// Set the ready bit of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void set_ready(unsigned int phys_reg);


	//////////////////////////////////////////
	// Functions related to Reg. Read Stage.//
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Return the contents (value) of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	unsigned long long read(unsigned int phys_regs);

	//////////////////////////////////////////
	// Functions related to Writeback Stage.//
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Write a value into the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void write(unsigned int phys_reg, unsigned long long value);

	// +Parth
	// This functions copies the shadow mapTable to the RMT
	// Also rolls back the FL head index and predictor history
	void resolve(unsigned int ID, predictor &TP);

	//////////////////////////////////////////
	// Functions related to Retire Stage.   //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// This function attempts to commit the instructions which are
	// present in the head trace of AL.
	//
	// void commit(bool &committed, bool &load, bool &store, bool &branch,
	//	    bool &exception, unsigned int &offending_PC);
	
};
#endif
