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
#define SCTK_IB_MODULE_NAME "QP"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "utlist.h"
#include <errno.h>

/*-----------------------------------------------------------
 *  DEVICE
 *----------------------------------------------------------*/

sctk_ib_device_t *sctk_ib_device_open(struct sctk_ib_rail_info_s* rail_ib, int rail_nb) {
    /*XXX Do not touch the following variable:
      Infiniband issue if dev_list is global */
  struct ibv_device** dev_list;
  sctk_ib_device_t* device;
  int devices_nb;

  dev_list = ibv_get_device_list (&devices_nb);
  /* Check parameters */
  if (!dev_list)
  {
    sctk_error ("No IB devices found");
    sctk_abort();
  }
  if (rail_nb >= devices_nb)
  {
    sctk_error("Cannot open rail. You asked rail %d on %d", rail_nb, devices_nb);
    sctk_abort();
  }

  sctk_ib_debug("Opening rail %d on %d", rail_nb, devices_nb);
  device = sctk_malloc(sizeof(sctk_ib_device_t));
  assume(device);

  device->context = ibv_open_device (dev_list[rail_nb]);
  if (!device->context)
  {
    sctk_error ("Cannot get devive context.");
    sctk_abort();
  }

  if ( ibv_query_device(device->context, &device->dev_attr) != 0)
  {
    sctk_error ("Unable to get device attr.");
    sctk_abort();
  }

  if (ibv_query_port(device->context, 1, &device->port_attr) != 0)
  {
    sctk_error ("Unable to get port attr");
    sctk_abort();
  }

//  rail->lid = device->port_attr.lid;
  srand48 (getpid () * time (NULL));
  rail_ib->device = device;
  return device;
}

/*-----------------------------------------------------------
 *  Protection Domain
 *----------------------------------------------------------*/
  struct ibv_pd*
sctk_ib_pd_init(sctk_ib_device_t *device)
{

  device->pd = ibv_alloc_pd(device->context);
  if (!device->pd) {
    sctk_error ("Cannot get IB protection domain.");
    sctk_abort();
  }
  return device->pd;
}

/*-----------------------------------------------------------
 *  Completion queue
 *----------------------------------------------------------*/
  struct ibv_cq*
sctk_ib_cq_init(sctk_ib_device_t* device,
    sctk_ib_config_t *config)
{
  struct ibv_cq *cq;
  cq = ibv_create_cq (device->context, config->ibv_cq_depth, NULL, NULL, 0);

  if (!cq) {
    sctk_error("Cannot create Completion Queue");
    sctk_abort();
  }
  return cq;
}

  char *
sctk_ib_cq_print_status (enum ibv_wc_status status)
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


/*-----------------------------------------------------------
 *  Exchange keys
 *----------------------------------------------------------*/
sctk_ib_qp_keys_t sctk_ib_qp_keys_convert( char* msg)
{
  sctk_ib_qp_keys_t keys;
  sscanf(msg, "%05"SCNu16":%010"SCNu32":%010"SCNu32, &keys.lid, &keys.qp_num, &keys.psn);
  sctk_nodebug("LID : %lu|psn : %lu|qp_num : %lu", keys.lid, keys.psn, keys.qp_num);

  return keys;
}

/*-----------------------------------------------------------
 *  Queue Pair Keys
 *----------------------------------------------------------*/

void sctk_ib_qp_keys_send(
    struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote)
{
  LOAD_DEVICE(rail_ib);
  int key_max = sctk_pmi_get_max_key_len();
  int val_max = sctk_pmi_get_max_val_len();
  char key[key_max];
  char val[key_max];

  /* FIXME: Change 0 if several rails at the same time */
  snprintf(key, key_max,"IB%02d:%06d:%06d", 0, sctk_process_rank, remote->rank);
  snprintf(val, val_max, "%05"SCNu16":%010"SCNu32":%010"SCNu32, device->port_attr.lid, remote->qp->qp_num, remote->psn);
  sctk_nodebug("Send KEY %s\t%s", key, val);
  sctk_pmi_put_connection_info_str(val, val_max, key);
}

  sctk_ib_qp_keys_t
