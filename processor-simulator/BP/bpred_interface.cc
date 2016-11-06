#include <stdio.h>
#include <assert.h>
#include "ss.h"
#include "parameters.h"
#include "bpred_interface.h"

//unsigned int HIST_MASK = 0xfffc;
//unsigned int HIST_BIT  = 0x8000;
//unsigned int PC_MASK   = 0x3fff;


//
// Constructor
//
// Initialize control flow prediction stuff.
//
bpred_interface::bpred_interface()
{
    int i;

    BTB = new BpredPredictAutomaton[BTB_SIZE];
    pred_table = new BpredPredictAutomaton[TABLE_SIZE];
    conf_table = new BpredPredictAutomaton[TABLE_SIZE];
    fm_table = new BpredPredictAutomaton[TABLE_SIZE];		// "FM"

    cti_head = 0;
    cti_tail = 0;
    for (i = 0; i < MAX_Q_DEPTH_b; i++)
    {
	cti_Q[i].RAS_action = 0;
	cti_Q[i].history = 0;
	cti_Q[i].state = 0;
    }

    for (i = 0; i < BTB_SIZE; i++)
    {
	BTB[i].tag = 0;
	BTB[i].pred = 0;
	BTB[i].hyst = 0;
    }

    for (i = 0; i < TABLE_SIZE; i++)
    {
	pred_table[i].tag = 0;
	pred_table[i].pred = 1;
	pred_table[i].hyst = 0;
	conf_table[i].pred = CONF_MAX;
	fm_table[i].pred = FM_MAX;				// "FM"
    }

    RAS = NULL;

    stat_num_pred = 0;
    stat_num_miss = 0;
    stat_num_cond_pred = 0;
    stat_num_cond_miss = 0;
    stat_num_flush_RAS = 0;
    stat_num_conf_corr = 0;
    stat_num_conf_ncorr = 0;
    stat_num_nconf_corr = 0;
    stat_num_nconf_ncorr = 0;
}
    
//
// Destructor
//
// Generic destructor
//
bpred_interface::~bpred_interface()
{
    if (RAS)
	delete RAS;
    RAS = NULL;
}
    

//-------------------------------------------------------------------
//
// Internal functions
//
//-------------------------------------------------------------------

void bpred_interface::update()
{
    u_int	current;
    u_int	new_tail;

    //
    // Update pointers
    //
    current = cti_tail;
    new_tail = (current + 1) & (MAX_Q_MASK_b);
    cti_tail = new_tail;

    //
    // Update history;
    //
    if (cti_Q[current].is_cond)
    {
	if (cti_Q[current].taken)
            cti_Q[new_tail].history = (cti_Q[current].history >> 1) | HIST_BIT;
	else
            cti_Q[new_tail].history = (cti_Q[current].history >> 1);
    }
    else
    {
        cti_Q[new_tail].history = cti_Q[current].history;
    }
}

void bpred_interface::RAS_update()
{
    RAS_node_b *ptr;

    if (cti_Q[cti_head].flush_RAS)
    {
	if (RAS)
	    delete (RAS);
	RAS = NULL;
	return;
    }

    if (cti_Q[cti_head].RAS_action == -1)
    {
	//
	// POP
	//
	assert (RAS);
	ptr = RAS;
	RAS = RAS->next;

	assert (ptr->addr == cti_Q[cti_head].RAS_address);

	ptr->next = NULL;
	delete ptr;
    }
    else if (cti_Q[cti_head].RAS_action == 1)
    {
	//
	// PUSH
	//
	ptr = new RAS_node_b();
	ptr->addr = cti_Q[cti_head].RAS_address;
	ptr->next = RAS;
	RAS = ptr;
    }
}

