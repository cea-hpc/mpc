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
#define _GNU_SOURCE
#include <sched.h>


#include "ethread_posix.h"
#include "ethread_engine_internal.h"

#include "thread_ptr.h"
#include "thread.h"

/****************
* PTHREAD ONCE *
****************/

static inline int __once_initialized(mpc_thread_once_t *
                                     once_control)
{
#ifdef mpc_thread_once_t_is_contiguous_int
	return *( (mpc_thread_once_t *)once_control) == SCTK_THREAD_ONCE_INIT;

#else
	mpc_thread_once_t once_init = SCTK_THREAD_ONCE_INIT;
	return memcpy
	               ( (void *)&once_init, (void *)once_control,
	               sizeof(mpc_thread_once_t) ) == 0;
#endif
}

int _mpc_thread_ethread_posix_once(mpc_thread_once_t *once_control,
                                   void (*init_routine)(void) )
{
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

	if(__once_initialized(once_control) )
	{
		mpc_thread_mutex_lock(&lock);
		if(__once_initialized(once_control) )
		{
			init_routine();
#ifdef mpc_thread_once_t_is_contiguous_int
			*once_control = !SCTK_THREAD_ONCE_INIT;
#else
			once_control[0] = SCTK_THREAD_ONCE_INIT;
#endif
		}
		mpc_thread_mutex_unlock(&lock);
	}
	return 0;
}

/**************
* ATTRIBUTES *
**************/

#define _mpc_thread_ethread_check_attr(attr)    do { if(attr == NULL){ return EINVAL; } if(attr->ptr == NULL){ return EINVAL; } } while(0)

int _mpc_thread_ethread_posix_attr_setdetachstate(_mpc_thread_ethread_attr_t *attr, int detachstate)
{
	/*attributs posix
	 * SCTK_THREAD_CREATE_JOINABLE
	 * SCTK_THREAD_CREATE_DETACHED
	 */
	_mpc_thread_ethread_check_attr(attr);

	if(detachstate != SCTK_THREAD_CREATE_JOINABLE &&
	   detachstate != SCTK_THREAD_CREATE_DETACHED)
	{
		return EINVAL;
	}
	else
	{
		(attr->ptr)->detached = detachstate;
	}
	return 0;
}

