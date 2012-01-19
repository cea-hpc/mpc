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

#include "sctk_ib_cm.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_config.h"


#include "sctk_rpc.h"
#include <infiniband/verbs.h>
#include "sctk_alloc.h"
#include "sctk_pmi.h"
#include "sctk_iso_alloc.h"
#include "sctk_net_tools.h"
#include "sctk_io_helper.h"

static int sockfd;
static int port;
static char hostname[256];

extern  sctk_net_ibv_qp_local_t *rc_sr_local;
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

static int
sctk_create_recv_socket ()
{
  int portno;
  struct sockaddr_in serv_addr;
  int fd;


  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    sctk_error ("ERROR opening socket");

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));

  portno = 1023;
  sctk_nodebug ("Looking for an available port\n");
  do
    {
      portno++;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons ((unsigned short) portno);

    }
  while (bind (fd, (struct sockaddr *) &serv_addr,
	       sizeof (serv_addr)) < 0);
  sctk_nodebug ("Looking for an available port found %d\n", portno);
  port = portno;
  gethostname (hostname, 256);

  sctk_nodebug("TCP server is running on port %d", portno);
  listen (fd, 40);

  return fd;
}

static void* server(void* arg)
{
  int fd;
  unsigned int clilen;
  struct sockaddr_in cli_addr;
  char msg[256];
  sctk_net_ibv_qp_remote_t *remote;
  sctk_net_ibv_qp_exchange_keys_t keys;
  int src;

  clilen = sizeof (cli_addr);

  sctk_nodebug("NEW SERVER INIT");

  while (1)
  {
    fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (fd < 0)
    {
      perror ("accept");
    }
    assert (fd >= 0);

    /* receive RTR */
    sctk_safe_checked_read(fd, &src, sizeof(int));
    sctk_safe_checked_read(fd, msg, 256);
    sctk_nodebug("Msg received from %d %s", src, msg);

    ALLOCATOR_LOCK(src, IBV_CHAN_RC_SR);
    remote = sctk_net_ibv_allocator_get(src, IBV_CHAN_RC_SR);
    if (!remote)
    {
      remote = sctk_net_ibv_comp_rc_sr_allocate_init(src, rc_sr_local);
      sctk_net_ibv_allocator_register(src, remote, IBV_CHAN_RC_SR);
    }
    ALLOCATOR_UNLOCK(src, IBV_CHAN_RC_SR);
    assume(remote);

    /* send RTR */
    if (remote->is_rtr == 0) {
      ALLOCATOR_LOCK(src, IBV_CHAN_RC_SR);
      if (remote->is_rtr == 0) {
        keys = sctk_net_ibv_qp_exchange_convert(msg);
        sctk_net_ibv_qp_allocate_rtr(remote, &keys);
        remote->is_rtr = 1;
      }
      ALLOCATOR_UNLOCK(src, IBV_CHAN_RC_SR);
    }

    snprintf(msg, 256,"%05"SCNu16":%010"SCNu32":%010"SCNu32, rail->lid, remote->qp->qp_num,
        remote->psn);
    sctk_safe_checked_write(fd, msg, 256);

    /* send RTR */
    int done;
    sctk_safe_checked_read(fd, &done, sizeof(int));
    if (remote->is_rts == 0) {
      ALLOCATOR_LOCK(src, IBV_CHAN_RC_SR);
      if (remote->is_rts == 0) {
        sctk_net_ibv_qp_allocate_rts(remote);
        remote->is_rtr = 1;
      }
      ALLOCATOR_UNLOCK(src, IBV_CHAN_RC_SR);
    }

    if (remote->is_connected == 0) {
      ALLOCATOR_LOCK(src, IBV_CHAN_RC_SR);
      if (remote->is_connected == 0) {
        remote->is_connected = 1;
        sctk_nodebug("2 Client %d CONNECTED", src);
      }
      ALLOCATOR_UNLOCK(src, IBV_CHAN_RC_SR);
    }

    close(fd);
  }
  return NULL;
}