bool bpred_interface::RAS_lookup(u_int *target)
{
    int count = 0;
    u_int index;
    u_int exit = (cti_head - 1) & MAX_Q_MASK_b;

    for (index = (cti_tail - 1) & MAX_Q_MASK_b; 
	 index != exit; 
	 index = (index - 1) & MAX_Q_MASK_b)
    {
	count += cti_Q[index].RAS_action;
	if (cti_Q[index].flush_RAS)
	{
	    *target = 0;
	    return (false);
	}
	if (count == 1)
	{
	    *target = (cti_Q[index].RAS_address);
	    return (true);
	}
    }

    RAS_node_b *ptr = RAS; 
    for ( ; count < 0; count++)
    {
	if (ptr == NULL) break;
	ptr = ptr->next;
    }

    if (ptr)
    {
	*target = ptr->addr;
	return (true);
    }
    else
    {
	*target = 0;
	return (false);
    }
    
}

void bpred_interface::update_predictions(bool fm)		// "FM"
{
    u_int	conf_index;
    u_int	pred_index;
    u_int	btb_index;
    u_int	history;

    //
    // Update prediction
    //
    if (cti_Q[cti_head].is_cond)
    {
	pred_index = ((cti_Q[cti_head].history & HIST_MASK) ^
		((cti_Q[cti_head].pc / SS_INST_SIZE) & PC_MASK)) & INDEX_MASK;
	pred_table[pred_index].update((u_int)cti_Q[cti_head].taken);

	//
	// update conf
	//
	history = cti_Q[cti_head].history;
	if (cti_Q[cti_head].original_pred != cti_Q[cti_head].pc + 8)
	    history = (history >> 1) | HIST_BIT;
	else
	    history = (history >> 1);
        
	conf_index = ((history & HIST_MASK) ^ 
		((cti_Q[cti_head].pc / SS_INST_SIZE) & PC_MASK)) & INDEX_MASK;
	bool corr = (cti_Q[cti_head].target == cti_Q[cti_head].original_pred);

	conf_table[conf_index].conf_update((u_int)corr, CONF_RESET, CONF_MAX);

	//
	// "FM": update conf
	//
	fm_table[pred_index].conf_update((u_int)!fm, FM_RESET, FM_MAX);

    }


    //
    // update BTB
    //
    if (cti_Q[cti_head].use_BTB)
    {
        btb_index = (cti_Q[cti_head].pc / SS_INST_SIZE) & BTB_MASK;
	BTB[btb_index].update(cti_Q[cti_head].target);
    }

}



void bpred_interface::make_predictions(unsigned int branch_history)
{
    u_int	conf_index;
    u_int	pred_index;
    u_int	btb_index;
    u_int	temp_target;
    u_int	history;

    //
    // Predict if cti is "taken"
    //
    if (cti_Q[cti_tail].is_cond)
    {
	if (branch_history == 0xFFFFFFFF)
	{
	    history = cti_Q[cti_tail].history;
	    pred_index = ((cti_Q[cti_tail].history & HIST_MASK) ^
		((cti_Q[cti_tail].pc / SS_INST_SIZE) & PC_MASK)) & INDEX_MASK;
	}
	else
	{
	    history = branch_history;
	    pred_index = ((branch_history & HIST_MASK) ^
		((cti_Q[cti_tail].pc / SS_INST_SIZE) & PC_MASK)) & INDEX_MASK;
	    cti_Q[cti_tail].use_global_history = true;
	    cti_Q[cti_tail].global_history = branch_history;
	}
	    
	cti_Q[cti_tail].taken = pred_table[pred_index].pred;

	if (cti_Q[cti_tail].taken)
	    history = (history >> 1) | HIST_BIT;
	else
	    history = (history >> 1);
        
	conf_index = ((history & HIST_MASK) ^ 
		((cti_Q[cti_tail].pc / SS_INST_SIZE) & PC_MASK)) & INDEX_MASK;
	cti_Q[cti_tail].conf = conf_table[conf_index].pred;

	cti_Q[cti_tail].fm = fm_table[pred_index].pred;		// "FM"
    }
    else
    {
	cti_Q[cti_tail].taken = true;
	cti_Q[cti_tail].conf = 0xff;
	cti_Q[cti_tail].fm = 0xff;				// "FM"
    }

    //
    // Predict cti target
    //
    if (cti_Q[cti_tail].is_return)
    {
	//
	// Use RAS
	//
	if (RAS_lookup(&temp_target))
	{
	    cti_Q[cti_tail].target = temp_target;
	    cti_Q[cti_tail].RAS_action = -1;
	    cti_Q[cti_tail].RAS_address = temp_target;
	}
	else
	{
	    cti_Q[cti_tail].target = 0;
	    cti_Q[cti_tail].conf = 0;
	}
    }
    else
    {
	if (cti_Q[cti_tail].taken)
	{
	    //
	    // use BTB
	    //
	    if (cti_Q[cti_tail].use_BTB)
	    {
	        btb_index = (cti_Q[cti_tail].pc / SS_INST_SIZE) & BTB_MASK;
	        cti_Q[cti_tail].target = BTB[btb_index].pred;
	    }
	    else
	    {
	        cti_Q[cti_tail].target = cti_Q[cti_tail].comp_target;
	    }
	}
	else
	{
	    //
	    // (pc + SS_INST_SIZE) prediction
	    //
	    cti_Q[cti_tail].target = cti_Q[cti_tail].pc + SS_INST_SIZE;
	}
    }

    //
    // RAS update for calls
    //
    if (cti_Q[cti_tail].is_call)
    {
	cti_Q[cti_tail].RAS_action = 1;
	cti_Q[cti_tail].RAS_address = cti_Q[cti_tail].pc + SS_INST_SIZE;
    }
}

