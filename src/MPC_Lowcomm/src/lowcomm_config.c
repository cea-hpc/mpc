#include "lowcomm_config.h"

#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>
#include <dlfcn.h>

#include "coll.h"

#include <sctk_alloc.h>

#include "sctk_topological_polling.h"


/***********
 * HELPERS *
 ***********/

mpc_conf_config_type_t * __get_type_by_name ( char * prefix, char *name )
{
	char path[512];
	snprintf(path, 512, "%s.%s", prefix, name);
	mpc_conf_config_type_elem_t *elem = mpc_conf_root_config_get(path);

	if(!elem)
	{
		mpc_common_debug_error("Could not retrieve %s\n", path);
        return NULL;
	}

	return mpc_conf_config_type_elem_get_inner(elem);
}


/*******************
* COLLECTIVE CONF *
*******************/

static struct _mpc_lowcomm_coll_conf __coll_conf;

struct _mpc_lowcomm_coll_conf *_mpc_lowcomm_coll_conf_get(void)
{
	return &__coll_conf;
}

static void __mpc_lowcomm_coll_conf_set_default(void)
{
#ifdef SCTK_USE_CHECKSUM
	__coll_conf.checksum = 0;
#endif

	snprintf(__coll_conf.algorithm, MPC_CONF_STRING_SIZE, "noalloc");
	__coll_conf.mpc_lowcomm_coll_init_hook = NULL;

	/* Barrier */
	__coll_conf.barrier_arity = 8;

	/* Bcast */
	__coll_conf.bcast_max_size        = 1024;
	__coll_conf.bcast_max_arity       = 32;
	__coll_conf.bcast_check_threshold = 512;

	/* Allreduce */
	__coll_conf.allreduce_max_size        = 4096;
	__coll_conf.allreduce_max_arity       = 8;
	__coll_conf.allreduce_check_threshold = 8192;
	__coll_conf.allreduce_max_slots       = 65536;

	/* SHM */
	__coll_conf.shm_reduce_interleave       = 16;
	__coll_conf.shm_bcast_interleave        = 16;
	__coll_conf.shm_reduce_pipelined_blocks = 16;
}

static mpc_conf_config_type_t *__mpc_lowcomm_coll_conf_init(void)
{
	__mpc_lowcomm_coll_conf_set_default();

	mpc_conf_config_type_t *interleave = mpc_conf_config_type_init("interleave",
	                                                               PARAM("reduce", &__coll_conf.shm_reduce_interleave, MPC_CONF_INT, "Number of overallping SHM reduce"),
	                                                               PARAM("bcast", &__coll_conf.shm_bcast_interleave, MPC_CONF_INT, "Number of overallping SHM bcast"),
	                                                               NULL);

	mpc_conf_config_type_t *shm = mpc_conf_config_type_init("shm",
	                                                        PARAM("reducepipeline", &__coll_conf.shm_reduce_pipelined_blocks, MPC_CONF_INT, "Number of blocks in the SHM reduce pipeline"),
	                                                        PARAM("interleave", interleave, MPC_CONF_TYPE, "Interleave configuration"),
	                                                        NULL);


	mpc_conf_config_type_t *barrier = mpc_conf_config_type_init("barrier",
	                                                            PARAM("arity", &__coll_conf.barrier_arity, MPC_CONF_INT, "Arity of the lowcomm barrier"),
	                                                            NULL);

	mpc_conf_config_type_t *bcast = mpc_conf_config_type_init("bcast",
	                                                          PARAM("maxsize", &__coll_conf.bcast_max_size, MPC_CONF_LONG_INT, "Maximum outoing size per rank for a bcast defines arity (max_size / size)"),
	                                                          PARAM("arity", &__coll_conf.bcast_max_arity, MPC_CONF_INT, "Maximum arity  for a bcast"),
	                                                          PARAM("check", &__coll_conf.bcast_check_threshold, MPC_CONF_LONG_INT, "Maximum size for checking messages immediately for copy"),
	                                                          NULL);

	mpc_conf_config_type_t *allreduce = mpc_conf_config_type_init("allreduce",
	                                                              PARAM("maxsize", &__coll_conf.allreduce_max_size, MPC_CONF_LONG_INT, "Maximum outoing size per rank for allreduce defines arity (max_size / size)"),
	                                                              PARAM("arity", &__coll_conf.allreduce_max_arity, MPC_CONF_INT, "Maximum arity  for allreduce"),
	                                                              PARAM("check", &__coll_conf.allreduce_check_threshold, MPC_CONF_LONG_INT, "Maximum size for checking messages immediately for copy"),
	                                                              PARAM("maxslots", &__coll_conf.allreduce_max_slots, MPC_CONF_INT, "Maximum number of of slots for allreduce"),
	                                                              NULL);

	mpc_conf_config_type_t *coll = mpc_conf_config_type_init("coll",
	                                                         PARAM("algo", __coll_conf.algorithm, MPC_CONF_STRING, "Name of the collective module to use (simple, opt, hetero, noalloc)"),
	                                                         PARAM("barrier", barrier, MPC_CONF_TYPE, "Options for Barrier"),
	                                                         PARAM("bcast", bcast, MPC_CONF_TYPE, "Options for Bcast"),
	                                                         PARAM("allreduce", allreduce, MPC_CONF_TYPE, "Options for Bcast"),
	                                                         PARAM("shm", shm, MPC_CONF_TYPE, "Options for Shared Memory Collectives"),
	                                                         NULL);



	return coll;
}

