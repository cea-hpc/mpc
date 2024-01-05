#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
#ifndef MPC_THREAD_H_
#define MPC_THREAD_H_

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#include <mpc_threads_config.h>
#include <mpc_thread_engines.h>

#if defined(SunOS_SYS) || defined(AIX_SYS) || defined(HP_UX_SYS)
/* typedef clockid_t __clockid_t; */
struct timespec;
#endif

#ifdef __cplusplus
extern "C"
{
#endif


#define SCTK_ETHREAD_STACK_SIZE            (10ull * 1024 * 1024)
#define SCTK_ETHREAD_THREAD_STACK_SIZE     (4ull * 1024 * 1024)
#define SCTK_ETHREAD_STACK_SIZE_FORTRAN    (1024ull * 1024 * 1024)

struct sched_param;
struct mpc_thread_rank_info_s;

int mpc_thread_atfork(void (*__prepare)(void),
                      void (*__parent)(void), void (*__child)(void) );
int mpc_thread_attr_destroy(mpc_thread_attr_t *__attr);
int mpc_thread_attr_getdetachstate(const mpc_thread_attr_t *__attr,
                                   int *__detachstate);
int mpc_thread_attr_getguardsize(const mpc_thread_attr_t *
                                 __attr, size_t *__guardsize);
int mpc_thread_attr_getinheritsched(const mpc_thread_attr_t *
                                    __attr, int *__inherit);
int mpc_thread_attr_getschedparam(const mpc_thread_attr_t *
                                  __attr, struct sched_param *__param);
int mpc_thread_attr_getschedpolicy(const mpc_thread_attr_t *__attr,
                                   int *__policy);
int mpc_thread_setaffinity_np(mpc_thread_t thread, size_t cpusetsize,
                              const mpc_cpu_set_t *cpuset);
int mpc_thread_getaffinity_np(mpc_thread_t thread, size_t cpusetsize,
                              mpc_cpu_set_t *cpuset);
int mpc_thread_attr_getscope(const mpc_thread_attr_t *__attr,
                             int *__scope);
int mpc_thread_attr_getstackaddr(const mpc_thread_attr_t *__attr,
                                 void **__stackaddr);
int mpc_thread_attr_getstack(const mpc_thread_attr_t *__attr,
                             void **__stackaddr, size_t *__stacksize);
int mpc_thread_attr_getstacksize(const mpc_thread_attr_t *__attr,
                                 size_t *__stacksize);
int mpc_thread_attr_init(mpc_thread_attr_t *__attr);
int mpc_thread_attr_setdetachstate(mpc_thread_attr_t *__attr,
                                   int __detachstate);
int mpc_thread_attr_setguardsize(mpc_thread_attr_t *__attr,
                                 size_t __guardsize);
int mpc_thread_attr_setinheritsched(mpc_thread_attr_t *__attr,
                                    int __inherit);
int mpc_thread_attr_setschedparam(mpc_thread_attr_t *__attr,
                                  const struct sched_param *__param);
int mpc_thread_attr_setschedpolicy(mpc_thread_attr_t *__attr,
                                   int __policy);
int mpc_thread_attr_setscope(mpc_thread_attr_t *__attr, int __scope);
int mpc_thread_attr_setstackaddr(mpc_thread_attr_t *__attr,
                                 void *__stackaddr);
int mpc_thread_attr_setstack(mpc_thread_attr_t *__attr,
                             void *__stackaddr, size_t __stacksize);
int mpc_thread_attr_setstacksize(mpc_thread_attr_t *__attr,
                                 size_t __stacksize);
int
mpc_thread_attr_setbinding(mpc_thread_attr_t *__attr, int __binding);
int
mpc_thread_attr_getbinding(mpc_thread_attr_t *__attr, int *__binding);


int mpc_thread_barrierattr_destroy(mpc_thread_barrierattr_t *__attr);
int mpc_thread_barrierattr_init(mpc_thread_barrierattr_t *__attr);
int mpc_thread_barrierattr_setpshared(mpc_thread_barrierattr_t *
                                      __attr, int __pshared);
int mpc_thread_core_barrier_destroy(mpc_thread_barrier_t *__barrier);
int mpc_thread_core_barrier_init(mpc_thread_barrier_t *__barrier,
                                 const mpc_thread_barrierattr_t *
                                 __attr, unsigned int __count);
int mpc_thread_core_barrier_wait(mpc_thread_barrier_t *__barrier);

int mpc_thread_cancel(mpc_thread_t __cancelthread);
int mpc_thread_condattr_destroy(mpc_thread_condattr_t *__attr);
int mpc_thread_condattr_getpshared(const mpc_thread_condattr_t *
                                   __attr, int *__pshared);
int mpc_thread_condattr_init(mpc_thread_condattr_t *__attr);
int mpc_thread_condattr_setpshared(mpc_thread_condattr_t *__attr,
                                   int __pshared);
int mpc_thread_condattr_setclock(mpc_thread_condattr_t *
                                 attr, clockid_t clock_id);
int mpc_thread_condattr_getclock(mpc_thread_condattr_t *
                                 attr, clockid_t *clock_id);
int mpc_thread_cond_broadcast(mpc_thread_cond_t *__cond);
int mpc_thread_cond_destroy(mpc_thread_cond_t *__cond);
int mpc_thread_cond_init(mpc_thread_cond_t *__cond,
                         const mpc_thread_condattr_t *__cond_attr);
int mpc_thread_cond_signal(mpc_thread_cond_t *__cond);
int mpc_thread_cond_timedwait(mpc_thread_cond_t *__cond,
                              mpc_thread_mutex_t *__mutex,
                              const struct timespec *__abstime);
int mpc_thread_cond_clockwait(mpc_thread_cond_t *__cond,
                              mpc_thread_mutex_t *__mutex,
                              clockid_t clock_id,
                              const struct timespec *__abstime);
int mpc_thread_cond_wait(mpc_thread_cond_t *__cond,
                         mpc_thread_mutex_t *__mutex);
int _mpc_thread_create_vp(mpc_thread_t *__threadp,
                          const mpc_thread_attr_t *__attr,
                          void *(*__start_routine)(void *),
                          void *__arg, long task_id, long local_task_id);
int
mpc_thread_core_thread_create(mpc_thread_t *__threadp,
                              const mpc_thread_attr_t *__attr,
                              void *(*__start_routine)(void *), void *__arg);

int mpc_thread_detach(mpc_thread_t __th);
int mpc_thread_equal(mpc_thread_t __thread1, mpc_thread_t __thread2);
void mpc_thread_exit(void *__retval);
int mpc_thread_getconcurrency(void);
int mpc_thread_getcpuclockid(mpc_thread_t __thread_id,
                             clockid_t *__clock_id);
int mpc_thread_getschedparam(mpc_thread_t __target_thread,
                             int *__policy, struct sched_param *__param);
void *mpc_thread_getspecific(mpc_thread_keys_t __key);
int mpc_thread_join(mpc_thread_t __th, void **__thread_return);
int mpc_thread_kill(mpc_thread_t thread, int signo);
int mpc_thread_sigsuspend(sigset_t *set);
int mpc_thread_process_kill(pid_t pid, int sig);
int mpc_thread_sigpending(sigset_t *set);
int mpc_thread_sigmask(int how, const sigset_t *newmask,
                       sigset_t *oldmask);
int mpc_thread_sigwait(const sigset_t *set, int *sig);
int mpc_thread_keys_create(mpc_thread_keys_t *__key,
                           void (*__destr_function)(void *) );
int mpc_thread_keys_delete(mpc_thread_keys_t __key);
int mpc_thread_mutexattr_destroy(mpc_thread_mutexattr_t *__attr);
int mpc_thread_mutexattr_getpshared(const mpc_thread_mutexattr_t *
                                    __attr, int *__pshared);
int mpc_thread_mutexattr_gettype(const mpc_thread_mutexattr_t *
                                 __attr, int *__kind);
int mpc_thread_mutexattr_init(mpc_thread_mutexattr_t *__attr);
int mpc_thread_mutexattr_setpshared(mpc_thread_mutexattr_t *__attr,
                                    int __pshared);
int mpc_thread_mutexattr_settype(mpc_thread_mutexattr_t *__attr,
                                 int __kind);
int mpc_thread_mutex_destroy(mpc_thread_mutex_t *__mutex);
int mpc_thread_mutex_init(mpc_thread_mutex_t *__mutex,
                          const mpc_thread_mutexattr_t *__mutex_attr);
int mpc_thread_mutex_lock(mpc_thread_mutex_t *__mutex);
int mpc_thread_mutex_spinlock(mpc_thread_mutex_t *__mutex);
int mpc_thread_mutex_timedlock(mpc_thread_mutex_t *__mutex,
                               const struct timespec *__abstime);
int mpc_thread_mutex_clocklock(mpc_thread_mutex_t *__mutex,
                               clockid_t clock_id,
                               const struct timespec *__abstime);
int mpc_thread_mutex_trylock(mpc_thread_mutex_t *__mutex);
int mpc_thread_mutex_unlock(mpc_thread_mutex_t *__mutex);

int mpc_thread_sem_init(mpc_thread_sem_t *sem, int pshared,
                        unsigned int value);
int mpc_thread_sem_wait(mpc_thread_sem_t *sem);
int mpc_thread_sem_trywait(mpc_thread_sem_t *sem);
int mpc_thread_sem_post(mpc_thread_sem_t *sem);
int mpc_thread_sem_getvalue(mpc_thread_sem_t *sem, int *sval);
int mpc_thread_sem_destroy(mpc_thread_sem_t *sem);
mpc_thread_sem_t *mpc_thread_sem_open(const char *__name,
                                      int __oflag, ...);
int mpc_thread_sem_close(mpc_thread_sem_t *__sem);
int mpc_thread_sem_unlink(const char *__name);
int mpc_thread_sem_timedwait(mpc_thread_sem_t *__sem,
                             const struct timespec *__abstime);

int mpc_thread_once(mpc_thread_once_t *__once_control,
                    void (*__init_routine)(void) );

int mpc_thread_rwlockattr_destroy(mpc_thread_rwlockattr_t *__attr);
int mpc_thread_rwlockattr_getpshared(const mpc_thread_rwlockattr_t *
                                     __attr, int *__pshared);
int mpc_thread_rwlockattr_init(mpc_thread_rwlockattr_t *__attr);
int mpc_thread_rwlockattr_setpshared(mpc_thread_rwlockattr_t *
                                     __attr, int __pshared);
int mpc_thread_rwlock_destroy(mpc_thread_rwlock_t *__rwlock);
int mpc_thread_rwlock_init(mpc_thread_rwlock_t *__rwlock,
                           const mpc_thread_rwlockattr_t *__attr);
int mpc_thread_rwlock_rdlock(mpc_thread_rwlock_t *__rwlock);

int mpc_thread_rwlock_timedrdlock(mpc_thread_rwlock_t *
                                  __rwlock,
                                  const struct timespec *__abstime);
int mpc_thread_rwlock_clockrdlock(mpc_thread_rwlock_t *
                                  __rwlock,
                                  clockid_t clock_id,
                                  const struct timespec *__abstime);
int mpc_thread_rwlock_timedwrlock(mpc_thread_rwlock_t *
                                  __rwlock,
                                  const struct timespec *__abstime);
int mpc_thread_rwlock_clockwrlock(mpc_thread_rwlock_t *
                                  __rwlock,
                                  clockid_t clock_id,
                                  const struct timespec *__abstime);

int mpc_thread_rwlock_tryrdlock(mpc_thread_rwlock_t *__rwlock);
int mpc_thread_rwlock_trywrlock(mpc_thread_rwlock_t *__rwlock);
int mpc_thread_rwlock_unlock(mpc_thread_rwlock_t *__rwlock);
int mpc_thread_rwlock_wrlock(mpc_thread_rwlock_t *__rwlock);

mpc_thread_t mpc_thread_self(void);

int mpc_thread_setcancelstate(int __state, int *__oldstate);
int mpc_thread_setcanceltype(int __type, int *__oldtype);
int mpc_thread_setconcurrency(int __level);
int mpc_thread_setschedparam(mpc_thread_t __target_thread,
                             int __policy,
                             const struct sched_param *__param);
int mpc_thread_setspecific(mpc_thread_keys_t __key,
                           const void *__pointer);

int mpc_thread_spin_destroy(mpc_thread_spinlock_t *__lock);
int mpc_thread_spin_init(mpc_thread_spinlock_t *__lock, int __pshared);
int mpc_thread_spin_lock(mpc_thread_spinlock_t *__lock);
int mpc_thread_spin_trylock(mpc_thread_spinlock_t *__lock);
int mpc_thread_spin_unlock(mpc_thread_spinlock_t *__lock);

void mpc_thread_testcancel(void);
int mpc_thread_yield(void);
unsigned int mpc_thread_sleep(unsigned int seconds);
int mpc_thread_usleep(unsigned int seconds);
int mpc_thread_nanosleep(const struct timespec *req,
                         struct timespec *rem);

int mpc_thread_getpriority_max(int policy);
int mpc_thread_getpriority_min(int policy);
int mpc_thread_attr_getschedpolicy(const mpc_thread_attr_t *, int *);
int mpc_thread_barrierattr_getpshared(const mpc_thread_barrierattr_t
                                      *, int *);
int mpc_thread_mutex_getprioceiling(const mpc_thread_mutex_t *, int *);
int mpc_thread_mutex_setprioceiling(mpc_thread_mutex_t *, int, int *);
int mpc_thread_mutexattr_getprioceiling(const mpc_thread_mutexattr_t
                                        *, int *);
int mpc_thread_mutexattr_setprioceiling(mpc_thread_mutexattr_t *, int);
int mpc_thread_mutexattr_getprotocol(const mpc_thread_mutexattr_t *
                                     attr, int *protocol);
int mpc_thread_mutexattr_setprotocol(mpc_thread_mutexattr_t *, int);

int mpc_thread_setschedprio(mpc_thread_t, int);
int mpc_thread_getattr_np(mpc_thread_t th, mpc_thread_attr_t *attr);


unsigned long mpc_thread_atomic_add(volatile unsigned long *ptr,
                                    unsigned long val);
unsigned long sctk_tls_entry_add(unsigned long size, void (*func)(void *) );
void sctk_tls_init_key(unsigned long key, void (*func)(void *) );

/* Futexes */

long  mpc_thread_futex(int sysop, void *addr1, int op, int val1,
                       struct timespec *timeout, void *addr2, int val3);
long  mpc_thread_futex_with_vaargs(int sysop, ...);

/* PUSH & POP */

void mpc_thread_cleanup_push(struct _sctk_thread_cleanup_buffer *__buffer,
                             void (*__routine)(void *),
                             void *__arg);


void mpc_thread_cleanup_pop(struct _sctk_thread_cleanup_buffer *__buffer, int __execute);

/* MPC_MPI Trampolines */

void mpc_thread_mpi_task_atexit(int (*trampoline)(void (*func)(void) ) );

struct mpc_mpi_cl_per_mpi_process_ctx_s;
void mpc_thread_mpi_ctx_set(struct mpc_mpi_cl_per_mpi_process_ctx_s * (*trampoline)(void) );

void mpc_thread_wait_for_value(volatile int *data, int value);
int mpc_thread_timed_wait_for_value(volatile int *data, int value, unsigned int max_time_in_usec);
void mpc_thread_wait_for_value_and_poll(volatile int *data, int value,
                                        void (*func)(void *), void *arg);
void
mpc_thread_kernel_wait_for_value_and_poll(int *data, int value,
                                          void (*func)(void *), void *arg);

void mpc_thread_freeze(mpc_thread_mutex_t *lock,
                       void **list);
void mpc_thread_wake(void **list);

void mpc_thread_spawn_mpi_tasks(void *(*run)(void *), void *arg);

/* Rank and topo getters */

struct mpc_thread_rank_info_s *mpc_thread_rank_info_get(void);
int mpc_thread_get_pu(void);
int mpc_thread_get_global_pu(void);
int mpc_thread_get_thread_id(void);
int mpc_thread_get_current_local_tasks_nb();
void *mpc_thread_get_parent_mpi_task_ctx(void);

/* Thread data */

struct sctk_thread_data_s;
typedef struct sctk_thread_data_s   sctk_thread_data_t;

sctk_thread_data_t *mpc_thread_data_get();

void *mpc_thread_mpi_data_get();
void mpc_thread_mpi_data_set(void *mpi_thread_ctx);

/* Thread MPI disguise */

typedef struct mpc_thread_mpi_disguise_s
{
	struct sctk_thread_data_s *my_disguisement;
	void *                     ctx_disguisement;
}mpc_thread_mpi_disguise_t;

mpc_thread_mpi_disguise_t *mpc_thread_disguise_get();
int mpc_thread_disguise_set(struct sctk_thread_data_s *th_data, void *mpi_ctx);


double mpc_thread_getactivity(int i);


int mpc_thread_get_task_placement_and_count(int i, int *nbVp);
int mpc_thread_get_task_placement(int i);

int mpc_thread_migrate_to_core(const int cpu);

/************************************************************************/
/* Internal MPC Futex Codes                                             */
/************************************************************************/


#define SCTK_FUTEX_WAIT               21210
#define SCTK_FUTEX_WAKE               21211
#define SCTK_FUTEX_REQUEUE            21212
#define SCTK_FUTEX_CMP_REQUEUE        21213
#define SCTK_FUTEX_WAKE_OP            21214
#define SCTK_FUTEX_WAIT_BITSET        21215
#define SCTK_FUTEX_WAKE_BITSET        21216
#define SCTK_FUTEX_LOCK_PI            21217
#define SCTK_FUTEX_TRYLOCK_PI         21218
#define SCTK_FUTEX_UNLOCK_PI          21219
#define SCTK_FUTEX_CMP_REQUEUE_PI     21220
#define SCTK_FUTEX_WAIT_REQUEUE_PI    21221

/* WAITERS */
#define SCTK_FUTEX_WAITERS            (1 << 31)

/* OPS */

#define SCTK_FUTEX_OP_SET          0
#define SCTK_FUTEX_OP_ADD          1
#define SCTK_FUTEX_OP_OR           2
#define SCTK_FUTEX_OP_ANDN         3
#define SCTK_FUTEX_OP_XOR          4
#define SCTK_FUTEX_OP_ARG_SHIFT    8

/* CMP */

#define SCTK_FUTEX_OP_CMP_EQ    0
#define SCTK_FUTEX_OP_CMP_NE    1
#define SCTK_FUTEX_OP_CMP_LT    2
#define SCTK_FUTEX_OP_CMP_LE    3
#define SCTK_FUTEX_OP_CMP_GT    4
#define SCTK_FUTEX_OP_CMP_GE    5


#ifdef __cplusplus
}
#endif
#endif /* MPC_THREAD_H_ */
