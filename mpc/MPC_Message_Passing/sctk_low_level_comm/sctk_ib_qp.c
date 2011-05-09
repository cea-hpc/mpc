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
#include "sctk_bootstrap.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_const.h"
#include "sctk_ib_config.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_comp_rc_sr.h"
#include "sctk_ib_config.h"
#include "sctk_mpcrun_client.h"
#include "sctk_alloc.h"
#include "sctk_debug.h"

#include <string.h>
#include <inttypes.h>

static int    dev_nb;


/*-----------------------------------------------------------
 *  RAIL
 *----------------------------------------------------------*/

  void
sctk_net_ibv_qp_rail_init(sctk_net_ibv_mmu_t* mmu)
{
  /* get the devices' list */
//  dev_list = ibv_get_device_list (&dev_nb);
//  if (!dev_list)
//  {
//    sctk_error ("No IB devices found");
//    sctk_abort();
//  }
}

  sctk_net_ibv_qp_rail_t*
sctk_net_ibv_qp_pick_rail(int rail_nb)
{
  struct ibv_device_attr dev_attr;
  struct ibv_port_attr port_attr;
/*
     TODO Do not touch the following variable: infiniband issue if dev_list is global
     */
  struct ibv_device** dev_list;

  dev_list = ibv_get_device_list (&dev_nb);
  if (!dev_list)
  {
    sctk_error ("No IB devices found");
    sctk_abort();
  }

  if (rail_nb > dev_nb)
    return NULL;

  sctk_net_ibv_qp_rail_t* rail = sctk_malloc(sizeof(sctk_net_ibv_qp_rail_t));

  rail->context = ibv_open_device (dev_list[rail_nb]);
  if (rail->context == NULL)
  {
    sctk_error ("Cannot get IB context.");
    sctk_abort();
  }

  if ( ibv_query_device
      (rail->context, &dev_attr) != 0)
  {
    sctk_error ("Unable to get device attr.");
    sctk_abort();
  }
  rail->max_mr = dev_attr.max_mr;
  sctk_nodebug("Max mr : %d",dev_attr.max_mr );
  sctk_nodebug("Max qp : %d",dev_attr.max_qp );

  if (ibv_query_port
      (rail->context, ibv_adm_port, &port_attr))
  {
    sctk_error ("Unable to get port attr");
    sctk_abort();
  }

  rail->lid = port_attr.lid;
  srand48 (getpid () * time (NULL));

  return rail;
}

int sctk_net_ibv_qp_send_post_pending(sctk_net_ibv_qp_remote_t* remote)
{
  struct sctk_list_elem *tmp, *to_free;
  sctk_net_ibv_ibuf_t* ibuf;
  int rc;

  sctk_thread_mutex_lock(&remote->send_wqe_lock);
  tmp = remote->pending_send_wqe.head;
  while (tmp) {

    if ( (remote->ibv_got_send_wqe + 1) > ibv_qp_tx_depth)
    {
      sctk_thread_mutex_unlock(&remote->send_wqe_lock);
      return 0;
    }

    ibuf = (sctk_net_ibv_ibuf_t*) tmp->elem;

    sctk_nodebug("Got %d", remote->ibv_got_send_wqe);
    rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
    assume(rc == 0);
    sctk_nodebug("Found ibuf %p pending", ibuf);

    remote->ibv_got_send_wqe++;
    remote->ibv_free_send_wqe--;

    to_free = tmp;
    tmp = tmp->p_next;
    sctk_list_remove(&remote->pending_send_wqe, to_free);
  }

  sctk_thread_mutex_unlock(&remote->send_wqe_lock);
  return 0;
}

void sctk_net_ibv_qp_send_free_wqe(sctk_net_ibv_qp_remote_t* remote )
{
  assume(remote);
  sctk_thread_mutex_lock(&remote->send_wqe_lock);
  remote->ibv_got_send_wqe --;
  remote->ibv_free_send_wqe ++;

  sctk_nodebug("FREE ibv_got_send_wqe : %d", remote->ibv_got_send_wqe);
  sctk_thread_mutex_unlock(&remote->send_wqe_lock);
}

int sctk_net_ibv_qp_send_get_wqe(int dest_process, sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_qp_remote_t* remote;

    /* check if the TCP connection is active. If not, connect peers */
  remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  ibuf->remote = remote;
  int rc;

  sctk_thread_mutex_lock(&remote->send_wqe_lock);
  if ( (remote->ibv_got_send_wqe + 1) > ibv_qp_tx_depth) {
    sctk_nodebug("No more WQE available");
    sctk_list_push(&remote->pending_send_wqe, ibuf);
    sctk_thread_mutex_unlock(&remote->send_wqe_lock);

    return -1;
  } else {
    remote->ibv_got_send_wqe++;
    remote->ibv_free_send_wqe--;
    sctk_nodebug("GET ibv_got_send_wqe : %d", remote->ibv_got_send_wqe);

    /*
     * we can post the message to the queue because we are sure that there
     * is a free slot.
     */
    sctk_thread_mutex_unlock(&remote->send_wqe_lock);
    rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
    assume(rc == 0);

    return remote->ibv_got_send_wqe;
  }
}


