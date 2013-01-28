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
#include <mpcmp.h>
#include <mpc_mpi.h>
#include <uthash.h>
#include "utarray.h"
#include <opa_primitives.h>
#include "sctk_pmi.h"

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/
#define SCTK_MAX_COMMUNICATOR_TAB 10
typedef struct sctk_internal_communicator_s{
  sctk_communicator_t id;

  sctk_internal_collectives_struct_t* collectives;
  
  /* nb task in the comm */
  int nb_task;
  
  /* min numer of local task */
  int last_local;
  /* max number of local task */
  int first_local;
  /* nb task involved with the comm for the current process */
  int local_tasks;

  int is_inter_comm;

  /* gives MPI_COMM_WORLD rank for this task */	
  int* local_to_global;
  /* gives task rank for given comm */ 
  int* global_to_local;
  /* gives id of process which schedules the task */
  int* task_to_process;
  /* process involved with the comm */
  int *process;
  /* nb of process for the comm */
  int process_nb;

  sctk_spin_rwlock_t lock;

  sctk_spinlock_t creation_lock;
  
  struct sctk_internal_communicator_s* new_comm;
  volatile int has_zero;

  OPA_int_t nb_to_delete;

  UT_hash_handle hh;
  
  /* remote group involved with  the communicator */
  int *remote_task_list_in_global_ranks;
  /* size of the remote group */
  int remote_size;
  
} sctk_internal_communicator_t;

typedef struct sctk_process_ht_s {
  int process_id;
  UT_hash_handle hh;
} sctk_process_ht_t;

static sctk_internal_communicator_t* sctk_communicator_table = NULL;
static sctk_internal_communicator_t* sctk_communicator_array[SCTK_MAX_COMMUNICATOR_TAB];
static sctk_spin_rwlock_t sctk_communicator_local_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
static sctk_spinlock_t sctk_communicator_all_table_lock = SCTK_SPINLOCK_INITIALIZER;