int _mpc_thread_ethread_posix_attr_getdetachstate(const _mpc_thread_ethread_attr_t *attr,
                                                  int *detachstate)
{
	_mpc_thread_ethread_check_attr(attr);
	*detachstate = attr->ptr->detached;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setschedpolicy(_mpc_thread_ethread_attr_t *attr, int policy)
{
	/*attributs posix
	 * SCTK_SCHED_OTHER (normal)
	 * SCTK_SCHED_RR (real time, round robin)
	 * SCTK_SCHED_FIFO (realtime, first-in first-out)
	 */
	_mpc_thread_ethread_check_attr(attr);
	if(policy == SCTK_SCHED_FIFO || policy == SCTK_SCHED_RR)
	{
		return ENOTSUP;
	}
	else if(policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}
	else
	{
		attr->ptr->schedpolicy = policy;
	}
	return 0;
}

int _mpc_thread_ethread_posix_attr_getschedpolicy(const _mpc_thread_ethread_attr_t *attr,
                                                  int *policy)
{
	_mpc_thread_ethread_check_attr(attr);
	*policy = attr->ptr->schedpolicy;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setinheritsched(_mpc_thread_ethread_attr_t *attr, int inherit)
{
	/*attributs posix
	 * SCTK_THREAD_EXPLICIT_SCHED
	 * SCTK_THREAD_INHERIT_SCHED
	 */
	_mpc_thread_ethread_check_attr(attr);
	if(inherit != SCTK_THREAD_INHERIT_SCHED &&
	   inherit != SCTK_THREAD_EXPLICIT_SCHED)
	{
		return EINVAL;
	}
	else
	{
		attr->ptr->inheritsched = inherit;
	}
	return 0;
}

int _mpc_thread_ethread_posix_attr_getinheritsched(const _mpc_thread_ethread_attr_t *attr,
                                                   int *inherit)
{
	_mpc_thread_ethread_check_attr(attr);
	*inherit = attr->ptr->inheritsched;
	return 0;
}

int _mpc_thread_ethread_posix_attr_getscope(const _mpc_thread_ethread_attr_t *attr, int *scope)
{
	_mpc_thread_ethread_check_attr(attr);
	*scope = attr->ptr->scope;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setscope(_mpc_thread_ethread_attr_t *attr, int scope)
{
	_mpc_thread_ethread_check_attr(attr);
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(scope != SCTK_ETHREAD_SCOPE_SYSTEM &&
	   scope != SCTK_ETHREAD_SCOPE_PROCESS)
	{
		return EINVAL;
	}

/*
 *  if (scope == SCTK_ETHREAD_SCOPE_SYSTEM)
 *    return ENOTSUP;
 */
	attr->ptr->scope = scope;
	return 0;
}

int _mpc_thread_ethread_posix_attr_getstacksize(const _mpc_thread_ethread_attr_t *attr,
                                                size_t *stacksize)
{
	if(attr == NULL || stacksize == NULL)
	{
		return ESRCH;
	}
	else
	{
		_mpc_thread_ethread_check_attr(attr);
		*stacksize = attr->ptr->stack_size;
	}
	return 0;
}

int _mpc_thread_ethread_posix_attr_getstackaddr(const _mpc_thread_ethread_attr_t *attr, void **stackaddr)
{
	if(attr == NULL || stackaddr == NULL)
	{
		return EINVAL;
	}
	_mpc_thread_ethread_check_attr(attr);
	*stackaddr = (void *)attr->ptr->stack;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setstacksize(_mpc_thread_ethread_attr_t *attr, size_t stacksize)
{
	if(stacksize < SCTK_THREAD_STACK_MIN)
	{
		return EINVAL;
	}

	else if(attr == NULL)
	{
		return ESRCH;
	}
	_mpc_thread_ethread_check_attr(attr);
	attr->ptr->stack_size = (int)stacksize;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setstackaddr(_mpc_thread_ethread_attr_t *attr, void *stackaddr)
{
	_mpc_thread_ethread_check_attr(attr);
	if(attr == NULL)
	{
		return ESRCH; /*la norme ne pr�cise pas le signal a envoyer */
	}
	_mpc_thread_ethread_check_attr(attr);
	attr->ptr->stack = (char *)stackaddr;
	return 0;
}

int _mpc_thread_ethread_posix_attr_setstack(_mpc_thread_ethread_attr_t *attr, void *stackaddr,
                                            size_t stacksize)
{
	_mpc_thread_ethread_check_attr(attr);
	if(attr == NULL || stacksize < SCTK_THREAD_STACK_MIN)   /*on effectue pas  le test d'alignement */
	{
		return EINVAL;
	}
	attr->ptr->stack_size = (int)stacksize;
	attr->ptr->stack      = (char *)stackaddr;
	return 0;
}

int _mpc_thread_ethread_posix_attr_getstack(_mpc_thread_ethread_attr_t *attr, void **stackaddr,
                                            size_t *stacksize)
{
	_mpc_thread_ethread_check_attr(attr);
	if(stackaddr != NULL)
	{
		*stackaddr = (void *)attr->ptr->stack;
	}
	if(stacksize != NULL)
	{
		*stacksize = attr->ptr->stack_size;
	}
	return 0;
}

int _mpc_thread_ethread_posix_attr_setguardsize(_mpc_thread_ethread_attr_t *attr,
                                                size_t guardsize)
{
	_mpc_thread_ethread_check_attr(attr);
	if(attr == NULL)
	{
		return EINVAL;
	}
	attr->ptr->guardsize = guardsize;
	return 0;
}

int _mpc_thread_ethread_posix_attr_getguardsize(_mpc_thread_ethread_attr_t *attr,
                                                size_t *guardsize)
{
	_mpc_thread_ethread_check_attr(attr);
	if(attr == NULL)
	{
		return EINVAL;
	}
	*guardsize = attr->ptr->guardsize;
	return 0;
}

int _mpc_thread_ethread_posix_attr_destroy(_mpc_thread_ethread_attr_t *attr)
{
	sctk_free(attr->ptr);
	attr->ptr = NULL;
	return 0;
}

/**********
* CANCEL *
**********/
/* mpc_thread_self() is used not specialize the interface for each thread engine */

void _mpc_thread_ethread_posix_testcancel(void)
{
	_mpc_thread_ethread_per_thread_t *current = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();

	___mpc_thread_ethread_testcancel(current);
}

int _mpc_thread_ethread_posix_cancel(_mpc_thread_ethread_t target)
{
	mpc_common_nodebug("target vaux : %d", target);
	if(target == NULL)
	{
		return ESRCH;
	}
	if(target->status == ethread_joined)
	{
		return ESRCH;
	}

	if(target->cancel_state != SCTK_THREAD_CANCEL_DISABLE)
	{
		target->cancel_status = 1;
	}
	return 0;
}

int _mpc_thread_ethread_posix_setcancelstate(int state, int *oldstate)
{
	_mpc_thread_ethread_per_thread_t *owner = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();

	if(oldstate != NULL)
	{
		*oldstate = owner->cancel_state;
	}
	if(state != SCTK_THREAD_CANCEL_DISABLE &&
	   state != SCTK_THREAD_CANCEL_ENABLE)
	{
		return EINVAL;
	}
	else
	{
		owner->cancel_state = (char)state;
	}
	return 0;
}

int _mpc_thread_ethread_posix_setcanceltype(int type, int *oldtype)
{
	_mpc_thread_ethread_per_thread_t *owner = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();

	mpc_common_nodebug("%d %d", type, owner->cancel_type);
	if(oldtype != NULL)
	{
		*oldtype = owner->cancel_type;
	}
	if(type != SCTK_THREAD_CANCEL_DEFERRED &&
	   type != SCTK_THREAD_CANCEL_ASYNCHRONOUS)
	{
		mpc_common_nodebug("%d %d %d", type != SCTK_THREAD_CANCEL_DEFERRED,
		             type != SCTK_THREAD_CANCEL_ASYNCHRONOUS, type);
		return EINVAL;
	}
	else
	{
		owner->cancel_type = (char)type;
	}
	return 0;
}

/**************
* SEMAPHORES *
**************/

int _mpc_thread_ethread_posix_sem_init(_mpc_thread_ethread_sem_t *lock,
                                       int pshared, unsigned int value)
{
	_mpc_thread_ethread_sem_t lock_tmp = STCK_ETHREAD_SEM_INIT;

	lock_tmp.lock = (int)value;
	if(value > SCTK_SEM_VALUE_MAX)
	{
		return EINVAL;
	}
	if(pshared)
	{
		return -1;
	}
	*lock = lock_tmp;
	return 0;
}

int _mpc_thread_ethread_posix_sem_wait(_mpc_thread_ethread_sem_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       owner = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp    = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	_mpc_thread_ethread_mutex_cell_t cell;

	mpc_common_nodebug("%p lock on %p %d", owner, lock, lock->lock);
	___mpc_thread_ethread_testcancel(owner);
	mpc_common_spinlock_lock(&lock->spinlock);
	lock->lock--;
	if(lock->lock < 0)
	{
		cell.my_self = owner;
		cell.next    = NULL;
		cell.wake    = 0;
		if(lock->list == NULL)
		{
			lock->list      = &cell;
			lock->list_tail = &cell;
		}
		else
		{
			lock->list_tail->next = &cell;
			lock->list_tail       = &cell;
		}
		mpc_common_nodebug("%p blocked on %p", owner, lock);
		if(owner->status == ethread_ready)
		{
			owner->status          = ethread_blocked;
			owner->no_auto_enqueue = 1;
			mpc_common_spinlock_unlock(&lock->spinlock);
			___mpc_thread_ethread_sched_yield_vp_poll(vp, owner);
			owner->status = ethread_ready;
		}
		else
		{
			while(cell.wake != 1)
			{
				mpc_common_spinlock_unlock(&lock->spinlock);
				___mpc_thread_ethread_sched_yield_vp(vp, owner);
				mpc_common_spinlock_lock(&lock->spinlock);
			}
			mpc_common_spinlock_unlock(&lock->spinlock);
		}
	}
	else
	{
		/*lock->lock--; effectu� au d�but */
		mpc_common_spinlock_unlock(&lock->spinlock);
	}

	return 0;
}

int _mpc_thread_ethread_posix_sem_trywait(_mpc_thread_ethread_sem_t *lock)
{
/*    mpc_common_nodebug ("%p lock on %p %d", owner, lock, lock->lock);*/

	if(lock->lock <= 0)
	{
		mpc_common_nodebug("try_wait : sem occup�");
		errno = EAGAIN;
		return -1;
	}
	mpc_common_spinlock_lock(&lock->spinlock);

	if(lock->lock <= 0)
	{
		mpc_common_nodebug("try_wait : sem occup�");
		mpc_common_spinlock_unlock(&lock->spinlock);
		errno = EAGAIN;
		return -1;
	}
	else
	{
		lock->lock--;
		mpc_common_nodebug("try_wait : sem libre");
		mpc_common_spinlock_unlock(&lock->spinlock);
	}

	return 0;
}

int _mpc_thread_ethread_posix_sem_post(_mpc_thread_ethread_sem_t *lock, void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_mutex_cell_t *cell;
	_mpc_thread_ethread_per_thread_t *to_wake_up;

	mpc_common_spinlock_lock(&lock->spinlock);
	if(lock->list != NULL)
	{
		_mpc_thread_ethread_per_thread_t *to_wake;
		cell       = (_mpc_thread_ethread_mutex_cell_t *)lock->list;
		to_wake_up = cell->my_self;
		lock->list = lock->list->next;
		if(lock->list == NULL)
		{
			lock->list_tail = NULL;
		}
		to_wake    = (_mpc_thread_ethread_per_thread_t *)to_wake_up;
		cell->wake = 1;
		return_task(to_wake);
	}
	lock->lock++;
	mpc_common_spinlock_unlock(&lock->spinlock);
/*    mpc_common_nodebug ("%p unlock on %p %d", owner, lock, lock->lock);*/

	return 0;
}

int _mpc_thread_ethread_posix_sem_getvalue(_mpc_thread_ethread_sem_t *sem, int *sval)
{
	if(sval != NULL)
	{
		*sval = sem->lock;
	}
	return 0;
}

int _mpc_thread_ethread_posix_sem_destroy(_mpc_thread_ethread_sem_t *sem)
{
	if(sem->list != NULL)
	{
		return EBUSY;
	}
	else
	{
		return 0;
	}
}

typedef struct _mpc_thread_ethread_sem_name_s
{
	char *                                   name;
	volatile int                             nb;
	volatile int                             unlink;
	_mpc_thread_ethread_sem_t *                     sem;
	volatile struct _mpc_thread_ethread_sem_name_s *next;
} _mpc_thread_ethread_sem_name_t;

typedef struct
{
	mpc_common_spinlock_t             spinlock;
	volatile _mpc_thread_ethread_sem_name_t *next;
} _mpc_thread_ethread_sem_head_list;

#define SCTK_SEM_HEAD_INITIALIZER    { MPC_COMMON_SPINLOCK_INITIALIZER, NULL }

static _mpc_thread_ethread_sem_head_list __sctk_head_sem = SCTK_SEM_HEAD_INITIALIZER;

_mpc_thread_ethread_sem_t *_mpc_thread_ethread_posix_sem_open(const char *name, int oflag, ...)
{
	int value = 0;
	__UNUSED__ int mode = 0;

	if(oflag & O_CREAT)
	{
		va_list ap;
		va_start(ap, oflag);
		mode  = (int)va_arg(ap, mode_t);
		value = va_arg(ap, int);
		va_end(ap);
	}

	int length;
	_mpc_thread_ethread_sem_t *sem = NULL;
	volatile _mpc_thread_ethread_sem_name_t *tmp, *sem_struct;
	char *_name;

	sem_struct = __sctk_head_sem.next;
	if(name == NULL)
	{
		errno = EINVAL;
		return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
	}

	length = strlen(name);
	_name  = (char *)sctk_malloc( (length + 1) * sizeof(char) );
	if(name[0] == '/')
	{
		strncpy(_name, name + 1, length + 1);
		_name[length - 1] = '\0';
	}
	else
	{
		strncpy(_name, name, length + 1);
		_name[length] = '\0';
	}
	/* We search the semaphore */
	tmp = sem_struct;
	mpc_common_spinlock_lock(&__sctk_head_sem.spinlock);
	while(tmp != NULL)
	{
		if(strcmp(tmp->name, _name) == 0 && tmp->unlink == 0)
		{
			break;
		}
		else
		{
			sem_struct = tmp;
			tmp        = tmp->next;
		}
	}

	if( (tmp != NULL) && ( (oflag & O_CREAT) && (oflag & O_EXCL) ) )
	{
		errno = EEXIST;
		mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
		return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
	}
	else if(tmp != NULL)
	{
		sctk_free(_name);
		tmp->nb++;
		mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
		return tmp->sem;
	}
	if(tmp == NULL && (oflag & O_CREAT) )
	{
		_mpc_thread_ethread_sem_name_t *maillon;
		int ret;
		sem = (_mpc_thread_ethread_sem_t *)
		      sctk_malloc(sizeof(_mpc_thread_ethread_sem_t) );
		if(sem == NULL)
		{
			sctk_free(_name);
			errno = ENOSPC;
			mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
			return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
		}
		ret = _mpc_thread_ethread_posix_sem_init(sem, 0, value);
		if(ret != 0)
		{
			sctk_free(_name);
			sctk_free(sem);
			errno = ret;
			mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
			return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
		}
		maillon = (_mpc_thread_ethread_sem_name_t *)
		          sctk_malloc(sizeof(_mpc_thread_ethread_sem_name_t) );
		mpc_common_nodebug("%p pointe sur %p avec maillon vaux %p\n",
		             sem_struct, sem_struct, maillon);
		if(maillon == NULL)
		{
			sctk_free(_name);
			sctk_free(sem);
			errno = ENOSPC;
			mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
			return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
		}
		if(sem_struct != NULL)
		{
			maillon->next    = sem_struct->next;
			sem_struct->next = maillon;
		}
		else
		{
			__sctk_head_sem.next = maillon;
		}
		maillon->name   = _name;
		maillon->sem    = sem;
		maillon->nb     = 1;
		maillon->unlink = 0;
		mpc_common_nodebug("__sctk_head_sem %p pointe sur %p avec maillon vaux %p\n",
		             __sctk_head_sem, __sctk_head_sem.next, maillon);
	}
	if(tmp == NULL && !(oflag & O_CREAT) )
	{
		sctk_free(_name);
		errno = ENOENT;
		mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
		return (_mpc_thread_ethread_sem_t *)SCTK_SEM_FAILED;
	}
	mpc_common_nodebug("sem vaux %p avec une valeur de %d", sem, sem->lock);
	mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
	return sem;
}

static inline void __semaphore_clean(volatile
                                     _mpc_thread_ethread_sem_name_t *
                                     sem_struct)
{
	sctk_free(sem_struct->name);
	sem_struct->name = NULL;
	sctk_free(sem_struct->sem);
	sem_struct->sem = NULL;
}

int _mpc_thread_ethread_posix_sem_close(_mpc_thread_ethread_sem_t *sem)
{
	volatile _mpc_thread_ethread_sem_name_t *tmp, *sem_struct;

	sem_struct = __sctk_head_sem.next;
	/*on recherche le s�maphore */
	tmp = sem_struct;
	mpc_common_spinlock_lock(&__sctk_head_sem.spinlock);
	while(tmp != NULL && sem != tmp->sem)
	{
		sem_struct = tmp;
		tmp        = tmp->next;
	}

	mpc_common_nodebug("la recherche a trouv� : %p pour le s�maphore %p\n",
	             tmp, sem);
	if(tmp == NULL)
	{
		errno = EINVAL;
		mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
		return -1;
	}
	tmp->nb--;
	if(tmp->nb == 0 && tmp->unlink != 0)
	{
		mpc_common_nodebug
		        ("close d�truit le maillon %p avec le s�maphore %p", tmp, tmp->sem);
		mpc_common_nodebug("sem_struct : %p , tmp : %p ", sem_struct, tmp);
		if(__sctk_head_sem.next == tmp)
		{
			__sctk_head_sem.next = tmp->next;
		}
		else
		{
			sem_struct->next = tmp->next;
		}

		__semaphore_clean(tmp);
		sctk_free( (_mpc_thread_ethread_sem_name_t *)tmp);
	}
	mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
	return 0;
}

int _mpc_thread_ethread_posix_sem_unlink(const char *name)
{
	char *_name;
	volatile _mpc_thread_ethread_sem_name_t *tmp, *sem_struct;
	int length;

	sem_struct = __sctk_head_sem.next;
	if(name == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	length = strlen(name);
	_name  = (char *)sctk_malloc( (length + 1) * sizeof(char) );
	if(name[0] == '/')
	{
		strncpy(_name, name + 1, length + 1);
		_name[length - 1] = '\0';
	}
	else
	{
		strncpy(_name, name, length + 1);
		_name[length] = '\0';
	}
	/*on recherche le s�maphore */
	tmp = sem_struct;
	mpc_common_spinlock_lock(&__sctk_head_sem.spinlock);
	while(tmp != NULL)
	{
		if(strcmp(tmp->name, _name) == 0 && tmp->unlink == 0)
		{
			break;
		}
		else
		{
			sem_struct = tmp;
			tmp        = tmp->next;
		}
	}
	if(tmp == NULL)
	{
		sctk_free(_name);
		errno = ENOENT;
		mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
		return -1;
	}
	else if(tmp->nb == 0)
	{                       /*tout le monde a d�ja ferm� ce s�maphore, on le d�truit */
		mpc_common_nodebug
		        ("unlink d�truit le maillon %p avec le s�maphore %p", tmp,
		        tmp->sem);
		if(__sctk_head_sem.next == tmp)
		{
			__sctk_head_sem.next = tmp->next;
		}
		else
		{
			sem_struct->next = tmp->next;
		}

		__semaphore_clean(tmp);
		sctk_free( (_mpc_thread_ethread_sem_name_t *)tmp);
	}
	else
	{
		tmp->unlink = 1;
	}
	mpc_common_spinlock_unlock(&__sctk_head_sem.spinlock);
	sctk_free(_name);
	return 0;
}

/**********
* DETACH *
**********/

int _mpc_thread_ethread_posix_detach(_mpc_thread_ethread_t th)
{
	if(th == NULL)
	{
		return ESRCH;
	}
	else if(th->status == ethread_joined)
	{
		return ESRCH;
	}
	else if(th->attr.detached == SCTK_ETHREAD_CREATE_DETACHED)
	{
		return EINVAL;
	}
	else
	{
		return 0;
	}
}

/******************
* SCHED PRIORITY *
******************/

int _mpc_thread_ethread_posix_sched_get_priority_max(int policy)
{
	if(policy != SCTK_SCHED_FIFO && policy != SCTK_SCHED_RR &&
	   policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}
	else
	{
		return 0;
	}
}

int _mpc_thread_ethread_posix_sched_get_priority_min(int policy)
{
	if(policy != SCTK_SCHED_FIFO && policy != SCTK_SCHED_RR &&
	   policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}
	else
	{
		return 0;
	}
}

/**************
* MUTEX ATTR *
**************/

int _mpc_thread_ethread_posix_mutexattr_init(_mpc_thread_ethread_mutexattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	attr->kind = SCTK_THREAD_MUTEX_DEFAULT;
	return 0;
}

int _mpc_thread_ethread_posix_mutexattr_destroy(_mpc_thread_ethread_mutexattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	return 0;
}

int _mpc_thread_ethread_posix_mutexattr_gettype(const _mpc_thread_ethread_mutexattr_t *attr,
                                                int *kind)
{
	if(attr == NULL || kind == NULL)
	{
		return EINVAL;
	}
	*kind = attr->kind;
	return 0;
}

int _mpc_thread_ethread_posix_mutexattr_settype(_mpc_thread_ethread_mutexattr_t *attr, int kind)
{
	if( (attr == NULL) ||
	    (kind != SCTK_THREAD_MUTEX_DEFAULT &&
	     kind != SCTK_THREAD_MUTEX_RECURSIVE &&
	     kind != SCTK_THREAD_MUTEX_ERRORCHECK &&
	     kind != SCTK_THREAD_MUTEX_NORMAL) )
	{
		return EINVAL;
	}
	attr->kind = kind;
	return 0;
}

int _mpc_thread_ethread_posix_mutexattr_getpshared(const _mpc_thread_ethread_mutexattr_t *attr,
                                                   int *pshared)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	*pshared = SCTK_THREAD_PROCESS_PRIVATE;
	return 0;
}

int _mpc_thread_ethread_posix_mutexattr_setpshared(_mpc_thread_ethread_mutexattr_t *attr, int pshared)
{
	if(attr == NULL ||
	   (pshared != SCTK_THREAD_PROCESS_SHARED &&
	    pshared != SCTK_THREAD_PROCESS_PRIVATE) )
	{
		return EINVAL;
	}
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		return ENOTSUP; /*pas dans la norme mais on le supporte pas */
	}
	return 0;
}

/*********
* MUTEX *
*********/

int _mpc_thread_ethread_posix_mutex_destroy(_mpc_thread_ethread_mutex_t *mutex)
{
	if(mutex->lock > 0)
	{
		return EBUSY;
	}
	return 0;
}

/**************
* CONDITIONS *
**************/

int _mpc_thread_ethread_posix_cond_init(_mpc_thread_ethread_cond_t *cond,
                                        const _mpc_thread_ethread_condattr_t *attr)
{
	if(cond == NULL)
	{
		return EINVAL;
	}
	if(attr != NULL)
	{
		if(attr->pshared == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr\n");
			abort();
		}
	}
	cond->is_init = 0;
	mpc_common_spinlock_init(&(cond->lock), 0);
	cond->list      = NULL;
	cond->list_tail = NULL;
	return 0;
}

int _mpc_thread_ethread_posix_cond_destroy(_mpc_thread_ethread_cond_t *cond)
{
	mpc_common_nodebug(" %p %p %d\n", cond->list, cond->list_tail, cond->is_init);
	if(cond->is_init != 0)
	{
		return EINVAL;
	}
	if(cond->list != NULL && cond->list_tail != NULL)
	{
		return EBUSY;
	}
	return 0;
}

int _mpc_thread_ethread_posix_cond_wait(_mpc_thread_ethread_cond_t *cond,
                                        _mpc_thread_ethread_mutex_t *mutex,
                                        void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_per_thread_t *       owner = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp    = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	_mpc_thread_ethread_mutex_cell_t cell;
	int ret;

	___mpc_thread_ethread_testcancel(owner);
	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond == NULL || mutex == NULL)
	{
		return EINVAL;
	}
	cell.my_self = owner;
	cell.next    = NULL;
	cell.wake    = 0;
	if(cond->list == NULL)
	{
		cond->list      = &cell;
		cond->list_tail = &cell;
	}
	else
	{
		cond->list_tail->next = &cell;
		cond->list_tail       = &cell;
	}
/*    mpc_common_nodebug("%p blocked on %p", owner, lock);*/
	if(owner->status == ethread_ready)
	{
		owner->status          = ethread_blocked;
		owner->no_auto_enqueue = 1;
		ret = ___mpc_thread_ethread_mutex_unlock(vp, owner, return_task, mutex);
		if(ret != 0)
		{
			cond->list = cond->list->next;
			if(cond->list == NULL)
			{
				cond->list_tail = NULL;
			}
			owner->status          = ethread_ready;
			owner->no_auto_enqueue = 0;
			mpc_common_spinlock_unlock(&(cond->lock) );
			return EPERM;
		}
		mpc_common_spinlock_unlock(&(cond->lock) );
		___mpc_thread_ethread_sched_yield_vp_poll(vp, owner);
	}
	else
	{
		ret = ___mpc_thread_ethread_mutex_unlock(vp, owner, return_task, mutex);
		if(ret != 0)
		{
			cond->list = cond->list->next;
			if(cond->list == NULL)
			{
				cond->list_tail = NULL;
			}
			mpc_common_spinlock_unlock(&(cond->lock) );
			return EPERM;
		}
		mpc_common_spinlock_unlock(&(cond->lock) );
		while(cell.wake != 1)
		{
			___mpc_thread_ethread_sched_yield_vp(vp, owner);
		}
	}
	___mpc_thread_ethread_mutex_lock(vp, owner, mutex);
	return 0;
}

int _mpc_thread_ethread_posix_cond_timedwait(__UNUSED__ _mpc_thread_ethread_cond_t *cond,
                                             __UNUSED__ _mpc_thread_ethread_mutex_t *mutex,
                                             __UNUSED__ const struct timespec *abstime,
                                             __UNUSED__ void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{

#if 0
	_mpc_thread_ethread_per_thread_t *       owner = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp    = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	_mpc_thread_ethread_mutex_cell_t cell;
	struct timeval            tv;
	int ret;

	___mpc_thread_ethread_testcancel(owner);
	if(cond == NULL || mutex == NULL)
	{
		return EINVAL;
	}
	gettimeofday(&tv, NULL);
	mpc_common_nodebug("temps : %d %d  =  %d %d ", tv.tv_sec,
	             tv.tv_usec * 1000, abstime->tv_sec, abstime->tv_nsec);
	if(tv.tv_sec > abstime->tv_sec)
	{
		if(tv.tv_usec * 1000 > (abstime->tv_nsec) )
		{
			mpc_common_nodebug("sortie direct");
			return ETIMEDOUT;
		}
	}

	mpc_common_spinlock_lock(&(cond->lock) );
	cell.my_self = owner;
	cell.next    = NULL;
	cell.wake    = -1;
	if(cond->list == NULL)
	{
		cond->list      = &cell;
		cond->list_tail = &cell;
	}
	else
	{
		cond->list_tail->next = &cell;
		cond->list_tail       = &cell;
	}
	ret = ___mpc_thread_ethread_mutex_unlock(vp, owner, return_task, mutex);
	if(ret != 0)
	{
		cond->list = cond->list->next;
		if(cond->list == NULL)
		{
			cond->list_tail = NULL;
		}
		owner->status          = ethread_ready;
		owner->no_auto_enqueue = 0;
		mpc_common_spinlock_unlock(&(cond->lock) );
		mpc_common_nodebug("sortie eperm");
		return EPERM;
	}
	mpc_common_nodebug("cond ->list = %p", cond->list);
	mpc_common_spinlock_unlock(&(cond->lock) );
	ret = 0;
	while(cell.wake != 1)
	{
		___mpc_thread_ethread_sched_yield_vp(vp, owner);
		gettimeofday(&tv, NULL);
		if(tv.tv_sec > abstime->tv_sec)
		{
			if(tv.tv_usec * 1000 > (abstime->tv_nsec) )
			{
				_mpc_thread_ethread_cond_t *ptr;
				ptr = cond;

				mpc_common_spinlock_lock(&(cond->lock) );

				cell.wake     = 1;
				owner->status = ethread_ready;
				mpc_common_nodebug("cond ->list 2 = %p", cond->list);

				/*
				 * on reparcours toute la liste pour savoir ou on est
				 * on peut etre absent (apres un broadcast qui c'est produit
				 * entre le d�but du while et ici  par exemple
				 */
				while(ptr->list != NULL)
				{
					if(ptr->list->next == &cell)
					{
						if(cond->list_tail == &cell)
						{
							cond->list_tail = ptr->list;
							ptr->list->next = NULL;
						}
						else
						{
							ptr->list->next = ptr->list->next->next;
						}
					}
					ptr->list = ptr->list->next;
				}
				/*cond->list=cond->list->next; */
				if(cond->list == NULL)
				{
					cond->list_tail = NULL;
				}

				ret = ETIMEDOUT;
				mpc_common_nodebug("cond ->list 3 = %p", cond->list);
				mpc_common_spinlock_unlock(&(cond->lock) );
			}
		}
	}

	mpc_common_nodebug("le mutex a �t� r�cup�r�");
	___mpc_thread_ethread_mutex_lock(vp, owner, mutex);
	return ret;
}

#else
	not_implemented();
	return 0;
#endif
}

int _mpc_thread_ethread_posix_cond_broadcast(_mpc_thread_ethread_cond_t *cond,
                                             void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_per_thread_t *to_wake;
	_mpc_thread_ethread_mutex_cell_t *cell, *cell_next;

	if(cond == NULL)
	{
		return EINVAL;
	}
	mpc_common_spinlock_lock(&(cond->lock) );
	while(cond->list != NULL)
	{
		cell_next = cond->list->next;
		cell      = (_mpc_thread_ethread_mutex_cell_t *)cond->list;
		to_wake   = (_mpc_thread_ethread_per_thread_t *)cell->my_self;
		if(cell->wake == -1)
		{
			cell->wake = 1;
		}
		else
		{
			cell->wake = 1;
			return_task(to_wake);
		}

		cond->list = cell_next;
	}
	cond->list_tail = NULL;
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

int _mpc_thread_ethread_posix_cond_signal(_mpc_thread_ethread_cond_t *cond,
                                          void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_per_thread_t *to_wake;
	_mpc_thread_ethread_mutex_cell_t *cell, *cell_next;

	if(cond == NULL)
	{
		return EINVAL;
	}
	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond->list != NULL)
	{
		cell_next = cond->list->next;
		cell      = (_mpc_thread_ethread_mutex_cell_t *)cond->list;
		to_wake   = (_mpc_thread_ethread_per_thread_t *)cell->my_self;
		if(cell->wake == -1)
		{
			cell->wake = 1;
		}
		else
		{
			cell->wake = 1;
			return_task(to_wake);
		}
		cond->list = cell_next;
	}
	if(cond->list == NULL)
	{
		cond->list_tail = NULL;
	}
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

/************************
* CONDITION ATTRIBUTES *
************************/

int _mpc_thread_ethread_posix_condattr_destroy(_mpc_thread_ethread_condattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	return 0;
}

int _mpc_thread_ethread_posix_condattr_init(_mpc_thread_ethread_condattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
	attr->clock   = 0;
	return 0;
}

int _mpc_thread_ethread_posix_condattr_getpshared(const _mpc_thread_ethread_condattr_t *attr,
                                                  int *pshared)
{
	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}
	*pshared = attr->pshared;
	return 0;
}

int _mpc_thread_ethread_posix_condattr_setpshared(_mpc_thread_ethread_condattr_t *attr, int pshared)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE &&
	   pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		return ENOTSUP;
	}
	attr->pshared = (char)pshared;
	return 0;
}

/*************
* SPINLOCKS *
*************/

int _mpc_thread_ethread_posix_spin_init(_mpc_thread_ethread_spinlock_t *lock, int pshared)
{
	mpc_common_spinlock_init(lock, 0);
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		return ENOTSUP; /*pas dans la norme mais on le supporte pas */
	}
	return 0;
}

