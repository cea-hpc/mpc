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
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_prof.h"
#include "sctk_pmi.h"
#include "utlist.h"
#include <sctk_spinlock.h>
#include <errno.h>

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "QP"
#include "sctk_ib_toolkit.h"

/*-----------------------------------------------------------
 *  DEVICE
 *----------------------------------------------------------*/
sctk_ib_device_t *sctk_ib_device_init(struct sctk_ib_rail_info_s* rail_ib) {
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
 device = sctk_malloc(sizeof(sctk_ib_device_t));
  assume(device);

  rail_ib->device = device;
  rail_ib->device->dev_list = dev_list;
  rail_ib->device->dev_nb = devices_nb;
  /* Specific to ondemand (de)connexions */
  rail_ib->device->ondemand.qp_list = NULL;
  rail_ib->device->ondemand.qp_list_ptr = NULL;
  rail_ib->device->ondemand.lock = SCTK_SPINLOCK_INITIALIZER;

  return device;
}

sctk_ib_device_t *sctk_ib_device_open(struct sctk_ib_rail_info_s* rail_ib, int rail_nb) {
  LOAD_DEVICE(rail_ib);
  struct ibv_device** dev_list = device->dev_list;
  int devices_nb = device->dev_nb;

  if (rail_nb >= devices_nb)
  {
    sctk_error("Cannot open rail. You asked rail %d on %d", rail_nb, devices_nb);
    sctk_abort();
  }

  sctk_ib_nodebug("Opening rail %d on %d", rail_nb, devices_nb);

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
  sctk_nodebug("device %d", device->dev_attr.max_qp_wr);
  srand48 (getpid () * time (NULL));
  rail_ib->device->dev_index = rail_nb;
  rail_ib->device->dev = dev_list[rail_nb];
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
  /* destroy the QP */
  ibv_destroy_qp(remote->qp);
  /* We cannot really remove the entry... We leave it as it*/
  /* free(remote); */
}

  struct ibv_qp*
sctk_ib_qp_init(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote, struct ibv_qp_init_attr* attr, int rank)
{
  LOAD_DEVICE(rail_ib);

  remote->qp = ibv_create_qp (device->pd, attr);
  PROF_INC_RAIL_IB(rail_ib, qp_created);
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
  attr.sq_sig_all = 0;
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

  struct ibv_qp_attr
sctk_ib_qp_state_reset_attr(struct sctk_ib_rail_info_s* rail_ib,
    uint32_t psn, int *flags)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  /* Reset the state of the QP */
  attr.qp_state = IBV_QPS_RESET;

  *flags = IBV_QP_STATE;

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
  LOAD_CONFIG(rail_ib);
  device->srq = ibv_create_srq(device->pd, attr);
  config->ibv_max_srq_ibufs_posted = attr->attr.max_wr;

  sctk_ib_debug("Initializing SRQ with %d entries (max:%d)",
      attr->attr.max_wr, sctk_ib_srq_get_max_srq_wr(rail_ib));

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

  attr.attr.srq_limit = config->ibv_srq_credit_thread_limit;
  attr.attr.max_wr = config->ibv_max_srq_ibufs;
  attr.attr.max_sge = config->ibv_max_sg_rq;

  return attr;
}

int
sctk_ib_srq_get_max_srq_wr (struct sctk_ib_rail_info_s* rail_ib)
{
  LOAD_DEVICE(rail_ib);
  return device->dev_attr.max_srq_wr;
}

int
sctk_ib_qp_get_cap_flags(struct sctk_ib_rail_info_s* rail_ib) {
  LOAD_DEVICE(rail_ib);
  return device->dev_attr.device_cap_flags;
}


/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
  void
