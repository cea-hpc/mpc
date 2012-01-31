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
#ifndef __SCTK__INFINIBAND_QP_H_
#define __SCTK__INFINIBAND_QP_H_

#include <stdio.h>
#include <stdlib.h>
#include "sctk_ib_const.h"
#include "sctk_ib_config.h"
#include "sctk_ib_list.h"
#include "sctk_spinlock.h"
#include <infiniband/verbs.h>
#include <inttypes.h>

struct sctk_net_ibv_mmu_s;
struct sctk_net_ibv_ibuf_s;

typedef struct
{
  struct ibv_context      *context;  /* context */
  struct sctk_net_ibv_mmu_s   *mmu;      /* mmu */
  uint16_t                lid;
  uint32_t                max_mr;    /* packet sequence number */
} sctk_net_ibv_qp_rail_t;

typedef struct sctk_net_ibv_qp_local_s
{
  struct ibv_context      *context;  /* context */
  struct ibv_pd           *pd;       /* protection domain */
  struct ibv_srq          *srq;      /* shared received quue */
  struct ibv_cq           *send_cq;  /* outgoing completion queues */
  struct ibv_cq           *recv_cq;  /* incoming completion queues */
} sctk_net_ibv_qp_local_t;

typedef struct sctk_net_ibv_qp_remote_s
{
  int                     rank;      /* rank of the remote process */
  struct ibv_qp           *qp;       /* queue pair */
  uint32_t                rq_psn;    /* starting receive packet sequence number
                                        (should match remote QP's sq_psn) */

  uint32_t                psn;       /* packet sequence number */
  uint32_t                dest_qp_num;/* destination qp number */

  uint8_t                 is_rtr;
  uint8_t                 is_rts;
  uint8_t                 is_connected;
  uint8_t                 is_usuable;

  /* pending wqe. These WQE are queued when all
   * the QP's WQE are busy */
  sctk_thread_mutex_t  send_wqe_lock;
  struct sctk_list_header  pending_send_wqe;

  /* WQE */
  uint32_t                     ibv_got_recv_wqe;
  uint32_t                     ibv_free_recv_wqe;
  uint32_t                     ibv_got_send_wqe;
  uint32_t                     ibv_free_send_wqe;

} sctk_net_ibv_qp_remote_t;

/*-----------------------------------------------------------
 *  Exchange keys
 *----------------------------------------------------------*/

typedef struct
{
  uint16_t lid;
  uint32_t qp_num;
  uint32_t psn;
} sctk_net_ibv_qp_exchange_keys_t;

#include "sctk.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_mmu.h"
#include "sctk_config.h"

static char* sctk_net_ibv_ibuf_print_flag (int flag);
  char *
sctk_net_ibv_cq_print_status (enum ibv_wc_status status);

  /* FUNCTIONS */
static void
sctk_net_ibv_poll_check_wc(struct ibv_wc wc, sctk_net_ibv_allocator_type_t type) {

  sctk_net_ibv_ibuf_header_t* header;
  struct sctk_net_ibv_ibuf_s* ibuf;

  if (wc.status != IBV_WC_SUCCESS)
  {

    ibuf = (struct sctk_net_ibv_ibuf_s*) wc.wr_id;
    assume(ibuf);
    header = (sctk_net_ibv_ibuf_header_t*) (ibuf->buffer);

    sctk_error ("\033[1;31m\nIB - FATAL ERROR FROM PROCESS %d\n"
        "################################\033[0m\n"
        "Work ID is   : %d\n"
        "Status       : %s\n"
        "ERROR Vendor : %d\n"
        "Byte_len     : %d\n"
        "Flag         : %s\n"
        "Ibuf type    : %d\n"
        "Ptp type     : %d\n"
        "Buff_nb      : %d\n"
        "Total_buffs  : %d\n"
        "Dest process : %d\n"
        "\033[1;31m################################\033[0m\n",
        sctk_process_rank,
        wc.wr_id, sctk_net_ibv_cq_print_status(wc.status),
        wc.vendor_err, wc.byte_len, sctk_net_ibv_ibuf_print_flag(ibuf->flag),
        header->ibuf_type, header->ptp_type, header->buff_nb, header->total_buffs, ibuf->dest_process);

    sctk_abort();
  }
}


static sctk_spinlock_t ptp_recv_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_spinlock_t ptp_send_lock = SCTK_SPINLOCK_INITIALIZER;
  static inline int
sctk_net_ibv_cq_poll(struct ibv_cq* cq, int pending_nb, void (*ptr_func)(struct ibv_wc*, int dest), sctk_net_ibv_allocator_type_t type)
{
  struct ibv_wc wc;
  int ne = 0;
  int i;
  int found = 0;

  for (i=0; i < pending_nb; ++i)
  {
      ne = ibv_poll_cq (cq, 1, &wc);
      if (ne)
      {
        //        sctk_spinlock_lock(&ptp_recv_lock);
        sctk_net_ibv_poll_check_wc(wc, type);
        ptr_func(&wc, 0);
        found = 1;
      } else {
        return found;
      }
    }
  return found;
}

  static inline void