sctk_ib_qp_keys_recv(sctk_ib_qp_t *remote, int dest_process)
{
  sctk_ib_qp_keys_t qp_keys;
  int key_max = sctk_pmi_get_max_key_len();
  int val_max = sctk_pmi_get_max_val_len();
  char key[key_max];
  char val[key_max];

  snprintf(key, key_max,"IB%02d:%06d:%06d", 0, dest_process, sctk_process_rank);
  sctk_pmi_get_connection_info_str(val, val_max, key);
  sctk_nodebug("Got KEY %s\t%s", key, val);
  qp_keys = sctk_ib_qp_keys_convert(val);

  return qp_keys;
}

/*-----------------------------------------------------------
 *  Queue Pair Keys
 *----------------------------------------------------------*/
sctk_ib_qp_t* sctk_ib_qp_new()
{
  sctk_ib_qp_t *remote;

  remote = sctk_malloc(sizeof(sctk_ib_qp_t));
  assume(remote);
  memset(remote, 0, sizeof(sctk_ib_qp_t));

  return remote;
}

void
sctk_ib_qp_free(sctk_ib_qp_t* remote) {

  if (!remote) {
    sctk_error("Trying to free a remote entry which is not initialized");
    sctk_abort();
  }
  free(remote);
}

  struct ibv_qp*
sctk_ib_qp_init(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote, struct ibv_qp_init_attr* attr, int rank)
{
  LOAD_DEVICE(rail_ib);

  remote->qp = ibv_create_qp (device->pd, attr);
  if (!remote->qp) {
    sctk_error("Cannot create QP for rank %d", rank);
    sctk_abort();
  }
  sctk_nodebug("QP Initialized for rank %d %p", remote->rank, remote->qp);
  return remote->qp;
}

  struct ibv_qp_init_attr
sctk_ib_qp_init_attr(struct sctk_ib_rail_info_s* rail_ib)
{
  LOAD_DEVICE(rail_ib);
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_init_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_init_attr));

  attr.send_cq  = device->send_cq;
  attr.recv_cq  = device->recv_cq;
  attr.srq      = device->srq;
  attr.cap.max_send_wr  = config->ibv_qp_tx_depth;
  attr.cap.max_recv_wr  = config->ibv_qp_rx_depth;
  attr.cap.max_send_sge = config->ibv_max_sg_sq;
  attr.cap.max_recv_sge = config->ibv_max_sg_rq;
  attr.cap.max_inline_data = config->ibv_max_inline;
  /* RC Transport by default */
  attr.qp_type = IBV_QPT_RC;
  /* if this value is set to 1, all work requests (WR) will
   * generate completion queue events (CQE). If this value is set to 0,
   * only WRs that are flagged will generate CQE's*/
  attr.sq_sig_all = 1;
  return attr;
}

  struct ibv_qp_attr
sctk_ib_qp_state_init_attr(struct sctk_ib_rail_info_s* rail_ib,
    int *flags)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  /*init state */
  attr.qp_state = IBV_QPS_INIT;
  /* pkey index, normally 0 */
  attr.pkey_index = 0;
  /* physical port number (1 .. n) */
  attr.port_num = config->ibv_adm_port;
  attr.qp_access_flags =
    IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
    IBV_ACCESS_REMOTE_READ;

  *flags = IBV_QP_STATE |
    IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

  return attr;
}

  struct ibv_qp_attr
sctk_ib_qp_state_rtr_attr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_keys_t* keys, int *flags)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  attr.qp_state = IBV_QPS_RTR;
  /* 512 is the recommended value */
  attr.path_mtu = IBV_MTU_1024;
  /* QP number of remote QP */
  /* maximul number if resiyrces for incoming RDMA request */
  attr.max_dest_rd_atomic = config->ibv_rdma_dest_depth;
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
  attr.ah_attr.port_num = config->ibv_adm_port;

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
sctk_ib_qp_state_rts_attr(struct sctk_ib_rail_info_s* rail_ib,
    uint32_t psn, int *flags)
{
  LOAD_CONFIG(rail_ib);
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
  attr.max_rd_atomic = config->ibv_rdma_dest_depth;

  *flags = IBV_QP_STATE |
    IBV_QP_TIMEOUT |
    IBV_QP_RETRY_CNT |
    IBV_QP_RNR_RETRY |
    IBV_QP_SQ_PSN |
    IBV_QP_MAX_QP_RD_ATOMIC;

  return attr;
}

  void
