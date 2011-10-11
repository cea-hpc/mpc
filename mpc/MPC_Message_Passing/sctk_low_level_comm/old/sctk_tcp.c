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
/* #                                                                      # */
/* ######################################################################## */
#include <stdlib.h>
#include <math.h>
#include <sched.h>
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_rpc.h"
#include "sctk_tcp.h"
#include "sctk_alloc.h"
#include "sctk_iso_alloc.h"
#include "sctk_spinlock.h"

#include "sctk_pmi.h"

#ifdef MPC_USE_TCP
#include "sctk_net_tools.h"
#include "sctk_mpcrun_client.h"

/* #define SCTK_USE_STATIC_ALLOC_ONLY */

#define SCTK_MAX_PROCESS_NUMBER 1024
#define MAX_HOST_SIZE 255
typedef struct
{
  int rank;
  volatile int fd;
  int port;
  char name[255];
  sctk_spinlock_t lock;
} tpc_ident_t;

static tpc_ident_t *idents;
static int sockfd;
static int portno;

static inline void sctk_connection_check(int process);

/**************************************************************************/
/* RDMA                                                                   */
/**************************************************************************/
typedef struct sctk_tcp_rdma_s
{
  void *local_addr;
  void *dist_addr;
  size_t size;
  int process;
  int mode;
  int *local_ack;
  int *dist_ack;
} sctk_tcp_rdma_t;

#define SCTK_TCP_RDMA_READ 0
#define SCTK_TCP_RDMA_WRITE 1

static void
sctk_tcp_rdma_lock (tpc_ident_t * ident)
{
  sctk_spinlock_lock (&(ident->lock));
}

static void
sctk_tcp_rdma_unlock (tpc_ident_t * ident)
{
  sctk_spinlock_unlock (&(ident->lock));
}


static sctk_thread_mutex_t rdma_t_poll_lock = SCTK_THREAD_MUTEX_INITIALIZER;
static volatile sctk_tcp_rdma_t *rdma_t_poll = NULL;
static volatile int rdma_t_poll_nb = 0;

static inline sctk_tcp_rdma_t *
sctk_tcp_rdma_get ()
{
  sctk_tcp_rdma_t *tmp;
  sctk_thread_mutex_lock (&rdma_t_poll_lock);
  if (rdma_t_poll == NULL)
    {
      tmp = sctk_malloc (sizeof (sctk_tcp_rdma_t));
    }
  else
    {
      tmp = (sctk_tcp_rdma_t *) rdma_t_poll;
      rdma_t_poll = (sctk_tcp_rdma_t *) tmp->local_addr;
      rdma_t_poll_nb--;
    }
  sctk_thread_mutex_unlock (&rdma_t_poll_lock);
  return tmp;
}

static inline void
sctk_tcp_rdma_free (sctk_tcp_rdma_t * tmp)
{
  if (tmp->local_ack != NULL)
    {
      *tmp->local_ack = 1;
    }

  if (rdma_t_poll_nb >= 10)
    {
      sctk_free (tmp);
    }
  else
    {
      sctk_thread_mutex_lock (&rdma_t_poll_lock);
      if (rdma_t_poll_nb >= 10)
	{
	  sctk_free (tmp);
	}
      else
	{
	  rdma_t_poll_nb++;
	  tmp->local_addr = (sctk_tcp_rdma_t *) rdma_t_poll;
	  rdma_t_poll = tmp;
	}
      sctk_thread_mutex_unlock (&rdma_t_poll_lock);
    }
}

static inline void
sctk_tcp_rdma_init (sctk_tcp_rdma_t * rdma, void *local_addr, void *dist_addr,
		    size_t size, int process, int *local_ack, int *dist_ack,
		    int mode)
{
  if (local_ack != NULL)
    {
      *local_ack = 0;
    }
  rdma->local_addr = local_addr;
  rdma->dist_addr = dist_addr;
  rdma->size = size;
  rdma->process = process;
  rdma->local_ack = local_ack;
  rdma->dist_ack = dist_ack;
  rdma->mode = mode;
}

