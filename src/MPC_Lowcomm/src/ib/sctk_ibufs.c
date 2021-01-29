/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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
#include "sctk_ibufs.h"

//#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME    "IBUF"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_config.h"
#include "sctk_ib_topology.h"
#include "sctk_ibufs_rdma.h"
#include "utlist.h"
#include "sctk_rail.h"
#include <sctk_alloc.h>


static inline _mpc_lowcomm_ib_ibuf_numa_t *__ibuf_get_numaware(struct sctk_ib_rail_info_s *rail_ib)
{
	struct _mpc_lowcomm_ib_topology_numa_node_s *closest_node = _mpc_lowcomm_ib_topology_get_numa_node(rail_ib);
	_mpc_lowcomm_ib_ibuf_numa_t *node = NULL;

	node = &closest_node->ibufs;

	return node;
}

/* Init a pool of buffers on the numa node pointed by 'node'
 * FIXME: use malloc_on_node instead of memalign
 * Carreful: this function is *NOT* thread-safe */
void _mpc_lowcomm_ib_ibuf_init_numa(struct sctk_ib_rail_info_s *rail_ib,
                                    struct _mpc_lowcomm_ib_ibuf_numa_s *node,
                                    int nb_ibufs,
                                    char is_initial_allocation)
{
	LOAD_CONFIG(rail_ib);
	LOAD_POOL(rail_ib);
	_mpc_lowcomm_ib_ibuf_region_t *region = NULL;
	void *       ptr           = NULL;
	void *       ibuf;
	_mpc_lowcomm_ib_ibuf_t *ibuf_ptr;
	int          i;


	/* If this allocation is done during initialization */
	if(is_initial_allocation)
	{
		if(pool->default_node == NULL)
		{
			pool->default_node = node;
		}

		mpc_common_spinlock_init(&node->lock, 0);
		mpc_common_spinlock_init(&node->srq_cache_lock, 0);
		node->regions        = NULL;
		node->free_entry     = NULL;
		node->free_srq_cache = NULL;
		OPA_store_int(&node->free_srq_nb, 0);
		node->free_srq_cache_nb = 0;
		OPA_store_int(&node->free_nb, 0);
	}

	region = sctk_malloc_on_node(sizeof(_mpc_lowcomm_ib_ibuf_region_t), 0);
	ib_assume(region);

	/* XXX: replaced by memalign_on_node */
	sctk_posix_memalign( ( void ** )&ptr, getpagesize(), nb_ibufs * config->eager_limit);
	ib_assume(ptr);
	memset(ptr, 0, nb_ibufs * config->eager_limit);

	/* XXX: replaced by memalign_on_node */
	sctk_posix_memalign(&ibuf, getpagesize(), nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t) );
	ib_assume(ibuf);
	memset(ibuf, 0, nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t) );

	region->size_ibufs     = config->eager_limit;
	region->list           = ibuf;
	region->nb             = nb_ibufs;
	region->node           = node;
	region->rail           = rail_ib;
	region->channel        = MPC_LOWCOMM_IB_RC_SR_CHANNEL;
	region->ibuf           = ibuf;
	region->allocated_size = (nb_ibufs * (config->eager_limit + sizeof(_mpc_lowcomm_ib_ibuf_t) ) );
	DL_APPEND(node->regions, region);

	/* register buffers at once
	 * FIXME: Always task on NUMA node 0 which firs-touch all pages... really bad */
	region->mmu_entry = _mpc_lowcomm_ib_mmu_entry_new(rail_ib, ptr, nb_ibufs * config->eager_limit);
	mpc_common_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

	/* init all buffers - the last one */
	for(i = 0; i < nb_ibufs; ++i)
	{
		ibuf_ptr         = ( _mpc_lowcomm_ib_ibuf_t * )ibuf + i;
		ibuf_ptr->region = region;
		ibuf_ptr->size   = 0;
		ibuf_ptr->flag   = FREE_FLAG;
		ibuf_ptr->index  = i;

		ibuf_ptr->buffer = ( unsigned char * )( ( char * )ptr + (i * config->eager_limit) );
		ib_assume(ibuf_ptr->buffer);
		DL_APPEND(node->free_entry, ( ( _mpc_lowcomm_ib_ibuf_t * )ibuf + i) );
	}

	OPA_add_int(&node->free_nb, nb_ibufs);

	node->nb += nb_ibufs;
}