int _mpc_thread_ethread_posix_spin_destroy(_mpc_thread_ethread_spinlock_t *lock)
{
	if(OPA_load_int(lock) != 0)
	{
		return EBUSY;
	}
	return 0;
}

/*on ne supporte pas le EDEADLOCK
 * La norme n'est pas precise a ce sujet
 */
int _mpc_thread_ethread_posix_spin_lock(_mpc_thread_ethread_spinlock_t *lock)
{
	/*mpc_common_spinlock_lock(lock); */
	long i = SCTK_SPIN_DELAY;

	while(mpc_common_spinlock_trylock(lock) )
	{
		while(expect_true(OPA_load_int(lock) != 0) )
		{
			i--;
			if(expect_false(i <= 0) )
			{
				mpc_thread_yield();
				i = SCTK_SPIN_DELAY;
			}
		}
	}
	return 0;
}

int _mpc_thread_ethread_posix_spin_trylock(_mpc_thread_ethread_spinlock_t *lock)
{
	if(mpc_common_spinlock_trylock(lock) != 0)
	{
		return EBUSY;
	}
	else
	{
		return 0;
	}
}

int _mpc_thread_ethread_posix_spin_unlock(_mpc_thread_ethread_spinlock_t *lock)
{
	mpc_common_spinlock_unlock(lock);
	return 0;
}

