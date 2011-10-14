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

#warning "To remove and store data per communicators"
#define SCTK_MAX_COMMUNICATOR_NUMBER 10
typedef int sctk_communicator_t;
#define SCTK_COMM_WORLD 0
#define SCTK_COMM_SELF 1

void sctk_communicator_init();
int sctk_get_nb_task_local (const sctk_communicator_t communicator);
int sctk_get_nb_task_total (const sctk_communicator_t communicator);
  void sctk_get_rank_size_total (const sctk_communicator_t communicator,
				 int *rank, int *size, int glob_rank);
int sctk_get_rank (const sctk_communicator_t communicator,
		   const int comm_world_rank);
int sctk_get_comm_world_rank (const sctk_communicator_t communicator,
		   const int rank);
sctk_communicator_t sctk_delete_communicator (const sctk_communicator_t);
void sctk_communicator_delete();
int sctk_is_net_message (int dest);
#endif
