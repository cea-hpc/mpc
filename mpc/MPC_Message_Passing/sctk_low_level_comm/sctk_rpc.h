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
#ifndef __SCTK__RPC_H_
#define __SCTK__RPC_H_
#ifdef __cplusplus
extern "C"
{
#endif
  typedef void (*sctk_rpc_t) (void *arg);
  typedef void (*sctk_rpc_function_t) (sctk_rpc_t func, int destination,
				       void *arg, size_t arg_size);
  typedef void (*sctk_rpc_retrive_function_t) (void *dest, void *src,
					       size_t arg_size, int process,
					       int *ack);
  typedef void (*sctk_rpc_send_function_t) (void *dest, void *src,
					    size_t arg_size, int process,
					    int *ack);


  void sctk_rpc_init_func (sctk_rpc_function_t func,
			   sctk_rpc_retrive_function_t func_retrive,
			   sctk_rpc_send_function_t func_send);
  void sctk_rpc_execute (sctk_rpc_t func, void *arg);

  /*RPC call are blocking calls */
  void sctk_perform_rpc (sctk_rpc_t func, int destination, void *arg,
			 size_t arg_size);
  void sctk_perform_rpc_retrive (void *dest, void *src, size_t arg_size,
				 int process, int *ack);
  void sctk_perform_rpc_send (void *dest, void *src, size_t arg_size,
			      int process, int *ack);

  void sctk_set_max_rpc_size_comm (size_t size);
  size_t sctk_get_max_rpc_size_comm (void);
  void *sctk_rpc_get_slot (void *arg);

  void* sctk_rpc_get_driver();
  void* sctk_rpc_get_driver_retrive();
  void* sctk_rpc_get_driver_send();



  void sctk_register_function (sctk_rpc_t func);
#ifdef __cplusplus
}
#endif
#endif
