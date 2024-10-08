/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef LOWCOMM_CONFIG_H_
#define LOWCOMM_CONFIG_H_

#include <mpc_config.h>
#include <mpc_lowcomm_types.h>
#include <mpc_conf.h>

/*******************
* COLLECTIVE CONF *
*******************/

struct _mpc_lowcomm_coll_conf
{
	char algorithm[MPC_CONF_STRING_SIZE];
	void   ( *mpc_lowcomm_coll_init_hook )(mpc_lowcomm_communicator_t id);

	/* Barrier */
	int    barrier_arity;

	/* Bcast */
	size_t bcast_max_size;
	int    bcast_max_arity;
	size_t bcast_check_threshold;

	/* Allreduce */
	size_t allreduce_max_size;
	int    allreduce_max_arity;
	size_t allreduce_check_threshold;
	int    allreduce_max_slots;

	/* SHM */
	int    shm_reduce_interleave;
	int    shm_reduce_pipelined_blocks;
	int    shm_bcast_interleave;
};

struct _mpc_lowcomm_coll_conf *_mpc_lowcomm_coll_conf_get(void);

void mpc_lowcomm_coll_init_hook(mpc_lowcomm_communicator_t id);

/************************
* DRIVER CONFIGURATION *
************************/

/******************************** STRUCTURE *********************************/
/**Options for MPC Fault-Tolerance module.**/
struct _mpc_lowcomm_config_struct_ft
{
	/**Set to true to enable Fault-Tolerance support**/
	int enabled;
};


/******************************** STRUCTURE *********************************/
/****/
struct _mpc_lowcomm_config_struct_offload_ops_t
{
	/**Enable on-demand optimization through ID hardware propagation**/
	int ondemand;
	/**Enable collective optimization for Portals**/
	int collectives;
};

struct _mpc_lowcomm_config_struct_control_flow_t
{
	/**Maximum number of token per chunk, eg maximum number of that can be
         * granted to one remote process.**/
	int max_chunk;
	/**Minimum number of token per chunk, eg maximum number of that can be
         * granted to one remote process.**/
	int min_chunk;
	/**Maximum number of token.**/
	int max_tokens;
	/**Enable collective optimization for Portals**/
	int collectives;
};

/******************************** STRUCTURE *********************************/
/**Portals-based driver**/
struct _mpc_lowcomm_config_struct_net_driver_portals
{
	/**Max size of messages allowed to use the eager protocol.**/
	size_t                                          eager_limit;
	/**Min number of communicators (help to avoid dynamic PT entry allocation)**/
	int                                             min_comms;
	/**Above this value, RDV messages will be split in multiple GET requests**/
	size_t                                          block_cut;
	/**Size of eager blocks **/
	size_t                                          eager_block_size;
	/**Number of eager blocks **/
	int                                             num_eager_blocks;
	/**Max iovec**/
	int                                             max_iovecs;
	/**Set max registerable size (default: INT_MAX)**/
	size_t                                          max_msg_size;
	/**Set min fragment size when using multirail(default: INT_MAX)**/
	size_t                                          min_frag_size;
	/**Should the driver be used with matching offload.**/
	int                                             offload;
	/**List of available optimizations taking advantage of triggered Ops**/
	struct _mpc_lowcomm_config_struct_offload_ops_t offloading;
	/**Configuration for control flow**/
	struct _mpc_lowcomm_config_struct_control_flow_t control_flow;
};


/** OFI-based driver */
struct _mpc_lowcomm_config_struct_net_driver_ofi
{
	unsigned int request_cache_size;
	unsigned int eager_size;
	unsigned int eager_per_buff;
	int enable_multi_recv;
	unsigned int number_of_multi_recv_buff;
	unsigned int bcopy_size;
	char endpoint_type[MPC_CONF_STRING_SIZE];
	char provider[MPC_CONF_STRING_SIZE];
};

/******************************** STRUCTURE *********************************/
/**TCP-based driver**/
struct _mpc_lowcomm_config_struct_net_driver_tcp
{
        size_t max_msg_size;
};

