/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK__INFINIBAND_IBUFS_H_
#define __SCTK__INFINIBAND_IBUFS_H_

#include <infiniband/verbs.h>
#include <mpc_common_types.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_debug.h>

#include "ib.h"
#include "ibmmu.h"

enum   _mpc_lowcomm_ib_cq_type_t
{
	MPC_LOWCOMM_IB_SEND_CQ,
	MPC_LOWCOMM_IB_RECV_CQ
};

struct sctk_rail_info_s;
struct _mpc_lowcomm_ib_rail_info_s;
struct _mpc_lowcomm_ib_qp_s;
struct _mpc_lowcomm_ib_ibuf_rdma_region_s;

/**
 * Description: NUMA aware buffers pool interface.
 *  Buffers are stored together in regions. Buffers are
 *  deallocated region per region.
 *  TODO: Use HWLOC for detecting which core is the closest to the
 *  IB card.
 */

/*-----------------------------------------------------------
*  STRUCTURES
*----------------------------------------------------------*/
typedef struct   _mpc_lowcomm_ib_ibuf_header_s
{
	/* Protocol used */
	int                        piggyback;
	int                        src_task;
	int                        dest_task;
	_mpc_lowcomm_ib_protocol_t protocol;
	/* Poison */
	char                        poison;
}_mpc_lowcomm_ib_ibuf_header_t;

#define IBUF_GET_HEADER(buffer)             ( (_mpc_lowcomm_ib_ibuf_header_t *)buffer)
#define IBUF_GET_HEADER_SIZE    (sizeof(_mpc_lowcomm_ib_ibuf_header_t) )
#define IBUF_GET_PROTOCOL(buffer)           (IBUF_GET_HEADER(buffer)->protocol)
#define IBUF_SET_PROTOCOL(buffer, x)        (IBUF_GET_HEADER(buffer)->protocol = x)
/* Get the piggyback of the eager message */
#define IBUF_GET_EAGER_PIGGYBACK(buffer)    (IBUF_GET_HEADER(buffer)->piggyback)

#define IBUF_SET_DEST_TASK(buffer, x)       (IBUF_GET_HEADER(buffer)->dest_task = x)
#define IBUF_GET_DEST_TASK(buffer)          (IBUF_GET_HEADER(buffer)->dest_task)
#define IBUF_SET_SRC_TASK(buffer, x)        (IBUF_GET_HEADER(buffer)->src_task = x)
#define IBUF_GET_SRC_TASK(buffer)           (IBUF_GET_HEADER(buffer)->src_task)
#define IBUF_GET_CHANNEL(ibuf)              (ibuf->region->channel)

#define IMM_DATA_NULL              ~0
#define IMM_DATA_RDMA_PIGGYBACK    (0x1 << 31)
#define IMM_DATA_RDMA_MSG          (0x1 << 30)

/* Release the buffer after freeing */
#define IBUF_RELEASE               (1 << 0)
/* Do not release the buffer after freeing */
#define IBUF_DO_NOT_RELEASE        (1 << 1)

/* Only for debugging */
#define IBUF_POISON                0x77
#define IBUF_SET_POISON(buffer)    (IBUF_GET_HEADER(buffer)->poison = IBUF_POISON)
#define IBUF_GET_POISON(buffer)    (IBUF_GET_HEADER(buffer)->poison)
#define IBUF_CHECK_POISON(buffer) \
	do\
	{                                                                                                                                                                                               \
		if(IBUF_GET_HEADER(buffer)->poison != IBUF_POISON)\
		{\
			mpc_common_debug_error("Wrong header received from buffer %p (got=%d expected=%d %p)", buffer, IBUF_GET_HEADER(buffer)->poison, IBUF_POISON, &IBUF_GET_HEADER(buffer)->poison); \
			assume(0); \
		}\
	} while(0)