void _mpc_lowcomm_ib_ibuf_free_numa(struct _mpc_lowcomm_ib_ibuf_numa_s *node)
{
	struct _mpc_lowcomm_ib_ibuf_region_s *region, *tmp;
	_mpc_lowcomm_ib_ibuf_t *buf, *tmp2;

	DL_FOREACH_SAFE(node->free_entry, buf, tmp2)
	{
		DL_DELETE(node->free_entry, buf);
	}
	assume(node->free_entry == NULL);
	OPA_store_int(&node->free_nb, 0);
	node->nb = 0;

	DL_FOREACH_SAFE(node->free_srq_cache, buf, tmp2)
	{
		DL_DELETE(node->free_srq_cache, buf);
	}
	assume(node->free_srq_cache == NULL);
	OPA_store_int(&node->free_srq_nb, 0);
	node->free_srq_cache_nb = 0;

	DL_FOREACH_SAFE(node->regions, region, tmp)
	{
		sctk_free(region->mmu_entry->addr); /* MMU entry */
		region->mmu_entry->addr = NULL;
		_mpc_lowcomm_ib_mmu_entry_release(region->mmu_entry);
		region->mmu_entry = NULL;
		sctk_free((void*)region->list);
		region->list = NULL;
		DL_DELETE(node->regions, region);
		sctk_free(region);
		region = NULL;
	}
	assume(node->regions == NULL);
}

void _mpc_lowcomm_ib_ibuf_set_node_srq_buffers(struct sctk_ib_rail_info_s *rail_ib,
                                               _mpc_lowcomm_ib_ibuf_numa_t *node)
{
	LOAD_POOL(rail_ib);
	pool->node_srq_buffers = node;
	node->is_srq_pool      = 1;
}

/* Init the pool of buffers on each NUMA node */
void _mpc_lowcomm_ib_ibuf_pool_init(struct sctk_ib_rail_info_s *rail_ib)
{
	_mpc_lowcomm_ib_ibuf_poll_t *pool;

	pool = sctk_malloc(sizeof(_mpc_lowcomm_ib_ibuf_poll_t) );

	/* WARNING: Do not remove the following memset.
	 * Be sure that is_srq_poll is set to 0 !! */
	memset(pool, 0, sizeof(_mpc_lowcomm_ib_ibuf_poll_t) );
	mpc_common_spinlock_init(&pool->post_srq_lock, 0);
	/* update pool buffer */
	rail_ib->pool_buffers = pool;
}

void _mpc_lowcomm_ib_ibuf_pool_free(sctk_ib_rail_info_t *rail_ib)
{
	sctk_free(rail_ib->pool_buffers);
	rail_ib->pool_buffers = NULL;
}

/*-----------------------------------------------------------
*  WR INITIALIZATION
*----------------------------------------------------------*/
static inline void __recv_init(_mpc_lowcomm_ib_ibuf_t *ibuf)
{
	LOAD_CONFIG(ibuf->region->rail);

	ibuf->to_release    = IBUF_RELEASE;
	ibuf->in_srq        = 1;
	ibuf->send_imm_data = 0;

	/* For the SRQ, remote is set to 'NULL' as it is shared
	 * between all processes */
	ibuf->remote = NULL;

	ibuf->desc.wr.send.next  = NULL;
	ibuf->desc.wr.send.wr_id = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge  = 1;
	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length  = config->eager_limit;

	ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t )(ibuf->buffer);

	ibuf->flag = RECV_IBUF_FLAG;
}

static inline int __send_inline_init(_mpc_lowcomm_ib_ibuf_t *ibuf, size_t size)
{
	LOAD_CONFIG(ibuf->region->rail);
	int is_inlined = 0;


	/* If data may be inlined */
	if(size <= config->max_inline)
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
		ibuf->flag = SEND_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
		ibuf->flag = SEND_IBUF_FLAG;
	}

	ibuf->in_srq              = 0;
	ibuf->send_imm_data       = 0;
	ibuf->to_release          = IBUF_RELEASE;
	ibuf->desc.wr.send.next   = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_SEND;
	ibuf->desc.wr.send.wr_id  = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge = 1;

	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length  = size;
	ibuf->desc.sg_entry.lkey    = ibuf->region->mmu_entry->mr->lkey;
	ibuf->desc.sg_entry.addr    = ( uintptr_t )(ibuf->buffer);

	return is_inlined;
}

