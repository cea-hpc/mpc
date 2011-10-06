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
#ifndef __SCTK_COLLECTIVE_COMMUNICATIONS_H_
#define __SCTK_COLLECTIVE_COMMUNICATIONS_H_

#include <stdio.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef unsigned int sctk_communicator_t;
  typedef unsigned int sctk_datatype_t;
#define sctk_null_data_type ((sctk_datatype_t)(-1))

  typedef struct sctk_collective_communications_data_s
  {
    void *restrict data_in;
    void *restrict data_out;
    struct sctk_collective_communications_data_s **data_out_tab;
    volatile int done;
  } sctk_collective_communications_data_t;

  typedef struct sctk_migration_data_s
  {
    void *restrict data;
    volatile int done;
    struct sctk_migration_data_s *next;
  } sctk_migration_data_t;

  typedef struct sctk_virtual_processor_s
  {
    sctk_collective_communications_data_t data;
    volatile int nb_task_involved;
    volatile int nb_task_registered;
    volatile void *restrict list;
    sctk_thread_mutex_t lock;

    struct sctk_virtual_processor_s *father;
  } sctk_virtual_processor_t;

  typedef struct sctk_collective_communications_s
  {
    sctk_thread_mutex_t lock;
    sctk_thread_mutex_t lock_migration_list;
    sctk_collective_communications_data_t data;
    sctk_virtual_processor_t **virtual_processors;
    int nb_task_involved;
    volatile int nb_task_registered;
    volatile int initialized;
    volatile int reinitialize;
    int *restrict last_vp;
    int *restrict last_process;
    int nb_task_involved_in_this_process;
    volatile int *receive_list;
    int receive_number;
    int send_process;
    void *net_tmp_data;
    volatile int *involved_list;
    int involved_list_next;
    int involved_nb;
    sctk_communicator_t id;

    volatile sctk_migration_data_t *migration_list;
  } sctk_collective_communications_t;

  void sctk_net_realloc (sctk_collective_communications_t * com, size_t size);
  void sctk_collective_communication_init_mem (void);
  sctk_collective_communications_t *sctk_collective_communications_init (const
									 int
									 nb_task_involved);
    sctk_collective_communications_t
    * sctk_collective_communications_create (const int nb_task_involved);
  void sctk_collective_communications_duplicate(sctk_collective_communications_t
						* from , sctk_collective_communications_t
						*tmp,int nb_task_involved,const sctk_communicator_t com_id);
  void sctk_collective_communications_delete
    (sctk_collective_communications_t * com);
  /* void sctk_perform_collective_communication (const size_t elem_size, */
  /* 					      const size_t nb_elem, */
  /* 					      const void *data_in, */
  /* 					      void *data_out, */
  /* 					      void (*func) (const void *, */
  /* 							    void *, */
  /* 							    size_t, */
  /* 							    sctk_datatype_t), */
  /* 					      const sctk_communicator_t */
  /* 					      com_id, const int vp, */
  /* 					      const int task_id, */
  /* 					      const sctk_datatype_t */
  /* 					      data_type); */

  void
    sctk_perform_collective_communication_barrier (const
						   sctk_communicator_t
						   com_id, const int vp,
						   const int task_id);

  void
    sctk_perform_collective_communication_reduction (const size_t
						     elem_size,
						     const size_t nb_elem,
						     const void *data_in,
						     void *const data_out,
						     void (*func) (const
								   void *,
								   void *,
								   size_t,
								   sctk_datatype_t),
						     const
						     sctk_communicator_t
						     com_id, const int vp,
						     const int task_id,
						     const sctk_datatype_t
						     data_type);


  void
    sctk_perform_collective_communication_broadcast (const size_t
						     elem_size,
						     const size_t nb_elem,
                 const int root,
						     const void *data_in,
						     void *const data_out,
						     const
						     sctk_communicator_t
						     com_id, const int vp,
						     const int task_id,
						     const sctk_datatype_t
						     data_type);

  void
    sctk_perform_collective_communication_rpc (const size_t elem_size,
					       const size_t nb_elem,
					       const void *data_in,
					       void *const data_out,
					       void (*func) (const void *,
							     void *,
							     size_t,
							     sctk_datatype_t),
					       const sctk_communicator_t
					       com_id, const int vp,
					       const int task_id,
					       const sctk_datatype_t
					       data_type);

  void sctk_init_collective_communicator (const int vp, const int task_id,
					  const sctk_communicator_t
					  com_id, const int process);
  void sctk_terminaison_barrier (const int id);

  int sctk_get_rank (const sctk_communicator_t communicator,
		     const int comm_world_rank);

