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

#ifndef __MPC_LOWCOMM_IB_H_
#define __MPC_LOWCOMM_IB_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "mpc_common_spinlock.h"
#include "uthash.h"
#include "mpc_common_types.h"

/*-----------------------------------------------------------
 *  IB module debugging
 *----------------------------------------------------------*/
/* Uncomment this macro to activate the IB debug */
//#define IB_DEBUG

#ifdef IB_DEBUG
#warning "WARNING !!!! Debug activated !!!! WARNING"
#endif

#ifdef IB_DEBUG
#define ib_assume(x) assume(x)
/* const for debugging IB */
#define DEBUG_IB_MMU
#define DEBUG_IB_BUFS
#else
#define ib_assume(x) (void)(0)
#endif

/*-----------------------------------------------------------
 *----------------------------------------------------------*/

struct sctk_rail_info_s;
struct _mpc_lowcomm_ib_ibuf_poll_s;
struct _mpc_lowcomm_ib_ibuf_s;
struct sctk_ib_mmu_s;
struct _mpc_lowcomm_ib_topology_s;
struct sctk_ib_config_s;
struct sctk_ib_device_s;
struct sctk_ib_qp_s;
struct _mpc_lowcomm_ib_cp_ctx_s;
struct mpc_lowcomm_ptp_message_s;
struct sctk_rail_info_s;
struct mpc_lowcomm_ptp_message_content_to_copy_s;
struct sctk_ib_buffered_entry_s;
struct sctk_ib_qp_ht_s;

typedef struct sctk_ib_rail_info_s
{
	struct _mpc_lowcomm_ib_ibuf_poll_s *pool_buffers;
	/* struct sctk_ib_mmu_s    *mmu; */
	struct _mpc_lowcomm_ib_topology_s *topology;
	struct sctk_runtime_config_struct_net_driver_infiniband *config;
	struct sctk_ib_device_s *device;
	/* Collaborative polling */
	struct _mpc_lowcomm_ib_cp_ctx_s *cp;

	/* HashTable where all remote are stored.
	* qp_num is the key of the HT */
	struct sctk_ib_qp_ht_s         *remotes;
	/* Pointer to the generic rail */
	struct sctk_rail_info_s        *rail;
	/* Rail number among other IB rails */
	int rail_nb;

	/* For Eager messages -> pool of MPC headers */
	struct mpc_lowcomm_ptp_message_s *eager_buffered_ptp_message;
	void* eager_buffered_start_addr;
	mpc_common_spinlock_t eager_lock_buffered_ptp_message;
} sctk_ib_rail_info_t;

typedef struct sctk_ib_route_info_s
{
	struct sctk_ib_qp_s *remote;
} sctk_ib_route_info_t;

/* ib protocol used */
typedef enum sctk_ib_protocol_e
{
    MPC_LOWCOMM_IB_EAGER_PROTOCOL        = 111,
    MPC_LOWCOMM_IB_BUFFERED_PROTOCOL     = 222,
    MPC_LOWCOMM_IB_RDMA_PROTOCOL         = 333,
    MPC_LOWCOMM_IB_NULL_PROTOCOL         = 444,
} _mpc_lowcomm_ib_protocol_t;


/* 2 first bits: MPC_LOWCOMM_IB_RDMA_NOT_SET, MPC_LOWCOMM_IB_RDMA_ZEROCOPY, MPC_LOWCOMM_IB_RDMA_RECOPY */
/* third bit: if MPC_LOWCOMM_IB_RDMA_MATCH */
/* forth bit: if done  */
/* free from 1<<4 */
typedef volatile enum sctk_ib_rdma_status_e
{
	MPC_LOWCOMM_IB_RDMA_NOT_SET = 1,
	MPC_LOWCOMM_IB_RDMA_ZEROCOPY = 2,
	MPC_LOWCOMM_IB_RDMA_RECOPY = 3,
	MPC_LOWCOMM_IB_RDMA_MATCH = 4,
	MPC_LOWCOMM_IB_RDMA_DONE = 8,
	MPC_LOWCOMM_IB_RDMA_ALLOCATION = 1 << 4,
}
sctk_ib_rdma_status_t;

#define MASK_BASE   0x03
#define MASK_MATCH  0x04
#define MASK_DONE   0x08

/* Headers */
typedef struct sctk_ib_header_rdma_s
{
	/* HT */
	UT_hash_handle hh;
	int ht_key;

	size_t requested_size;
	mpc_common_spinlock_t lock;
	struct sctk_rail_info_s *rail;
	struct sctk_rail_info_s *remote_rail;
	struct sctk_ib_qp_s *remote_peer;
	struct mpc_lowcomm_ptp_message_content_to_copy_s *copy_ptr;
	/* For collaborative polling: src and dest of msg */
	int source_task;
	int destination_task;
	/* Local structure */
	struct
	{
		struct mpc_lowcomm_ptp_message_s *msg_header;
		sctk_ib_rdma_status_t status;
		struct sctk_ib_mmu_entry_s *mmu_entry;
		void  *addr;
		size_t size;
		void  *aligned_addr;
		size_t aligned_size;
		/* Timestamp when the request has been sent */
		double req_timestamp;
		double send_rdma_timestamp;
		double send_ack_timestamp;
		/* Local structure ready to be read */
		volatile int ready;
	} local;
	/* Remote structure */
	struct
	{
		/* msg_header of the remote peer */
		struct mpc_lowcomm_ptp_message_s *msg_header;
		void  *addr;
		size_t size;
		uint32_t rkey;
		void  *aligned_addr;
		size_t aligned_size;
	} remote;
} sctk_ib_header_rdma_t;