int _mpc_lowcomm_ib_ibuf_write_with_imm_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                             void *local_address,
                                             uint32_t lkey,
                                             void *remote_address,
                                             uint32_t rkey,
                                             unsigned int len,
                                             char to_release,
                                             uint32_t imm_data)
{
	LOAD_CONFIG(ibuf->region->rail);
	int is_inlined = 0;

	/* If data may be inlined */
	if(len <= config->max_inline)
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
		ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
		ibuf->flag = RDMA_WRITE_IBUF_FLAG;
	}

	ibuf->in_srq              = 0;
	ibuf->send_imm_data       = imm_data;
	ibuf->to_release          = to_release;
	ibuf->desc.wr.send.next   = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
	ibuf->desc.wr.send.wr_id  = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge             = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t )remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey        = rkey;

	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = imm_data;
	ibuf->desc.sg_entry.length  = len;
	ibuf->desc.sg_entry.lkey    = lkey;
	ibuf->desc.sg_entry.addr    = ( uintptr_t )local_address;

	return is_inlined;
}

int _mpc_lowcomm_ib_ibuf_write_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                    void *local_address,
                                    uint32_t lkey,
                                    void *remote_address,
                                    uint32_t rkey,
                                    unsigned int len,
                                    int send_flags,
                                    char to_release)
{
	LOAD_CONFIG(ibuf->region->rail);
	int is_inlined = 0;

	/* If data may be inlined */
	if(len <= config->max_inline)
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | send_flags;
		ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = send_flags;
		ibuf->flag = RDMA_WRITE_IBUF_FLAG;
	}

	ibuf->in_srq              = 0;
	ibuf->send_imm_data       = 0;
	ibuf->to_release          = to_release;
	ibuf->desc.wr.send.next   = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
	ibuf->desc.wr.send.wr_id  = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge             = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t )remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey        = rkey;

	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length  = len;
	ibuf->desc.sg_entry.lkey    = lkey;
	ibuf->desc.sg_entry.addr    = ( uintptr_t )local_address;

	return is_inlined;
}

void _mpc_lowcomm_ib_ibuf_read_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                    void *local_address,
                                    uint32_t lkey,
                                    void *remote_address,
                                    uint32_t rkey,
                                    int len)
{
	ibuf->in_srq                  = 0;
	ibuf->send_imm_data           = 0;
	ibuf->to_release              = IBUF_RELEASE;
	ibuf->desc.wr.send.next       = NULL;
	ibuf->desc.wr.send.opcode     = IBV_WR_RDMA_READ;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id      = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge             = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t )remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey        = rkey;

	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length  = len;
	ibuf->desc.sg_entry.lkey    = lkey;
	ibuf->desc.sg_entry.addr    = ( uintptr_t )local_address;

	ibuf->flag = RDMA_READ_IBUF_FLAG;
}

void _mpc_lowcomm_ib_ibuf_fetch_and_add_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                             void *fetch_addr,
                                             uint32_t lkey,
                                             void *remote_address,
                                             uint32_t rkey,
                                             uint64_t add)
{
	ibuf->in_srq                  = 0;
	ibuf->send_imm_data           = 0;
	ibuf->to_release              = IBUF_RELEASE;
	ibuf->desc.wr.send.next       = NULL;
	ibuf->desc.wr.send.opcode     = IBV_WR_ATOMIC_FETCH_AND_ADD;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id      = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge = 1;

	ibuf->desc.wr.send.wr.atomic.remote_addr = (uintptr_t)remote_address;
	ibuf->desc.wr.send.wr.atomic.compare_add = add;
	ibuf->desc.wr.send.wr.atomic.rkey        = rkey;


	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;

	ibuf->desc.sg_entry.length = sizeof(uint64_t);
	ibuf->desc.sg_entry.lkey   = lkey;
	ibuf->desc.sg_entry.addr   = ( uintptr_t )fetch_addr;

	ibuf->flag = RDMA_FETCH_AND_OP_IBUF_FLAG;
}