void sctk_net_ibv_cm_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote)
{
  int clientsock_fd;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char* ip;
  char name[4096];
  char msg[256];
  sctk_net_ibv_qp_exchange_keys_t keys;

  sctk_nodebug ("Try connection to %s(dest %d) on port %d type %d", host, dest, port, AF_INET);

  clientsock_fd = socket (AF_INET, SOCK_STREAM, 0);
  if (clientsock_fd < 0)
    sctk_error ("ERROR opening socket");

#warning "Add a policy to choose the right hostname"
  //  sprintf(name,"%s-ib0",host);
  sprintf(name,"%s",host);
  sctk_nodebug("Connect to %s",name);

  int i = 0;
  do {
    server = gethostbyname (name);
    if (server == NULL)
    {
      fprintf (stderr, "ERROR, no such host\n");
      sctk_abort();
      exit (0);
    }
    ip = (char *) server->h_addr;

    if (!ip)
      usleep(500);

    ++i;
  } while (!ip && i < 20);
  assume(ip);

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  memmove ((char *) &serv_addr.sin_addr.s_addr,
      ip, server->h_length);
  serv_addr.sin_port = htons ((unsigned short) port);
  if (connect
      (clientsock_fd, (struct sockaddr *) (&serv_addr),
       sizeof (serv_addr)) < 0)
  {
    perror ("RETRY ERROR connecting (IB cm)");
    if (connect
	(clientsock_fd, (struct sockaddr *) (&serv_addr),
	 sizeof (serv_addr)) < 0)
      {
	perror ("ERROR connecting (IB cm)");
	abort ();
      }
  }

  /* send REQ */
  sctk_safe_checked_write(clientsock_fd, &sctk_process_rank, sizeof(int));
  snprintf(msg, 256, "%05d:%010d:%010d", rail->lid, remote->qp->qp_num, remote->psn);
  sctk_safe_checked_write(clientsock_fd, msg, 256);

  /* receive REP */
  sctk_safe_checked_read(clientsock_fd, msg, 256);
  if (remote->is_rtr == 0) {
    ALLOCATOR_LOCK(dest, IBV_CHAN_RC_SR);
    if (remote->is_rtr == 0) {
      keys = sctk_net_ibv_qp_exchange_convert(msg);
      sctk_net_ibv_qp_allocate_rtr(remote, &keys);
      remote->is_rtr = 1;
    }
    ALLOCATOR_UNLOCK(dest, IBV_CHAN_RC_SR);
  }

  if (remote->is_rts == 0) {
    ALLOCATOR_LOCK(dest, IBV_CHAN_RC_SR);
    if (remote->is_rts == 0) {
      sctk_net_ibv_qp_allocate_rts(remote);
      remote->is_rts = 1;
    }
    ALLOCATOR_UNLOCK(dest, IBV_CHAN_RC_SR);
  }

  if (remote->is_connected == 0) {
    ALLOCATOR_LOCK(dest, IBV_CHAN_RC_SR);
    if (remote->is_connected == 0) {
      remote->is_connected = 1;
      sctk_nodebug("2 Client %d CONNECTED", dest);
    }
    ALLOCATOR_UNLOCK(dest, IBV_CHAN_RC_SR);
  }

  /* send DONE */
  int done = 1;
  sctk_safe_checked_write(clientsock_fd, &done, sizeof(int));

  sctk_nodebug ("Try connection to %s on port %d done", name, port);
}

  void
sctk_net_ibv_cm_request(int process, char* host, int* port)
{
  char key[256];
  char val[256];
  int key_max;

  key_max = sctk_pmi_get_max_key_len();

  snprintf(key, key_max, "tcp.%d", process);
  sctk_nodebug("GET key %s", key);
  sctk_pmi_get_connection_info_str(val, key_max, key);
  sctk_nodebug("Got for process %d : %s", process, val);

  //FIXME
  sscanf(val, "%d:%s",port, host);
  sctk_nodebug("Hostname %s, port %d", host, *port);
}

void sctk_net_ibv_cm_server()
{
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;
  char msg[256];
  char key[256];
  int key_max;
  int val_max;

  key_max = sctk_pmi_get_max_key_len();
  val_max = sctk_pmi_get_max_val_len();

  sockfd = sctk_create_recv_socket();

  /* save server infos to PMI */
  snprintf(key, key_max, "tcp.%d", sctk_process_rank);
  snprintf(msg, val_max, "%d:%s", port, hostname);
  sctk_nodebug("Send to PMI %s with val %s. kvsname %s", msg, key, kvsname);

  sctk_pmi_put_connection_info_str(msg, val_max, key);

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, server, NULL);
}

/*-----------------------------------------------------------
 *  ASYNC EVENTS THREAD
 *----------------------------------------------------------*/
#define DESC_EVENT(event, desc, fatal)  \
  if ( (ibv_verbose_level > 0) || fatal) PRINT_DEBUG(1, "[async thread] "event":\t"desc); \
if (fatal) sctk_abort()

