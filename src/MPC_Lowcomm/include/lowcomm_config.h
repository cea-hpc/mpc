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


/********************************** ENUM ************************************/
/**Values used for topological polling in the rail configuration**/
typedef enum
{
	RAIL_POLL_NOT_SET = 0,  /**< This is for error handling */
	RAIL_POLL_NONE,  /**< This is to deactivate polling */
	RAIL_POLL_PU,  /**< This is for polling at core level */
	RAIL_POLL_CORE,  /**< This is for polling at core level */
	RAIL_POLL_SOCKET, /**< Socket level */
	RAIL_POLL_NUMA, /**< Numa level */
	RAIL_POLL_MACHINE /**< Machine level */
}rail_topological_polling_level_t;

/******************************** STRUCTURE *********************************/
/**Options for MPC Fault-Tolerance module.**/
struct _mpc_lowcomm_config_struct_ft
{
	/**Set to true to enable Fault-Tolerance support**/
	int enabled;
};


/********************************** ENUM ************************************/
/****/
enum _mpc_lowcomm_ofi_mode
{
	MPC_LOWCOMM_OFI_CONNECTED,
	MPC_LOWCOMM_OFI_CONNECTIONLESS
};

/********************************** ENUM ************************************/
/****/
enum _mpc_lowcomm_ofi_ep_type
{
	MPC_LOWCOMM_OFI_EP_MSG,
	MPC_LOWCOMM_OFI_EP_RDM,
	MPC_LOWCOMM_OFI_EP_UNSPEC
};

/********************************** ENUM ************************************/
/****/
enum _mpc_lowcomm_ofi_av_type
{
	MPC_LOWCOMM_OFI_AV_TABLE,
	MPC_LOWCOMM_OFI_AV_MAP,
	MPC_LOWCOMM_OFI_AV_UNSPEC
};

/********************************** ENUM ************************************/
/****/
enum _mpc_lowcomm_ofi_progress
{
	MPC_LOWCOMM_OFI_PROGRESS_MANUAL,
	MPC_LOWCOMM_OFI_PROGRESS_AUTO,
	MPC_LOWCOMM_OFI_PROGRESS_UNSPEC
};

/********************************** ENUM ************************************/
/****/
enum _mpc_lowcomm_ofi_rm_type
{
	MPC_LOWCOMM_OFI_RM_ENABLED,
	MPC_LOWCOMM_OFI_RM_DISABLED,
	MPC_LOWCOMM_OFI_RM_UNSPEC
};

struct _mpc_lowcomm_config_struct_net_driver_ofi
{
	/****/
	char                           slink[MPC_CONF_STRING_SIZE];
	enum _mpc_lowcomm_ofi_mode     link;
	/****/
	char                           sprogress[MPC_CONF_STRING_SIZE];
	enum _mpc_lowcomm_ofi_progress progress;
	/****/
	char                           sep_type[MPC_CONF_STRING_SIZE];
	enum _mpc_lowcomm_ofi_ep_type  ep_type;
	/****/
	char                           sav_type[MPC_CONF_STRING_SIZE];
	enum _mpc_lowcomm_ofi_av_type  av_type;
	/****/
	char                           srm_type[MPC_CONF_STRING_SIZE];
	enum _mpc_lowcomm_ofi_rm_type  rm_type;
	/****/
	char                           provider[MPC_CONF_STRING_SIZE];
};


