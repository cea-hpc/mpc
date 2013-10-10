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
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_polling.h"
#include "sctk_pmi.h"
#include "sctk_asm.h"
#include "utlist.h"
#include "sctk_ib_mpi.h"
#include <sctk_spinlock.h>
#include <errno.h>
#include <string.h>

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "QP"
#include "sctk_ib_toolkit.h"

/* Error handler */
#define SCTK_IB_ABORT_WITH_ERRNO(...)                           \
    sctk_error(__VA_ARGS__"(errno: %s)", strerror(errno));      \
    sctk_abort();

#define SCTK_IB_ABORT(...)        \
    sctk_error(__VA_ARGS__);      \
    sctk_abort();


/*-----------------------------------------------------------
 *  HT of remote peers.
 *  Used to determine the remote which has generated an event
 *  to the CQ of the SRQ.
 *----------------------------------------------------------*/
struct sctk_ib_qp_ht_s {
  int key;
  UT_hash_handle hh;
  struct sctk_ib_qp_s *remote;
};
static sctk_spin_rwlock_t __qp_ht_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

sctk_ib_qp_t*  sctk_ib_qp_ht_find(struct sctk_ib_rail_info_s* rail_ib, int key) {
  struct sctk_ib_qp_ht_s *entry = NULL;

  if  (rail_ib->remotes == NULL) return NULL;

  sctk_spinlock_read_lock(&__qp_ht_lock);
  HASH_FIND_INT(rail_ib->remotes, &key, entry);
  sctk_spinlock_read_unlock(&__qp_ht_lock);

  if (entry != NULL) return entry->remote;

  return NULL;
}

static inline void sctk_ib_qp_ht_add(struct sctk_ib_rail_info_s* rail_ib, struct sctk_ib_qp_s *remote, int key) {
  struct sctk_ib_qp_ht_s *entry = NULL;

#ifdef IB_DEBUG
  struct sctk_ib_qp_s *rem;
  rem = sctk_ib_qp_ht_find(rail_ib, key);
  ib_assume(rem == NULL);
#endif

  entry = sctk_malloc(sizeof(struct sctk_ib_qp_ht_s));
  ib_assume(entry);
  entry->key = key;
  entry->remote = remote;

  sctk_spinlock_write_lock(&__qp_ht_lock);
  HASH_ADD_INT(rail_ib->remotes, key, entry);
  sctk_spinlock_write_unlock(&__qp_ht_lock);
}


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
    SCTK_IB_ABORT("No IB devices found.");
  }
 device = sctk_malloc(sizeof(sctk_ib_device_t));
  ib_assume(device);

  rail_ib->device = device;
  rail_ib->device->dev_list = dev_list;
  rail_ib->device->dev_nb = devices_nb;
  rail_ib->device->eager_rdma_connections = 0;
  /* Specific to ondemand (de)connexions */
  rail_ib->device->ondemand.qp_list = NULL;
  rail_ib->device->ondemand.qp_list_ptr = NULL;
  rail_ib->device->ondemand.lock = SCTK_SPINLOCK_INITIALIZER;
  rail_ib->device->send_comp_channel = NULL;
  rail_ib->device->recv_comp_channel = NULL;

  return device;
}

