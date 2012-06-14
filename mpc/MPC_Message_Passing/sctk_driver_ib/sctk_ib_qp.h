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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
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

#include <infiniband/verbs.h>
#include <inttypes.h>

struct sctk_ib_qp_s;
/* Structure related to ondemand
 * connexions */
typedef struct sctk_ib_qp_ondemand_s {
  struct sctk_ib_qp_s *qp_list;
  /* 'Hand' of the clock */
  struct sctk_ib_qp_s *qp_list_ptr;
  sctk_spinlock_t lock;
} sctk_ib_qp_ondemand_t;

/* Structure associated to a device */
/* XXX: Put it in a file called sctk_ib_device.c */
typedef struct sctk_ib_device_s
{
  /* Devices */
  struct ibv_device        **dev_list;
  /* Number of devices */
  int                     dev_nb;
  /* Selected device */
  struct ibv_device       *dev;
  /* Selected device index */
  int                     dev_index;
  struct ibv_context      *context;  /* context */
  /* Attributs of the device */
  struct ibv_device_attr  dev_attr;
  /* Port attributs */
  struct ibv_port_attr    port_attr;
  /* XXX: do not add other fields here or the code segfaults.
   * Maybe a restriction of IB in the memory alignement */
  /* ID of the device */
  unsigned int id;
  struct ibv_pd           *pd;       /* protection domain */
  struct ibv_srq          *srq;      /* shared received quue */
  struct ibv_cq           *send_cq;  /* outgoing completion queues */
  struct ibv_cq           *recv_cq;  /* incoming completion queues */

  struct sctk_ib_qp_ondemand_s ondemand;

  /* Link rate & data rate*/
  char link_rate[64];
  int link_width;
  int data_rate;

  /* For eager RDMA channel */
  int eager_rdma_connections;

} sctk_ib_device_t;

#define ACK_UNSET   111
#define ACK_OK      222
#define ACK_CANCEL  333
typedef struct sctk_ibuf_rdma_s
{
  /* Lock for allocating pool */
  sctk_spinlock_t lock;
  char dummy[64];
  sctk_spinlock_t polling_lock;

  struct sctk_ibuf_rdma_pool_s *pool;

  /* If remote is RTR. Type: sctk_route_state_t */
  OPA_int_t state_rtr;
  /* If remote is RTS. Type: sctk_route_state_t */
  OPA_int_t state_rts;

  /* Mean size for rdma entries */
  float           mean_size;
  int             mean_iter;
  sctk_spinlock_t mean_size_lock;
  /* Max number of pending requests.
   * We this, we can get an approximation of the number
   * of slots to create for the RDMA */
  int max_pending_requests;
  /* If the process is the initiator of the request */
  char is_initiator;
  /* Counters */
  OPA_int_t miss_nb;     /* Number of RDMA miss */
  OPA_int_t hits_nb;     /* Number of RDMA hits */
  sctk_spinlock_t flushing_lock; /* Lock while flushing */
} sctk_ibuf_rdma_t;

/*Structure associated to a remote QP */
typedef struct sctk_ib_qp_s
{
  struct ibv_qp           *qp;       /* queue pair */
  uint32_t                rq_psn;    /* starting receive packet sequence number
                                        (should match remote QP's sq_psn) */
  uint32_t                psn;       /* packet sequence number */
  uint32_t                dest_qp_num;/* destination qp number */
  int                     rank; /* Process rank associated to the QP */
  /* QP state */
  OPA_int_t               state;
  /* If QP in Ready-to-Receive mode*/
  OPA_int_t               is_rtr;
  sctk_spinlock_t         lock_rtr;
  /* If QP in Ready-to-Send mode */
  OPA_int_t               is_rts;
  sctk_spinlock_t         lock_rts;

  /* Number of pending entries free in QP */
  unsigned int            free_nb;
  /* Lock when posting an element */
  sctk_spinlock_t         post_lock;
  /* Number of pending requests */
  OPA_int_t               pending_requests_nb;

  /* Route */
  sctk_route_table_t      *route_table;

  /* For linked-list */
  struct sctk_ib_qp_s *prev;
  struct sctk_ib_qp_s *next;

  /* Lock for sending messages */
  sctk_spin_rwlock_t lock_send;
  /* If a QP deconnexion should be canceled */
  OPA_int_t deco_canceled;
  /* ACK for the local and the remote peers */
  int local_ack;
  int remote_ack;
  int deco_lock;

  /* List of pending buffered messages */
  struct sctk_ib_buffered_table_s ib_buffered;

  /* Structure for ibuf rdma */
  struct sctk_ibuf_rdma_s rdma;
  /* Structs for requests */
  struct {
    int nb;
    int size_ibufs;
  } od_request;

  /* Is remote dynamically created ? */
  int ondemand;
  /* Bit for clock algorithm */
  int R;
} sctk_ib_qp_t;

void sctk_ib_qp_key_create_value(char *msg, size_t size, sctk_ib_cm_qp_connection_t* keys);
void sctk_ib_qp_key_fill(sctk_ib_cm_qp_connection_t* keys, sctk_ib_qp_t *remote,
    uint16_t lid, uint32_t qp_num, uint32_t psn);
void sctk_ib_qp_key_create_key(char *msg, size_t size, int rail, int src, int dest);
sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert( char* msg);

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ib_device_t *sctk_ib_device_init(struct sctk_ib_rail_info_s* rail_ib);
sctk_ib_device_t *sctk_ib_device_open(struct sctk_ib_rail_info_s* rail_ib, int rail_nb);