void __mpc_lowcomm_coll_conf_validate(void)
{
	if(!strcmp(__coll_conf.algorithm, "noalloc") )
	{
		__coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_noalloc;
	}
	else if(!strcmp(__coll_conf.algorithm, "simple") )
	{
		__coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_simple;
	}
	else if(!strcmp(__coll_conf.algorithm, "opt") )
	{
		__coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_opt;
	}
	else if(!strcmp(__coll_conf.algorithm, "hetero") )
	{
		__coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_hetero;
	}
	else
	{
		bad_parameter("mpcframework.lowcomm.coll.algo must be one of simple, opt, hetero, noalloc, had '%s'", __coll_conf.algorithm);
	}
}

void mpc_lowcomm_coll_init_hook(mpc_lowcomm_communicator_t id)
{
	assume(__coll_conf.mpc_lowcomm_coll_init_hook != NULL);
	(__coll_conf.mpc_lowcomm_coll_init_hook)(id);
}

/*************************
* NETWORK CONFIGURATION *
*************************/

static struct sctk_runtime_config_struct_networks __net_config;

struct sctk_runtime_config_struct_networks *_mpc_lowcomm_config_net_get(void)
{
	return &__net_config;
}

/*
 *  RAILS defines instance of configurations
 */

struct sctk_runtime_config_struct_net_rail * _mpc_lowcomm_conf_rail_unfolded_get ( char *name )
{
    int l = 0;
	struct sctk_runtime_config_struct_net_rail *ret = NULL;

	for ( l = 0; l < __net_config.rails_size; ++l )
	{
		if ( strcmp ( name, __net_config.rails[l]->name ) == 0 )
		{
			ret = __net_config.rails[l];
			break;
		}
	}

	return ret;
}

mpc_conf_config_type_t *_mpc_lowcomm_conf_conf_rail_get ( char *name )
{
    return __get_type_by_name("mpcframework.lowcomm.networking.rails", name);
}


static inline void __mpc_lowcomm_rail_conf_default(void)
{
    memset(__net_config.rails, 0, MPC_CONF_MAX_RAIL_COUNT * sizeof(struct sctk_runtime_config_struct_net_rail *));
	__net_config.rails_size = 0;
}

static inline void __append_new_rail_to_unfolded(struct sctk_runtime_config_struct_net_rail * rail)
{
    if(__net_config.rails_size == MPC_CONF_MAX_RAIL_COUNT)
    {
        bad_parameter("Cannot create more than %d rails when processing rail %s.\n", MPC_CONF_MAX_RAIL_COUNT, rail->name);
    }


    if(_mpc_lowcomm_conf_rail_unfolded_get(rail->name))
    {
        bad_parameter("Cannot append the '%s' rail twice", rail->name);
    }

    __net_config.rails[ __net_config.rails_size ] = rail;
    __net_config.rails_size++;
}