sctk_ib_device_t *sctk_ib_device_open(struct sctk_ib_rail_info_s* rail_ib, int rail_nb) {
  LOAD_DEVICE(rail_ib);
  struct ibv_device** dev_list = device->dev_list;
  int devices_nb = device->dev_nb;
  int link_width = -1;
  char *link_rate_string = "unknown";
  int link_rate = -1;
  int data_rate = -1;
  int i;


  /* If the rail number is -1, so we try to estimate which IB card is the closest */
  if (rail_nb == -1) {
    for (i=0; i < devices_nb; ++i) {
      int ret;

      ret = sctk_topology_is_ib_device_close_from_cpu(dev_list[i], sctk_get_cpu());
      if (ret != 0)
      {
        rail_nb = i;
        break;
      }
    }

    /* If no rail open, we open the first one */
    if (rail_nb == -1) {
      sctk_error("Cannot determine the closest IB card from the thread allocating the structures. Use the card 0");
      rail_nb = 0;
    }
  }


  if (rail_nb >= devices_nb)
  {
    SCTK_IB_ABORT("Cannot open rail. You asked rail %d on %d", rail_nb, devices_nb);
  }

  sctk_ib_nodebug("Opening rail %d on %d", rail_nb, devices_nb);

  device->context = ibv_open_device (dev_list[rail_nb]);
  if (!device->context)
  {
    SCTK_IB_ABORT_WITH_ERRNO("Cannot get device context.");
  }

  if ( ibv_query_device(device->context, &device->dev_attr) != 0)
  {
    SCTK_IB_ABORT_WITH_ERRNO("Unable to get device attr.");
  }

  if (ibv_query_port(device->context, 1, &device->port_attr) != 0)
  {
    SCTK_IB_ABORT("Unable to get port attr");
  }

  /* Get the link width */
  switch(device->port_attr.active_width) {
    case 1: link_width = 1; break;
    case 2: link_width = 4; break;
    case 4: link_width = 8; break;
    case 8: link_width = 12; break;
  }

  /* Get the link rate */
  switch(device->port_attr.active_speed) {
    case 0x01: link_rate = 2;  link_rate_string = "SDR"; break;
    case 0x02: link_rate = 4;  link_rate_string = "DDR"; break;
    case 0x04: link_rate = 8;  link_rate_string = "QDR"; break;
    case 0x08: link_rate = 14; link_rate_string = "FDR"; break;
    case 0x10: link_rate = 25; link_rate_string = "EDR"; break;
  }
  data_rate = link_width * link_rate;

  rail_ib->device->data_rate = data_rate;
  rail_ib->device->link_width = link_width;
  strcpy(rail_ib->device->link_rate, link_rate_string);

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
    SCTK_IB_ABORT("Cannot get IB protection domain.");
  }
  return device->pd;
}

/*-----------------------------------------------------------
 *  Completion queue
 *----------------------------------------------------------*/
  struct ibv_comp_channel *
sctk_ib_comp_channel_init(sctk_ib_device_t* device) {
  struct ibv_comp_channel * comp_channel;

  comp_channel = ibv_create_comp_channel(device->context);
  if (!comp_channel) {
    SCTK_IB_ABORT_WITH_ERRNO("Cannot create Completion Channel.");
  }
  return comp_channel;
}

  /*
   * Create a completion queue and associate it a completion channel.
   * This argument may be NULL.
   */
  struct ibv_cq*