void _mpc_lowcomm_ib_ibuf_compare_and_swap_init(_mpc_lowcomm_ib_ibuf_t *ibuf,
                                                void *res_addr,
                                                uint32_t local_key,
                                                void *remote_address,
                                                uint32_t rkey,
                                                uint64_t comp,
                                                uint64_t new)
{
	ibuf->in_srq                  = 0;
	ibuf->send_imm_data           = 0;
	ibuf->to_release              = IBUF_RELEASE;
	ibuf->desc.wr.send.next       = NULL;
	ibuf->desc.wr.send.opcode     = IBV_WR_ATOMIC_CMP_AND_SWP;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id      = ( uintptr_t )ibuf;

	ibuf->desc.wr.send.num_sge = 1;

	ibuf->desc.wr.send.wr.atomic.remote_addr = (uintptr_t)remote_address;
	ibuf->desc.wr.send.wr.atomic.compare_add = comp;
	ibuf->desc.wr.send.wr.atomic.swap        = new;
	ibuf->desc.wr.send.wr.atomic.rkey        = rkey;

	ibuf->desc.wr.send.sg_list  = &(ibuf->desc.sg_entry);
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length  = sizeof(uint64_t);
	ibuf->desc.sg_entry.lkey    = local_key;
	ibuf->desc.sg_entry.addr    = (uintptr_t)res_addr;

	ibuf->flag = RDMA_CAS_IBUF_FLAG;
}

static inline char *__ibuf_print_flag(enum _mpc_lowcomm_ib_ibuf_status flag)
{
	switch(flag)
	{
		case RDMA_READ_IBUF_FLAG:
			return "RDMA_READ_IBUF_FLAG";

			break;

		case RDMA_WRITE_IBUF_FLAG:
			return "RDMA_WRITE_IBUF_FLAG";

			break;

		case RDMA_FETCH_AND_OP_IBUF_FLAG:
			return "RDMA_FETCH_AND_OP_IBUF_FLAG";

			break;

		case RDMA_CAS_IBUF_FLAG:
			return "RDMA_CAS_IBUF_FLAG";

			break;

		case RDMA_WRITE_INLINE_IBUF_FLAG:
			return "RDMA_WRITE_INLINE_IBUF_FLAG";

			break;

		case NORMAL_IBUF_FLAG:
			return "NORMAL_IBUF_FLAG";

			break;

		case RECV_IBUF_FLAG:
			return "RECV_IBUF_FLAG";

			break;

		case SEND_IBUF_FLAG:
			return "SEND_IBUF_FLAG";

			break;

		case SEND_INLINE_IBUF_FLAG:
			return "SEND_INLINE_IBUF_FLAG";

			break;

		case BARRIER_IBUF_FLAG:
			return "BARRIER_IBUF_FLAG";

			break;

		case BUSY_FLAG:
			return "BUSY_FLAG";

			break;

		case FREE_FLAG:
			return "FREE_FLAG";

			break;

		case EAGER_RDMA_POLLED:
			return "EAGER_RDMA_POLLED";

			break;
	}

	return NULL;
}

void _mpc_lowcomm_ib_ibuf_print_rdma(_mpc_lowcomm_ib_ibuf_t *ibuf, char *desc)
{
	sprintf(desc, "region       :%p\n"
	              "buffer       :%p\n"
	              "size         :%lu\n"
	              "flag         :%s\n"
	              "remote       :%p\n"
	              "in_srq       :%d\n"
	              "next         :%p\n"
	              "prev         :%p\n"
	              "sg_entry.length :%u\n"
	              "local addr   :%p\n"
	              "remote addr  :%p\n", ibuf->region,
	        ibuf->buffer,
	        ibuf->size,
	        __ibuf_print_flag(ibuf->flag),
	        ibuf->remote,
	        ibuf->in_srq,
	        ibuf->next,
	        ibuf->prev,
	        ibuf->desc.sg_entry.length,
	        ( void * )ibuf->desc.sg_entry.addr,
	        ( void * )ibuf->desc.wr.send.wr.rdma.remote_addr);
}