void bpred_interface::decode()
{
    SS_INST_TYPE	inst;
    inst = cti_Q[cti_tail].inst;

    cti_Q[cti_tail].use_BTB = false;
    cti_Q[cti_tail].is_cond = false;
    cti_Q[cti_tail].is_call = false;
    cti_Q[cti_tail].is_return = false;

    switch (SS_OPCODE(inst))
    {
        case JUMP:
            break;

        case JAL:
	    cti_Q[cti_tail].is_call = true;
	    break;

        case JALR:
	    cti_Q[cti_tail].use_BTB = true;
	    cti_Q[cti_tail].is_call = true;
	    break;

        case JR:
            if ( (RS) == 31 )
            {
    		cti_Q[cti_tail].is_return = true;
	    }
	    else
	    {
	        cti_Q[cti_tail].use_BTB = true;
	    }
	    break;

        case BEQ : case BNE : case BLEZ: case BGTZ:
	case BLTZ: case BGEZ: case BC1F: case BC1T:
	    cti_Q[cti_tail].is_cond = true;
	    break;

	default:
	    break;
    }
}
    
//-------------------------------------------------------------------
//
// Interface functions
//
//-------------------------------------------------------------------
    
//
// 	get_pred()
//
// Get the next prediction.
// Returns the predicted target, and also assigns a tag to the prediction,
// which is used as a kind of sequence number by 'update_pred'.
//
unsigned int bpred_interface::get_pred(unsigned int branch_history, 
			unsigned int PC, SS_INST_TYPE inst,
			unsigned int comp_target, unsigned int *pred_tag)
{

    unsigned int tag;

    tag = cti_tail;

    cti_Q[cti_tail].RAS_action = 0;
    cti_Q[cti_tail].RAS_address = 0;
    cti_Q[cti_tail].flush_RAS = false;
    cti_Q[cti_tail].pc = PC;
    cti_Q[cti_tail].inst = inst;
    cti_Q[cti_tail].comp_target = comp_target;
    cti_Q[cti_tail].state = 1;
    cti_Q[cti_tail].is_conf = true;
    cti_Q[cti_tail].use_global_history = false;

    decode();
    make_predictions(branch_history);
    cti_Q[cti_tail].original_pred = cti_Q[cti_tail].target;
    update();

    *pred_tag = tag;
    return (cti_Q[tag].target);

}
unsigned int bpred_interface::get_pred(unsigned int branch_history,
			unsigned int PC, SS_INST_TYPE inst,
			unsigned int comp_target, unsigned int *pred_tag,
			bool *conf,
			bool *fm)				// "FM"
{

    unsigned int tag;

    tag = cti_tail;

    cti_Q[cti_tail].RAS_action = 0;
    cti_Q[cti_tail].RAS_address = 0;
    cti_Q[cti_tail].flush_RAS = false;
    cti_Q[cti_tail].pc = PC;
    cti_Q[cti_tail].inst = inst;
    cti_Q[cti_tail].comp_target = comp_target;
    cti_Q[cti_tail].state = 1;
    cti_Q[cti_tail].is_conf = true;
    cti_Q[cti_tail].use_global_history = false;

    decode();
    make_predictions(branch_history);

    cti_Q[cti_tail].original_pred = cti_Q[cti_tail].target;
    if (cti_Q[cti_tail].conf > CONF_THRESHOLD)
	*conf = true;
    else
	*conf = false;

    // "FM"
    if (cti_Q[cti_tail].fm > FM_THRESHOLD)
	*fm = false;
    else
	*fm = true;

    update();

    *pred_tag = tag;
    return (cti_Q[tag].target);

}