sctk_ib_cq_init(sctk_ib_device_t* device,
    sctk_ib_config_t *config, struct ibv_comp_channel * comp_channel)
{
  struct ibv_cq *cq;
  cq = ibv_create_cq (device->context, config->ibv_cq_depth, NULL,
      comp_channel, 0);

  if (!cq) {
    SCTK_IB_ABORT_WITH_ERRNO("Cannot create Completion Queue.");
  }

  if (comp_channel != NULL) {
    int ret = ibv_req_notify_cq(cq, 0);
    if (ret != 0) {
      SCTK_IB_ABORT_WITH_ERRNO("Couldn't request CQ notification");
    }
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
void sctk_ib_qp_key_print(sctk_ib_cm_qp_connection_t *keys) {
 sctk_nodebug(
     "LID=%lu psn=%lu qp_num=%lu",
     keys->lid,
     keys->psn,
     keys->qp_num);
}

void sctk_ib_qp_key_fill(sctk_ib_cm_qp_connection_t* keys, sctk_ib_qp_t *remote,
    sctk_uint16_t lid, sctk_uint32_t qp_num, sctk_uint32_t psn) {

  keys->lid = lid;
  keys->qp_num = qp_num;
  keys->psn = psn;
}


void sctk_ib_qp_key_create_value(char *msg, size_t size, sctk_ib_cm_qp_connection_t* keys) {
  int ret;

  ret = snprintf(msg, size,
      "%08x:%08x:%08x",
      keys->lid,
      keys->qp_num,
      keys->psn);
  /* We assume the value doest not overflow with the buffer */
  ib_assume(ret < size);

  sctk_ib_qp_key_print(keys);
}

void sctk_ib_qp_key_create_key(char *msg, size_t size, int rail_id, int src, int dest) {
  int ret;
  /* We create the key with the number of the rail */
  ret = snprintf(msg, size,"IB-%02d|%06d:%06d", rail_id, src, dest);
  sctk_nodebug("key: %s", msg);
  /* We assume the value doest not overflow with the buffer */
  ib_assume(ret < size);
}

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert( char* msg)
{
  sctk_ib_cm_qp_connection_t keys;
  sscanf(msg, "%08x:%08x:%08x",
      (unsigned int *)&keys.lid,
      (unsigned int *)&keys.qp_num,
      (unsigned int *)&keys.psn);

  sctk_ib_qp_key_print(&keys);

  return keys;
}

/*-----------------------------------------------------------
 *  Queue Pair Keys: Used for the ring connection
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
  int ret;

  sctk_ib_cm_qp_connection_t qp_keys = {
    .lid = device->port_attr.lid,
    .qp_num = remote->qp->qp_num,
    .psn = remote->psn,
  };

  sctk_ib_qp_key_create_key(key, key_max, rail_ib->rail->rail_number, sctk_process_rank, remote->rank);
  sctk_ib_qp_key_create_value(val, val_max, &qp_keys);
  ret = sctk_pmi_put_connection_info_str(val, val_max, key);
  ib_assume(ret == SCTK_PMI_SUCCESS);
}

  sctk_ib_cm_qp_connection_t
sctk_ib_qp_keys_recv(
    struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, int dest_process)
{
  sctk_ib_cm_qp_connection_t qp_keys;
  int key_max = sctk_pmi_get_max_key_len();
  int val_max = sctk_pmi_get_max_val_len();
  char key[key_max];
  char val[key_max];
  int ret;

  sctk_ib_qp_key_create_key(key, key_max, rail_ib->rail->rail_number, dest_process, sctk_process_rank);
  snprintf(key, key_max,"IB-%02d|%06d:%06d", rail_ib->rail->rail_number, dest_process, sctk_process_rank);
  ret = sctk_pmi_get_connection_info_str(val, val_max, key);
  ib_assume(ret == SCTK_PMI_SUCCESS);
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
  ib_assume(remote);
  memset(remote, 0, sizeof(sctk_ib_qp_t));

  return remote;
}

void
sctk_ib_qp_free(sctk_ib_qp_t* remote) {

  if (!remote) {
    SCTK_IB_ABORT("Trying to free a remote entry which is not initialized");
  }
  /* destroy the QP */
  ibv_destroy_qp(remote->qp);
  /* We do not remove the entry. */
  /* free(remote); */
}

  struct ibv_qp*
sctk_ib_qp_init(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t* remote, struct ibv_qp_init_attr* attr, int rank)
{
  LOAD_DEVICE(rail_ib);

  remote->qp = ibv_create_qp (device->pd, attr);
  PROF_INC(rail_ib->rail, ib_qp_created);
  if (!remote->qp) {
    SCTK_IB_ABORT("Cannot create QP for rank %d", rank);
  }
  sctk_nodebug("QP Initialized for rank %d %p", remote->rank, remote->qp);

  /* Add QP to HT */
  struct sctk_ib_qp_s *rem;
  rem = sctk_ib_qp_ht_find(rail_ib, remote->qp->qp_num);
  if (rem == NULL) {
    sctk_ib_qp_ht_add(rail_ib, remote, remote->qp->qp_num);
  }
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
    sctk_ib_cm_qp_connection_t* keys, int *flags)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_qp_attr attr;
  memset (&attr, 0, sizeof (struct ibv_qp_attr));

  attr.qp_state = IBV_QPS_RTR;
  /* 512 is the recommended value */
  attr.path_mtu = IBV_MTU_2048;
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
    sctk_uint32_t psn, int *flags)
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
    sctk_uint32_t psn, int *flags)
{
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
  sctk_nodebug("Modify QP for remote %p to state %d", remote, attr->qp_state );
  if (ibv_modify_qp(remote->qp, attr, flags) != 0)
  {
    SCTK_IB_ABORT_WITH_ERRNO("Cannot modify Queue Pair");
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
  if (!device->srq)
  {
    SCTK_IB_ABORT_WITH_ERRNO("Cannot create Shared Received Queue");
    sctk_abort();
  }

//  config->ibv_max_srq_ibufs_posted = attr->attr.max_wr;
  sctk_ib_debug("Initializing SRQ with %d entries (max:%d)",
      attr->attr.max_wr, sctk_ib_srq_get_max_srq_wr(rail_ib));
  config->ibv_srq_credit_limit = config->ibv_max_srq_ibufs_posted / 2;

  return device->srq;
}

  struct ibv_srq_init_attr
sctk_ib_srq_init_attr(struct sctk_ib_rail_info_s* rail_ib)
{
  LOAD_CONFIG(rail_ib);
  struct ibv_srq_init_attr attr;

  memset (&attr, 0, sizeof (struct ibv_srq_init_attr));

  attr.attr.srq_limit = config->ibv_srq_credit_thread_limit;
  attr.attr.max_wr = sctk_ib_srq_get_max_srq_wr(rail_ib);
  attr.attr.max_sge = config->ibv_max_sg_rq;

  return attr;
}

TODO("Check the max WR !!")
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

  sctk_nodebug("QP reinited for rank %d", rank);

  remote->route_table = route_table;
  remote->psn = lrand48 () & 0xffffff;
  remote->rank = rank;
  remote->free_nb = config->ibv_qp_tx_depth;
  remote->post_lock = SCTK_SPINLOCK_INITIALIZER;
  /* For buffered eager */
  remote->ib_buffered.entries = NULL;
  remote->ib_buffered.lock = SCTK_SPINLOCK_INITIALIZER;
  remote->unsignaled_counter = 0;
  OPA_store_int(&remote->ib_buffered.number, 0);

  /* Add it to the Cicrular Linked List if the QP is created from
   * an ondemand request */
  if (ondemand) {
    remote->R = 1;
    remote->ondemand = 1;
    sctk_spinlock_lock(&od->lock);
    sctk_nodebug("[%d] Add QP to rank %d %p", rail_ib->rail->rail_number, remote->rank, remote);
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
  sctk_ib_qp_set_pending_data(remote, 0);
  remote->rdma.previous_max_pending_data = 0;
  sctk_ib_qp_set_deco_canceled(remote, ACK_OK);

  sctk_ib_qp_set_local_flush_ack(remote, ACK_UNSET);
  sctk_ib_qp_set_remote_flush_ack(remote, ACK_UNSET);

  remote->lock_rtr = SCTK_SPINLOCK_INITIALIZER;
  remote->lock_rts = SCTK_SPINLOCK_INITIALIZER;
  /* Lock for sending messages */
  remote->lock_send = SCTK_SPINLOCK_INITIALIZER;
  remote->flushing_lock = SCTK_SPINLOCK_INITIALIZER;
  /* RDMA */
  sctk_ibuf_rdma_remote_init(remote);

  attr = sctk_ib_qp_state_init_attr(rail_ib, &flags);
  sctk_ib_qp_modify(remote, &attr, flags);
}

void
sctk_ib_qp_allocate_rtr(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote,sctk_ib_cm_qp_connection_t* keys)
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
  sctk_route_table_t *route_table = remote->route_table;
  int flags;

  attr = sctk_ib_qp_state_reset_attr(rail_ib, remote->psn, &flags);
  sctk_nodebug("Modify QR RESET for rank %d", remote->rank);
  sctk_ib_qp_modify(remote, &attr, flags);

  sctk_ib_qp_allocate_set_rtr(remote, 0);
  sctk_ib_qp_allocate_set_rts(remote, 0);
}


