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

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__INFINIBAND_IBUFS_H_
#define __SCTK__INFINIBAND_IBUFS_H_

#include "infiniband/verbs.h"
#include "sctk_stdint.h"

enum sctk_ib_cq_type_t
{
    SCTK_IB_SEND_CQ,
    SCTK_IB_RECV_CQ
};

#include "sctk_spinlock.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_debug.h"

struct sctk_rail_info_s;
struct sctk_ib_rail_info_s;
struct sctk_ib_qp_s;
struct sctk_ibuf_rdma_region_s;


/**
   Description: NUMA aware buffers poll interface.
 *  Buffers are stored together in regions. Buffers are
 *  deallocated region per region.
 *  TODO: Use HWLOC for detecting which core is the closest to the
 *  IB card.
 */


/*-----------------------------------------------------------
 *  STRUCTURES
 *----------------------------------------------------------*/
typedef struct sctk_ibuf_header_s
{
	/* Poison */
	int poison;
	/* Protocol used */
	int piggyback;
	sctk_ib_protocol_t protocol;
	int src_task;
	int dest_task;
	int low_memory_mode;
}  __attribute__ ( ( aligned ( 16 ) ) ) sctk_ibuf_header_t;

#define IBUF_GET_HEADER(buffer) ((sctk_ibuf_header_t*) buffer)
#define IBUF_GET_HEADER_SIZE (sizeof(sctk_ibuf_header_t))
#define IBUF_GET_PROTOCOL(buffer) (IBUF_GET_HEADER(buffer)->protocol)
#define IBUF_SET_PROTOCOL(buffer,x) (IBUF_GET_HEADER(buffer)->protocol=x)
/* Get the piggyback of the eager message */
#define IBUF_GET_EAGER_PIGGYBACK(buffer) (IBUF_GET_HEADER(buffer)->piggyback)


#define IBUF_SET_DEST_TASK(buffer,x) (IBUF_GET_HEADER(buffer)->dest_task = x)
#define IBUF_GET_DEST_TASK(buffer) (IBUF_GET_HEADER(buffer)->dest_task)
#define IBUF_SET_SRC_TASK(buffer,x) (IBUF_GET_HEADER(buffer)->src_task  = x)
#define IBUF_GET_SRC_TASK(buffer) (IBUF_GET_HEADER(buffer)->src_task)
#define IBUF_GET_CHANNEL(ibuf) (ibuf->region->channel)

#define IMM_DATA_NULL ~0
#define IMM_DATA_RDMA_PIGGYBACK (0x1 << 31)
#define IMM_DATA_RDMA_MSG       (0x1 << 30)

/* Release the buffer after freeing */
#define IBUF_RELEASE (1<<0)
/* Do not release the buffer after freeing */
#define IBUF_DO_NOT_RELEASE (1<<1)

/* Only for debugging */
#define IBUF_POISON 213819238
#define IBUF_SET_POISON(buffer) (IBUF_GET_HEADER(buffer)->poison = IBUF_POISON)
#define IBUF_GET_POISON(buffer) (IBUF_GET_HEADER(buffer)->poison)
#define IBUF_CHECK_POISON(buffer) do {\
  if (IBUF_GET_HEADER(buffer)->poison != IBUF_POISON) { \
    sctk_error("Wrong header received from buffer %p (got=%d expected=%d %p)", buffer, IBUF_GET_HEADER(buffer)->poison, IBUF_POISON, &IBUF_GET_HEADER(buffer)->poison); \
    assume(0); \
  }} while(0)


typedef struct sctk_ibuf_desc_s
{
	union
	{
		struct ibv_recv_wr recv;
		struct ibv_send_wr send;
	} wr;

	union
	{
		struct ibv_recv_wr *recv;
		struct ibv_send_wr *send;
	} bad_wr;

	struct ibv_sge sg_entry;

	struct sctk_net_ibv_ibuf_s *next; /**< ibufs can be chained together. */
} sctk_ibuf_desc_t;

/** NUMA aware pool of buffers */
typedef struct sctk_ibuf_numa_s
{
	char is_srq_pool;  							/**< if this is a SRQ pool */
	struct sctk_ibuf_region_s  *regions;			/**< DL list of regions */
	struct sctk_ibuf_s         *free_entry;		/**< flag to the first free header */
	struct sctk_ibuf_s         *free_srq_cache;	/**< Free SRQ buffers */
	char padd3[64];
	sctk_spinlock_t lock;							/**< lock when moving pointers */
	char padd1[64];
	sctk_spinlock_t srq_cache_lock;
	unsigned int nb;								  /**< Number of buffers created total */
	OPA_int_t free_nb;							  /**< Number of buffers free */
	OPA_int_t free_srq_nb;						  /**< Number of SRQ buffers free */
	int free_srq_cache_nb;						  /**< Number of SRQ buffers free in cache */
	
	struct sctk_ib_topology_numa_node_s *numa_node; /**< Pointer to the topological NUMA node structure */


	/* MMU is now at a node level. We got many issues while
	 * sharing the MMU accross very large NUMA nodes (>=128 cores) */
	/* FIXME: create a NUMA interface */
	char padd2[64];
} sctk_ibuf_numa_t __attribute__ ( ( __aligned__ ( 64 ) ) );

