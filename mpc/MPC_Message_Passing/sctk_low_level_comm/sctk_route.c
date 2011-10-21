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

#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_communicator.h>
#include <sctk_spinlock.h>

static sctk_route_table_t* sctk_route_table = NULL;
static sctk_spin_rwlock_t sctk_route_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

void sctk_add_route(int dest, sctk_route_table_t* tmp){
  tmp->key.destination = dest;

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_ADD(hh,sctk_route_table,key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
  
}

sctk_route_table_t* sctk_get_route(int dest){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = sctk_get_process_rank_from_task_rank(dest);  
  sctk_debug("%d is in %d",dest,key.destination);

  sctk_spinlock_read_lock(&sctk_route_table_lock);
  HASH_FIND(hh,sctk_route_table,&key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_read_lock(&sctk_route_table_lock);
  
  if(tmp == NULL){
    not_implemented();
  }

  return tmp;
}