/*-----------------------------------------------------------
 *  Send a message to a remote QP
 *----------------------------------------------------------*/
/*
 * Returns if the counter has to be reset because a signaled message needs to be send
 */
static int check_signaled(struct sctk_ib_rail_info_s* rail_ib, sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf) {
  LOAD_CONFIG(rail_ib);
  char is_signaled = (ibuf->desc.wr.send.send_flags & IBV_SEND_SIGNALED) ? 1 : 0;

  if ( ! is_signaled ) {
    if (remote->unsignaled_counter + 1 > (config->ibv_qp_tx_depth >> 1) ) {
      ibuf->desc.wr.send.send_flags | IBV_SEND_SIGNALED;
      return 1;
    } else return 0;
  } else  return 1;

  return 0;
}

static void inc_signaled(struct sctk_ib_rail_info_s* rail_ib, sctk_ib_qp_t *remote, int need_reset) {
  LOAD_CONFIG(rail_ib);

  if ( need_reset ) {
   remote->unsignaled_counter = 0;
  } else {
   remote->unsignaled_counter ++;
  }
}


struct wait_send_s {
  struct sctk_ib_rail_info_s* rail_ib;
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

  PROF_TIME_START(a->rail_ib->rail, ib_post_send);
  sctk_spinlock_lock(&a->remote->lock_send);
  int need_reset = check_signaled(a->rail_ib, a->remote, a->ibuf);
  rc = ibv_post_send(a->remote->qp, &(a->ibuf->desc.wr.send),
      &(a->ibuf->desc.bad_wr.send));
  if (rc == 0) inc_signaled(a->rail_ib, a->remote, need_reset);
  sctk_spinlock_unlock(&a->remote->lock_send);
  PROF_TIME_END(a->rail_ib->rail, ib_post_send);
  if (rc == 0)
  {
    a->flag = 1;
  } else {
    sctk_notify_idle_message ();
  }
  return NULL;
}

