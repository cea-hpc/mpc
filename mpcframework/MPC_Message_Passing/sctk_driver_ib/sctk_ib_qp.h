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
#ifndef __SCTK__IB_QP_H_
#define __SCTK__IB_QP_H_

#include "sctk_ib_config.h"
#include "sctk_ib_buffered.h"
#include "sctk_spinlock.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_device.h"


#define ACK_UNSET   111
#define ACK_OK      222
#define ACK_CANCEL  333
typedef struct sctk_ibuf_rdma_s
{
	sctk_spinlock_t lock;			/**< Lock for allocating pool */
	char dummy1[64];
	sctk_spinlock_t polling_lock;
	char dummy2[64];
	struct sctk_ibuf_rdma_pool_s *pool;
	OPA_int_t state_rtr; 			/**< If remote is RTR. Type: sctk_endpoint_state_t */
	OPA_int_t state_rts;			/**< If remote is RTS. Type: sctk_endpoint_state_t */
	sctk_spinlock_t pending_data_lock;
	OPA_int_t       resizing_nb;			/**< Number of resizing */
	OPA_int_t       cancel_nb; 			/**< Number of Connection cancels */
	char dummy3[64];
	int max_pending_data;			/**< Max number of pending requests.
						* We this, we can get an approximation of the number
						* of slots to create for the RDMA */
	int previous_max_pending_data;  		/**< If the process is the initiator of the request */
	char is_initiator;
	/* Counters */
	OPA_int_t miss_nb;    			 /**< Number of RDMA miss */
	OPA_int_t hits_nb;     			 /**< Number of RDMA hits */
	sctk_spinlock_t flushing_lock;   		/**< Lock while flushing */
	char dummy[64];
	sctk_spinlock_t stats_lock;
	size_t messages_size;			/**< Cumulative sum of msg sizes */
	size_t messages_nb;				/**< Number of messages exchanged */
	double creation_timestamp;
} sctk_ibuf_rdma_t;

#define IBV_SR_SAMPLES 1000
/* Structure which stores some information about SR protocol
 * TODO: ideally, we should move some variables from ib_qp_s to here... */
typedef struct sctk_ibuf_sr_s
{
	/* Empty */
} sctk_ibuf_sr_t;

/*Structure associated to a remote QP */
typedef struct sctk_ib_qp_s
{
	struct ibv_qp           *qp;        /**< queue pair */
	sctk_uint32_t           rq_psn;     /**< starting receive packet sequence number
							(should match remote QP's sq_psn) */
	sctk_uint32_t           psn;        /**< packet sequence number */
	sctk_uint32_t           dest_qp_num;/**< destination qp number */
	int                     rank;       /**< Process rank associated to the QP */
	OPA_int_t               state; 	    /**< QP state */
	OPA_int_t               is_rtr;     /**< If QP in Ready-to-Receive mode*/
	sctk_spinlock_t         lock_rtr;
	OPA_int_t               is_rts;     /**< If QP in Ready-to-Send mode */
	sctk_spinlock_t         lock_rts;
	unsigned int            free_nb;    /**< Number of pending entries free in QP */
	sctk_spinlock_t         post_lock;  /**< Lock when posting an element */
	OPA_int_t               pending_data; 	/**< Number of pending requests */
	sctk_endpoint_t      * endpoint; /** Routes */
	/* For linked-list */
	struct sctk_ib_qp_s *prev;
	struct sctk_ib_qp_s *next;
	/* ---------------- */
	sctk_spinlock_t lock_send;		/**< Lock for sending messages */
	char dummy [64];
	sctk_spinlock_t flushing_lock; 	/**< Lock for flushing messages */
	int is_flushing_initiator;
	OPA_int_t deco_canceled;	          /**< If a QP deconnexion should be canceled */
	int local_ack;			/**< ACK for the local peers */
	int remote_ack;			/**< ACK for the remote peers */
	int unsignaled_counter;		/**< The number of unsignaled messages sent.
					 * \warning the number of unsignaled messages cannot
					 * excess the max number of send entries in  QP */
	struct sctk_ib_buffered_table_s ib_buffered; /**< List of pending buffered messages */
	struct sctk_ibuf_rdma_s rdma; 	     /**< Structure for ibuf rdma */
	struct sctk_ibuf_sr_s sr;
	struct
	{
		int nb;
		int size_ibufs;
	} od_request; 			/**< Structs for requests */
	int ondemand;			/**< Is remote dynamically created ? */
	int R;				/**< Bit for clock algorithm */
} sctk_ib_qp_t;

void sctk_ib_qp_key_create_value ( char *msg, size_t size, sctk_ib_cm_qp_connection_t *keys );
void sctk_ib_qp_key_fill ( sctk_ib_cm_qp_connection_t *keys,
                           sctk_ib_qp_t *remote,
                           sctk_uint16_t lid,
                           sctk_uint32_t qp_num,
                           sctk_uint32_t psn );

void sctk_ib_qp_key_create_key ( char *msg, size_t size, int rail, int src, int dest );
sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert ( char *msg );

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

char *sctk_ib_cq_print_status ( enum ibv_wc_status status );

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert ( char *msg );

void sctk_ib_qp_keys_send ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote );

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_recv ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, int dest_process );

sctk_ib_qp_t *sctk_ib_qp_new();
void sctk_ib_qp_destroy(sctk_ib_qp_t *remote);

struct ibv_qp *sctk_ib_qp_init ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, struct ibv_qp_init_attr *attr, int rank );
void sctk_ib_qp_free_all(struct sctk_ib_rail_info_s* rail_ib);

struct ibv_qp_init_attr sctk_ib_qp_init_attr ( struct sctk_ib_rail_info_s *rail_ib );

