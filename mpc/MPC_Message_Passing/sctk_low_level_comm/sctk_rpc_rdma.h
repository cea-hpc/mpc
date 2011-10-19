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

typedef struct
{
  void (*func) (void *);
  char buffer[16];
} sctk_net_rpc_t;

static sctk_net_rpc_t *sctk_net_rpc_send;
static sctk_net_rpc_t *sctk_net_rpc_recv;
static int volatile *volatile sctk_net_rpc_send_flag;
static int volatile *volatile sctk_net_rpc_recv_flag;
static sctk_net_rdma_ack_t volatile *volatile sctk_net_rpc_recv_ack;
static size_t sctk_net_rpc_t_size = 0;

#define sctk_net_rpc_tab(a,b) ((sctk_net_rpc_t*)((char*)a + (b * sctk_net_rpc_t_size)))

#define sctk_net_rpc_global_var(func,a) func##__##a
#define sctk_net_rpc_global_registered(func,a) sctk_net_rpc_global_registered_val->sctk_net_rpc_global_var(func,a)
typedef struct
{
  int sctk_net_rpc_global_var (sctk_net_adm_poll_func, flag);
  int sctk_net_rpc_global_var (sctk_net_rpc_driver, flag);
  int sctk_net_rpc_global_var (sctk_net_rpc_retrive_driver, done_dist);
} sctk_net_rpc_global_registered_t;

static sctk_net_rpc_global_registered_t *sctk_net_rpc_global_registered_val =
  NULL;

static void
sctk_net_preinit_rpc (int use_global_isoalloc)
{
  sctk_net_rpc_t_size =
    sizeof (sctk_net_rpc_t) + sctk_get_max_rpc_size_comm () - 16;

  if(use_global_isoalloc){
    /*No need to initialize pages thanks iso_allocation */
    sctk_net_rpc_send =
      sctk_iso_malloc (sctk_process_number * (sctk_net_rpc_t_size));
    sctk_net_rpc_recv =
      sctk_iso_malloc (sctk_process_number * (sctk_net_rpc_t_size));
    sctk_net_rpc_send_flag =
      sctk_iso_malloc (sctk_process_number * sizeof (int));
    sctk_net_rpc_recv_ack =
      sctk_iso_malloc (sctk_process_number * sizeof (sctk_net_rdma_ack_t));
    sctk_net_rpc_recv_flag =
      sctk_iso_malloc (sctk_process_number * sizeof (int));

    sctk_net_rpc_global_registered_val =
      sctk_iso_malloc (sizeof (sctk_net_rpc_global_registered_t));
    sctk_nodebug ("%p %p", sctk_net_rpc_send, sctk_net_rpc_recv);
  } else {
    /*No need to initialize pages thanks iso_allocation */
    sctk_net_rpc_send =
      sctk_iso_malloc (sctk_process_number * (sctk_net_rpc_t_size));
    sctk_net_rpc_recv =
      sctk_iso_malloc (sctk_process_number * (sctk_net_rpc_t_size));
    sctk_net_rpc_send_flag =
      sctk_iso_malloc (sctk_process_number * sizeof (int));
    sctk_net_rpc_recv_ack =
      sctk_iso_malloc (sctk_process_number * sizeof (sctk_net_rdma_ack_t));
    sctk_net_rpc_recv_flag =
      sctk_iso_malloc (sctk_process_number * sizeof (int));

    sctk_net_rpc_global_registered_val =
      sctk_iso_malloc (sizeof (sctk_net_rpc_global_registered_t));
    sctk_nodebug ("%p %p", sctk_net_rpc_send, sctk_net_rpc_recv);
  }
}

static void
sctk_net_adm_poll_func (void *arg)
{
  /*RPC deamon */
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  int i;

  sctk_thread_mutex_lock (&lock);

  for (i = 0; i < sctk_process_number; i++)
    {
      if (sctk_net_rpc_recv_flag[i] != 0)
	{
	  void *tmp_arg;
	  sctk_rpc_t func;
	  sctk_net_rpc_global_registered (sctk_net_adm_poll_func, flag) = 0;
//	  sctk_nodebug ("Message RPC from %d WAIT", i);

	  sctk_net_rdma_wait ((sctk_net_rdma_ack_t *) &
			      (sctk_net_rpc_recv_ack[i]));

	  tmp_arg =
	    sctk_rpc_get_slot (sctk_net_rpc_tab (sctk_net_rpc_recv, i)->
			       buffer);
	  func = sctk_net_rpc_tab (sctk_net_rpc_recv, i)->func;
	  sctk_net_rpc_recv_flag[i] = 0;

	  assume (sctk_net_rdma_write
		  (&
		   (sctk_net_rpc_global_registered
		    (sctk_net_adm_poll_func, flag)),
		   (int *) &(sctk_net_rpc_send_flag[sctk_process_rank]),
		   sizeof (int), i, NULL, NULL) == 0);

	  sctk_nodebug ("RPC recv message from %d todo func %p", i, func);
	  sctk_rpc_execute (func, tmp_arg);
	  sctk_nodebug ("RPC recv message from %d done", i);
	}
    }
  sctk_thread_mutex_unlock (&lock);
}

