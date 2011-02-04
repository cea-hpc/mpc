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

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "sctk_list.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_const.h"
#include "sctk_bootstrap.h"
#include "sctk.h"
#include "sctk_thread.h"
#include "sctk_config.h"

sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;

  /* FIXME: change to HashTables */
  size_t size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);

  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t));

  sctk_net_ibv_allocator->entry = sctk_malloc(size);
  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_sr_lock, NULL);
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_rdma_lock, NULL);
  }

  /* init pending queues */
  sctk_net_ibv_allocator_pending_init(IBV_CHAN_RC_SR);
  sctk_net_ibv_allocator_pending_init(IBV_CHAN_RC_RDMA);
  return sctk_net_ibv_allocator;
}

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      if (sctk_net_ibv_allocator->entry[rank].rc_sr)
      {
        sctk_error("Process %d already registered to sctk_net_ibv_allocator RC_SR", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_sr = entry;
      break;

    case IBV_CHAN_RC_RDMA:
      if (sctk_net_ibv_allocator->entry[rank].rc_rdma)
      {
        sctk_error("Process %d already registered to chan RC_RDMA", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_rdma = entry;
      sctk_nodebug("Registering %p", &sctk_net_ibv_allocator->entry[rank]);
      break;
  }
}

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;
  }
}

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;
  }
}

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      return sctk_net_ibv_allocator->entry[rank].rc_sr;
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_nodebug("Geting %p", &sctk_net_ibv_allocator->entry[rank]);
      sctk_nodebug("Allocator get : %p", sctk_net_ibv_allocator->entry[rank].rc_rdma);
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;
  }
  return NULL;
}


/*-----------------------------------------------------------
 *  PENGING MESSAGES
 *----------------------------------------------------------*/
void
sctk_net_ibv_allocator_pending_init(
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_list_new(&rc_sr_pending, 0);
      return;
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_list_new(&rc_rdma_pending, 0);
      return;
      break;
  }
}

void
sctk_net_ibv_allocator_pending_push(
    void* ptr,
    size_t size,
    int allocation_needed,
    sctk_net_ibv_allocator_type_t type)
{
  void* msg;

  if (allocation_needed)
  {
    msg = sctk_malloc(size);
    memcpy(msg, ptr, size);
  } else {
    msg = ptr;
  }

  switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_nodebug("New Message RC_SR pushed %p", msg);
      sctk_list_lock(&rc_sr_pending);
      sctk_list_push(&rc_sr_pending, msg);
      sctk_list_unlock(&rc_sr_pending);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_nodebug("New Message RC_RDMA pushed ");
      sctk_list_lock(&rc_rdma_pending);
      sctk_list_push(
          &rc_rdma_pending, msg);
      sctk_list_unlock(&rc_rdma_pending);
      break;
  }
}

/*-----------------------------------------------------------
 *  RC_SR
 *----------------------------------------------------------*/

  void
sctk_net_ibv_allocator_rc_sr_buffers_init(sctk_net_ibv_qp_rail_t* rail)
{
  int i;

  rc_sr_local = sctk_net_ibv_comp_rc_sr_create_local(rail);

  /* recv buffers */
  rc_sr_recv_buff = sctk_net_ibv_comp_rc_sr_new( IBV_PENDING_RECV,  SCTK_EAGER_THRESHOLD);
  sctk_net_ibv_comp_rc_sr_init(rail, rc_sr_local, rc_sr_recv_buff, RC_SR_WR_RECV);
  for (i = 0; i < IBV_PENDING_RECV; ++i)  {
    sctk_net_ibv_comp_rc_sr_post_recv(rc_sr_local, rc_sr_recv_buff, i);
  }
  sctk_nodebug("WR RECV %d<->%d", rc_sr_recv_buff->wr_begin, rc_sr_recv_buff->wr_end);

  /* ptp send buffers */
  rc_sr_ptp_send_buff = sctk_net_ibv_comp_rc_sr_new( IBV_PENDING_SEND_PTP, SCTK_EAGER_THRESHOLD);
  sctk_net_ibv_comp_rc_sr_init(rail, rc_sr_local, rc_sr_ptp_send_buff, RC_SR_WR_SEND);
  sctk_nodebug("WR PTP SEND %d<->%d", rc_sr_ptp_send_buff->wr_begin, rc_sr_ptp_send_buff->wr_end);

  /* coll send buffers */
  rc_sr_coll_send_buff = sctk_net_ibv_comp_rc_sr_new( IBV_PENDING_SEND_COLL, SCTK_EAGER_THRESHOLD);
  sctk_net_ibv_comp_rc_sr_init(rail, rc_sr_local, rc_sr_coll_send_buff, RC_SR_WR_SEND);
  sctk_nodebug("WR COLL SEND %d<->%d", rc_sr_coll_send_buff->wr_begin, rc_sr_coll_send_buff->wr_end);
}

  sctk_net_ibv_rc_sr_process_t*
