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
#ifndef __SCTK_COMMUNICATOR_H_
#define __SCTK_COMMUNICATOR_H_

#include <stdio.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_collective_communications.h"

#ifdef __cplusplus
extern "C"
{
#endif


  typedef struct sctk_internal_communicator_s
  {
    sctk_communicator_t communicator_number;
    sctk_communicator_t origin_communicator;
    volatile sctk_communicator_t new_communicator;
    int nb_task_involved;
    volatile int nb_task_registered;

    int *rank_in_communicator;

    size_t local_communicator_size;
    int *global_in_communicator_local;
    size_t remote_communicator_size;
    int *global_in_communicator_remote;

    sctk_thread_mutex_t lock;
    sctk_collective_communications_t *collective_communications;
    int is_inter_comm;
  } sctk_internal_communicator_t;

#define SCTK_MAX_COMMUNICATOR_NUMBER 10

#define SCTK_COMM_WORLD 0
#define SCTK_COMM_SELF 1

#define SCTK_COMM_EMPTY ((sctk_communicator_t)(-1))

  void sctk_get_rank_size_local (const sctk_communicator_t communicator,
				 int *rank, int *size, int glob_rank);
  int sctk_get_nb_task_local (const sctk_communicator_t communicator);

  void sctk_get_rank_size_remote (const sctk_communicator_t communicator,
				  int *rank, int *size, int glob_rank);
  int sctk_get_nb_task_remote (const sctk_communicator_t communicator);

  int sctk_get_rank (const sctk_communicator_t communicator,
		     const int comm_world_rank);


  int sctk_is_inter_comm (const sctk_communicator_t communicator);
  sctk_communicator_t sctk_create_communicator (const sctk_communicator_t
						origin_communicator,
						const int
						nb_task_involved,
						const int *task_list,
						int is_inter_comm);
  sctk_communicator_t sctk_delete_communicator (const sctk_communicator_t
						communicator);


  int sctk_is_valid_comm (const sctk_communicator_t communicator);
  void sctk_communicator_init (const int nb_task);
  void sctk_communicator_delete (void);
  void
    sctk_get_free_communicator_on_root (const sctk_communicator_t
					origin_communicator);

  sctk_internal_communicator_t*
  sctk_update_new_communicator (const sctk_communicator_t
				     origin_communicator,
				     const int nb_task_involved,
				     const int *task_list);
  void sctk_update_free_communicator (const sctk_communicator_t communicator);
  sctk_internal_communicator_t *sctk_get_communicator (const
						       sctk_communicator_t
						       com_id);
#ifdef __cplusplus
}
#endif
#endif