sctk_ib_qp_modify( sctk_ib_qp_t* remote, struct ibv_qp_attr* attr, int flags)
{

  if (ibv_modify_qp(remote->qp, attr, flags) != 0)
  {
    sctk_error("Cannot modify Queue Pair");
    sctk_abort();
  }
}

/*-----------------------------------------------------------
 *  Shared Receive queue
 *----------------------------------------------------------*/
  struct ibv_srq*
sctk_ib_srq_init(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_srq_init_attr* attr)
{
  LOAD_DEVICE(rail_ib);
  device->srq = ibv_create_srq(device->pd, attr);
  if (!device->srq)
  {
    perror("error");
    sctk_error("Cannot create Shared Received Queue");
    sctk_abort();
  }
  return device->srq;
}

  struct ibv_srq_init_attr
sctk_ib_srq_init_attr(struct sctk_ib_rail_info_s* rail_ib)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_srq_init_attr attr;

  memset (&attr, 0, sizeof (struct ibv_srq_init_attr));

  attr.attr.max_wr = config->ibv_max_srq_ibufs_posted;
  attr.attr.max_sge = config->ibv_max_sg_rq;

  return attr;
}

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
  void
sctk_ib_qp_allocate_init(struct sctk_ib_rail_info_s* rail_ib,
    int rank, sctk_ib_qp_t* remote)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_init_attr  init_attr;
  struct ibv_qp_attr       attr;
  int flags;

  remote->psn = lrand48 () & 0xffffff;
  remote->rank = rank;
  remote->is_connected = 0;
  remote->free_nb = config->ibv_qp_tx_depth;
  remote->post_lock = SCTK_SPINLOCK_INITIALIZER;

  init_attr = sctk_ib_qp_init_attr(rail_ib);
  sctk_ib_qp_init(rail_ib, remote, &init_attr, rank);

  attr = sctk_ib_qp_state_init_attr(rail_ib, &flags);
  sctk_ib_qp_modify(remote, &attr, flags);
}

void
sctk_ib_qp_allocate_rtr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote,sctk_ib_qp_keys_t* keys)
{
  struct ibv_qp_attr       attr;
  int flags;

  attr = sctk_ib_qp_state_rtr_attr(rail_ib, keys, &flags);
  sctk_nodebug("Modify QR RTR for rank %d", remote->rank);
  sctk_ib_qp_modify(remote, &attr, flags);
  remote->is_rtr = 1;
}

void
sctk_ib_qp_allocate_rts(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote)
{
  struct ibv_qp_attr       attr;
  int flags;

  attr = sctk_ib_qp_state_rts_attr(rail_ib, remote->psn, &flags);
  sctk_nodebug("Modify QR RTS for rank %d", remote->rank);
  sctk_ib_qp_modify(remote, &attr, flags);
  remote->is_rts = 1;
}

struct wait_send_s {
  int flag;
  sctk_ib_qp_t *remote;
  sctk_ibuf_t *ibuf;
};

void sctk_ib_qp_release_entry(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote) {
  not_implemented();

  sctk_spinlock_lock(&remote->post_lock);
  remote->free_nb++;
  sctk_spinlock_unlock(&remote->post_lock);
}

static void* wait_send(void *arg){
  struct wait_send_s *a = (struct wait_send_s*) arg;
  int rc;

  not_implemented();
  if (a->remote->free_nb > 0) {
    if (sctk_spinlock_trylock(&a->remote->post_lock) == 0) {
      if (a->remote->free_nb > 0) {
        rc = ibv_post_send(a->remote->qp, &(a->ibuf->desc.wr.send),
            &(a->ibuf->desc.bad_wr.send));
        assume(rc == 0);
        a->remote->free_nb--;
        a->flag = 1;
      }
      sctk_spinlock_unlock(&a->remote->post_lock);
    }
  }
  return NULL;
}

  void
