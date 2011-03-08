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
#include <slurm/pmi.h>

#include "sctk_bootstrap.h"

#include "sctk_infiniband_cm.h"
#include "sctk_infiniband_qp.h"
#include "sctk_infiniband_config.h"

#ifdef MPC_USE_INFINIBAND

#include "sctk_rpc.h"
#include <infiniband/verbs.h>
#include "sctk_alloc.h"
#include "sctk_iso_alloc.h"
#include "sctk_net_tools.h"

static int sockfd;
static int port;
static char hostname[256];

extern  sctk_net_ibv_qp_local_t *rc_sr_local;

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
    read(fd, &src, sizeof(int));

    read(fd, msg, 256);
    sctk_nodebug("Msg received from %d %s", src, msg);

    sctk_net_ibv_allocator_lock(src, IBV_CHAN_RC_SR);
    sctk_nodebug("Lock obtained 1");
    remote = sctk_net_ibv_allocator_get(src, IBV_CHAN_RC_SR);
    if (!remote)
    {
      remote = sctk_net_ibv_comp_rc_sr_allocate_init(src, rc_sr_local);
      sctk_net_ibv_allocator_register(src, remote, IBV_CHAN_RC_SR);
    }
    assume(remote);

    if (remote->is_connected == 0)
    {
      sctk_nodebug("2 Client %d CONNECTED", src);
      keys = sctk_net_ibv_qp_exchange_convert(msg);
      sctk_net_ibv_qp_allocate_recv(remote, &keys);
      remote->is_connected = 1;
    } else {
      sctk_nodebug("2 Client %d already CONNECTED", src);
    }

    sctk_net_ibv_allocator_unlock(src, IBV_CHAN_RC_SR);

    snprintf(msg, 256, "%05d:%010d:%010d", rail->lid, remote->qp->qp_num,
        remote->psn);
    write(fd, msg, 256);

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

//  sprintf(name,"%s-ib0",host);
  sprintf(name,"%s",host);
  sctk_nodebug("Connect to %s",name);

  server = gethostbyname (name);
  if (server == NULL)
  {
    fprintf (stderr, "ERROR, no such host\n");
    exit (0);
  }
  ip = (char *) server->h_addr;

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  memmove ((char *) &serv_addr.sin_addr.s_addr,
      ip, server->h_length);
  serv_addr.sin_port = htons ((unsigned short) port);
  if (connect
      (clientsock_fd, (struct sockaddr *) (&serv_addr),
       sizeof (serv_addr)) < 0)
  {
    perror ("ERROR connecting");
    abort ();
  }

  write(clientsock_fd, &sctk_process_rank, sizeof(int));

  snprintf(msg, 256, "%05d:%010d:%010d", rail->lid, remote->qp->qp_num, remote->psn);
  write(clientsock_fd, msg, 256);

  read(clientsock_fd, msg, 256);

  sctk_nodebug("msg received : %s", msg);

  sctk_net_ibv_allocator_lock(dest, IBV_CHAN_RC_SR);
  if (remote->is_connected == 0) {
    sctk_nodebug("1 Client %d CONNECTED", dest);
    keys = sctk_net_ibv_qp_exchange_convert(msg);
    sctk_net_ibv_qp_allocate_recv(remote, &keys);
    remote->is_connected = 1;
  } else {
    sctk_nodebug("1 Client %d already CONNECTED", dest);
  }
  sctk_net_ibv_allocator_unlock(dest, IBV_CHAN_RC_SR);

  sctk_nodebug ("Try connection to %s on port %d done", name, port);
}

  void
sctk_net_ibv_cm_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port)
{
  char key[256];
  char val[256];
  int key_max;

  key_max = sctk_bootstrap_get_max_key_len();

  snprintf(key, key_max, "tcp.%d", process);
  sctk_nodebug("GET key %s", key);
  sctk_bootstrap_get(key, val, key_max);
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

  key_max = sctk_bootstrap_get_max_key_len();
  val_max = sctk_bootstrap_get_max_val_len();

  sockfd = sctk_create_recv_socket();

  /* save server infos to PMI */
  snprintf(key, key_max, "tcp.%d", sctk_process_rank);
  snprintf(msg, val_max, "%d:%s", port, hostname);
  sctk_nodebug("Send to PMI %s with val %s. kvsname %s", msg, key, kvsname);

  sctk_bootstrap_register(key, msg, val_max);

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, server, NULL);
}

/*-----------------------------------------------------------
 *  ASYNC EVENTS THREAD
 *----------------------------------------------------------*/
#define DESC_EVENT(desc)  \
  if (ibv_verbose_level > 0) sctk_debug("[async thread] "desc)

void* async_thread(void* context)
{
  struct ibv_async_event event;

  while(1)
  {
    if(ibv_get_async_event((struct ibv_context*) context, &event))
    {
      sctk_error("[async thread] cannot get event");
    }

    if (ibv_verbose_level > 0)
      sctk_debug("[async thread] got event %d", event.event_type);

    switch (event.event_type)
    {
      case IBV_EVENT_CQ_ERR:
        break;

      case IBV_EVENT_QP_FATAL:
        break;

      case	IBV_EVENT_QP_REQ_ERR:
        break;

      case	IBV_EVENT_QP_ACCESS_ERR:
        break;

	    case IBV_EVENT_COMM_EST:
        break;

      case IBV_EVENT_SQ_DRAINED:
        break;

      case IBV_EVENT_PATH_MIG:
        break;

      case IBV_EVENT_PATH_MIG_ERR:
        break;

      case	IBV_EVENT_DEVICE_FATAL:
        break;

      case IBV_EVENT_PORT_ACTIVE:
        break;

      case IBV_EVENT_PORT_ERR:
        break;

      case	IBV_EVENT_LID_CHANGE:
        break;

      case IBV_EVENT_PKEY_CHANGE:
        break;

      case IBV_EVENT_SM_CHANGE:
        break;

      case IBV_EVENT_SRQ_ERR:
        break;

      case IBV_EVENT_SRQ_LIMIT_REACHED:
        DESC_EVENT("SRQ limit was reached");
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local);
        break;

      case IBV_EVENT_QP_LAST_WQE_REACHED:
        DESC_EVENT("Last WQE Reached on a QP associated with an SRQ CQ events");
        break;

      case IBV_EVENT_CLIENT_REREGISTER:
        DESC_EVENT("SM sent a CLIENT_REREGISTER request to a port CA events");
        break;
    }

    ibv_ack_async_event(&event);
  }
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