mpc_conf_config_type_t *__new_rail_conf_instance(
	char *name,
	int priority,
	char *device,
	char * idle_poll_range,
	char * idle_poll_trigger,
	char *topology,
	int ondemand,
	int rdma,
	char *config)
{
    struct sctk_runtime_config_struct_net_rail * ret = sctk_malloc(sizeof(struct sctk_runtime_config_struct_net_rail));
    assume(ret != NULL);

    if(_mpc_lowcomm_conf_rail_unfolded_get(name))
    {
        bad_parameter("Cannot append the '%s' rail twice", name);
    }

    /* This fills in the struct */

    memset(ret, 0, sizeof(struct sctk_runtime_config_struct_net_rail));

    /* For unfolded retrieval */
    snprintf(ret->name, MPC_CONF_STRING_SIZE, name);
    ret->priority = priority;
    snprintf(ret->device, MPC_CONF_STRING_SIZE, device);
    snprintf(ret->any_source_polling.srange, MPC_CONF_STRING_SIZE, idle_poll_range);
    ret->any_source_polling.range = sctk_rail_convert_polling_set_from_string(idle_poll_range);
    snprintf(ret->any_source_polling.strigger, MPC_CONF_STRING_SIZE, idle_poll_trigger);
    ret->any_source_polling.trigger = sctk_rail_convert_polling_set_from_string(idle_poll_trigger);
    snprintf(ret->topology, MPC_CONF_STRING_SIZE, topology);
    ret->ondemand = ondemand;
    ret->rdma = rdma;
    snprintf(ret->config, MPC_CONF_STRING_SIZE, config);

    mpc_conf_config_type_t *gates = mpc_conf_config_type_init("gates", NULL);

    mpc_conf_config_type_t *idle_poll = mpc_conf_config_type_init("idlepoll",
	                                                              PARAM("range", ret->any_source_polling.srange , MPC_CONF_STRING, "Which cores can idle poll"),
                                                                  PARAM("trigger", ret->any_source_polling.strigger , MPC_CONF_STRING, "Which granularity can idle poll"),
                                                                  NULL);

    /* This fills in a rail definition */
  	mpc_conf_config_type_t *rail = mpc_conf_config_type_init(name,
	                                                         PARAM("priority", &ret->priority, MPC_CONF_INT, "How rails should be sorted (taken in decreasing order)"),
	                                                         PARAM("device", ret->device, MPC_CONF_STRING, "Name of the device to use can be a regular expression if starting with '!'"),
	                                                         PARAM("idlepoll", idle_poll, MPC_CONF_TYPE, "Parameters for idle polling"),
	                                                         PARAM("topology", ret->topology, MPC_CONF_STRING, "Topology to be bootstrapped on this network"),
	                                                         PARAM("ondemand", &ret->ondemand, MPC_CONF_BOOL, "Are on-demmand connections allowed on this network"),
	                                                         PARAM("rdma", &ret->ondemand, MPC_CONF_BOOL, "Can this rail provide RDMA capabilities"),
	                                                         PARAM("config", ret->config, MPC_CONF_STRING, "Name of the rail configuration to be used for this rail"),
	                                                         PARAM("gates", gates, MPC_CONF_TYPE, "Gates to check before taking this rail"),
	                                                         NULL);  


    __append_new_rail_to_unfolded(ret);

    return rail;
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_rail_conf_init()
{
	__mpc_lowcomm_rail_conf_default();

    /* Here we instanciate default rails */
    mpc_conf_config_type_t *shm_mpi = __new_rail_conf_instance("shmmpi", 99, "default", "machine", "socket", "fully", 0, 0, "shmconfigmpi");
    mpc_conf_config_type_t *tcp_mpi = __new_rail_conf_instance("tcpmpi", 9, "default", "machine", "socket", "ring", 1, 1, "tcpconfigmpi");
#ifdef MPC_USE_PORTALS
    mpc_conf_config_type_t *portals_mpi = __new_rail_conf_instance("portalsmpi", 6, "default", "machine", "socket", "ring", 1, 1, "portals_config");
#endif
#ifdef MPC_USE_INFINIBAND
    mpc_conf_config_type_t *ib_mpi = __new_rail_conf_instance("ibmpi", 1, "!mlx.*", "machine", "socket", "ring", 1, 1, "ib_config_mpi");
#endif


  	mpc_conf_config_type_t *rails = mpc_conf_config_type_init("rails",
	                                                         PARAM("shmmpi", shm_mpi, MPC_CONF_TYPE, "A rail with only SHM"),
                                                             PARAM("tcpmpi", tcp_mpi, MPC_CONF_TYPE, "A rail with TCP and SHM"),
#ifdef MPC_USE_PORTALS
                                                             PARAM("portalsmpi", portals_mpi, MPC_CONF_TYPE, "A rail with Portals 4"),
#endif
#ifdef MPC_USE_INFINIBAND
                                                             PARAM("ibmpi", ib_mpi, MPC_CONF_TYPE, "A rail with Infiniband and SHM"),
#endif
	                                                         NULL);  

    return rails;
}

mpc_conf_config_type_t * ___new_default_rail(char * name)
{
    return __new_rail_conf_instance(name,
                                    1,
                                    "default",
                                    "machine",
                                    "socket",
                                    "ring",
                                    1,
                                    0,
                                    "tcp_mpi");
}


static inline mpc_conf_config_type_t *___mpc_lowcomm_rail_all(void)
{
	mpc_conf_config_type_elem_t *eall_rails = mpc_conf_root_config_get("mpcframework.lowcomm.networking.rails");

    assume(eall_rails != NULL);
    assume(eall_rails->type == MPC_CONF_TYPE);

	return  mpc_conf_config_type_elem_get_inner(eall_rails);
}


static inline mpc_conf_config_type_t * ___mpc_lowcomm_rail_instanciate_from_default(mpc_conf_config_type_elem_t * elem)
{
    mpc_conf_config_type_t *default_rail = ___new_default_rail(elem->name);

    /* Here we override with what was already present in the config */
    mpc_conf_config_type_t * current_rail = mpc_conf_config_type_elem_get_inner(elem);
   
    return mpc_conf_config_type_elem_update(default_rail, current_rail, 1);
}


static inline void ___assert_single_elem_in_gate(mpc_conf_config_type_t * gate, char * elem_name)
{
	if(mpc_conf_config_type_count(gate) != 1)
	{
		bad_parameter("Gate definition '%s' expects only a single element '%s'.", gate->name, elem_name);
	}
}

static inline long int __gate_get_long_int(mpc_conf_config_type_t * gate, char * val_key, char * doc)
{
	mpc_conf_config_type_elem_t *val = mpc_conf_config_type_get(gate, val_key);

	if(!val)
	{
		mpc_conf_config_type_print(gate, MPC_CONF_FORMAT_XML);
		bad_parameter("'%s' gate should contain a '%s' value", gate->name, val_key);
	}

	if(doc)
	{
		mpc_conf_config_type_elem_set_doc(val, doc);
	}

	long int ret = 0;

	if(val->type == MPC_CONF_INT)
	{
		int * ival = (int*)val->addr;
		ret = *ival;
	}else if(val->type == MPC_CONF_LONG_INT)
	{
		ret = *((long int*)val->addr);
	}
	else
	{
		mpc_conf_config_type_print(gate, MPC_CONF_FORMAT_XML);
		bad_parameter("In gate '%s' entry '%s' should be either INT or LONG_INT not '%s'", gate->name, val_key, mpc_conf_type_name(val->type));
	}

	return ret;
}

static inline int __gate_get_bool(mpc_conf_config_type_t * gate, char * val_key, char * doc)
{
	mpc_conf_config_type_elem_t *val = mpc_conf_config_type_get(gate, val_key);

	if(!val)
	{
		mpc_conf_config_type_print(gate, MPC_CONF_FORMAT_XML);
		bad_parameter("'%s' gate should contain a '%s' value", gate->name, val_key);
	}

	if(doc)
	{
		mpc_conf_config_type_elem_set_doc(val, doc);
	}

	long int ret = 0;

	if(val->type == MPC_CONF_BOOL)
	{
		int * ival = (int*)val->addr;
		ret = *ival;
	}
	else
	{
		mpc_conf_config_type_print(gate, MPC_CONF_FORMAT_XML);
		bad_parameter("In gate '%s' entry '%s' should be BOOL (true or false) not '%s'", gate->name, val_key, mpc_conf_type_name(val->type));
	}

	return ret;
}


static inline int ___parse_rail_gate(struct sctk_runtime_config_struct_net_gate * cur_unfolded_gate,
								 mpc_conf_config_type_elem_t* tgate)
{
	mpc_conf_config_type_t * gate = mpc_conf_config_type_elem_get_inner(tgate);

	char * name = gate->name;

	cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_NONE;

	if(!strcmp(name, "boolean"))
	{
		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_BOOLEAN;


	}else if(!strcmp(name, "probabilistic"))
	{
		mpc_conf_config_type_elem_set_doc(tgate, "A gate defining a propability of taking the rail (0-100)");
		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_PROBABILISTIC;
		long int proba = __gate_get_long_int(gate, "probability", "Probability of taking the rail");
		___assert_single_elem_in_gate(gate, "probability");
		cur_unfolded_gate->value.probabilistic.probability = proba;
	}else if(!strcmp(name, "minsize"))
	{
		mpc_conf_config_type_elem_set_doc(tgate, "A gate defining a minimum size in bytes for taking the rail");
		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_MINSIZE;
		long int minsize = __gate_get_long_int(gate, "value", "Minimum size to use this rail");
		___assert_single_elem_in_gate(gate, "value");
		cur_unfolded_gate->value.minsize.value = minsize;
	}else if(!strcmp(name, "maxsize"))
	{
		mpc_conf_config_type_elem_set_doc(tgate, "A gate defining a maximum size in bytes for taking the rail");
		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_MAXSIZE;
		long int maxsize = __gate_get_long_int(gate, "value", "Maximum size to use this rail");
		___assert_single_elem_in_gate(gate, "value");
		cur_unfolded_gate->value.maxsize.value = maxsize;
	}else if(!strcmp(name, "msgtype"))
	{
		mpc_conf_config_type_elem_set_doc(tgate, "A gate filtering message types (process, task, emulated_rma, common)");

		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_MSGTYPE;

		cur_unfolded_gate->value.msgtype.process = __gate_get_bool(gate, "process", 
																	   "Process Specific Messages can use this rail");
		cur_unfolded_gate->value.msgtype.task = __gate_get_bool(gate, "task",
																	"Task specific messages can use this rail");
		cur_unfolded_gate->value.msgtype.emulated_rma = __gate_get_bool(gate, "emulatedrma",
																			"Emulated RDMA can use this rail");
		cur_unfolded_gate->value.msgtype.common = __gate_get_bool(gate, "common",
																	"Common messages (MPI) can use this raill");

	}else if(!strcmp(name, "user"))
	{
		mpc_conf_config_type_elem_set_doc(tgate, "A gate filtering message types using a custom function");

		cur_unfolded_gate->type = MPC_CONF_RAIL_GATE_USER;
		
		mpc_conf_config_type_elem_t *fname = mpc_conf_config_type_get(gate, "funcname");

		if(!fname)
		{
			bad_parameter("Gate %s requires a function name to be passed in as 'funcname'", name);
		}

		mpc_conf_config_type_elem_set_doc(fname, "Function used to filter our messages int func( sctk_rail_info_t * rail, mpc_lowcomm_ptp_message_t * message , void * gate_config )");


		if(fname->type != MPC_CONF_STRING)
		{
		bad_parameter("In gate '%s' entry 'funcname' should be STRING not '%s'", name, mpc_conf_type_name(fname->type));

		}

		char * gate_name = mpc_conf_type_elem_get_as_string(fname);

		cur_unfolded_gate->value.user.gatefunc.name = strdup(gate_name);

		void * ptr = dlsym(NULL, gate_name);
		cur_unfolded_gate->value.user.gatefunc.value = ptr;
	}else
	{
		bad_parameter("Cannot parse gate type '%s' available types are:\n[%s]", name, "boolean, probabilistic, minsize, maxsize, msgtype, user");
	}


	return 0;
}






static inline void __mpc_lowcomm_rail_unfold_gates(struct sctk_runtime_config_struct_net_rail *unfolded_rail,
												   mpc_conf_config_type_t *gates_type)
{
	int i;

	for(i = 0 ; i < mpc_conf_config_type_count(gates_type); i++)
    {
        mpc_conf_config_type_elem_t* gate = mpc_conf_config_type_nth(gates_type, i);
		struct sctk_runtime_config_struct_net_gate * cur_unfolded_gate = &unfolded_rail->gates[unfolded_rail->gates_size];

		if(___parse_rail_gate(cur_unfolded_gate, gate) == 0)
		{
			/* Gate  is ok continue */
			unfolded_rail->gates_size++;	
		}
		else
		{
			bad_parameter("Failed parsing gate %s in rail %s", gate->name, unfolded_rail->name);
		}

	}

}



static inline void ___mpc_lowcomm_rail_conf_validate(void)
{
    /* First we need to parse back the polling levels from strings back to values */
    int i;
    for( i = 0 ; i < __net_config.rails_size; i++)
    {
        __net_config.rails[i]->any_source_polling.range = sctk_rail_convert_polling_set_from_string(__net_config.rails[i]->any_source_polling.srange);
        __net_config.rails[i]->any_source_polling.trigger = sctk_rail_convert_polling_set_from_string(__net_config.rails[i]->any_source_polling.strigger);
    }

    /* Now we need to populate rails with possibly new instances
       this is done by (1) copying the default struct and then (2)
       updating element by element if present in the configuration
       this allows partial rail definition */

    mpc_conf_config_type_t * all_rails = ___mpc_lowcomm_rail_all();

    for(i = 0 ; i < mpc_conf_config_type_count(all_rails); i++)
    {
        mpc_conf_config_type_elem_t* rail = mpc_conf_config_type_nth(all_rails, i);
        if(!_mpc_lowcomm_conf_rail_unfolded_get(rail->name))
        {
            mpc_conf_config_type_t * new_rail = ___mpc_lowcomm_rail_instanciate_from_default(rail);
			mpc_conf_config_type_release((mpc_conf_config_type_t**)&all_rails->elems[i]->addr);
			all_rails->elems[i]->addr = new_rail;
        }
    }

	/* It is now time to unpack gates values for each rail */
	for(i = 0 ; i < mpc_conf_config_type_count(all_rails); i++)
    {
        mpc_conf_config_type_elem_t* rail = mpc_conf_config_type_nth(all_rails, i);
		mpc_conf_config_type_t *rail_type = mpc_conf_config_type_elem_get_inner(rail);

		mpc_conf_config_type_elem_t *gates  = mpc_conf_config_type_get(rail_type, "gates");

		if(gates)
		{
			struct sctk_runtime_config_struct_net_rail *unfolded_rail = _mpc_lowcomm_conf_rail_unfolded_get(rail->name);
			mpc_conf_config_type_t *gates_type = mpc_conf_config_type_elem_get_inner(gates);
			
			__mpc_lowcomm_rail_unfold_gates(unfolded_rail, gates_type);
		}
		else
		{
			bad_parameter("There should be a gate entry in rail %s", rail->name );
		}
	}
}

/*_
 * CLI defines group of rails
 */

mpc_conf_config_type_t *_mpc_lowcomm_conf_cli_get ( char *name )
{
    return __get_type_by_name("mpcframework.lowcomm.networking.cli.options", name);
}

static mpc_conf_config_type_t *___mpc_lowcomm_cli_conf_option_init(char *name, char *rail1, char *rail2)
{
	char *ar1 = malloc(sizeof(char)*MPC_CONF_STRING_SIZE);

	assume(ar1);
	snprintf(ar1, MPC_CONF_STRING_SIZE, rail1);

	mpc_conf_config_type_t *rails = NULL;

	if(rail2)
	{
		char *ar2 = malloc(sizeof(char)*MPC_CONF_STRING_SIZE);
		assume(ar2);
		snprintf(ar2, MPC_CONF_STRING_SIZE, rail2);

		rails = mpc_conf_config_type_init(name,
		                                  PARAM("first", ar1, MPC_CONF_STRING, "First rail to pick"),
		                                  PARAM("second", ar2, MPC_CONF_STRING, "Second rail to pick"),
		                                  NULL);
	}
	else
	{
		rails = mpc_conf_config_type_init(name,
		                                  PARAM("first", ar1, MPC_CONF_STRING, "First rail to pick"),
		                                  NULL);
	}

	int i;

	/* All were allocated */
	for(i = 0; i < mpc_conf_config_type_count(rails); i++)
	{
		mpc_conf_config_type_elem_set_to_free(mpc_conf_config_type_nth(rails, i), 1);
	}


	return rails;
}

static mpc_conf_config_type_t *__mpc_lowcomm_cli_conf_init(void)
{
	mpc_conf_config_type_t *cliopt = mpc_conf_config_type_init("options",
#ifdef MPC_USE_PORTALS
	                                                           PARAM("portals", ___mpc_lowcomm_cli_conf_option_init("tcp", "portalsmpi", NULL), MPC_CONF_TYPE, "Combination of TCP and SHM"),
#endif
#ifdef MPC_USE_INFINIBAND
	                                                           PARAM("ib", ___mpc_lowcomm_cli_conf_option_init("tcp", "shmmpi", "ibmpi"), MPC_CONF_TYPE, "Combination of TCP and SHM"),
#endif
	                                                           PARAM("tcp", ___mpc_lowcomm_cli_conf_option_init("tcp", "shmmpi", "tcpmpi"), MPC_CONF_TYPE, "Combination of TCP and SHM"),
	                                                           PARAM("shm", ___mpc_lowcomm_cli_conf_option_init("shm", "shmmpi", NULL), MPC_CONF_TYPE, "SHM Only"),

	                                                           NULL);

	mpc_conf_config_type_t *cli = mpc_conf_config_type_init("cli",
	                                                        PARAM("default", __net_config.cli_default_network, MPC_CONF_STRING, "Default Network CLI option to choose"),
	                                                        PARAM("options", cliopt, MPC_CONF_TYPE, "CLI alternaltives for network configurations"),
	                                                        NULL);

	return cli;
}

static inline void _mpc_lowcomm_net_config_default(void)
{
#ifdef MPC_USE_PORTALS
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "portals");
#elif defined(MPC_USE_INFINIBAND)
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "ib");
#else
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "tcp");
#endif
}