static void
sctk_net_rpc_driver (void (*func) (void *), int destination, void *arg,
		     size_t arg_size)
{
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_net_rdma_ack_t done;
  sctk_net_rpc_global_registered (sctk_net_rpc_driver, flag) = 1;
  sctk_thread_mutex_lock (&lock);

  sctk_thread_wait_for_value ((int *) &(sctk_net_rpc_send_flag[destination]),
			      0);

  sctk_nodebug ("Send RPC %p to %d size %lu flag %p", func, destination,
		arg_size, &(sctk_net_rpc_send_flag[destination]));

  sctk_net_rpc_send_flag[destination] = 1;
  sctk_net_rpc_tab (sctk_net_rpc_send, destination)->func = func;
  memcpy (sctk_net_rpc_tab (sctk_net_rpc_send, destination)->buffer, arg,
	  arg_size);

  assume (sctk_net_rdma_write
	  (sctk_net_rpc_tab (sctk_net_rpc_send, destination),
	   sctk_net_rpc_tab (sctk_net_rpc_recv, sctk_process_rank),
	   sctk_net_rpc_t_size, destination, &done,
	   (sctk_net_rdma_ack_t *) &
	   (sctk_net_rpc_recv_ack[sctk_process_rank])) == 0);

  sctk_net_rdma_wait (&(done));

  assume (sctk_net_rdma_write
	  (&(sctk_net_rpc_global_registered (sctk_net_rpc_driver, flag)),
	   (void *) &(sctk_net_rpc_recv_flag[sctk_process_rank]),
	   sizeof (int), destination, &done, NULL) == 0);

  sctk_nodebug ("Send RPC %p to %d size %lu flag %p done WAIT", func,
		destination, arg_size,
		&(sctk_net_rpc_send_flag[destination]));

  sctk_net_rdma_wait (&(done));

  sctk_nodebug ("Send RPC %p to %d size %lu flag %p done SENDED", func,
		destination, arg_size,
		&(sctk_net_rpc_send_flag[destination]));

  sctk_thread_wait_for_value ((int *) &(sctk_net_rpc_send_flag[destination]),
			      0);

  sctk_nodebug ("Send RPC %p to %d size %lu flag %p done", func, destination,
		arg_size, &(sctk_net_rpc_send_flag[destination]));
  sctk_thread_mutex_unlock (&lock);
}

static void
sctk_net_rpc_send_driver (void *dest, void *src, size_t arg_size, int process,
			  int *ack, uint32_t rkey)
{
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  static int done_dist = 1;
  sctk_net_rdma_ack_t done;
  sctk_thread_mutex_lock (&lock);
  if (arg_size)
    {
      sctk_nodebug ("Send message");

      sctk_net_rdma_write (src, dest, arg_size, process, &done, NULL);
      sctk_net_rdma_wait (&(done));

    }
  sctk_nodebug ("ACK sctk_net_rpc_send_driver %p", &done);

  sctk_net_rdma_write (&done_dist, ack, sizeof (int), process, &done, NULL);
  sctk_net_rdma_wait (&(done));

  sctk_thread_mutex_unlock (&lock);
}

static void
sctk_net_rpc_retrive_driver (void *dest, void *src, size_t arg_size,
			     int process, int *ack, uint32_t rkey)
{
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_net_rdma_ack_t done;
  sctk_net_rpc_global_registered (sctk_net_rpc_retrive_driver, done_dist) = 1;

  sctk_thread_mutex_lock (&lock);
  sctk_nodebug ("RETRIEVE : read %p to %p size %lu", src, dest, arg_size);
  if (dest && src)
    {
      while (sctk_net_rdma_read (dest, src, arg_size, process, &done) != 0)
	sctk_thread_yield ();
      sctk_net_rdma_wait (&(done));
    }

  while (sctk_net_rdma_write
	 (&
	  (sctk_net_rpc_global_registered
	   (sctk_net_rpc_retrive_driver, done_dist)), ack, sizeof (int),
	  process, &done, NULL) != 0)
    sctk_thread_yield ();
  sctk_net_rdma_wait (&(done));
  sctk_thread_mutex_unlock (&lock);
}