/* Structure included in msg header */
typedef struct mpc_lowcomm_ib_tail_s
{
	_mpc_lowcomm_ib_protocol_t protocol;

	union
	{
		struct
		{
			/* If msg has been recopied (do not read msg
			* from network buffer */
			int recopied;
			/* Ibuf to release */
			struct _mpc_lowcomm_ib_ibuf_s *ibuf;
		} eager;

		struct
		{
			struct sctk_ib_buffered_entry_s *entry;
			struct sctk_rail_info_s *rail;
			int ready;
		} buffered;
		struct sctk_ib_header_rdma_s rdma;
	};
} sctk_ib_msg_header_t;

/* XXX: Should not be declared here but in CM */
struct sctk_endpoint_s *sctk_ib_create_remote();
void sctk_ib_init_remote ( int dest, struct sctk_rail_info_s *rail, struct sctk_endpoint_s *route_table, int ondemand );

void sctk_ib_add_static_route ( int dest, struct sctk_endpoint_s *tmp, struct sctk_rail_info_s *rail );
void sctk_ib_add_dynamic_route ( int dest, struct sctk_endpoint_s *tmp, struct sctk_rail_info_s *rail );
int sctk_ib_route_dynamic_is_connected ( struct sctk_endpoint_s *tmp );
void sctk_ib_route_dynamic_set_connected ( struct sctk_endpoint_s *tmp, int connected );

/* IB header: generic */
#define IBUF_GET_EAGER_HEADER(buffer) \
(sctk_ib_eager_t*) ((char*) buffer + sizeof(_mpc_lowcomm_ib_ibuf_header_t))
#define IBUF_GET_BUFFERED_HEADER(buffer) \
(sctk_ib_buffered_t*) ((char*) buffer + sizeof(_mpc_lowcomm_ib_ibuf_header_t))
#define IBUF_GET_RDMA_HEADER(buffer) \
(sctk_ib_rdma_t*) ((char*) buffer + sizeof(_mpc_lowcomm_ib_ibuf_header_t))

/* IB payload: generic */
#define IBUF_GET_EAGER_PAYLOAD(buffer) \
(void*) ((char*) IBUF_GET_EAGER_HEADER(buffer) + sizeof(sctk_ib_eager_t))
#define IBUF_GET_BUFFERED_PAYLOAD(buffer) \
(void*) ((char*) IBUF_GET_BUFFERED_HEADER(buffer) + sizeof(sctk_ib_buffered_t))
#define IBUF_GET_RDMA_PAYLOAD(buffer) \
(void*) ((char*) IBUF_GET_RDMA_HEADER(buffer) + sizeof(sctk_ib_rdma_t))

/* MPC header */
#define IBUF_GET_EAGER_MSG_HEADER(buffer) \
(mpc_lowcomm_ptp_message_body_t*) IBUF_GET_EAGER_PAYLOAD(buffer)

/* MPC payload */
#define IBUF_GET_EAGER_MSG_PAYLOAD(buffer) \
(void*) ((char*) IBUF_GET_EAGER_PAYLOAD(buffer) + sizeof(mpc_lowcomm_ptp_message_body_t))
#define IBUF_GET_RDMA_MSG_PAYLOAD(buffer) \
(void*) ((char*) IBUF_GET_RDMA_PAYLOAD(buffer) + sizeof(mpc_lowcomm_ptp_message_body_t))

/* Different types of messages */
#define IBUF_GET_RDMA_REQ(buffer) ((sctk_ib_rdma_req_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_ACK(buffer) ((sctk_ib_rdma_ack_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DONE(buffer) ((sctk_ib_rdma_done_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DATA_READ(buffer) ((sctk_ib_rdma_data_read_t*) IBUF_GET_RDMA_PAYLOAD(buffer))
#define IBUF_GET_RDMA_DATA_WRITE(buffer) ((sctk_ib_rdma_data_write_t*) IBUF_GET_RDMA_PAYLOAD(buffer))

/* Size */
#define IBUF_GET_EAGER_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_eager_t))

#define IBUF_GET_BUFFERED_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_buffered_t))

#define IBUF_GET_RDMA_SIZE (IBUF_GET_HEADER_SIZE + sizeof(sctk_ib_rdma_t))
#define IBUF_GET_RDMA_REQ_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_req_t))
#define IBUF_GET_RDMA_ACK_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_ack_t))
#define IBUF_GET_RDMA_DONE_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_done_t))
#define IBUF_GET_RDMA_DATA_READ_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_data_read_t))
#define IBUF_GET_RDMA_DATA_WRITE_SIZE (IBUF_GET_RDMA_SIZE + sizeof(sctk_ib_rdma_data_write_t))

void sctk_network_free_msg ( struct mpc_lowcomm_ptp_message_s *msg );
/* For getting stats from network usage */

#ifdef __cplusplus
}
#endif
#endif