struct ibv_pd* sctk_ib_pd_init(sctk_ib_device_t *device);

struct ibv_cq* sctk_ib_cq_init(sctk_ib_device_t* device,
    sctk_ib_config_t *config);

char* sctk_ib_cq_print_status (enum ibv_wc_status status);

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert( char* msg);

void sctk_ib_qp_keys_send(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote);

sctk_ib_cm_qp_connection_t
sctk_ib_qp_keys_recv(
    struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, int dest_process);

sctk_ib_qp_t* sctk_ib_qp_new();

  struct ibv_qp*
sctk_ib_qp_init(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote, struct ibv_qp_init_attr* attr, int rank);

struct ibv_qp_init_attr
sctk_ib_qp_init_attr(struct sctk_ib_rail_info_s* rail_ib);

struct ibv_qp_attr
sctk_ib_qp_state_init_attr(struct sctk_ib_rail_info_s* rail_ib,
    int *flags);

struct ibv_qp_attr
sctk_ib_qp_state_rtr_attr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_cm_qp_connection_t* keys, int *flags);

struct ibv_qp_attr
sctk_ib_qp_state_rts_attr(struct sctk_ib_rail_info_s* rail_ib,
    uint32_t psn, int *flags);

void
sctk_ib_qp_modify( sctk_ib_qp_t* remote, struct ibv_qp_attr* attr, int flags);

void sctk_ib_qp_allocate_init(struct sctk_ib_rail_info_s* rail_ib,
    int rank, sctk_ib_qp_t* remote, int ondemand, sctk_route_table_t* route);

void sctk_ib_qp_allocate_rtr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote,sctk_ib_cm_qp_connection_t* keys);

void sctk_ib_qp_allocate_rts(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);

  void
sctk_ib_qp_allocate_reset(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);

  struct ibv_srq*
sctk_ib_srq_init(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_srq_init_attr* attr);

  struct ibv_srq_init_attr
sctk_ib_srq_init_attr(struct sctk_ib_rail_info_s* rail_ib);

void
sctk_ib_qp_send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf, int is_control_message);

void sctk_ib_qp_release_entry(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);

int sctk_ib_qp_get_cap_flags(struct sctk_ib_rail_info_s* rail_ib);

/* Number of pending requests */
__UNUSED__ static void
sctk_ib_qp_inc_requests_nb(sctk_ib_qp_t *remote) {
  OPA_incr_int(&remote->pending_requests_nb);
}
__UNUSED__ static void
sctk_ib_qp_decr_requests_nb(sctk_ib_qp_t *remote) {
  OPA_decr_int(&remote->pending_requests_nb);
}
__UNUSED__ static int
sctk_ib_qp_get_requests_nb(sctk_ib_qp_t *remote) {
  return OPA_load_int(&remote->pending_requests_nb);
}
__UNUSED__ static void
sctk_ib_qp_set_requests_nb(sctk_ib_qp_t *remote, int i) {
  OPA_store_int(&remote->pending_requests_nb, i);
}

/* Flush ACK */
__UNUSED__ static int
sctk_ib_qp_get_local_flush_ack(sctk_ib_qp_t *remote) {
  return remote->local_ack;
}
__UNUSED__ static void
sctk_ib_qp_set_local_flush_ack(sctk_ib_qp_t *remote, int i) {
  remote->local_ack = i;
}
__UNUSED__ static int
sctk_ib_qp_get_remote_flush_ack(sctk_ib_qp_t *remote) {
  return remote->remote_ack;
}
__UNUSED__ static void
sctk_ib_qp_set_remote_flush_ack(sctk_ib_qp_t *remote, int i) {
  remote->remote_ack = i;
}


/* Flush cancel */
__UNUSED__ static int
sctk_ib_qp_get_deco_canceled(sctk_ib_qp_t *remote) {
  return OPA_load_int(&remote->deco_canceled);
}
__UNUSED__ static void
sctk_ib_qp_set_deco_canceled(sctk_ib_qp_t *remote, int i) {
  OPA_store_int(&remote->deco_canceled, i);
}

int
sctk_ib_srq_get_max_srq_wr (struct sctk_ib_rail_info_s* rail_ib);

/*-----------------------------------------------------------
 *  Change the state of a QP
 *----------------------------------------------------------*/
__UNUSED__ static inline void
sctk_ib_qp_allocate_set_rtr(sctk_ib_qp_t *remote, int enabled) {
  OPA_store_int(&remote->is_rtr, enabled);
}
__UNUSED__ static inline void
sctk_ib_qp_allocate_set_rts(sctk_ib_qp_t *remote, int enabled) {
  OPA_store_int(&remote->is_rts, enabled);
}
__UNUSED__ static inline int
sctk_ib_qp_allocate_get_rtr(sctk_ib_qp_t *remote) {
  return (int) OPA_load_int(&remote->is_rtr);
}
__UNUSED__ static inline int
sctk_ib_qp_allocate_get_rts(sctk_ib_qp_t *remote) {
  return (int) OPA_load_int(&remote->is_rts);
}


void sctk_ib_qp_select_victim(struct sctk_ib_rail_info_s* rail_ib);
void sctk_ib_qp_deco_victim(struct sctk_ib_rail_info_s* rail_ib,
    sctk_route_table_t* route_table);


/*-----------------------------------------------------------
 *  QP HT
 *----------------------------------------------------------*/
sctk_ib_qp_t*  sctk_ib_qp_ht_find(struct sctk_ib_rail_info_s* rail_ib, int key);

#endif
#endif