void _mpc_lowcomm_ib_ibuf_print(_mpc_lowcomm_ib_ibuf_t *ibuf, char *desc)
{
	char *poison = (IBUF_GET_HEADER(ibuf->buffer)->poison != IBUF_POISON) ? "NAK" : "OK";
	char  channel[5];

	_mpc_lowcomm_ib_ibuf_channel_t_print(channel, ibuf->region->channel);

	sprintf(desc, "rail         :%d\n"
	              "region       :%p\n"
	              "channel type : %s\n"
	              "buffer       :%p\n"
	              "size         :%lu\n"
	              "flag         :%s\n"
	              "remote       :%p\n"
	              "in_srq       :%d\n"
	              "next         :%p\n"
	              "prev         :%p\n"
	              "sg_entry.length :%u\n"
	              "poison       :%s", ibuf->region->rail->rail->rail_number,
	        ibuf->region,
	        channel,
	        ibuf->buffer,
	        ibuf->size,
	        __ibuf_print_flag(ibuf->flag),
	        ibuf->remote,
	        ibuf->in_srq,
	        ibuf->next,
	        ibuf->prev,
	        ibuf->desc.sg_entry.length,
	        poison);
}

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_send_sr(struct sctk_ib_rail_info_s *rail_ib)
{
	LOAD_CONFIG(rail_ib);
	_mpc_lowcomm_ib_ibuf_t *     ibuf;
	_mpc_lowcomm_ib_ibuf_numa_t *node = __ibuf_get_numaware(rail_ib);

	mpc_common_spinlock_t *lock = &node->lock;

	mpc_common_spinlock_lock(lock);

	/* Allocate additionnal buffers if no more are available */
	if(!node->free_entry)
	{
		_mpc_lowcomm_ib_ibuf_init_numa(rail_ib, node, config->size_ibufs_chunk, 0);
	}

	/* update pointers from buffer pool */
	ibuf = node->free_entry;
	DL_DELETE(node->free_entry, node->free_entry);
	OPA_decr_int(&node->free_nb);

	mpc_common_spinlock_unlock(lock);

	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_NULL_PROTOCOL);

#ifdef DEBUG_IB_BUFS
	assume(ibuf);

	if(ibuf->flag != FREE_FLAG)
	{
		mpc_common_debug_error("Wrong flag (%d) got from ibuf", ibuf->flag);
	}
#endif

	/* Prepare the buffer for sending */
	IBUF_SET_POISON(ibuf->buffer);

	return ibuf;
}

/*
 * Prepare a message to send. According to the size and the remote, the function
 * choose between the SR and the RDMA channel.
 * If '*size' == ULONG_MAX, the fonction returns in '*size' the maximum size
 * of the payload for the buffer. The user next needs to manually call sctk_ib_prepare.
 */