#define SCTK_USE_SPECIFIC_COLLECTIVE_COMMS

  static inline void sctk_barrier (const sctk_communicator_t com_id)
  {
    int vp;
    int id;
    sctk_thread_data_t *tmp;

      vp = sctk_thread_get_vp ();
      tmp = sctk_thread_data_get ();

      sctk_assert (tmp != NULL);

      id = sctk_get_rank (com_id, tmp->task_id);

#ifdef SCTK_USE_SPECIFIC_COLLECTIVE_COMMS
      sctk_perform_collective_communication_barrier (com_id, vp, id);
#else
      sctk_perform_collective_communication (0, 0,
					     NULL,
					     NULL,
					     NULL,
					     com_id, vp, id,
					     sctk_null_data_type);
#endif
  } static inline
    void sctk_all_reduce (const void *buffer_in, void *buffer_out,
			  const size_t elem_size,
			  const size_t elem_number,
			  void (*func) (const void *, void *, size_t,
					sctk_datatype_t),
			  const sctk_communicator_t com_id,
			  const sctk_datatype_t data_type)
  {
    int vp;
    int id;
    sctk_thread_data_t *tmp;

    vp = sctk_thread_get_vp ();
    tmp = sctk_thread_data_get ();

    sctk_assert (tmp != NULL);

    id = sctk_get_rank (com_id, tmp->task_id);

#ifdef SCTK_USE_SPECIFIC_COLLECTIVE_COMMS
    if (elem_size * elem_number != 0)
      {
	sctk_perform_collective_communication_reduction (elem_size,
							 elem_number,
							 buffer_in,
							 buffer_out,
							 func, com_id,
							 vp, id, data_type);
      }
    else
      {
	sctk_perform_collective_communication_barrier (com_id, vp, id);
      }
#else
    sctk_perform_collective_communication (elem_size, elem_number,
					   buffer_in,
					   buffer_out,
					   func, com_id, vp, id, data_type);
#endif
  }

  static inline
    void sctk_broadcast (void *buffer, const size_t size,
			 const int root, const sctk_communicator_t com_id)
  {
    int vp;
    int id;
    sctk_thread_data_t *tmp;

    vp = sctk_thread_get_vp ();
    tmp = sctk_thread_data_get ();

    sctk_assert (tmp != NULL);

    id = sctk_get_rank (com_id, tmp->task_id);

#ifdef SCTK_USE_SPECIFIC_COLLECTIVE_COMMS
    if (id == root)
      {
	sctk_perform_collective_communication_broadcast (1, size,
               root,
							 buffer, NULL,
							 com_id, vp, id,
							 sctk_null_data_type);
      }
    else
      {
	sctk_perform_collective_communication_broadcast (1, size,
               root,
							 NULL, buffer,
							 com_id, vp, id,
							 sctk_null_data_type);
      }
#else
    if (id == root)
      {
	sctk_perform_collective_communication (1, size,
					       buffer, NULL, NULL,
					       com_id, vp, id,
					       sctk_null_data_type);
      }
    else
      {
	sctk_perform_collective_communication (1, size,
					       NULL, buffer, NULL,
					       com_id, vp, id,
					       sctk_null_data_type);
      }
#endif
  }
#ifdef __cplusplus
}
#endif
#endif