struct ibv_qp_attr sctk_ib_qp_state_init_attr ( struct sctk_ib_rail_info_s *rail_ib,  int *flags );

struct ibv_qp_attr sctk_ib_qp_state_rtr_attr ( struct sctk_ib_rail_info_s *rail_ib,  sctk_ib_cm_qp_connection_t *keys, int *flags );

struct ibv_qp_attr sctk_ib_qp_state_rts_attr ( struct sctk_ib_rail_info_s *rail_ib,  sctk_uint32_t psn, int *flags );

void sctk_ib_qp_modify ( sctk_ib_qp_t *remote, struct ibv_qp_attr *attr, int flags );

void sctk_ib_qp_allocate_init ( struct sctk_ib_rail_info_s *rail_ib, int rank, sctk_ib_qp_t *remote, int ondemand, sctk_endpoint_t *route );

void sctk_ib_qp_allocate_rtr ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ib_cm_qp_connection_t *keys );

void sctk_ib_qp_allocate_rts ( struct sctk_ib_rail_info_s *rail_ib,  sctk_ib_qp_t *remote );

void sctk_ib_qp_allocate_reset ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote );

struct ibv_srq *sctk_ib_srq_init ( struct sctk_ib_rail_info_s *rail_ib,  struct ibv_srq_init_attr *attr );
void sctk_ib_srq_free(sctk_ib_rail_info_t *rail_ib);

struct ibv_srq_init_attr sctk_ib_srq_init_attr ( struct sctk_ib_rail_info_s *rail_ib );

int sctk_ib_qp_send_ibuf ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf );

void sctk_ib_qp_release_entry ( struct sctk_ib_rail_info_s *rail_ib,  sctk_ib_qp_t *remote );

int sctk_ib_qp_get_cap_flags ( struct sctk_ib_rail_info_s *rail_ib );

/* Amount of data pending */
__UNUSED__ static int sctk_ib_qp_fetch_and_add_pending_data ( sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
	const int size = ( int ) ibuf->desc.sg_entry.length;
	return OPA_fetch_and_add_int ( &remote->pending_data, size );
}

__UNUSED__ static int sctk_ib_qp_fetch_and_sub_pending_data ( sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
	const int size = - ( ( int ) ibuf->desc.sg_entry.length );
	return OPA_fetch_and_add_int ( &remote->pending_data, size );
}

__UNUSED__ static void sctk_ib_qp_add_pending_data ( sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
	const size_t size = ibuf->desc.sg_entry.length;
	OPA_add_int ( &remote->pending_data, size );
}

__UNUSED__ static int sctk_ib_qp_get_pending_data ( sctk_ib_qp_t *remote )
{
	return OPA_load_int ( &remote->pending_data );
}
__UNUSED__ static void sctk_ib_qp_set_pending_data ( sctk_ib_qp_t *remote, int i )
{
	OPA_store_int ( &remote->pending_data, i );
}

/* Flush ACK */
__UNUSED__ static int sctk_ib_qp_get_local_flush_ack ( sctk_ib_qp_t *remote )
{
	return remote->local_ack;
}
__UNUSED__ static void sctk_ib_qp_set_local_flush_ack ( sctk_ib_qp_t *remote, int i )
{
	remote->local_ack = i;
}
__UNUSED__ static int sctk_ib_qp_get_remote_flush_ack ( sctk_ib_qp_t *remote )
{
	return remote->remote_ack;
}
__UNUSED__ static void sctk_ib_qp_set_remote_flush_ack ( sctk_ib_qp_t *remote, int i )
{
	remote->remote_ack = i;
}


/* Flush cancel */
__UNUSED__ static int sctk_ib_qp_get_deco_canceled ( sctk_ib_qp_t *remote )
{
	return OPA_load_int ( &remote->deco_canceled );
}

__UNUSED__ static void sctk_ib_qp_set_deco_canceled ( sctk_ib_qp_t *remote, int i )
{
	OPA_store_int ( &remote->deco_canceled, i );
}

int sctk_ib_srq_get_max_srq_wr ( struct sctk_ib_rail_info_s *rail_ib );

/*-----------------------------------------------------------
 *  Change the state of a QP
 *----------------------------------------------------------*/
__UNUSED__ static inline void sctk_ib_qp_allocate_set_rtr ( sctk_ib_qp_t *remote, int enabled )
{
	OPA_store_int ( &remote->is_rtr, enabled );
}

__UNUSED__ static inline void sctk_ib_qp_allocate_set_rts ( sctk_ib_qp_t *remote, int enabled )
{
	OPA_store_int ( &remote->is_rts, enabled );
}

__UNUSED__ static inline int sctk_ib_qp_allocate_get_rtr ( sctk_ib_qp_t *remote )
{
	return ( int ) OPA_load_int ( &remote->is_rtr );
}
__UNUSED__ static inline int sctk_ib_qp_allocate_get_rts ( sctk_ib_qp_t *remote )
{
	return ( int ) OPA_load_int ( &remote->is_rts );
}

void sctk_ib_qp_select_victim ( struct sctk_ib_rail_info_s *rail_ib );
void sctk_ib_qp_deco_victim ( struct sctk_ib_rail_info_s *rail_ib, sctk_endpoint_t *endpoint );

int sctk_ib_qp_check_flush ( struct sctk_ib_rail_info_s *rail_ib,  sctk_ib_qp_t *remote );

void sctk_ib_qp_try_flush ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote );

/*-----------------------------------------------------------
 *  QP HT
 *----------------------------------------------------------*/
sctk_ib_qp_t  *sctk_ib_qp_ht_find ( struct sctk_ib_rail_info_s *rail_ib, int key );

#endif
#endif