_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_send(struct sctk_ib_rail_info_s *rail_ib,
                                            sctk_ib_qp_t *remote,
                                            size_t *size)
{
	LOAD_CONFIG(rail_ib);
	_mpc_lowcomm_ib_ibuf_t *         ibuf = NULL;
	size_t                s;
	size_t                limit;
	_mpc_lowcomm_endpoint_state_t state;

	s = *size;
	/***** RDMA CHANNEL *****/
	state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

	if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
	{
		/* Double lock checking */
		mpc_common_spinlock_lock(&remote->rdma.flushing_lock);
		state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

		if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
		{
			/* WARNING: 'free_nb' must be decremented just after
			 * checking the state of the RDMA buffer. */
			OPA_incr_int(&remote->rdma.pool->busy_nb[REGION_SEND]);
			mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);

			limit = _mpc_lowcomm_ib_ibuf_rdma_eager_limit_get(remote);
			mpc_common_nodebug("Eager limit: %lu", limit);

			if(s == ULONG_MAX)
			{
				s = limit - IBUF_RDMA_GET_SIZE;
			}

			if( (IBUF_RDMA_GET_SIZE + s) <= limit)
			{
				mpc_common_nodebug("requested:%lu max:%lu header:%lu", s, limit, IBUF_RDMA_GET_SIZE);
				ibuf = _mpc_lowcomm_ib_ibuf_rdma_pick(remote);
				mpc_common_nodebug("Picked a rdma buffer: %p", ibuf);

				/* A buffer has been picked-up */
				if(ibuf)
				{
					mpc_common_nodebug("Picking from RDMA %d", ibuf->index);
#ifdef DEBUG_IB_BUFS
					_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

					if( (state != _MPC_LOWCOMM_ENDPOINT_CONNECTED) && (state != _MPC_LOWCOMM_ENDPOINT_FLUSHING) )
					{
						mpc_common_debug_error("Got a wrong state : %d", state);
						not_reachable();
					}
#endif
					goto exit;
				}
				else
				{
					OPA_incr_int(&remote->rdma.miss_nb);
				}
			}
			else
			{
				OPA_incr_int(&remote->rdma.miss_nb);
			}

			/* If we cannot pick a buffer from the RDMA channel, we switch to SR */
			OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_SEND]);
			_mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
		}
		else
		{
			mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
		}
	}

	s = *size;

	/***** SR CHANNEL *****/
	if(s == ULONG_MAX)
	{
		s = config->eager_limit;
	}

	if(s <= config->eager_limit)
	{
		mpc_common_nodebug("Picking from SR");

		_mpc_lowcomm_ib_ibuf_numa_t *     node = __ibuf_get_numaware(rail_ib);
		mpc_common_spinlock_t *lock = &node->lock;

		mpc_common_spinlock_lock(lock);

		/* Allocate additionnal buffers if no more are available */
		if(!node->free_entry)
		{
			_mpc_lowcomm_ib_ibuf_init_numa(rail_ib, node, config->size_ibufs_chunk, 0);
		}

		/* update pointers from buffer pool */
		ibuf = node->free_entry;
		DL_DELETE(node->free_entry, node->free_entry);
		OPA_decr_int(&node->free_nb);

		mpc_common_spinlock_unlock(lock);

		IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_NULL_PROTOCOL);

#ifdef DEBUG_IB_BUFS
		assume(ibuf);

		if(ibuf->flag != FREE_FLAG)
		{
			mpc_common_debug_error("Wrong flag (%d) got from ibuf", ibuf->flag);
			mpc_common_debug_abort();
		}
#endif
		ibuf->flag = BUSY_FLAG;
	}
	else
	{
		/* We can reach this block if there is no more slots for the RDMA channel and the
		 * requested size is larger than the size of a SR slot. At this time, we switch can switch to the
		 * buffered protocol */
		return NULL;
	}

exit:

	/* We update the size if it is equal to ULONG_MAX*/
	if(*size == ULONG_MAX)
	{
		*size = s;
	}
	else
	{
		/* Prepare the buffer for sending */
		_mpc_lowcomm_ib_ibuf_prepare(remote, ibuf, *size);
	}

	IBUF_SET_POISON(ibuf->buffer);


	_mpc_lowcomm_ib_ibuf_rdma_remote_check(rail_ib, remote);
	return ibuf;
}

/* pick a new buffer from the ibuf list. Function *MUST* be locked to avoid
 * oncurrent calls.
 * - remote: process where picking buffer. It may be NULL. In this case,
 *   we pick a buffer from the SR channel */