typedef struct _mpc_lowcomm_ib_ibuf_desc_s
{
	union
	{
		struct ibv_recv_wr recv;
		struct ibv_send_wr send;
	}                           wr;

	union
	{
		struct ibv_recv_wr *recv;
		struct ibv_send_wr *send;
	}                           bad_wr;

	struct ibv_sge              sg_entry;

	struct sctk_net_ibv_ibuf_s *next; /**< ibufs can be chained together. */
} _mpc_lowcomm_ib_ibuf_desc_t;

/** NUMA aware pool of buffers */
typedef struct _mpc_lowcomm_ib_ibuf_numa_s
{
	mpc_common_spinlock_t                        lock;    /**< lock when moving pointers */
	char                                         _padd[128];
	mpc_common_spinlock_t                        srq_cache_lock;
	char                                         _padd2[128];
	char                                         is_srq_pool;       /**< if this is a SRQ pool */
	struct _mpc_lowcomm_ib_ibuf_region_s *       regions;           /**< DL list of regions */
	struct _mpc_lowcomm_ib_ibuf_s *              free_entry;        /**< flag to the first free header */
	struct _mpc_lowcomm_ib_ibuf_s *              free_srq_cache;    /**< Free SRQ buffers */
	unsigned int                                 nb;                /**< Number of buffers created total */
	OPA_int_t                                    free_nb;           /**< Number of buffers free */
	OPA_int_t                                    free_srq_nb;       /**< Number of SRQ buffers free */
	int                                          free_srq_cache_nb; /**< Number of SRQ buffers free in cache */

	struct _mpc_lowcomm_ib_topology_numa_node_s *numa_node;         /**< Pointer to the topological NUMA node structure */

	/* MMU is now at a node level. We got many issues while
	 * sharing the MMU accross very large NUMA nodes (>=128 cores) */
	/* FIXME: create a NUMA interface */
} _mpc_lowcomm_ib_ibuf_numa_t __attribute__( (__aligned__(64) ) );

/** Channel where the region has been allocated */
enum _mpc_lowcomm_ib_ibuf_channel_t
{
	MPC_LOWCOMM_IB_RC_SR_CHANNEL = 1 << 0,
	MPC_LOWCOMM_IB_RDMA_CHANNEL  = 1 << 1,
	MPC_LOWCOMM_IB_SEND_CHANNEL  = 1 << 2,
	MPC_LOWCOMM_IB_RECV_CHANNEL  = 1 << 3,
};

__UNUSED__ static void _mpc_lowcomm_ib_ibuf_channel_t_print(char *str, enum _mpc_lowcomm_ib_ibuf_channel_t channel);

static void _mpc_lowcomm_ib_ibuf_channel_t_print(char *str, enum _mpc_lowcomm_ib_ibuf_channel_t channel)
{
	str[0] = (channel & MPC_LOWCOMM_IB_RC_SR_CHANNEL) == MPC_LOWCOMM_IB_RC_SR_CHANNEL ? '1' : '0';
	str[1] = (channel & MPC_LOWCOMM_IB_RDMA_CHANNEL) == MPC_LOWCOMM_IB_RDMA_CHANNEL ? '1' : '0';
	str[2] = (channel & MPC_LOWCOMM_IB_SEND_CHANNEL) == MPC_LOWCOMM_IB_SEND_CHANNEL ? '1' : '0';
	str[3] = (channel & MPC_LOWCOMM_IB_RECV_CHANNEL) == MPC_LOWCOMM_IB_RECV_CHANNEL ? '1' : '0';
	str[4] = '\0';
}