/******************************** STRUCTURE *********************************/
/**Thread-based Shared Memory (TBSM) driver**/
struct _mpc_lowcomm_config_struct_net_driver_tbsm
{
        size_t max_iov;
        size_t eager_limit;
        size_t max_msg_size;
        int    bcopy_buf_size;
};

/******************************** STRUCTURE *********************************/
/**TCP-Based RDMA implementation**/
struct _mpc_lowcomm_config_struct_net_driver_tcp_rdma
{
};


/********************************** ENUM ************************************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum _mpc_lowcomm_config_struct_net_driver_type
{
	MPC_LOWCOMM_CONFIG_DRIVER_NONE,
	MPC_LOWCOMM_CONFIG_DRIVER_PORTALS,
	MPC_LOWCOMM_CONFIG_DRIVER_TCP,
	MPC_LOWCOMM_CONFIG_DRIVER_TBSM,
	MPC_LOWCOMM_CONFIG_DRIVER_OFI,
	MPC_LOWCOMM_CONFIG_DRIVER_SHM,
	MPC_LOWCOMM_CONFIG_DRIVER_COUNT
};

static const char * const _mpc_lowcomm_config_struct_net_driver_type_name[MPC_LOWCOMM_CONFIG_DRIVER_COUNT] =
{
	"none",
	"ptl",
	"tcp",
	"tbsm",
	"ofi",
	"shm"
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct _mpc_lowcomm_config_struct_net_driver
{
	enum _mpc_lowcomm_config_struct_net_driver_type type;
	union
	{
#ifdef MPC_USE_OFI
		struct _mpc_lowcomm_config_struct_net_driver_ofi         ofi;
#endif
#ifdef MPC_USE_PORTALS
		struct _mpc_lowcomm_config_struct_net_driver_portals     portals;
#endif
		struct _mpc_lowcomm_config_struct_net_driver_tcp         tcp;
		struct _mpc_lowcomm_config_struct_net_driver_tbsm        tbsm;
	}                                               value;
};

/******************************** STRUCTURE *********************************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct _mpc_lowcomm_config_struct_net_driver_config
{
	/**Name of the driver configuration to be referenced in rail definitions.**/
	char                                        name[MPC_CONF_STRING_SIZE];
	/**Define the related driver to use and its configuration.**/
	struct _mpc_lowcomm_config_struct_net_driver driver;
};

/**********************
* RAIL CONFIGURATION *
**********************/

struct sctk_runtime_config_funcptr
{
	char *name;
	void  (*value)();
};


/******************************** STRUCTURE *********************************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/

#define MPC_CONF_MAX_RAIL_GATE 16

struct _mpc_lowcomm_config_struct_net_rail
{
	/**Define the name of current rail.**/
	char                                                  name[MPC_CONF_STRING_SIZE];
	/**Number which defines the order in which routes are tested (higher first).**/
	int                                                   priority;
	/**Define the name of the device to use in this rail.**/
	char                                                 device[MPC_CONF_STRING_SIZE];
	/**Define if on-demand connections are allowed on this rail.**/
	int                                                   ondemand;
	/**Defines if the rail has RDMA enabled.**/
	int                                                   rdma;
	/**Defines if the rail can send to self.**/
	int                                                   self;
	/**Define the driver config to use for this rail.**/
	char                                                 config[MPC_CONF_STRING_SIZE];
	/** Max number of rail instance. **/
        int                                                   max_ifaces;
	/** Define if rail has tag offloading capabilities. **/
        int                                                   offload;
	/** Define if rail is composable, only intranode comm. **/
        int                                                   composable;
};

/******************************** STRUCTURE *********************************/
/**Base structure to contain the protocol configuration**/
struct _mpc_lowcomm_config_struct_protocol
{
	/**Is multirail enabled (default: true)**/
	int  multirail_enabled;
	/**Type of rendez-vous to use (default: mode get).**/
	int  rndv_mode;
	/**Force offload if possible (ie offload interface available)**/
	int  offload;
	/** Maximum number of entries in the MMU */
	int max_mmu_entries;
	/** Maximum size of pinned memory in the MMU */
	size_t mmu_max_size;
};
typedef struct _mpc_lowcomm_config_struct_protocol lcr_protocol_config_t;

