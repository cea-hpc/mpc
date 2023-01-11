#include "lowcomm_config.h"

#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>
#include <dlfcn.h>
#include <ctype.h>

//FIXME: enable verbosity when only lowcomm module is loaded
#include <mpc_common_flags.h>
#include <limits.h>

#include "coll.h"

#include <sctk_alloc.h>

#include "sctk_topological_polling.h"

#ifdef MPC_USE_OFI
#include "ofi.h"
#endif


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
    memset(__net_config.configs, 0, MPC_CONF_MAX_CONFIG_COUNT * sizeof(struct _mpc_lowcomm_config_struct_net_driver_config *));
	__net_config.configs_size = 0;
}

struct _mpc_lowcomm_config_struct_net_driver_config * _mpc_lowcomm_conf_driver_unfolded_get(char * name)
{
	if(!name)
	{
		return NULL;
	}

	int i;

	for(i = 0 ; i < __net_config.configs_size; i++)
	{
		if(!strcmp(name, __net_config.configs[i]->name))
		{
			return __net_config.configs[i];
		}
	}

	return NULL;
}


static inline void __append_new_driver_to_unfolded(struct _mpc_lowcomm_config_struct_net_driver_config * driver_config)
{
    if(__net_config.configs_size == MPC_CONF_MAX_CONFIG_COUNT)
    {
        bad_parameter("Cannot create more than %d driver config when processing config %s.\n", MPC_CONF_MAX_CONFIG_COUNT, driver_config->name);
    }


    if(_mpc_lowcomm_conf_driver_unfolded_get(driver_config->name))
    {
        bad_parameter("Cannot append the '%s' driver configuration twice", driver_config->name);
    }

    __net_config.configs[ __net_config.configs_size ] = driver_config;
    __net_config.configs_size++;
}


static inline mpc_conf_config_type_t *__init_driver_shm(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = SCTK_RTCFG_net_driver_shm;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_shm *shm = &driver->value.shm;

	/* Buffered */

	shm->buffered_priority = 0;
	shm->buffered_min_size = 0;
	shm->buffered_max_size = 4096;
	shm->buffered_zerocopy = 0;

	/* CMA */

#ifdef MPC_USE_CMA
	shm->cma_enable = 1;
#else
	shm->cma_enable = 0;
#endif
	shm->cma_priority = 1;
	shm->cma_min_size = 4096;
	shm->cma_max_size = 0;
	shm->cma_zerocopy = 0;


	/* Frag */
	shm->frag_priority = 2;
	shm->frag_min_size = 4096;
	shm->frag_max_size = 0;
	shm->frag_zerocopy = 0;

	/* Size parameters */
	shm->shmem_size = 1024;
	shm->cells_num  = 8192;

	/*
	  Create the config object
	*/

	mpc_conf_config_type_t *buffered = mpc_conf_config_type_init("buffered",
	                                                             PARAM("priority", &shm->buffered_priority, MPC_CONF_INT, "Defines priority for the SHM buffered message"),
	                                                             PARAM("minsize", &shm->buffered_min_size, MPC_CONF_INT, "Defines the min size for the SHM buffered message"),
	                                                             PARAM("maxsize", &shm->buffered_max_size, MPC_CONF_INT, "Defines the max size for the SHM buffered message"),
	                                                             PARAM("zerocopy", &shm->buffered_zerocopy, MPC_CONF_BOOL, "Defines if mode zerocopy should be actived for SHM buffered message"),
	                                                             NULL);

	#ifdef MPC_USE_CMA
	mpc_conf_config_type_t *cma = mpc_conf_config_type_init("cma",
	                                                        PARAM("enabled", &shm->cma_enable, MPC_CONF_BOOL, "Enable messages through Cross Memory Attach (CMA)"),
	                                                        PARAM("priority", &shm->cma_priority, MPC_CONF_INT, "Defines priority for the SHM CMA message"),
	                                                        PARAM("minsize", &shm->cma_min_size, MPC_CONF_INT, "Defines the min size for the SHM CMA message"),
	                                                        PARAM("maxsize", &shm->cma_max_size, MPC_CONF_INT, "Defines the max size for the SHM CMA message"),
	                                                        PARAM("zerocopy", &shm->cma_zerocopy, MPC_CONF_BOOL, "Defines if mode zerocopy should be actived for SHM CMA message"),
	                                                        NULL);
	#endif

	mpc_conf_config_type_t *frag = mpc_conf_config_type_init("frag",
	                                                         PARAM("priority", &shm->frag_priority, MPC_CONF_INT, "Defines priority for the SHM frag message"),
	                                                         PARAM("minsize", &shm->frag_min_size, MPC_CONF_INT, "Defines the min size for the SHM frag message"),
	                                                         PARAM("maxsize", &shm->frag_max_size, MPC_CONF_INT, "Defines the max size for the SHM frag message"),
	                                                         PARAM("zerocopy", &shm->frag_zerocopy, MPC_CONF_BOOL, "Defines if mode zerocopy should be actived for SHM frag message"),
	                                                         NULL);

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("shm",
	                                                        PARAM("buffered", buffered, MPC_CONF_TYPE, "Configuration for Buffered Messages"),
	#ifdef MPC_USE_CMA
	                                                        PARAM("cma", cma, MPC_CONF_TYPE, "Configuration for CMA Messages"),
	#endif
															PARAM("frag", frag, MPC_CONF_TYPE, "Configuration for fragmented Messages"),
															PARAM("size", &shm->shmem_size, MPC_CONF_INT, "Size of the memory region"),
															PARAM("cellnum", &shm->cells_num, MPC_CONF_INT, "Number of cells in the memory region"),
	                                                        NULL);

	return ret;
}