/**********
* RWLOCK *
**********/

/*on ne supporte pas les attributs dans le sens on ne supporte pas le fait
 * qu'on soit sur plusieurs processus*/
int _mpc_thread_ethread_posix_rwlock_init(_mpc_thread_ethread_rwlock_t *rwlock,
                                          const _mpc_thread_ethread_rwlockattr_t *attr)
{
	_mpc_thread_ethread_rwlock_t rwlock2 = SCTK_ETHREAD_RWLOCK_INIT;

	if(attr != NULL)
	{
		if(attr->pshared == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr\n");
			abort();
		}
	}
	if(rwlock == NULL)
	{
		return EINVAL;
	}
	*rwlock = rwlock2;
	return 0;
}

int _mpc_thread_ethread_posix_rwlock_destroy(_mpc_thread_ethread_rwlock_t *rwlock)
{
	if(rwlock == NULL)
	{
		return EINVAL;
	}
	return 0;
}

static inline int __rwlock_lock(_mpc_thread_ethread_rwlock_t *
                                rwlock,
                                unsigned char type,
                                _mpc_thread_ethread_virtual_processor_t
                                *vp,
                                _mpc_thread_ethread_per_thread_t
                                *owner)
{
	_mpc_thread_ethread_rwlock_cell_t cell;

	mpc_common_spinlock_lock(&rwlock->spinlock);
	rwlock->lock++;

	mpc_common_nodebug
	        (" rwlock : \nlock = %d\ncurrent = %d\nwait = %d\ntype : %d\n",
	        rwlock->lock, rwlock->current, rwlock->wait, type);

	if(type == SCTK_RWLOCK_READ)
	{
		/* reader */
		if( (rwlock->current == SCTK_RWLOCK_READ ||
		     rwlock->current == SCTK_RWLOCK_ALONE) &&
		    (rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT) )
		{
			rwlock->current = SCTK_RWLOCK_READ;
			mpc_common_spinlock_unlock(&rwlock->spinlock);
			return 0;
		}
	}
	else if(type == SCTK_RWLOCK_WRITE)
	{
		/* writer */
		if(rwlock->current == SCTK_RWLOCK_ALONE &&
		   rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT)
		{
			rwlock->current = SCTK_RWLOCK_WRITE;
			mpc_common_spinlock_unlock(&rwlock->spinlock);
			return 0;
		}

		rwlock->wait = SCTK_RWLOCK_WR_WAIT;
	}
	else
	{
		not_reachable();
	}

	cell.my_self = owner;
	cell.next    = NULL;
	cell.wake    = 0;
	cell.type    = type;
	if(rwlock->list == NULL)
	{
		rwlock->list      = &cell;
		rwlock->list_tail = &cell;
	}
	else
	{
		rwlock->list_tail->next = &cell;
		rwlock->list_tail       = &cell;
	}
	mpc_common_nodebug("blocked on %p", rwlock);
	if(owner->status == ethread_ready)
	{
		owner->status          = ethread_blocked;
		owner->no_auto_enqueue = 1;
		mpc_common_spinlock_unlock(&rwlock->spinlock);
		___mpc_thread_ethread_sched_yield_vp_poll(vp, owner);
		owner->status = ethread_ready;
		mpc_common_spinlock_lock(&rwlock->spinlock);
	}
	else
	{
		while(cell.wake != 1)
		{
			mpc_common_spinlock_unlock(&rwlock->spinlock);
			___mpc_thread_ethread_sched_yield_vp(vp, owner);
			mpc_common_spinlock_lock(&rwlock->spinlock);
		}
	}

	mpc_common_spinlock_unlock(&rwlock->spinlock);
	return 0;
}