sctk_net_ibv_alloc_rc_sr_find_from_qp_num(uint32_t qp_num)
{
  sctk_net_ibv_rc_sr_process_t          *entry = NULL;

  int i;
  for(i=0; i < sctk_process_number; ++i)
  {
    entry = sctk_net_ibv_allocator_get(i, IBV_CHAN_RC_SR);

    if (entry
        && (entry->qp->qp_num == qp_num))
    {
      sctk_nodebug("FOUND process %d", entry->rank);
      return entry;
    }
  }
  return NULL;
}


/*-----------------------------------------------------------
 *  RDMA
 *----------------------------------------------------------*/
void
sctk_net_ibv_allocator_rc_rdma_process_next_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma)
{
  sctk_net_ibv_rc_rdma_request_ack_t* ack;
  sctk_net_ibv_qp_remote_t* remote_rc_sr;
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_entry_recv_t *last_entry_recv = NULL;


  sctk_net_ibv_allocator_lock(entry_rc_rdma->remote.rank, IBV_CHAN_RC_RDMA);
  last_entry_recv = sctk_net_ibv_comp_rc_rdma_check_pending_request(entry_rc_rdma);

  /* if we have pop an element */
  if ( last_entry_recv )
  {
    last_entry_recv->used = 1;
    sctk_net_ibv_allocator_unlock(entry_rc_rdma->remote.rank, IBV_CHAN_RC_RDMA);

    sctk_nodebug("there is one message to perform (dest %d PSN %d)!",
        last_entry_recv->src_process, last_entry_recv->psn);

    sctk_nodebug("Element to pop");
    entry = sctk_net_ibv_comp_rc_sr_pick_header(rc_sr_ptp_send_buff);
    ack = (sctk_net_ibv_rc_rdma_request_ack_t* ) &(entry->msg_header->payload);

    //TODO reorder post_recv and prepate_ack

    /* post buffer and register ptr*/
    sctk_net_ibv_comp_rc_rdma_post_recv(
        rail,
        entry_rc_rdma,
        rc_rdma_local, last_entry_recv,
        &(ack->dest_ptr), &(ack->dest_rkey) );


    sctk_net_ibv_comp_rc_rdma_prepare_ack(rail, entry_rc_rdma, ack,
        last_entry_recv);

    /* send msg RC_SR_RDVZ_ACK */
    remote_rc_sr = sctk_net_ibv_allocator_get(last_entry_recv->src_process, IBV_CHAN_RC_SR);

    sctk_net_ibv_comp_rc_sr_send(remote_rc_sr, entry, sizeof(sctk_thread_ptp_message_t), RC_SR_RDVZ_ACK, NULL);
  }

  sctk_net_ibv_allocator_unlock(entry_rc_rdma->remote.rank, IBV_CHAN_RC_RDMA);
}


/*-----------------------------------------------------------
 *  SEARCH FUNCTIONS
 *----------------------------------------------------------*/

  sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(uint32_t qp_num)
{
  sctk_net_ibv_rc_rdma_process_t          *entry_rdma = NULL;

  int i;
  for(i=0; i < sctk_process_number; ++i)
  {
    entry_rdma = sctk_net_ibv_allocator_get(i, IBV_CHAN_RC_RDMA);

    if (entry_rdma
        && (entry_rdma->remote.qp->qp_num == qp_num))
    {
      sctk_nodebug("FOUND process %d", entry_rdma->remote.rank);
      return entry_rdma;
    }
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_ptp_send_buff,
      rc_sr_coll_send_buff);
}

void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_sr_recv_buff, rc_rdma_local, lookup, dest);
}


/*-----------------------------------------------------------
 *  POLLING RC RDMA
 *----------------------------------------------------------*/

void sctk_net_ibv_rc_rdma_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_rdma_poll_recv();
}

void sctk_net_ibv_rc_rdma_send_cq(struct ibv_wc* wc, int lookup, int dest)
{

  sctk_net_ibv_rc_rdma_poll_send(wc,
      rail, rc_sr_local,
      rc_sr_ptp_send_buff);
}


/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
void sctk_net_ibv_allocator_ptp_poll_all()
{
  sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_SR);
  sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_RDMA);

  sctk_net_ibv_sched_poll_pending();
}