/*-----------------------------------------------------------
 *  NEW/FREE
 *----------------------------------------------------------*/
  sctk_net_ibv_qp_local_t*
sctk_net_ibv_qp_new(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t *qp;

  qp = sctk_malloc(sizeof(sctk_net_ibv_qp_local_t));
  assume(qp);

  memset(qp, 0, sizeof(sctk_net_ibv_qp_local_t));
  qp->context = rail->context;

  return qp;
}

void
sctk_net_ibv_qp_free(sctk_net_ibv_qp_local_t* qp) {

  if (qp)
    free(qp);
}

/*-----------------------------------------------------------
 *  Protection Domain
 *----------------------------------------------------------*/

  struct ibv_pd*
sctk_net_ibv_pd_init(sctk_net_ibv_qp_local_t* local)
{

  local->pd = ibv_alloc_pd(local->context);
  if (!local->pd)
  {
    sctk_error ("Cannot get IB protection domain.");
    sctk_abort();
  }
  return local->pd;
}

/*-----------------------------------------------------------
 *  Queue Pair
 *----------------------------------------------------------*/

  struct ibv_qp*
sctk_net_ibv_qp_init(sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t* remote, struct ibv_qp_init_attr* attr, int rank)
{
  remote->qp = ibv_create_qp (local->pd, attr);
  if (!remote->qp)
  {
    sctk_error("Cannot create QP");
    sctk_abort();
  }

  sctk_list_new(&remote->pending_send_wqe, 0, 0);
  sctk_thread_mutex_init(&remote->send_wqe_lock, NULL);
  sctk_nodebug("Initialized : %d", remote->rank);


  return remote->qp;
}

  struct ibv_qp_init_attr
sctk_net_ibv_qp_init_attr(struct ibv_cq* send_cq, struct ibv_cq* recv_cq, struct ibv_srq* srq)
{
  struct ibv_qp_init_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_init_attr));

  attr.send_cq = send_cq;
  attr.recv_cq = recv_cq;
  attr.srq = srq;
  attr.cap.max_send_wr = ibv_qp_tx_depth;
  attr.cap.max_recv_wr = ibv_qp_rx_depth;
  attr.cap.max_send_sge = ibv_max_sg_sq;
  attr.cap.max_recv_sge = ibv_max_sg_rq;
  attr.cap.max_inline_data = ibv_max_inline;
  /* RC Transport by default */
  attr.qp_type = IBV_QPT_RC;
  /* if this value is set to 1, all work requests (WR) will
   * generate completion queue events (CQE). If this value is set to 0,
   * only WRs that are flagged will generate CQE's*/
  attr.sq_sig_all = 1;

  return attr;
}

  struct ibv_qp_attr
sctk_net_ibv_qp_state_init_attr(int *flags)
{
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  /*init state */
  attr.qp_state = IBV_QPS_INIT;
  /* pkey index, normally 0 */
  attr.pkey_index = 0;
  /* physical port number (1 .. n) */
  attr.port_num = ibv_adm_port;
  attr.qp_access_flags =
    IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
    IBV_ACCESS_REMOTE_READ;

  *flags = IBV_QP_STATE |
    IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

  return attr;
}

  struct ibv_qp_attr
sctk_net_ibv_qp_state_rtr_attr(sctk_net_ibv_qp_exchange_keys_t* keys, int *flags)
{
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  attr.qp_state = IBV_QPS_RTR;
  /* 512 is the recommended value */
  attr.path_mtu = IBV_MTU_1024;
  /* QP number of remote QP */
  /* maximul number if resiyrces for incoming RDMA request */
  attr.max_dest_rd_atomic = ibv_rdma_dest_depth;
  /* maximum RNR NAK timer (recommanded value: 12) */
  attr.min_rnr_timer = 12;

  /* an address handle (AH) needs to be created and filled in as appropriate. */
  attr.ah_attr.is_global = 0;
  /* destination LID */
  attr.ah_attr.dlid = keys->lid;
  /* QP number of remote QP */
  attr.dest_qp_num = keys->qp_num;
  attr.rq_psn = keys->psn;
  /* service level */
  attr.ah_attr.sl = 0;
  /* source path bits */
  attr.ah_attr.src_path_bits = 0;
  attr.ah_attr.port_num = ibv_adm_port;

  *flags = IBV_QP_STATE |
    IBV_QP_AV |
    IBV_QP_PATH_MTU |
    IBV_QP_DEST_QPN |
    IBV_QP_RQ_PSN |
    IBV_QP_MAX_DEST_RD_ATOMIC |
    IBV_QP_MIN_RNR_TIMER;

  return attr;
}

  struct ibv_qp_attr