/** Region of buffers where several buffers are allocated */
typedef struct _mpc_lowcomm_ib_ibuf_region_s
{
	/* Locks */
	mpc_common_spinlock_t                 lock;
	void *                                buffer_addr; /**< Address of the buffer */
	struct _mpc_lowcomm_ib_ibuf_region_s *next;
	struct _mpc_lowcomm_ib_ibuf_region_s *prev;
	uint32_t                              nb;                  /**< Number of buffer for the region */
	int                                   size_ibufs;          /**< Size of the buffers */
	uint32_t                              nb_previous;         /**< Number of buffer for the region (previous)*/
	int                                   size_ibufs_previous; /**< Size of the buffers (previous)*/
	size_t                                allocated_size;      /**< Memory allocated for the region */
	struct _mpc_lowcomm_ib_rail_info_s *          rail;                /**< A region is associated to a rail */
	_mpc_lowcomm_ib_mmu_entry_t *                 mmu_entry;           /**< MMU entry */
	uint32_t                              polled_nb;           /**< Number of messages polled */
	struct _mpc_lowcomm_ib_ibuf_s *       list;                /** List of buffers */
	struct _mpc_lowcomm_ib_ibuf_s *       ibuf;                /**< Pointer to the addr where ibufs are  allocated */
	enum _mpc_lowcomm_ib_ibuf_channel_t   channel;             /**< Channel where the region has been allocated */
	/* Specific to SR */
	struct _mpc_lowcomm_ib_ibuf_numa_s *  node;
	/*** Specific to RDMA ***/
	/* Pointers to 'head' and 'tail' pointers */
	struct _mpc_lowcomm_ib_ibuf_s *       head;
	struct _mpc_lowcomm_ib_ibuf_s *       tail;
	struct _mpc_lowcomm_ib_qp_s *                 remote;
	/* For clock algorithm */
	int                                   R_bit;
} _mpc_lowcomm_ib_ibuf_region_t;

/** Poll of ibufs */
typedef struct _mpc_lowcomm_ib_ibuf_poll_s
{
	/* Lock to authorize only 1 task to post
	 * new buffers in SRQ */
	mpc_common_spinlock_t               post_srq_lock;
	struct _mpc_lowcomm_ib_ibuf_numa_s *node_srq_buffers;

	struct _mpc_lowcomm_ib_ibuf_numa_s *default_node;
} _mpc_lowcomm_ib_ibuf_poll_t;

/** type of an ibuf */
enum _mpc_lowcomm_ib_ibuf_status
{
	BUSY_FLAG                   = 11,
	FREE_FLAG                   = 22,
	RDMA_READ_IBUF_FLAG         = 33,
	RDMA_WRITE_IBUF_FLAG        = 44,
	RDMA_FETCH_AND_OP_IBUF_FLAG = 45,
	RDMA_CAS_IBUF_FLAG          = 46,
	NORMAL_IBUF_FLAG            = 55,
	SEND_IBUF_FLAG              = 66,
	SEND_INLINE_IBUF_FLAG       = 77,
	RECV_IBUF_FLAG              = 88,
	BARRIER_IBUF_FLAG           = 99,
	EAGER_RDMA_POLLED           = 1010,
	RDMA_WRITE_INLINE_IBUF_FLAG = 1111,
};

/** Describes an ibuf */
typedef struct _mpc_lowcomm_ib_ibuf_s
{
	/* XXX: desc must be first in the structure
	 * for data alignment troubles */
	struct _mpc_lowcomm_ib_ibuf_desc_s    desc;
	struct _mpc_lowcomm_ib_ibuf_region_s *region;
	unsigned int                          index;
	unsigned char *                       buffer; /**< pointer to the buffer */
	size_t                                size;   /**< size of the buffer */
	enum _mpc_lowcomm_ib_ibuf_status      flag;   /**< status of the buffer */
	/* the following infos aren't transmitted by the network */
	struct _mpc_lowcomm_ib_qp_s *                 remote;
	void *                                supp_ptr;
	char                                  in_srq; /**< If the buffer is in a shaed receive queue */
	char                                  to_release;
	struct ibv_wc                         wc;     /**< We store the wc of the ibuf */
	/* Linked list used for CP */
	struct _mpc_lowcomm_ib_ibuf_s *       next;
	struct _mpc_lowcomm_ib_ibuf_s *       prev;
	int *                                 poison_flag_head; /**< Poison flag */
	volatile int *                        head_flag;        /**< Head flag */
	size_t *                              size_flag;        /**< Size flag */
	int                                   previous_flag;    /**< Previous flag for RDMA */
	double                                polled_timestamp; /**< Timestamp when the ibuf has been polled from the CQ */
	struct sctk_rail_info_s *             rail;             /**< We save the rail where the message has been pooled*/
	enum _mpc_lowcomm_ib_cq_type_t        cq;
	uint32_t                              send_imm_data;
} _mpc_lowcomm_ib_ibuf_t;

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/
void _mpc_lowcomm_ib_ibuf_pool_init(struct _mpc_lowcomm_ib_rail_info_s *rail);
void _mpc_lowcomm_ib_ibuf_pool_free(struct _mpc_lowcomm_ib_rail_info_s *rail);

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_send_tst(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, struct _mpc_lowcomm_ib_qp_s *remote, size_t *size);

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_send(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, struct _mpc_lowcomm_ib_qp_s *remote, size_t *size);

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_send_sr(struct _mpc_lowcomm_ib_rail_info_s *rail_ib);

