
//#include "mpcomp_lock.h"
#include <mpcomp.h>
 
void mpcomp_init_lock(mpcomp_lock_t *lock)
{
  sctk_thread_mutex_init(lock,NULL);
}

void mpcomp_init_nest_lock(mpcomp_nest_lock_t *lock)
{

 sctk_thread_mutexattr_t attr; 
 sctk_thread_mutexattr_init(&attr);
 sctk_thread_mutexattr_settype(&attr,SCTK_THREAD_MUTEX_RECURSIVE);
 sctk_thread_mutex_init(lock,&attr);
 sctk_thread_mutexattr_destroy(&attr);
}

void mpcomp_destroy_lock(mpcomp_lock_t *lock)
{
  sctk_thread_mutex_destroy(lock);
}

void mpcomp_destroy_nest_lock(mpcomp_nest_lock_t *lock) 
{

 sctk_thread_mutex_destroy(lock);

}

void mpcomp_set_lock(mpcomp_lock_t *lock)
{
 sctk_thread_mutex_lock(lock);
}

void mpcomp_set_nest_lock(mpcomp_nest_lock_t *lock)
{
 sctk_thread_mutex_lock(lock);
}

void mpcomp_unset_lock(mpcomp_lock_t *lock)
{
 sctk_thread_mutex_unlock(lock);
}

void mpcomp_unset_nest_lock(mpcomp_nest_lock_t *lock)
{ 
 sctk_thread_mutex_unlock(lock);
}

int mpcomp_test_lock(mpcomp_lock_t *lock)
{
 return (sctk_thread_mutex_trylock(lock) == 0);
}

int mpcomp_test_nest_lock(mpcomp_nest_lock_t *lock)
{
 return (sctk_thread_mutex_trylock(lock) == 0);
}
