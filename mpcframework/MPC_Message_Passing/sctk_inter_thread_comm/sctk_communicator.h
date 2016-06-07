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

#include <sctk_types.h>

/******************************** STRUCTURE *********************************/
struct sctk_internal_collectives_struct_s;

/********************************* FUNCTION *********************************/
void sctk_set_internal_collectives ( const sctk_communicator_t id, struct sctk_internal_collectives_struct_s *tmp );
void sctk_get_rank_size_total ( const sctk_communicator_t communicator, int *rank, int *size, int glob_rank );
void sctk_communicator_world_init ( int task_nb );
void sctk_communicator_self_init();
void sctk_communicator_delete();

int sctk_get_remote_comm_world_rank ( const sctk_communicator_t communicator, const int rank );
int sctk_get_first_task_local ( const sctk_communicator_t communicator );
int sctk_get_comm_world_rank ( const sctk_communicator_t communicator, const int rank );
int sctk_get_last_task_local ( const sctk_communicator_t communicator );
int sctk_get_nb_task_remote ( const sctk_communicator_t communicator );
int sctk_get_nb_task_local ( const sctk_communicator_t communicator );
int sctk_get_nb_task_total ( const sctk_communicator_t communicator );
int sctk_is_in_local_group ( const sctk_communicator_t id );
int sctk_get_remote_leader ( const sctk_communicator_t );
int sctk_get_local_leader ( const sctk_communicator_t );
int sctk_is_inter_comm ( const sctk_communicator_t );
int sctk_is_in_group ( const sctk_communicator_t communicator );
int sctk_get_rank ( const sctk_communicator_t communicator, const int comm_world_rank );
int sctk_get_node_rank_from_task_rank ( const int rank );

int sctk_get_process_rank_from_task_rank ( int rank );
int sctk_get_comm_number();

inline int sctk_get_process_nb_in_array ( const sctk_communicator_t communicator );
inline int *sctk_get_process_array ( const sctk_communicator_t communicator );

sctk_communicator_t sctk_get_peer_comm ( const sctk_communicator_t communicator );
sctk_communicator_t sctk_create_communicator ( const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list );
sctk_communicator_t sctk_create_intercommunicator ( const sctk_communicator_t local_comm, const int local_leader, const sctk_communicator_t peer_comm, const int remote_leader,
                                                    const int tag, const int first );
sctk_communicator_t sctk_duplicate_communicator ( const sctk_communicator_t origin_communicator, int is_inter_comm, int rank );
sctk_communicator_t sctk_get_local_comm_id ( const sctk_communicator_t communicator );
sctk_communicator_t sctk_delete_communicator ( const sctk_communicator_t );
sctk_communicator_t sctk_create_communicator_from_intercomm ( const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list, int first );
sctk_communicator_t sctk_create_intercommunicator_from_intercommunicator (const sctk_communicator_t origin_communicator, const int nb_task_involved, const int *task_list, int remote_leader, int local_com);
struct sctk_internal_collectives_struct_s *sctk_get_internal_collectives ( const sctk_communicator_t communicator );

#endif
