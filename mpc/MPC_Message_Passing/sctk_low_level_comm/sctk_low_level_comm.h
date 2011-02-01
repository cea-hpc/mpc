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
#ifndef __SCTK_NET_MSG_H_
#define __SCTK_NET_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "sctk.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_collective_communications.h"
#include "sctk_rpc.h"

   /*RPC*/ void sctk_net_send_task_location (int task, int process);
  void sctk_net_send_task_end (int task, int process);
  void sctk_net_update_communicator (int task, sctk_communicator_t comm,
				     int vp);
  void sctk_net_reinit_communicator (int task, sctk_communicator_t comm,
				     int vp);
  void sctk_net_get_free_communicator (const sctk_communicator_t
				       origin_communicator);
  void sctk_net_update_new_communicator (const sctk_communicator_t
					 origin_communicator,
					 const int nb_task_involved,
					 const int *task_list,
					 const int process);
  void sctk_net_set_free_communicator (const sctk_communicator_t
				       origin_communicator,
				       const sctk_communicator_t
				       communicator);
  void sctk_net_migration (const int rank, const int process);

  void sctk_rpc_collective_op (const size_t elem_size,
			       const size_t nb_elem,
			       const void *data_in,
			       void *const data_out,
			       void (*func) (const void *, void *,
					     size_t, sctk_datatype_t),
			       const sctk_communicator_t com_id,
			       const int vp, const int task_id,
			       const sctk_datatype_t data_type, int process);

    /*COLLECTIVES*/
    void
    sctk_collective_init_func (void (*collective_func)
			       (sctk_collective_communications_t * com,
				sctk_virtual_processor_t * my_vp,
				const size_t elem_size, const size_t nb_elem,
        const int root,
				void (*func) (const void *, void *, size_t,
					      sctk_datatype_t),
				const sctk_datatype_t data_type));
  void sctk_net_collective_op (sctk_collective_communications_t * com,
			       sctk_virtual_processor_t * my_vp,
			       const size_t elem_size, const size_t nb_elem,
             const int root,
			       void (*func) (const void *, void *, size_t,
					     sctk_datatype_t),
			       const sctk_datatype_t data_type);

  /*PT 2 PT */

  void
    sctk_send_init_func (void (*send_func)
			 (sctk_thread_ptp_message_t * msg, int dest_process),
			 void (*copy_func) (sctk_thread_ptp_message_t *
					    restrict dest,
					    sctk_thread_ptp_message_t *
					    restrict src),
			 void (*free_func) (sctk_thread_ptp_message_t *
					    item));
  void sctk_net_send_ptp_message (sctk_thread_ptp_message_t * msg,
				  int dest_process);
  void sctk_net_copy_message_func (sctk_thread_ptp_message_t * restrict dest,
				   sctk_thread_ptp_message_t * restrict src);

  void sctk_net_free_func (sctk_thread_ptp_message_t * item);

  void
  sctk_net_abort ();
  /* DSM */
  void
    sctk_net_get_pages_init (void (*func)
			     (void *addr, size_t size, int process));
  void sctk_net_get_pages (void *addr, size_t size, int process);

    /*POLLING*/ extern void (*sctk_net_adm_poll) (void *);
  extern void (*sctk_net_ptp_poll) (void *);

  void
    sctk_register_ptr (void (*func) (void *addr, size_t size),
		       void (*unfunc) (void *addr, size_t size));

    /*INIT*/ void sctk_net_init_driver (char *name);

  /*migration */

  int sctk_is_net_migration_available (void);
  void sctk_set_net_migration_available (int val);

  void sctk_net_sctk_specific_adm_rpc_remote (void * arg);
  extern void (*sctk_net_sctk_specific_adm_rpc_remote_ptr) (void * arg);

  /* geters */
  void* sctk_net_get_send_ptp_message_ptr();
  void* sctk_net_get_copy_message_func_driver();
  void* sctk_net_get_free_func_driver();
  void* sctk_net_get_collective_op_ptr();

  /* module used */
  char sctk_module_name[256];

  /*update_communicator*/
  typedef struct
  {
    int task_id;
    int vp;
    sctk_communicator_t com;
    int src;
  } sctk_update_communicator_t;



#ifdef __cplusplus
}
#endif
#endif