void bpred_interface::set_nconf(unsigned int pred_tag)
{
    cti_Q[pred_tag].is_conf = false;
}

//
//	    fix_pred()
//
// Tells the predictor that the instruciton has executed and was found
// to be mispredicted.  This backs up the history to the point after the
// misprediction.
//
// The prediction is identified via a tag, originally gotten from get_pred.
//
void bpred_interface::fix_pred(unsigned int pred_tag, unsigned int next_pc)
{
    u_int temp_target;

    cti_tail = pred_tag;

    if (cti_Q[cti_tail].is_cond)
    {
        bool taken;
        taken = (next_pc != (cti_Q[cti_tail].pc + SS_INST_SIZE));
	cti_Q[cti_tail].taken = taken;
    }

    cti_Q[cti_tail].target = next_pc;

    if (cti_Q[cti_tail].is_return)
    {
	if ((RAS_lookup(&temp_target)) && (temp_target == next_pc))
	{
	    cti_Q[cti_tail].RAS_action = -1;
	    cti_Q[cti_tail].RAS_address = next_pc;
	    cti_Q[cti_tail].flush_RAS = false;
	}
	else
	{
	    cti_Q[cti_tail].RAS_action = 0;
	    cti_Q[cti_tail].flush_RAS = true;
	}
    }

    update();
}


//
//      verify_pred()
//
// Tells the predictor to go ahead and update state for a given prediction.
//
// The prediction is identified via a tag, originally gotten from get_pred.
//
void bpred_interface::verify_pred(unsigned int pred_tag, unsigned int next_pc,
				  bool fm)			// "FM"
{
    assert (pred_tag == cti_head);
    assert (cti_Q[cti_head].state == 1);
    //assert (next_pc = cti_Q[cti_head].target);
    assert (next_pc == cti_Q[cti_head].target);
    assert (!cti_Q[cti_head].use_global_history ||
            (cti_Q[cti_head].global_history == cti_Q[cti_head].history));

    //
    // Update things
    //
    update_predictions(fm);					// "FM"
    RAS_update();

    //
    // STATS
    //
    stat_num_pred++;
    bool conf = cti_Q[cti_head].conf > CONF_THRESHOLD;
    //bool conf = cti_Q[cti_head].is_conf;
    if (cti_Q[cti_head].original_pred != next_pc)
    {
	stat_num_miss++;
	if (conf)
	{
	    //printf("%08x (%08x) pred:%08x act:%08x\n", cti_Q[cti_head].pc,
	    //    cti_Q[(cti_head - 1) & MAX_Q_MASK_b].target,
	    //	cti_Q[cti_head].original_pred,
	    //	cti_Q[cti_head].target);
	    stat_num_conf_ncorr++;
	}
	else
	    stat_num_nconf_ncorr++;
    }
    else
    {
	if (conf)
	    stat_num_conf_corr++;
	else
	    stat_num_nconf_corr++;
    }
    if (cti_Q[cti_head].flush_RAS)
	stat_num_flush_RAS++;
    if (cti_Q[cti_head].is_cond)
    {
        stat_num_cond_pred++;
        if (cti_Q[cti_head].original_pred != next_pc)
	    stat_num_cond_miss++;
    }

    //
    // Clear state
    //
    cti_Q[cti_head].RAS_action = 0;
    cti_Q[cti_head].RAS_address = 0;
    cti_Q[cti_head].flush_RAS = false;
    cti_Q[cti_head].pc = 0;
    cti_Q[cti_head].comp_target = 0;
    cti_Q[cti_head].state = 0;

    cti_head = (cti_head + 1) & MAX_Q_MASK_b;
}