static inline void
sctk_tcp_rdma_init_proceed (sctk_tcp_rdma_t * rdma)
{
  tpc_ident_t *ident;
  sctk_nodebug("rdma->process %d %d",rdma->process,sctk_process_rank);
  assert (rdma->process != sctk_process_rank);
  ident = &(idents[rdma->process]);
  sctk_nodebug ("LOCK to Write to fd %d %d", ident->fd, rdma->process);
  sctk_tcp_rdma_lock (ident);
  while (ident->fd < 0)
    sctk_thread_yield ();
  sctk_nodebug ("Write to fd %d size %lu", ident->fd, rdma->size);
  sctk_pmi_send(rdma, sizeof (sctk_tcp_rdma_t),ident->fd);
  if (rdma->mode == SCTK_TCP_RDMA_WRITE)
    {
      sctk_nodebug ("Write to fd %d size %lu", ident->fd, rdma->size);
      sctk_pmi_send(rdma->local_addr, rdma->size,ident->fd);
    }
  sctk_tcp_rdma_unlock (ident);
  sctk_tcp_rdma_free (rdma);
}

static inline int
sctk_tcp_rdma_write (void *local_addr, void *dist_addr, size_t size,
		     int process, int *local_ack, int *dist_ack)
{
  sctk_tcp_rdma_t *rdma;
  rdma = sctk_tcp_rdma_get ();
  if (rdma == NULL)
    {
      return 1;
    }
  if (local_ack != NULL)
    {
      *local_ack = 0;
    }
  sctk_tcp_rdma_init (rdma, local_addr, dist_addr, size, process, local_ack,
		      dist_ack, SCTK_TCP_RDMA_WRITE);
  sctk_nodebug ("RDMA write to %d size %lu, %p dist ack", process, size,
		dist_ack);
  sctk_tcp_rdma_init_proceed (rdma);
  sctk_nodebug ("DONE RDMA write to %d size %lu", process, size);
  return 0;
}

static inline int
sctk_tcp_rdma_read (void *local_addr, void *dist_addr, size_t size,
		    int process, int *ack)
{
  sctk_tcp_rdma_t *rdma;
  rdma = sctk_tcp_rdma_get ();
  if (rdma == NULL)
    {
      return 1;
    }
  if (ack != NULL)
    {
      *ack = 0;
    }
  sctk_nodebug ("ACK %p", ack);
  sctk_tcp_rdma_init (rdma, local_addr, dist_addr, size, process, NULL, ack,
		      SCTK_TCP_RDMA_READ);
  sctk_tcp_rdma_init_proceed (rdma);
  return 0;
}

static void *
sctk_rdma_server (void *arg)
{
  tpc_ident_t *ident;
  ident = (tpc_ident_t *) arg;

  sctk_nodebug ("sctk_rdma_server launch %d", ident->rank);
  while (ident->fd < 0)
    sched_yield ();

  sctk_nodebug ("sctk_rdma_server ready %d", ident->rank);

  while (1)
    {
      sctk_tcp_rdma_t rdma;
      sctk_pmi_recv(&rdma, sizeof (sctk_tcp_rdma_t),ident->fd);

      if (rdma.mode == SCTK_TCP_RDMA_READ)
	{
	  sctk_nodebug ("GET %p ack", rdma.local_ack);
	  while (sctk_tcp_rdma_write
		 (rdma.dist_addr, rdma.local_addr, rdma.size, ident->rank,
		  NULL, rdma.dist_ack) != 0);
	}
      else
	{
	  sctk_nodebug ("Remote write %p %lu ACK %p", rdma.dist_addr,
			rdma.size, rdma.dist_ack);
	  if (rdma.dist_ack != NULL)
	    {
	      if (*(rdma.dist_ack) != 0)
		{
		  sctk_error ("Dist ack %p != 0", rdma.dist_ack);
		  assume (*(rdma.dist_ack) == 0);
		}
	      assume (*(rdma.dist_ack) == 0);
	      *(rdma.dist_ack) = 0;
	    }
	  sctk_pmi_recv(rdma.dist_addr, rdma.size,ident->fd);

	  if (rdma.dist_ack != NULL)
	    {
	      *(rdma.dist_ack) = 1;
	    }
	}
    }
  return NULL;
}

