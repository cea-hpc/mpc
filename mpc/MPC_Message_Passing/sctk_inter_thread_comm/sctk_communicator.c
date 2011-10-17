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

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/
#define SCTK_MAX_COMMUNICATOR_TAB 10
typedef struct{
  sctk_communicator_t id;

  sctk_internal_collectives_struct_t* collectives; 

  int nb_task;
  int nb_task_local;
  int last_local;
  int first_local;
  int local_tasks;

  int* local_to_global;
  int* global_to_local;
  int* task_to_process;

  sctk_spin_rwlock_t lock;

  UT_hash_handle hh;
} sctk_internal_communicator_t;

static sctk_internal_communicator_t* sctk_communicator_table = NULL;
static sctk_internal_communicator_t* sctk_communicator_array[SCTK_MAX_COMMUNICATOR_TAB];
static sctk_spin_rwlock_t sctk_communicator_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/************************************************************************/
/*Communicator accessors                                                  */
/************************************************************************/
static inline 
sctk_internal_communicator_t * 
sctk_get_internal_communicator(const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  
  if(communicator >= SCTK_MAX_COMMUNICATOR_TAB){
    sctk_spinlock_read_lock(&sctk_communicator_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&communicator,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_read_unlock(&sctk_communicator_table_lock);
  } else {
    tmp = sctk_communicator_array[communicator];
  }

  assume(tmp != NULL);

  return tmp;
}

static inline void
sctk_set_internal_communicator(const sctk_communicator_t id,
			       sctk_internal_communicator_t * tmp){
  if(id >= SCTK_MAX_COMMUNICATOR_TAB){
    sctk_spinlock_write_lock(&sctk_communicator_table_lock);
    HASH_ADD(hh,sctk_communicator_table,id,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_write_unlock(&sctk_communicator_table_lock);
  } else {
    sctk_communicator_array[id] = tmp;
  }
}

static inline void
sctk_communicator_intern_write_lock(sctk_internal_communicator_t * tmp){
  if(sctk_migration_mode)
    sctk_spinlock_write_lock(&(tmp->lock));
}

static inline void
sctk_communicator_intern_write_unlock(sctk_internal_communicator_t * tmp){
  if(sctk_migration_mode)
    sctk_spinlock_write_unlock(&(tmp->lock));
}

static inline void
sctk_communicator_intern_read_lock(sctk_internal_communicator_t * tmp){
  if(sctk_migration_mode)
    sctk_spinlock_read_lock(&(tmp->lock));
}

static inline void
sctk_communicator_intern_read_unlock(sctk_internal_communicator_t * tmp){
  if(sctk_migration_mode)
    sctk_spinlock_read_unlock(&(tmp->lock));
}


/************************************************************************/
/*Collective accessors                                                  */
/************************************************************************/
struct sctk_internal_collectives_struct_s * 
sctk_get_internal_collectives(const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  
  tmp = sctk_get_internal_communicator(communicator);

  return tmp->collectives;
}

void
sctk_set_internal_collectives(const sctk_communicator_t id,
			      struct sctk_internal_collectives_struct_s * coll){
  sctk_internal_communicator_t * tmp;
  tmp = sctk_get_internal_communicator(id);

  tmp->collectives = coll;
}

/************************************************************************/
/*INIT/DELETE                                                           */
/************************************************************************/
static inline void
sctk_communicator_init_intern(const int nb_task, const sctk_communicator_t comm,		       
			      int nb_task_local, int last_local,  
			      int first_local, int local_tasks,
			      int* local_to_global, int* global_to_local,
			      int* task_to_process){
  sctk_internal_communicator_t * tmp;
  sctk_spin_rwlock_t lock = SCTK_SPIN_RWLOCK_INITIALIZER;

  tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
  memset(tmp,0,sizeof(sctk_internal_communicator_t));
  
  tmp->collectives = NULL;

  tmp->nb_task = nb_task;
  tmp->nb_task_local = nb_task_local;
  tmp->last_local = last_local;
  tmp->first_local = first_local;
  tmp->local_tasks = local_tasks;

  tmp->local_to_global = local_to_global;
  tmp->global_to_local = global_to_local;
  tmp->task_to_process = task_to_process;

  tmp->lock = lock;

  sctk_set_internal_communicator(comm,tmp);

  sctk_collectives_init_hook(comm);
}

void sctk_communicator_init(const int nb_task){
  int i;
  int pos;
  int nb_task_local;
  int last_local;
  int first_local;
  int local_tasks;

  pos = sctk_process_rank;
  local_tasks = nb_task / sctk_process_number;
  if (local_tasks % sctk_process_number > pos)
    local_tasks++;

  first_local = sctk_process_rank * local_tasks;
  last_local = first_local + local_tasks - 1;

  sctk_communicator_init_intern(nb_task,SCTK_COMM_WORLD,nb_task_local,last_local,
				first_local,local_tasks,NULL,NULL,NULL);
}

void sctk_communicator_delete(){}

sctk_communicator_t sctk_delete_communicator (const sctk_communicator_t comm){
    not_implemented();
}

/************************************************************************/
/*Accessors                                                             */
/************************************************************************/
inline
int sctk_get_nb_task_local (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  
  tmp = sctk_get_internal_communicator(communicator);  

  return tmp->local_tasks;
}

inline
int sctk_get_nb_task_total (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  
  tmp = sctk_get_internal_communicator(communicator);

  return tmp->nb_task;
}

inline
int sctk_get_rank (const sctk_communicator_t communicator,
		   const int comm_world_rank){
  sctk_internal_communicator_t * tmp;
  
  if(communicator == SCTK_COMM_SELF){
    return 0;
  }

  tmp = sctk_get_internal_communicator(communicator);
  if(tmp->global_to_local != NULL){
    sctk_communicator_intern_read_lock(tmp);
    not_implemented();
    sctk_communicator_intern_read_unlock(tmp);
  } else {
    return comm_world_rank;
  }
}

int sctk_get_comm_world_rank (const sctk_communicator_t communicator,
			      const int rank){
  sctk_internal_communicator_t * tmp;
  
  if(communicator == SCTK_COMM_SELF){
    return 0;
  }

  tmp = sctk_get_internal_communicator(communicator);
  if(tmp->local_to_global != NULL){
    sctk_communicator_intern_read_lock(tmp);
    not_implemented();
    sctk_communicator_intern_read_unlock(tmp);
  } else {
    return rank;
  }
}

void sctk_get_rank_size_total (const sctk_communicator_t communicator,
			       int *rank, int *size, int glob_rank){
  *size = sctk_get_nb_task_total(communicator);
  *rank = sctk_get_rank(communicator,glob_rank);
}

int sctk_is_net_message (int dest){
  sctk_internal_communicator_t * tmp;
  tmp = sctk_get_internal_communicator(SCTK_COMM_WORLD);

  if(tmp->task_to_process != NULL){
    sctk_communicator_intern_read_lock(tmp);
    not_implemented();
    sctk_communicator_intern_read_unlock(tmp);
  } else {
    if((dest >= tmp->first_local) && (dest <= tmp->last_local)){
      return 0;
    } else {
      return 1; 
    }
  }
}