/******************************** STRUCTURE *********************************/
/**Declare a fake driver to test the configuration system.**/
struct _mpc_lowcomm_config_struct_net_driver_infiniband
{
	/**Define the pkey value**/
	char                   pkey[MPC_CONF_STRING_SIZE];
	/**Defines the port number to use.**/
	int                    adm_port;
	/**Defines the verbose level of the Infiniband interface .**/
	int                    verbose_level;
	/**Size of the eager buffers (short messages).**/
	size_t                 eager_limit;
	/**Max size for using the Buffered protocol (message split into several Eager messages).**/
	size_t                 buffered_limit;
	/**Number of entries to allocate in the QP for sending messages. If too low, may cause an QP overrun**/
	int                    qp_tx_depth;
	/**Number of entries to allocate in the QP for receiving messages. Must be 0 if using SRQ**/
	int                    qp_rx_depth;
	/**Number of entries to allocate in the CQ. If too low, may cause a CQ overrun**/
	int                    cq_depth;
	/**Number of RDMA resources on QP (covers both max_dest_rd_atomic and max_rd_atomic)**/
	int                    rdma_depth;
	/**Max pending RDMA operations for send**/
	int                    max_sg_sq;
	/**Max pending RDMA operations for recv**/
	int                    max_sg_rq;
	/**Max size for inlining messages**/
	size_t                 max_inline;
	/**Defines if RDMA connections may be resized.**/
	int                    rdma_resizing;
	/**Number of RDMA buffers allocated for each neighbor**/
	int                    max_rdma_connections;
	/**Max number of Eager buffers to allocate during the initialization step**/
	int                    init_ibufs;
	/**Defines the number of receive buffers initially allocated. The number is on-the-fly expanded when needed (see init_recv_ibufs_chunk)**/
	int                    init_recv_ibufs;
	/**Max number of Eager buffers which can be posted to the SRQ. This number cannot be higher than the number fixed by the HW**/
	int                    max_srq_ibufs_posted;
	/**Min number of free recv Eager buffers before the activation of the asynchronous thread. If this thread is activated too many times, the performance may be decreased.**/
	int                    srq_credit_thread_limit;
	/**Number of new buffers allocated when no more buffers are available.**/
	int                    size_ibufs_chunk;
	/**Defines if the Infiniband interface must crash quietly.**/
	int                    quiet_crash;
	/**Defines if the asynchronous may be started at the MPC initialization.**/
	int                    async_thread;
	/**Defines the minimum size for the Eager RDMA buffers**/
	size_t                 rdma_min_size;
	/**Defines the maximun size for the Eager RDMA buffers**/
	size_t                 rdma_max_size;
	/**Defines the minimum number of Eager RDMA buffers**/
	int                    rdma_min_nb;
	/**Defines the maximum number of Eager RDMA buffers**/
	int                    rdma_max_nb;
	/**Defines the minimum size for the Eager RDMA buffers (resizing)**/
	size_t                 rdma_resizing_min_size;
	/**Defines the maximum size for the Eager RDMA buffers (resizing)**/
	size_t                 rdma_resizing_max_size;
	/**Defines the minimum number of Eager RDMA buffers (resizing)**/
	int                    rdma_resizing_min_nb;
	/**Defines the maximum number of Eager RDMA buffers (resizing)**/
	int                    rdma_resizing_max_nb;
	/**Defines the number of receive buffers allocated on the fly.**/
	int                    size_recv_ibufs_chunk;
};

/******************************** STRUCTURE *********************************/
/**Global Parameters for IB common structs.**/
struct _mpc_lowcomm_config_struct_ib_global
{
	/**Defines if the MMU cache is enabled.**/
	int    mmu_cache_enabled;
	/**Number of entries to keep in the cache.**/
	int    mmu_cache_entry_count;
	/**Total size of entries to keep in the cache.**/
	size_t mmu_cache_maximum_size;
	/**Maximum size of an pinned entry.**/
	size_t mmu_cache_maximum_pin_size;
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
	/**Set max registerable size (default: SIZE_MAX)**/
	int                                             max_msg_size;
	/**List of available optimizations taking advantage of triggered Ops**/
	struct _mpc_lowcomm_config_struct_offload_ops_t offloading;
};

/******************************** STRUCTURE *********************************/
/**TCP-based driver**/
struct _mpc_lowcomm_config_struct_net_driver_tcp
{
        size_t max_msg_size;
};

/******************************** STRUCTURE *********************************/
/**TCP-Based RDMA implementation**/
struct _mpc_lowcomm_config_struct_net_driver_tcp_rdma
{
};