sctk_ib_qp_send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf)
{
  int rc, need_wait = 1;
  struct wait_send_s wait_send_arg;
  sctk_nodebug("Send message to process %d %p", remote->rank, remote->qp);

  ibuf->remote = remote;

//  if (remote->free_nb > 0) {
//    sctk_spinlock_lock(&remote->post_lock);
//    if (remote->free_nb > 0) {
      rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
      if( rc != 0) {
        not_implemented();
      }
//      assume(rc == 0);
//      remote->free_nb--;
//      need_wait = 0;
//    }
//    sctk_spinlock_unlock(&remote->post_lock);
//  }

#if 0
  if (need_wait)
  {
    wait_send_arg.flag = 0;
    wait_send_arg.remote = remote;
    wait_send_arg.ibuf = ibuf;

    sctk_ib_nodebug("QP full, waiting for posting message...");
    sctk_thread_wait_for_value_and_poll (&wait_send_arg.flag, 1,
        (void (*)(void *)) wait_send, &wait_send_arg);
  }
#endif
}


#if 0
int sctk_ib_qp_send_post_pending(sctk_ib_qp_remote_t* remote, int need_lock)
{
  struct sctk_list_header *tmp;
  sctk_ib_ibuf_t* ibuf;
  int rc;

  if (sctk_ib_list_is_empty(&remote->pending_send_wqe))
    return 0;

  if (need_lock)
    sctk_spinlock_lock(&remote->send_wqe_lock);

  do {
    tmp = sctk_ib_list_pop(&remote->pending_send_wqe);

    if (tmp) {
      ibuf = sctk_ib_list_get_entry(tmp, sctk_ib_ibuf_t, list_header);

      if ( (remote->ibv_got_send_wqe + 1) > ibv_qp_tx_depth)
      {
        if (need_lock)
          sctk_spinlock_unlock(&remote->send_wqe_lock);

        return 1;
      }

      sctk_nodebug("Got %d", remote->ibv_got_send_wqe);
      rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
      sctk_assert(rc == 0);
      sctk_nodebug("Found ibuf %p pending", ibuf);

      remote->ibv_got_send_wqe++;
      remote->ibv_free_send_wqe--;
    }
  } while (tmp);

    if (need_lock)
      sctk_spinlock_unlock(&remote->send_wqe_lock);

    return 0;
}

void sctk_ib_qp_send_free_wqe(sctk_ib_qp_remote_t* remote )
{
  sctk_assert(remote);
  sctk_spinlock_lock(&remote->send_wqe_lock);
  remote->ibv_got_send_wqe --;
  remote->ibv_free_send_wqe ++;

  sctk_nodebug("FREE ibv_got_send_wqe : %d", remote->ibv_got_send_wqe);
  sctk_spinlock_unlock(&remote->send_wqe_lock);
}

int sctk_ib_qp_send_get_wqe(int dest_process, sctk_ib_ibuf_t* ibuf)
{
  sctk_ib_qp_remote_t* remote;
  int rc;

  /* check if the TCP connection is active. If not, connect peers */
  remote = sctk_ib_comp_rc_sr_check_and_connect(dest_process);

  ibuf->remote = remote;

  sctk_spinlock_lock(&remote->send_wqe_lock);
  sctk_ib_qp_send_post_pending(remote, 0);


//#if 0
  /* TODO: do not post buffer if QP is full */
  while ( (remote->ibv_got_send_wqe + 1) > ibv_qp_tx_depth) {
    sctk_spinlock_unlock(&remote->send_wqe_lock);
    sctk_thread_yield();
    sctk_spinlock_lock(&remote->send_wqe_lock);
  }
//#endif

  if ( (remote->ibv_got_send_wqe + 1) > ibv_qp_tx_depth) {
    sctk_nodebug("No more WQE available");
    sctk_ib_list_push_tail(&remote->pending_send_wqe, &ibuf->list_header);
    sctk_spinlock_unlock(&remote->send_wqe_lock);

    return -1;
  } else {
    remote->ibv_got_send_wqe++;
    remote->ibv_free_send_wqe--;
    sctk_nodebug("GET ibv_got_send_wqe : %d", remote->ibv_got_send_wqe);

    /*
     * we can post the message to the queue because we are sure that there
     * is a free slot.
     */
    sctk_spinlock_unlock(&remote->send_wqe_lock);

    rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
    assume(rc == 0);

    return remote->ibv_got_send_wqe;
  }
}

#endif

#endif