void sctk_net_ibv_allocator_ptp_poll(sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, SCTK_PENDING_IN_NUMBER, sctk_net_ibv_rc_sr_recv_cq);
      sctk_net_ibv_cq_poll(rc_sr_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_sr_send_cq);
      break;

    case IBV_CHAN_RC_RDMA :
      sctk_net_ibv_cq_poll(rc_rdma_local->recv_cq, SCTK_PENDING_IN_NUMBER, sctk_net_ibv_rc_rdma_recv_cq);
      sctk_net_ibv_cq_poll(rc_rdma_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_rdma_send_cq);
      break;
  }
}

int sctk_net_ibv_allocator_ptp_lookup(int dest, sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_lookup(rc_sr_local->recv_cq, sctk_net_ibv_rc_sr_recv_cq, dest);
      sctk_net_ibv_cq_lookup(rc_sr_local->send_cq, sctk_net_ibv_rc_sr_send_cq, dest);
      break;

    case IBV_CHAN_RC_RDMA :
      sctk_net_ibv_cq_lookup(rc_rdma_local->recv_cq, sctk_net_ibv_rc_rdma_recv_cq, dest);
      sctk_net_ibv_cq_lookup(rc_rdma_local->send_cq, sctk_net_ibv_rc_rdma_send_cq, dest);
      break;
  }
}

int sctk_net_ibv_allocator_ptp_lookup_all(int dest)
{
  int ret;

  /* poll pending messages */
  sctk_net_ibv_sched_poll_pending();

  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_SR);
  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_RDMA);
}


/* * * * * * * * * * * * * * *
 * TCP SERVER
 * * * * * * * * * * * * * * */

static int sockfd;
static int port;
static char hostname[256];

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
    int rank;
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
      remote->is_connected = 1;
      sctk_nodebug("2 Client %d CONNECTED", src);
      keys = sctk_net_ibv_qp_exchange_convert(msg);
      sctk_net_ibv_qp_allocate_recv(remote, &keys);
    } else {
      sctk_nodebug("2 Client %d already CONNECTED", src);
    }

    sctk_net_ibv_allocator_unlock(src, IBV_CHAN_RC_SR);

    snprintf(msg, 256, "%05d:%010d:%010d", rail->lid, remote->qp->qp_num,
        remote->psn);
    write(fd, msg, 256);

    close(fd);
  }
}

int sctk_net_ibv_tcp_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote)
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

  sprintf(name,"%s-ib0",host);
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
    remote->is_connected = 1;
    sctk_nodebug("1 Client %d CONNECTED", dest);
    keys = sctk_net_ibv_qp_exchange_convert(msg);
    sctk_net_ibv_qp_allocate_recv(remote, &keys);
  } else {
    sctk_nodebug("1 Client %d already CONNECTED", dest);
  }
  sctk_net_ibv_allocator_unlock(dest, IBV_CHAN_RC_SR);

  sctk_nodebug ("Try connection to %s on port %d done", name, port);
  return clientsock_fd;
}

  char*
sctk_net_ibv_tcp_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port)
{
  char key[256];
  char val[256];
  int key_max;
  char* kvsname;
  int res;

  key_max = sctk_bootstrap_get_max_key_len();

  snprintf(key, key_max, "tcp.%d", process);
  sctk_nodebug("GET key %s", key);
  sctk_bootstrap_get(key, val, key_max);
  //  res = PMI_KVS_Get(kvsname, key, val, val_max);
//  assume(res == PMI_SUCCESS);
  sctk_nodebug("Got for process %d : %s", process, val);

  //FIXME
  sscanf(val, "%d:%s",port, host);
  sctk_nodebug("Hostname %s, port %d", host, *port);
}

void sctk_net_ibv_tcp_server()
{
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;
  char msg[256];
  char key[256];
  int key_max;
  int val_max;
  int res;

  key_max = sctk_bootstrap_get_max_key_len();
  val_max = sctk_bootstrap_get_max_val_len();

  sockfd = sctk_create_recv_socket();

  /* save server infos to PMI */
  snprintf(key, key_max, "tcp.%d", sctk_process_rank);
  snprintf(msg, val_max, "%d:%s", port, hostname);
  sctk_nodebug("Send to PMI %s with val %s. kvsname %s", msg, key, kvsname);

  sctk_bootstrap_register(key, msg, val_max);

//  res = PMI_KVS_Put(kvsname,key,msg);
//  assume(res == PMI_SUCCESS);

//  res = PMI_KVS_Commit(kvsname);
//  assume(res == PMI_SUCCESS);

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, server, NULL);
}