static inline _mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_pick_recv(struct sctk_ib_rail_info_s *rail_ib, struct _mpc_lowcomm_ib_ibuf_numa_s *node)
{
	LOAD_CONFIG(rail_ib);
	_mpc_lowcomm_ib_ibuf_t *ibuf;
	assume(node);

	/* Allocate additionnal buffers if no more are available */
	if(!node->free_entry)
	{
		_mpc_lowcomm_ib_ibuf_init_numa(rail_ib, node, config->size_recv_ibufs_chunk, 0);
	}

	/* update pointers from buffer pool */
	ibuf = node->free_entry;
	DL_DELETE(node->free_entry, node->free_entry);
	OPA_decr_int(&node->free_nb);



#ifdef DEBUG_IB_BUFS
	assume(ibuf);

	if(ibuf->flag != FREE_FLAG)
	{
		mpc_common_debug_error("Wrong flag (%d) got from ibuf", ibuf->flag);
	}

	mpc_common_nodebug("ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, node->id);
#endif


	return ibuf;
}

static inline int __srq_post(struct sctk_ib_rail_info_s *rail_ib,
                             _mpc_lowcomm_ib_ibuf_numa_t *node,
                             int force)
{
	assume(node);
	LOAD_DEVICE(rail_ib);
	LOAD_CONFIG(rail_ib);
	int                    i;
	int                    nb_posted = 0;
	_mpc_lowcomm_ib_ibuf_t *          ibuf;
	int                    rc;
	mpc_common_spinlock_t *lock = &node->lock;
	int                    nb_ibufs;
	int                    free_srq_nb;

	/* limit of buffer posted */
	free_srq_nb = OPA_load_int(&node->free_srq_nb);
	nb_ibufs    = config->max_srq_ibufs_posted - free_srq_nb;
	mpc_common_nodebug("TRY Post %d ibufs in SRQ (free:%d max:%d)", nb_ibufs, free_srq_nb, config->max_srq_ibufs_posted);


	if(nb_ibufs <= 0)
	{
		return 0;
	}

	if(force)
	{
		mpc_common_spinlock_lock(&rail_ib->pool_buffers->post_srq_lock);
	}
	else if(mpc_common_spinlock_trylock(&rail_ib->pool_buffers->post_srq_lock) != 0)
	{
		goto exit;
	}

	/* Only 1 task can post to the SRQ at the same time. No need more concurrency */
	//  if (mpc_common_spinlock_trylock(&rail_ib->pool_buffers->post_srq_lock) == 0)
	{
		/* limit of buffer posted */
		free_srq_nb = OPA_load_int(&node->free_srq_nb);
		nb_ibufs    = config->max_srq_ibufs_posted - free_srq_nb;
		mpc_common_nodebug("Post %d ibufs in SRQ (free:%d max:%d force:%d)", nb_ibufs, free_srq_nb, config->max_srq_ibufs_posted, force);

		mpc_common_spinlock_lock(lock);

		for(i = 0; i < nb_ibufs; ++i)
		{
			/* No need lock */
			ibuf = _mpc_lowcomm_ib_ibuf_pick_recv(rail_ib, node);
#ifdef DEBUG_IB_BUFS
			assume(ibuf);
#endif

			__recv_init(ibuf);
			rc = ibv_post_srq_recv(device->srq, &(ibuf->desc.wr.recv), &(ibuf->desc.bad_wr.recv) );

			/* Cannot post more buffers */
			if(rc != 0)
			{
				ibuf->flag   = FREE_FLAG;
				ibuf->in_srq = 0;

				/* change counters */
				OPA_incr_int(&node->free_nb);
				DL_PREPEND(node->free_entry, ibuf);
				break;
			}
			else
			{
				nb_posted++;
			}
		}

		mpc_common_spinlock_unlock(lock);

		OPA_add_int(&node->free_srq_nb, nb_posted);

		mpc_common_spinlock_unlock(&rail_ib->pool_buffers->post_srq_lock);
	}

exit:
	return nb_posted;
}

/* FIXME: check with buffers near IB card */
int _mpc_lowcomm_ib_ibuf_srq_post(struct sctk_ib_rail_info_s *rail_ib)
{
	LOAD_POOL(rail_ib);

	return __srq_post(rail_ib, pool->node_srq_buffers, 1);
}

static inline void __release_in_srq(struct sctk_ib_rail_info_s *rail_ib,
                                    _mpc_lowcomm_ib_ibuf_numa_t *node,
                                    int decr_free_srq_nb)
{
	if(decr_free_srq_nb)
	{
		/* limit of buffer posted */
		OPA_decr_int(&node->free_srq_nb);
	}

	__srq_post(rail_ib, node, 0);
}

void _mpc_lowcomm_ib_ibuf_srq_release(struct sctk_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	_mpc_lowcomm_ib_ibuf_numa_t *node = ibuf->region->node;

	__release_in_srq(rail_ib, node, 1);
}

/* release one buffer given as parameter.
 * is_srq: if the buffer is released from the SRQ */
void _mpc_lowcomm_ib_ibuf_release(struct sctk_ib_rail_info_s *rail_ib,
                                  _mpc_lowcomm_ib_ibuf_t *ibuf,
                                  int decr_free_srq_nb)
{
	ib_assume(ibuf);

	if(ibuf->to_release & IBUF_RELEASE)
	{
		if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RDMA_CHANNEL)
		{
			_mpc_lowcomm_ib_ibuf_rdma_release(rail_ib, ibuf);
		}
		else
		{
			ib_assume(IBUF_GET_CHANNEL(ibuf) == MPC_LOWCOMM_IB_RC_SR_CHANNEL);
			_mpc_lowcomm_ib_ibuf_numa_t *     node = ibuf->region->node;
			mpc_common_spinlock_t *lock = &node->lock;

			if(ibuf->in_srq)
			{
				/* If buffer from SRQ */
				ibuf->flag = FREE_FLAG;
				IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_NULL_PROTOCOL);

				{
					_mpc_lowcomm_ib_ibuf_numa_t *     closest_node   = __ibuf_get_numaware(rail_ib);
					mpc_common_spinlock_t *srq_cache_lock = &closest_node->srq_cache_lock;

					mpc_common_spinlock_lock(srq_cache_lock);
					const int srq_cache_nb = ++closest_node->free_srq_cache_nb;
					DL_APPEND(closest_node->free_srq_cache, ibuf);

					TODO("Number of SRQ buffers in the cache -> in the configuration")

					/* If the max number of SRQ ibufs is reached for the current node, we flush
					 * the list to the global queue of buffers */
					if(srq_cache_nb > 40)
					{
						OPA_add_int(&node->free_nb, srq_cache_nb);

						mpc_common_spinlock_lock(lock);
						DL_CONCAT(node->free_entry, closest_node->free_srq_cache);
						mpc_common_spinlock_unlock(lock);

						closest_node->free_srq_cache_nb = 0;
						closest_node->free_srq_cache    = NULL;
					}

					mpc_common_spinlock_unlock(srq_cache_lock);
				}

				/* If SRQ, we check and try to post more messages to SRQ */
				__release_in_srq(rail_ib, node, decr_free_srq_nb);
			}
			else
			{
				/* TODO: only for debugging */
#if 0
				_mpc_lowcomm_ib_ibuf_numa_t *closest_node = __ibuf_get_numaware(rail_ib);
				assume(closest_node == node);
#endif
				ibuf->flag = FREE_FLAG;
				IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_NULL_PROTOCOL);

				OPA_incr_int(&node->free_nb);
				mpc_common_spinlock_lock(lock);
				DL_APPEND(node->free_entry, ibuf);
				mpc_common_spinlock_unlock(lock);
			}
		}
	}
	else
	{
		not_reachable();
	}
}