//
//	flush pending predictions
//
void bpred_interface::flush()
{
    cti_tail = cti_head;
}

//
// Dump stats
//
void bpred_interface::dump_stats(FILE *fp)
{
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "\tBTB_SIZE   = %d\n", BTB_SIZE);
    fprintf(fp, "\tBTB_MASK   = %x (%d)\n", BTB_MASK, BTB_MASK);
    fprintf(fp, "\tTABLE_SIZE = %d\n", TABLE_SIZE);
    fprintf(fp, "\tINDEX_MASK = %x (%d)\n", INDEX_MASK, INDEX_MASK);
    fprintf(fp, "\tHIST_MASK  = %x (%d)\n", HIST_MASK, HIST_MASK);
    fprintf(fp, "\tHIST_BIT   = %x (%d)\n", HIST_BIT, HIST_BIT);
    fprintf(fp, "\tPC_MASK    = %x (%d)\n", PC_MASK, PC_MASK);
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "bpred: ALL CTI\n");
    fprintf(fp, "bpred:     %d / %d = %f\n", stat_num_miss, stat_num_pred,
				(float)stat_num_miss / (float)stat_num_pred);
    fprintf(fp, "bpred: \n");
    fprintf(fp, "bpred: CONDITIONAL\n");
    fprintf(fp, "bpred:     %d / %d = %f\n", stat_num_cond_miss, 
				stat_num_cond_pred,
				(float)stat_num_cond_miss / 
				(float)stat_num_cond_pred);
    fprintf(fp, "bpred: \n");
    fprintf(fp, "bpred: RAS flushes = %d\n",stat_num_flush_RAS);
    fprintf(fp, "bpred: \n");
    fprintf(fp, "bpred: conf_corr    = %d (%.3f)\n", stat_num_conf_corr,
    		(double)stat_num_conf_corr / (double)stat_num_pred);
    fprintf(fp, "bpred: conf_ncorr   = %d (%.3f)\n", stat_num_conf_ncorr,
    		(double)stat_num_conf_ncorr / (double)stat_num_pred);
    fprintf(fp, "bpred: nconf_corr   = %d (%.3f)\n", stat_num_nconf_corr,
    		(double)stat_num_nconf_corr / (double)stat_num_pred);
    fprintf(fp, "bpred: nconf_ncorr  = %d (%.3f)\n", stat_num_nconf_ncorr,
    		(double)stat_num_nconf_ncorr / (double)stat_num_pred);
    fprintf(fp, "----------------------------------------------------------\n");
}

unsigned int bpred_interface::update_global_history(unsigned int ghistory, 
				   SS_INST_TYPE inst, unsigned int pc, 
				   unsigned int next_pc)
{
    switch (SS_OPCODE(inst))
    {
        case BEQ : case BNE : case BLEZ: case BGTZ:
	case BLTZ: case BGEZ: case BC1F: case BC1T:
	    if (next_pc != pc + SS_INST_SIZE) // if taken
                ghistory = (ghistory >> 1) | HIST_BIT;
	    else
                ghistory = (ghistory >> 1);
	    return (ghistory);
	    break;
	default:
	    return (ghistory);
	    break;
    }
}