static inline int __rwlock_trylock(_mpc_thread_ethread_rwlock_t *rwlock, int type)
{
	mpc_common_spinlock_lock(&rwlock->spinlock);
	/*si on n'est pas sur de devoir attendre */
	if(rwlock->current != SCTK_RWLOCK_WRITE &&
	   rwlock->wait != SCTK_RWLOCK_WR_WAIT)
	{
		if(type == SCTK_RWLOCK_READ)
		{
			rwlock->current = SCTK_RWLOCK_READ;
			rwlock->lock++;
			mpc_common_spinlock_unlock(&rwlock->spinlock);
			return 0;
		}
	}
	/*si il y a aucune attente , alors on peut �crire directement */
	if(type == SCTK_RWLOCK_WRITE &&
	   rwlock->current == SCTK_RWLOCK_ALONE &&
	   rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT)
	{
		rwlock->current = SCTK_RWLOCK_WRITE;
		rwlock->lock++;
		mpc_common_spinlock_unlock(&rwlock->spinlock);
		return 0;
	}
	mpc_common_spinlock_unlock(&rwlock->spinlock);
	return EBUSY;
}

int _mpc_thread_ethread_posix_rwlock_rdlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	_mpc_thread_ethread_per_thread_t *owner     = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	return __rwlock_lock(rwlock, SCTK_RWLOCK_READ,
	                     vp, owner);
}