/******************************** STRUCTURE *********************************/
/**Inter-Process shared memory communication implementation**/
struct _mpc_lowcomm_config_struct_net_driver_shm
{
	/**Defines priority for the SHM buffered message**/
	int buffered_priority;
	/**Defines the min size for the SHM buffered message**/
	int buffered_min_size;
	/**Defines the min size for the SHM buffered message**/
	int buffered_max_size;
	/**Defines if mode zerocopy should be actived for SHM buffered message**/
	int buffered_zerocopy;
	/****/
	int cma_enable;
	/**Defines priority for the SHM CMA message**/
	int cma_priority;
	/**Defines the min size for the SHM CMA message**/
	int cma_min_size;
	/**Defines the min size for the SHM CMA message**/
	int cma_max_size;
	/**Defines if mode zerocopy should be actived for SHM CMA message**/
	int cma_zerocopy;
	/**Defines priority for the SHM fragmented message**/
	int frag_priority;
	/**Defines the min size for the SHM fragmented message**/
	int frag_min_size;
	/**Defines the min size for the SHM fragmented message**/
	int frag_max_size;
	/**Defines if mode zerocopy should be actived for SHM fragmented message**/
	int frag_zerocopy;
	/**Size of shared memory region.**/
	int shmem_size;
	/**Size of shared memory region.**/
	int cells_num;
};

/********************************** ENUM ************************************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum _mpc_lowcomm_config_struct_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_portals,
	SCTK_RTCFG_net_driver_tcp,
	SCTK_RTCFG_net_driver_shm,
	SCTK_RTCFG_net_driver_ofi,
	SCTK_RTCFG_net_driver_topological,
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct _mpc_lowcomm_config_struct_net_driver
{
	enum _mpc_lowcomm_config_struct_net_driver_type type;
	union
	{
#ifdef MPC_USE_INFINIBAND
		struct _mpc_lowcomm_config_struct_net_driver_infiniband  infiniband;
#endif
#ifdef MPC_USE_PORTALS
		struct _mpc_lowcomm_config_struct_net_driver_portals     portals;
#endif
#ifdef MPC_USE_OFI
		struct _mpc_lowcomm_config_struct_net_driver_ofi ofi;
#endif
		struct _mpc_lowcomm_config_struct_net_driver_tcp         tcp;
		struct _mpc_lowcomm_config_struct_net_driver_shm         shm;
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
typedef struct _mpc_lowcomm_config_struct_net_driver_config lcr_driver_config_t;

/**********************
* RAIL CONFIGURATION *
**********************/

struct sctk_runtime_config_funcptr
{
	char *name;
	void  (*value)();
};


/******************************** STRUCTURE *********************************/
/**This gate applies given thruth value to messages.**/
struct _mpc_lowcomm_config_struct_gate_boolean
{
	/**whereas to accept input messages or not**/
	int value;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a parametrized probability.**/
struct _mpc_lowcomm_config_struct_gate_probabilistic
{
	/**Probability to choose this rail in percents (ralatively to this single rail, integer)**/
	int probability;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at least a given value.**/
struct _mpc_lowcomm_config_struct_gate_min_size
{
	/**Minimum size to choose this rail (with units)**/
	size_t value;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at most a given value.**/
struct _mpc_lowcomm_config_struct_gate_max_size
{
	/**Maximum size to choose this rail (with units)**/
	size_t value;
};

/******************************** STRUCTURE *********************************/
/**This gate can be used define which type of message can use a given rail.**/
struct _mpc_lowcomm_config_struct_gate_message_type
{
	/**Process Specific Messages can use this rail**/
	int process;
	/**Task specific messages can use this rail**/
	int task;
	/**Task specific messages can use this rail**/
	int emulated_rma;
	/**Common messages (MPI) can use this rail**/
	int common;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a user defined function.**/
struct _mpc_lowcomm_config_struct_gate_user
{
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/********************************** ENUM ************************************/
/**Defines gates and their configuration.**/
enum _mpc_lowcomm_config_struct_net_gate_type
{
	MPC_CONF_RAIL_GATE_NONE,
	MPC_CONF_RAIL_GATE_BOOLEAN,
	MPC_CONF_RAIL_GATE_PROBABILISTIC,
	MPC_CONF_RAIL_GATE_MINSIZE,
	MPC_CONF_RAIL_GATE_MAXSIZE,
	MPC_CONF_RAIL_GATE_MSGTYPE,
	MPC_CONF_RAIL_GATE_USER,
};

/******************************** STRUCTURE *********************************/
/**Defines gates and their configuration.**/
struct _mpc_lowcomm_config_struct_net_gate
{
	enum _mpc_lowcomm_config_struct_net_gate_type type;
	union
	{
		struct _mpc_lowcomm_config_struct_gate_boolean       boolean;
		struct _mpc_lowcomm_config_struct_gate_probabilistic probabilistic;
		struct _mpc_lowcomm_config_struct_gate_min_size      minsize;
		struct _mpc_lowcomm_config_struct_gate_max_size      maxsize;
		struct _mpc_lowcomm_config_struct_gate_message_type  msgtype;
		struct _mpc_lowcomm_config_struct_gate_user          user;
	}                                             value;
};

/******************************** STRUCTURE *********************************/
/**Defines a topological polling configuration.**/
struct _mpc_lowcomm_config_struct_topological_polling
{
	/** Polling range as string */
	char srange[MPC_CONF_STRING_SIZE];
	/**Define the subset of cores involved in the polling.**/
	rail_topological_polling_level_t range;

