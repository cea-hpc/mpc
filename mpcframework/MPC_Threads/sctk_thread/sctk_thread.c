#define _GNU_SOURCE
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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <sctk_ib_cp.h>
#endif

#include <sctk_topology.h>

#undef sleep
#undef usleep
#define SCTK_DONOT_REDEFINE_KILL

#include "sctk_debug.h"
#include "sctk_config.h"
#include "sctk_thread.h"
#include "sctk_kernel_thread.h"
#include "sctk_alloc.h"
#include "sctk_launch.h"
#include "sctk_topology.h"
#include "sctk_asm.h"
#include "sctk_tls.h"
#include <extls.h>
#include <unistd.h>
#include "sctk_posix_thread.h"
#include "sctk_internal_thread.h"
#ifdef MPC_OpenMP
#include"mpcomp_internal.h"
#endif
#ifdef MPC_Debugger
#include "sctk_thread_dbg.h"
#endif
#include "sctk.h"
#include "sctk_context.h"
#include "sctk_runtime_config.h"
#include "sctk_runtime_config_struct_defaults.h"

#include "sctk_thread_generic.h"

#ifdef MPC_MPI
#include <mpc_internal_thread.h>
#include "mpc_datatypes.h"
#endif

#ifdef MPC_Message_Passing
#include <sctk_communicator.h>
#include "sctk_pmi.h"
#include "sctk_ib_prof.h"
#include <sctk_low_level_comm.h>
#endif
#include <errno.h>
#include "sctk_futex.h"

extern int errno;
typedef unsigned sctk_long_long sctk_timer_t;

volatile int start_func_done = 0;

static sctk_thread_key_t _sctk_thread_handler_key;

/* #define SCTK_CHECK_CODE_RETURN */
#ifdef SCTK_CHECK_CODE_RETURN
#define sctk_check(res,val) assume(res == val)
#else
#define sctk_check(res,val) (void)(0)
#endif

static volatile long sctk_nb_user_threads = 0;
extern volatile int sctk_online_program;

struct sctk_alloc_chain *sctk_thread_tls = NULL;

void
sctk_thread_init (void)
{
  sctk_thread_tls = sctk_get_current_alloc_chain();
#ifdef MPC_PosixAllocator
  assert(sctk_thread_tls != NULL);
#endif
#ifdef SCTK_CHECK_CODE_RETURN
  fprintf (stderr, "Thread librarie return code check enable!!\n");
#endif
  sctk_futex_context_init();
  /*Check all types */
}