sctk_net_ibv_qp_state_rts_attr( uint32_t psn, int *flags)
{
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  attr.qp_state = IBV_QPS_RTS;
  /* local ACK timeout (recommanted value: 14) */
  attr.timeout = 14;
  /* retry count (recommended value: 7) */
  attr.retry_cnt = 7;
  /* RNR retry count (recommended value: 7) */
  attr.rnr_retry = 7;
  /* packet sequence number */
  attr.sq_psn = psn;
  /* number or outstanding RDMA reads and atomic operations allowed */
  attr.max_rd_atomic = ibv_rdma_dest_depth;

  *flags = IBV_QP_STATE |
    IBV_QP_TIMEOUT |
    IBV_QP_RETRY_CNT |
    IBV_QP_RNR_RETRY |
    IBV_QP_SQ_PSN |
    IBV_QP_MAX_QP_RD_ATOMIC;

  return attr;
}


  void
sctk_net_ibv_qp_modify( sctk_net_ibv_qp_remote_t* remote, struct ibv_qp_attr* attr, int flags)
{

  if (ibv_modify_qp(remote->qp, attr, flags) != 0)
  {
    sctk_error("Cannot modify Queue Pair");
    sctk_abort();
  }
}

/*-----------------------------------------------------------
 *  Completion queue
 *----------------------------------------------------------*/

  struct ibv_cq*
sctk_net_ibv_cq_init(sctk_net_ibv_qp_rail_t* rail)
{
  struct ibv_cq* cq;

  cq = ibv_create_cq (rail->context, ibv_cq_depth, NULL, NULL, 0);

  if (!cq)
  {
    sctk_error("Cannot create Completion Queue");
    sctk_abort();
  }

  return cq;
}

  char *
