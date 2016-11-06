////////////////////////////////////////////////////////////////////////////
//
// STRUCTURES and CLASSES
//
////////////////////////////////////////////////////////////////////////////
 
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned int  u_int;


class BpredPredictAutomaton
{
  public:
    u_int			tag;
    u_int                      	pred;
    u_char			hyst;

  BpredPredictAutomaton()
  {
    tag = 0;
    hyst = 0;
    pred = 0;
  }

  ~BpredPredictAutomaton()
  {
  }

  void update(u_int outcome)
  {
    if (outcome == pred)
    {
	if (hyst < 1)
	    hyst++;
    }
    else
    {
	if (hyst)
	    hyst--;
	else
	    pred = outcome;
    }
  }

  void conf_update(u_int outcome, bool use_reset, u_int max_value)
  {
    if (outcome)
    {
        if (pred < max_value)
	    pred++;
    }
    else
    {
	if (use_reset)
	    pred = 0;
        else if (pred)
	    pred--;
    }
  }

};


class RAS_node_b
{
  public:
    u_int		addr;
    RAS_node_b		*next;

    RAS_node_b()
    {
	next = NULL;
    }

    ~RAS_node_b()
    {
	if (next)
	    delete next;
    }
};

class CTI_entry_b
{
  public:
    int				RAS_action; // 1 push, -1 pop, 0 nothing
    u_int			RAS_address;
    bool			flush_RAS;
    u_int			history;

    u_int			pc;
    SS_INST_TYPE		inst;
    u_int			comp_target;
    u_int			original_pred;
    u_int			target;
    bool			taken;

    bool			use_BTB;
    bool			is_return;
    bool			is_call;
    bool			is_cond;

    u_int			state;
    u_int			conf;
    u_int			fm;		// "FM"
    bool			is_conf;

    bool			use_global_history;
    u_int			global_history;
};
