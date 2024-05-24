#include "lowcomm_config.h"

#include <stdio.h>
#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>
#include <dlfcn.h>
#include <ctype.h>

//FIXME: enable verbosity when only lowcomm module is loaded
#include <mpc_common_flags.h>
#include <limits.h>

#include "coll.h"
#include "mpc_conf_types.h"

#include <sctk_alloc.h>

/***********
* HELPERS *
***********/

mpc_conf_config_type_t *__get_type_by_name(char *prefix, char *name)
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

	snprintf(__coll_conf.algorithm, MPC_CONF_STRING_SIZE, "%s", "noalloc");
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

/************************
* DRIVER CONFIGURATION *
************************/

static struct _mpc_lowcomm_config_struct_networks __net_config;

static inline void __mpc_lowcomm_driver_conf_default(void)
{
	memset(__net_config.configs, 0, MPC_CONF_MAX_CONFIG_COUNT * sizeof(struct _mpc_lowcomm_config_struct_net_driver_config *) );
	__net_config.configs_size = 0;
}

struct _mpc_lowcomm_config_struct_net_driver_config *_mpc_lowcomm_conf_driver_unfolded_get(char *name)
{
	if(!name)
	{
		return NULL;
	}

	int i;

	for(i = 0; i < __net_config.configs_size; i++)
	{
		if(!strcmp(name, __net_config.configs[i]->name) )
		{
			return __net_config.configs[i];
		}
	}

	return NULL;
}

static inline void __append_new_driver_to_unfolded(struct _mpc_lowcomm_config_struct_net_driver_config *driver_config)
{
	if(__net_config.configs_size == MPC_CONF_MAX_CONFIG_COUNT)
	{
		bad_parameter("Cannot create more than %d driver config when processing config %s.\n", MPC_CONF_MAX_CONFIG_COUNT, driver_config->name);
	}


	if(_mpc_lowcomm_conf_driver_unfolded_get(driver_config->name) )
	{
		bad_parameter("Cannot append the '%s' driver configuration twice", driver_config->name);
	}

	__net_config.configs[__net_config.configs_size] = driver_config;
	__net_config.configs_size++;
}


#ifdef MPC_USE_OFI
static inline mpc_conf_config_type_t *__init_driver_ofi(struct _mpc_lowcomm_config_struct_net_driver *driver, char * provider)
{
	driver->type = MPC_LOWCOMM_CONFIG_DRIVER_OFI;

	snprintf(driver->value.ofi.provider, MPC_CONF_STRING_SIZE, "%s", provider);

	driver->value.ofi.eager_size = 8192;
	driver->value.ofi.eager_per_buff = 100;
	driver->value.ofi.request_cache_size = 2048;
	driver->value.ofi.number_of_multi_recv_buff = 3;
	driver->value.ofi.bcopy_size = 1024;
	driver->value.ofi.enable_multi_recv = 1;
	(void)snprintf(driver->value.ofi.endpoint_type, MPC_CONF_STRING_SIZE, "FI_EP_RDM");
	/* We set some verbs specific configurations*/
	if(!strcmp(driver->value.ofi.provider, "verbs"))
	{
		(void)snprintf(driver->value.ofi.endpoint_type, MPC_CONF_STRING_SIZE, "FI_EP_MSG");
		driver->value.ofi.bcopy_size = 8192;
	}

	return mpc_conf_config_type_init("ofi",
														PARAM("reqcachesize", &driver->value.ofi.request_cache_size,
																MPC_CONF_INT, "Number of request to put in the OFI cache"),
														PARAM("eagersize", &driver->value.ofi.eager_size,
																MPC_CONF_INT, "Size of eager messages"),
														PARAM("bcopysize", &driver->value.ofi.bcopy_size,
																MPC_CONF_INT, "Size of bcopy messages"),
														PARAM("eagerperbuff", &driver->value.ofi.eager_per_buff,
																MPC_CONF_INT, "Number of eagers per recv buffer"),
														PARAM("multirecv", &driver->value.ofi.enable_multi_recv,
																MPC_CONF_INT, "Activate multi recv support FI_MULTI_RECV capability required"),
														PARAM("numrecvbuff", &driver->value.ofi.number_of_multi_recv_buff,
																MPC_CONF_INT, "Number of receive buffers (only for multirecv == True)"),
                                          PARAM("eptype",
                                                driver->value.ofi.endpoint_type,
                                                MPC_CONF_STRING,
                                                "Type of OFI endpoint to use (FI_EP_UNSPEC, FI_EP_MSG, FI_EP_DGRAM, FI_EP_RDM, FI_EP_SOCK_STREAM, FI_EP_SOCK_DGRAM,)."),
                                          PARAM("provider",
                                                driver->value.ofi.provider,
                                                MPC_CONF_STRING,
                                                "Name of the requested OFI provider."),
                                          NULL);
}
#endif

static inline mpc_conf_config_type_t *__init_driver_shm(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = MPC_LOWCOMM_CONFIG_DRIVER_SHM;


	mpc_conf_config_type_t *ret =
                mpc_conf_config_type_init("shm",
                                          NULL);

	return ret;
}