/**************************************************************************/
/* RDMA DRIVER                                                            */
/**************************************************************************/
typedef int sctk_net_rdma_ack_t;

static int
sctk_net_rdma_write (void *local_addr, void *dist_addr, size_t size,
		     int process, sctk_net_rdma_ack_t * local_ack,
		     sctk_net_rdma_ack_t * dist_ack)
{
  return sctk_tcp_rdma_write (local_addr, dist_addr, size, process, local_ack,
			      dist_ack);
}
static int
sctk_net_rdma_read (void *local_addr, void *dist_addr, size_t size,
		    int process, sctk_net_rdma_ack_t * ack)
{
  return sctk_tcp_rdma_read (local_addr, dist_addr, size, process, ack);
}

static void
sctk_net_rdma_wait (sctk_net_rdma_ack_t * ack)
{
  sctk_thread_wait_for_value (ack, 1);
  /* WARNING have to reinit the sctk_net_rdma_ack_t structure */
  sctk_nodebug ("wait %p", ack);
  *ack = 0;
}

static int
sctk_net_rdma_test (sctk_net_rdma_ack_t * ack)
{
  return *ack;

}

/******************************************************************/
/* RPC                                                            */
/******************************************************************/
#include "sctk_rpc_rdma.h"

/******************************************************************/
/* COLLECTIVES                                                    */
/******************************************************************/
#define collective_op_steps 32
#define collective_buffer_size (1024)
#include "sctk_collectives_rdma.h"

void sctk_net_tcp_init_new_com(sctk_internal_communicator_t* comm,
    int nb_involved, int* task_list)
{

}
void sctk_net_tcp_free_com(int com_id)
{
  sctk_nodebug("Communicator freed : comm id %d",com_id);
  sctk_collective[com_id].coll_nb = 0;
}


/******************************************************************/
/* PT 2 PT                                                        */
/******************************************************************/
#define SCTK_NET_MESSAGE_NUMBER 128
#define SCTK_NET_MESSAGE_SPLIT_THRESHOLD (32*1024)
#define SCTK_NET_MESSAGE_BUFFER (SCTK_NET_MESSAGE_SPLIT_THRESHOLD*SCTK_NET_MESSAGE_NUMBER)
#include "sctk_pt2pt_rdma.h"

/******************************************************************/
/* DSM                                                            */
/******************************************************************/
#include "sctk_dsm_rdma.h"

/******************************************************************/
/* CONNECTION MAP                                                 */
/******************************************************************/
typedef struct {
  int process;
  void* tmp;
} sctk_connection_map_rpc_t;

typedef sctk_connection_map_rpc_t sctk_connection_map_t;

static sctk_connection_map_t  * volatile *sctk_connection_map = NULL;
static sctk_thread_mutex_t *sctk_connection_map_lock = NULL;