void* async_thread(void* context)
{
  struct ibv_async_event event;
  struct ibv_srq_attr mod_attr;
  int rc;

  while(1)
  {
    if(ibv_get_async_event((struct ibv_context*) context, &event))
    {
      sctk_error("[async thread] cannot get event");
    }

    switch (event.event_type)
    {
      case IBV_EVENT_CQ_ERR:
        DESC_EVENT("IBV_EVENT_CQ_ERR", "CQ is in error (CQ overrun)", 1);
        break;

      case IBV_EVENT_QP_FATAL:
        DESC_EVENT("IBV_EVENT_QP_FATAL", "Error occurred on a QP and it transitioned to error state", 1);
        break;

      case	IBV_EVENT_QP_REQ_ERR:
        DESC_EVENT("IBV_EVENT_QP_REQ_ERR", "Invalid Request Local Work Queue Error", 1);
        break;

      case	IBV_EVENT_QP_ACCESS_ERR:
        DESC_EVENT("IBV_EVENT_QP_ACCESS_ERR", "Local access violation error", 1);
        break;

      case IBV_EVENT_COMM_EST:
        DESC_EVENT("IBV_EVENT_COMM_EST", "Communication was established on a QP", 0);
        break;

      case IBV_EVENT_SQ_DRAINED:
        DESC_EVENT("IBV_EVENT_SQ_DRAINED", "Send Queue was drained of outstanding messages in progress", 0);
        break;

      case IBV_EVENT_PATH_MIG:
        DESC_EVENT("IBV_EVENT_PATH_MIG", "A connection failed to migrate to the alternate path", 0);
        break;

      case IBV_EVENT_PATH_MIG_ERR:
        DESC_EVENT("IBV_EVENT_PATH_MIG_ERR", "A connection failed to migrate to the alternate path", 0);
        break;

      case	IBV_EVENT_DEVICE_FATAL:
        DESC_EVENT("IBV_EVENT_DEVICE_FATAL", "Error occurred on a QP and it transitioned to error state", 1);
        break;

      case IBV_EVENT_PORT_ACTIVE:
        DESC_EVENT("IBV_EVENT_PORT_ACTIVE", "Link became active on a port", 0);
        break;

      case IBV_EVENT_PORT_ERR:
        DESC_EVENT("IBV_EVENT_PORT_ERR", "Link became unavailable on a port ", 1);
        break;

      case	IBV_EVENT_LID_CHANGE:
        DESC_EVENT("IBV_EVENT_LID_CHANGE", "LID was changed on a port", 0);
        break;

      case IBV_EVENT_PKEY_CHANGE:
        DESC_EVENT("IBV_EVENT_PKEY_CHANGE", "P_Key table was changed on a port", 0);
        break;

      case IBV_EVENT_SM_CHANGE:
        DESC_EVENT("IBV_EVENT_SM_CHANGE", "SM was changed on a port", 0);
        break;

      case IBV_EVENT_SRQ_ERR:
        DESC_EVENT("IBV_EVENT_SRQ_ERR", "Error occurred on an SRQ", 1);
        break;

      case IBV_EVENT_SRQ_LIMIT_REACHED:
        DESC_EVENT("IBV_EVENT_SRQ_LIMIT_REACHED","SRQ limit was reached", 0);
        /*  -1: force to post new buffers */
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, -1, 1);

        mod_attr.srq_limit  = ibv_srq_credit_thread_limit;
        mod_attr.max_wr     = ibv_max_srq_ibufs;
        rc = ibv_modify_srq(rc_sr_local->srq, &mod_attr, IBV_SRQ_LIMIT);
        assume(rc == 0);
        break;

      case IBV_EVENT_QP_LAST_WQE_REACHED:
        DESC_EVENT("IBV_EVENT_QP_LAST_WQE_REACHED","Last WQE Reached on a QP associated with an SRQ CQ events", 1);
        break;

      case IBV_EVENT_CLIENT_REREGISTER:
        DESC_EVENT("IBV_EVENT_CLIENT_REREGISTER","SM sent a CLIENT_REREGISTER request to a port CA events", 0);
        break;

      default:
        DESC_EVENT("UNKNOWN_EVENT","unknown event received", 0);
        break;
    }

    ibv_ack_async_event(&event);
  }
  sctk_nodebug("Async thread exits...");
  return NULL;
}


void sctk_net_ibv_async_init(struct ibv_context *context)
{
  sctk_thread_attr_t attr;
  sctk_thread_t pidt;

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, async_thread, (void*) context);
}


#endif