static mpc_conf_config_type_t *__mpc_lowcomm_network_conf_init(void)
{
	_mpc_lowcomm_net_config_default();

	mpc_conf_config_type_t *cli = __mpc_lowcomm_cli_conf_init();

	mpc_conf_config_type_t *rails = __mpc_lowcomm_rail_conf_init();

	mpc_conf_config_type_t *network = mpc_conf_config_type_init("networking",
	                                                            PARAM("rails", rails, MPC_CONF_TYPE, "Rail definitions for Networks"),
	                                                            PARAM("cli", cli, MPC_CONF_TYPE, "Name definitions for networks"),
	                                                            NULL);

	return network;
}

/* This is the CLI unfolding and validation */

static inline void ___mpc_lowcomm_cli_option_validate(mpc_conf_config_type_elem_t *opt)
{
	if(opt->type != MPC_CONF_TYPE)
	{
		bad_parameter("mpcframework.lowcomm.networking.cli.options.%s erroneous should be MPC_CONF_TYPE not ", opt->name, mpc_conf_type_name(opt->type) );
	}


	mpc_conf_config_type_t *toptions = mpc_conf_config_type_elem_get_inner(opt);

	int i;
	for(i = 0; i < mpc_conf_config_type_count(toptions); i++)
	{
		mpc_conf_config_type_elem_t *elem = mpc_conf_config_type_nth(toptions, i);

		if(elem->type != MPC_CONF_STRING)
		{
			mpc_conf_config_type_elem_print(opt, MPC_CONF_FORMAT_XML);
			bad_parameter("mpcframework.lowcomm.networking.cli.options.%s.%s erroneous should be MPC_CONF_STRING not %s", opt->name, elem->name, mpc_conf_type_name(elem->type) );
		}

        char * rail_name = mpc_conf_type_elem_get_as_string(elem);

        /* Now check that the rail does exist */
        if(!_mpc_lowcomm_conf_conf_rail_get(rail_name))
        {
            mpc_conf_config_type_elem_print(opt, MPC_CONF_FORMAT_XML);
            bad_parameter("There is no such rail named '%s'", rail_name);
        }
	}
}