	/** Polling trigger as string */
	char strigger[MPC_CONF_STRING_SIZE];
	/**Define the subset of cores involved in the polling.**/
	rail_topological_polling_level_t trigger;
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
	/**Define how the any-source polling is done.**/
	struct _mpc_lowcomm_config_struct_topological_polling any_source_polling;
	/**Define the network topology to apply on this rail.**/
	char                                                  topology[MPC_CONF_STRING_SIZE];
	/**Define if on-demand connections are allowed on this rail.**/
	int                                                   ondemand;
	/**Defines if the rail has RDMA enabled.**/
	int                                                   rdma;
	/**Define the driver config to use for this rail.**/
	char                                                 config[MPC_CONF_STRING_SIZE];
	/**List of gates to be applied in this config.**/
	struct _mpc_lowcomm_config_struct_net_gate            gates[MPC_CONF_MAX_RAIL_GATE];
	/** Number of elements in gates array. **/
	int                                                   gates_size;
	/**Used for topological rail selection**/
	struct _mpc_lowcomm_config_struct_net_rail *          subrails;
	/** Number of elements in subrails array. **/
	int                                                   subrails_size;
	/** Max number of rail instance. **/
        int                                                   max_ifaces;
	/** Define if rail has tag offloading capabilities. **/
        int                                                   offload;
};
typedef struct _mpc_lowcomm_config_struct_net_rail lcr_rail_config_t;

/******************************** STRUCTURE *********************************/
/**Base structure to contain the protocol configuration**/
#define MPC_CONF_MAX_LCR 5
struct _mpc_lowcomm_config_struct_net_lcr
{
	char name[MPC_CONF_STRING_SIZE]; /* CLI name needed to get its config */
	int count; /* Number of rails for this CLI */
};

struct _mpc_lowcomm_config_struct_protocol
{
	/**Is multirail enabled**/
	int  multirail_enabled;
	/**Comma separated list of transport to use (default: tcp).**/
	char transports[MPC_CONF_STRING_SIZE];
	/**Comma separated list of devices to use (default: eth0, ptl0).**/
	char devices[MPC_CONF_STRING_SIZE];
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
#ifdef MPC_USE_INFINIBAND
	struct _mpc_lowcomm_config_struct_ib_global infiniband;
#endif
	struct _mpc_lowcomm_workshare_config workshare;
	struct _mpc_lowcomm_config_mem_pool memorypool;
};

struct _mpc_lowcomm_config *_mpc_lowcomm_conf_get(void);

struct _mpc_lowcomm_config_struct_net_driver_config * _mpc_lowcomm_conf_driver_unfolded_get(char * name);


void _mpc_lowcomm_config_register(void);
void _mpc_lowcomm_config_validate(void);


#endif /* LOWCOMM_CONFIG_H_ */
