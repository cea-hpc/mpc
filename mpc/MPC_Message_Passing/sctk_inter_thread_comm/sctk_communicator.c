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


#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <uthash.h>

typedef struct{
  sctk_communicator_t comm_id;
  int nb_task_local;
  int sctk_last_local;
  int sctk_first_local;
  int sctk_local_tasks;
  UT_hash_handle hh;
} sctk_internal_communicator_t;

static sctk_internal_communicator_t* sctk_communicator_table = NULL;
static sctk_spin_rwlock_t sctk_communicator_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

static int sctk_total_task_number = 0;
static int sctk_last_local = 0;
static int sctk_first_local = 0;
static int sctk_local_tasks = 0;

static void sctk_init_internal_communicator(int comm_id, int nb_task_local){
  not_implemented();
}

void sctk_communicator_init(const int nb_task){
  int i;
  int pos;
  sctk_total_task_number = nb_task;

  pos = sctk_process_rank;
  sctk_local_tasks = sctk_total_task_number / sctk_process_number;
  if (sctk_local_tasks % sctk_process_number > pos)
    sctk_local_tasks++;

  sctk_first_local = sctk_process_rank * sctk_local_tasks;
  sctk_last_local = sctk_first_local + sctk_local_tasks - 1;

  sctk_collectives_init(SCTK_COMM_WORLD);
}

static int sctk_get_nb_task_local_dup_world(){
  return sctk_local_tasks;
}

static int sctk_get_nb_task_total_dup_world(){
  return sctk_total_task_number;
}

int sctk_get_nb_task_local (const sctk_communicator_t communicator){
  if(sctk_communicator_table == NULL){
    if(communicator == SCTK_COMM_SELF){
      return 1;
    } else {
      return sctk_get_nb_task_local_dup_world();
    }
  } else {
    sctk_internal_communicator_t* tmp; 

    sctk_spinlock_read_lock(&sctk_communicator_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&communicator,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_read_unlock(&sctk_communicator_table_lock);
    if(tmp != NULL){
      not_implemented();
    } else {
      return sctk_get_nb_task_local_dup_world();
    }
  }
}

int sctk_get_nb_task_total (const sctk_communicator_t communicator){
  if(sctk_communicator_table == NULL){
    if(communicator == SCTK_COMM_SELF){
      return 1;
    } else {
      return sctk_get_nb_task_total_dup_world();
    }
  } else {
    sctk_internal_communicator_t* tmp; 

    sctk_spinlock_read_lock(&sctk_communicator_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&communicator,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_read_unlock(&sctk_communicator_table_lock);
    if(tmp != NULL){
      not_implemented();
    } else {
      return sctk_get_nb_task_total_dup_world();
    }
  }
}

int sctk_get_rank (const sctk_communicator_t communicator,
		   const int comm_world_rank){
  if(sctk_communicator_table == NULL){
    if(communicator == SCTK_COMM_SELF){
      return 0;
    } else {
      return comm_world_rank;
    }
  } else {
    sctk_internal_communicator_t* tmp; 

    sctk_spinlock_read_lock(&sctk_communicator_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&communicator,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_read_unlock(&sctk_communicator_table_lock);
    if(tmp != NULL){
      not_implemented();
    } else {
      return comm_world_rank;
    }
  }
}