sctk_ib_qp_allocate_init(struct sctk_ib_rail_info_s* rail_ib,
    int rank, sctk_ib_qp_t* remote, int ondemand, sctk_route_table_t* route_table)
{
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);
  sctk_ib_qp_ondemand_t *od = &device->ondemand;
  struct ibv_qp_init_attr  init_attr;
  struct ibv_qp_attr       attr;
  int flags;

  remote->route_table = route_table;
  remote->psn = lrand48 () & 0xffffff;
  remote->rank = rank;
  remote->free_nb = config->ibv_qp_tx_depth;
  remote->post_lock = SCTK_SPINLOCK_INITIALIZER;
  /* For buffered eager */
  remote->ib_buffered.entries = NULL;
  remote->ib_buffered.lock = SCTK_SPINLOCK_INITIALIZER;

  /* Add it to the Cicrular Linked List if the QP is created from
   * an ondemand request */
  if (ondemand) {
    remote->R = 1;
    remote->ondemand = 1;
    sctk_spinlock_lock(&od->lock);
    sctk_debug("Add QP to rank %d %p", remote->rank, remote);
    CDL_PREPEND(od->qp_list, remote);
    if (od->qp_list_ptr == NULL) {
      od->qp_list_ptr = remote;
    }
    sctk_spinlock_unlock(&od->lock);
  }

  init_attr = sctk_ib_qp_init_attr(rail_ib);
  sctk_ib_qp_init(rail_ib, remote, &init_attr, rank);
  sctk_ib_qp_allocate_set_rtr(remote, 0);
  sctk_ib_qp_allocate_set_rts(remote, 0);
  sctk_ib_qp_set_requests_nb(remote, 0);
  sctk_ib_qp_set_deco_canceled(remote, ACK_OK);

  sctk_ib_qp_set_local_flush_ack(remote, ACK_UNSET);
  sctk_ib_qp_set_remote_flush_ack(remote, ACK_UNSET);

  remote->deco_lock = 0;
  remote->lock_rtr = SCTK_SPINLOCK_INITIALIZER;
  remote->lock_rts = SCTK_SPINLOCK_INITIALIZER;
  /* Lock for sending messages */
  sctk_spin_rwlock_init(&remote->lock_send);

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
  sctk_ib_qp_allocate_set_rtr(remote, 1);
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
  sctk_ib_qp_allocate_set_rts(remote, 1);
}

  void
sctk_ib_qp_allocate_reset(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote)
{
  struct ibv_qp_attr       attr;
  int flags;

  attr = sctk_ib_qp_state_reset_attr(rail_ib, remote->psn, &flags);
  sctk_nodebug("Modify QR RESET for rank %d", remote->rank);
  sctk_ib_qp_modify(remote, &attr, flags);
}


/*-----------------------------------------------------------
 *  Send a message to a remote QP
 *----------------------------------------------------------*/
struct wait_send_s {
  int flag;
  sctk_ib_qp_t *remote;
  sctk_ibuf_t *ibuf;
};

void sctk_ib_qp_release_entry(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote) {
  not_implemented();
}

static void* wait_send(void *arg){
  struct wait_send_s *a = (struct wait_send_s*) arg;
  int rc;

  rc = ibv_post_send(a->remote->qp, &(a->ibuf->desc.wr.send),
      &(a->ibuf->desc.bad_wr.send));
  if (rc == 0)
  {
    a->flag = 1;
  }
  return NULL;
}

/* Send a message without locks. This is usefull for QP deconnexion */
  static inline void __send_ibuf_nolock(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf)
{
  int rc;
  LOAD_DEVICE(rail_ib);
  sctk_ib_qp_ondemand_t *od = &device->ondemand;

  sctk_nodebug("Send no-lock message to process %d %p", remote->rank, remote->qp);

  ibuf->remote = remote;

  rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
  if( rc != 0) {
    struct wait_send_s wait_send_arg;
    wait_send_arg.flag = 0;
    wait_send_arg.remote = remote;
    wait_send_arg.ibuf = ibuf;

    assume(0);
    sctk_ib_nodebug("QP full, waiting for posting message...");
    sctk_thread_wait_for_value_and_poll (&wait_send_arg.flag, 1,
        (void (*)(void *)) wait_send, &wait_send_arg);
  }
  /* We inc the number of pending requests */
  sctk_ib_qp_inc_requests_nb(remote);
}