static inline mpc_conf_config_type_t *__init_driver_tbsm(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = MPC_LOWCOMM_CONFIG_DRIVER_TBSM;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_tbsm *tbsm = &driver->value.tbsm;

        tbsm->eager_limit    = 8192;
        tbsm->max_msg_size   = INT_MAX;
        tbsm->bcopy_buf_size = 8192;

	/*
	  Create the config object
	*/

	mpc_conf_config_type_t *ret =
                mpc_conf_config_type_init("tbsm",
                                          PARAM("eagerlimit",
                                                &tbsm->eager_limit,
                                                MPC_CONF_INT,
                                                "Eager limit."),
                                          PARAM("maxmsgsize",
                                                &tbsm->max_msg_size,
                                                MPC_CONF_LONG_INT,
                                                "Max message size."),
                                          PARAM("bcopybufsize",
                                                &tbsm->bcopy_buf_size,
                                                MPC_CONF_INT,
                                                "Size of buffered copies."),
                                          NULL);

	return ret;
}


static inline mpc_conf_config_type_t *__init_driver_tcp(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = MPC_LOWCOMM_CONFIG_DRIVER_TCP;

	/*
	 * Set defaults
	 */

	struct _mpc_lowcomm_config_struct_net_driver_tcp *tcp = &driver->value.tcp;

	tcp->max_msg_size = INT_MAX;

	/*
	 * Create the config object
	 */

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("tcp",
	                                                        PARAM("maxmsgsize", &tcp->max_msg_size, MPC_CONF_INT, "Maximum message size (in B)"),
	                                                        NULL);

	return ret;
}

#ifdef MPC_USE_PORTALS


static inline mpc_conf_config_type_t *__init_driver_portals(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = MPC_LOWCOMM_CONFIG_DRIVER_PORTALS;

	/*
	 * Set defaults
	 */

	struct _mpc_lowcomm_config_struct_net_driver_portals *portals = &driver->value.portals;

	portals->eager_limit = 8192;
	portals->min_comms = 1;
	portals->block_cut = 2147483648;
	portals->offloading.collectives = 0;
	portals->offloading.ondemand = 0;
        portals->max_iovecs = 8;
        portals->num_eager_blocks = 32;
        portals->eager_block_size = 8*8192;
	portals->max_msg_size = 2147483648;
        portals->min_frag_size = 524288; // octets

	/*
	 * Create the config object
	 */

	mpc_conf_config_type_t *offload = mpc_conf_config_type_init("offload",
	                                                            PARAM("collective", &portals->offloading.collectives, MPC_CONF_BOOL, "Enable collective optimization for Portals."),
	                                                            PARAM("ondemand", &portals->offloading.ondemand, MPC_CONF_BOOL, "Enable on-demand optimization through ID hardware propagation."),
	                                                            NULL);


	mpc_conf_config_type_t *ret =
		mpc_conf_config_type_init("portals",
		                          PARAM("eagerlimit", &portals->eager_limit, MPC_CONF_LONG_INT, "Max size of messages allowed to use the eager protocol."),
		                          PARAM("maxmsgsize", &portals->max_msg_size, MPC_CONF_INT, "Max size of messages allowed to be sent."),
		                          PARAM("minfragsize", &portals->min_frag_size, MPC_CONF_LONG_INT, "Min size of fragments sent with multirail."),
		                          PARAM("eagerblocksize", &portals->eager_block_size, MPC_CONF_INT, "Size of eager block."),
		                          PARAM("numeagerblocks", &portals->num_eager_blocks, MPC_CONF_INT, "Number of eager blocks."),
		                          PARAM("mincomm", &portals->min_comms, MPC_CONF_INT, "Min number of communicators (help to avoid dynamic PT entry allocation)"),
		                          PARAM("blockcut", &portals->block_cut, MPC_CONF_LONG_INT, "Above this value, RDV messages will be split in multiple GET requests"),
		                          PARAM("offload", offload, MPC_CONF_TYPE, "List of available optimizations taking advantage of triggered Ops"),
		                          NULL);

	return ret;
}

#endif


/**
 * @brief This updates string values to enum values
 *
 * @param driver driver to update
 */