static inline void ___mpc_lowcomm_cli_conf_validate(void)
{
	/* Make sure that the default is actually a CLI option */
	mpc_conf_config_type_elem_t *def = mpc_conf_root_config_get("mpcframework.lowcomm.networking.cli.default");

	assume(def != NULL);

	char path_to_default[128];
	snprintf(path_to_default, 128, "mpcframework.lowcomm.networking.cli.options.%s", mpc_conf_type_elem_get_as_string(def) );

	mpc_conf_config_type_elem_t *default_cli = mpc_conf_root_config_get(path_to_default);
	mpc_conf_config_type_elem_t *options     = mpc_conf_root_config_get("mpcframework.lowcomm.networking.cli.options");

	if(!default_cli)
	{
		if(options)
		{
			mpc_conf_config_type_elem_print(options, MPC_CONF_FORMAT_XML);
		}


		bad_parameter("Could not locate mpcframework.lowcomm.networking.cli.default='%s' in mpcframework.lowcomm.networking.cli.options", mpc_conf_type_elem_get_as_string(def) );
	}


	/* Now check all cli option for compliance */
	mpc_conf_config_type_t *toptions = mpc_conf_config_type_elem_get_inner(options);
	int i;

	for(i = 0; i < mpc_conf_config_type_count(toptions); i++)
	{
		___mpc_lowcomm_cli_option_validate(mpc_conf_config_type_nth(toptions, i) );
	}
}