static inline mpc_conf_config_type_t *__init_driver_tcp(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = SCTK_RTCFG_net_driver_tcp;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_tcp *tcp = &driver->value.tcp;

        tcp->max_msg_size = INT_MAX;

	/*
	  Create the config object
	*/

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("tcp",
                                                                PARAM("maxmsgsize", &tcp->max_msg_size, MPC_CONF_INT, "Maximum message size (in B)"),
	                                                        NULL);

	return ret;
}


#ifdef MPC_USE_PORTALS


static inline mpc_conf_config_type_t *__init_driver_portals(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = SCTK_RTCFG_net_driver_portals;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_portals *portals = &driver->value.portals;

	portals->eager_limit = 1024;
	portals->min_comms = 1;
	portals->block_cut = 2147483648;
	portals->offloading.collectives = 0;
	portals->offloading.ondemand = 0;
        portals->max_iovecs = 8;
        portals->num_eager_blocks = 4;
        portals->eager_block_size = 4*8192;
	portals->max_msg_size = 2147483648;
        portals->min_frag_size = 524288; // octets

	/*
	  Create the config object
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

#ifdef MPC_USE_INFINIBAND

static inline mpc_conf_config_type_t *__init_driver_ib(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = SCTK_RTCFG_net_driver_infiniband;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_infiniband *ib = &driver->value.infiniband;

	/* Debug */
	ib->verbose_level = 0;
	ib->quiet_crash = 0;
	ib->async_thread = 0;

	/* Networking params */
	snprintf(ib->pkey, MPC_CONF_STRING_SIZE, "%s", "undefined");
	ib->adm_port = 1;

	/*
	 * Card settings
     */

	/* ibv_qp_init_attr for Infiniband Initialization */
	ib->qp_tx_depth = 15000;
	ib->qp_rx_depth = 0;
	ib->rdma_depth = 16;
	ib->max_sg_sq = 4;
	ib->max_sg_rq = 4;
	ib->max_inline = 128;
	/* Completion queues */
	ib->cq_depth = 40000;
	/* Srq */
	ib->srq_credit_thread_limit = 100;

	/*
	 * IBUF
	 */
	ib->init_ibufs = 1024;
	ib->size_ibufs_chunk = 100;

	ib->init_recv_ibufs = 1024;
	ib->size_recv_ibufs_chunk = 400;

	ib->max_srq_ibufs_posted = 1500;
	ib->max_rdma_connections = 0;

	/* Ibuf RDMA */

	ib->rdma_min_size = 1024;
	ib->rdma_max_size = 4 * 1024;
	ib->rdma_min_nb = 8;
	ib->rdma_max_nb = 32;

	/* Ibuf RDMA resizing */
	ib->rdma_resizing = 0;
	ib->rdma_resizing_min_size = 1024;
	ib->rdma_resizing_max_size = 4 * 1024;
	ib->rdma_resizing_min_nb = 8;
	ib->rdma_resizing_max_nb = 32;


	/* General */
	ib->eager_limit = 4 * 1024;
	ib->buffered_limit = 48 * 1024;

	/*
	  Create the config object
	*/

	mpc_conf_config_type_t *debug = mpc_conf_config_type_init("debug",
															PARAM("verbose", &ib->verbose_level, MPC_CONF_INT, "Defines the verbose level of the Infiniband interface."),
															PARAM("quietcrash", &ib->quiet_crash, MPC_CONF_BOOL, "Defines if the Infiniband interface must crash quietly."),
															PARAM("asyncthread", &ib->async_thread, MPC_CONF_BOOL, "Asynchronous debug thread should be started at initialization."),
	                                                        NULL);

	mpc_conf_config_type_t *network = mpc_conf_config_type_init("network",
															PARAM("pkey", ib->pkey, MPC_CONF_STRING, "Define the Infiniband Partition KEY (PKEY) for this rail."),
															PARAM("port", &ib->adm_port, MPC_CONF_INT, "Defines the port number to use."),
	                                                        NULL);

	mpc_conf_config_type_t *card = mpc_conf_config_type_init("card",
															PARAM("qptxdepth", &ib->qp_tx_depth, MPC_CONF_INT, "Number of entries to allocate in the QP for sending messages. If too low, may cause an QP overrun."),
															PARAM("qprxdepth", &ib->qp_rx_depth, MPC_CONF_INT, "Number of entries to allocate in the QP for receiving messages. Must be 0 if using SRQ."),
															PARAM("maxrdma", &ib->rdma_depth, MPC_CONF_INT, "Number of RDMA resources on QP (covers both max_dest_rd_atomic and max_rd_atomic)."),
															PARAM("maxsgsq", &ib->max_sg_sq, MPC_CONF_INT, "Max pending RDMA operations for send."),
															PARAM("maxsgrq", &ib->max_sg_rq, MPC_CONF_INT, "Max pending RDMA operations for recv."),
															PARAM("maxinline", &ib->max_inline, MPC_CONF_LONG_INT, "Max size for inlining messages."),
															PARAM("cqdepth", &ib->cq_depth, MPC_CONF_INT, "Number of entries to allocate in the CQ. If too low, may cause a CQ overrun."),
															PARAM("srqcredit", &ib->srq_credit_thread_limit, MPC_CONF_INT, "Set the SRQ size."),
	                                                        NULL);

	mpc_conf_config_type_t *rdma = mpc_conf_config_type_init("rdma",
															PARAM("minsize", &ib->rdma_min_size, MPC_CONF_LONG_INT, "Defines the minimum size for the Eager RDMA buffers."),
															PARAM("maxsize", &ib->rdma_max_size, MPC_CONF_LONG_INT, "Defines the maximun size for the Eager RDMA buffers."),
															PARAM("mincount", &ib->rdma_min_nb, MPC_CONF_INT, "Defines the minimum number of Eager RDMA buffers."),
															PARAM("maxcount", &ib->rdma_max_nb, MPC_CONF_INT, "Defines the maximum number of Eager RDMA buffers."),
															NULL);

	mpc_conf_config_type_t *rdma_resized = mpc_conf_config_type_init("rdmaresized",
															PARAM("enabled", &ib->rdma_resizing, MPC_CONF_BOOL, "Defines if RDMA connections may be resized."),
															PARAM("minsize", &ib->rdma_resizing_min_size, MPC_CONF_LONG_INT, "Defines the minimum size for the Eager RDMA buffers."),
															PARAM("maxsize", &ib->rdma_resizing_max_size, MPC_CONF_LONG_INT, "Defines the maximun size for the Eager RDMA buffers."),
															PARAM("mincount", &ib->rdma_resizing_min_nb, MPC_CONF_INT, "Defines the minimum number of Eager RDMA buffers."),
															PARAM("maxcount", &ib->rdma_resizing_max_nb, MPC_CONF_INT, "Defines the maximum number of Eager RDMA buffers."),
															NULL);

	mpc_conf_config_type_t *ibuf = mpc_conf_config_type_init("ibuf",
															PARAM("initcnt", &ib->init_ibufs, MPC_CONF_INT, "Max number of Eager buffers to allocate during the initialization step."),
															PARAM("chunk", &ib->size_ibufs_chunk, MPC_CONF_INT, "Number of new buffers allocated when no more buffers are available."),
															PARAM("initrecv", &ib->init_recv_ibufs, MPC_CONF_INT, "Defines the number of receive buffers initially allocated. The number is on-the-fly expanded when needed (see init_recv_ibufs_chunk)."),
															PARAM("recvchunk", &ib->size_recv_ibufs_chunk, MPC_CONF_INT, "Defines the number of receive buffers allocated on the fly."),
															PARAM("maxsrq", &ib->max_srq_ibufs_posted, MPC_CONF_INT, "Max number of Eager buffers which can be posted to the SRQ. This number cannot be higher than the number fixed by the HW."),
															PARAM("maxrdma", &ib->max_rdma_connections, MPC_CONF_INT, "Number of RDMA buffers allocated for each neighbor."),
															PARAM("rdma", rdma, MPC_CONF_TYPE, "RMDA buffers configuration."),
															PARAM("rdmaresized", rdma_resized, MPC_CONF_TYPE, "Resized RMDA buffers configuration."),
	                                                        NULL);

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("ib",
															PARAM("debug", debug, MPC_CONF_TYPE, "Debug parameters."),
															PARAM("network", network, MPC_CONF_TYPE, "Low-level network parameters."),
															PARAM("card", card, MPC_CONF_TYPE, "Infiniband card parameters."),
															PARAM("ibuf", ibuf, MPC_CONF_TYPE, "Infiniband buffer configuration."),
															PARAM("eagerlimit", &ib->eager_limit, MPC_CONF_INT, "Max size of messages allowed to use the eager protocol."),
															PARAM("bufferedlimit", &ib->buffered_limit, MPC_CONF_INT, "Max size for using the Buffered protocol (message split into several Eager messages)."),

	                                                        NULL);

	return ret;
}