static inline void __mpc_lowcomm_driver_conf_unfold_values(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	switch(driver->type)
	{
		default:
			return;
	}
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_driver_conf_default_driver(char *config_name, char *driver_type)
{
	struct _mpc_lowcomm_config_struct_net_driver_config *new_conf = sctk_malloc(sizeof(struct _mpc_lowcomm_config_struct_net_driver_config) );

	assume(new_conf != NULL);

	memset(new_conf, 0, sizeof(struct _mpc_lowcomm_config_struct_net_driver_config) );

	snprintf(new_conf->name, MPC_CONF_STRING_SIZE, "%s", config_name);

	mpc_conf_config_type_t *driver = NULL;

	if(!strcmp(driver_type, "tbsm"))
	{
		driver = __init_driver_tbsm(&new_conf->driver);
	}
	else if(!strcmp(driver_type, "tcp"))
	{
		driver = __init_driver_tcp(&new_conf->driver);
	}
	else if(!strcmp(driver_type, "shm"))
	{
		driver = __init_driver_shm(&new_conf->driver);
	}
#if defined(MPC_USE_PORTALS)
	else if(!strcmp(driver_type, "portals") )
	{
		driver = __init_driver_portals(&new_conf->driver);
	}
#endif
#if defined MPC_USE_OFI
	else if(!strcmp(driver_type, "tcpofi"))
	{
		driver = __init_driver_ofi(&new_conf->driver, "tcp");
	}
	else if(!strcmp(driver_type, "verbsofi"))
	{
		driver = __init_driver_ofi(&new_conf->driver, "verbs");
	}
	else if(!strcmp(driver_type, "shmofi"))
	{
		driver = __init_driver_ofi(&new_conf->driver, "shm");
	}
	else if(!strcmp(driver_type, "ofi"))
	{
		/* The default driver is added with the TCP provider */
		driver = __init_driver_ofi(&new_conf->driver, "tcp");
	}
#endif

	if(!driver)
	{
		bad_parameter("Cannot create default config for driver '%s': no such driver", driver_type);
	}


	mpc_conf_config_type_t *ret = mpc_conf_config_type_init(config_name,
	                                                        PARAM(driver->name, driver, MPC_CONF_TYPE, "Driver configuration"),
	                                                        NULL);

	__append_new_driver_to_unfolded(new_conf);

	return ret;
}

static inline mpc_conf_config_type_t *___mpc_lowcomm_driver_all(void)
{
	mpc_conf_config_type_elem_t *eall_configs = mpc_conf_root_config_get("mpcframework.lowcomm.networking.configs");

	assume(eall_configs != NULL);
	assume(eall_configs->type == MPC_CONF_TYPE);

	return mpc_conf_config_type_elem_get_inner(eall_configs);
}

static inline mpc_conf_config_type_t *___mpc_lowcomm_driver_instanciate_from_default(mpc_conf_config_type_t *config)
{
	if(mpc_conf_config_type_count(config) != 1)
	{
		bad_parameter("Config %s should only contain a single driver configuration", config->name);
	}

	mpc_conf_config_type_elem_t *driver_dest = mpc_conf_config_type_nth(config, 0);

	/* Here we get the name of the inner driver */
	if(driver_dest->type != MPC_CONF_TYPE)
	{
		bad_parameter("Config %s.%s should only contain a driver definition (expected MPC_CONF_TYPE not %s)", config->name, driver_dest->name, mpc_conf_type_name(driver_dest->type) );
	}

	mpc_conf_config_type_t *default_config = __mpc_lowcomm_driver_conf_default_driver(config->name, driver_dest->name);

	return mpc_conf_config_type_elem_update(default_config, config, 16);
}

void ___mpc_lowcomm_driver_conf_validate()
{
	int i;

	mpc_conf_config_type_t *all_configs = ___mpc_lowcomm_driver_all();

	/* Here we merge new driver config with defaults from the driver */
	for(i = 0; i < mpc_conf_config_type_count(all_configs); i++)
	{
		mpc_conf_config_type_elem_t *confige = mpc_conf_config_type_nth(all_configs, i);
		mpc_conf_config_type_t *     config  = mpc_conf_config_type_elem_get_inner(confige);

		struct _mpc_lowcomm_config_struct_net_driver_config *unfold = _mpc_lowcomm_conf_driver_unfolded_get(config->name);

		if(!unfold)
		{
			mpc_conf_config_type_t *new_config = ___mpc_lowcomm_driver_instanciate_from_default(config);
			mpc_conf_config_type_release( (mpc_conf_config_type_t **)&all_configs->elems[i]->addr);
			all_configs->elems[i]->addr = new_config;
			/* Reget new conf */
			unfold = _mpc_lowcomm_conf_driver_unfolded_get(config->name);

			if(!unfold)
			{
				continue;
			}
		}

		/* This updates enums from string values */
		__mpc_lowcomm_driver_conf_unfold_values(&unfold->driver);
	}
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_driver_conf_init()
{
	__mpc_lowcomm_driver_conf_default();

	mpc_conf_config_type_t * shm  = __mpc_lowcomm_driver_conf_default_driver("shmconfigmpi", "shm");
	mpc_conf_config_type_t * tcp  = __mpc_lowcomm_driver_conf_default_driver("tcpconfigmpi", "tcp");
	mpc_conf_config_type_t * tbsm = __mpc_lowcomm_driver_conf_default_driver("tbsmconfigmpi", "tbsm");

#if defined (MPC_USE_OFI)
	mpc_conf_config_type_t * tcp_ofi = __mpc_lowcomm_driver_conf_default_driver("tcpofi", "tcpofi");
	mpc_conf_config_type_t * verbs_ofi = __mpc_lowcomm_driver_conf_default_driver("verbsofi", "verbsofi");
	mpc_conf_config_type_t * shm_ofi = __mpc_lowcomm_driver_conf_default_driver("shmofi", "shmofi");
#endif

#if defined(MPC_USE_PORTALS)
	mpc_conf_config_type_t *portals = __mpc_lowcomm_driver_conf_default_driver("portalsconfigmpi", "portals");
#endif


	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("configs",
	                                                        PARAM("shmconfigmpi", shm, MPC_CONF_TYPE, "Default configuration for the SHM driver"),

	                                                        PARAM("tcpconfigmpi", tcp, MPC_CONF_TYPE, "Default configuration for the TCP driver"),
	                                                        PARAM("tbsmconfigmpi", tbsm, MPC_CONF_TYPE, "Default configuration for the TBSM driver"),
#if defined(MPC_USE_PORTALS)
	                                                        PARAM("portalsconfigmpi", portals, MPC_CONF_TYPE, "Default configuration for the Portals4 Driver"),
#endif
#if defined(MPC_USE_OFI)
	                                                        PARAM("tcpofi", tcp_ofi, MPC_CONF_TYPE, "Default configuration for the OFI Driver in TCP"),
	                                                        PARAM("verbsofi", verbs_ofi, MPC_CONF_TYPE, "Default configuration for the OFI Driver in Verbs"),
	                                                        PARAM("shmofi", shm_ofi, MPC_CONF_TYPE, "Default configuration for the OFI Driver in SHM"),
#endif
	                                                        NULL);

	return ret;
}

/*************************
* NETWORK CONFIGURATION *
*************************/

struct _mpc_lowcomm_config_struct_networks *_mpc_lowcomm_config_net_get(void)
{
	return &__net_config;
}

/*
 *  RAILS defines instance of configurations
 */

struct _mpc_lowcomm_config_struct_net_rail *_mpc_lowcomm_conf_rail_unfolded_get(char *name)
{
	int l = 0;
	struct _mpc_lowcomm_config_struct_net_rail *ret = NULL;

	for(l = 0; l < __net_config.rails_size; ++l)
	{
		if(strcmp(name, __net_config.rails[l]->name) == 0)
		{
			ret = __net_config.rails[l];
			break;
		}
	}

	return ret;
}

mpc_conf_config_type_t *_mpc_lowcomm_conf_conf_rail_get(char *name)
{
	return __get_type_by_name("mpcframework.lowcomm.networking.rails", name);
}

static inline void __mpc_lowcomm_rail_conf_default(void)
{
	memset(__net_config.rails, 0, MPC_CONF_MAX_RAIL_COUNT * sizeof(struct _mpc_lowcomm_config_struct_net_rail *) );
	__net_config.rails_size = 0;
}

static inline void __append_new_rail_to_unfolded(struct _mpc_lowcomm_config_struct_net_rail *rail)
{
	if(__net_config.rails_size == MPC_CONF_MAX_RAIL_COUNT)
	{
		bad_parameter("Cannot create more than %d rails when processing rail %s.\n", MPC_CONF_MAX_RAIL_COUNT, rail->name);
	}


	if(_mpc_lowcomm_conf_rail_unfolded_get(rail->name) )
	{
		bad_parameter("Cannot append the '%s' rail twice", rail->name);
	}

	__net_config.rails[__net_config.rails_size] = rail;
	__net_config.rails_size++;
}

mpc_conf_config_type_t *__new_rail_conf_instance(
	char *name,
	int priority,
	char *device,
	int ondemand,
	int rdma,
	int self,
	int offload,
	int max_ifaces,
	char *config)
{
	struct _mpc_lowcomm_config_struct_net_rail *ret = sctk_malloc(sizeof(struct _mpc_lowcomm_config_struct_net_rail) );

	assume(ret != NULL);

	if(_mpc_lowcomm_conf_rail_unfolded_get(name) )
	{
		bad_parameter("Cannot append the '%s' rail twice", name);
	}

	/* This fills in the struct */

	memset(ret, 0, sizeof(struct _mpc_lowcomm_config_struct_net_rail) );

	/* For unfolded retrieval */
	snprintf(ret->name, MPC_CONF_STRING_SIZE, "%s", name);
	ret->priority = priority;
	snprintf(ret->device, MPC_CONF_STRING_SIZE, "%s", device);
	ret->ondemand   = ondemand;
	ret->rdma       = rdma;
	ret->offload    = offload;
	ret->max_ifaces = max_ifaces;
	snprintf(ret->config, MPC_CONF_STRING_SIZE, "%s", config);

	/* This fills in a rail definition */
	mpc_conf_config_type_t *rail = mpc_conf_config_type_init(name,
	                                                         PARAM("priority", &ret->priority, MPC_CONF_INT, "How rails should be sorted (taken in decreasing order)"),
	                                                         PARAM("device", ret->device, MPC_CONF_STRING, "Name of the device to use can be a regular expression if starting with '!'"),
	                                                         PARAM("ondemand", &ret->ondemand, MPC_CONF_BOOL, "Are on-demmand connections allowed on this network"),
	                                                         PARAM("rdma", &ret->rdma, MPC_CONF_BOOL, "Can this rail provide RDMA capabilities"),
	                                                         PARAM("sm", &ret->self, MPC_CONF_BOOL, "Can this rail provide SHM capabilities"),
	                                                         PARAM("offload", &ret->offload, MPC_CONF_BOOL, "Can this rail provide tag offload capabilities"),
	                                                         PARAM("maxifaces", &ret->max_ifaces, MPC_CONF_INT, "Maximum number of rails instances that can be used for multirail"),
	                                                         PARAM("config", ret->config, MPC_CONF_STRING, "Name of the rail configuration to be used for this rail"),
	                                                         NULL);


	__append_new_rail_to_unfolded(ret);

	return rail;
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_rail_conf_init()
{
	__mpc_lowcomm_rail_conf_default();

	/* Here we instanciate default rails */
	mpc_conf_config_type_t *shm_mpi = __new_rail_conf_instance("shmmpi", 99, "any", 0, 1, 0, 0, 1, "shmconfigmpi");
	mpc_conf_config_type_t *tcp_mpi = __new_rail_conf_instance("tcpmpi", 1, "any", 1, 0, 0, 0, 1, "tcpconfigmpi");
   mpc_conf_config_type_t *tbsm_mpi = __new_rail_conf_instance("tbsmmpi", 100, "any", 1, 0, 1, 0, 1, "tbsmconfigmpi");

#ifdef MPC_USE_PORTALS
	mpc_conf_config_type_t *portals_mpi = __new_rail_conf_instance("portalsmpi", 21, "any", 1, 1, 0, 1, 1, "portalsconfigmpi");
#endif

#ifdef MPC_USE_OFI
	mpc_conf_config_type_t *shm_ofi = __new_rail_conf_instance("shmofirail", 98, "any", 1, 1, 0, 0, 1, "shmofi");
	mpc_conf_config_type_t *verbs_ofi = __new_rail_conf_instance("verbsofirail", 21, "any", 1, 1, 0, 0, 1, "verbsofi");
	mpc_conf_config_type_t *tcp_ofi = __new_rail_conf_instance("tcpofirail", 20, "any", 1, 1, 0, 0, 1, "tcpofi");
#endif

	mpc_conf_config_type_t *rails = mpc_conf_config_type_init("rails",
	                                                          PARAM("shmmpi", shm_mpi, MPC_CONF_TYPE, "A rail with SHM"),
	                                                          PARAM("tcpmpi", tcp_mpi, MPC_CONF_TYPE, "A rail with TCP"),
                                                             PARAM("tbsmmpi", tbsm_mpi, MPC_CONF_TYPE, "A rail with Thread Based SHM"),
#ifdef MPC_USE_PORTALS
	                                                          PARAM("portalsmpi", portals_mpi, MPC_CONF_TYPE, "A rail with Portals 4"),
#endif
#ifdef MPC_USE_OFI
	                                                          PARAM("tcpofirail", tcp_ofi, MPC_CONF_TYPE, "A rail with OFI TCP"),
	                                                          PARAM("verbsofirail", verbs_ofi, MPC_CONF_TYPE, "A rail with OFI Verbs"),
	                                                          PARAM("shmofirail", shm_ofi, MPC_CONF_TYPE, "A rail with OFI SHM"),

#endif
	                                                          NULL);

	return rails;
}

mpc_conf_config_type_t *___new_default_rail(char *name)
{
	return __new_rail_conf_instance(name,
	                                1,
	                                "any",
	                                1,
	                                0,
									0,
	                                0,
	                                1,
	                                "tcpconfigmpi");
}

static inline mpc_conf_config_type_t *___mpc_lowcomm_rail_all(void)
{
	mpc_conf_config_type_elem_t *eall_rails = mpc_conf_root_config_get("mpcframework.lowcomm.networking.rails");

	assume(eall_rails != NULL);
	assume(eall_rails->type == MPC_CONF_TYPE);

	return mpc_conf_config_type_elem_get_inner(eall_rails);
}

static inline mpc_conf_config_type_t *___mpc_lowcomm_rail_instanciate_from_default(mpc_conf_config_type_elem_t *elem)
{
	mpc_conf_config_type_t *default_rail = ___new_default_rail(elem->name);

	/* Here we override with what was already present in the config */
	mpc_conf_config_type_t *current_rail = mpc_conf_config_type_elem_get_inner(elem);

	return mpc_conf_config_type_elem_update(default_rail, current_rail, 1);
}

static inline void ___mpc_lowcomm_rail_conf_validate(void)
{
	/* Now we need to populate rails with possibly new instances
	 * this is done by (1) copying the default struct and then (2)
	 * updating element by element if present in the configuration
	 * this allows partial rail definition */

	mpc_conf_config_type_t *all_rails = ___mpc_lowcomm_rail_all();

	int i;

	for(i = 0; i < mpc_conf_config_type_count(all_rails); i++)
	{
		mpc_conf_config_type_elem_t *rail = mpc_conf_config_type_nth(all_rails, i);
		if(!_mpc_lowcomm_conf_rail_unfolded_get(rail->name) )
		{
			mpc_conf_config_type_t *new_rail = ___mpc_lowcomm_rail_instanciate_from_default(rail);
			mpc_conf_config_type_release( (mpc_conf_config_type_t **)&all_rails->elems[i]->addr);
			all_rails->elems[i]->addr = new_rail;
		}
	}

	/* Now check that rail configs are known */
	for(i = 0; i < mpc_conf_config_type_count(all_rails); i++)
	{
		mpc_conf_config_type_elem_t *rail      = mpc_conf_config_type_nth(all_rails, i);
		mpc_conf_config_type_t *     rail_type = mpc_conf_config_type_elem_get_inner(rail);

		mpc_conf_config_type_elem_t *config = mpc_conf_config_type_get(rail_type, "config");
		assume(config != NULL);
		char *conf_val = mpc_conf_type_elem_get_as_string(config);

		if(!_mpc_lowcomm_conf_driver_unfolded_get(conf_val) )
		{
			mpc_conf_config_type_elem_print(rail, MPC_CONF_FORMAT_XML);
			bad_parameter("There is no driver configuration %s in mpcframework.lowcomm.networing.configs", conf_val);
		}
	}
}

/*_
 * CLI defines group of rails
 */

mpc_conf_config_type_t *_mpc_lowcomm_conf_cli_get(char *name)
{
	return __get_type_by_name("mpcframework.lowcomm.networking.cli.options", name);
}

static mpc_conf_config_type_t *___mpc_lowcomm_cli_conf_option_init(char *name, char *rail1, char *rail2, char * rail3)
{
	char *ar1 = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);

	assume(ar1);
	snprintf(ar1, MPC_CONF_STRING_SIZE, "%s", rail1);

	mpc_conf_config_type_t *rails = NULL;

	if(rail3)
	{
		char *ar2 = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
		assume(ar2);
		snprintf(ar2, MPC_CONF_STRING_SIZE, "%s", rail2);

		char *ar3 = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
		assume(ar3);
		snprintf(ar3, MPC_CONF_STRING_SIZE, "%s", rail3);

		rails = mpc_conf_config_type_init(name,
		                                  PARAM("first", ar1, MPC_CONF_STRING, "First rail to pick"),
		                                  PARAM("second", ar2, MPC_CONF_STRING, "Second rail to pick"),
		                                  PARAM("third", ar3, MPC_CONF_STRING, "Third rail to pick"),
		                                  NULL);
	}
	else if(rail2)
	{
		char *ar2 = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
		assume(ar2);
		snprintf(ar2, MPC_CONF_STRING_SIZE, "%s", rail2);

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
	                                                           PARAM("shm", ___mpc_lowcomm_cli_conf_option_init("shm", "tbsmmpi", "shmmpi", NULL), MPC_CONF_TYPE, "Combination of TBSM and SHM"),

#ifdef MPC_USE_OFI
	                                                           PARAM("tcpshm", ___mpc_lowcomm_cli_conf_option_init("tcpshm", "tbsmmpi", "shmmpi", "tcpofirail"), MPC_CONF_TYPE, "Combination of TCP and TBSM"),
	                                                           PARAM("tcp", ___mpc_lowcomm_cli_conf_option_init("tcp", "tbsmmpi", "tcpofirail", NULL), MPC_CONF_TYPE, "Combination of TCP and TBSM"),
	                                                           PARAM("verbs", ___mpc_lowcomm_cli_conf_option_init("verbs", "tbsmmpi", "verbsofirail", NULL), MPC_CONF_TYPE, "Combination of Verbs and TBSM"),
	                                                           PARAM("verbsshm", ___mpc_lowcomm_cli_conf_option_init("verbsshm", "tbsmmpi", "shmmpi", "verbsofirail"), MPC_CONF_TYPE, "Combination of Verbs and TBSM"),
#else
	                                                           PARAM("tcp", ___mpc_lowcomm_cli_conf_option_init("tcp", "tbsmmpi", "tcpmpi", NULL), MPC_CONF_TYPE, "TCP Alone"),
#endif
#ifdef MPC_USE_PORTALS
	                                                           PARAM("portals4", ___mpc_lowcomm_cli_conf_option_init("portals4", "tbsmmpi", "portalsmpi", NULL), MPC_CONF_TYPE, "Combination of Portals and SHM"),
#endif
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
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "portals4");
#elif MPC_USE_OFI
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "tcpshm");
#else
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "tcp");
#endif
}

