#include "lowcomm_config.h"

#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>

#include "coll.h"

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

struct sctk_runtime_config_struct_networks *_mpc_lowcomm_net_config_get(void)
{
	return &__net_config;
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
		                                  PARAM("first", ar1, MPC_CONF_STRING, "First rail in order"),
		                                  PARAM("second", ar2, MPC_CONF_STRING, "Second rail in order"),
		                                  NULL);
	}
	else
	{
		rails = mpc_conf_config_type_init(name,
		                                  PARAM("first", ar1, MPC_CONF_STRING, "First rail in order"),
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
	                                                           PARAM("portals", ___mpc_lowcomm_cli_conf_option_init("tcp", "portals_mpi", NULL), MPC_CONF_TYPE, "Combination of TCP and SHM"),
#endif
#ifdef MPC_USE_INFINIBAND
	                                                           PARAM("ib", ___mpc_lowcomm_cli_conf_option_init("tcp", "shm_mpi", "ib_mpi"), MPC_CONF_TYPE, "Combination of TCP and SHM"),
#endif
	                                                           PARAM("tcp", ___mpc_lowcomm_cli_conf_option_init("tcp", "shm_mpi", "tcp_mpi"), MPC_CONF_TYPE, "Combination of TCP and SHM"),
	                                                           PARAM("shm", ___mpc_lowcomm_cli_conf_option_init("shm", "shm_mpi", NULL), MPC_CONF_TYPE, "SHM Only"),

	                                                           NULL);

	mpc_conf_config_type_t *cli = mpc_conf_config_type_init("cli",
	                                                        PARAM("default", &__net_config.cli_default_network, MPC_CONF_STRING, "Default Network CLI option to choose"),
	                                                        PARAM("options", cliopt, MPC_CONF_TYPE, "CLI alternaltives for network configurations"),
	                                                        NULL);

	return cli;
}

static mpc_conf_config_type_t *__mpc_lowcomm_network_conf_init(void)
{
	_mpc_lowcomm_net_config_default();

	mpc_conf_config_type_t *cli = __mpc_lowcomm_cli_conf_init();

	mpc_conf_config_type_t *network = mpc_conf_config_type_init("networking",
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

	/*TODO: check rails actually exist */
}

void __mpc_lowcomm_network_conf_validate(void)
{
	/* Validate and unfold CLI Options */
	___mpc_lowcomm_cli_conf_validate();
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