#endif

#ifdef MPC_USE_OFI

static inline void __driver_ofi_unfold(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	assume(driver->type == SCTK_RTCFG_net_driver_ofi);
	struct _mpc_lowcomm_config_struct_net_driver_ofi *ofi = &driver->value.ofi;

	ofi->link = mpc_lowcomm_ofi_decode_mode(ofi->slink);
	ofi->progress = mpc_lowcomm_ofi_decode_progress(ofi->sprogress);
	ofi->ep_type = mpc_lowcomm_ofi_decode_eptype(ofi->sep_type);
	ofi->av_type = mpc_lowcomm_ofi_decode_avtype(ofi->sav_type);
	ofi->rm_type = mpc_lowcomm_ofi_decode_rmtype(ofi->srm_type);
}


static inline mpc_conf_config_type_t *__init_driver_ofi(struct _mpc_lowcomm_config_struct_net_driver *driver)
{
	driver->type = SCTK_RTCFG_net_driver_ofi;

	/*
	Set defaults
	*/

	struct _mpc_lowcomm_config_struct_net_driver_ofi *ofi = &driver->value.ofi;

	snprintf(ofi->slink, MPC_CONF_STRING_SIZE, "%s", "connected");
	snprintf(ofi->sprogress, MPC_CONF_STRING_SIZE, "%s", "auto");
	snprintf(ofi->sep_type, MPC_CONF_STRING_SIZE, "%s", "unspecified");
	snprintf(ofi->sav_type, MPC_CONF_STRING_SIZE, "%s", "unspecified");
	snprintf(ofi->srm_type, MPC_CONF_STRING_SIZE, "%s", "unspecified");
	snprintf(ofi->provider, MPC_CONF_STRING_SIZE, "%s", "tcp");

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("ofi",
															PARAM("link", ofi->slink, MPC_CONF_STRING, "Link protocol (connected, connectionless)."),
															PARAM("progress", ofi->sprogress, MPC_CONF_STRING, "Progress model (inline, auto)."),
															PARAM("endpoints", ofi->sep_type, MPC_CONF_STRING, "Enpoint types (connected, datagram, unspecified)."),
															PARAM("address", ofi->sav_type, MPC_CONF_STRING, "Address vector types (map, table, unspecified)."),
															PARAM("ressource", ofi->srm_type, MPC_CONF_STRING, "Ressource management types (enabled, disabled, unspecified)."),
															PARAM("provider", ofi->provider, MPC_CONF_STRING, "Provider to use (as shown in 'fi_info' command)"),
	                                                        NULL);

	__driver_ofi_unfold(driver);

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
	switch (driver->type)
	{
#ifdef MPC_USE_OFI
	case SCTK_RTCFG_net_driver_ofi:
		__driver_ofi_unfold(driver);
		break;
#endif
	default:
		return;
	}
}


