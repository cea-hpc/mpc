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
#include "sctk_spinlock.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"

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
  /* ID of the device */
  unsigned int id;
//  uint16_t                lid;

  struct ibv_pd           *pd;       /* protection domain */
  struct ibv_srq          *srq;      /* shared received quue */
  struct ibv_cq           *send_cq;  /* outgoing completion queues */
  struct ibv_cq           *recv_cq;  /* incoming completion queues */

  struct sctk_ib_qp_ondemand_s ondemand;
} sctk_ib_device_t;

/* Structure associated to a remote QP */
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

  /* For linked-list */
  struct sctk_ib_qp_s *prev;
  struct sctk_ib_qp_s *next;

  /* Lock for sending messages */
  sctk_spin_rwlock_t lock_send;

  /* Is remote dynamically created ? */
  int ondemand;
  /* Bit for clock algorithm */
  int R;
} sctk_ib_qp_t;

/*-----------------------------------------------------------
 *  Exchanged keys: used to connect QPs
 *----------------------------------------------------------*/
typedef struct
{
  uint16_t lid;
  uint32_t qp_num;
  uint32_t psn;
} sctk_ib_qp_keys_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ib_device_t *sctk_ib_device_init(struct sctk_ib_rail_info_s* rail_ib);
sctk_ib_device_t *sctk_ib_device_open(struct sctk_ib_rail_info_s* rail_ib, int rail_nb);

struct ibv_pd* sctk_ib_pd_init(sctk_ib_device_t *device);

struct ibv_cq* sctk_ib_cq_init(sctk_ib_device_t* device,
    sctk_ib_config_t *config);

char* sctk_ib_cq_print_status (enum ibv_wc_status status);

sctk_ib_qp_keys_t sctk_ib_qp_keys_convert( char* msg);

void sctk_ib_qp_keys_send(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote);

sctk_ib_qp_keys_t
sctk_ib_qp_keys_recv(sctk_ib_qp_t *remote, int dest_process);

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
    sctk_ib_qp_keys_t* keys, int *flags);

struct ibv_qp_attr
sctk_ib_qp_state_rts_attr(struct sctk_ib_rail_info_s* rail_ib,
    uint32_t psn, int *flags);

void
sctk_ib_qp_modify( sctk_ib_qp_t* remote, struct ibv_qp_attr* attr, int flags);

void sctk_ib_qp_allocate_init(struct sctk_ib_rail_info_s* rail_ib,
    int rank, sctk_ib_qp_t* remote, int ondemand);

void sctk_ib_qp_allocate_rtr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote,sctk_ib_qp_keys_t* keys);

void sctk_ib_qp_allocate_rts(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);

  struct ibv_srq*
sctk_ib_srq_init(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_srq_init_attr* attr);

  struct ibv_srq_init_attr
sctk_ib_srq_init_attr(struct sctk_ib_rail_info_s* rail_ib);

void
sctk_ib_qp_send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf);

void sctk_ib_qp_release_entry(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);

int sctk_ib_qp_get_cap_flags(struct sctk_ib_rail_info_s* rail_ib);

static void
sctk_ib_qp_inc_requests_nb(sctk_ib_qp_t *remote) {
  OPA_incr_int(&remote->pending_requests_nb);
}
static void
sctk_ib_qp_decr_requests_nb(sctk_ib_qp_t *remote) {
  OPA_decr_int(&remote->pending_requests_nb);
}
static int
sctk_ib_qp_get_requests_nb(sctk_ib_qp_t *remote) {
  OPA_load_int(&remote->pending_requests_nb);
}
static int
sctk_ib_qp_set_requests_nb(sctk_ib_qp_t *remote) {
  OPA_load_int(&remote->pending_requests_nb);
}

void
sctk_ib_qp_flush(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote);


/*-----------------------------------------------------------
 *  Change the state of a QP
 *----------------------------------------------------------*/
static inline void
sctk_ib_qp_allocate_set_rtr(sctk_ib_qp_t *remote, int enabled) {
  OPA_store_int(&remote->is_rtr, enabled);
}
static inline void
sctk_ib_qp_allocate_set_rts(sctk_ib_qp_t *remote, int enabled) {
  OPA_store_int(&remote->is_rts, enabled);
}
static inline int
sctk_ib_qp_allocate_get_rtr(sctk_ib_qp_t *remote) {
  return (int) OPA_load_int(&remote->is_rtr);
}
static inline int
sctk_ib_qp_allocate_get_rts(sctk_ib_qp_t *remote) {
  return (int) OPA_load_int(&remote->is_rts);
}




#endif
#endif