/******************************** STRUCTURE *********************************/
/**Base structure to contain the network configuration**/

#define MPC_CONF_MAX_RAIL_COUNT 128
#define MPC_CONF_MAX_CONFIG_COUNT 128

struct _mpc_lowcomm_config_struct_networks
{
	/**Define the configuration driver list to reuse in rail definitions.**/
	struct _mpc_lowcomm_config_struct_net_driver_config * configs[MPC_CONF_MAX_CONFIG_COUNT];
	/** Number of elements in configs array. **/
	int                                                  configs_size;

	/**List of rails to declare in MPC.**/
	struct _mpc_lowcomm_config_struct_net_rail *         rails[MPC_CONF_MAX_RAIL_COUNT];
	/** Number of elements in rails array. **/
	int                                                  rails_size;

	/** Name of the default CLI option to choose */
	char                                               cli_default_network[MPC_CONF_STRING_SIZE];
};

typedef enum
{
	WS_STEAL_MODE_ROUNDROBIN = 0,
	WS_STEAL_MODE_RANDOM,
	WS_STEAL_MODE_PRODUCER,
	WS_STEAL_MODE_LESS_STEALERS,
	WS_STEAL_MODE_LESS_STEALERS_PRODUCER,
	WS_STEAL_MODE_TOPOLOGICAL,
	WS_STEAL_MODE_STRICTLY_TOPOLOGICAL,
	WS_STEAL_MODE_COUNT
} ws_steal_mode;

struct _mpc_lowcomm_workshare_config
{
  /** Workshare steal policy **/
	ws_steal_mode steal_mode;
  /** Stealing from end of the loop or not **/
	int steal_from_end;
  /** Schedule (guided, dynamic, static) **/
	int schedule;
  /** Steal schedule (guided, dynamic, static) when stealing **/
	int steal_schedule;
  /** Chunk size **/
	int chunk_size;
  /** Chunk size when stealing **/
	int steal_chunk_size;
  /** Enable workshare stealing or not **/
  int enable_stealing;
};


struct _mpc_lowcomm_config_struct_protocol *_mpc_lowcomm_config_proto_get(void);

struct _mpc_lowcomm_config_struct_networks *_mpc_lowcomm_config_net_get(void);

/** @brief Helper to get a table of rail config from CLI
*   @return The configuration or NULL
*/
int _mpc_lowcomm_conf_load_rail_from_cli(struct _mpc_lowcomm_config_struct_net_rail ***cli_rail_configs_p,
                                         int *num_configs);

/** @brief Get a pointer to a given CLI configuration
*   @param name Name of the requested configuration
*   @return The configuration or NULL
*/
mpc_conf_config_type_t *_mpc_lowcomm_conf_cli_get( char *name );

/** @brief Get a pointer to a given RAIL configuration
*   @param name Name of the requested configuration
*   @return The configuration or NULL
*/
mpc_conf_config_type_t *_mpc_lowcomm_conf_conf_rail_get ( char *name );

/** @brief Get a pointer to a given rail
*   @param name Name of the requested rail
*   @return The rail or NULL
*/
struct _mpc_lowcomm_config_struct_net_rail * _mpc_lowcomm_conf_rail_unfolded_get ( char *name );


/***************
 * MEMORY POOL *
 ***************/

struct _mpc_lowcomm_config_mem_pool
{
    int enabled;
    long int size;
    int autodetect;
    int force_process_linear;
    long int per_proc_size;
};


/************************************
* GLOBAL CONFIGURATION FOR LOWCOMM *
************************************/

struct _mpc_lowcomm_config
{
#ifdef SCTK_USE_CHECKSUM
	int checksum;
#endif
	struct _mpc_lowcomm_workshare_config workshare;
	struct _mpc_lowcomm_config_mem_pool memorypool;
};

struct _mpc_lowcomm_config *_mpc_lowcomm_conf_get(void);

struct _mpc_lowcomm_config_struct_net_driver_config * _mpc_lowcomm_conf_driver_unfolded_get(char * name);


void _mpc_lowcomm_config_register(void);
void _mpc_lowcomm_config_validate(void);


#endif /* LOWCOMM_CONFIG_H_ */
