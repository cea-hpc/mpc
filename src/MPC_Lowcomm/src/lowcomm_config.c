#include "lowcomm_config.h"

#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>

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

	__coll_conf.algorithm = strdup("noalloc");
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
	                                                         PARAM("algo", &__coll_conf.algorithm, MPC_CONF_STRING, "Name of the collective module to use (simple, opt, hetero, noalloc)"),
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
    ret->name = strdup(name);
    ret->priority = priority;
    ret->device = strdup(device);
    ret->any_source_polling.srange = strdup(idle_poll_range);
    ret->any_source_polling.range = sctk_rail_convert_polling_set_from_string(idle_poll_range);
    ret->any_source_polling.strigger = strdup(idle_poll_trigger);
    ret->any_source_polling.trigger = sctk_rail_convert_polling_set_from_string(idle_poll_trigger);
    ret->topology = strdup(topology);
    ret->ondemand = ondemand;
    ret->rdma = rdma;
    ret->config = strdup(config);

    mpc_conf_config_type_t *gates = mpc_conf_config_type_init("gates", NULL);

    mpc_conf_config_type_t *idle_poll = mpc_conf_config_type_init("idlepoll",
	                                                              PARAM("range", &ret->any_source_polling.srange , MPC_CONF_STRING, "Which cores can idle poll"),
                                                                  PARAM("trigger", &ret->any_source_polling.strigger , MPC_CONF_STRING, "Which granularity can idle poll"),
                                                                  NULL);

    /* This fills in a rail definition */
  	mpc_conf_config_type_t *rail = mpc_conf_config_type_init(name,
	                                                         PARAM("priority", &ret->priority, MPC_CONF_INT, "How rails should be sorted (taken in decreasing order)"),
	                                                         PARAM("device", &ret->device, MPC_CONF_STRING, "Name of the device to use can be a regular expression if starting with '!'"),
	                                                         PARAM("idlepoll", idle_poll, MPC_CONF_TYPE, "Parameters for idle polling"),
	                                                         PARAM("topology", &ret->topology, MPC_CONF_STRING, "Topology to be bootstrapped on this network"),
	                                                         PARAM("ondemand", &ret->ondemand, MPC_CONF_BOOL, "Are on-demmand connections allowed on this network"),
	                                                         PARAM("rdma", &ret->ondemand, MPC_CONF_BOOL, "Can this rail provide RDMA capabilities"),
	                                                         PARAM("config", &ret->config, MPC_CONF_STRING, "Name of the rail configuration to be used for this rail"),
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


	/* First check current in default to ensure that all entries are known */
    int i;
    for(i = 0 ; i < mpc_conf_config_type_count(current_rail); i++)
    {
        mpc_conf_config_type_elem_t* current_elem = mpc_conf_config_type_nth(current_rail, i);

        /* Now get the elem to ensure it already exists */
        mpc_conf_config_type_elem_t *default_elem = mpc_conf_config_type_get(default_rail, current_elem->name);

        if(!default_elem)
        {
            mpc_conf_config_type_elem_print(elem, MPC_CONF_FORMAT_XML);
            bad_parameter("Rail definitions does not contain '%s' elements", current_elem->name);
        }

    }


	/* Now check default in current to push missing elements from default */
	for(i = 0 ; i < mpc_conf_config_type_count(default_rail); i++)
    {
        mpc_conf_config_type_elem_t* default_elem = mpc_conf_config_type_nth(default_rail, i);
        /* Now get the elem to ensure it already exists */
        mpc_conf_config_type_elem_t *existing_elem = mpc_conf_config_type_get(current_rail, default_elem->name);

        if(existing_elem)
        {
			/* Here we need to update the default elem */
            mpc_conf_config_type_elem_set_from_elem(default_elem, existing_elem);
        }
    }

	/* Release default conf */
	mpc_conf_config_type_release(&default_rail);

    /* Reorder according to default rail */

    if(__net_config.rails)
    {
        if(__net_config.rails[0])
        {
            mpc_conf_config_type_t *first_rail = _mpc_lowcomm_conf_conf_rail_get ( __net_config.rails[0]->name );
            mpc_config_type_match_order(current_rail, first_rail);
        }
    }

	//mpc_conf_config_type_release(&default_rail);
   
    return default_rail;
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
            ___mpc_lowcomm_rail_instanciate_from_default(rail);
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
	char **ar1 = malloc(sizeof(char **) );

	assume(ar1);
	*ar1 = strdup(rail1);

	mpc_conf_config_type_t *rails = NULL;

	if(rail2)
	{
		char **ar2 = malloc(sizeof(char **) );
		assume(ar2);
		*ar2 = strdup(rail2);


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
	                                                        PARAM("default", &__net_config.cli_default_network, MPC_CONF_STRING, "Default Network CLI option to choose"),
	                                                        PARAM("options", cliopt, MPC_CONF_TYPE, "CLI alternaltives for network configurations"),
	                                                        NULL);

	return cli;
}

static inline void _mpc_lowcomm_net_config_default(void)
{
#ifdef MPC_USE_PORTALS
	__net_config.cli_default_network = strdup("portals");
#elif defined(MPC_USE_INFINIBAND)
	__net_config.cli_default_network = strdup("ib");
#else
	__net_config.cli_default_network = strdup("tcp");
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