int _mpc_thread_ethread_posix_rwlock_tryrdlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	return __rwlock_trylock(rwlock, SCTK_RWLOCK_READ);
}

int _mpc_thread_ethread_posix_rwlock_wrlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	_mpc_thread_ethread_per_thread_t *owner     = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	return __rwlock_lock(rwlock, SCTK_RWLOCK_WRITE,
	                     vp, owner);
}

int _mpc_thread_ethread_posix_rwlock_trywrlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	return __rwlock_trylock(rwlock, SCTK_RWLOCK_WRITE);
}

int _mpc_thread_ethread_posix_rwlock_unlock(_mpc_thread_ethread_rwlock_t *lock, void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_rwlock_cell_t *cell;
	_mpc_thread_ethread_per_thread_t *to_wake_up;
	int first = 1;

	mpc_common_spinlock_lock(&(lock->spinlock) );
	mpc_common_nodebug("unlock : %p", lock->list);
	lock->lock--;
	if(lock->list != NULL)
	{
		lock->wait = SCTK_RWLOCK_NO_WR_WAIT;

		/*
		 * Wake all readers
		 */
		do
		{
			_mpc_thread_ethread_per_thread_t *to_wake;
			cell = (_mpc_thread_ethread_rwlock_cell_t *)lock->list;

			lock->current = cell->type;

			if(cell->type == SCTK_RWLOCK_WRITE)
			{
				lock->wait = SCTK_RWLOCK_WR_WAIT;
			}

			if(first == 1 || lock->wait == SCTK_RWLOCK_NO_WR_WAIT)
			{
				lock->list = lock->list->next;
				to_wake_up = cell->my_self;
				to_wake    = (_mpc_thread_ethread_per_thread_t *)to_wake_up;
				cell->wake = 1;
				return_task(to_wake);
				first = 0;
			}
		} while(lock->list != NULL && lock->wait == SCTK_RWLOCK_NO_WR_WAIT);
	}
	if(lock->list == NULL)
	{
		lock->list_tail = NULL;
	}
	if(lock->lock <= 0)
	{
		lock->wait    = SCTK_RWLOCK_NO_WR_WAIT;
		lock->current = SCTK_RWLOCK_ALONE;
	}
	mpc_common_spinlock_unlock(&(lock->spinlock) );

	return 0;
}