static inline mpc_conf_config_type_t *__mpc_lowcomm_driver_conf_default_driver(char * config_name, char * driver_type)
{
	struct _mpc_lowcomm_config_struct_net_driver_config * new_conf = sctk_malloc(sizeof(struct _mpc_lowcomm_config_struct_net_driver_config));
	assume(new_conf != NULL);

	memset(new_conf, 0, sizeof(struct _mpc_lowcomm_config_struct_net_driver_config));

	snprintf(new_conf->name, MPC_CONF_STRING_SIZE, "%s", config_name);

	mpc_conf_config_type_t *driver = NULL;

	if(!strcmp(driver_type, "shm"))
	{
		driver = __init_driver_shm(&new_conf->driver);
	}
	else if(!strcmp(driver_type, "tcp"))
	{
		driver = __init_driver_tcp(&new_conf->driver);
	}
#ifdef MPC_USE_OFI
	else if(!strcmp(driver_type, "ofi"))
	{
		driver = __init_driver_ofi(&new_conf->driver);
	}
#endif
#if defined(MPC_USE_INFINIBAND)
	else if(!strcmp(driver_type, "ib"))
	{
		driver = __init_driver_ib(&new_conf->driver);
	}
#endif
#if defined(MPC_USE_PORTALS)
	else if(!strcmp(driver_type, "portals"))
	{
		driver = __init_driver_portals(&new_conf->driver);
	}
#endif

	if(!driver)
	{
		bad_parameter("Cannot create default config for driver '%s': no such driver", driver_type);
	}
	else
	{
		assume(!strcmp(driver_type, driver->name) );
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

	return  mpc_conf_config_type_elem_get_inner(eall_configs);
}

static inline mpc_conf_config_type_t * ___mpc_lowcomm_driver_instanciate_from_default(mpc_conf_config_type_t * config)
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

		struct _mpc_lowcomm_config_struct_net_driver_config * unfold = _mpc_lowcomm_conf_driver_unfolded_get(config->name);

		if(!unfold)
		{
			mpc_conf_config_type_t * new_config = ___mpc_lowcomm_driver_instanciate_from_default(config);
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

	mpc_conf_config_type_t * shm = __mpc_lowcomm_driver_conf_default_driver("shmconfigmpi", "shm");
	mpc_conf_config_type_t * tcp = __mpc_lowcomm_driver_conf_default_driver("tcpconfigmpi", "tcp");

#if defined(MPC_USE_PORTALS)
	mpc_conf_config_type_t * portals = __mpc_lowcomm_driver_conf_default_driver("portalsconfigmpi", "portals");
#endif
#if defined(MPC_USE_INFINIBAND)
	mpc_conf_config_type_t * ib = __mpc_lowcomm_driver_conf_default_driver("ibconfigmpi", "ib");
#endif
#if defined(MPC_USE_OFI)
	mpc_conf_config_type_t * ofi = __mpc_lowcomm_driver_conf_default_driver("oficonfigmpi", "ofi");
#endif

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("configs",
	                                                        PARAM("shmconfigmpi", shm, MPC_CONF_TYPE, "Default configuration for the SHM driver"),
	                                                        PARAM("tcpconfigmpi", tcp, MPC_CONF_TYPE, "Default configuration for the TCP driver"),
#if defined(MPC_USE_PORTALS)
	                                                        PARAM("portalsconfigmpi", portals, MPC_CONF_TYPE, "Default configuration for the Portals4 Driver"),
#endif
#if defined(MPC_USE_INFINIBAND)
	                                                        PARAM("ibconfigmpi", ib, MPC_CONF_TYPE, "Default configuration for the Infiniband Driver"),
#endif
#if defined(MPC_USE_OFI)
	                                                        PARAM("oficonfigmpi", ofi, MPC_CONF_TYPE, "Default configuration for the OFI Driver"),
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

struct _mpc_lowcomm_config_struct_net_rail * _mpc_lowcomm_conf_rail_unfolded_get ( char *name )
{
    int l = 0;
	struct _mpc_lowcomm_config_struct_net_rail *ret = NULL;

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
    memset(__net_config.rails, 0, MPC_CONF_MAX_RAIL_COUNT * sizeof(struct _mpc_lowcomm_config_struct_net_rail *));
	__net_config.rails_size = 0;
}

static inline void __append_new_rail_to_unfolded(struct _mpc_lowcomm_config_struct_net_rail * rail)
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
        int offload,
        int max_ifaces,
	char *config)
{
    struct _mpc_lowcomm_config_struct_net_rail * ret = sctk_malloc(sizeof(struct _mpc_lowcomm_config_struct_net_rail));
    assume(ret != NULL);

    if(_mpc_lowcomm_conf_rail_unfolded_get(name))
    {
        bad_parameter("Cannot append the '%s' rail twice", name);
    }

    /* This fills in the struct */

    memset(ret, 0, sizeof(struct _mpc_lowcomm_config_struct_net_rail));

    /* For unfolded retrieval */
    snprintf(ret->name, MPC_CONF_STRING_SIZE, "%s", name);
    ret->priority = priority;
    snprintf(ret->device, MPC_CONF_STRING_SIZE, "%s", device);
    snprintf(ret->any_source_polling.srange, MPC_CONF_STRING_SIZE, "%s", idle_poll_range);
    ret->any_source_polling.range = sctk_rail_convert_polling_set_from_string(idle_poll_range);
    snprintf(ret->any_source_polling.strigger, MPC_CONF_STRING_SIZE, "%s", idle_poll_trigger);
    ret->any_source_polling.trigger = sctk_rail_convert_polling_set_from_string(idle_poll_trigger);
    snprintf(ret->topology, MPC_CONF_STRING_SIZE, "%s", topology);
    ret->ondemand = ondemand;
    ret->rdma = rdma;
    ret->offload = offload;
    ret->max_ifaces = max_ifaces;
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
	                                                         PARAM("rdma", &ret->rdma, MPC_CONF_BOOL, "Can this rail provide RDMA capabilities"),
	                                                         PARAM("offload", &ret->offload, MPC_CONF_BOOL, "Can this rail provide tag offload capabilities"),
	                                                         PARAM("maxifaces", &ret->max_ifaces, MPC_CONF_INT, "Maximum number of rails instances that can be used for multirail"),
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
    mpc_conf_config_type_t *shm_mpi = __new_rail_conf_instance("shmmpi", 99, "default", "machine", "socket", "fully", 0, 0, 0, 0, "shmconfigmpi");
    mpc_conf_config_type_t *tcp_mpi = __new_rail_conf_instance("tcpmpi", 9, "default", "machine", "socket", "ring", 1, 0, 0, 1, "tcpconfigmpi");
#ifdef MPC_USE_PORTALS
    mpc_conf_config_type_t *portals_mpi = __new_rail_conf_instance("portalsmpi", 6, "default", "machine", "socket", "ring", 1, 1, 1, 1, "portalsconfigmpi");
#endif
#ifdef MPC_USE_INFINIBAND
    mpc_conf_config_type_t *ib_mpi = __new_rail_conf_instance("ibmpi", 1, "!mlx.*", "machine", "socket", "ring", 1, 1, 0, 0, "ibconfigmpi");
#endif
#ifdef MPC_USE_OFI
    mpc_conf_config_type_t *ofi_mpi = __new_rail_conf_instance("ofimpi", 1, "default", "machine", "socket", "ring", 1, 1, 0, 0, "oficonfigmpi");
#endif

  	mpc_conf_config_type_t *rails = mpc_conf_config_type_init("rails",
	                                                         PARAM("shmmpi", shm_mpi, MPC_CONF_TYPE, "A rail with only SHM"),
                                                             PARAM("tcpmpi", tcp_mpi, MPC_CONF_TYPE, "A rail with TCP and SHM"),
#ifdef MPC_USE_PORTALS
                                                             PARAM("portalsmpi", portals_mpi, MPC_CONF_TYPE, "A rail with Portals 4"),
#endif
#ifdef MPC_USE_INFINIBAND
                                                             PARAM("ibmpi", ib_mpi, MPC_CONF_TYPE, "A rail with Infiniband"),
#endif
#ifdef MPC_USE_OFI
                                                             PARAM("ofimpi", ofi_mpi, MPC_CONF_TYPE, "A rail with OFI"),
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
                                    0,
                                    1,
                                    "tcpconfigmpi");
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


static inline int ___parse_rail_gate(struct _mpc_lowcomm_config_struct_net_gate * cur_unfolded_gate,
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

static inline void __mpc_lowcomm_rail_unfold_gates(struct _mpc_lowcomm_config_struct_net_rail *unfolded_rail,
												   mpc_conf_config_type_t *gates_type)
{
	int i;

	for(i = 0 ; i < mpc_conf_config_type_count(gates_type); i++)
    {
        mpc_conf_config_type_elem_t* gate = mpc_conf_config_type_nth(gates_type, i);
		struct _mpc_lowcomm_config_struct_net_gate * cur_unfolded_gate = &unfolded_rail->gates[unfolded_rail->gates_size];

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
			struct _mpc_lowcomm_config_struct_net_rail *unfolded_rail = _mpc_lowcomm_conf_rail_unfolded_get(rail->name);
			mpc_conf_config_type_t *gates_type = mpc_conf_config_type_elem_get_inner(gates);

			__mpc_lowcomm_rail_unfold_gates(unfolded_rail, gates_type);
		}
		else
		{
			bad_parameter("There should be a gate entry in rail %s", rail->name );
		}
	}

	/* Now check that rail configs are known */
	for(i = 0 ; i < mpc_conf_config_type_count(all_rails); i++)
    {
        mpc_conf_config_type_elem_t* rail = mpc_conf_config_type_nth(all_rails, i);
		mpc_conf_config_type_t *rail_type = mpc_conf_config_type_elem_get_inner(rail);

		mpc_conf_config_type_elem_t *config  = mpc_conf_config_type_get(rail_type, "config");
		assume(config != NULL);
		char * conf_val = mpc_conf_type_elem_get_as_string(config);

		if(!_mpc_lowcomm_conf_driver_unfolded_get(conf_val))
		{
			mpc_conf_config_type_elem_print(rail, MPC_CONF_FORMAT_XML);
			bad_parameter("There is no driver configuration %s in mpcframework.lowcomm.networing.configs", conf_val);
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
	snprintf(ar1, MPC_CONF_STRING_SIZE, "%s", rail1);

	mpc_conf_config_type_t *rails = NULL;

	if(rail2)
	{
		char *ar2 = malloc(sizeof(char)*MPC_CONF_STRING_SIZE);
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
#ifdef MPC_USE_PORTALS
	                                                           PARAM("portals4", ___mpc_lowcomm_cli_conf_option_init("portals4", "portalsmpi", NULL), MPC_CONF_TYPE, "Combination of Portals and SHM"),
#endif
#ifdef MPC_USE_INFINIBAND
	                                                           PARAM("ib", ___mpc_lowcomm_cli_conf_option_init("ib", "shmmpi", "ibmpi"), MPC_CONF_TYPE, "Combination of Infiniband and SHM"),
#endif
#ifdef MPC_USE_OFI
	                                                           PARAM("ofi", ___mpc_lowcomm_cli_conf_option_init("ofi", "shmmpi", "ofimpi"), MPC_CONF_TYPE, "Combination of OFI and SHM"),
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
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "%s", "portals4");
#elif defined(MPC_USE_INFINIBAND)
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "%s", "ib");
#else
	snprintf(__net_config.cli_default_network, MPC_CONF_STRING_SIZE, "%s", "tcp");
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

#ifdef MPC_USE_INFINIBAND
static inline mpc_conf_config_type_t * __init_infiniband_global_conf(void)
{
	struct _mpc_lowcomm_config_struct_ib_global *ibg = &__lowcomm_conf.infiniband;

	ibg->mmu_cache_enabled = 1;
	ibg->mmu_cache_entry_count = 1024;
	ibg->mmu_cache_maximum_size = 4 * 1024 * 1024 * 1024llu;
	ibg->mmu_cache_maximum_pin_size = 1024 * 1024 * 1024llu;


	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("ibmmu",
															PARAM("enabled", &ibg->mmu_cache_enabled, MPC_CONF_BOOL, "Defines if the MMU cache is enabled."),
															PARAM("count", &ibg->mmu_cache_entry_count, MPC_CONF_INT, "Number of entries to keep in the cache."),
															PARAM("maxsize", &ibg->mmu_cache_maximum_size, MPC_CONF_LONG_INT, "Total size of entries to keep in the cache."),
															PARAM("maxpin", &ibg->mmu_cache_maximum_pin_size, MPC_CONF_LONG_INT, "Maximum size of an pinned entry."),
															NULL);
	assume(ret != NULL);

	return ret;
}
#endif

static struct _mpc_lowcomm_config_struct_protocol __protocol_conf;

static struct _mpc_lowcomm_config_struct_protocol *__mpc_lowcomm_proto_conf_init()
{
	memset(&__protocol_conf, 0, sizeof(struct _mpc_lowcomm_config_struct_protocol));
	return &__protocol_conf;
}

struct _mpc_lowcomm_config_struct_protocol *_mpc_lowcomm_config_proto_get(void)
{
	return &__protocol_conf;
}

static inline mpc_conf_config_type_t *__mpc_lowcomm_protocol_conf_init(void)
{
	struct _mpc_lowcomm_config_struct_protocol *proto = __mpc_lowcomm_proto_conf_init();

        proto->multirail_enabled = 1; /* default multirail enabled */
	proto->rndv_mode         = 1; /* default rndv get */
	proto->offload           = 0; /* default no offload */
        snprintf(proto->transports, MPC_CONF_STRING_SIZE, "tcp");
        snprintf(proto->devices, MPC_CONF_STRING_SIZE, "any");

	mpc_conf_config_type_t *ret = mpc_conf_config_type_init("protocol",
			PARAM("verbosity", &mpc_common_get_flags()->verbosity, MPC_CONF_INT, "Debug level message (1-3)"),
			PARAM("rndvmode", &proto->rndv_mode, MPC_CONF_INT, "Type of rendez-vous to use (default: mode get)"),
			PARAM("offload", &proto->offload, MPC_CONF_INT, "Force offload if possible (ie offload interface available)"),
			PARAM("multirailenabled", &proto->multirail_enabled, MPC_CONF_INT, "Is multirail enabled ?"),
			PARAM("transports", proto->transports, MPC_CONF_STRING, "Coma separated list of supported transports (tcp, ptl, all)"),
			PARAM("devices", proto->devices, MPC_CONF_STRING, "Coma separated list of devices to use (eth0, ptl0, any)"),
			NULL);

	return ret;
}

static inline mpc_conf_config_type_t * __init_workshare_conf(void)
{
	struct _mpc_lowcomm_workshare_config *ws = &__lowcomm_conf.workshare;
	ws->schedule = 2;
	ws->steal_schedule = 2;
	ws->chunk_size = 1;
	ws->steal_chunk_size = 1;
	ws->enable_stealing = 1;
	ws->steal_from_end = 0;
	ws->steal_mode = WS_STEAL_MODE_ROUNDROBIN;

	char * tmp;
	if ((tmp = getenv("WS_SCHEDULE")) != NULL) {
    int ok = 0;
    int offset = 0;
    int ws_schedule = 2;
    if (strncmp(tmp, "dynamic", 7) == 0) {
      ws_schedule = 1;
      offset = 7;
      ok = 1;
    }
    if (strncmp(tmp, "guided", 6) == 0) {
      ws_schedule = 2;
      offset = 6;
      ok = 1;
    }
    if (strncmp(tmp, "static", 6) == 0) {
      ws_schedule = 0;
      offset = 6;
      ok = 1;
    }
  ws->schedule = ws_schedule;
    if (ok) {
      int chunk_size = 0;
      /* Check for chunk size, if present */
      switch (tmp[offset]) {
        case ',':
          chunk_size = atoi(&tmp[offset + 1]);
          if (chunk_size <= 0) {
            fprintf(stderr, "Warning: Incorrect chunk size within WS_SCHEDULE "
                "variable: <%s>\n",
                tmp);
            chunk_size = 0;
          } else {
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
    } else {
      fprintf(stderr, "Warning: Unknown schedule <%s> (must be guided, "
          "dynamic or static),"
          " fallback to default schedule <guided>\n",
          tmp);
    }
  }

  if ((tmp = getenv("WS_STEAL_SCHEDULE")) != NULL) {
    int ok = 0;
    int offset = 0;
    int ws_steal_schedule = 2;
    if (strncmp(tmp, "dynamic", 7) == 0) {
      ws_steal_schedule = 1;
      offset = 7;
      ok = 1;
    }
    if (strncmp(tmp, "guided", 6) == 0) {
      ws_steal_schedule = 2;
      offset = 6;
      ok = 1;
    }
    if (strncmp(tmp, "static", 6) == 0) {
      ws_steal_schedule = 0;
      offset = 6;
      ok = 1;
    }
    ws->steal_schedule = ws_steal_schedule;
    if (ok) {
      int chunk_size = 0;
      /* Check for chunk size, if present */
      switch (tmp[offset]) {
        case ',':
          chunk_size = atoi(&tmp[offset + 1]);
          if (chunk_size <= 0) {
            fprintf(stderr, "Warning: Incorrect chunk size within WS_STEAL_SCHEDULE "
                "variable: <%s>\n",
                tmp);
            chunk_size = 0;
          } else {
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
    } else {
      fprintf(stderr, "Warning: Unknown schedule <%s> (must be guided "
          "or dynamic),"
          " fallback to default schedule <guided>\n",
          tmp);
    }
  }

  /******* WS STEAL MODE *********/
  if ((tmp = getenv("WS_STEAL_MODE")) != NULL) {
    int ws_steal_mode = strtol(tmp, NULL, 10);
    if (isdigit(tmp[0]) && ws_steal_mode >= 0 &&
        ws_steal_mode < WS_STEAL_MODE_COUNT) {
      ws->steal_mode = ws_steal_mode;
    }
    else
    {
      if((strcmp(tmp,"random") == 0) )
        ws->steal_mode = WS_STEAL_MODE_RANDOM;
      else if(strcmp(tmp,"roundrobin") == 0)
        ws->steal_mode = WS_STEAL_MODE_ROUNDROBIN;
      else if(strcmp(tmp,"producer") == 0)
        ws->steal_mode = WS_STEAL_MODE_PRODUCER;
      else if(strcmp(tmp,"less_stealers") == 0)
        ws->steal_mode = WS_STEAL_MODE_LESS_STEALERS;
      else if(strcmp(tmp,"less_stealers_producer") == 0)
        ws->steal_mode = WS_STEAL_MODE_LESS_STEALERS_PRODUCER;
      else if(strcmp(tmp,"topological") == 0)
        ws->steal_mode = WS_STEAL_MODE_TOPOLOGICAL;
      else if(strcmp(tmp,"strictly_topological") == 0)
        ws->steal_mode = WS_STEAL_MODE_STRICTLY_TOPOLOGICAL;
    }
  }

  if ((tmp = getenv("WS_STEAL_FROM_END")) != NULL) {
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
  if ((tmp = getenv("WS_ENABLE_STEALING")) != NULL) {
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
															PARAM("schedule", &ws->schedule, MPC_CONF_LONG_INT, "Workshare schedule"),
															PARAM("stealschedule", &ws->steal_schedule, MPC_CONF_LONG_INT, "Workshare steal schedule"),
															PARAM("chunksize", &ws->chunk_size, MPC_CONF_LONG_INT, "Workshare chunk size"),
															PARAM("stealchunksize", &ws->steal_chunk_size, MPC_CONF_LONG_INT, "Workshare chunk size"),
															NULL);
	assume(ret != NULL);
	return ret;
}


/************
 * MEM POOL *
 ************/

static inline void __mem_pool_defaults( void )
{
	struct _mpc_lowcomm_config_mem_pool *mpool = &__lowcomm_conf.memorypool;

	mpool->enabled = 1;
	mpool->size = 1024 * 1024 * 20;
	mpool->autodetect = 1;
	mpool->force_process_linear = 1;
	mpool->per_proc_size = 1024 * 1024 * 20;
}

mpc_conf_config_type_t *__init_mem_pool_config( void )
{
	  struct _mpc_lowcomm_config_mem_pool *mpool = &__lowcomm_conf.memorypool;

	mpc_conf_config_type_t *conf = mpc_conf_config_type_init( "memorypool",
								PARAM( "enabled", &mpool->enabled, MPC_CONF_BOOL,
										"Enable the MPI_Alloc_mem shared memory pool" ),
								PARAM( "size", &mpool->size, MPC_CONF_LONG_INT,
										"Size of the MPI_Alloc_mem pool" ),
								PARAM( "autodetect", &mpool->autodetect, MPC_CONF_BOOL,
										"Allow the MPI_Alloc_mem pool to grow linear for some apps" ),
								PARAM( "forcelinear", &mpool->force_process_linear, MPC_CONF_BOOL,
										"Force the size to be a quantum per local process" ),
								PARAM( "quantum", &mpool->per_proc_size, MPC_CONF_LONG_INT,
										"Quantum to allocate to each process when linear forced" ),
								NULL );

	return conf;
}

/************
Init and Release
**************/

static void __lowcomm_conf_default(void)
{
#ifdef SCTK_USE_CHECKSUM
	__lowcomm_conf.checksum = 0;
#endif
#ifdef MPC_USE_INFINIBAND
	__init_infiniband_global_conf();
#endif
	__mem_pool_defaults();
}

void _mpc_lowcomm_config_register(void)
{
	__lowcomm_conf_default();

	mpc_conf_config_type_t *coll     = __mpc_lowcomm_coll_conf_init();
	mpc_conf_config_type_t *networks = __mpc_lowcomm_network_conf_init();
	mpc_conf_config_type_t *protocol = __mpc_lowcomm_protocol_conf_init();
	mpc_conf_config_type_t *workshare = __init_workshare_conf();
	mpc_conf_config_type_t *mempool = __init_mem_pool_config();


        mpc_conf_config_type_t *lowcomm = mpc_conf_config_type_init("lowcomm",
#ifdef SCTK_USE_CHECKSUM
                                                                    PARAM("checksum", &__lowcomm_conf.checksum, MPC_CONF_BOOL, "Enable buffer checksum for P2P messages"),
#endif
#ifdef MPC_USE_INFINIBAND
                                                                    PARAM("ibmmu", __init_infiniband_global_conf(), MPC_CONF_TYPE, "Infiniband global Memory Management Unit (MMU) configuration."),
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