sctk_net_ibv_cq_lookup(struct ibv_cq* cq, int nb_pending, void (*ptr_func)(struct ibv_wc*, int dest), int dest, sctk_net_ibv_allocator_type_t type)
{
  struct ibv_wc wc[nb_pending];
  int ne = 0;
  int i;

  ne = ibv_poll_cq (cq, nb_pending, wc);

  for (i = 0; i < ne; ++i)
  {
    sctk_nodebug("%d elements found", ne);

    sctk_net_ibv_poll_check_wc(wc[i], type);

    ptr_func(&wc[i], dest);
  }
}


/*-----------------------------------------------------------
 *  RAIL
 *----------------------------------------------------------*/

void
sctk_net_ibv_qp_rail_init();

sctk_net_ibv_qp_rail_t*
sctk_net_ibv_qp_pick_rail(int rail_nb);

/*-----------------------------------------------------------
 *  NEW/FREE
 *----------------------------------------------------------*/

sctk_net_ibv_qp_local_t*
sctk_net_ibv_qp_new(sctk_net_ibv_qp_rail_t* rail);

void
sctk_net_ibv_qp_free(sctk_net_ibv_qp_local_t* qp);

/*-----------------------------------------------------------
 *  Protection Domain
 *----------------------------------------------------------*/

struct ibv_pd*
sctk_net_ibv_pd_init(sctk_net_ibv_qp_local_t* qp);

/*-----------------------------------------------------------
 *  Queue Pair
 *----------------------------------------------------------*/
/* attrs */
struct ibv_qp_init_attr
sctk_net_ibv_qp_init_attr(struct ibv_cq* send_cq, struct ibv_cq* recv_cq, struct ibv_srq* srq);

struct ibv_qp_attr
sctk_net_ibv_qp_state_init_attr(int *flags);

struct ibv_qp_attr
sctk_net_ibv_qp_state_rtr_attr(sctk_net_ibv_qp_exchange_keys_t* keys, int *flags);

struct ibv_qp_attr
sctk_net_ibv_qp_state_rts_attr( uint32_t psn, int *flags);

/* functions */
void
sctk_net_ibv_qp_modify( sctk_net_ibv_qp_remote_t* remote, struct ibv_qp_attr* attr, int flags);

struct ibv_qp*
sctk_net_ibv_qp_init(sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t* remote, struct ibv_qp_init_attr* attr, int rank);

int sctk_net_ibv_qp_send_post_pending(sctk_net_ibv_qp_remote_t* remote, int need_lock);
/*-----------------------------------------------------------
 *  Completion queue
 *----------------------------------------------------------*/

struct ibv_cq*
sctk_net_ibv_cq_init(sctk_net_ibv_qp_rail_t* rail);


int
sctk_net_ibv_cq_garbage_collector(struct ibv_cq* cq, int nb_pending, void (*ptr_func)(struct ibv_wc*), sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_cq_lookup(struct ibv_cq* cq, int nb_pending, void (*ptr_func)(struct ibv_wc*, int dest), int dest, sctk_net_ibv_allocator_type_t type);

/*-----------------------------------------------------------
 *  Shared Receive queue
 *----------------------------------------------------------*/

struct ibv_srq*
sctk_net_ibv_srq_init( sctk_net_ibv_qp_local_t* qp, struct ibv_srq_init_attr* attr);

struct ibv_srq_init_attr
sctk_net_ibv_srq_init_attr();

/*-----------------------------------------------------------
 *  Exchange keys
 *----------------------------------------------------------*/

sctk_net_ibv_qp_exchange_keys_t
sctk_net_ibv_qp_exchange_convert( char* msg);

void sctk_net_ibv_qp_exchange_send(
    int i,
    sctk_net_ibv_qp_rail_t* rail,
    sctk_net_ibv_qp_remote_t* remote);

sctk_net_ibv_qp_exchange_keys_t
sctk_net_ibv_qp_exchange_recv(int i,
    sctk_net_ibv_qp_local_t* qp, int dest_process);


/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
void
sctk_net_ibv_qp_allocate_init(int rank,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t* remote);

void
sctk_net_ibv_qp_allocate_rtr(
    sctk_net_ibv_qp_remote_t *remote,
    sctk_net_ibv_qp_exchange_keys_t* keys);

void
sctk_net_ibv_qp_allocate_rts(
    sctk_net_ibv_qp_remote_t *remote);


/*-----------------------------------------------------------
 *  WQE
 *----------------------------------------------------------*/
int sctk_net_ibv_qp_send_get_wqe(int dest_process, struct sctk_net_ibv_ibuf_s* ibuf);

void sctk_net_ibv_qp_send_free_wqe(sctk_net_ibv_qp_remote_t* remote );


#endif
#endif