static inline void sctk_connection_check_no_lock(int process){
  assume(sctk_connection_map[process] == NULL);
  if(process < sctk_process_rank){
    size_t max_obj_size;
    sctk_connection_map_rpc_t msg;
    sctk_connection_map_t* buf;
    void* tmp = NULL;
    buf = sctk_malloc(sizeof(sctk_connection_map_t));

    msg.process = sctk_process_rank;

    max_obj_size = sctk_get_max_rpc_size_comm ();
    sctk_nodebug("Need connection to %d, max_size %lu",process,max_obj_size);

    assume(max_obj_size >= sizeof(sctk_connection_map_t));
    sctk_perform_rpc ((sctk_rpc_t) sctk_net_sctk_specific_adm_rpc_remote, process,
		      &msg, sizeof (sctk_connection_map_rpc_t));

    tmp = sctk_malloc(sizeof (sctk_net_ptp_slot_t));

    /* Prepare local pt2pt points*/
    buf->tmp = tmp;

    sctk_mpcrun_send_to_process(buf,sizeof(sctk_connection_map_t),process);
    sctk_mpcrun_read_to_process(buf,sizeof(sctk_connection_map_t),process);

    sctk_nodebug("Remote tab %p",buf->tmp);
    sctk_net_init_ptp_per_process(process,tmp,buf->tmp);

    sctk_connection_map[process] = buf;
    sctk_nodebug("Need connection to %d, max_size %lu done",process,max_obj_size);
  } else {
    sctk_connection_map_rpc_t msg;
    sctk_nodebug("Need connection to %d: ASK",process);
    msg.process = sctk_process_rank;
    sctk_perform_rpc ((sctk_rpc_t) sctk_net_sctk_specific_adm_rpc_remote, process,
		      &msg, sizeof (sctk_connection_map_rpc_t));
    while(sctk_connection_map[process] == NULL) sctk_thread_yield();
  }
  sctk_nodebug("Conenction to %d done",process);
}

static inline void sctk_connection_check(int process){
  assume(process != sctk_process_rank);
#ifndef SCTK_USE_STATIC_ALLOC_ONLY
  if(sctk_connection_map[process] == NULL){
    sctk_thread_mutex_lock(&(sctk_connection_map_lock[process]));
    if(sctk_connection_map[process] == NULL){
      sctk_connection_check_no_lock(process);
    }
    sctk_thread_mutex_unlock(&(sctk_connection_map_lock[process]));
  }
#endif
}

#ifndef SCTK_USE_STATIC_ALLOC_ONLY
static void
sctk_tcp_sctk_specific_adm_rpc_remote (sctk_connection_map_rpc_t * arg)
{
  if(arg->process > sctk_process_rank){
    void* tmp = NULL;
    sctk_connection_map_t* buf;
    if(sctk_connection_map[arg->process] == NULL){
      size_t max_obj_size;
      max_obj_size = sctk_get_max_rpc_size_comm ();
      buf = sctk_malloc(sizeof(sctk_connection_map_t));
      sctk_nodebug("Need connection from %d, max_size %lu",arg->process,max_obj_size);

      sctk_mpcrun_read_to_process( buf,sizeof(sctk_connection_map_t),arg->process);

      sctk_nodebug("Remote tab %p %d %d",buf->tmp,buf->process, arg->process);

      tmp = sctk_malloc(sizeof (sctk_net_ptp_slot_t));

      /* Prepare local pt2pt points*/
      sctk_net_init_ptp_per_process(arg->process,tmp,buf->tmp);

      buf->tmp = tmp;

      sctk_mpcrun_send_to_process(buf,sizeof(sctk_connection_map_t),arg->process);

      sctk_connection_map[arg->process] = buf;
    }
  } else {
    sctk_nodebug("Remote connection required");
    if(sctk_thread_mutex_trylock(&(sctk_connection_map_lock[arg->process])) == 0){
      if(sctk_connection_map[arg->process] == NULL){
	sctk_nodebug("Remote connection accepted");
	sctk_connection_check_no_lock(arg->process);
      } else {
	sctk_nodebug("Remote connection rejected");
      }
      sctk_thread_mutex_unlock(&(sctk_connection_map_lock[arg->process]));
    } else {
      sctk_nodebug("Remote connection rejected");
    }
  }
}
#endif

/******************************************************************/
/* INIT/END                                                       */
/******************************************************************/
#include "sctk_init_rdma.h"