void __mpc_lowcomm_network_conf_validate(void)
{
	/* Validate and unfold CLI Options */
	___mpc_lowcomm_cli_conf_validate();
    ___mpc_lowcomm_rail_conf_validate();
}

/************************************
* GLOBAL CONFIGURATION FOR LOWCOMM *
************************************/

struct _mpc_lowcomm_conf __lowcomm_conf;

struct _mpc_lowcomm_conf *_mpc_lowcomm_conf_get(void)
{
	return &__lowcomm_conf;
}

static void __lowcomm_conf_default(void)
{
#ifdef SCTK_USE_CHECKSUM
	__lowcomm_conf.checksum = 0;
#endif
}

void _mpc_lowcomm_config_register(void)
{
	__lowcomm_conf_default();

	mpc_conf_config_type_t *coll     = __mpc_lowcomm_coll_conf_init();
	mpc_conf_config_type_t *networks = __mpc_lowcomm_network_conf_init();


	mpc_conf_config_type_t *lowcomm = mpc_conf_config_type_init("lowcomm",
#ifdef SCTK_USE_CHECKSUM
	                                                            PARAM("checksum", &__lowcomm_conf.checksum, MPC_CONF_BOOL, "Enable buffer checksum for P2P messages"),
#endif
	                                                            PARAM("coll", coll, MPC_CONF_TYPE, "Lowcomm collective configuration"),
	                                                            PARAM("networking", networks, MPC_CONF_TYPE, "Lowcomm Networking configuration"),

	                                                            NULL);

	mpc_conf_root_config_append("mpcframework", lowcomm, "MPC Lowcomm Configuration");
}

void _mpc_lowcomm_config_validate(void)
{
	__mpc_lowcomm_coll_conf_validate();
	__mpc_lowcomm_network_conf_validate();
    
}