/* Send a message without locks. Messages which are sent are not blocked by locks.
 * This is useful for QP deconnexion */
  static inline void __send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf)
{
  int rc;

  sctk_nodebug("[%d] Send no-lock message to process %d %p %d", rail_ib->rail_nb, remote->rank, remote->qp, rail_ib->rail->rail_number);

  ibuf->remote = remote;

  /* We lock here because ibv_post_send uses mutexes which provide really bad performances.
   * Instead we encapsulate the call between spinlocks */
  PROF_TIME_START(rail_ib->rail, ib_post_send_lock);
  sctk_spinlock_lock(&remote->lock_send);
  PROF_TIME_END(rail_ib->rail, ib_post_send_lock);

  PROF_TIME_START(rail_ib->rail, ib_post_send);
  int need_reset = check_signaled(rail_ib, remote, ibuf);
  rc = ibv_post_send(remote->qp, &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
  PROF_TIME_END(rail_ib->rail, ib_post_send);

  if (rc == 0) inc_signaled(rail_ib, remote, need_reset);
  sctk_spinlock_unlock(&remote->lock_send);
  if( rc != 0) {
    struct wait_send_s wait_send_arg;
    wait_send_arg.flag = 0;
    wait_send_arg.remote = remote;
    wait_send_arg.ibuf = ibuf;
    wait_send_arg.rail_ib = rail_ib;

//#warning "We should remove these sctk_error and use a counter instead"
    sctk_error("[%d] NO LOCK QP full for remote %d, waiting for posting message... (pending: %d)", rail_ib->rail->rail_number,
        remote->rank, sctk_ib_qp_get_pending_data(remote));
    sctk_thread_wait_for_value_and_poll (&wait_send_arg.flag, 1,
        (void (*)(void *)) wait_send, &wait_send_arg);

    sctk_nodebug("[%d] NO LOCK QP message sent to remote %d", rail_ib->rail->rail_number, remote->rank);
  }
  sctk_nodebug("SENT no-lock message to process %d %p", remote->rank, remote->qp);
}

/* Send an ibuf to a remote.
 * is_control_message: if the message is a control message
 */
  int
sctk_ib_qp_send_ibuf(struct sctk_ib_rail_info_s* rail_ib,
    sctk_ib_qp_t *remote, sctk_ibuf_t* ibuf, int is_control_message) {

#ifdef IB_DEBUG
  if(IBUF_GET_PROTOCOL(ibuf->buffer)  ==  null_protocol) {
    sctk_ib_toolkit_print_backtrace();
    not_reachable();
  }
#endif


  /* We inc the number of pending requests */
  sctk_ib_qp_add_pending_data(remote, ibuf);

  __send_ibuf(rail_ib, remote, ibuf);

  /* We release the buffer if it has been inlined & if it
   * do not generate an event once the transmission done */
  if ( ( ibuf->flag == SEND_INLINE_IBUF_FLAG
      || ibuf->flag == RDMA_WRITE_INLINE_IBUF_FLAG)
      && ( ( (ibuf->desc.wr.send.send_flags & IBV_SEND_SIGNALED) == 0) ) ) {

      struct sctk_ib_polling_s poll;
      POLL_INIT(&poll);

      /* Decrease the number of pending requests */
      int current_pending;
      current_pending = sctk_ib_qp_fetch_and_sub_pending_data(remote, ibuf);
      sctk_ibuf_rdma_update_max_pending_data(rail_ib, remote, current_pending);
      sctk_network_poll_send_ibuf(rail_ib->rail, ibuf, 0, &poll);
      return 0;
  }
  return 1;
}

#endif
