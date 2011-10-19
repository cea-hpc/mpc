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
#include <stdint.h>

static sctk_rpc_function_t sctk_rpc_driver = NULL;
static sctk_rpc_retrive_function_t sctk_rpc_driver_retrive = NULL;
static sctk_rpc_send_function_t sctk_rpc_driver_send = NULL;
static size_t max_rpc_size_comm = 0;

void *
sctk_rpc_get_slot (void *arg)
{
  void *tmp_arg;
  tmp_arg = sctk_malloc (max_rpc_size_comm);
  memcpy (tmp_arg, arg, max_rpc_size_comm);
  return tmp_arg;
}

static void
sctk_rpc_put_slot (void *ptr)
{
  sctk_free (ptr);
}

void
sctk_rpc_init_func (sctk_rpc_function_t func,
		    sctk_rpc_retrive_function_t func_retrive,
		    sctk_rpc_send_function_t func_send)
{
  sctk_rpc_driver = func;
  sctk_rpc_driver_retrive = func_retrive;
  sctk_rpc_driver_send = func_send;
}

/* RPC functions hooks */
void*
sctk_rpc_get_driver()
{
  return sctk_rpc_driver;
}

void*
sctk_rpc_get_driver_retrive()
{
  return sctk_rpc_driver_retrive;
}

void*
sctk_rpc_get_driver_send()
{
  return sctk_rpc_driver_send;
}


#define SCTK_RPC_FUNC_NUMBER 15
static sctk_rpc_t volatile sctk_rpc_generic_func[SCTK_RPC_FUNC_NUMBER];

void
sctk_register_function (sctk_rpc_t func)
{
  int i;
  for (i = 0; i < SCTK_RPC_FUNC_NUMBER; i++)
    {
      if (sctk_rpc_generic_func[i] == NULL)
	{
	  sctk_rpc_generic_func[i] = func;
	  sctk_nodebug ("Register %p in %d", func, i);
	  return;
	}
    }
  not_reachable ();
}

static sctk_rpc_t
sctk_convert_real_to_gen_function (sctk_rpc_t func)
{
  long i;
  for (i = 0; i < SCTK_RPC_FUNC_NUMBER; i++)
    {
      if (sctk_rpc_generic_func[i] == func)
	{
	  sctk_nodebug ("Matching real->gen %p is %ld", func, i);
	  return (sctk_rpc_t) i;
	}
    }
  not_reachable ();
  return NULL;
}

static sctk_rpc_t
sctk_convert_gen_to_real_function (sctk_rpc_t func)
{
  long i;
  i = (long) func;
  sctk_nodebug ("Matching gen->real %ld is %p", i, sctk_rpc_generic_func[i]);
  return sctk_rpc_generic_func[i];
}

void
sctk_perform_rpc (sctk_rpc_t func, int destination, void *arg,
		  size_t arg_size)
{
  assume (arg_size <= max_rpc_size_comm);
  sctk_rpc_driver (sctk_convert_real_to_gen_function (func), destination, arg,
		   arg_size);
}

void
sctk_rpc_execute (sctk_rpc_t func, void *arg)
{
  sctk_convert_gen_to_real_function (func) (arg);
  sctk_rpc_put_slot (arg);
}

void
sctk_set_max_rpc_size_comm (size_t size)
{
  if (size > max_rpc_size_comm)
    {
      max_rpc_size_comm = size;
    }
}

size_t
sctk_get_max_rpc_size_comm ()
{
  return max_rpc_size_comm;
}


void
sctk_perform_rpc_retrive (void *dest, void *src, size_t arg_size, int process,
			  int *ack, uint32_t rkey)
{
  sctk_nodebug ("GET %p %p %lu on %d %p", dest, src, arg_size, process, ack);
  sctk_rpc_driver_retrive (dest, src, arg_size, process, ack, rkey);
  sctk_nodebug ("GET %p %p %lu on %d DONE", dest, src, arg_size, process,
		ack);
}

void
sctk_perform_rpc_send (void *dest, void *src, size_t arg_size, int process,
		       int *ack, uint32_t rkey)
{
  sctk_nodebug ("PUT %p %p %lu on %d %p", dest, src, arg_size, process, ack);
  sctk_rpc_driver_send (dest, src, arg_size, process, ack, rkey);
  sctk_nodebug ("PUT %p %p %lu on %d DONE", dest, src, arg_size, process,
		ack);
}
