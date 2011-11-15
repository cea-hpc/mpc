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

#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_reorder.h>
#include <sctk_communicator.h>
#include <sctk_spinlock.h>

static sctk_route_table_t* sctk_dynamic_route_table = NULL;
static sctk_route_table_t* sctk_static_route_table = NULL;
static sctk_spin_rwlock_t sctk_route_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static sctk_rail_info_t* rails = NULL;

void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  sctk_add_dynamic_reorder_buffer(dest);

  sctk_spinlock_write_lock(&sctk_route_table_lock);
  HASH_ADD(hh,sctk_dynamic_route_table,key,sizeof(sctk_route_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_route_table_lock);
  
}

void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail){
  tmp->key.destination = dest;
  tmp->key.rail = rail->rail_number;
  tmp->rail = rail;

  sctk_add_static_reorder_buffer(dest);
  HASH_ADD(hh,sctk_static_route_table,key,sizeof(sctk_route_key_t),tmp);  
}

sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail){
  sctk_route_key_t key;
  sctk_route_table_t* tmp;

  key.destination = dest; 
  key.rail = rail->rail_number;

  
  HASH_FIND(hh,sctk_static_route_table,&key,sizeof(sctk_route_key_t),tmp);
  if(tmp == NULL){
    sctk_spinlock_read_lock(&sctk_route_table_lock);
    HASH_FIND(hh,sctk_dynamic_route_table,&key,sizeof(sctk_route_key_t),tmp);
    sctk_spinlock_read_lock(&sctk_route_table_lock);
  }
  
  if(tmp == NULL){
    dest = rail->route(dest,rail);
    return sctk_get_route_to_process(dest,rail);
  }

  return tmp;
}

sctk_route_table_t* sctk_get_route(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  int process;

  process = sctk_get_process_rank_from_task_rank(dest);

  tmp = sctk_get_route_to_process(process,rail);

  return tmp;
}

void sctk_route_set_rail_nb(int i){
  rails = sctk_malloc(i*sizeof(sctk_rail_info_t));
}

sctk_rail_info_t* sctk_route_get_rail(int i){
  return &(rails[i]);
}

int sctk_route_ring(int dest, sctk_rail_info_t* rail){
    int old_dest;

    old_dest = dest;
    dest = (dest + sctk_process_number -1) % sctk_process_number;
    sctk_nodebug("Route via dest - 1 %d to %d",dest,old_dest);
    
    return dest;
}