/** Channel where the region has been allocated */
enum sctk_ibuf_channel
{
    RC_SR_CHANNEL = 1 << 0,
    RDMA_CHANNEL  = 1 << 1,
    SEND_CHANNEL  = 1 << 2,
    RECV_CHANNEL  = 1 << 3,
};

__UNUSED__ static void sctk_ibuf_channel_print ( char *str, enum sctk_ibuf_channel channel );

static void sctk_ibuf_channel_print ( char *str, enum sctk_ibuf_channel channel )
{
	str[0] = ( channel & RC_SR_CHANNEL ) == RC_SR_CHANNEL ? '1' : '0';
	str[1] = ( channel & RDMA_CHANNEL )  == RDMA_CHANNEL ? '1' : '0';
	str[2] = ( channel & SEND_CHANNEL )  == SEND_CHANNEL ? '1' : '0';
	str[3] = ( channel & RECV_CHANNEL )  == RECV_CHANNEL ? '1' : '0';
	str[4] = '\0';
}

/** Region of buffers where several buffers are allocated */
typedef struct sctk_ibuf_region_s
{
	void *buffer_addr;									/**< Address of the buffer */
	struct sctk_ibuf_region_s *next;
	struct sctk_ibuf_region_s *prev;
	sctk_uint32_t nb;									/**< Number of buffer for the region */
	int size_ibufs;										/**< Size of the buffers */
	uint32_t nb_previous;								/**< Number of buffer for the region (previous)*/
	int size_ibufs_previous;							/**< Size of the buffers (previous)*/
	size_t allocated_size;								/**< Memory allocated for the region */
	struct sctk_ib_rail_info_s *rail;					/**< A region is associated to a rail */
	sctk_ib_mmu_entry_t *mmu_entry;				/**< MMU entry */
	sctk_uint32_t polled_nb;							/**< Number of messages polled */
	struct sctk_ibuf_s *list;							/** List of buffers */
	struct sctk_ibuf_s *ibuf;							/**< Pointer to the addr where ibufs are  allocated */
	enum sctk_ibuf_channel channel;						/**< Channel where the region has been allocated */
	/* Specific to SR */
	struct sctk_ibuf_numa_s *node;
	/*** Specific to RDMA ***/
	/* Pointers to 'head' and 'tail' pointers */
	struct sctk_ibuf_s *head;
	struct sctk_ibuf_s *tail;
	struct sctk_ib_qp_s *remote;
	/* Locks */
	sctk_spinlock_t lock;
	/* For clock algorithm */
	int R_bit;
} sctk_ibuf_region_t;

/** Poll of ibufs */
typedef struct sctk_ibuf_pool_s
{
	/* Lock to authorize only 1 task to post
	* new buffers in SRQ */
	sctk_spinlock_t post_srq_lock;
	struct sctk_ibuf_numa_s *node_srq_buffers;

	struct sctk_ibuf_numa_s *default_node;
} sctk_ibuf_pool_t;

/** type of an ibuf */
enum sctk_ibuf_status
{
    BUSY_FLAG             = 11,
    FREE_FLAG             = 22,
    RDMA_READ_IBUF_FLAG   = 33,
    RDMA_WRITE_IBUF_FLAG  = 44,
    RDMA_FETCH_AND_OP_IBUF_FLAG  = 45,
    RDMA_CAS_IBUF_FLAG  = 46,
    NORMAL_IBUF_FLAG      = 55,
    SEND_IBUF_FLAG        = 66,
    SEND_INLINE_IBUF_FLAG = 77,
    RECV_IBUF_FLAG        = 88,
    BARRIER_IBUF_FLAG     = 99,
    EAGER_RDMA_POLLED     = 1010,
    RDMA_WRITE_INLINE_IBUF_FLAG  = 1111,
};