/**************
* RWLOCKATTR *
**************/

int _mpc_thread_ethread_posix_rwlockattr_getpshared(const _mpc_thread_ethread_rwlockattr_t *attr,
                                                    int *pshared)
{
	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}
	*pshared = attr->pshared;
	return 0;
}

int _mpc_thread_ethread_posix_rwlockattr_setpshared(_mpc_thread_ethread_rwlockattr_t *attr,
                                                    int pshared)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE &&
	   pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		return ENOTSUP;
	}
	attr->pshared = pshared;
	return 0;
}

int _mpc_thread_ethread_posix_rwlockattr_init(_mpc_thread_ethread_rwlockattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
	return 0;
}

int _mpc_thread_ethread_posix_rwlockattr_destroy(_mpc_thread_ethread_rwlockattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	return 0;
}

/***********
* BARRIER *
***********/

int _mpc_thread_ethread_posix_barrier_init(_mpc_thread_ethread_barrier_t *barrier,
                                           const _mpc_thread_ethread_barrierattr_t *attr,
                                           unsigned count)
{
	if(barrier == NULL)
	{
		return EINVAL;
	}
	if(count == 0)
	{
		return EINVAL;
	}
	if(attr != NULL)
	{
		if(attr->pshared == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr\n");
			abort();
		}
	}

	mpc_common_spinlock_init(&barrier->spinlock, 0);
	barrier->list      = NULL;
	barrier->list_tail = NULL;
	barrier->nb_max    = count;
	barrier->lock      = count;
	return 0;
}