int _mpc_lowcomm_ib_ibuf_srq_post(struct _mpc_lowcomm_ib_rail_info_s *rail_ib);

void _mpc_lowcomm_ib_ibuf_srq_release(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_ibuf_t *ibuf);

void _mpc_lowcomm_ib_ibuf_print(_mpc_lowcomm_ib_ibuf_t *ibuf, char *desc);

void *_mpc_lowcomm_ib_ibuf_get_buffer(_mpc_lowcomm_ib_ibuf_t *ibuf);

/*-----------------------------------------------------------
*  WR INITIALIZATION
*----------------------------------------------------------*/

void _mpc_lowcomm_ib_ibuf_rdma_recv_init(_mpc_lowcomm_ib_ibuf_t *ibuf, void *local_address, uint32_t lkey);

int _mpc_lowcomm_ib_ibuf_send_inline_init(_mpc_lowcomm_ib_ibuf_t *ibuf, size_t size);

int _mpc_lowcomm_ib_ibuf_write_with_imm_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                             void *local_address,
                                             uint32_t lkey,
                                             void *remote_address,
                                             uint32_t rkey,
                                             unsigned int len,
                                             char to_release,
                                             uint32_t imm_data);

int _mpc_lowcomm_ib_ibuf_write_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                    void *local_address,
                                    uint32_t lkey,
                                    void *remote_address,
                                    uint32_t rkey,
                                    unsigned int len,
                                    int send_flags,
                                    char to_release);

void _mpc_lowcomm_ib_ibuf_read_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                    void *local_address,
                                    uint32_t lkey,
                                    void *remote_address,
                                    uint32_t rkey,
                                    int len);

void _mpc_lowcomm_ib_ibuf_fetch_and_add_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                             void *fetch_addr,
                                             uint32_t lkey,
                                             void *remote_address,
                                             uint32_t rkey,
                                             uint64_t add);

void _mpc_lowcomm_ib_ibuf_compare_and_swap_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                                void *res_addr,
                                                uint32_t local_key,
                                                void *remote_address,
                                                uint32_t rkey,
                                                uint64_t comp,
                                                uint64_t new);

void _mpc_lowcomm_ib_ibuf_release(struct _mpc_lowcomm_ib_rail_info_s *rail_ib,
                                  _mpc_lowcomm_ib_ibuf_t *ibuf,
                                  int decr_free_srq_nb);

void _mpc_lowcomm_ib_ibuf_prepare(struct _mpc_lowcomm_ib_qp_s *remote,
                                  _mpc_lowcomm_ib_ibuf_t *ibuf,
                                  size_t size);

void _mpc_lowcomm_ib_ibuf_init_task(int rank, int vp);

void _mpc_lowcomm_ib_ibuf_set_node_srq_buffers(struct _mpc_lowcomm_ib_rail_info_s *rail_ib,
                                               _mpc_lowcomm_ib_ibuf_numa_t *node);

void _mpc_lowcomm_ib_ibuf_init_numa(struct _mpc_lowcomm_ib_rail_info_s *rail_ib,
                                    struct _mpc_lowcomm_ib_ibuf_numa_s *node,
                                    int nb_ibufs,
                                    char is_initial_allocation);
void _mpc_lowcomm_ib_ibuf_free_numa(struct _mpc_lowcomm_ib_ibuf_numa_s *node);

#endif
