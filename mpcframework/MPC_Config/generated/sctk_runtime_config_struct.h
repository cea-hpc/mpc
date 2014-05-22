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
/**Options for MPC memory allocator.**/
struct sctk_runtime_config_struct_allocator
{	/**Enable or disable NUMA migration of allocator pages on thread migration.**/
	bool numa_migration;
	/**If the new segment is less than N time smaller than factor, realloc will allocate a new segment, otherwise it will keep the same one. Use 1 to force realloc every time (may be slower but consume less memory).**/
	int realloc_factor;
	/**If the new segment is smaller of N bytes than threashold, realloc will allocate a new segment, otherwise it will keep the same one. Use 0 to force realloc every time (may be slower but consume less memory).**/
	size_t realloc_threashold;
	/**Permit to enable of disable NUMA support in MPC Allocator.**/
	bool numa;
	/**If true, enable usage of abort() on free error, otherwise try to continue by skipping.**/
	bool strict;
	/**Maximum amount of memory to keep in memory sources (one per NUMA node). Use 0 to disable cache, huge value to keep all.**/
	size_t keep_mem;
	/**Maximum size of macro blocs to keep in memory source for reuse. Use 0 to disable cache, huge value to keep all.**/
	size_t keep_max;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC launcher.**/
struct sctk_runtime_config_struct_launcher
{	/**Default verbosity level from 0 to 3. Can be override by -vv on mpcrun.**/
	int verbosity;
	/**Display the MPC banner at launch time to print some informations about the topology. Can be override by MPC_DISABLE_BANNER.**/
	bool banner;
	/**Automatically kill the MPC processes after a given timeout. Use 0 to disable. Can be override by MPC_AUTO_KILL_TIMEOUT.**/
	int autokill;
	/**Permit to extend the launchers available via 'mpcrun -l=...' by providing scripts (named mpcrun_XXXX) in a user directory. Can be override by MPC_USER_LAUNCHERS.**/
	char * user_launchers;
	/**Activate randomization of base addresses**/
	bool keep_rand_addr;
	/**Deactivate randomization of base addresses**/
	bool disable_rand_addr;
	/**Do not use mpc for execution (deprecated?)**/
	bool disable_mpc;
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
	bool vers_details;
	/**Select the type of outputs for the profiling**/
	char * profiling;
	/**Enable usage of hyperthreaded cores if available on current architecture.**/
	bool enable_smt;
	/**Enable the restriction on CPU number to share node**/
	bool share_node;
	/**Restart MPC from a previous checkpoint**/
	bool restart;
	/**Enable MPC checkpointing**/
	bool checkpoint;
	/**Enable migration**/
	bool migration;
	/**Enable reporting.**/
	bool report;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Debugger**/
struct sctk_runtime_config_struct_debugger
{	/**Print colored text in terminal**/
	bool colors;
	/****/
	int max_filename_size;
};

/********************************** ENUM ************************************/
/****/
enum ibv_rdvz_protocol
{
	IBV_RDVZ_WRITE_PROTOCOL,
	IBV_RDVZ_READ_PROTOCOL
};

/******************************** STRUCTURE *********************************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_infiniband
{	/**Define a network's type (0=signalization, 1=data)**/
	int network_type;
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
	/**Number of MMU entries allocated when no more MMU entries are available.**/
	int size_mr_chunk;
	/**Defines if the MMU cache is enabled.**/
	int mmu_cache_enabled;
	/**Defines the number of MMU cache allowed.**/
	int mmu_cache_entries;
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
	/**Number of outstanding RDMA reads and atomic operations on the destination QP (to be confirmed)**/
	int rdma_depth;
	/**Number of responder resources for handling incoming RDMA reads and atomic operations (to be confirmed)**/
	int rdma_dest_depth;
	/**Defines if the low memory mode should be activated**/
	bool low_memory;
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
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_portals
{	/**Fake param.**/
	int fake_param;
};

/******************************** STRUCTURE *********************************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_tcp
{	/**Fake param.**/
	int fake_param;
};

/********************************** ENUM ************************************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum sctk_runtime_config_struct_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_tcp,
	SCTK_RTCFG_net_driver_tcpoib,
	SCTK_RTCFG_net_driver_portals,
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct sctk_runtime_config_struct_net_driver
{
	enum sctk_runtime_config_struct_net_driver_type type;
	union {
		struct sctk_runtime_config_struct_net_driver_infiniband infiniband;
		struct sctk_runtime_config_struct_net_driver_tcp tcp;
		struct sctk_runtime_config_struct_net_driver_tcp tcpoib;
		struct sctk_runtime_config_struct_net_driver_portals portals;
	} value;
};

/******************************** STRUCTURE *********************************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct sctk_runtime_config_struct_net_driver_config
{	/**Name of the driver configuration to be referenced in rail definitions.**/
	char * name;
	/**Define the related driver to use and its configuration.**/
	struct sctk_runtime_config_struct_net_driver driver;
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network provided by '-net'.**/
struct sctk_runtime_config_struct_net_cli_option
{	/**Define the name of the option.**/
	char * name;
	/**Define the driver config to use for this rail.**/
	char * * rails;
	/** Number of elements in rails array. **/
	int rails_size;
};

/******************************** STRUCTURE *********************************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/
struct sctk_runtime_config_struct_net_rail
{	/**Define the name of current rail.**/
	char * name;
	/**Define the name of the device to use in this rail.**/
	char * device;
	/**Define the network topology to apply on this rail.**/
	char * topology;
	/**Define the driver config to use for this rail.**/
	char * config;
};