int
sctk_thread_atfork (void (*__prepare) (void), void (*__parent) (void),
		    void (*__child) (void))
{
  int res;

  res = __sctk_ptr_thread_atfork (__prepare, __parent, __child);

  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_destroy (sctk_thread_attr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_attr_destroy (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getdetachstate (const sctk_thread_attr_t * __attr,
				 int *__detachstate)
{
  int res;
  res = __sctk_ptr_thread_attr_getdetachstate (__attr, __detachstate);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getguardsize (const sctk_thread_attr_t * restrict __attr,
			       size_t * restrict __guardsize)
{
  int res;
  res = __sctk_ptr_thread_attr_getguardsize (__attr, __guardsize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getinheritsched (const sctk_thread_attr_t *
				  restrict __attr, int *restrict __inherit)
{
  int res;
  res = __sctk_ptr_thread_attr_getinheritsched (__attr, __inherit);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getschedparam (const sctk_thread_attr_t * restrict __attr,
				struct sched_param *restrict __param)
{
  int res;
  res = __sctk_ptr_thread_attr_getschedparam (__attr, __param);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getschedpolicy (const sctk_thread_attr_t * restrict __attr,
				 int *restrict __policy)
{
  int res;
  res = __sctk_ptr_thread_attr_getschedpolicy (__attr, __policy);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getscope (const sctk_thread_attr_t * restrict __attr,
			   int *restrict __scope)
{
  int res;
  res = __sctk_ptr_thread_attr_getscope (__attr, __scope);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getstackaddr (const sctk_thread_attr_t * restrict __attr,
			       void **restrict __stackaddr)
{
  int res;
  res = __sctk_ptr_thread_attr_getstackaddr (__attr, __stackaddr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getstack (const sctk_thread_attr_t * restrict __attr,
			   void **restrict __stackaddr,
			   size_t * restrict __stacksize)
{
  int res;
  res = __sctk_ptr_thread_attr_getstack (__attr, __stackaddr, __stacksize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getstacksize (const sctk_thread_attr_t * restrict __attr,
			       size_t * restrict __stacksize)
{
  int res;
  res = __sctk_ptr_thread_attr_getstacksize (__attr, __stacksize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_init (sctk_thread_attr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_attr_init (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setdetachstate (sctk_thread_attr_t * __attr,
				 int __detachstate)
{
  int res;
  res = __sctk_ptr_thread_attr_setdetachstate (__attr, __detachstate);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setguardsize (sctk_thread_attr_t * __attr,
			       size_t __guardsize)
{
  int res;
  res = __sctk_ptr_thread_attr_setguardsize (__attr, __guardsize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setinheritsched (sctk_thread_attr_t * __attr, int __inherit)
{
  int res;
  res = __sctk_ptr_thread_attr_setinheritsched (__attr, __inherit);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setschedparam (sctk_thread_attr_t * restrict __attr,
				const struct sched_param *restrict __param)
{
  int res;
  res = __sctk_ptr_thread_attr_setschedparam (__attr, __param);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setschedpolicy (sctk_thread_attr_t * __attr, int __policy)
{
  int res;
  res = __sctk_ptr_thread_attr_setschedpolicy (__attr, __policy);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setscope (sctk_thread_attr_t * __attr, int __scope)
{
  int res;
  sctk_nodebug ("thread attr_setscope %d == %d || %d", __scope,
		SCTK_THREAD_SCOPE_PROCESS, SCTK_THREAD_SCOPE_SYSTEM);
  res = __sctk_ptr_thread_attr_setscope (__attr, __scope);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setstackaddr (sctk_thread_attr_t * __attr, void *__stackaddr)
{
  int res;
  res = __sctk_ptr_thread_attr_setstackaddr (__attr, __stackaddr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setstack (sctk_thread_attr_t * __attr, void *__stackaddr,
			   size_t __stacksize)
{
  int res;
  res = __sctk_ptr_thread_attr_setstack (__attr, __stackaddr, __stacksize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setstacksize (sctk_thread_attr_t * __attr,
			       size_t __stacksize)
{
  int res;
  res = __sctk_ptr_thread_attr_setstacksize (__attr, __stacksize);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_setbinding (sctk_thread_attr_t * __attr, int __binding)
{
  int res;
  res = __sctk_ptr_thread_attr_setbinding (__attr, __binding);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_attr_getbinding (sctk_thread_attr_t * __attr, int *__binding)
{
  int res;
  res = __sctk_ptr_thread_attr_getbinding (__attr, __binding);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrierattr_destroy (sctk_thread_barrierattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_barrierattr_destroy (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrierattr_init (sctk_thread_barrierattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_barrierattr_init (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrierattr_setpshared (sctk_thread_barrierattr_t * __attr,
				    int __pshared)
{
  int res;
  res = __sctk_ptr_thread_barrierattr_setpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrierattr_getpshared (const sctk_thread_barrierattr_t *
				    __attr, int *__pshared)
{
  int res;
  res = __sctk_ptr_thread_barrierattr_getpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrier_destroy (sctk_thread_barrier_t * __barrier)
{
  int res;
  res = __sctk_ptr_thread_barrier_destroy (__barrier);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrier_init (sctk_thread_barrier_t * restrict __barrier,
			  const sctk_thread_barrierattr_t * restrict __attr,
			  unsigned int __count)
{
  int res;
  res = __sctk_ptr_thread_barrier_init (__barrier, __attr, __count);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_barrier_wait (sctk_thread_barrier_t * __barrier)
{
  int res;
  res = __sctk_ptr_thread_barrier_wait (__barrier);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cancel (sctk_thread_t __cancelthread)
{
  int res;
  res = __sctk_ptr_thread_cancel (__cancelthread);
  sctk_check (res, 0);
  sctk_thread_yield ();
  return res;
}

int
sctk_thread_condattr_destroy (sctk_thread_condattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_condattr_destroy (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_condattr_getpshared (const sctk_thread_condattr_t *
				 restrict __attr, int *restrict __pshared)
{
  int res;
  res = __sctk_ptr_thread_condattr_getpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_condattr_init (sctk_thread_condattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_condattr_init (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_condattr_setpshared (sctk_thread_condattr_t * __attr,
				 int __pshared)
{
  int res;
  res = __sctk_ptr_thread_condattr_setpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_condattr_setclock (sctk_thread_condattr_t * __attr,
			       clockid_t __clockid)
{
  int res;
  res = __sctk_ptr_thread_condattr_setclock (__attr, __clockid);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_condattr_getclock (sctk_thread_condattr_t * __attr,
			       clockid_t * __clockid)
{
  int res;
  res = __sctk_ptr_thread_condattr_getclock (__attr, __clockid);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_broadcast (sctk_thread_cond_t * __cond)
{
  int res;
  res = __sctk_ptr_thread_cond_broadcast (__cond);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_destroy (sctk_thread_cond_t * __cond)
{
  int res;
  res = __sctk_ptr_thread_cond_destroy (__cond);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_init (sctk_thread_cond_t * restrict __cond,
		       const sctk_thread_condattr_t * restrict __cond_attr)
{
  int res;
  res = __sctk_ptr_thread_cond_init (__cond, __cond_attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_signal (sctk_thread_cond_t * __cond)
{
  int res;
  res = __sctk_ptr_thread_cond_signal (__cond);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_timedwait (sctk_thread_cond_t * restrict __cond,
			    sctk_thread_mutex_t * restrict __mutex,
			    const struct timespec *restrict __abstime)
{
  int res;
  res = __sctk_ptr_thread_cond_timedwait (__cond, __mutex, __abstime);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_cond_wait (sctk_thread_cond_t * restrict __cond,
		       sctk_thread_mutex_t * restrict __mutex)
{
  int res;
  res = __sctk_ptr_thread_cond_wait (__cond, __mutex);
  sctk_check (res, 0);
  return res;
}

static sctk_thread_data_t sctk_main_datas = SCTK_THREAD_DATA_INIT;

int
sctk_thread_get_vp ()
{
  return __sctk_ptr_thread_get_vp ();
}

/* Register, unregister running thread */
volatile int sctk_current_local_tasks_nb = 0;
sctk_thread_mutex_t sctk_current_local_tasks_lock =
  SCTK_THREAD_MUTEX_INITIALIZER;

void sctk_unregister_task (const int i){
  sctk_thread_mutex_lock (&sctk_current_local_tasks_lock);
  sctk_current_local_tasks_nb--;
  sctk_thread_mutex_unlock (&sctk_current_local_tasks_lock);
}

void sctk_register_task (const int i){
  sctk_thread_mutex_lock (&sctk_current_local_tasks_lock);
  sctk_current_local_tasks_nb++;
  sctk_thread_mutex_unlock (&sctk_current_local_tasks_lock);
}

/* Return the number of running tasks (i.e tasks already registered) */
int sctk_thread_get_current_local_tasks_nb() {
  return sctk_current_local_tasks_nb;
}

#include <dlfcn.h>
void __tbb_init_for_mpc(cpu_set_t * cpuset, int cpuset_len)
{
	void* next = dlsym(RTLD_NEXT, "__tbb_init_for_mpc");
	if(next)
	{
		void(*call)(cpu_set_t*,int) = (void(*)(cpu_set_t*,int))next;
		call(cpuset,cpuset_len);
	}
	else
	{
          sctk_nodebug("Calling fake TBB Initializer");
        }
}

void __tbb_finalize_for_mpc()
{
	void* next = dlsym(RTLD_NEXT, "__tbb_finalize_for_mpc");
	if(next)
	{
		void(*call)() = (void(*)())next;
		call();
	}
	else
	{
          sctk_nodebug("Calling fake TBB Finalizer");
        }
}

static void *
sctk_thread_create_tmp_start_routine (sctk_thread_data_t * __arg)
{
  void *res;
  sctk_thread_data_t tmp;
  struct _sctk_thread_cleanup_buffer *ptr_cleanup;
  tmp = *__arg;

  // DEBUG
  assume_m(sctk_get_cpu() == tmp.bind_to,
           "sctk_thread_create_tmp_start_routine BUGUED");
  // assert(sctk_get_cpu() == tmp.bind_to);
  // ENDDEBUG

  /* Bind the thread to the right core if we are using pthreads */
  if (sctk_get_thread_val() == sctk_pthread_thread_init) {
    sctk_bind_to_cpu (tmp.bind_to);
  }

  //mark the given TLS as currant thread allocator
  sctk_set_tls (tmp.tls);

  //do NUMA migration if required
  sctk_alloc_posix_numa_migrate();

  ptr_cleanup = NULL;
  sctk_thread_setspecific (_sctk_thread_handler_key, &ptr_cleanup);

  sctk_free (__arg);
  tmp.virtual_processor = sctk_thread_get_vp ();
  sctk_nodebug ("%d on %d", tmp.task_id, tmp.virtual_processor);

  sctk_thread_data_set (&tmp);
  sctk_thread_add (&tmp, sctk_thread_self());

  sctk_nodebug ("%d on %d", tmp.task_id, tmp.virtual_processor);

  /* Initialization of the profiler */
#ifdef MPC_Profiler
	sctk_internal_profiler_init();
#endif

#ifdef MPC_Message_Passing
  if (tmp.task_id >= 0)
    {
      sctk_ptp_per_task_init (tmp.task_id);
#ifdef MPC_USE_INFINIBAND
      sctk_network_initialize_task_collaborative_ib (tmp.task_id, tmp.virtual_processor);
#endif

#if defined(SCTK_USE_TLS)
      /* TLS INTIALIZATION */
      sctk_tls_init();
#endif

      sctk_register_thread_initial (tmp.task_id);
      sctk_terminaison_barrier (tmp.task_id);
      sctk_online_program = 1;
#ifdef MPC_USE_INFINIBAND
      sctk_ib_prof_init_reference_clock();
#endif
      sctk_terminaison_barrier (tmp.task_id);
    }
#endif

  /** ** **/
  sctk_report_creation (sctk_thread_self());
  /** **/

#ifndef MPC_Message_Passing
#ifdef MPC_OpenMP
  __mpcomp_init() ;
#endif
#endif
  // hmt
  // thread priority

  // sctk_thread_generic_scheduler_t* sched;
  // sched = &(sctk_thread_generic_self()->sched);
  // printf("THREAD PRIORITY : %d %d\n",
  //        sched->th->attr.kind.mask,
  //        sched->th->attr.kind.priority
  //      );

  // printf("BEFORE MPI: %d\n", sctk_thread_generic_getkind_mask_self());
  // set the KIND_MASK_MPI to the current thread
  sctk_thread_generic_addkind_mask_self(KIND_MASK_MPI);
  sctk_thread_generic_set_basic_priority_self(
      sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority);
  sctk_thread_generic_setkind_priority_self(
      sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority);
  sctk_thread_generic_set_current_priority_self(
      sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority);
  // printf("AFTER MPI: %d\n", sctk_thread_generic_getkind_mask_self());

  // printf("THREAD PRIORITY : %d %d\n",
  //        sctk_thread_generic_getkind_mask_self(),
  //        sctk_thread_generic_getkind_priority_self()
  //      );

  // endthread priority
  // endhmt

#if defined(MPC_USE_CUDA)
  sctk_thread_yield();
  sctk_accl_cuda_init_context();
#endif

  sctk_tls_dtors_init(&(tmp.dtors_head));
  
  /* BEGIN TBB SETUP */
  /**
   * #define macros are not used for TBB code injections
   * avoiding MPC recompilation when the user code wants to load TBB
   * Instead, we define two functions :
   *    - __tbb_init_for_mpc ( called around sctk_thread.c:643)
   *    - __tbb_finalize_for_mpc (called around sctk_thread.c:658)
   *
   * These functions are set via #pragma weak, empty when TBB is not
   * loaded and bound to our patchs in TBB when loaded.
   *
   * TODO: due to some issues, weak functions are replaced by dlsym accesses for now
   */
  int init_cpu = sctk_get_cpu();
  int nbvps, i, nb_cpusets;
  cpu_set_t * cpuset;

  sctk_get_init_vp_and_nbvp (sctk_get_task_rank(), &nbvps);
  init_cpu = sctk_topology_convert_logical_pu_to_os(init_cpu);
  
  nb_cpusets = 1;
  cpuset = sctk_malloc(sizeof(cpu_set_t) * nb_cpusets);

  CPU_ZERO(cpuset);
 
  for (i = 0; i < nbvps; ++i) {
	CPU_SET(init_cpu + i, cpuset);
  }
  
  __tbb_init_for_mpc(cpuset, nb_cpusets);
  /* END TBB SETUP */

  res = tmp.__start_routine (tmp.__arg);

  /** ** **/
  sctk_report_death (sctk_thread_self());
  /** **/

  /* BEGIN TBB FINALIZE */
  __tbb_finalize_for_mpc();
  /* END TBB FINALIZE */
  
  sctk_tls_dtors_free(&(tmp.dtors_head));

#ifdef MPC_Message_Passing
  if (tmp.task_id >= 0)
    {
      sctk_nodebug ("sctk_terminaison_barrier");
      sctk_terminaison_barrier (tmp.task_id);
      sctk_online_program = 0;
      sctk_terminaison_barrier (tmp.task_id);
      sctk_nodebug ("sctk_terminaison_barrier done");
#ifdef MPC_USE_INFINIBAND
      sctk_network_finalize_task_collaborative_ib(tmp.task_id);
#endif
      sctk_unregister_task (tmp.task_id);
      sctk_net_send_task_end (tmp.task_id, sctk_process_rank);
    }
  else
    {
      not_reachable ();
    }
#else
  sctk_thread_mutex_lock (&sctk_current_local_tasks_lock);
  sctk_current_local_tasks_nb--;
  sctk_thread_mutex_unlock (&sctk_current_local_tasks_lock);
#endif

  sctk_thread_remove (&tmp);
  sctk_thread_data_set (NULL);
  sctk_nodebug ("THREAD END");
  return res;
}

int
sctk_thread_create (sctk_thread_t * restrict __threadp,
		    const sctk_thread_attr_t * restrict __attr,
		    void *(*__start_routine) (void *), void *restrict __arg,
		    long task_id, long local_task_id)
{
  int res;
  sctk_thread_data_t *tmp;
  struct sctk_alloc_chain *tls;
  int previous_binding;
  static unsigned int core = 0;
  int new_binding;

  /* We bind the parent thread to the vp where the child
   * will be created. We must bind before calling
   * __sctk_crete_thread_memory_area(). The child thread
   * will be bound to the same core after its creation */

  new_binding = sctk_get_init_vp (core);

  core++;

  previous_binding = sctk_bind_to_cpu (new_binding);

  // DEBUG
  assert(new_binding == sctk_get_cpu());
  // ENDDEBUG

  tls = __sctk_create_thread_memory_area ();
  sctk_nodebug ("create tls %p", tls);
  assume (tls != NULL);
  tmp = (sctk_thread_data_t *)
    __sctk_malloc (sizeof (sctk_thread_data_t), tls);
  sctk_nodebug ("create thread_data %p", tmp);

  tmp->tls = tls;
  tmp->__arg = __arg;
  tmp->__start_routine = __start_routine;
  tmp->user_thread = 0;
  tmp->bind_to = new_binding;

  tmp->task_id = sctk_safe_cast_long_int (task_id);
  tmp->local_task_id = sctk_safe_cast_long_int (local_task_id);

#ifdef SCTK_USE_TLS
  extls_ctx_t* old_ctx = sctk_extls_storage;
  extls_ctx_t** cur_tx = ((extls_ctx_t**)extls_get_context_storage_addr());
  *cur_tx = malloc(sizeof(extls_ctx_t));
  extls_ctx_herit(old_ctx, *cur_tx, LEVEL_TASK);
  extls_ctx_restore(*cur_tx);
#ifndef MPC_DISABLE_HLS
  extls_ctx_bind(*cur_tx, tmp->bind_to);
#endif
#endif

  // create user thread
  res = __sctk_ptr_thread_create (__threadp, __attr, (void *(*)(void *))
				  sctk_thread_create_tmp_start_routine,
				  (void *) tmp);

  /* We reset the binding */
  {
    sctk_bind_to_cpu(previous_binding);
  	extls_ctx_restore(old_ctx);
  }

  sctk_check (res, 0);
  return res;
}


extern void MPC_Init_thread_specific();
extern void MPC_Release_thread_specific();

static void sctk_thread_data_reset();
static void *
sctk_thread_create_tmp_start_routine_user (sctk_thread_data_t * __arg)
{
  void *res;
  sctk_thread_data_t tmp;

  while (start_func_done == 0)
    sctk_thread_yield();
  sctk_thread_data_reset();

  /* FIXME Intel OMP: at some point, in pthread mode, the ptr_cleanup variable seems to
   * be corrupted. */
  struct _sctk_thread_cleanup_buffer ** ptr_cleanup = malloc(
      sizeof(struct _sctk_thread_cleanup_buffer*));
  tmp = *__arg;


  sctk_set_tls (tmp.tls);
  *ptr_cleanup = NULL;
  sctk_thread_setspecific (_sctk_thread_handler_key, ptr_cleanup);

#ifdef MPC_MPI
  __MPC_reinit_task_specific (tmp.father_data);
#endif

  sctk_free (__arg);
  tmp.virtual_processor = sctk_thread_get_vp ();
  sctk_nodebug("%d on %d", tmp.task_id, tmp.virtual_processor);

  sctk_thread_data_set (&tmp);
  sctk_thread_add (&tmp,sctk_thread_self());

  sctk_alloc_posix_numa_migrate();


  /* Note that the profiler is not initialized in user threads */

  /** ** **/

  sctk_report_creation (sctk_thread_self());
  /** **/


#ifdef MPC_MPI
   MPC_Init_thread_specific();
#endif

   // hmt
   // printf("BEFORE PTHREAD: %d\n", sctk_thread_generic_getkind_mask_self());
   sctk_thread_generic_addkind_mask_self(KIND_MASK_PTHREAD);
   sctk_thread_generic_set_basic_priority_self(
       sctk_runtime_config_get()->modules.scheduler.posix_basic_priority);
   sctk_thread_generic_setkind_priority_self(
       sctk_runtime_config_get()->modules.scheduler.posix_basic_priority);
   sctk_thread_generic_set_current_priority_self(
       sctk_runtime_config_get()->modules.scheduler.posix_basic_priority);
   // printf("AFTER PTHREAD: %d\n", sctk_thread_generic_getkind_mask_self());
   // endhmt

   res = tmp.__start_routine(tmp.__arg);

#ifdef MPC_MPI
  MPC_Release_thread_specific();
#endif

  /** ** **/
  sctk_report_death (sctk_thread_self());
  /** **/

  sctk_free(ptr_cleanup);
  sctk_thread_remove (&tmp);
  sctk_thread_data_set (NULL);
  return res;
}


/*
 * Sylvain: the following code was use for creating Intel OpenMP threads with MPC.
 * The TLS variables are automatically recopied into the OpenMP threads
 * */
#if 0
#include <dlfcn.h>
#define LIB "/lib64/libpthread.so.0"

int sctk_real_pthread_create (pthread_t  *thread,
    __const pthread_attr_t *attr, void * (*start_routine)(void *), void *arg) {
  static void *handle = NULL;
  static int (*real_pthread_create)(pthread_t  *,
      __const pthread_attr_t *,
      void * (*)(void *), void *) = NULL;

  if (real_pthread_create == NULL)
  {
    handle = dlopen(LIB,RTLD_LAZY);
    if (handle == NULL)
    {
      fprintf(stderr,"dlopen(%s) failed\n",LIB);
      exit(1);
    }
    real_pthread_create = dlsym(handle,"pthread_create");
    if (real_pthread_create == NULL)
    {
      fprintf(stderr,"dlsym(%s) failed\n","pthread_create");
      exit(1);
    }
  }
  real_pthread_create(thread, attr, start_routine, arg);
}

__thread pthread_t *sctk_pthread_bypass_thread = NULL;
int sctk_pthread_bypass_create(pthread_t  *thread,
    __const pthread_attr_t *attr,
    void * (*start_routine)(void *), void * arg) {
  int ret;
  int recursive = 0;

  if (sctk_pthread_bypass_thread == thread) {
    recursive = 1;
  }

  sctk_pthread_bypass_thread = thread;

  /* Check if we are trying to create a thread previously passed here */

  sctk_warning("PTHREAD CREATE %p!!", thread);
  if (sctk_thread_data_get()->task_id == -1 || recursive) {
    sctk_warning("NOT MPI task creates a thread",sctk_thread_data_get()->task_id);
    ret=sctk_real_pthread_create(thread,attr, start_routine, arg);
  } else {
    sctk_net_set_mode_hybrid();
    sctk_warning("MPI task %d creates a thread",sctk_thread_data_get()->task_id);
//	  sctk_thread_attr_setscope (attr, SCTK_THREAD_SCOPE_SYSTEM);
  	ret=sctk_user_thread_create (thread, attr, start_routine, arg);
  }
  sctk_pthread_bypass_thread = NULL;
  return ret;
}
#endif

int
sctk_user_thread_create (sctk_thread_t * restrict __threadp,
			 const sctk_thread_attr_t * restrict __attr,
			 void *(*__start_routine) (void *),
			 void *restrict __arg)
{
  int res;
  sctk_thread_data_t *tmp;
  sctk_thread_data_t *tmp_father;
  struct sctk_alloc_chain *tls;
  static sctk_spinlock_t lock = 0;
  int user_thread;
  int scope_init;

  sctk_spinlock_lock (&lock);
  sctk_nb_user_threads++;
  user_thread = sctk_nb_user_threads;

  sctk_spinlock_unlock (&lock);

  tls = __sctk_create_thread_memory_area ();
  sctk_nodebug ("create tls %p attr %p", tls, __attr);
  tmp = (sctk_thread_data_t *)
    __sctk_malloc (sizeof (sctk_thread_data_t), tls);

  tmp_father = sctk_thread_data_get ();

  tmp->tls = tls;
  tmp->__arg = __arg;
  tmp->__start_routine = __start_routine;
  tmp->task_id = -1;
  tmp->user_thread = user_thread;
#ifdef MPC_MPI
  tmp->father_data = __MPC_get_task_specific ();
#else
  tmp->father_data = NULL;
#endif

  tmp->tls_level = LEVEL_THREAD;

  if (tmp_father)
    {
      tmp->task_id = tmp_father->task_id;
    }
  sctk_nodebug("Create Thread with MPI rank %d", tmp->task_id);

#if 0
  /* Must be disabled because it unbind midcro VPs */
  if (__attr != NULL)
    {
      sctk_thread_attr_getscope (__attr, &scope_init);
      sctk_nodebug ("Thread to create with scope %d ", scope_init);
      if (scope_init == SCTK_THREAD_SCOPE_SYSTEM) {
        sctk_restrict_binding();
      }
    } else {
        sctk_restrict_binding();
    }
#endif

#ifdef SCTK_USE_TLS
  extls_ctx_t* old_ctx = sctk_extls_storage;
  extls_ctx_t** cur_tx = ((extls_ctx_t**)extls_get_context_storage_addr());
  *cur_tx = malloc(sizeof(extls_ctx_t));
  extls_ctx_herit(old_ctx, *cur_tx, LEVEL_THREAD);
  extls_ctx_restore(*cur_tx);
#endif

  res = __sctk_ptr_thread_user_create (__threadp, __attr,
				       (void *(*)(void *))
				       sctk_thread_create_tmp_start_routine_user,
				       (void *) tmp);
  sctk_check (res, 0);

#ifdef SCTK_USE_TLS
  extls_ctx_restore(old_ctx);
#endif

#ifndef NO_INTERNAL_ASSERT
  if (__attr != NULL)
    {
      sctk_thread_attr_getscope (__attr, &scope_init);
      sctk_nodebug ("Thread created with scope %d", scope_init);
    }
#endif

  return res;
}

int
sctk_thread_detach (sctk_thread_t __th)
{
  int res;
  res = __sctk_ptr_thread_detach (__th);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_equal (sctk_thread_t __thread1, sctk_thread_t __thread2)
{
  int res;
  res = __sctk_ptr_thread_equal (__thread1, __thread2);
  sctk_check (res, 0);
  return res;
}

/* At exit generic trampoline */

int sctk_atexit(void (*function)(void))
{
#ifdef MPC_MPI
	/* We may have a TASK context replacing the proces one */
	sctk_info("Calling the MPC atexit function");
	int ret  = __MPC_atexit_task_specific( function );
	
	if( ret == 0 )
	{
		/* We were in a task and managed to register ourselves */
		return ret;
	}
	/* It failed we may not be in a task then call libc */
	sctk_info("Calling the default atexit function");
#endif
	/* We have no task level fallback to libc */
	return atexit( function );
}



/* Futex Generic Trampoline */

int  sctk_thread_futex(void *addr1, int op, int val1, 
					  struct timespec *timeout, void *addr2, int val3)
{
	return __sctk_ptr_thread_futex(addr1, op, val1, timeout, addr2, val3);
}


static void
_sctk_thread_cleanup_end (struct _sctk_thread_cleanup_buffer **__buffer)
{
  if (__buffer != NULL)
    {
      sctk_nodebug ("end %p %p", __buffer, *__buffer);
      if (*__buffer != NULL)
	{
	  struct _sctk_thread_cleanup_buffer *tmp;
	  tmp = *__buffer;
	  while (tmp != NULL)
	    {
	      tmp->__routine (tmp->__arg);
	      tmp = tmp->next;
	    }
	}
    }
}

void
sctk_thread_exit_cleanup ()
{
  sctk_thread_data_t *tmp;
  struct _sctk_thread_cleanup_buffer **__head;


  /** ** **/
  sctk_report_death (sctk_thread_self());
  /** **/
  tmp = sctk_thread_data_get ();

  sctk_thread_remove (tmp);

  __head = sctk_thread_getspecific (_sctk_thread_handler_key);

  _sctk_thread_cleanup_end (__head);

  sctk_thread_setspecific (_sctk_thread_handler_key, NULL);


  sctk_nodebug ("%p", tmp);
  if (tmp != NULL)
    {
      sctk_nodebug ("ici %p %d", tmp, tmp->task_id);
#ifdef MPC_Message_Passing
      if (tmp->task_id >= 0 && tmp->user_thread == 0)
	{
	  sctk_nodebug ("sctk_terminaison_barrier");
	  sctk_terminaison_barrier (tmp->task_id);
	  sctk_nodebug ("sctk_terminaison_barrier done");
	  sctk_unregister_task (tmp->task_id);
	  sctk_net_send_task_end (tmp->task_id, sctk_process_rank);
	}
#endif
    }
}

void
sctk_thread_exit (void *__retval)
{
  sctk_thread_exit_cleanup ();
  __sctk_ptr_thread_exit (__retval);
}

int
sctk_thread_getconcurrency (void)
{
  int res;
  res = __sctk_ptr_thread_getconcurrency ();
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_getcpuclockid (sctk_thread_t __thread_id, clockid_t * __clock_id)
{
  int res;
  res = __sctk_ptr_thread_getcpuclockid (__thread_id, __clock_id);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sched_get_priority_max (int policy)
{
  int res;
  res = __sctk_ptr_thread_sched_get_priority_max (policy);
  return res;
}

int
sctk_thread_sched_get_priority_min (int policy)
{
  int res;
  res = __sctk_ptr_thread_sched_get_priority_min (policy);
  return res;
}

int
sctk_thread_getschedparam (sctk_thread_t __target_thread,
			   int *restrict __policy,
			   struct sched_param *restrict __param)
{
  int res;
  res = __sctk_ptr_thread_getschedparam (__target_thread, __policy, __param);
  sctk_check (res, 0);
  return res;
}

void *
sctk_thread_getspecific (sctk_thread_key_t __key)
{
  return __sctk_ptr_thread_getspecific (__key);
}

int
sctk_thread_join (sctk_thread_t __th, void **__thread_return)
{
  int res;
  res = __sctk_ptr_thread_join (__th, __thread_return);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_kill (sctk_thread_t thread, int signo)
{
  int res;
  res = __sctk_ptr_thread_kill (thread, signo);
  sctk_check (res, 0);
  sctk_thread_yield ();
  return res;
}

int
sctk_thread_process_kill (pid_t pid, int sig)
{
#ifndef WINDOWS_SYS
  int res;
  res = kill (pid, sig);
  sctk_check (res, 0);
  sctk_thread_yield ();
  return res;
#else
  not_available ();
  return 1;
#endif
}

int
sctk_thread_sigmask (int how, const sigset_t * newmask, sigset_t * oldmask)
{
  int res;
  res = __sctk_ptr_thread_sigmask (how, newmask, oldmask);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sigwait (const sigset_t * set, int *sig)
{
  not_implemented ();
  assume (set == NULL);
  assume (sig == NULL);
  return 0;
}

int
sctk_thread_sigpending (sigset_t * set)
{
  int res;
  res = __sctk_ptr_thread_sigpending (set);
  sctk_check (res, 0);
  sctk_thread_yield ();
  return res;
}

int
sctk_thread_sigsuspend (sigset_t * set)
{
  int res;
  res = __sctk_ptr_thread_sigsuspend (set);
  sctk_check (res, 0);
  sctk_thread_yield ();
  return res;
}

/*int
sctk_thread_sigaction( int signum, const struct sigaction* act,
		struct sigaction* oldact ){
  int res;
  res = __sctk_ptr_thread_sigaction( signum, act, oldact );
  return res;
}*/

int
sctk_thread_key_create (sctk_thread_key_t * __key,
			void (*__destr_function) (void *))
{
  int res;
  res = __sctk_ptr_thread_key_create (__key, __destr_function);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_key_delete (sctk_thread_key_t __key)
{
  int res;
  res = __sctk_ptr_thread_key_delete (__key);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_destroy (sctk_thread_mutexattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_destroy (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_getpshared (const sctk_thread_mutexattr_t *
				  restrict __attr, int *restrict __pshared)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_getpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_getprioceiling (const sctk_thread_mutexattr_t *
				      attr, int *prioceiling)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_getprioceiling (attr, prioceiling);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t * attr,
				      int prioceiling)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_setprioceiling (attr, prioceiling);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_getprotocol (const sctk_thread_mutexattr_t *
				   attr, int *protocol)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_getprotocol (attr, protocol);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t * attr,
				   int protocol)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_setprotocol (attr, protocol);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_gettype (const sctk_thread_mutexattr_t *
			       restrict __attr, int *restrict __kind)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_gettype (__attr, __kind);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_init (sctk_thread_mutexattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_init (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * __attr,
				  int __pshared)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_setpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutexattr_settype (sctk_thread_mutexattr_t * __attr, int __kind)
{
  int res;
  res = __sctk_ptr_thread_mutexattr_settype (__attr, __kind);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_destroy (sctk_thread_mutex_t * __mutex)
{
  int res;
  res = __sctk_ptr_thread_mutex_destroy (__mutex);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_init (sctk_thread_mutex_t * restrict __mutex,
			const sctk_thread_mutexattr_t * restrict __mutex_attr)
{
  int res;
  res = __sctk_ptr_thread_mutex_init (__mutex, __mutex_attr);
  sctk_check (res, 0);
  return res;
}

#pragma weak user_sctk_thread_mutex_lock=sctk_thread_mutex_lock
int
sctk_thread_mutex_lock (sctk_thread_mutex_t * __mutex)
{
  int res;
  res = __sctk_ptr_thread_mutex_lock (__mutex);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_spinlock (sctk_thread_mutex_t * __mutex)
{
  int res;
  res = __sctk_ptr_thread_mutex_spinlock (__mutex);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_timedlock (sctk_thread_mutex_t * restrict __mutex,
			     const struct timespec *restrict __abstime)
{
  int res;
  res = __sctk_ptr_thread_mutex_timedlock (__mutex, __abstime);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_trylock (sctk_thread_mutex_t * __mutex)
{
  int res;
  res = __sctk_ptr_thread_mutex_trylock (__mutex);
  sctk_check (res, 0);
  return res;
}

#pragma weak user_sctk_thread_mutex_unlock=sctk_thread_mutex_unlock
int
sctk_thread_mutex_unlock (sctk_thread_mutex_t * __mutex)
{
  int res;
  res = __sctk_ptr_thread_mutex_unlock (__mutex);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_mutex_getprioceiling (const sctk_thread_mutex_t * restrict mutex,
				  int *restrict prioceiling)
{
  int res = 0;
  not_implemented ();
  return res;
}

int
sctk_thread_mutex_setprioceiling (sctk_thread_mutex_t * restrict mutex,
				  int prioceiling, int *restrict old_ceiling)
{
  int res = 0;
  not_implemented ();
  return res;
}

int
sctk_thread_sem_init (sctk_thread_sem_t * sem, int pshared,
		      unsigned int value)
{
  int res;
  res = __sctk_ptr_thread_sem_init (sem, pshared, value);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sem_wait (sctk_thread_sem_t * sem)
{
  int res;
  res = __sctk_ptr_thread_sem_wait (sem);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sem_trywait (sctk_thread_sem_t * sem)
{
  int res;
  res = __sctk_ptr_thread_sem_trywait (sem);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sem_post (sctk_thread_sem_t * sem)
{
  int res;
  res = __sctk_ptr_thread_sem_post (sem);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sem_getvalue (sctk_thread_sem_t * sem, int *sval)
{
  int res;
  res = __sctk_ptr_thread_sem_getvalue (sem, sval);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_sem_destroy (sctk_thread_sem_t * sem)
{
  int res;
  res = __sctk_ptr_thread_sem_destroy (sem);
  sctk_check (res, 0);
  return res;
}

sctk_thread_sem_t *
sctk_thread_sem_open (const char *__name, int __oflag, ...)
{
  sctk_thread_sem_t *tmp;
  if ((__oflag & SCTK_O_CREAT))
    {

      va_list ap;
      int value;
	  mode_t mode;
      va_start (ap, __oflag);
      mode = va_arg (ap, mode_t);
      value = va_arg (ap, int);
      va_end (ap);
      tmp = __sctk_ptr_thread_sem_open (__name, __oflag, mode, value);
    }
  else
    {
      tmp = __sctk_ptr_thread_sem_open (__name, __oflag);
    }
  return tmp;
}

int
sctk_thread_sem_close (sctk_thread_sem_t * __sem)
{
  int res = 0;
  res = __sctk_ptr_thread_sem_close (__sem);
  return res;
}

int
sctk_thread_sem_unlink (const char *__name)
{
  int res = 0;
  res = __sctk_ptr_thread_sem_unlink (__name);
  return res;
}

int
sctk_thread_sem_timedwait (sctk_thread_sem_t * __sem,
			   const struct timespec *__abstime)
{
  int res = 0;
  res = __sctk_ptr_thread_sem_timedwait( __sem, __abstime);
  return res;
}


int
sctk_thread_once (sctk_thread_once_t * __once_control,
		  void (*__init_routine) (void))
{
  int res;
  res = __sctk_ptr_thread_once (__once_control, __init_routine);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_rwlockattr_destroy (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlockattr_getpshared (const sctk_thread_rwlockattr_t *
				   restrict __attr, int *restrict __pshared)
{
  int res;
  res = __sctk_ptr_thread_rwlockattr_getpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlockattr_init (sctk_thread_rwlockattr_t * __attr)
{
  int res;
  res = __sctk_ptr_thread_rwlockattr_init (__attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t * __attr,
				   int __pshared)
{
  int res;
  res = __sctk_ptr_thread_rwlockattr_setpshared (__attr, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_destroy (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_destroy (__rwlock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_init (sctk_thread_rwlock_t * restrict __rwlock,
			 const sctk_thread_rwlockattr_t * restrict __attr)
{
  int res;
  res = __sctk_ptr_thread_rwlock_init (__rwlock, __attr);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_rdlock (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_rdlock (__rwlock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_timedrdlock (sctk_thread_rwlock_t * restrict __rwlock,
				const struct timespec *restrict __abstime)
{
  int res;
  res = __sctk_ptr_thread_rwlock_timedrdlock (__rwlock, __abstime);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_timedwrlock (sctk_thread_rwlock_t * restrict __rwlock,
				const struct timespec *restrict __abstime)
{
  int res;
  res = __sctk_ptr_thread_rwlock_timedwrlock (__rwlock, __abstime);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_tryrdlock (__rwlock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_trywrlock (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_trywrlock (__rwlock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_unlock (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_unlock (__rwlock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_rwlock_wrlock (sctk_thread_rwlock_t * __rwlock)
{
  int res;
  res = __sctk_ptr_thread_rwlock_wrlock (__rwlock);
  sctk_check (res, 0);
  return res;
}

sctk_thread_t
sctk_thread_self (void)
{
  return __sctk_ptr_thread_self ();
}

sctk_thread_t
sctk_thread_self_check (void)
{
  return __sctk_ptr_thread_self_check ();
}

int
sctk_thread_setcancelstate (int __state, int *__oldstate)
{
  int res;
  res = __sctk_ptr_thread_setcancelstate (__state, __oldstate);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_setcanceltype (int __type, int *__oldtype)
{
  int res;
  res = __sctk_ptr_thread_setcanceltype (__type, __oldtype);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_setconcurrency (int __level)
{
  int res;
  res = __sctk_ptr_thread_setconcurrency (__level);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_setschedprio (sctk_thread_t __p, int __i)
{
  int res;
  res = __sctk_ptr_thread_setschedprio (__p, __i);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_setschedparam (sctk_thread_t __target_thread, int __policy,
			   const struct sched_param *__param)
{
  int res;
  res = __sctk_ptr_thread_setschedparam (__target_thread, __policy, __param);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_setspecific (sctk_thread_key_t __key, const void *__pointer)
{
  int res;
  res = __sctk_ptr_thread_setspecific (__key, __pointer);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_spin_destroy (sctk_thread_spinlock_t * __lock)
{
  int res;
  res = __sctk_ptr_thread_spin_destroy (__lock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_spin_init (sctk_thread_spinlock_t * __lock, int __pshared)
{
  int res;
  res = __sctk_ptr_thread_spin_init (__lock, __pshared);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_spin_lock (sctk_thread_spinlock_t * __lock)
{
  int res;
  res = __sctk_ptr_thread_spin_lock (__lock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_spin_trylock (sctk_thread_spinlock_t * __lock)
{
  int res;
  res = __sctk_ptr_thread_spin_trylock (__lock);
  sctk_check (res, 0);
  return res;
}

int
sctk_thread_spin_unlock (sctk_thread_spinlock_t * __lock)
{
  int res;
  res = __sctk_ptr_thread_spin_unlock (__lock);
  sctk_check (res, 0);
  return res;
}

void
sctk_thread_testcancel (void)
{
  __sctk_ptr_thread_testcancel ();
}

int
sctk_thread_yield (void)
{
  return __sctk_ptr_thread_yield ();
}

typedef struct sctk_thread_sleep_pool_s
{
  sctk_timer_t wake_time;
  int done;
} sctk_thread_sleep_pool_t;

static void
sctk_thread_sleep_pool (sctk_thread_sleep_pool_t * wake_time)
{
  if (wake_time->wake_time < sctk_timer)
    {
      wake_time->done = 0;
    }
}

unsigned int
sctk_thread_sleep (unsigned int seconds)
{
  __sctk_ptr_thread_testcancel ();
  sctk_thread_sleep_pool_t wake_time;

  wake_time.done = 1;
  wake_time.wake_time =
    (((sctk_timer_t) seconds * (sctk_timer_t) 1000) /
     (sctk_timer_t) sctk_time_interval) + sctk_timer + 1;
  sctk_thread_yield ();

  __sctk_ptr_thread_testcancel ();
  sctk_thread_wait_for_value_and_poll (&(wake_time.done), 0,
				       (void (*)(void *))
				       sctk_thread_sleep_pool,
				       (void *) &wake_time);
  __sctk_ptr_thread_testcancel ();
  return 0;
}

int
sctk_thread_usleep (unsigned int useconds)
{
  __sctk_ptr_thread_testcancel ();
  sctk_thread_sleep_pool_t wake_time;

  wake_time.done = 1;
  wake_time.wake_time =
    (((sctk_timer_t) useconds / (sctk_timer_t) 1000) /
     (sctk_timer_t) sctk_time_interval) + sctk_timer + 1;
  sctk_thread_yield ();

  __sctk_ptr_thread_testcancel ();
  sctk_thread_wait_for_value_and_poll (&(wake_time.done), 0,
				       (void (*)(void *))
				       sctk_thread_sleep_pool,
				       (void *) &wake_time);
  __sctk_ptr_thread_testcancel ();
  return 0;
}

/*on ne prend pas en compte la precision en dessous de la micro-seconde
on ne gere pas les interruptions non plus*/
int
sctk_thread_nanosleep (const struct timespec *req, struct timespec *rem)
{
  if (req == NULL)
    return SCTK_EINVAL;
  if ((req->tv_sec < 0) || (req->tv_nsec < 0) || (req->tv_nsec > 999999999)){
	errno = SCTK_EINVAL;
    return -1;
  }
  sctk_thread_sleep (req->tv_sec);
  sctk_thread_usleep (req->tv_nsec / 1000);
  if (rem != NULL)
    {
      rem->tv_sec = 0;
      rem->tv_nsec = 0;
    }
  return 0;
}

double
sctk_thread_get_activity (int i)
{
  double tmp;
  tmp = __sctk_ptr_thread_get_activity (i);
  sctk_nodebug ("sctk_thread_get_activity %d %f", i, tmp);
  return tmp;
}

int
sctk_thread_dump (char *file)
{
  return __sctk_ptr_thread_dump (file);
}

int
sctk_thread_migrate (void)
{
  return __sctk_ptr_thread_migrate ();
}

int
sctk_thread_restore (sctk_thread_t thread, char *type, int vp)
{
  return __sctk_ptr_thread_restore (thread, type, vp);
}

int
sctk_thread_dump_clean (void)
{
  return __sctk_ptr_thread_dump_clean ();
}

void
sctk_thread_wait_for_value_and_poll (volatile int *data, int value,
				     void (*func) (void *), void *arg)
{
  __sctk_ptr_thread_wait_for_value_and_poll (data, value, func, arg);
}

int
sctk_thread_timed_wait_for_value (volatile int *data, int value, unsigned int max_time_in_usec)
{
	unsigned int end_time = (((sctk_timer_t) max_time_in_usec / (sctk_timer_t) 1000) /
     (sctk_timer_t) sctk_time_interval) + sctk_timer + 1;
	
	unsigned int trials = 0;
	
	while( *data != value )
	{
		if( end_time < sctk_timer )
		{
			/* TIMED OUT */
			return 1;
		}
		
		int left_to_wait = end_time - sctk_timer;
		
		if( (30 < trials) && ( 5000 < left_to_wait ) )
		{
			sctk_thread_usleep( 1000 );
		}
		else
		{
			sctk_thread_yield();
		}

		trials++;
	}
	
	return 0;
}

void extls_wait_for_value(volatile int *data, int value)
{
  __sctk_ptr_thread_wait_for_value_and_poll (data, value, NULL, NULL);
}

void
sctk_thread_wait_for_value (volatile int *data, int value)
{
  __sctk_ptr_thread_wait_for_value_and_poll (data, value, NULL, NULL);
}


void
sctk_thread_freeze_thread_on_vp (sctk_thread_mutex_t * lock, void **list)
{
  __sctk_ptr_thread_freeze_thread_on_vp (lock, list);
}

void
sctk_thread_wake_thread_on_vp (void **list)
{
  __sctk_ptr_thread_wake_thread_on_vp (list);
}

int
sctk_thread_getattr_np (sctk_thread_t th, sctk_thread_attr_t * attr)
{
  return __sctk_ptr_thread_getattr_np (th, attr);
}


sctk_thread_key_t stck_task_data;

void
sctk_thread_data_init ()
{
  sctk_thread_key_create (&stck_task_data, NULL);
  sctk_nodebug ("stck_task_data = %d", stck_task_data);
  sctk_thread_data_set (&sctk_main_datas);
}

static void sctk_thread_data_reset() {
  sctk_thread_data_t *tmp;
  tmp = (sctk_thread_data_t *)sctk_thread_getspecific(stck_task_data);
  if (tmp == NULL)
    sctk_thread_data_set(&sctk_main_datas);
}

void sctk_thread_data_set(sctk_thread_data_t *task_id) {
  sctk_thread_setspecific (stck_task_data, (void *) task_id);
}

sctk_thread_data_t *
sctk_thread_data_get ()
{
  sctk_thread_data_t *tmp;
  if (sctk_multithreading_initialised == 0)
    tmp = &sctk_main_datas;
  else
    tmp = (sctk_thread_data_t *) sctk_thread_getspecific (stck_task_data);

  return tmp;
}

#if 0
static void
sctk_net_poll (void *arg)
{
#ifdef MPC_Message_Passing
/*   static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER; */
/*   if(sctk_thread_mutex_trylock(&lock) == 0){ */
  if (sctk_net_adm_poll != NULL)
    sctk_net_adm_poll (arg);
  if (sctk_net_ptp_poll != NULL)
    sctk_net_ptp_poll (arg);
/*     sctk_thread_mutex_unlock(&(lock)); */
/*   } */
#endif
}
#endif


volatile sctk_timer_t sctk_timer = 0;

volatile int sctk_thread_running = 1;

static void *
sctk_thread_timer (void *arg)
{
  sctk_ethread_mxn_init_kethread ();
  assume (arg == NULL);
  while (sctk_thread_running)
    {
      kthread_usleep (sctk_time_interval * 1000);
      sctk_timer++;
    }
  return NULL;
}

#ifdef MPC_MPI
static void *
sctk_thread_migration (void *arg)
{
  sctk_ethread_mxn_init_kethread ();
  assume (arg == NULL);
  while (sctk_thread_running)
    {
      kthread_usleep (100 * 1000);
      sctk_net_migration_check();
    }
  return NULL;
}
#endif


static int sctk_first_local = 0;
static int sctk_last_local = 0;

extern double __sctk_profiling__start__sctk_init_MPC;
extern double __sctk_profiling__end__sctk_init_MPC;

double sctk_profiling_get_init_time() {
  return (__sctk_profiling__end__sctk_init_MPC-__sctk_profiling__start__sctk_init_MPC)/1000000;
}

// default algorithme
int sctk_get_init_vp_and_nbvp_default(int i, int *nbVp) {

  /*   int rank_in_node; */
  int slot_size;
  int task_nb;
  int proc;
  int first;
  int last;
  int cpu_nb;
  int total_tasks_number;
  int cpu_per_task;
  int j;

  cpu_nb = sctk_get_cpu_number(); // number of cpu per process

  total_tasks_number = sctk_get_total_tasks_number();

/*   rank_in_node = i - sctk_first_local; */

/*   sctk_nodebug("check for %d(%d) %d cpu",i,rank_in_node,cpu_nb); */

  assume ((sctk_last_local != 0) || (total_tasks_number == 1)
	  || (sctk_process_rank == 0));

  task_nb = sctk_last_local - sctk_first_local + 1;

  slot_size = task_nb / cpu_nb;
  cpu_per_task = cpu_nb / task_nb;

  if(cpu_per_task < 1){
    cpu_per_task = 1;
  }

  /* TODO: cpu_per_task=1 => put MPI tasks close */
  // cpu_per_task = 1 ;

  /* Normalize i if i the the global number insted of localnumber*/
  // hmt
  if(i >= task_nb) i = i - sctk_first_local;
  sctk_nodebug("i normalized=%d", i);

  first = 0;
  sctk_nodebug("cpu_per_task %d", cpu_per_task);
  j = 0;
  for (proc = 0; proc < cpu_nb; proc += cpu_per_task) {
    int local_slot_size;
    local_slot_size = slot_size;
    sctk_nodebug("local_slot_size=%d", local_slot_size);
    if ((task_nb % cpu_nb) > j) {
      local_slot_size++;
      sctk_nodebug("local_slot_size inside the if=%d", local_slot_size);
    }

    sctk_nodebug("%d proc %d slot size", proc, local_slot_size);
    last = first + local_slot_size - 1;
    sctk_nodebug("First %d last %d", first, last);
    if ((i >= first) && (i <= last)) {
      if ((cpu_nb % task_nb > j) && (cpu_nb > task_nb)) {
        *nbVp = cpu_per_task + 1;
      } else {
        *nbVp = cpu_per_task;
      }
      sctk_nodebug("sctk_get_init_vp: Put task %d on VP %d", i, proc);
      return proc;
    }
    first = last + 1;
    j++;
    if ((cpu_nb % task_nb >= j) && (cpu_nb > task_nb))
      proc++;
  }
  sctk_nodebug("sctk_get_init_vp: (After loop) Put task %d on VP %d", i, proc);
  fflush(stdout);
  sctk_abort();
  return proc;
}

int sctk_get_init_vp_and_nbvp_numa_packed(int i, int *nbVp) {

  /*   int rank_in_node; */
  // declaration
  int slot_size;
  int task_nb;
  int proc;
  int first;
  int last;
  int cpu_nb;
  int total_tasks_number;
  int cpu_per_task;
  int j;

  int nb_numa_node_per_node;
  int nb_cpu_per_numa_node;
  int nb_task_per_numa_node;

  // initialization
  cpu_nb = sctk_get_cpu_number(); // number of cpu per process
  total_tasks_number = sctk_get_total_tasks_number();

  nb_numa_node_per_node =
      sctk_get_numa_node_number(); // number of numa nodes in the node
  nb_cpu_per_numa_node =
      cpu_nb / nb_numa_node_per_node; // number of cores per numa nodes

  // config
  int T = sctk_last_local - sctk_first_local + 1; // number of task per node
  int C = cpu_nb;                                 // number of cpu per node
  int N = nb_numa_node_per_node;                  // number of numa nodes
  int t = T % N;                                  // buffer task
  int cpu_per_numa_node = C / N;                  // cpu per numa node

  int Tnk; // task per numa node

  // task already treated
  int tat = 0;

  *nbVp = 1;

  int k; // numa_node ID
  for (k = 0; k < N; k++) {

    // printf("for k=%d\n",k);
    // On ajoute une tache en plus aux premiers noeuds numas
    Tnk = T / N;
    if (k < t) {
      Tnk += 1;
      // printf("if [k=%d] < [t=%d] -> Tnk+=1 %d\n", k, t, Tnk);
    }

    // Si  tat <= i < Tnk+tat // est ce qu on est dans ce noeud numa la ?
    if (tat <= i && i < (Tnk + tat)) {
      // printf("if [tat=%d] <= [i=%d] < [Tnk=%d+tat=%d] %d\n", tat, i, Tnk,
      // tat, Tnk+tat);

      // update *nbVP
      if (k * (cpu_per_numa_node) + (i - tat) ==
          k * (cpu_per_numa_node) + Tnk - 1)
        *nbVp = (cpu_per_numa_node - Tnk) + 1;

      // printf("proc=%d, *nbVp=%d\n", k*(cpu_per_numa_node) + (i-tat),*nbVp);

      // return numanode id * (C/N) + (i - tat)
      //      fixe premier cpu du noeud numa     + difference de la tache en
      //      cours
      return k * (cpu_per_numa_node) + (i - tat);
    }
    // Si nous avons une tache en plus
    // printf("else tat=%d += Tnk=%d -> %d\n", tat, Tnk, tat+Tnk);
    tat += Tnk;
  }

  sctk_debug("sctk_get_init_vp: (After loop) Put task %d on VP %d", i, proc);
  sctk_abort ();
  return proc;
}

int sctk_get_init_vp_and_nbvp_numa(int i, int *nbVp) {
  int task_nb =
      sctk_last_local - sctk_first_local + 1; // number of task per process
  int cpu_nb = sctk_get_cpu_number();         // number of cpu per process
  int numa_node_per_node_nb = sctk_get_numa_node_number();
  int cpu_per_numa_node = cpu_nb / numa_node_per_node_nb;

  int global_id = i;
  int numa_node_id = (global_id * numa_node_per_node_nb) / task_nb;
  int local_id =
      global_id - (((numa_node_id * task_nb) + numa_node_per_node_nb - 1) /
                   numa_node_per_node_nb);

  int task_per_numa_node =
      (((numa_node_id + 1) * task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb) -
      (((numa_node_id)*task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb);

  int proc_local = (local_id * cpu_per_numa_node) / task_per_numa_node;
  int proc_global = proc_local + (numa_node_id * cpu_per_numa_node);

  // calculate nbVp value
  int proc_next_global;
  int proc_next_local;

  if (local_id < task_per_numa_node) {
    proc_next_global =
        ((local_id + 1) * cpu_per_numa_node) / task_per_numa_node;
    proc_next_local = proc_next_global + numa_node_id * cpu_per_numa_node;
    *nbVp = proc_next_local - proc_global;
  } else {
    *nbVp = cpu_per_numa_node - proc_local;
  }
  if (*nbVp < 1)
    *nbVp = 1;

  // DEBUG
  // char hostname[1024];
  // gethostname(hostname,1024);
  // FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
  // fprintf(hostname_fd,"INIT        task_nb %d cpu_per_node %d
  // numa_node_per_node_nb %d numa_node_id %d task_per_numa_node %d local_id %d
  // global_id %d proc_global %d current_cpu %d sctk_get_local_task_number %d
  // nbVp %d\n",
  //        task_nb,
  //        cpu_nb ,
  //        numa_node_per_node_nb,
  //        numa_node_id ,
  //        task_per_numa_node,
  //        local_id  ,
  //        global_id ,
  //        proc_global,
  //        sctk_get_cpu(),
  //        sctk_get_local_task_number(),
  //        *nbVp
  //       );
  // fflush(hostname_fd);
  // fclose(hostname_fd);
  // ENDDEBUG

  return proc_global;
}

int sctk_get_init_vp_and_nbvp(int i, int *nbVp) {
  // return sctk_get_init_vp_and_nbvp_default (i, nbVp);
  // return sctk_get_init_vp_and_nbvp_numa (i, nbVp);
  // return sctk_get_init_vp_and_nbvp_numa_packed (i, nbVp);

  // function pointer to get the thread placement policy from the config
  int (*thread_placement_policy)(int i, int *nbVp);
  thread_placement_policy = (int (*)(int, int *))sctk_runtime_config_get()
                                ->modules.thread.placement_policy.value;
  return thread_placement_policy(i, nbVp);
}

int
sctk_get_init_vp (int i)
{
     int dummy;
     return sctk_get_init_vp_and_nbvp (i, &dummy);
}


static struct _sctk_thread_cleanup_buffer
  *ptr_cleanup_sctk_thread_init_no_mpc;
void
sctk_thread_init_no_mpc (void)
{
  sctk_thread_key_create (&_sctk_thread_handler_key, (void (*)(void *)) NULL);
  ptr_cleanup_sctk_thread_init_no_mpc = NULL;
  sctk_thread_setspecific (_sctk_thread_handler_key,
			   &ptr_cleanup_sctk_thread_init_no_mpc);
}


#if defined(Linux_SYS)
typedef  unsigned long poff_t;

static poff_t dataused(void) {
  poff_t mem_used = 0;
  FILE* f = fopen("/proc/self/stat","r");
  if (f){
    // See proc(1), category 'stat'
    int z_pid;
    char z_comm[4096];
    char z_state;
    int z_ppid;
    int z_pgrp;
    int z_session;
    int z_tty_nr;
    int z_tpgid;
    unsigned long z_flags;
    unsigned long z_minflt;
    unsigned long z_cminflt;
    unsigned long z_majflt;
    unsigned long z_cmajflt;
    unsigned long z_utime;
    unsigned long z_stime;
    long z_cutime;
    long z_cstime;
    long z_priority;
    long z_nice;
    long z_zero;
    long z_itrealvalue;
    long z_starttime;
    unsigned long z_vsize;
    long z_rss;
    assume(fscanf(f,"%d %s %c %d %d %d %d %d",
		  &z_pid,z_comm,&z_state,&z_ppid,&z_pgrp,&z_session,&z_tty_nr,&z_tpgid) == 8);

    assume(fscanf(f,"%lu %lu %lu %lu %lu %lu %lu",
		  &z_flags,&z_minflt,&z_cminflt,&z_majflt,&z_cmajflt,&z_utime,&z_stime) == 7);

    assume(fscanf(f,"%ld %ld %ld %ld %ld %ld %ld %lu %ld",
		  &z_cutime,&z_cstime,&z_priority,&z_nice,&z_zero,&z_itrealvalue,&z_starttime,&z_vsize,&z_rss) == 9);
    int pz =  getpagesize();

    mem_used = (poff_t)(z_rss * pz);
    fclose(f);

  }
  return mem_used ;
}
#else
poff_t dataused(void) {
  return 0;
}
#endif

double sctk_profiling_get_dataused() {
  return (double)dataused();
}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#pragma weak MPC_Process_hook
void  MPC_Process_hook()
{
    /*This function is used to intercept MPC process's creation when profiling*/
}
#endif

static int sctk_ignore_sigpipe()
{
    struct sigaction act ;

    if (sigaction(SIGPIPE, (struct sigaction *)NULL, &act) == -1)
        return -1 ;

    if (act.sa_handler == SIG_DFL) {
        act.sa_handler = SIG_IGN ;
        if (sigaction(SIGPIPE, &act, (struct sigaction *)NULL) == -1)
            return -1 ;
    }
    return 0 ;
}
void
sctk_start_func (void *(*run) (void *), void *arg)
{
	int i, cnt;
	int total_tasks_number;
	int local_threads;
	int start_thread;
	kthread_t timer_thread;

#ifdef MPC_MPI
	sctk_thread_t migration_thread;
	sctk_thread_attr_t migration_thread_attr;
#endif
	FILE *file;
	struct _sctk_thread_cleanup_buffer *ptr_cleanup;
	char name[SCTK_MAX_FILENAME_SIZE];
	sctk_thread_t *threads = NULL;
	int thread_to_join = 0;

#ifdef MPC_MPI
	sctk_datatype_init();
#endif

	sctk_thread_key_create (&_sctk_thread_handler_key, (void (*)(void *)) NULL);
	ptr_cleanup = NULL;
	sctk_thread_setspecific (_sctk_thread_handler_key, &ptr_cleanup);

	kthread_create (&timer_thread, sctk_thread_timer, NULL);

#ifdef MPC_MPI
	sctk_thread_attr_init (&migration_thread_attr);
	sctk_thread_attr_setscope (&migration_thread_attr, SCTK_THREAD_SCOPE_SYSTEM);
	sctk_user_thread_create (&migration_thread, &migration_thread_attr, sctk_thread_migration, NULL);
	sctk_mpc_init_keys ();
#endif

  /* Fill the profiling parent key array */
#ifdef MPC_Profiler
  sctk_profiler_array_init_parent_keys();
#endif

	total_tasks_number = sctk_get_total_tasks_number();

/* 	sprintf (name, "%s/Process_%d", sctk_store_dir, sctk_process_rank); */

	/*Prepare free pages */
	{
		void *p1, *p2, *p3, *p4;
		p1 = sctk_malloc (1024 * 16);
		p2 = sctk_malloc (1024 * 16);
		p3 = sctk_malloc (1024 * 16);
		p4 = sctk_malloc (1024 * 16);
		sctk_free (p1);
		sctk_free (p2);
		sctk_free (p3);
		sctk_free (p4);
	}

	/*Init brk for tasks */
	{
		size_t s;
		void *tmp;
		size_t start = 0;
		size_t size;
		sctk_enter_no_alloc_land ();
		size = (size_t) (SCTK_MAX_MEMORY_SIZE);

		tmp = sctk_get_heap_start ();
		sctk_nodebug ("INIT ADRESS %p", tmp);
		s = (size_t) tmp /*  + 1*1024*1024*1024 */ ;

		sctk_nodebug ("Max allocation %luMo %lu",
			  (unsigned long) (size /
					   (1024 * 1024 * sctk_process_number)), s);

		start = s;
		start = start / SCTK_MAX_MEMORY_OFFSET;
		start = start * SCTK_MAX_MEMORY_OFFSET;
		if (s > start)
		  start += SCTK_MAX_MEMORY_OFFSET;

		tmp = (void *) start;
		sctk_nodebug ("INIT ADRESS REALIGNED %p", tmp);

		if (sctk_process_number > 1)
		  sctk_mem_reset_heap ((size_t) tmp, size);

		sctk_nodebug ("Heap start at %p (%p %p)", sctk_get_heap_start (),
			  (void *) s, tmp);
	}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
	MPC_Process_hook();
#endif

	if (sctk_restart_mode == 0)
	{
		sctk_leave_no_alloc_land ();
		if((getenv("SCTK_MIC_NB_TASK") != NULL) || 
		   (getenv("SCTK_HOST_NB_TASK") != NULL) ||
		   (getenv("SCTK_NB_HOST") != NULL) ||
		   (getenv("SCTK_NB_MIC") != NULL))
		{
			int node_rank,process_on_node_rank,process_on_node_number;
			int host_number, mic_nb_task, host_nb_task;

			host_number = (getenv("SCTK_NB_HOST") != NULL) ? atoi(getenv("SCTK_NB_HOST")) : 0;
			mic_nb_task = (getenv("SCTK_MIC_NB_TASK") != NULL) ? atoi(getenv("SCTK_MIC_NB_TASK")) : 0;
			host_nb_task = (getenv("SCTK_HOST_NB_TASK") != NULL) ? atoi(getenv("SCTK_HOST_NB_TASK")) : 0;
			
#ifdef MPC_Message_Passing
			sctk_pmi_get_node_rank(&node_rank);
			sctk_pmi_get_process_on_node_rank(&process_on_node_rank);
			sctk_pmi_get_process_on_node_number(&process_on_node_number);
#endif
			
			#if __MIC__
				local_threads = mic_nb_task/process_on_node_number;
				if ((mic_nb_task % process_on_node_number) > process_on_node_rank)
				{
					local_threads++;
					start_thread = (host_nb_task * host_number) + ((node_rank - host_number) * mic_nb_task) + (local_threads*process_on_node_rank);
				}
				else
				{
					int i=0;
					start_thread = (host_nb_task * host_number) + ((node_rank - host_number) * mic_nb_task);
					for(i=0 ; i<process_on_node_rank ; i++)
					{
						if((mic_nb_task % process_on_node_number) > i)
						start_thread += (local_threads+1);
						else
						start_thread += local_threads;
					}
				}
			#else
				local_threads = host_nb_task/process_on_node_number;
				if ((host_nb_task % process_on_node_number) > process_on_node_rank)
				{
					local_threads++;
					start_thread = (node_rank * host_nb_task) + (local_threads*process_on_node_rank);
				}
				else
				{
					int i=0;
					start_thread = (node_rank * host_nb_task);
					for(i=0 ; i<process_on_node_rank ; i++)
					{
						if((host_nb_task % process_on_node_number) > i)
						start_thread += (local_threads+1);
						else
						start_thread += local_threads;
					}
				}
			#endif
			sctk_debug("lt %d, Process %d %d-%d", local_threads, sctk_process_rank, start_thread, start_thread + local_threads - 1);
		}
		else
		{
			local_threads = total_tasks_number / sctk_process_number;
			if (total_tasks_number % sctk_process_number > sctk_process_rank)
				local_threads++;

			start_thread = local_threads * sctk_process_rank;
			if (total_tasks_number % sctk_process_number <= sctk_process_rank)
				start_thread += total_tasks_number % sctk_process_number;
		}

/* 		if (sctk_check_point_restart_mode) */
/* 		{ */
/* 			file = fopen (name, "w"); */
/* 			fprintf (file, "Task %d->%d\n", start_thread, */
/* 						   start_thread + local_threads - 1); */
/* 			fprintf (file, "%lu\n", sctk_get_heap_size ()); */
/* 			fclose (file); */
/* 		} */

		sctk_first_local = start_thread;
		sctk_last_local = start_thread + local_threads - 1;
		threads =
		(sctk_thread_t *) sctk_malloc (local_threads * sizeof (sctk_thread_t));
		sctk_current_local_tasks_nb = local_threads;

		cnt = 0;

		for (i = start_thread; i < start_thread + local_threads; i++)
		{
			sctk_nodebug ("Thread %d try create", i);
			sctk_thread_create (&(threads[thread_to_join]), NULL, run, arg, (long) i, cnt++);
			sctk_nodebug ("Thread %d created", i);
			thread_to_join++;
		}

		sctk_nodebug ("All thread created");
	}
	else
	{
#if 1
	  not_implemented();
#else
		FILE *last;
		unsigned long step;
		char task_name[SCTK_MAX_FILENAME_SIZE];
		int error = 0;

		sprintf (task_name, "%s/last_point", sctk_store_dir);
		last = fopen (task_name, "r");
		if (last != NULL)
		{
			assume(fscanf (last, "%lu\n", &step) == 1);
			fclose (last);
		}
		else
		{
			step = 0;
		}
		if(step == 0)
		{
			sctk_error ("%s corrupted. Repair it manually!", task_name);
			abort ();
		}

		  sctk_nodebug ("Total task number = %d", total_tasks_number);

		check:
		sctk_current_local_tasks_nb = 0;

		for (i = 0; i < total_tasks_number; i++)
		{
			char file_name[SCTK_MAX_FILENAME_SIZE];
			int restart = 0;
			int proc = 0;
			void *self_p = NULL;
			int vp = 0;
			sprintf (task_name, "%s/Task_%d", sctk_store_dir, i);
			file = fopen (task_name, "r");
			if (file == NULL)
			{
			  sctk_error ("Unable to restore: file %s missing", task_name);
			  abort ();
			}
			assume (file != NULL);
			assume(fscanf (file, "Restart %d\n", &restart) == 1);
			assume(fscanf (file, "Process %d\n", &proc) == 1);
			assume(fscanf (file, "Thread %p\n", &self_p) == 1);
			assume(fscanf (file, "Virtual processor %d\n", &vp) == 1);
			fclose (file);
			sprintf (file_name, "%s/task_%p_%lu", sctk_store_dir, self_p, step);
			if (sctk_check_file (file_name) != 0)
			{
			  if (0 == sctk_process_rank)
			{
			  fprintf (stderr, "File %s corrupted!!!!\n", file_name);
			}
			  error = 1;
			  break;
			}
			error = 0;
		}

		if(error != 0)
		{
			assume (step > 0);
			step--;
			error = 0;
			goto check;
		}

		if(0 == sctk_process_rank)
		{
			fprintf (stderr, "Restart from checkpoint %lu\n", step);
		}

		/*Update memory */
		if (0 == sctk_process_rank)
		{
			fprintf (stderr, "Restore dynamic allocations ");
		}
		for (i = 0; i < total_tasks_number; i++)
		{
			int restart = 0;
			int proc = 0;
			void *self_p = NULL;
			int vp = 0;

			sprintf (task_name, "%s/Task_%d", sctk_store_dir, i);
			file = fopen (task_name, "r");

			assume (file != NULL);
			assume(fscanf (file, "Restart %d\n", &restart) == 1);
			assume(fscanf (file, "Process %d\n", &proc) == 1);
			assume(fscanf (file, "Thread %p\n", &self_p) == 1);
			assume(fscanf (file, "Virtual processor %d\n", &vp) == 1);

			fclose (file);
			sctk_nodebug ("Task %d is in %d with %p on vp %d", i, proc,
			self_p, vp);
			/*update brk dues to migrations */
			sprintf (task_name, "%s/task_%p_%lu", sctk_store_dir, self_p, step);
			__sctk_update_memory (task_name);

			if (0 == sctk_process_rank)
			{
				fprintf (stderr, ".");
			}
		}

		if (0 == sctk_process_rank)
		{
			fprintf (stderr, " done\n");
		}

		sctk_leave_no_alloc_land ();

		if (0 == sctk_process_rank)
		{
			fprintf (stderr, "Restore tasks ");
		}

		for (i = 0; i < total_tasks_number; i++)
		{
			int restart = 0;
			int proc = 0;
			void *self_p = NULL;
			int vp = 0;
			sprintf (task_name, "%s/Task_%d", sctk_store_dir, i);
			file = fopen (task_name, "r");
			assume (file != NULL);
			assume(fscanf (file, "Restart %d\n", &restart) == 1);
			assume(fscanf (file, "Process %d\n", &proc) == 1);
			assume(fscanf (file, "Thread %p\n", &self_p) == 1);
			assume(fscanf (file, "Virtual processor %d\n", &vp) == 1);
			fclose (file);
			sctk_nodebug ("Task %d is in %d with %p on vp %d", i, proc,
			self_p, vp);

#ifdef MPC_Message_Passing
			sctk_register_task(i);
			sctk_register_restart_thread (i, proc);
#endif

			if (proc == sctk_process_rank)
			{
				sctk_nodebug ("Restore task %d", i);
				sprintf (task_name, "%s/task_%p_%lu", sctk_store_dir,
				self_p, step);
				sctk_thread_restore (self_p, task_name, vp);
				sctk_nodebug ("Restore task %d done", i);
			}
			else
			{
				/*update brk dues to migrations */
				sprintf (task_name, "%s/task_%p_%lu", sctk_store_dir,
				self_p, step);
				__sctk_update_memory (task_name);
			}
			if (0 == sctk_process_rank)
			{
				fprintf (stderr, ".");
			}
		}

		if (0 == sctk_process_rank)
		{
			fprintf (stderr, " done\n");
		}
#endif
	}

	sctk_nodebug("sctk_current_local_tasks_nb %d",sctk_current_local_tasks_nb);
        start_func_done = 1;
        __sctk_profiling__end__sctk_init_MPC =
            sctk_get_time_stamp_gettimeofday();
        if (sctk_process_rank == 0) {
          if (sctk_runtime_config_get()->modules.launcher.banner) {
            fprintf(stderr,
                    "Initialization time: %.1fs - Memory used: %0.fMB\n",
                    sctk_profiling_get_init_time(),
                    sctk_profiling_get_dataused() / (1024.0 * 1024.0));
          }
        }

        sctk_thread_wait_for_value_and_poll((int *)&sctk_current_local_tasks_nb,
                                            0, NULL, NULL);

        sctk_multithreading_initialised = 0;

        sctk_thread_running = 0;

#ifdef MPC_MPI
        sctk_datatype_release();
#endif

#ifdef MPC_Message_Passing
        sctk_ignore_sigpipe();
        sctk_communicator_delete();
#endif

        sctk_free(threads);

        remove(name);

        sctk_runtime_config_clean_hash_tables();
}

void
sctk_kthread_wait_for_value_and_poll (int *data, int value,
				      void (*func) (void *), void *arg)
{
  while ((*data) != value)
    {
      if (func != NULL)
	{
	  func (arg);
	}
      kthread_usleep (500);
    }
}

int
sctk_is_restarted ()
{
  return (sctk_restart_mode != 0);
}

int
sctk_thread_proc_migration (const int cpu)
{
  int tmp;
  sctk_thread_data_t *tmp_data;
  tmp_data = sctk_thread_data_get ();
/*   sctk_assert(tmp_data != NULL); */
  tmp = __sctk_ptr_thread_proc_migration (cpu);
  tmp_data->virtual_processor = sctk_thread_get_vp ();
  return tmp;
}


void
_sctk_thread_cleanup_push (struct _sctk_thread_cleanup_buffer *__buffer,
			   void (*__routine) (void *), void *__arg)
{
  struct _sctk_thread_cleanup_buffer **__head;
  __buffer->__routine = __routine;
  __buffer->__arg = __arg;
  __head = sctk_thread_getspecific (_sctk_thread_handler_key);
  sctk_nodebug ("%p %p %p", __buffer, __head, *__head);
  __buffer->next = *__head;
  *__head = __buffer;
  sctk_nodebug ("%p %p %p", __buffer, __head, *__head);
}

void
_sctk_thread_cleanup_pop (struct _sctk_thread_cleanup_buffer *__buffer,
			  int __execute)
{
  struct _sctk_thread_cleanup_buffer **__head;
  __head = sctk_thread_getspecific (_sctk_thread_handler_key);
  *__head = __buffer->next;
  if (__execute)
    {
      __buffer->__routine (__buffer->__arg);
    }
}

void
sctk_get_thread_info (int *task_id, int *thread_id)
{
  sctk_thread_data_t *task_id_p;
  sctk_thread_data_t tmp = SCTK_THREAD_DATA_INIT;

  task_id_p = sctk_thread_data_get ();
  if (task_id_p != NULL)
    {
      tmp = *task_id_p;
    }
  *task_id = tmp.task_id;
  *thread_id = tmp.user_thread;
}

unsigned long
sctk_thread_atomic_add (volatile unsigned long *ptr, unsigned long val)
{
  unsigned long tmp;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_thread_mutex_lock (&lock);
  tmp = *ptr;
  *ptr = tmp + val;
  sctk_thread_mutex_unlock (&lock);
  return tmp;
}

/* Used by GCC to bypass TLS destructor calls */
int __cxa_thread_mpc_atexit(void(*dfunc)(void*), void* obj, void* dso_symbol)
{
	sctk_thread_data_t *th;

	th = sctk_thread_data_get();
	sctk_tls_dtors_add(&(th->dtors_head), obj, dfunc);
	return 0;
}

