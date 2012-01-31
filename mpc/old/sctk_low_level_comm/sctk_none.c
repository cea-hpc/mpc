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
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_rpc.h"
#include "sctk_none.h"

#ifdef MPC_USE_NONE

static void
sctk_net_rpc_driver (void (*func) (void *), int destination, void *arg,
		     size_t arg_size)
{
  not_reachable ();
  assume (func);
  assume (destination);
  assume (arg);
  assume (arg_size);
}

static void
sctk_net_rpc_send_driver (void *dest, void *src, size_t arg_size, int process,
			  int *ack, uint32_t rkey)
{
  not_reachable ();
  assume (dest);
  assume (src);
  assume (arg_size);
  assume (process);
  assume (ack);
}
static void
sctk_net_rpc_retrive_driver (void *dest, void *src, size_t arg_size,
			     int process, int *ack, uint32_t rkey)
{
  not_reachable ();
  assume (dest);
  assume (src);
  assume (arg_size);
  assume (process);
  assume (ack);
}

static void
sctk_net_collective_op_driver (sctk_collective_communications_t * com,
			       sctk_virtual_processor_t * my_vp,
			       const size_t elem_size,
			       const size_t nb_elem,
             const int root,
			       void (*func) (const void *, void *, size_t,
					     sctk_datatype_t),
			       const sctk_datatype_t data_type)
{
  not_reachable ();
  assume (com);
  assume (my_vp);
  assume (elem_size);
  assume (nb_elem);
  assume (func);
  assume (data_type);
}

static void
sctk_net_send_ptp_message_driver (sctk_thread_ptp_message_t * msg,
				  int dest_process)
{
  not_reachable ();
  assume (msg);
  assume (dest_process);
}

static void
sctk_net_copy_message_func_driver (sctk_thread_ptp_message_t * restrict dest,
				   sctk_thread_ptp_message_t * restrict src)
{
  not_reachable ();
  assume (dest);
  assume (src);
}

static void
sctk_net_free_func_driver (sctk_thread_ptp_message_t * item)
{
  not_reachable ();
  assume (item);
}

void
sctk_net_init_driver_none (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
  sctk_rpc_init_func (sctk_net_rpc_driver, sctk_net_rpc_retrive_driver,
		      sctk_net_rpc_send_driver);
  sctk_send_init_func (sctk_net_send_ptp_message_driver,
		       sctk_net_copy_message_func_driver,
		       sctk_net_free_func_driver);
  sctk_collective_init_func (sctk_net_collective_op_driver);
  not_reachable ();
}

void
sctk_net_preinit_driver_none (void)
{
  not_reachable ();
}
#else
void
sctk_net_init_driver_none (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
  not_available ();
}

void
sctk_net_preinit_driver_none (void)
{
  not_available ();
}
#endif