void _mpc_lowcomm_ib_ibuf_prepare(sctk_ib_qp_t *remote,
                                  _mpc_lowcomm_ib_ibuf_t *ibuf,
                                  size_t size)
{
	if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RDMA_CHANNEL)
	{
		const _mpc_lowcomm_ib_ibuf_region_t *region = IBUF_RDMA_GET_REGION(remote, REGION_SEND);

		/* Initialization of the buffer */
		_mpc_lowcomm_ib_ibuf_write_init(ibuf,
		                  IBUF_RDMA_GET_BASE_FLAG(ibuf),           /* Local addr */
		                  region->mmu_entry->mr->lkey,             /* lkey */
		                  IBUF_RDMA_GET_REMOTE_ADDR(remote, ibuf), /* Remote addr */
		                  remote->rdma.pool->rkey[REGION_RECV],    /* rkey */
		                  size + IBUF_RDMA_GET_SIZE,               /* size */
		                  IBV_SEND_SIGNALED, IBUF_RELEASE);        /* imm_data: index of the ibuf in the region */

		/* Move tail flag */
		_mpc_lowcomm_ib_ibuf_rdma_set_tail_flag(ibuf, size);
	}
	else
	{
		ib_assume(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RC_SR_CHANNEL);
		__send_inline_init(ibuf, size);
	}
}