static void *
sctk_server (void *arg)
{
  int fd;
  unsigned int clilen;
  struct sockaddr_in cli_addr;
  int i;

  sctk_nodebug ("Launch server");
  clilen = sizeof (cli_addr);

  for (i = 0; i <= sctk_process_rank - 1; i++)
    {
      int rank;
      fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (fd < 0)
	{
	  perror ("accept");
	}
      assert (fd >= 0);

      sctk_pmi_recv(&rank, sizeof (int),fd);
      sctk_nodebug ("Connection done from %d", rank);
      idents[rank].fd = fd;
    }
  sctk_nodebug ("Launch server done");
  return arg;
}


void
sctk_net_init_driver_tcp (int *argc, char ***argv)
{
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;
  int i;
  assume (argc != NULL);
  assume (argv != NULL);
  sctk_nodebug ("sctk_net_init_driver_tcp init thread");
  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, sctk_server, NULL);

  for (i = sctk_process_rank + 1; i < sctk_process_number; i++)
    {
      idents[i].fd = sctk_tcp_connect_to (idents[i].port, idents[i].name);
      sctk_pmi_send(&sctk_process_rank,
				 sizeof (int),idents[i].fd);
    }

  sctk_nodebug ("sctk_net_init_driver_tcp join thread");
  sctk_thread_join (pidt, NULL);
  sctk_nodebug ("sctk_net_init_driver_tcp joined thread");

  sctk_nodebug ("sctk_net_init_driver_tcp connect done");
  for (i = 0; i < sctk_process_number; i++)
    {
      if (i != sctk_process_rank)
	{
	  sctk_user_thread_create (&pidt, &attr, sctk_rdma_server,
				   &(idents[i]));
	}
    }
  sctk_nodebug ("sctk_net_init_driver_tcp done");

#ifdef SCTK_USE_STATIC_ALLOC_ONLY
  sctk_net_init_ptp (1);
#else
  sctk_net_init_ptp (0);
#endif
  sctk_net_init_driver_init_modules ("TCP");
}

static void
sctk_create_recv_socket ()
{
  struct sockaddr_in serv_addr;

  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    sctk_error ("ERROR opening socket");

  memset ((char *) &serv_addr, 0, sizeof (serv_addr));

  portno = 1023;
  sctk_nodebug ("Looking for an available port");
  do
    {
      portno++;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons ((unsigned short) portno);

    }
  while (bind (sockfd, (struct sockaddr *) &serv_addr,
	       sizeof (serv_addr)) < 0);
  sctk_nodebug ("Looking for an available port found %d", portno);

  listen (sockfd, 1);

}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_preinit_driver_tcp
 *  Description:
 * ==================================================================
 */