/** Describes an ibuf */
typedef struct sctk_ibuf_s
{
	/* XXX: desc must be first in the structure
	* for data alignment troubles */
	struct sctk_ibuf_desc_s desc;
	struct sctk_ibuf_region_s *region;
	unsigned int index;
	unsigned char *buffer;				/**< pointer to the buffer */
	size_t size;						/**< size of the buffer */
	enum sctk_ibuf_status flag;			/**< status of the buffer */
	/* the following infos aren't transmitted by the network */
	struct sctk_ib_qp_s    *remote;
	void *supp_ptr;
	char in_srq;						/**< If the buffer is in a shaed receive queue */
	char to_release;
	struct ibv_wc wc;					/**< We store the wc of the ibuf */
	/* Linked list used for CP */
	struct sctk_ibuf_s *next;
	struct sctk_ibuf_s *prev;
	int *poison_flag_head;				/**< Poison flag */
	volatile int volatile *head_flag;	/**< Head flag */
	size_t *size_flag;					/**< Size flag */
	int previous_flag;					/**< Previous flag for RDMA */
	double polled_timestamp;			/**< Timestamp when the ibuf has been polled from the CQ */
	struct sctk_rail_info_s *rail;	/**< We save the rail where the message has been pooled*/
	enum sctk_ib_cq_type_t cq;
	uint32_t send_imm_data;
} sctk_ibuf_t;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ibuf_pool_init ( struct sctk_ib_rail_info_s *rail );
void sctk_ibuf_pool_free ( struct sctk_ib_rail_info_s *rail );

sctk_ibuf_t *sctk_ibuf_pick_send_tst ( struct sctk_ib_rail_info_s *rail_ib, struct sctk_ib_qp_s *remote, size_t *size );

sctk_ibuf_t *sctk_ibuf_pick_send ( struct sctk_ib_rail_info_s *rail_ib, struct sctk_ib_qp_s *remote, size_t *size );

sctk_ibuf_t *sctk_ibuf_pick_send_sr ( struct sctk_ib_rail_info_s *rail_ib );

int sctk_ibuf_srq_check_and_post ( struct sctk_ib_rail_info_s *rail_ib );

void sctk_ibuf_release_from_srq ( struct sctk_ib_rail_info_s *rail_ib, sctk_ibuf_t *ibuf );

void sctk_ibuf_print ( sctk_ibuf_t *ibuf, char *desc );

void *sctk_ibuf_get_buffer ( sctk_ibuf_t *ibuf );


/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_ibuf_recv_init ( sctk_ibuf_t *ibuf );

void sctk_ibuf_rdma_recv_init ( sctk_ibuf_t *ibuf, void *local_address, sctk_uint32_t lkey );

void sctk_ibuf_barrier_send_init ( sctk_ibuf_t *ibuf,
                                   void *local_address,
                                   sctk_uint32_t lkey,
                                   void *remote_address,
                                   sctk_uint32_t rkey,
                                   int len );

void sctk_ibuf_send_init ( sctk_ibuf_t *ibuf, size_t size );

int sctk_ibuf_send_inline_init ( sctk_ibuf_t *ibuf, size_t size );

int sctk_ibuf_rdma_write_with_imm_init ( sctk_ibuf_t *ibuf,
                                         void *local_address,
                                         sctk_uint32_t lkey,
                                         void *remote_address,
                                         sctk_uint32_t rkey,
                                         unsigned int len,
                                         char to_release,
                                         sctk_uint32_t imm_data );

int sctk_ibuf_rdma_write_init ( sctk_ibuf_t *ibuf,
                                void *local_address,
                                sctk_uint32_t lkey,
                                void *remote_address,
                                sctk_uint32_t rkey,
                                unsigned int len,
                                int send_flags,
                                char to_release );

void sctk_ibuf_rdma_read_init ( sctk_ibuf_t *ibuf,
                                void *local_address,
                                sctk_uint32_t lkey,
                                void *remote_address,
                                sctk_uint32_t rkey,
                                int len );

void sctk_ibuf_rdma_fetch_and_add_init( sctk_ibuf_t *ibuf,
										void *fetch_addr,
										sctk_uint32_t lkey,
										void *remote_address,
										sctk_uint32_t rkey,
										sctk_uint64_t add );

void sctk_ibuf_rdma_CAS_init( sctk_ibuf_t *ibuf,
							  void *  res_addr,
							  sctk_uint32_t local_key,
							  void *remote_address,
							  sctk_uint32_t rkey,
							  sctk_uint64_t comp,
							  sctk_uint64_t new );



void sctk_ibuf_release ( struct sctk_ib_rail_info_s *rail_ib,
                         sctk_ibuf_t *ibuf,
                         int decr_free_srq_nb );

void sctk_ibuf_prepare ( struct sctk_ib_qp_s *remote,
                         sctk_ibuf_t *ibuf,
                         size_t size );

void sctk_ibuf_init_task ( int rank, int vp );

void sctk_ibuf_pool_set_node_srq_buffers ( struct sctk_ib_rail_info_s *rail_ib,
                                           sctk_ibuf_numa_t *node );

void sctk_ibuf_init_numa_node ( struct sctk_ib_rail_info_s *rail_ib,
                                struct sctk_ibuf_numa_s *node,
                                int nb_ibufs,
                                char is_initial_allocation );
void sctk_ibuf_free_numa_node( struct sctk_ibuf_numa_s *node);


#endif
#endif