/* Send a message with locks */
  static inline void __send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf)
{
  int rc;
  LOAD_DEVICE(rail_ib);
  sctk_ib_qp_ondemand_t *od = &device->ondemand;

  sctk_nodebug("Send locked message to process %d %p", remote->rank, remote->qp);

  ibuf->remote = remote;

  /* If the route is beeing flushed, we can cancel the QP deconnexion */
  if (sctk_route_get_state(remote->route_table) == state_flushing) {
    sctk_warning("Try to interrupt the QP deconnexion");
    /* FIXME: we do not cancel a deconnexion for the moment */
    //    sctk_ib_qp_set_deco_canceled(remote, ACK_CANCEL);
  }

  /* We avoid the send of new messages while deconnecting */
  sctk_spinlock_read_lock(&remote->lock_send);

  /* Check the state of a QP */
  if (sctk_route_get_state(remote->route_table) != state_connected ) {
    sctk_route_table_t* new_route;

    sctk_warning("QP deconnected. Recomputing route");
    new_route = sctk_get_route_to_process(remote->route_table->key.destination, rail_ib->rail);
    assume(new_route);
    remote->route_table = new_route;

    not_implemented();
  }

  rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
  if( rc != 0) {
    struct wait_send_s wait_send_arg;
    wait_send_arg.flag = 0;
    wait_send_arg.remote = remote;
    wait_send_arg.ibuf = ibuf;

    sctk_ib_nodebug("QP full, waiting for posting message...");
    sctk_thread_wait_for_value_and_poll (&wait_send_arg.flag, 1,
        (void (*)(void *)) wait_send, &wait_send_arg);
  }
  /* We inc the number of pending requests */
  sctk_ib_qp_inc_requests_nb(remote);

  if (remote->ondemand) {
    assume(od->qp_list_ptr);
    sctk_spinlock_lock(&od->lock);
    /* Update the Clock for QP deconnexion. If the bit is set to 1, we change it
     * to 0 and we go to the next QP */
    if (remote->R == 1) {
      remote->R = 0;
      od->qp_list_ptr = od->qp_list_ptr->next;
    }
    sctk_spinlock_unlock(&od->lock);
  }

  sctk_spinlock_read_unlock(&remote->lock_send);
}

  void
sctk_ib_qp_send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf, int is_control_message) {

  if(IBUF_GET_PROTOCOL(ibuf->buffer)  ==  null_protocol) {
    sctk_ib_toolkit_print_backtrace();
    assume(0);
  }

  if (is_control_message) {
    __send_ibuf_nolock(rail_ib, remote, ibuf);
  } else {
    __send_ibuf(rail_ib, remote, ibuf);
  }

  /* We release the buffer if it has been inlined */
  if (ibuf->flag == SEND_INLINE_IBUF_FLAG) {
    sctk_ibuf_release(rail_ib, ibuf);
  }
}
/*-----------------------------------------------------------
 *  Flush messages from a QP. We block the QP to send new messages.
 *  The function waits until no more message has to be sent
 *----------------------------------------------------------*/
struct flush_send_s {
  int flag;
  sctk_ib_qp_t *remote;
};

static void* flush_send(void *arg){
  struct flush_send_s *a = (struct flush_send_s*) arg;

  /* Check if there is no more pending requests */
  if (sctk_ib_qp_get_requests_nb(a->remote) == 0) {
    a->flag = 1;
  }

  /* Check if the flush has been canceled */
  if (a->flag == 0) {
    if (sctk_ib_qp_get_deco_canceled(a->remote) == 1) {
      /* We leave the flush function */
      a->flag = 1;
    }
  }
  return NULL;
}

/*
 * Flush all pending message from the QP.
 * New messages which are not control messages may no more use this QP.
 * We return 1 if ths flush is canceled, 0 if we can proceed to the deconnexion
 */
static inline int
sctk_ib_qp_flush(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote) {

  sctk_nodebug("Flushing messages from QP");
  sctk_route_set_state(remote->route_table, state_flushing);

  /* If all requests have not been flushed */
  if (sctk_ib_qp_get_requests_nb(remote) != 0) {
    struct flush_send_s flush_send_arg;
    flush_send_arg.flag = 0;
    flush_send_arg.remote = remote;

    sctk_thread_wait_for_value_and_poll (&flush_send_arg.flag, 1,
        (void (*)(void *)) flush_send, &flush_send_arg);
  }

  sctk_nodebug("Message flushed from QP");

  return sctk_ib_qp_get_deco_canceled(remote);
}