int _mpc_thread_ethread_posix_barrier_destroy(_mpc_thread_ethread_barrier_t *barrier)
{
	if(barrier == NULL)
	{
		return EINVAL;
	}
	return 0;
}

int _mpc_thread_ethread_posix_barrier_wait(_mpc_thread_ethread_barrier_t *barrier, void (*return_task)(_mpc_thread_ethread_per_thread_t *) )
{
	_mpc_thread_ethread_per_thread_t *owner     = (_mpc_thread_ethread_per_thread_t *)mpc_thread_self();
	_mpc_thread_ethread_virtual_processor_t *vp = (_mpc_thread_ethread_virtual_processor_t *)owner->vp;

	_mpc_thread_ethread_mutex_cell_t cell;
	int ret;

	ret          = 0;
	cell.my_self = owner;
	cell.next    = NULL;
	cell.wake    = 0;

	mpc_common_spinlock_lock(&barrier->spinlock);

	barrier->lock--;

	if(barrier->lock > 0)
	{
		if(barrier->list == NULL)
		{
			barrier->list      = &cell;
			barrier->list_tail = &cell;
		}
		else
		{
			barrier->list_tail->next = &cell;
			barrier->list_tail       = &cell;
		}
		mpc_common_nodebug("blocked on %p", barrier);
		if(owner->status == ethread_ready)
		{
			owner->status          = ethread_blocked;
			owner->no_auto_enqueue = 1;
			mpc_common_spinlock_unlock(&barrier->spinlock);
			___mpc_thread_ethread_sched_yield_vp_poll(vp, owner);
			owner->status = ethread_ready;
			mpc_common_spinlock_lock(&barrier->spinlock);
		}
		else
		{
			mpc_common_spinlock_unlock(&barrier->spinlock);
			while(cell.wake != 1)
			{
				___mpc_thread_ethread_sched_yield_vp(vp, owner);
			}
		}
	}
	else
	{
		_mpc_thread_ethread_mutex_cell_t *list;

		list = (_mpc_thread_ethread_mutex_cell_t *)barrier->list;

		barrier->list_tail = NULL;
		barrier->lock      = barrier->nb_max;
		barrier->list      = NULL;
		mpc_common_spinlock_unlock(&barrier->spinlock);

		while(list != NULL)
		{
			_mpc_thread_ethread_mutex_cell_t *cell2;
			_mpc_thread_ethread_per_thread_t *to_wake;
			_mpc_thread_ethread_per_thread_t *to_wake_up;
			cell2       = (_mpc_thread_ethread_mutex_cell_t *)list;
			list        = list->next;
			to_wake_up  = cell2->my_self;
			to_wake     = (_mpc_thread_ethread_per_thread_t *)to_wake_up;
			cell2->wake = 1;
			return_task(to_wake);
		}
		ret = SCTK_THREAD_BARRIER_SERIAL_THREAD;
	}
	mpc_common_spinlock_unlock(&barrier->spinlock);
	return ret;
}

/***************
* BARRIERATTR *
***************/

int _mpc_thread_ethread_posix_barrierattr_getpshared(const _mpc_thread_ethread_barrierattr_t *
                                                     attr, int *pshared)
{
	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}
	*pshared = attr->pshared;
	return 0;
}

int _mpc_thread_ethread_posix_barrierattr_setpshared(_mpc_thread_ethread_barrierattr_t *attr,
                                                     int pshared)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE &&
	   pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		return ENOTSUP;
	}
	attr->pshared = pshared;
	return 0;
}

int _mpc_thread_ethread_posix_barrierattr_init(_mpc_thread_ethread_barrierattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
	return 0;
}

int _mpc_thread_ethread_posix_barrierattr_destroy(_mpc_thread_ethread_barrierattr_t *attr)
{
	if(attr == NULL)
	{
		return EINVAL;
	}
	return 0;
}

/****************
* NON PORTABLE *
****************/

int _mpc_thread_ethread_posix_getattr_np(_mpc_thread_ethread_t th, _mpc_thread_ethread_attr_t *attr)
{
	attr->ptr = (_mpc_thread_ethread_attr_intern_t *)
	            sctk_malloc(sizeof(_mpc_thread_ethread_attr_intern_t) );
	*attr->ptr = th->attr;
	return 0;
}
