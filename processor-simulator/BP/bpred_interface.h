#ifndef _bpred_interface_h
#define _bpred_interface_h
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Other files assumed to be included before this file:
//
//	ss.h
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include "bpred_interface.params"

#include "bpred_predictor_struct.h"

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// CLASS FOR INTERFACE
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
class bpred_interface
{

private:

#include "bpred_interface.vars"


public:

    
//-------------------------------------------------------------------
//
// Stat variables
//
//-------------------------------------------------------------------

unsigned int stat_num_pred;
unsigned int stat_num_miss;
unsigned int stat_num_cond_pred;
unsigned int stat_num_cond_miss;
unsigned int stat_num_flush_RAS;
unsigned int stat_num_conf_corr;
unsigned int stat_num_conf_ncorr;
unsigned int stat_num_nconf_corr;
unsigned int stat_num_nconf_ncorr;


//-------------------------------------------------------------------
//
// Constructor / Destructor
//
//-------------------------------------------------------------------


    //
    // Constructor
    //
    // Initialize control flow prediction stuff.
    //
    bpred_interface();

    
    //
    // Destructor
    //
    // Generic destructor
    //
    ~bpred_interface();
    

//-------------------------------------------------------------------
//
// Functions for prediction
//
//-------------------------------------------------------------------
    
    //
    // 	get_pred()
    //
    // Get the next prediction.
    // Returns the predicted target, and also assigns a tag to the prediction,
    // which is used as a kind of sequence number by 'update_pred'.
    //
    unsigned int get_pred(unsigned int branch_history,
			unsigned int PC, SS_INST_TYPE inst,
			unsigned int comp_target, unsigned int *pred_tag);
    unsigned int get_pred(unsigned int branch_history,
			unsigned int PC, SS_INST_TYPE inst,
			unsigned int comp_target, unsigned int *pred_tag,
			bool *conf,
			bool *fm);	// "FM"

    void set_nconf(unsigned int pred_tag);
    //
    //	    fix_pred()
    //
    // Tells the predictor that the instruciton has executed and was found
    // to be mispredicted.  This backs up the history to the point after the
    // misprediction.
    //
    // The prediction is identified via a tag, originally gotten from get_pred.
    //
    void fix_pred(unsigned int pred_tag, unsigned int next_pc);

    //
    //      verify_pred()
    //
    // Tells the predictor to go ahead and update state for a given prediction.
    //
    // The prediction is identified via a tag, originally gotten from get_pred.
    //
    void verify_pred(unsigned int pred_tag, unsigned int next_pc,
		     bool fm);		// "FM"


    //  flush()
    //
    // flush pending predictions
    //
    void flush();

    //
    // Dump stats
    //
    void dump_stats(FILE *fp);

    unsigned int update_global_history(unsigned int ghistory, 
				   SS_INST_TYPE inst, unsigned int pc, 
				   unsigned int next_pc);
    
}; // end of bpred_interface class definition
#endif