/******************************** STRUCTURE *********************************/
/**Base structure to contain the network configuration**/
struct sctk_runtime_config_struct_networks
{	/**Define the configuration driver list to reuse in rail definitions.**/
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
{	/****/
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
{	/****/
	bool checksum;
	/****/
	struct sctk_runtime_config_funcptr send_msg;
	/****/
	char * network_mode;
	/****/
	bool dyn_reordering;
	/**Enable usage of polling during idle.**/
	bool enable_idle_polling;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Message Passing**/
struct sctk_runtime_config_struct_mpc
{	/**Print debug messages**/
	bool log_debug;
	/****/
	bool hard_checking;
	/****/
	bool buffering;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC OpenMP.**/
struct sctk_runtime_config_struct_openmp
{	/**Number of VPs for each OpenMP team**/
	int vp;
	/**Runtime schedule type and chunck size**/
	char * schedule;
	/**Number of threads to use during execution**/
	int nb_threads;
	/**Dynamic adjustment of the number of threads**/
	bool adjustment;
	/**Bind threads to processor core**/
	bool proc_bind;
	/**Nested parallelism**/
	bool nested;
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
	bool warn_nested;
	/**MPI/OpenMP hybrid mode (simple-mixed, alternating)**/
	char * mode;
};

/******************************** STRUCTURE *********************************/
/**Options for the internal MPC Profiler**/
struct sctk_runtime_config_struct_profiler
{	/**Prefix of MPC Profiler outputs**/
	char * file_prefix;
	/**Add a timestamp to profiles file names**/
	bool append_date;
	/**Profile in color when outputed to stdout**/
	bool color_stdout;
	/**Color for levels of profiler output**/
	char * * level_colors;
	/** Number of elements in level_colors array. **/
	int level_colors_size;
};

/******************************** STRUCTURE *********************************/
/**Options for MPC threads.**/
struct sctk_runtime_config_struct_thread
{	/**Max number of accesses to the lock before calling thread_yield**/
	int spin_delay;
	/****/
	int interval;
	/**Define the stack size of MPC user threads**/
	size_t kthread_stack_size;
};

/******************************** STRUCTURE *********************************/
struct sctk_runtime_config_modules
{
	struct sctk_runtime_config_struct_allocator allocator;
	struct sctk_runtime_config_struct_launcher launcher;
	struct sctk_runtime_config_struct_debugger debugger;
	struct sctk_runtime_config_struct_inter_thread_comm inter_thread_comm;
	struct sctk_runtime_config_struct_low_level_comm low_level_comm;
	struct sctk_runtime_config_struct_mpc mpc;
	struct sctk_runtime_config_struct_openmp openmp;
	struct sctk_runtime_config_struct_profiler profiler;
	struct sctk_runtime_config_struct_thread thread;
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