/*-----------------------------------------------------------
 *  On demand deconnexion
 *----------------------------------------------------------*/
/* Select a victim from the clock algorithm */
void sctk_ib_qp_select_victim(struct sctk_ib_rail_info_s* rail_ib) {
  sctk_ib_qp_t *current_qp;
  LOAD_DEVICE(rail_ib);
  sctk_ib_qp_ondemand_t *od = &device->ondemand;
  int cancel;

  sctk_spinlock_lock(&od->lock);

  current_qp = od->qp_list_ptr;
  if (current_qp == NULL) {
    sctk_warning("There is no QP to deconnect");
    sctk_spinlock_unlock(&od->lock);
    return;
  }

  while(current_qp) {
    if (current_qp->R == 0) {
      /* If the element to remove is the last element */
      if (current_qp->next == current_qp) {
        od->qp_list_ptr = NULL;
      } else {
        od->qp_list_ptr = od->qp_list_ptr->next;
      }
      /* We can deconnect this QP */
      goto exit;
    } else {
      current_qp->R = 0;
    }
    current_qp = current_qp->next;
    od->qp_list_ptr = current_qp;
  }

exit:
  assume(current_qp);
  /* Remove the QP from the list -> cannot be no more deconnected */
  CDL_DELETE(od->qp_list, current_qp);
  sctk_spinlock_unlock(&od->lock);
  sctk_warning("Proceeding to a QP deconnexion: QP to process %d elected %p", current_qp->rank, current_qp);

  /* Send a deconnexion request */
  sctk_ib_cm_deco_request_send(rail_ib, current_qp->route_table);

  /* ---> We block the message sending and wait until all
   * task is out from the send function */
  sctk_spinlock_write_lock(&current_qp->lock_send);
  /* Deconnection lock which is released once the deconnexion
   * finished*/
  current_qp->deco_lock = 1;

  /* Flush all pending requests */
  cancel = sctk_ib_qp_flush(rail_ib, current_qp);
  /* FIXME: Restore deco variable */
  /* sctk_ib_qp_set_deco_canceled(remote, ACK_OK); */

  if (cancel == ACK_CANCEL) {
    not_implemented();
    sctk_route_set_state(current_qp->route_table, state_connected);
    sctk_ib_qp_set_local_flush_ack(current_qp, ACK_CANCEL);
  } else {
    /* Flush OK */
    sctk_ib_qp_set_local_flush_ack(current_qp, ACK_OK);
  }

  /* <--- We release the message sending */
  sctk_spinlock_write_unlock(&current_qp->lock_send);
}

/* CAREFULL: this function may stop the polling function from polling */
void sctk_ib_qp_deco_victim(struct sctk_ib_rail_info_s* rail_ib,
    sctk_route_table_t* route_table) {
  sctk_ib_qp_t *remote = route_table->data.ib.remote;
  int cancel;

  /* ---> We block the message sending and wait until all
   * task is out from the send function */
  sctk_spinlock_write_lock(&remote->lock_send);
  remote->deco_lock = 1;

  /* Flush all pending requests */
  cancel = sctk_ib_qp_flush(rail_ib,remote);
  /* FIXME: Restore deco variable */
  /* sctk_ib_qp_set_deco_canceled(remote, ACK_OK); */

  /* Restore QP and cancel deconnexion */
  if (cancel == ACK_CANCEL) {
    not_implemented();
    sctk_route_set_state(remote->route_table, state_connected);
    sctk_ib_qp_set_local_flush_ack(remote, ACK_CANCEL);
  } else {
    /* Flush OK */
    sctk_ib_qp_set_local_flush_ack(remote, ACK_OK);
  }

  /* Send the deco ack to the remote */
  sctk_ib_cm_deco_ack_send(rail_ib->rail, route_table, cancel);

  /* <--- We release the message sending */
  sctk_spinlock_write_unlock(&remote->lock_send);
}

#endif