void
sctk_net_preinit_driver_tcp(sctk_net_driver_pointers_functions_t* pointers)
{
  int i;
  char msg[MAX_HOST_SIZE];

  sctk_create_recv_socket ();
  sctk_mpcrun_client_init_connect ();

  idents = sctk_malloc (sizeof (tpc_ident_t) * sctk_process_number);
  memset (idents, 0, sizeof (tpc_ident_t) * sctk_process_number);
  idents[sctk_process_rank].rank = sctk_process_rank;
  idents[sctk_process_rank].fd = -1;
  idents[sctk_process_rank].port = portno;
  gethostname (idents[sctk_process_rank].name, MAX_HOST_SIZE);
  idents[sctk_process_rank].lock = 0;

  /*
   * Existant :
   *
   * assume (sctk_mpcrun_client
   *     (MPC_SERVER_REGISTER, NULL, 0, &(idents[sctk_process_rank]),
   *     sizeof (tpc_ident_t)) == 0);
   */

  sprintf(msg,"%d:%d:%d:%d",idents[sctk_process_rank].rank, idents[sctk_process_rank].fd, idents[sctk_process_rank].port, idents[sctk_process_rank].lock);
  sctk_pmi_put_connection_info(msg, MAX_HOST_SIZE, SCTK_PMI_TAG_TCP);
  sctk_pmi_put_connection_info(idents[sctk_process_rank].name, MAX_HOST_SIZE, SCTK_PMI_TAG_TCP + 1);
  /** fin modif **/

  sctk_connection_map = sctk_malloc(sctk_process_number*sizeof(sctk_connection_map_t**));
  sctk_connection_map_lock = sctk_malloc(sctk_process_number*sizeof(sctk_thread_mutex_t));
  for (i = 0; i < sctk_process_number; i++)
    {
      static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
      sctk_connection_map[i] = NULL;
      sctk_connection_map_lock[i] = lock;
    }

  sctk_pmi_barrier();

  for (i = 0; i < sctk_process_number; i++)
    {

	  /*
	   * Existant :
	   *
	   * while (sctk_mpcrun_client
	   *     (MPC_SERVER_GET, &(idents[i]), sizeof (tpc_ident_t), &i,
	   *     sizeof (int)) != 0)
	   * {
	   *     // sleep disabled because of thread API error
	   *     //sleep (1);
	   * }
	   */

	  sctk_pmi_get_connection_info(msg, MAX_HOST_SIZE, SCTK_PMI_TAG_TCP, i);
	  sscanf(msg, "%d:%d:%d:%d", &idents[i].rank, &idents[i].fd, &idents[i].port, &idents[i].lock);
	  sctk_pmi_get_connection_info(idents[i].name, MAX_HOST_SIZE, SCTK_PMI_TAG_TCP + 1, i);
	  /** fin modif **/

      sctk_nodebug ("Process %d = %s %d", i, idents[i].name, idents[i].fd);
    }
  assume (sctk_process_number <= SCTK_MAX_PROCESS_NUMBER);
  sctk_nodebug ("TCP initilized for %d processes", sctk_process_number);

  sctk_nodebug ("allocations");

#ifdef SCTK_USE_STATIC_ALLOC_ONLY
  sctk_net_preinit_driver_init_modules ();
#else
  sctk_net_sctk_specific_adm_rpc_remote_ptr = (void(*)(void*))sctk_tcp_sctk_specific_adm_rpc_remote;
  sctk_net_preinit_driver_init_modules_no_global ();

#endif
  /*
     INIT FUNC FOR MODULES
     */
  pointers->rpc_driver          = sctk_net_rpc_driver;
  pointers->rpc_driver_retrive  = sctk_net_rpc_retrive_driver;
  pointers->rpc_driver_send     = sctk_net_rpc_send_driver;
  pointers->net_send_ptp_message= sctk_net_send_ptp_message_driver;
  pointers->net_copy_message    = sctk_net_copy_message_func_driver;
  pointers->net_free            = sctk_net_free_func_driver;

  pointers->collective          = sctk_net_collective_op_driver;

  pointers->net_adm_poll        = sctk_net_adm_poll_func;	/* RPC */
  pointers->net_ptp_poll        = sctk_net_ptp_poll_func;	/* PTP */

  pointers->net_new_comm        = sctk_net_tcp_init_new_com;
  pointers->net_free_comm       = sctk_net_tcp_free_com;

  sctk_nodebug ("sctk_net_preinit_driver_tcp done");
}

  void
sctk_net_init_driver_ipoib (int *argc, char ***argv)
{
  sctk_use_tcp_o_ib = 1;
  sctk_net_init_driver_tcp(argc,argv);
  sctk_net_init_driver_init_modules ("IPOIB");
}

  void
sctk_net_preinit_driver_ipoib (sctk_net_driver_pointers_functions_t* pointers)
{
  sctk_net_preinit_driver_tcp(pointers);
}
#else
  void
sctk_net_init_driver_tcp (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
  not_available ();
}

  void
sctk_net_preinit_driver_tcp (sctk_net_driver_pointers_functions_t* pointers)
{
  not_available ();
}
  void
sctk_net_init_driver_ipoib (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
  not_available ();
}

  void sctk_net_preinit_driver_ipoib (sctk_net_driver_pointers_functions_t* pointers)
{
  not_available ();
}
#endif