sctk_net_ibv_cq_print_status (enum ibv_wc_status status)
{
  switch (status)
  {
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


static void
sctk_net_ibv_poll_check_wc(struct ibv_wc wc, sctk_net_ibv_allocator_type_t type) {

  if (wc.status != IBV_WC_SUCCESS)
  {
    sctk_net_ibv_ibuf_t* ibuf;
    sctk_net_ibv_ibuf_header_t* header;

    ibuf = (sctk_net_ibv_ibuf_t*) wc.wr_id;
    assume(ibuf);
    header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

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
        "\033[1;31m################################\033[0m\n",
        sctk_process_rank,
        wc.wr_id, sctk_net_ibv_cq_print_status(wc.status),
        wc.vendor_err, wc.byte_len, sctk_net_ibv_ibuf_print_flag(ibuf->flag),
        header->ibuf_type, header->ptp_type, header->buff_nb, header->total_buffs);

    sctk_abort();
  }
}


  void
sctk_net_ibv_cq_poll(struct ibv_cq* cq, int pending_nb, void (*ptr_func)(struct ibv_wc*, int lookup, int dest), sctk_net_ibv_allocator_type_t type)
{
  struct ibv_wc wc;
  int ne = 0;
  int i;

  for (i=0; i < pending_nb; ++i)
  {
    ne = ibv_poll_cq (cq, 1, &wc);
    if (ne)
    {
      sctk_net_ibv_poll_check_wc(wc, type);
      ptr_func(&wc, 0, 0);
    } else {
      return;
    }
  }
}

  void
sctk_net_ibv_cq_lookup(struct ibv_cq* cq, int nb_pending, void (*ptr_func)(struct ibv_wc*, int lookup, int dest), int dest, sctk_net_ibv_allocator_type_t type)
{
  struct ibv_wc wc[nb_pending];
  int ne = 0;
  int i;

  ne = ibv_poll_cq (cq, nb_pending, wc);

  for (i = 0; i < ne; ++i)
  {
    sctk_nodebug("%d elements found", ne);

    sctk_net_ibv_poll_check_wc(wc[i], type);

    ptr_func(&wc[i], 1, dest);
  }
}

  int
sctk_net_ibv_cq_garbage_collector(struct ibv_cq* cq, int nb_pending, void (*ptr_func)(struct ibv_wc*), sctk_net_ibv_allocator_type_t type)
{
  struct ibv_wc wc;
  int ne = 0;
  int i;

  sctk_nodebug("garbage");

  for (i = 0; i < nb_pending; ++i)
  {
    ne = ibv_poll_cq (cq, 1, &wc);
    if (ne)
    {
      sctk_nodebug("%d elements found", ne);

      sctk_net_ibv_poll_check_wc(wc, type);
      ptr_func(&wc);
    } else {
      return 0;
    }
  }
  return ne;
}



/*-----------------------------------------------------------
 *  Shared Receive queue
 *----------------------------------------------------------*/

  struct ibv_srq*
sctk_net_ibv_srq_init( sctk_net_ibv_qp_local_t* local, struct ibv_srq_init_attr* attr)
{

  local->srq = ibv_create_srq(local->pd, attr);
  if (!local->srq)
  {
    sctk_error("Cannot create Shared Received Queue");
    sctk_abort();
  }

  sctk_nodebug("SRQ : %p", local->srq);
  return local->srq;
}

  struct ibv_srq_init_attr
sctk_net_ibv_srq_init_attr()
{
  struct ibv_srq_init_attr attr;

  memset (&attr, 0, sizeof (struct ibv_srq_init_attr));

  attr.attr.max_wr = ibv_max_srq_ibufs;
  attr.attr.max_sge = ibv_max_sg_rq;

  return attr;
}


/*-----------------------------------------------------------
 *  Exchange keys
 *----------------------------------------------------------*/
  sctk_net_ibv_qp_exchange_keys_t
sctk_net_ibv_qp_exchange_convert( char* msg)
{
  sctk_net_ibv_qp_exchange_keys_t keys;

  sscanf(msg, "%05"SCNu16":%010"SCNu32":%010"SCNu32, &keys.lid, &keys.qp_num, &keys.psn);

  sctk_nodebug("LID : %lu|psn : %lu|qp_num : %lu", keys.lid, keys.psn, keys.qp_num);

  return keys;
}

void
sctk_net_ibv_qp_exchange_send(
    int i,
    sctk_net_ibv_qp_rail_t* rail,
    sctk_net_ibv_qp_remote_t* remote)
{
  int key_max;
  int val_max;

  key_max = sctk_bootstrap_get_max_key_len();
  val_max = sctk_bootstrap_get_max_val_len();
  char key[key_max];
  char val[key_max];

  snprintf(key, key_max,"%06d:%06d:%06d", i, sctk_process_rank, remote->rank);
  sctk_nodebug("Send KEY (%d) %s", key_max, key);

  snprintf(val, val_max, "%05"SCNu16":%010"SCNu32":%010"SCNu32, rail->lid, remote->qp->qp_num, remote->psn);
  sctk_nodebug("Send VAL (%d) %s", val_max, val);

  sctk_bootstrap_register(key, val, val_max);
}

  sctk_net_ibv_qp_exchange_keys_t
sctk_net_ibv_qp_exchange_recv(int i, sctk_net_ibv_qp_local_t* qp, int dest_process)
{
  sctk_net_ibv_qp_exchange_keys_t qp_keys;
  int key_max;
  int val_max;

  key_max = sctk_bootstrap_get_max_key_len();
  val_max = sctk_bootstrap_get_max_val_len();
  char key[key_max];
  char val[key_max];

  snprintf(key, key_max,"%06d:%06d:%06d", i, dest_process, sctk_process_rank);
  sctk_nodebug("Recv KEY %s", key);

  sctk_bootstrap_get(key, val, val_max);

  sctk_nodebug("Recv VAL %s", val);

  qp_keys = sctk_net_ibv_qp_exchange_convert(val);

  return qp_keys;
}

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/

  void
sctk_net_ibv_qp_allocate_init(int rank,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t* remote)
{
  struct ibv_qp_init_attr  init_attr;
  struct ibv_qp_attr       attr;
  int flags;

  remote->psn = lrand48 () & 0xffffff;
  remote->rank = rank;
  remote->is_connected = 0;

  init_attr = sctk_net_ibv_qp_init_attr(local->send_cq,
      local->recv_cq, local->srq);
  sctk_net_ibv_qp_init(local, remote, &init_attr, rank);

  attr = sctk_net_ibv_qp_state_init_attr(&flags);
  sctk_net_ibv_qp_modify(remote, &attr, flags);
}

void
sctk_net_ibv_qp_allocate_recv(
    sctk_net_ibv_qp_remote_t *remote,
    sctk_net_ibv_qp_exchange_keys_t* keys)
{
  struct ibv_qp_attr       attr;
  int flags;

  attr = sctk_net_ibv_qp_state_rtr_attr(keys, &flags);
  sctk_nodebug("Modify QR RTR for process %d", rank);
  sctk_net_ibv_qp_modify(remote, &attr, flags);

  attr = sctk_net_ibv_qp_state_rts_attr(remote->psn, &flags);
  sctk_nodebug("Modify QR RTS for process %d", rank);
  sctk_net_ibv_qp_modify(remote, &attr, flags);
}
#endif
