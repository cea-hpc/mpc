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
#ifndef __SCTK__IB_POLLING_H_
#define __SCTK__IB_POLLING_H_

#include <infiniband/verbs.h>
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "utlist.h"

typedef struct sctk_ib_polling_s{
  int recv_found_own;
  int recv_found_other;

  int send_found_own;
  int send_found_other;
} sctk_ib_polling_t;

#define HOSTNAME 2048
#define POLL_INIT(x) memset((x), 0, sizeof(sctk_ib_polling_t));

__UNUSED__  static inline char *
sctk_ib_polling_print_status (enum ibv_wc_status status)
{
  switch (status) {
    case IBV_WC_SUCCESS:
      return ("IBV_WC_SUCCESS: success");
      break;
    case IBV_WC_LOC_LEN_ERR:
      return ("IBV_WC_LOC_LEN_ERR: local length error");
      break;
    case IBV_WC_LOC_QP_OP_ERR:
      return ("IBV_WC_LOC_QP_OP_ERR: local QP op error");
      break;
    case IBV_WC_LOC_EEC_OP_ERR:
      return ("IBV_WC_LOC_EEC_OP_ERR: local EEC op error");
      break;
    case IBV_WC_LOC_PROT_ERR:
      return ("IBV_WC_LOC_PROT_ERR: local protection error");
      break;
    case IBV_WC_WR_FLUSH_ERR:
      return ("IBV_WC_WR_FLUSH_ERR: write flush error");
      break;
    case IBV_WC_MW_BIND_ERR:
      return ("IBV_WC_MW_BIND_ERR: MW bind error");
      break;
    case IBV_WC_BAD_RESP_ERR:
      return ("IBV_WC_BAD_RESP_ERR: bad response error");
      break;
    case IBV_WC_LOC_ACCESS_ERR:
      return ("IBV_WC_LOC_ACCESS_ERR: local access error");
      break;
    case IBV_WC_REM_INV_REQ_ERR:
      return ("IBV_WC_REM_INV_REQ_ERR: remote invalid request error");
      break;
    case IBV_WC_REM_ACCESS_ERR:
      return ("IBV_WC_REM_ACCESS_ERR: remote access error");
      break;
    case IBV_WC_REM_OP_ERR:
      return ("IBV_WC_REM_OP_ERR: remote op error");
      break;
    case IBV_WC_RETRY_EXC_ERR:
      return ("IBV_WC_RETRY_EXC_ERR: retry exceded error");
      break;
    case IBV_WC_RNR_RETRY_EXC_ERR:
      return ("IBV_WC_RNR_RETRY_EXC_ERR: RNR retry exceded error");
      break;
    case IBV_WC_LOC_RDD_VIOL_ERR:
      return ("IBV_WC_LOC_RDD_VIOL_ERR: local RDD violation error");
      break;
    case IBV_WC_REM_INV_RD_REQ_ERR:
      return ("IBV_WC_REM_INV_RD_REQ_ERR: remote invalid read request error");
      break;
    case IBV_WC_REM_ABORT_ERR:
      return ("IBV_WC_REM_ABORT_ERR: remote abort error");
      break;
    case IBV_WC_INV_EECN_ERR:
      return ("IBV_WC_INV_EECN_ERR: invalid EECN error");
      break;
    case IBV_WC_INV_EEC_STATE_ERR:
      return ("IBV_WC_INV_EEC_STATE_ERR: invalid EEC state error");
      break;
    case IBV_WC_FATAL_ERR:
      return ("IBV_WC_FATAL_ERR: fatal error");
      break;
    case IBV_WC_RESP_TIMEOUT_ERR:
      return ("IBV_WC_RESP_TIMEOUT_ERR: response timeout error");
      break;
    case IBV_WC_GENERAL_ERR:
      return ("IBV_WC_GENERAL_ERR: general error");
      break;
  }
  return ("ERROR NOT KNOWN");
}