static mpc_conf_config_type_t *__mpc_lowcomm_network_conf_init(void)
{
	_mpc_lowcomm_net_config_default();

	mpc_conf_config_type_t *cli = __mpc_lowcomm_cli_conf_init();

	mpc_conf_config_type_t *rails = __mpc_lowcomm_rail_conf_init();

	mpc_conf_config_type_t *configs = __mpc_lowcomm_driver_conf_init();

	mpc_conf_config_type_t *network = mpc_conf_config_type_init("networking",
	                                                            PARAM("configs", configs, MPC_CONF_TYPE, "Driver configurations for Networks"),
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

		char *rail_name = mpc_conf_type_elem_get_as_string(elem);

		/* Now check that the rail does exist */
		if(!_mpc_lowcomm_conf_conf_rail_get(rail_name) )
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
	___mpc_lowcomm_driver_conf_validate();
	___mpc_lowcomm_rail_conf_validate();
	___mpc_lowcomm_cli_conf_validate();
}

/************************************
* GLOBAL CONFIGURATION FOR LOWCOMM *
************************************/

struct _mpc_lowcomm_config __lowcomm_conf;

struct _mpc_lowcomm_config *_mpc_lowcomm_conf_get(void)
{
	return &__lowcomm_conf;
}

static struct _mpc_lowcomm_config_struct_protocol __protocol_conf;

static struct _mpc_lowcomm_config_struct_protocol *__mpc_lowcomm_proto_conf_init()
{
	memset(&__protocol_conf, 0, sizeof(struct _mpc_lowcomm_config_struct_protocol) );
	return &__protocol_conf;
}

struct _mpc_lowcomm_config_struct_protocol *_mpc_lowcomm_config_proto_get(void)
{
	return &__protocol_conf;
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_protocol_conf_init(void)
{
	struct _mpc_lowcomm_config_struct_protocol *proto = __mpc_lowcomm_proto_conf_init();

	proto->multirail_enabled    = 1; /* default multirail enabled */
	proto->rndv_mode            = 1; /* default rndv get */
	proto->offload              = 0; /* default no offload */
	proto->max_mmu_entries = 1024;
	proto->mmu_max_size = 4294967296ull;

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("protocol",
	                                                        PARAM("verbosity", &mpc_common_get_flags()->verbosity, MPC_CONF_INT, "Debug level message (1-3)"),
	                                                        PARAM("rndvmode", &proto->rndv_mode, MPC_CONF_INT, "Type of rendez-vous to use (default: mode get)"),
	                                                        PARAM("offload", &proto->offload, MPC_CONF_INT, "Force offload if possible (ie offload interface available)"),
	                                                        PARAM("multirailenabled", &proto->multirail_enabled, MPC_CONF_INT, "Is multirail enabled ?"),
	                                                        PARAM("maxmmuentries", &proto->max_mmu_entries, MPC_CONF_INT, "Maximum number of entries in the pinning cache"),
	                                                        PARAM("maxmmusize", &proto->mmu_max_size, MPC_CONF_LONG_INT, "Maximum size of the pinning cache"),
	                                                        NULL);

	return ret;
}

static inline mpc_conf_config_type_t *__init_workshare_conf(void)
{
	struct _mpc_lowcomm_workshare_config *ws = &__lowcomm_conf.workshare;

	ws->schedule         = 2;
	ws->steal_schedule   = 2;
	ws->chunk_size       = 1;
	ws->steal_chunk_size = 1;
	ws->enable_stealing  = 1;
	ws->steal_from_end   = 0;
	ws->steal_mode       = WS_STEAL_MODE_ROUNDROBIN;

	char *tmp;

	if( (tmp = getenv("WS_SCHEDULE") ) != NULL)
	{
		int ok          = 0;
		int offset      = 0;
		int ws_schedule = 2;
		if(strncmp(tmp, "dynamic", 7) == 0)
		{
			ws_schedule = 1;
			offset      = 7;
			ok          = 1;
		}
		if(strncmp(tmp, "guided", 6) == 0)
		{
			ws_schedule = 2;
			offset      = 6;
			ok          = 1;
		}
		if(strncmp(tmp, "static", 6) == 0)
		{
			ws_schedule = 0;
			offset      = 6;
			ok          = 1;
		}
		ws->schedule = ws_schedule;
		if(ok)
		{
			int chunk_size = 0;
			/* Check for chunk size, if present */
			switch(tmp[offset])
			{
				case ',':
					chunk_size = atoi(&tmp[offset + 1]);
					if(chunk_size <= 0)
					{
						fprintf(stderr, "Warning: Incorrect chunk size within WS_SCHEDULE "
						                "variable: <%s>\n",
						        tmp);
						chunk_size = 0;
					}
					else
					{
						ws->chunk_size = chunk_size;
					}
					break;

				case '\0':
					ws->chunk_size = 1;
					break;

				default:
					fprintf(stderr,
					        "Syntax error with environment variable WS_SCHEDULE <%s>,"
					        " should be \"static|dynamic|guided[,chunk_size]\"\n",
					        tmp);
					exit(1);
			}
		}
		else
		{
			fprintf(stderr, "Warning: Unknown schedule <%s> (must be guided, "
			                "dynamic or static),"
			                " fallback to default schedule <guided>\n",
			        tmp);
		}
	}

	if( (tmp = getenv("WS_STEAL_SCHEDULE") ) != NULL)
	{
		int ok                = 0;
		int offset            = 0;
		int ws_steal_schedule = 2;
		if(strncmp(tmp, "dynamic", 7) == 0)
		{
			ws_steal_schedule = 1;
			offset            = 7;
			ok = 1;
		}
		if(strncmp(tmp, "guided", 6) == 0)
		{
			ws_steal_schedule = 2;
			offset            = 6;
			ok = 1;
		}
		if(strncmp(tmp, "static", 6) == 0)
		{
			ws_steal_schedule = 0;
			offset            = 6;
			ok = 1;
		}
		ws->steal_schedule = ws_steal_schedule;
		if(ok)
		{
			int chunk_size = 0;
			/* Check for chunk size, if present */
			switch(tmp[offset])
			{
				case ',':
					chunk_size = atoi(&tmp[offset + 1]);
					if(chunk_size <= 0)
					{
						fprintf(stderr, "Warning: Incorrect chunk size within WS_STEAL_SCHEDULE "
						                "variable: <%s>\n",
						        tmp);
						chunk_size = 0;
					}
					else
					{
						ws->steal_chunk_size = chunk_size;
					}
					break;

				case '\0':
					ws->chunk_size = 1;
					break;

				default:
					fprintf(stderr,
					        "Syntax error with environment variable WS_STEAL_SCHEDULE <%s>,"
					        " should be \"dynamic|guided[,chunk_size]\"\n",
					        tmp);
					exit(1);
			}
		}
		else
		{
			fprintf(stderr, "Warning: Unknown schedule <%s> (must be guided "
			                "or dynamic),"
			                " fallback to default schedule <guided>\n",
			        tmp);
		}
	}

	/******* WS STEAL MODE *********/
	if( (tmp = getenv("WS_STEAL_MODE") ) != NULL)
	{
		int ws_steal_mode = strtol(tmp, NULL, 10);
		if(isdigit(tmp[0]) && ws_steal_mode >= 0 &&
		   ws_steal_mode < WS_STEAL_MODE_COUNT)
		{
			ws->steal_mode = ws_steal_mode;
		}
		else
		{
			if( (strcmp(tmp, "random") == 0) )
			{
				ws->steal_mode = WS_STEAL_MODE_RANDOM;
			}
			else if(strcmp(tmp, "roundrobin") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_ROUNDROBIN;
			}
			else if(strcmp(tmp, "producer") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_PRODUCER;
			}
			else if(strcmp(tmp, "less_stealers") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_LESS_STEALERS;
			}
			else if(strcmp(tmp, "less_stealers_producer") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_LESS_STEALERS_PRODUCER;
			}
			else if(strcmp(tmp, "topological") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_TOPOLOGICAL;
			}
			else if(strcmp(tmp, "strictly_topological") == 0)
			{
				ws->steal_mode = WS_STEAL_MODE_STRICTLY_TOPOLOGICAL;
			}
		}
	}

	if( (tmp = getenv("WS_STEAL_FROM_END") ) != NULL)
	{
		if(strcmp(tmp, "1") == 0 || strcmp(tmp, "TRUE") == 0 || strcmp(tmp, "true") == 0)
		{
			ws->steal_from_end = 1;
		}
		else
		{
			ws->steal_from_end = 0;
		}
	}

	/******* ENABLING WS *********/
	if( (tmp = getenv("WS_ENABLE_STEALING") ) != NULL)
	{
		if(strcmp(tmp, "1") == 0 || strcmp(tmp, "TRUE") == 0 || strcmp(tmp, "true") == 0)
		{
			ws->enable_stealing = 1;
		}
		else
		{
			ws->enable_stealing = 0;
		}
	}

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("workshare",
	                                                        PARAM("enablestealing", &ws->enable_stealing, MPC_CONF_INT, "Defines if workshare stealing is enabled."),
	                                                        PARAM("stealmode", &ws->steal_mode, MPC_CONF_INT, "Workshare stealing mode"),
	                                                        PARAM("stealfromend", &ws->steal_from_end, MPC_CONF_INT, "Stealing from end or not"),
	                                                        PARAM("schedule", &ws->schedule, MPC_CONF_INT, "Workshare schedule"),
	                                                        PARAM("stealschedule", &ws->steal_schedule, MPC_CONF_INT, "Workshare steal schedule"),
	                                                        PARAM("chunksize", &ws->chunk_size, MPC_CONF_INT, "Workshare chunk size"),
	                                                        PARAM("stealchunksize", &ws->steal_chunk_size, MPC_CONF_INT, "Workshare chunk size"),
	                                                        NULL);

	assume(ret != NULL);
	return ret;
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

	mpc_conf_config_type_t *coll      = __mpc_lowcomm_coll_conf_init();
	mpc_conf_config_type_t *networks  = __mpc_lowcomm_network_conf_init();
	mpc_conf_config_type_t *protocol  = __mpc_lowcomm_protocol_conf_init();
	mpc_conf_config_type_t *workshare = __init_workshare_conf();


	mpc_conf_config_type_t *lowcomm = mpc_conf_config_type_init("lowcomm",
#ifdef SCTK_USE_CHECKSUM
	                                                            PARAM("checksum", &__lowcomm_conf.checksum, MPC_CONF_BOOL, "Enable buffer checksum for P2P messages"),
#endif
	                                                            PARAM("coll", coll, MPC_CONF_TYPE, "Lowcomm collective configuration"),
	                                                            PARAM("protocol", protocol, MPC_CONF_TYPE, "Lowcomm protocol configuration"),
	                                                            PARAM("networking", networks, MPC_CONF_TYPE, "Lowcomm Networking configuration"),
	                                                            PARAM("workshare", workshare, MPC_CONF_TYPE, "Workshare configuration"),
	                                                            NULL);

	mpc_conf_root_config_append("mpcframework", lowcomm, "MPC Lowcomm Configuration");
}

void _mpc_lowcomm_config_validate(void)
{
	__mpc_lowcomm_coll_conf_validate();
	__mpc_lowcomm_network_conf_validate();
}
