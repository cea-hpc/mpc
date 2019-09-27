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

#include "sctk_thread_sem.h"
#include <sctk_thread_generic.h>
#include <limits.h>
#include <errno.h>

extern int errno;
static mpc_common_spinlock_t sem_named_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_thread_generic_sem_named_list_t* named_list_head = NULL;

void sctk_thread_generic_sems_init(){
  sctk_thread_generic_check_size (sctk_thread_generic_sem_t, sctk_thread_cond_t);

  {
	static sctk_thread_generic_sem_t loc = SCTK_THREAD_GENERIC_SEM_INIT;
	static sctk_thread_sem_t glob ;
	assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_sem_t)) == 0);
  }
}

int
sctk_thread_generic_sems_sem_init( sctk_thread_generic_sem_t* sem, 
                        int pshared, unsigned int value ){

  /*
	ERRORS:
	EINVAL value exceeds SEM_VALUE_MAX
	ENOSYS pshared is nonzero, but the system does not support 
		   process-shared semaphores
	*/

  if( value > SCTK_SEM_VALUE_MAX ){
	errno = SCTK_EINVAL;
	return -1;
  }

  sctk_thread_generic_sem_t local = SCTK_THREAD_GENERIC_SEM_INIT;
  sctk_thread_generic_sem_t* local_ptr = &local;

  if( pshared == SCTK_THREAD_PROCESS_SHARED ){
	errno = SCTK_ENOTSUP;
	return -1;
  }

  local_ptr->lock = value;
  (*sem) = local;

  return 0;
}

int
sctk_thread_generic_sems_sem_wait( sctk_thread_generic_sem_t* sem,   
                        sctk_thread_generic_scheduler_t* sched ){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	EINTR  |>NOT IMPLEMENTED<| The call was interrupted by a signal handler
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  int ret = 0;
  void** tmp = (void**) sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;
  sctk_thread_generic_mutex_cell_t cell;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  mpc_common_spinlock_lock(&(sem->spinlock));
  if( sem->lock > 0 ){
	sem->lock--;
	mpc_common_spinlock_unlock(&(sem->spinlock));
	return ret;
  }

  cell.sched = sched;
  DL_APPEND( sem->list, &cell );
  sctk_thread_generic_thread_status(sched,sctk_thread_generic_blocked);
  sctk_nodebug("WAIT SEM LOCK sleep %p",sched);
  tmp[sctk_thread_generic_sem] = (void*) sem;
  sctk_thread_generic_register_spinlock_unlock(sched,&(sem->spinlock));
  sctk_thread_generic_sched_yield(sched);
  tmp[sctk_thread_generic_sem] = NULL;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  return ret;
}

int
sctk_thread_generic_sems_sem_trywait( sctk_thread_generic_sem_t* sem,
                        __UNUSED__ sctk_thread_generic_scheduler_t* sched ){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	EAGAIN The operation could not be performed without blocking
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  int ret = 0;

  if( mpc_common_spinlock_trylock(&(sem->spinlock)) == 0 ){
	if( sem->lock > 0 ){
	  sem->lock--;
	} else {
	  errno = SCTK_EAGAIN;
	  ret = -1;
	}
	mpc_common_spinlock_unlock(&(sem->spinlock));
  } else {
	errno = SCTK_EAGAIN;
	ret = -1;
  }

  return ret;
}