__UNUSED__ static inline void
sctk_ib_polling_check_wc(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_wc wc) {
  sctk_ib_config_t *config = (rail_ib)->config;
  struct sctk_ibuf_s* ibuf;
  char host[HOSTNAME];
  char ibuf_desc[4096];

  if (wc.status != IBV_WC_SUCCESS) {
    ibuf = (struct sctk_ibuf_s*) wc.wr_id;
    assume(ibuf);
    gethostname(host, HOSTNAME);

    if (config->ibv_quiet_crash){
      sctk_error ("\033[1;31mIB - PROCESS %d CRASHED (%s)\033[0m: %s",
          sctk_process_rank, host, sctk_ib_polling_print_status(wc.status));
    } else {
      sctk_ibuf_print(ibuf, ibuf_desc);
      sctk_error ("\033[1;31m\nIB - FATAL ERROR FROM PROCESS %d (%s)\n"
          "################################\033[0m\n"
          "Work ID is   : %d\n"
          "Status       : %s\n"
          "ERROR Vendor : %d\n"
          "Byte_len     : %d\n"
          "Dest process : %d\n"
          "\033[1;31m######### IBUF DESC ############\033[0m\n"
          "%s\n"
          "\033[1;31m################################\033[0m",
          sctk_process_rank, host,
          wc.wr_id, sctk_ib_polling_print_status(wc.status),
          wc.vendor_err, wc.byte_len,
          ibuf->dest_process, ibuf_desc);
    }
    sctk_abort();
  }
}

__UNUSED__ static inline int sctk_ib_cq_poll(sctk_rail_info_t* rail,
    struct ibv_cq *cq, const int poll_nb, struct sctk_ib_polling_s *poll,
    int (*ptr_func)(sctk_rail_info_t* rail, struct ibv_wc*, struct sctk_ib_polling_s *poll))
{
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  struct ibv_wc wc;
  int found_nb = 0;
  int i, res = 0;

  for(i=0; i < poll_nb; ++i)
  {
      res = ibv_poll_cq (cq, 1, &wc);
      if (res) {
        sctk_ib_polling_check_wc(rail_ib, wc);
        ptr_func(rail, &wc, poll);
        found_nb++;
      }
  }
  return found_nb;
}

#if 0
/*-----------------------------------------------------------
 *  PROCESS
 *----------------------------------------------------------*/
typedef struct sctk_net_ibv_allocator_request_s
{
  sctk_net_ibv_ibuf_type_t      ibuf_type;
  sctk_net_ibv_ibuf_ptp_type_t  ptp_type;
  sctk_net_ibv_allocator_type_t channel;

  int dest_process;
  int dest_task;
  void* msg;
  void* msg_header;
  size_t size;
  size_t payload_size;
  size_t total_copied;

  /* for collective */
  uint8_t com_id;
  uint32_t psn;
#ifdef SCTK_USE_CHECKSUM
  uint32_t crc_full_payload;
#endif

  int buff_nb;
  int total_buffs;

  struct sctk_net_ibv_allocator_task_s *task;
} sctk_net_ibv_allocator_request_t;


UNUSED static void sctk_net_ibv_allocator_request_show(sctk_net_ibv_allocator_request_t* h)
{
  sctk_nodebug("#### Request ####"
      "ibuf_type : %d\n"
      "ptp_type : %d\n"
      "dest_process : %d\n"
      "dest_task : %d\n"
      "size : %lu\n"
      "psn : %lu\n"
      "payload_size : %lu\n"
      "buff_nb : %d\n"
      "total_buffs : %d\n"
      " com_id : %d\n",
      h->ibuf_type, h->ptp_type, h->dest_process,
      h->dest_task, h->size, h->psn, h->payload_size,
      h->buff_nb, h->total_buffs, h->com_id);
}

/* pending wc */
struct pending_wc_s
{
  struct ibv_wc wc;
  struct sctk_list_header list_header;
  struct OPA_Queue_element_hdr_t header;
};

/* local tasks */
typedef struct sctk_net_ibv_allocator_task_s
{
  /* ID of the task */
  int task_nb;
  /* vp where the task is located */
  int vp;
  /* node number */
  int node;
  /* task in polling */
  int in_polling;
  /* nb_pending msg*/
  int nb_pending;
  int max_pending;

  /* scheduler */
  sctk_spinlock_t sched_lock;
  sched_sn_t* remote;

  /* one pending list for each local task */
  struct sctk_list_header pending_msg;
  int pending_msg_nb;

  struct sctk_list_header** frag_eager_list;
  sctk_spinlock_t lock;

  /* TODO: new implementation from OPA queues */
  OPA_Queue_info_t* recv_wc;
  sctk_spinlock_t recv_lock;

  /* TODO: utash header */
  UT_hash_handle hh;
} sctk_net_ibv_allocator_task_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

#endif
#endif
#endif