/************************************************************************/
/*Communicator accessors                                                */
/************************************************************************/
static inline
sctk_internal_communicator_t *
sctk_check_internal_communicator_no_lock(const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  
  //assume(communicator >= 0);
  if(communicator == -1)
	return NULL;

  if(communicator >= SCTK_MAX_COMMUNICATOR_TAB){
    sctk_spinlock_read_lock(&sctk_communicator_local_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&communicator,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_read_unlock(&sctk_communicator_local_table_lock);
  } else {
    tmp = sctk_communicator_array[communicator];
  }

  return tmp;
}
static inline
sctk_internal_communicator_t *
sctk_check_internal_communicator(const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_check_internal_communicator_no_lock(communicator);

  return tmp;
}

int get_remote_task_list_in_global_ranks(const sctk_communicator_t communicator, int i){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_check_internal_communicator_no_lock(communicator);

  return tmp->remote_task_list_in_global_ranks[i];
}

extern volatile int sctk_online_program;
static inline
sctk_internal_communicator_t *
sctk_get_internal_communicator(const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_check_internal_communicator(communicator);

  if(tmp == NULL){
    sctk_error("Communicator %d does not exist",communicator);
    if(sctk_online_program == 0){
      exit(0);
    }
    not_reachable();
  }

  return tmp;
}

static inline int
sctk_set_internal_communicator_no_lock_no_check(const sctk_communicator_t id,
			       sctk_internal_communicator_t * tmp){
  sctk_nodebug("Insert comm %d",id);
  if(id >= SCTK_MAX_COMMUNICATOR_TAB){
    sctk_spinlock_write_lock(&sctk_communicator_local_table_lock);
    HASH_ADD(hh,sctk_communicator_table,id,sizeof(sctk_communicator_t),tmp);
    sctk_spinlock_write_unlock(&sctk_communicator_local_table_lock);
  } else {
    sctk_communicator_array[id] = tmp;
  }
  return 0;
}

static inline int
sctk_del_internal_communicator_no_lock_no_check(const sctk_communicator_t id){
  sctk_internal_communicator_t * tmp;

  sctk_nodebug("Remove %d",id);

  if(id >= SCTK_MAX_COMMUNICATOR_TAB){
    sctk_spinlock_write_lock(&sctk_communicator_local_table_lock);
    HASH_FIND(hh,sctk_communicator_table,&id,sizeof(sctk_communicator_t),tmp);
    assume(tmp != NULL);
    HASH_DELETE(hh,sctk_communicator_table,tmp);
    sctk_spinlock_write_unlock(&sctk_communicator_local_table_lock);
  } else {
    sctk_communicator_array[id] = NULL;
  }
  return 0;
}

static inline int
sctk_set_internal_communicator(const sctk_communicator_t id,
			       sctk_internal_communicator_t * tmp){
  sctk_internal_communicator_t * tmp_check;
  sctk_spinlock_lock(&sctk_communicator_all_table_lock);
  tmp_check = sctk_check_internal_communicator_no_lock(id);
  if(tmp_check != NULL){
    sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
    return 1;
  }

  sctk_set_internal_communicator_no_lock_no_check(id,tmp);

  sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
  return 0;
}

static inline void
sctk_communicator_intern_write_lock(sctk_internal_communicator_t * tmp){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_write_lock(&(tmp->lock));
#endif
}

static inline void
sctk_communicator_intern_write_unlock(sctk_internal_communicator_t * tmp){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_write_unlock(&(tmp->lock));
#endif
}

static inline void
sctk_communicator_intern_read_lock(sctk_internal_communicator_t * tmp){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_read_lock(&(tmp->lock));
#endif
}

static inline void
sctk_communicator_intern_read_unlock(sctk_internal_communicator_t * tmp){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_read_unlock(&(tmp->lock));
#endif
}

static inline void sctk_comm_reduce(const sctk_communicator_t* mpc_restrict  in ,
				    sctk_communicator_t* mpc_restrict inout ,
				    size_t size ,sctk_datatype_t datatype){
  size_t i;
  for(i = 0; i < size; i++){
    if(inout[i] > in[i]) inout[i] = in[i];
  }
}

static inline
sctk_communicator_t
sctk_communicator_get_new_id(int local_root, int rank,
			     const sctk_communicator_t origin_communicator,
			     sctk_internal_communicator_t * tmp){
  sctk_communicator_t comm = -1;
  sctk_communicator_t i = 0;
  sctk_communicator_t ti = 0;
  int need_clean;

  do{
    need_clean = 0;
    if(rank == 0){
      sctk_internal_communicator_t * tmp_check;
      if((i == SCTK_COMM_WORLD) || (i == SCTK_COMM_SELF)) i++;
      if((i == SCTK_COMM_WORLD) || (i == SCTK_COMM_SELF)) i++;

      sctk_spinlock_lock(&sctk_communicator_all_table_lock);

      i--;
      do{
	i++;
	tmp_check = sctk_check_internal_communicator_no_lock(i);
      } while(tmp_check != NULL);

      comm = i;
      tmp->id = comm;
      sctk_set_internal_communicator_no_lock_no_check(i,tmp);

      sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
      sctk_nodebug("Try %d",comm);
    }

    /* Broadcast comm */
    sctk_broadcast (&comm,sizeof(sctk_communicator_t),0,origin_communicator);
    sctk_nodebug("Every one try %d",comm);

    if((local_root == 1) && (rank != 0)){
      /*Check if available*/
      sctk_internal_communicator_t * tmp_check;

      tmp->id = comm;
      sctk_spinlock_lock(&sctk_communicator_all_table_lock);
      tmp_check = sctk_check_internal_communicator_no_lock(comm);
      if(tmp_check != NULL){
	sctk_nodebug("Try %d fail",comm);
	comm = -1;
      } else {
	need_clean = 1;
	sctk_set_internal_communicator_no_lock_no_check(comm,tmp);
      }
      sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
    }

    sctk_nodebug("Every one try %d allreduce %d",comm,origin_communicator);
    /* Check if available every where*/
    ti = comm;
    sctk_all_reduce(&ti,&comm,sizeof(sctk_communicator_t),1,(MPC_Op_f)sctk_comm_reduce,
		    origin_communicator,0);

    sctk_nodebug("DONE Every one try %d allreduce %d",comm,origin_communicator);
    if(comm == -1){
      need_clean = 1;
    }

    sctk_nodebug("Every one try %d RES",comm);
    if(comm == -1){
      if((local_root == 1) && (need_clean == 1)){
	sctk_spinlock_lock(&sctk_communicator_all_table_lock);
	sctk_del_internal_communicator_no_lock_no_check(ti);
	sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
      }
    }
  }  while(comm == -1);

  assume(comm != SCTK_COMM_WORLD);
  assume(comm != SCTK_COMM_SELF);
  assume(comm > 0);
  return comm;
}


static inline
sctk_communicator_t
sctk_intercommunicator_get_new_id(int local_root, int rank, int local_leader,
			     const sctk_communicator_t origin_communicator, sctk_internal_communicator_t * tmp){
	sctk_communicator_t comm = -1;
	sctk_communicator_t i = 0;
	sctk_communicator_t ti = 0;
	int need_clean;

	do
	{
		need_clean = 0;
		if(rank == local_leader)
		{
			sctk_internal_communicator_t * tmp_check;
			if((i == SCTK_COMM_WORLD) || (i == SCTK_COMM_SELF)) i++;
			if((i == SCTK_COMM_WORLD) || (i == SCTK_COMM_SELF)) i++;

			sctk_spinlock_lock(&sctk_communicator_all_table_lock);

			i--;
			do
			{
				i++;
				tmp_check = sctk_check_internal_communicator_no_lock(i);
			} while(tmp_check != NULL);

			comm = i;
			tmp->id = comm;
			sctk_set_internal_communicator_no_lock_no_check(i,tmp);

			sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
			sctk_nodebug("Try %d",comm);
		}

		/* Broadcast comm */
		sctk_broadcast (&comm,sizeof(sctk_communicator_t),0,origin_communicator);
		sctk_nodebug("Every one try %d",comm);

		if((local_root == 1) && (rank != local_leader))
		{
			/*Check if available*/
			sctk_internal_communicator_t * tmp_check;

			tmp->id = comm;
			sctk_spinlock_lock(&sctk_communicator_all_table_lock);
			tmp_check = sctk_check_internal_communicator_no_lock(comm);
			if(tmp_check != NULL)
			{
				sctk_nodebug("Try %d fail",comm);
				comm = -1;
			} 
			else 
			{
				need_clean = 1;
				sctk_set_internal_communicator_no_lock_no_check(comm,tmp);
			}
			sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
		}

		sctk_nodebug("Every one try %d allreduce %d",comm,origin_communicator);
		/* Check if available every where*/
		ti = comm;
		sctk_all_reduce(&ti,&comm,sizeof(sctk_communicator_t),1,(MPC_Op_f)sctk_comm_reduce, origin_communicator,0);
		sctk_nodebug("DONE Every one try %d allreduce %d",comm,origin_communicator);
		sctk_nodebug("Every one try %d RES",comm);
		
		if(comm == -1)
		{
			if(local_root == 1)
			{
				sctk_spinlock_lock(&sctk_communicator_all_table_lock);
				sctk_del_internal_communicator_no_lock_no_check(ti);
				sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
			}
		}
	}  while(comm == -1);

	assume(comm != SCTK_COMM_WORLD);
	assume(comm != SCTK_COMM_SELF);
	assume(comm > 0);
	sctk_nodebug("return comm = %d", comm);
	return comm;
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

  sctk_nodebug("Communicator %d",id);
  tmp = sctk_get_internal_communicator(id);

  tmp->collectives = coll;
}

/************************************************************************/
/*INIT/DELETE                                                           */
/************************************************************************/
static int int_cmp(const void *a, const void *b)
{
  const int *ia = (const int *)a;
  const int *ib = (const int *)b;
  return *ia  - *ib;
}

static inline void
sctk_communicator_init_intern_init_only(const int nb_task,
						 int last_local,
						 int first_local, int local_tasks,
						 int* local_to_global, int* global_to_local,
						 int* task_to_process,
             int *process_array,
             int process_nb,
						 sctk_internal_communicator_t * tmp){
  sctk_spin_rwlock_t lock = SCTK_SPIN_RWLOCK_INITIALIZER;
  sctk_spinlock_t spinlock = SCTK_SPINLOCK_INITIALIZER;


  tmp->collectives = NULL;

  tmp->nb_task = nb_task;
  tmp->last_local = last_local;
  tmp->first_local = first_local;
  tmp->local_tasks = local_tasks;
  tmp->process = process_array;
  tmp->process_nb = process_nb;
  if (process_array != NULL) {
    qsort(process_array, process_nb, sizeof(int), int_cmp);
  }

  tmp->local_to_global = local_to_global;
  tmp->global_to_local = global_to_local;
  tmp->task_to_process = task_to_process;
  tmp->is_inter_comm = 0;

  tmp->lock = lock;

  tmp->creation_lock = spinlock;
  tmp->new_comm = NULL;
  tmp->has_zero = 0;
  OPA_store_int(&(tmp->nb_to_delete),0);
}

static inline void
sctk_communicator_init_intern_no_alloc(const int nb_task, const sctk_communicator_t comm,
				        int last_local,
				       int first_local, int local_tasks,
				       int* local_to_global, int* global_to_local,
				       int* task_to_process,
               int *process_array,
               int process_nb,
				       sctk_internal_communicator_t * tmp){

  sctk_communicator_init_intern_init_only(nb_task,
					  last_local,
					  first_local,
					  local_tasks,
					  local_to_global,
					  global_to_local,
					  task_to_process,
            process_array,
            process_nb,
					  tmp);
  assume(sctk_set_internal_communicator(comm,tmp) == 0);

  sctk_collectives_init_hook(comm);
}

static inline void
sctk_communicator_init_intern(const int nb_task, const sctk_communicator_t comm,
			       int last_local,
			      int first_local, int local_tasks,
			      int* local_to_global, int* global_to_local,
			      int* task_to_process,
            int *process_array,
            int process_nb){
  sctk_internal_communicator_t * tmp;
  tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
  memset(tmp,0,sizeof(sctk_internal_communicator_t));

  sctk_communicator_init_intern_no_alloc(nb_task,
					 comm,
					 last_local,
					 first_local,
					 local_tasks,
					 local_to_global,
					 global_to_local,
					 task_to_process,
           process_array,
           process_nb,
					 tmp);
}

/*
 * Initialize the MPI_COMM_SELF communicator
 */
void sctk_communicator_self_init(){
  const int last_local = 0;
  const int first_local = 0;
  const int local_tasks = 1;
  int *process_array;

  /* Construct the list of processes */
  process_array = sctk_malloc (sizeof(int));
  *process_array = sctk_process_rank;

  sctk_communicator_init_intern(1,SCTK_COMM_SELF,last_local,
				first_local,local_tasks,NULL,NULL,NULL,process_array, 1);
}


/*
 * Initialize the MPI_COMM_WORLD communicator
 */
void sctk_communicator_world_init(const int nb_task){
  int i;
  int pos;
  int last_local;
  int first_local;
  int local_tasks;
  int *process_array;

  pos = sctk_process_rank;
  local_tasks = nb_task / sctk_process_number;
  sctk_nodebug("1: local_tasks %d",local_tasks);
  if (nb_task % sctk_process_number > pos)
    local_tasks++;
  sctk_nodebug("2: local_tasks %d",local_tasks);

  first_local = sctk_process_rank * local_tasks;

  if(nb_task % sctk_process_number <= pos){
    first_local += nb_task % sctk_process_number;
  }

  last_local = first_local + local_tasks - 1;
  /* Construct the list of processes */
  process_array = sctk_malloc (sctk_process_number * sizeof(int));
  for (i=0; i < sctk_process_number; ++i) process_array[i] = i;

  sctk_communicator_init_intern(nb_task,SCTK_COMM_WORLD,last_local,
				first_local,local_tasks,NULL,NULL,NULL,process_array,sctk_process_number);
  if(sctk_process_number > 1){
    sctk_pmi_barrier();
  }
}

void sctk_communicator_delete(){
  if(sctk_process_number > 1){
    sctk_pmi_barrier();
  }
}

sctk_communicator_t sctk_delete_communicator (const sctk_communicator_t comm){
  if( (comm == SCTK_COMM_WORLD) || (comm == SCTK_COMM_SELF) ){
    not_reachable();
    return comm;
  } else {
    sctk_internal_communicator_t * tmp;
    int is_master = 0;
    int val;
    int max_val;
    sctk_barrier (comm);
    tmp = sctk_get_internal_communicator(comm);
    sctk_barrier (comm);

    max_val = tmp->local_tasks;
    val = OPA_fetch_and_incr_int(&(tmp->nb_to_delete));

    if(val == max_val - 1){
      is_master = 1;
      sctk_spinlock_lock(&sctk_communicator_all_table_lock);
      sctk_free(tmp->local_to_global);
      sctk_free(tmp->global_to_local);
      sctk_free(tmp->task_to_process);
      if(tmp->is_inter_comm == 1)
		sctk_free(tmp->remote_task_list_in_global_ranks);

      sctk_del_internal_communicator_no_lock_no_check(comm);
      sctk_free(tmp);
      sctk_spinlock_unlock(&sctk_communicator_all_table_lock);
    } else {
      is_master = 0;
    }

    return MPC_COMM_NULL;
  }
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
int sctk_get_last_task_local (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->last_local;
}

inline
int sctk_get_first_task_local (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->first_local;
}

inline
int sctk_get_process_nb_in_array (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->process_nb;
}

inline
int *sctk_get_process_array (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->process;
}

inline
int sctk_get_nb_task_total (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->nb_task;
}

inline 
int sctk_get_nb_task_remote (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;
  tmp = sctk_get_internal_communicator(communicator);
  return tmp->remote_size;
}

inline
int sctk_get_rank (const sctk_communicator_t communicator,
		   const int comm_world_rank){
  sctk_internal_communicator_t * tmp;
  int ret;

  if (communicator == SCTK_COMM_WORLD) { /* COMM_WORLD communicator */
    return comm_world_rank;
  } else if (communicator == SCTK_COMM_SELF) { /* COMM_SELF communicator */
    return 0;
  }

  tmp = sctk_get_internal_communicator(communicator);
  if(tmp->global_to_local != NULL){
    sctk_communicator_intern_read_lock(tmp);
    sctk_nodebug("comm %d rank %d local %d",communicator,comm_world_rank,
        tmp->global_to_local[comm_world_rank]);
    ret = tmp->global_to_local[comm_world_rank];
    sctk_communicator_intern_read_unlock(tmp);
    return ret;
  } else {
    return comm_world_rank;
  }
}

int sctk_get_comm_world_rank (const sctk_communicator_t communicator,
			      const int rank){
  sctk_internal_communicator_t * tmp;
  int ret;

  if (communicator == SCTK_COMM_WORLD) { /* COMM_WORLD communicator */
    return rank;
  } else if (communicator == SCTK_COMM_SELF) { /* COMM_WORLD communicator */
    return rank;
  }

  /* Other communicators */
  tmp = sctk_get_internal_communicator(communicator);
  if(tmp->local_to_global != NULL){
    sctk_communicator_intern_read_lock(tmp);
    ret= tmp->local_to_global[rank];
    sctk_communicator_intern_read_unlock(tmp);
    return ret;
  } else {
    return rank;
  }
}

void sctk_get_rank_size_total (const sctk_communicator_t communicator,
			       int *rank, int *size, int glob_rank){
  *size = sctk_get_nb_task_total(communicator);
  *rank = sctk_get_rank(communicator,glob_rank);
}

int sctk_get_process_rank_from_task_rank(int rank){
  sctk_internal_communicator_t * tmp;
  int proc_rank;

  tmp = sctk_get_internal_communicator(SCTK_COMM_WORLD);
  if(tmp->task_to_process != NULL){
    sctk_communicator_intern_read_lock(tmp);
    proc_rank = tmp->task_to_process[rank];
    sctk_communicator_intern_read_unlock(tmp);
  } else {
    int local_tasks;
    int remain_tasks;
    local_tasks = tmp->nb_task / sctk_process_number;
    remain_tasks = tmp->nb_task % sctk_process_number;

    if(rank < (local_tasks + 1) * remain_tasks){
      proc_rank = rank / (local_tasks + 1);
    } else {
      proc_rank = remain_tasks + ((rank - (local_tasks + 1) * remain_tasks) / local_tasks);
    }
  }
  return proc_rank;
}

/************************************************************************/
/*Communicator creation                                                 */
/************************************************************************/
sctk_communicator_t
sctk_duplicate_communicator (const sctk_communicator_t origin_communicator,
			     int is_inter_comm,int rank){
  if(is_inter_comm == 0){
    sctk_internal_communicator_t * tmp;
    sctk_internal_communicator_t * new_tmp;
    int local_root = 0;
    sctk_communicator_t comm;

    sctk_barrier (origin_communicator);

    tmp = sctk_get_internal_communicator(origin_communicator);

    sctk_spinlock_lock(&(tmp->creation_lock));
    if(tmp->new_comm == NULL){
      local_root = 1;
      tmp->has_zero = 0;
      new_tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
      memset(new_tmp,0,sizeof(sctk_internal_communicator_t));
      tmp->new_comm = new_tmp;
      sctk_communicator_init_intern_init_only(tmp->nb_task,
					      tmp->last_local,
					      tmp->first_local,
					      tmp->local_tasks,
					      tmp->local_to_global,
					      tmp->global_to_local,
					      tmp->task_to_process,
                /* FIXME: process and process_nb have not been
                 * tested for now. Could aim to trouble */
                tmp->process,
                tmp->process_nb,
					      tmp->new_comm);
    }
    sctk_spinlock_unlock(&(tmp->creation_lock));
    sctk_barrier (origin_communicator);
    if(rank == 0){
      tmp->has_zero = 1;
      local_root = 1;
    }
    sctk_barrier (origin_communicator);
    if((tmp->has_zero == 1) && (rank != 0)){
      local_root = 0;
    }

    new_tmp = tmp->new_comm;

    comm = sctk_communicator_get_new_id(local_root,rank,origin_communicator,new_tmp);

    if(local_root){
      /* Check if the communicator has been well created */
      sctk_get_internal_communicator(comm);
      assume(comm >= 0);
      sctk_nodebug("Init collectives for comm %d",comm);
      sctk_collectives_init_hook(comm);
    }

    sctk_barrier (origin_communicator);
    tmp->new_comm = NULL;
    sctk_barrier (origin_communicator);
    sctk_barrier (new_tmp->id);
    assume(new_tmp->id != origin_communicator);
    return new_tmp->id;
  } else {
    not_implemented();
  }
  return MPC_COMM_NULL;
}


sctk_communicator_t sctk_create_intercommunicator (const sctk_communicator_t local_comm, const int local_leader,
		       const sctk_communicator_t peer_comm, const int remote_leader, const int tag)
{
	MPC_Status status;
	int local_size = 0, remote_size = 0, save_remote_size = 0;
	sctk_internal_communicator_t * tmp;
    sctk_internal_communicator_t * new_tmp;
    int i;
    int local_root = 0;
    int nb_task_involved;
    sctk_communicator_t comm;
    sctk_thread_data_t *thread_data;
    int rank = 0, grank = 0;
    int *remote_task_list;

	/* get task rank (COMM_WORLD) */
    thread_data = sctk_thread_data_get ();
    rank = thread_data->task_id;
    sctk_barrier (local_comm);
    /* get the local group rank of the task */
    grank = sctk_get_rank(local_comm, rank);
	/* get comm struct */
    tmp = sctk_get_internal_communicator(local_comm);
	nb_task_involved = tmp->nb_task;
	local_size = tmp->nb_task;
	  
    sctk_spinlock_lock(&(tmp->creation_lock));
    if(tmp->new_comm == NULL)
    {
		int local_tasks = 0;
		int* local_to_global;
		int* global_to_local;
		int* task_to_process;
		sctk_process_ht_t *process = NULL;
		sctk_process_ht_t *tmp_process = NULL;
		sctk_process_ht_t *current_process = NULL;
		int *process_array;
		int process_nb = 0;

		local_root = 1;
		tmp->has_zero = 0;
		/* allocate new comm */
		new_tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
		memset(new_tmp,0,sizeof(sctk_internal_communicator_t));

		local_to_global = sctk_malloc(nb_task_involved*sizeof(int));
		global_to_local = sctk_malloc(sctk_get_nb_task_total(SCTK_COMM_WORLD)*sizeof(int));
		task_to_process = sctk_malloc(nb_task_involved*sizeof(int));
		
		/* fill new comm */
		for(i = 0; i < nb_task_involved; i++)
		{
			local_to_global[i] = tmp->local_to_global[i];
			global_to_local[local_to_global[i]] = i;
			task_to_process[i] = sctk_get_process_rank_from_task_rank(local_to_global[i]);
			if(task_to_process[i] == sctk_process_rank)
			{
				local_tasks++;
			}
			/* build list of processes */
			HASH_FIND_INT(process, &task_to_process[i], tmp_process);
			if (tmp_process == NULL) 
			{
				tmp_process = sctk_malloc (sizeof(sctk_process_ht_t));
				tmp_process->process_id = task_to_process[i];
				HASH_ADD_INT(process, process_id, tmp_process);
				process_nb ++;
			}
		}
		process_array = sctk_malloc (process_nb * sizeof(int));
		i = 0;
		HASH_ITER(hh, process, current_process, tmp_process) 
		{
			process_array[i++] = current_process->process_id;
			HASH_DEL(process, current_process);
			free(current_process);
		}

		tmp->new_comm = new_tmp;
		/* init new comm */
		sctk_communicator_init_intern_init_only(nb_task_involved, -1, -1, local_tasks,
			local_to_global, global_to_local, task_to_process, process_array, process_nb, tmp->new_comm);
		
		tmp->remote_size = 0;
    }
    sctk_spinlock_unlock(&(tmp->creation_lock));
    sctk_barrier (local_comm);
    if(grank == local_leader)
    {
		tmp->has_zero = 1;
		local_root = 1;
    }
    sctk_barrier (local_comm);
    if((tmp->has_zero == 1) && (grank != local_leader))
    {
		local_root = 0;
    }

    new_tmp = tmp->new_comm;
    
    tmp->is_inter_comm = 1;
    
    if (grank == local_leader)
    {
		PMPI_Sendrecv(&local_size, 1, MPC_INT, remote_leader, tag, &remote_size, 1, MPC_INT, remote_leader, tag, peer_comm, &status);
	}
	
	sctk_broadcast (&remote_size,sizeof(int),local_leader,local_comm);
	save_remote_size = remote_size;
	new_tmp->remote_size = remote_size;
	assert(remote_size != 0);
	
	remote_task_list= sctk_malloc(remote_size*sizeof(int));
	if(grank == local_leader)
    {
		int *task_list = sctk_malloc(remote_size*sizeof(int));
		for(i = 0; i < local_size; i++)
		{
			task_list[i] = new_tmp->local_to_global[i];
		}
		PMPI_Sendrecv(&task_list, local_size, MPC_INT, remote_leader, tag, &remote_task_list, remote_size, MPC_INT, remote_leader, tag, peer_comm, &status);
	}

	sctk_broadcast(&remote_task_list, save_remote_size*sizeof(int), local_leader, local_comm);
	
	new_tmp->remote_task_list_in_global_ranks = remote_task_list;

	sctk_barrier(local_comm);
   
    sctk_nodebug("Determine New comm");
	comm = sctk_intercommunicator_get_new_id(local_root,grank,local_leader,local_comm,new_tmp);
    //~ sctk_debug("done comm = %d\n", comm);
    
    sctk_barrier(local_comm);
    
    if(local_root)
    {
      
		sctk_get_internal_communicator(comm);
		assume(comm >= 0);
		sctk_nodebug("Init collectives for comm %d",comm);
		sctk_collectives_init_hook(comm);
    }

	
    sctk_nodebug("New comm %d",comm);
    sctk_barrier (local_comm);
    tmp->new_comm = NULL;
    sctk_barrier (local_comm);
    assume(new_tmp->id != local_comm);
    
    sctk_barrier(local_comm);
    for(i = 0; i < nb_task_involved; i++)
    {
		if(new_tmp->local_to_global[i] == rank)
		{
			sctk_nodebug("rang %d : new_tmp->id = %d", rank, new_tmp->id);
			return new_tmp->id;
		}
    }
    sctk_nodebug("rang %d : MPC_COMM_NULL", rank);
    return MPC_COMM_NULL;
}

sctk_communicator_t sctk_create_communicator (const sctk_communicator_t
					      origin_communicator,
					      const int
					      nb_task_involved,
					      const int *task_list,
					      int is_inter_comm){
  if(is_inter_comm == 0){
    sctk_internal_communicator_t * tmp;
    sctk_internal_communicator_t * new_tmp;
    int local_root = 0;
    sctk_communicator_t comm;
    sctk_thread_data_t *thread_data;
    int i;
    int rank;

	/* get task id */
    thread_data = sctk_thread_data_get ();
    rank = thread_data->task_id;
    sctk_barrier (origin_communicator);
	/* get comm struct */
    tmp = sctk_get_internal_communicator(origin_communicator);

    sctk_spinlock_lock(&(tmp->creation_lock));
    if(tmp->new_comm == NULL){
      int local_tasks = 0;
      int* local_to_global;
      int* global_to_local;
      int* task_to_process;
      sctk_process_ht_t *process = NULL;
      sctk_process_ht_t *tmp_process = NULL;
      sctk_process_ht_t *current_process = NULL;
      int *process_array;
      int process_nb = 0;

      local_root = 1;
      tmp->has_zero = 0;
      /* allocate new comm */
      new_tmp = sctk_malloc(sizeof(sctk_internal_communicator_t));
      memset(new_tmp,0,sizeof(sctk_internal_communicator_t));

      local_to_global = sctk_malloc(nb_task_involved*sizeof(int));
      global_to_local = sctk_malloc(sctk_get_nb_task_total(SCTK_COMM_WORLD)*sizeof(int));
      task_to_process = sctk_malloc(nb_task_involved*sizeof(int));

	  /* fill new comm */
      for(i = 0; i < nb_task_involved; i++){
        local_to_global[i] = task_list[i];
        global_to_local[local_to_global[i]] = i;
        task_to_process[i] = sctk_get_process_rank_from_task_rank(local_to_global[i]);
        if(task_to_process[i] == sctk_process_rank){
          local_tasks++;
        }
        /* build list of processes */
        HASH_FIND_INT(process, &task_to_process[i], tmp_process);
        if (tmp_process == NULL) {
          tmp_process = sctk_malloc (sizeof(sctk_process_ht_t));
          tmp_process->process_id = task_to_process[i];
          HASH_ADD_INT(process, process_id, tmp_process);
          process_nb ++;
        }
      }
      process_array = sctk_malloc (process_nb * sizeof(int));
      i = 0;
      HASH_ITER(hh, process, current_process, tmp_process) {
        process_array[i++] = current_process->process_id;
        HASH_DEL(process, current_process);
        free(current_process);
      }

      tmp->new_comm = new_tmp;
      /* init new comm */
     sctk_communicator_init_intern_init_only(nb_task_involved,
					      -1,
					      -1,
					      local_tasks,
					      local_to_global,
					      global_to_local,
					      task_to_process,
						  process_array,
						  process_nb,
					      tmp->new_comm);
	  tmp->remote_size = 0;
	  tmp->remote_task_list_in_global_ranks = NULL;
    }
    sctk_spinlock_unlock(&(tmp->creation_lock));
    sctk_barrier (origin_communicator);
    if(rank == 0){
      tmp->has_zero = 1;
      local_root = 1;
    }
    sctk_barrier (origin_communicator);
    if((tmp->has_zero == 1) && (rank != 0)){
      local_root = 0;
    }


    new_tmp = tmp->new_comm;
    sctk_nodebug("Determine New comm");

    comm = sctk_communicator_get_new_id(local_root,rank,origin_communicator,new_tmp);

    if(local_root){
      /* Check if the communicator has been well created */
      sctk_get_internal_communicator(comm);
      assume(comm >= 0);
      sctk_nodebug("Init collectives for comm %d",comm);
      sctk_collectives_init_hook(comm);
    }

    sctk_nodebug("New comm %d",comm);
    sctk_barrier (origin_communicator);
    tmp->new_comm = NULL;
    sctk_barrier (origin_communicator);
    assume(new_tmp->id != origin_communicator);

    /*If not involved return MPC_COMM_NULL*/
    for(i = 0; i < nb_task_involved; i++){
      if(task_list[i] == rank){
	sctk_barrier (new_tmp->id);
	sctk_nodebug("new_tmp->id");
	return new_tmp->id;
      }
    }
    sctk_nodebug("MPC_COMM_NULL");
    return MPC_COMM_NULL;
  }
  return MPC_COMM_NULL;
}
int sctk_is_inter_comm (const sctk_communicator_t communicator){
  sctk_internal_communicator_t * tmp;

  tmp = sctk_get_internal_communicator(communicator);

  return tmp->is_inter_comm;
}
