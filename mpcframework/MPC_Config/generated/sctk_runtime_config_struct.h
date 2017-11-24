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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_bool.h>
#include "sctk_runtime_config_struct_defaults.h"

#ifndef SCTK_RUNTIME_CONFIG_STRUCT_H
#define SCTK_RUNTIME_CONFIG_STRUCT_H

#define SCTK_RUNTIME_CONFIG_MAX_PROFILES 16

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config_funcptr
{
	char * name;
	void (* value)();
};

/******************************** STRUCTURE *********************************/
/**CUDA-specific configuration**/
struct sctk_runtime_config_struct_accl_cuda
{	int init_done;
	/**Set to true to enable CUDA context-switch**/
	int enabled;
};

/******************************** STRUCTURE *********************************/
/**OpenACC-specific configuration**/
struct sctk_runtime_config_struct_accl_openacc
{	int init_done;
	/**Set to true to enable OpenACC in MPC**/
	int enabled;
};

/******************************** STRUCTURE *********************************/
/**OpenCL-specific configuration**/
struct sctk_runtime_config_struct_accl_opencl
{	int init_done;
	/**Set to true to enable OpenCL in MPC**/
	int enabled;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Accelerators module.**/
struct sctk_runtime_config_struct_accl
{	int init_done;
	/**Set to true to enable Accelerators support**/
	int enabled;
	/**Define CUDA-specific configuration**/
	struct sctk_runtime_config_struct_accl_cuda cuda;
	/**Define OpenACC-specific configuration**/
	struct sctk_runtime_config_struct_accl_openacc openacc;
	/**Define OpenCL-specific configuration**/
	struct sctk_runtime_config_struct_accl_opencl opencl;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC memory allocator.**/
struct sctk_runtime_config_struct_allocator
{	int init_done;
	/**Enable or disable NUMA migration of allocator pages on thread migration.**/
	int numa_migration;
	/**If the new segment is less than N time smaller than factor, realloc will allocate a new segment, otherwise it will keep the same one. Use 1 to force realloc every time (may be slower but consume less memory).**/
	int realloc_factor;
	/**If the new segment is smaller of N bytes than threashold, realloc will allocate a new segment, otherwise it will keep the same one. Use 0 to force realloc every time (may be slower but consume less memory).**/
	size_t realloc_threashold;
	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	int numa;
	/**If true, enable usage of abort() on free error, otherwise try to continue by skipping.**/
	int strict;
	/**Maximum amount of memory to keep in memory sources (one per NUMA node). Use 0 to disable cache, huge value to keep all.**/
	size_t keep_mem;
	/**Maximum size of macro blocs to keep in memory source for reuse. Use 0 to disable cache, huge value to keep all.**/
	size_t keep_max;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC launcher.**/
struct sctk_runtime_config_struct_launcher
{	int init_done;
	/**Default verbosity level from 0 to 3. Can be override by -vv on mpcrun.**/
	int verbosity;
	/**Display the MPC banner at launch time to print some informations about the topology. Can be override by MPC_DISABLE_BANNER.**/
	int banner;
	/**Automatically kill the MPC processes after a given timeout. Use 0 to disable. Can be override by MPC_AUTO_KILL_TIMEOUT.**/
	int autokill;
	/**Permit to extend the launchers available via 'mpcrun -l=...' by providing scripts (named mpcrun_XXXX) in a user directory. Can be override by MPC_USER_LAUNCHERS.**/
	char * user_launchers;
	/**Activate randomization of base addresses**/
	int keep_rand_addr;
	/**Deactivate randomization of base addresses**/
	int disable_rand_addr;
	/**Do not use mpc for execution (deprecated?)**/
	int disable_mpc;
	/**Initialize multithreading mode**/
	struct sctk_runtime_config_funcptr thread_init;
	/**Define the number of MPI tasks**/
	int nb_task;
	/**Define the number of MPC processes**/
	int nb_process;
	/**Define the number of virtual processors**/
	int nb_processor;
	/**Define the number of compute nodes**/
	int nb_node;
	/**Define which launcher to use**/
	char * launcher;
	/**Define the max number of tries to access the topology file before failing**/
	int max_try;
	/**Print the MPC version number**/
	int vers_details;
	/**Select the type of outputs for the profiling**/
	char * profiling;
	/**Enable usage of hyperthreaded cores if available on current architecture.**/
	int enable_smt;
	/**Enable the restriction on CPU number to share node**/
	int share_node;
	/**Restart MPC from a previous checkpoint**/
	int restart;
	/**Enable MPC checkpointing**/
	int checkpoint;
	/**Enable migration**/
	int migration;
	/**Enable reporting.**/
	int report;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Debugger**/
struct sctk_runtime_config_struct_debugger
{	int init_done;
	/**Print colored text in terminal**/
	int colors;
	/****/
	int max_filename_size;
	/**Should MPC capture common signals also connected to the MPC_BT_SIG environment variable which supersedes the config**/
	int mpc_bt_sig;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Fault-Tolerance module.**/
struct sctk_runtime_config_struct_ft
{	int init_done;
	/**Set to true to enable Fault-Tolerance support**/
	int enabled;
};

/********************************** ENUM ************************************/
/****/
enum ibv_rdvz_protocol
{
	IBV_RDVZ_WRITE_PROTOCOL,
	IBV_RDVZ_READ_PROTOCOL
};

/********************************** ENUM ************************************/
/**Values used for topological polling in the rail configuration**/
enum rail_topological_polling_level
{
	RAIL_POLL_NONE,
	RAIL_POLL_PU,
	RAIL_POLL_CORE,
	RAIL_POLL_SOCKET,
	RAIL_POLL_NUMA,
	RAIL_POLL_MACHINE
};

/******************************** STRUCTURE *********************************/
/**Declare a topological driver.**/
struct sctk_runtime_config_struct_net_driver_topological
{	int init_done;
	/**A test Param**/
	int dummy;
};

/******************************** STRUCTURE *********************************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_infiniband
{	int init_done;
	/**Define the pkey value**/
	char * pkey;
	/**Defines the port number to use.**/
	int adm_port;
	/**Defines the verbose level of the Infiniband interface .**/
	int verbose_level;
	/**Size of the eager buffers (short messages).**/
	int eager_limit;
	/**Max size for using the Buffered protocol (message split into several Eager messages).**/
	int buffered_limit;
	/**Number of entries to allocate in the QP for sending messages. If too low, may cause an QP overrun**/
	int qp_tx_depth;
	/**Number of entries to allocate in the QP for receiving messages. Must be 0 if using SRQ**/
	int qp_rx_depth;
	/**Number of entries to allocate in the CQ. If too low, may cause a CQ overrun**/
	int cq_depth;
	/**Number of RDMA resources on QP (covers both max_dest_rd_atomic and max_rd_atomic)**/
	int rdma_depth;
	/**Max pending RDMA operations for send**/
	int max_sg_sq;
	/**Max pending RDMA operations for recv**/
	int max_sg_rq;
	/**Max size for inlining messages**/
	int max_inline;
	/**Defines if RDMA connections may be resized.**/
	int rdma_resizing;
	/**Number of RDMA buffers allocated for each neighbor**/
	int max_rdma_connections;
	/**Max number of RDMA buffers resizing allowed**/
	int max_rdma_resizing;
	/**Max number of Eager buffers to allocate during the initialization step**/
	int init_ibufs;
	/**Defines the number of receive buffers initially allocated. The number is on-the-fly expanded when needed (see init_recv_ibufs_chunk)**/
	int init_recv_ibufs;
	/**Max number of Eager buffers which can be posted to the SRQ. This number cannot be higher than the number fixed by the HW**/
	int max_srq_ibufs_posted;
	/**Max number of Eager buffers which can be used by the SRQ. This number is not fixed by the HW**/
	int max_srq_ibufs;
	/**Min number of free recv Eager buffers before posting a new buffer.**/
	int srq_credit_limit;
	/**Min number of free recv Eager buffers before the activation of the asynchronous thread. If this thread is activated too many times, the performance may be decreased.**/
	int srq_credit_thread_limit;
	/**Number of new buffers allocated when no more buffers are available.**/
	int size_ibufs_chunk;
	/**Number of MMU entries allocated during the MPC initlization.**/
	int init_mr;
	/**Defines if the steal in MPI is allowed **/
	int steal;
	/**Defines if the Infiniband interface must crash quietly.**/
	int quiet_crash;
	/**Defines if the asynchronous may be started at the MPC initialization.**/
	int async_thread;
	/**Defines the number of entries for the CQ dedicated to received messages.**/
	int wc_in_number;
	/**Defines the number of entries for the CQ dedicated to sent messages.**/
	int wc_out_number;
	/**Defines if the low memory mode should be activated**/
	int low_memory;
	/**Defines the Rendezvous protocol to use (IBV_RDVZ_WRITE_PROTOCOL or IBV_RDVZ_READ_PROTOCOL)**/
	enum ibv_rdvz_protocol rdvz_protocol;
	/**Defines the minimum size for the Eager RDMA buffers**/
	int rdma_min_size;
	/**Defines the maximun size for the Eager RDMA buffers**/
	int rdma_max_size;
	/**Defines the minimum number of Eager RDMA buffers**/
	int rdma_min_nb;
	/**Defines the maximum number of Eager RDMA buffers**/
	int rdma_max_nb;
	/**Defines the minimum size for the Eager RDMA buffers (resizing)**/
	int rdma_resizing_min_size;
	/**Defines the maximum size for the Eager RDMA buffers (resizing)**/
	int rdma_resizing_max_size;
	/**Defines the minimum number of Eager RDMA buffers (resizing)**/
	int rdma_resizing_min_nb;
	/**Defines the maximum number of Eager RDMA buffers (resizing)**/
	int rdma_resizing_max_nb;
	/**Defines the number of receive buffers allocated on the fly.**/
	int size_recv_ibufs_chunk;
};

/******************************** STRUCTURE *********************************/
/**Global Parameters for IB common structs.**/
struct sctk_runtime_config_struct_ib_global
{	int init_done;
	/**Defines if the MMU cache is enabled.**/
	int mmu_cache_enabled;
	/**Number of entries to keep in the cache.**/
	int mmu_cache_entry_count;
	/**Total size of entries to keep in the cache.**/
	size_t mmu_cache_maximum_size;
	/**Maximum size of an pinned entry.**/
	size_t mmu_cache_maximum_pin_size;
};

/******************************** STRUCTURE *********************************/
/**Portals-based driver**/
struct sctk_runtime_config_struct_net_driver_portals
{	int init_done;
	/**Max size of messages allowed to use the eager protocol.**/
	int eager_limit;
	/**Min number of communicators (help to avoid dynamic PT entry allocation)**/
	int min_comms;
};

/******************************** STRUCTURE *********************************/
/**TCP-based driver**/
struct sctk_runtime_config_struct_net_driver_tcp
{	int init_done;
	/**Enable TCP over Infiniband (if elligible).**/
	int tcpoib;
};

/******************************** STRUCTURE *********************************/
/**TCP-Based RDMA implementation**/
struct sctk_runtime_config_struct_net_driver_tcp_rdma
{	int init_done;
	/**Enable TCP over Infiniband (if elligible).**/
	int tcpoib;
};

/******************************** STRUCTURE *********************************/
/**Inter-Process shared memory communication implementation**/
struct sctk_runtime_config_struct_net_driver_shm
{	int init_done;
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
enum sctk_runtime_config_struct_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_portals,
	SCTK_RTCFG_net_driver_tcp,
	SCTK_RTCFG_net_driver_tcprdma,
	SCTK_RTCFG_net_driver_shm,
	SCTK_RTCFG_net_driver_topological,
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct sctk_runtime_config_struct_net_driver
{
	enum sctk_runtime_config_struct_net_driver_type type;
	union {
		struct sctk_runtime_config_struct_net_driver_infiniband infiniband;
		struct sctk_runtime_config_struct_net_driver_portals portals;
		struct sctk_runtime_config_struct_net_driver_tcp tcp;
		struct sctk_runtime_config_struct_net_driver_tcp_rdma tcprdma;
		struct sctk_runtime_config_struct_net_driver_shm shm;
		struct sctk_runtime_config_struct_net_driver_topological topological;
	} value;
};

/******************************** STRUCTURE *********************************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct sctk_runtime_config_struct_net_driver_config
{	int init_done;
	/**Name of the driver configuration to be referenced in rail definitions.**/
	char * name;
	/**Define the related driver to use and its configuration.**/
	struct sctk_runtime_config_struct_net_driver driver;
};

/******************************** STRUCTURE *********************************/
/**This gate applies given thruth value to messages.**/
struct sctk_runtime_config_struct_gate_boolean
{	int init_done;
	/**whereas to accept input messages or not**/
	int value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a parametrized probability.**/
struct sctk_runtime_config_struct_gate_probabilistic
{	int init_done;
	/**Probability to choose this rail in percents (ralatively to this single rail, integer)**/
	int probability;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at least a given value.**/
struct sctk_runtime_config_struct_gate_min_size
{	int init_done;
	/**Minimum size to choose this rail (with units)**/
	size_t value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at most a given value.**/
struct sctk_runtime_config_struct_gate_max_size
{	int init_done;
	/**Maximum size to choose this rail (with units)**/
	size_t value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate can be used define which type of message can use a given rail.**/
struct sctk_runtime_config_struct_gate_message_type
{	int init_done;
	/**Process Specific Messages can use this rail**/
	int process;
	/**Task specific messages can use this rail**/
	int task;
	/**Task specific messages can use this rail**/
	int emulated_rma;
	/**Common messages (MPI) can use this rail**/
	int common;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a user defined function.**/
struct sctk_runtime_config_struct_gate_user
{	int init_done;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/********************************** ENUM ************************************/
/**Defines gates and their configuration.**/
enum sctk_runtime_config_struct_net_gate_type
{
	SCTK_RTCFG_net_gate_NONE,
	SCTK_RTCFG_net_gate_boolean,
	SCTK_RTCFG_net_gate_probabilistic,
	SCTK_RTCFG_net_gate_minsize,
	SCTK_RTCFG_net_gate_maxsize,
	SCTK_RTCFG_net_gate_msgtype,
	SCTK_RTCFG_net_gate_user,
};

/******************************** STRUCTURE *********************************/
/**Defines gates and their configuration.**/
struct sctk_runtime_config_struct_net_gate
{
	enum sctk_runtime_config_struct_net_gate_type type;
	union {
		struct sctk_runtime_config_struct_gate_boolean boolean;
		struct sctk_runtime_config_struct_gate_probabilistic probabilistic;
		struct sctk_runtime_config_struct_gate_min_size minsize;
		struct sctk_runtime_config_struct_gate_max_size maxsize;
		struct sctk_runtime_config_struct_gate_message_type msgtype;
		struct sctk_runtime_config_struct_gate_probabilistic user;
	} value;
};

/******************************** STRUCTURE *********************************/
/**Defines a topological polling configuration.**/
struct sctk_runtime_config_struct_topological_polling
{	int init_done;
	/**Define the subset of cores involved in the polling.**/
	enum rail_topological_polling_level range;
	/**Define the subset of cores involved in the polling.**/
	enum rail_topological_polling_level trigger;
};

/******************************** STRUCTURE *********************************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/
struct sctk_runtime_config_struct_net_rail
{	int init_done;
	/**Define the name of current rail.**/
	char * name;
	/**Number which defines the order in which routes are tested (higher first).**/
	int priority;
	/**Define the name of the device to use in this rail.**/
	char * device;
	/**Define how the idle polling is done.**/
	struct sctk_runtime_config_struct_topological_polling idle_polling;
	/**Define how the any-source polling is done.**/
	struct sctk_runtime_config_struct_topological_polling any_source_polling;
	/**Define the network topology to apply on this rail.**/
	char * topology;
	/**Define if on-demand connections are allowed on this rail.**/
	int ondemand;
	/**Defines if the rail has RDMA enabled.**/
	int rdma;
	/**Define the driver config to use for this rail.**/
	char * config;
	/**List of gates to be applied in this config.**/
	struct sctk_runtime_config_struct_net_gate * gates;
	/** Number of elements in gates array. **/
	int gates_size;
	/**Used for topological rail selection**/
	struct sctk_runtime_config_struct_net_rail * subrails;
	/** Number of elements in subrails array. **/
	int subrails_size;
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network provided by '-net'.**/
struct sctk_runtime_config_struct_net_cli_option
{	int init_done;
	/**Define the name of the option.**/
	char * name;
	/**Define the driver config to use for this rail.**/
	char * * rails;
	/** Number of elements in rails array. **/
	int rails_size;
};

/******************************** STRUCTURE *********************************/
/**Base structure to contain the network configuration**/
struct sctk_runtime_config_struct_networks
{	int init_done;
	/**Define the configuration driver list to reuse in rail definitions.**/
	struct sctk_runtime_config_struct_net_driver_config * configs;
	/** Number of elements in configs array. **/
	int configs_size;
	/**List of rails to declare in MPC.**/
	struct sctk_runtime_config_struct_net_rail * rails;
	/** Number of elements in rails array. **/
	int rails_size;
	/**List of networks available through the '-net' argument of mpcrun.**/
	struct sctk_runtime_config_struct_net_cli_option * cli_options;
	/** Number of elements in cli_options array. **/
	int cli_options_size;
};

/******************************** STRUCTURE *********************************/
/**Options for communication between threads**/
struct sctk_runtime_config_struct_inter_thread_comm
{	int init_done;
	/****/
	int barrier_arity;
	/****/
	int broadcast_arity_max;
	/****/
	int broadcast_max_size;
	/****/
	int broadcast_check_threshold;
	/****/
	int allreduce_arity_max;
	/****/
	int allreduce_max_size;
	/****/
	int allreduce_check_threshold;
	/**Slot size for allreduce**/
	int ALLREDUCE_MAX_SLOT;
	/****/
	struct sctk_runtime_config_funcptr collectives_init_hook;
};

/******************************** STRUCTURE *********************************/
/****/
struct sctk_runtime_config_struct_low_level_comm
{	int init_done;
	/****/
	int checksum;
	/****/
	struct sctk_runtime_config_funcptr send_msg;
	/****/
	char * network_mode;
	/****/
	int dyn_reordering;
	/**Enable usage of polling during idle.**/
	int enable_idle_polling;
	/**Global parameters for IB**/
	struct sctk_runtime_config_struct_ib_global ib_global;
};

/******************************** STRUCTURE *********************************/
/**Shared Memory Collectives for MPC**/
struct sctk_runtime_config_struct_collectives_shm
{	int init_done;
	/**MPI_Barrier intracom algorithm on shared communicators**/
	struct sctk_runtime_config_funcptr barrier_intra_shm;
	/**Type of MPI_Bcast intracom algorithm on shared communicators**/
	struct sctk_runtime_config_funcptr bcast_intra_shm;
	/**Alltoallv intracom algorithm**/
	struct sctk_runtime_config_funcptr alltoallv_intra_shm;
	/**MPI_Gatherv intracom algorithm for shared communicators**/
	struct sctk_runtime_config_funcptr gatherv_intra_shm;
	/**MPI_Scatterv intracom algorithm on shared communicators**/
	struct sctk_runtime_config_funcptr scatterv_intra_shm;
	/**MPI_Reduce intracom shared-mem algorithm**/
	struct sctk_runtime_config_funcptr reduce_intra_shm;
	/**Arrity being used to build topological communicators  '-1' means auto-compute to match processes and NUMA**/
	int topo_tree_arity;
	/**Dump topological comm tree in DOT (fname topoN.cdat) with N the communicator size**/
	int topo_tree_dump;
	/**Force the use of deterministic algorithms**/
	int coll_force_nocommute;
	/**Number of blocks for pipelined Reduce**/
	int reduce_pipelined_blocks;
	/**Size required to rely on pipelined reduce**/
	size_t reduce_pipelined_tresh;
	/**Number of reduce slots to allocate (required to be power of 2)**/
	int reduce_interleave;
	/**Number of bcast slots to allocate (required to be power of 2)**/
	int bcast_interleave;
};

/******************************** STRUCTURE *********************************/
/**Collectives intracom MPI**/
struct sctk_runtime_config_struct_collectives_intra
{	int init_done;
	/**MPI_Barrier intracom algorithm**/
	struct sctk_runtime_config_funcptr barrier_intra;
	/**Type of MPI_Bcast intracom algorithm**/
	struct sctk_runtime_config_funcptr bcast_intra;
	/**MPI_Allgather intracom algorithm**/
	struct sctk_runtime_config_funcptr allgather_intra;
	/**MPI_Allgatherv intracom algorithm**/
	struct sctk_runtime_config_funcptr allgatherv_intra;
	/**MPI_Alltoall intracom algorithm**/
	struct sctk_runtime_config_funcptr alltoall_intra;
	/**Alltoallv intracom algorithm**/
	struct sctk_runtime_config_funcptr alltoallv_intra;
	/**MPI_Alltoallw intracom algorithm**/
	struct sctk_runtime_config_funcptr alltoallw_intra;
	/**MPI_Gather intracom algorithm**/
	struct sctk_runtime_config_funcptr gather_intra;
	/**MPI_Gatherv intracom algorithm**/
	struct sctk_runtime_config_funcptr gatherv_intra;
	/**MPI_Scatter intracom algorithm**/
	struct sctk_runtime_config_funcptr scatter_intra;
	/**MPI_Scatterv intracom algorithm**/
	struct sctk_runtime_config_funcptr scatterv_intra;
	/**MPI_Scan intracom algorithm**/
	struct sctk_runtime_config_funcptr scan_intra;
	/**MPI_Exscan intracom algorithm**/
	struct sctk_runtime_config_funcptr exscan_intra;
	/**MPI_Reduce intracom algorithm**/
	struct sctk_runtime_config_funcptr reduce_intra;
	/**MPI_Allreduce intracom algorithm**/
	struct sctk_runtime_config_funcptr allreduce_intra;
	/**MPI_Reduce_scatter intracom algorithm**/
	struct sctk_runtime_config_funcptr reduce_scatter_intra;
	/**MPI_Reduce_scatter_block intracom algorithm**/
	struct sctk_runtime_config_funcptr reduce_scatter_block_intra;
};

/******************************** STRUCTURE *********************************/
/**Collectives intercom MPI**/
struct sctk_runtime_config_struct_collectives_inter
{	int init_done;
	/**MPI_Barrier intercom algorithm**/
	struct sctk_runtime_config_funcptr barrier_inter;
	/**MPI_Barrier intercom algorithm**/
	struct sctk_runtime_config_funcptr bcast_inter;
	/**MPI_Allgather intercom algorithm**/
	struct sctk_runtime_config_funcptr allgather_inter;
	/**MPI_Allgatherv intercom algorithm**/
	struct sctk_runtime_config_funcptr allgatherv_inter;
	/**MPI_Alltoall intercom algorithm**/
	struct sctk_runtime_config_funcptr alltoall_inter;
	/**MPI_Alltoallv intercom algorithm**/
	struct sctk_runtime_config_funcptr alltoallv_inter;
	/**MPI_Alltoallw intercom algorithm**/
	struct sctk_runtime_config_funcptr alltoallw_inter;
	/**MPI_Gather intercom algorithm**/
	struct sctk_runtime_config_funcptr gather_inter;
	/**MPI_Gatherv intercom algorithm**/
	struct sctk_runtime_config_funcptr gatherv_inter;
	/**MPI_Scatter intercom algorithm**/
	struct sctk_runtime_config_funcptr scatter_inter;
	/**MPI_Scatterv intercom algorithm**/
	struct sctk_runtime_config_funcptr scatterv_inter;
	/**MPI_Reduce intercom algorithm**/
	struct sctk_runtime_config_funcptr reduce_inter;
	/**MPI_Allreduce intercom algorithm**/
	struct sctk_runtime_config_funcptr allreduce_inter;
	/**MPI_Reduce_scatter intercom algorithm**/
	struct sctk_runtime_config_funcptr reduce_scatter_inter;
	/**MPI_Reduce_scatter_block intercom algorithm**/
	struct sctk_runtime_config_funcptr reduce_scatter_block_inter;
};

/******************************** STRUCTURE *********************************/
/**Progress thread NBC**/
struct sctk_runtime_config_struct_progress_thread
{	int init_done;
	/**If use progress threads for non blocking collectives**/
	int use_progress_thread;
	/**Algorithm of progress threads binding : sctk_get_progress_thread_binding_[bind,smart,numa_iter,numa]**/
	struct sctk_runtime_config_funcptr progress_thread_binding;
};

/******************************** STRUCTURE *********************************/
/**Options related to the MPI RMA support**/
struct sctk_runtime_config_struct_mpi_rma
{	int init_done;
	/**Enable the MPI_Alloc_mem shared memory pool**/
	int alloc_mem_pool_enable;
	/**Size of the MPI_Alloc_mem pool**/
	size_t alloc_mem_pool_size;
	/**Alloc the MPI_Alloc_mem pool to grow linear for some apps**/
	int alloc_mem_pool_autodetect;
	/**Force the size to be a quantum per local process**/
	int alloc_mem_pool_force_process_linear;
	/**Quantum to allocate to each process when linear forced**/
	size_t alloc_mem_pool_per_process_size;
	/**Maximum number of window threads to keep**/
	int win_thread_pool_max;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Message Passing**/
struct sctk_runtime_config_struct_mpc
{	int init_done;
	/**Print debug messages**/
	int log_debug;
	/****/
	int hard_checking;
	/****/
	int buffering;
};

/********************************** ENUM ************************************/
/****/
enum mpcomp_task_larceny_mode_t
{
	MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL,
	MPCOMP_TASK_LARCENY_MODE_RANDOM,
	MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER,
	MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN,
	MPCOMP_TASK_LARCENY_MODE_PRODUCER,
	MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER,
	MPCOMP_TASK_LARCENY_MODE_COUNT
};

/******************************** STRUCTURE *********************************/
/**Options for MPC OpenMP.**/
struct sctk_runtime_config_struct_openmp
{	int init_done;
	/**Number of VPs for each OpenMP team**/
	int vp;
	/**Runtime schedule type and chunck size**/
	char * schedule;
	/**Number of threads to use during execution**/
	int nb_threads;
	/**Dynamic adjustment of the number of threads**/
	int adjustment;
	/**Bind threads to processor core**/
	int proc_bind;
	/**Nested parallelism**/
	int nested;
	/**Stack size for OpenMP threads**/
	int stack_size;
	/**Behavior of threads while waiting**/
	int wait_policy;
	/**Maximum number of OpenMP threads among all teams**/
	int thread_limit;
	/**Maximum depth of nested parallelism**/
	int max_active_levels;
	/**Tree shape for OpenMP construct**/
	char * tree;
	/**Maximum number of threads for each team of a parallel region**/
	int max_threads;
	/**Maximum number of shared for loops w/ dynamic schedule alive**/
	int max_alive_for_dyn;
	/**Maximum number of shared for loops w/ guided schedule alive**/
	int max_alive_for_guided;
	/**Maximum number of alive sections construct**/
	int max_alive_sections;
	/**Maximum number of alive single construct**/
	int max_alive_single;
	/**Emit warning when entering nested parallelism**/
	int warn_nested;
	/**MPI/OpenMP hybrid mode (simple-mixed, alternating)**/
	char * mode;
	/**Affinity of threads for parallel regions (COMPACT, SCATTER, BALANCED)**/
	char * affinity;
	/**Depth of the new tasks lists in the tree**/
	int omp_new_task_depth;
	/**Depth of the untied tasks lists in the tree**/
	int omp_untied_task_depth;
	/**Task stealing policy**/
	enum mpcomp_task_larceny_mode_t omp_task_larceny_mode;
	/**Task max depth in task generation**/
	int omp_task_nesting_max;
	/** Max tasks in mpcomp list**/
	int mpcomp_task_max_delayed;
};

/******************************** STRUCTURE *********************************/
/**Options for the internal MPC Profiler**/
struct sctk_runtime_config_struct_profiler
{	int init_done;
	/**Prefix of MPC Profiler outputs**/
	char * file_prefix;
	/**Add a timestamp to profiles file names**/
	int append_date;
	/**Profile in color when outputed to stdout**/
	int color_stdout;
	/**Color for levels of profiler output**/
	char * * level_colors;
	/** Number of elements in level_colors array. **/
	int level_colors_size;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC threads.**/
struct sctk_runtime_config_struct_thread
{	int init_done;
	/**Max number of accesses to the lock before calling thread_yield**/
	int spin_delay;
	/****/
	int interval;
	/**Define the stack size of MPC user threads**/
	size_t kthread_stack_size;
	/**Initialize thread placement policy**/
	struct sctk_runtime_config_funcptr placement_policy;
};

/******************************** STRUCTURE *********************************/
/**Scheduler priority parameters**/
struct sctk_runtime_config_struct_scheduler
{	int init_done;
	/**Threshold for priority scheduling quantum**/
	double timestamp_threshold;
	/**Basic priority of polling tasks**/
	int task_polling_thread_basic_priority;
	/**Step of basic priority of polling tasks**/
	int task_polling_thread_basic_priority_step;
	/**Step of current priority of polling tasks**/
	int task_polling_thread_current_priority_step;
	/**Basic priority of polling tasks**/
	int sched_NBC_Pthread_basic_priority;
	/**Step of basic priority of nbc progress threads**/
	int sched_NBC_Pthread_basic_priority_step;
	/**Step of current priority of nbc progress threads**/
	int sched_NBC_Pthread_current_priority_step;
	/**Basic priority of MPI threads**/
	int mpi_basic_priority;
	/**Basic priority of OMP threads**/
	int omp_basic_priority;
	/**Basic priority of POSIX threads**/
	int posix_basic_priority;
	/**Basic priority of POSIX threads**/
	int progress_basic_priority;
};

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config_modules
{
	struct sctk_runtime_config_struct_accl accelerator;
	struct sctk_runtime_config_struct_allocator allocator;
	struct sctk_runtime_config_struct_launcher launcher;
	struct sctk_runtime_config_struct_debugger debugger;
	struct sctk_runtime_config_struct_ft ft_system;
	struct sctk_runtime_config_struct_inter_thread_comm inter_thread_comm;
	struct sctk_runtime_config_struct_low_level_comm low_level_comm;
	struct sctk_runtime_config_struct_collectives_shm collectives_shm;
	struct sctk_runtime_config_struct_collectives_intra collectives_intra;
	struct sctk_runtime_config_struct_collectives_inter collectives_inter;
	struct sctk_runtime_config_struct_progress_thread progress_thread;
	struct sctk_runtime_config_struct_mpc mpc;
	struct sctk_runtime_config_struct_mpi_rma rma;
	struct sctk_runtime_config_struct_openmp openmp;
	struct sctk_runtime_config_struct_profiler profiler;
	struct sctk_runtime_config_struct_thread thread;
	struct sctk_runtime_config_struct_scheduler scheduler;
};

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config
{
	int number_profiles;
	char* profiles_name_list[SCTK_RUNTIME_CONFIG_MAX_PROFILES];
	struct sctk_runtime_config_modules modules;
	struct sctk_runtime_config_struct_networks networks;
};

#endif /* SCTK_RUNTIME_CONFIG_STRUCT_H */