int
sctk_thread_generic_sems_sem_timedwait( sctk_thread_generic_sem_t* sem,
                        const struct timespec* time,
                        sctk_thread_generic_scheduler_t* sched ){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct or the value 
		   of nsecs in time argument is less than 0, or greater than or equal 
		   to 1000 million
	ETIMEDOUT The call timed out before the semaphore could be locked
	EINTR  |>NOT IMPLEMENTED<| The call was interrupted by a signal handler
	*/

  if( sem == NULL || time == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  if( time->tv_nsec < 0 || time->tv_nsec >= 1000000000 ){
	errno = SCTK_EINVAL;
	return -1;
  }

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  int ret = 0;
  struct timespec t_current;

  do{
	if( mpc_common_spinlock_trylock(&(sem->spinlock)) == 0 ){
	  if( sem->lock > 0 ){
		sem->lock--;
		errno = 0;
		ret = 0;
		mpc_common_spinlock_unlock(&(sem->spinlock));
		return ret;
	  } else {
		errno = SCTK_EAGAIN;
		ret = -1;
	  }
	  mpc_common_spinlock_unlock(&(sem->spinlock));
	} else {
	  errno = SCTK_EAGAIN;
	  ret = -1;
	}
	/* test cancel */
	sctk_thread_generic_check_signals( 0 );
	clock_gettime( CLOCK_REALTIME, &t_current );
  } while ( ret != 0 && ( t_current.tv_sec < time->tv_sec ||
		( t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec ) ) );
  
  if( ret != 0 ){
	errno = SCTK_ETIMEDOUT;
  }

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  if( sched->th->attr.nb_sig_pending > 0 ){
	errno = SCTK_EINTR;
  }

  return ret;
}

int
sctk_thread_generic_sems_sem_post( sctk_thread_generic_sem_t* sem,
                        __UNUSED__ sctk_thread_generic_scheduler_t* sched ){

  /*
	ERRORS:
	EINVAL     The value specified for the argument is not correct
	EOVERFLOW  The maximum allowable value for a semaphore would be exceeded
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  int ret = 0;
  sctk_thread_generic_mutex_cell_t* head;

  mpc_common_spinlock_lock(&(sem->spinlock));
  if( sem->list != NULL ){
	head = sem->list;
	DL_DELETE( sem->list, head );
	if(head->sched->status != sctk_thread_generic_running){
	  sctk_nodebug("ADD SEM UNLOCK wake %p",head->sched);
	  sctk_thread_generic_wake(head->sched);
	}
  } else {
	sem->lock++;
	if( sem->lock > SCTK_SEM_VALUE_MAX ){
	  errno = SCTK_EOVERFLOW;
	  ret = -1;
	}
  }
  mpc_common_spinlock_unlock(&(sem->spinlock));

  return ret;
}

int
sctk_thread_generic_sems_sem_getvalue( sctk_thread_generic_sem_t* sem,
                        int* sval ){

  /*
	ERRORS:
	EINVAL     The value specified for the argument is not correct
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  (*sval) = sem->lock;

  return 0;
}

int
sctk_thread_generic_sems_sem_destroy( sctk_thread_generic_sem_t* sem ){

  /*
	ERRORS:
	EINVAL     The value specified for the argument is not correct
	EBUSY      Threads are currently waiting on the semaphore argument 
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  if( sem->list != NULL ){
	errno = SCTK_EBUSY;
	return -1;
  }

  return 0;
}

sctk_thread_generic_sem_t*
sctk_thread_generic_sems_sem_open( const char* name, int oflag, ...){

  /*
	ERRORS:
	EACCES The semaphore exists, but the caller does not have permission to open it
	EEXIST Both O_CREAT and O_EXCL were specified in oflag, but a semaphore 
		   with this name already exists
	EINVAL value was greater than SEM_VALUE_MAX or name consists of just "/", 
		   followed by no other characters
	EMFILE |>NOT IMPLEMENTED<| The process already has the maximum number of files 
		   and open
	ENAMETOOLONG name was too long
	ENFILE |>NOT IMPLEMENTED<| The system limit on the total number of open files 
		   has been reached
	ENOENT The O_CREAT flag was not specified in oflag and no semaphore with 
		   this name exists; or, O_CREAT was specified, but name wasn't well formed
	ENOMEM Insufficient memory
	*/

  if( name == NULL ){
	errno = SCTK_EINVAL;
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  if( name[0] != '/' || (name[0] == '/' && name[1] == '\0') ){
	errno = SCTK_EINVAL;
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  int length = strlen( name );

  if( length > NAME_MAX-4 ){
	errno = SCTK_ENAMETOOLONG;
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  char* _name = (char *) sctk_malloc (length * sizeof (char));
  if( _name == NULL ){
	errno = SCTK_ENOMEM;
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  strcpy( _name, name + 1 );
  _name[length] = '\0';

  sctk_thread_generic_sem_t* sem_tmp;
  sctk_thread_generic_sem_named_list_t* sem_named_tmp;

  mpc_common_spinlock_lock( &sem_named_lock );
  sctk_thread_generic_sem_named_list_t* list = named_list_head;

  if( list != NULL ){
	DL_FOREACH( named_list_head, list ){
	  if( strcmp( list->name, _name ) == 0 
			  && list->unlink == 0 ) break;
	}
  }

  sctk_nodebug("la recherche a trouvé : %p , la création est à  : %d \n",
		  list, flags & SCTK_O_CREAT);

  if( list != NULL && ((oflag & SCTK_O_CREAT) 
			  && (oflag & SCTK_O_EXCL))){
	mpc_common_spinlock_unlock( &sem_named_lock );
	sctk_free( _name );
	errno = SCTK_EEXIST;
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  if( list != NULL ){
	if( oflag & SCTK_O_CREAT ){
	  va_list args;
	  va_start( args, oflag );
	  mode_t mode = va_arg( args, mode_t );
	  va_end(args);
	  if( (list->mode & mode) != mode ){
		sctk_free( _name );
		errno = SCTK_EACCES;
		mpc_common_spinlock_unlock( &sem_named_lock );
		return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
	  }
	}
	list->nb++;
	mpc_common_spinlock_unlock( &sem_named_lock );
	sctk_free (_name);
	return list->sem;
  } 
  else if( oflag & SCTK_O_CREAT ){
	va_list args;
	va_start( args, oflag );
	mode_t mode = va_arg( args, mode_t );
	unsigned int value = va_arg( args, int );
	va_end(args);

	sem_tmp = (sctk_thread_generic_sem_t*)
		sctk_malloc (sizeof (sctk_thread_generic_sem_t));
	
	if( sem_tmp == NULL ){
	  errno = SCTK_ENOMEM;
	  sctk_free (_name);
	  mpc_common_spinlock_unlock( &sem_named_lock );
	  return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
	}

	sem_named_tmp = (sctk_thread_generic_sem_named_list_t*)
		sctk_malloc (sizeof (sctk_thread_generic_sem_named_list_t));

	if( sem_named_tmp == NULL ){
	  errno = SCTK_ENOMEM;
	  sctk_free (_name);
	  sctk_free (sem_tmp);
	  mpc_common_spinlock_unlock( &sem_named_lock );
	  return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
	}

	int ret = sctk_thread_generic_sems_sem_init( sem_tmp, 0, value );

	if( ret != 0 ){
	  mpc_common_spinlock_unlock( &sem_named_lock );
	  return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
	}

	sem_named_tmp->name = _name;
	sem_named_tmp->nb = 1;
	sem_named_tmp->unlink = 0;
	sem_named_tmp->mode = mode;
	sem_named_tmp->sem = sem_tmp;

	DL_APPEND( named_list_head, sem_named_tmp );

	mpc_common_spinlock_unlock( &sem_named_lock );

	sctk_nodebug ("head %p pointe sur sem %p named %s\n",
			named_list_head, named_list_head->sem, head->name);
  }
  else {
	sctk_free( _name );
	errno = SCTK_ENOENT;
	mpc_common_spinlock_unlock( &sem_named_lock );
	return (sctk_thread_generic_sem_t*) SCTK_SEM_FAILED;
  }

  sctk_nodebug ("sem vaux %p avec une valeur de %d", sem_named_tmp->sem, sem_named_tmp->sem->lock);

  return sem_named_tmp->sem;
}

int
sctk_thread_generic_sems_sem_close( sctk_thread_generic_sem_t* sem ){

  /*
	ERRORS:
	EINVAL     The value specified for the argument is not correct
	*/

  if( sem == NULL ){
	errno = SCTK_EINVAL;
	return -1;
  }

  mpc_common_spinlock_lock( &sem_named_lock );
  sctk_thread_generic_sem_named_list_t* list = named_list_head;

  if( list != NULL ){
	DL_FOREACH( named_list_head, list ){
	  if( sem == list->sem ) break;
	}
  }

  if( list == NULL ){
	errno = SCTK_EINVAL;
	mpc_common_spinlock_unlock( &sem_named_lock );
	return -1;
  }

  list->nb--;

  if( list->nb == 0 && list->unlink != 0 ){
	DL_DELETE( named_list_head, list );
	sctk_free( list->sem );
	sctk_free( list );	
  }

  mpc_common_spinlock_unlock( &sem_named_lock );

  return 0;
}

int
sctk_thread_generic_sems_sem_unlink( const char* name ){

  /*
	ERRORS:
	EACCES  |>NOT IMPLEMENTED<| The caller does not have permission to unlink this semaphore
	ENAMETOOLONG  name was too long
	ENOENT  There is no semaphore with the given name
	*/

  int length;

  if( name == NULL ){
	errno = SCTK_ENOENT;
	return -1;
  }

  length = strlen (name);

  if( length > NAME_MAX-4 ){
	errno = SCTK_ENAMETOOLONG;
	return -1;
  }

  mpc_common_spinlock_lock( &sem_named_lock );
  sctk_thread_generic_sem_named_list_t* list = named_list_head;

  if( list != NULL ){
	DL_FOREACH( named_list_head, list ){
	  if( strcmp( list->name, name+1 ) == 0 
			  && list->unlink == 0 ) break;
	}
  }

  if( list == NULL ){
	errno = SCTK_ENOENT;
	mpc_common_spinlock_unlock( &sem_named_lock );
	return -1;
  }
  else if( list->nb == 0 ){
	DL_DELETE( named_list_head, list );
	sctk_free( list->sem );
	sctk_free( list );
  }
  else {
	list->unlink = 1;
  }

  mpc_common_spinlock_unlock( &sem_named_lock );

  return 0;
}

