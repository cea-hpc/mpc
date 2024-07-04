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
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* ######################################################################## */

#if defined(SCTK_USE_THREAD_DEBUG)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <mpc_arch.h>

#include "mpc_config.h"
#include "mpc_common_debug.h"
#include "sctk_thread_dbg.h"
#include "mpc_common_debug.h"
#include "mpc_thread.h"
#include "mpc_common_spinlock.h"
#include "sctk_context.h"
#include <thread_db.h>
#include <sys/syscall.h>
#include "tdb_remote.h"

/** ***************************************************************************** **/
void sctk_thread_add(sctk_thread_data_t *item, void *tid)
{
	item->tid    = tid;
	item->status = sctk_thread_running_status;
	/** *** */
	sctk_init_thread_debug(item);
	/** *** */
}

static inline int sctk_remove_thread(void *tid);

void sctk_thread_remove(sctk_thread_data_t *item)
{
	/** *** */
	sctk_remove_thread(item->tid);
	/** *** */
}

/** ***************************************************************************** **/
/** *********************   LIB_THREAD_DB   ************************************* **/
/** ***************************************************************************** **/
static int sctk_use_rtdb = 0;
static mpc_common_spinlock_t sctk_rtdb_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/* Translate a sctk status to a gdb status*/
/* static inline */
/* td_thr_state_e sctk_get_gdb_status (sctk_thread_status_t status) { */
/*   switch (status) { */
/*   case sctk_thread_running_status : return TD_THR_ACTIVE   ; */
/*   case mpc_thread_sleep_status   : return TD_THR_RUN; */
/*   case sctk_thread_blocked_status : return TD_THR_RUN; //return TD_THR_SLEEP ; */
/*   case sctk_thread_undef_status   : return TD_THR_ZOMBIE ; */
/*   case sctk_thread_check_status   : return TD_THR_ACTIVE ; */
/*   default : return TD_THR_UNKNOWN ; */
/*   } */
/* } */
#define sctk_get_gdb_status(status)    status

int sctk_init_thread_debug(sctk_thread_data_t *item)
{
	sctk_ethread_per_thread_t *  tid;
	volatile tdb_thread_debug_t *thread;

	if(!sctk_use_rtdb)
	{
		return 0;
	}

	tid = (sctk_ethread_per_thread_t *)item->tid;

	mpc_common_nodebug("enabled ? %d", sctk_use_rtdb);
	rtdb_add_thread(tid, &thread);
	rtdb_set_thread_startfunc(thread, item->__start_routine);
	rtdb_set_thread_stkbase(thread, tid->stack);
	rtdb_set_thread_stksize(thread, tid->stack_size);
	rtdb_set_thread_type(thread, TD_THR_USER);

#if defined MPC_USE_EXTLS
	rtdb_set_thread_extls(thread, tid->ctx.tls_ctx);
#endif

	mpc_common_nodebug("context");
#if defined(TDB_i686_ARCH_TDB) || defined(TDB_x86_64_ARCH_TDB)
#if SCTK_MCTX_MTH(mcsc)
	rtdb_set_thread_context(thread, tid->ctx.uc.uc_mcontext.gregs);
#elif SCTK_MCTX_MTH(sjlj)
	rtdb_set_thread_context(thread, tid->ctx.jb);
#elif SCTK_MCTX_MTH(libcontext)
	rtdb_set_thread_context(thread, tid->ctx.uc.uc_mcontext.gregs);
#else "Unknown context switch"
#error
#endif
#else
#error "Architecture not supported"
#endif
	tid->debug_p = (tdb_thread_debug_t *)thread;

	mpc_common_nodebug("refresh");

	/*initialiaze variable parts of the data*/
	rtdb_update_thread_this_lid(thread);

	return 0;
}

static inline int sctk_remove_thread(void *tid)
{
	if(!sctk_use_rtdb)
	{
		return 0;
	}

	rtdb_remove_thread_tid(tid);

	return 0;
}

/** ***************************************************************************** **/
void sctk_refresh_thread_debug(sctk_ethread_per_thread_t *tid, sctk_thread_status_t s)
{
	lwpid_t             lid;
	tdb_thread_debug_t *thread;

	if(!sctk_use_rtdb)
	{
		return;
	}
	thread = tid->debug_p;

	if(thread == NULL)
	{
		return;
	}

	/*   if (!sctk_use_rtdb) return ; */
	/*   if (s == sctk_thread_undef_status) return ; */

	/*   /\* the kernel threads are not managed bu rtdb *\/ */
	/*   if (thread == NULL) { */
	/*     mpc_common_nodebug("(NO THREAD/Thread %p NOT) IN THE LIST", tid); */
	/*     return ; */
	/*   } */

	rtdb_update_thread_state(thread, sctk_get_gdb_status(s) );
}

void sctk_refresh_thread_debug_migration(sctk_ethread_per_thread_t *tid)
{
	lwpid_t             lid;
	tdb_thread_debug_t *thread;

	if(!sctk_use_rtdb)
	{
		return;
	}
	thread = tid->debug_p;

	/*   if (!sctk_use_rtdb) return ; */

	/*   /\* the kernel threads are not managed bu rtdb *\/ */
	/*   if (thread == NULL) { */
	/*     mpc_common_nodebug("(NO THREAD/Thread %p NOT) IN THE LIST", tid); */
	/*     return ; */
	/*   } */

#if defined(TDB___linux__)
	lid = syscall(SYS_gettid);
#else
	lid = _lwp_self();
#endif

	rtdb_update_thread_lid(thread, lid);
}

/** ***************************************************************************** **/
/** ***************************************************************************** **/
/** ***************************************************************************** **/

int sctk_enable_lib_thread_db(void)
{
	rtdb_enable_lib_thread_db();
	sctk_use_rtdb = 1;

#if defined(MPC_I686_ARCH)
#if SCTK_MCTX_MTH(mcsc)
	rtdb_set_eip_offset(14);
	rtdb_set_esp_offset(7);
	rtdb_set_ebp_offset(6);
	rtdb_set_edi_offset(4);
	rtdb_set_ebx_offset(8);

#elif SCTK_MCTX_MTH(sjlj)
	rtdb_set_eip_offset(5);
	rtdb_set_esp_offset(4);
	rtdb_set_ebp_offset(3);
	rtdb_set_edi_offset(2);
	rtdb_set_ebx_offset(0);
#endif
#elif defined(MPC_X86_64_ARCH)
#if SCTK_MCTX_MTH(libcontext)
	rtdb_set_rbx_offset(SCTK_REG_RBX);
	rtdb_set_rbp_offset(SCTK_REG_RBP);
	rtdb_set_r12_offset(SCTK_REG_R12);
	rtdb_set_r13_offset(SCTK_REG_R13);
	rtdb_set_r14_offset(SCTK_REG_R14);
	rtdb_set_r15_offset(SCTK_REG_R15);
	rtdb_set_rsp_offset(SCTK_REG_RSP);
	rtdb_set_pc_offset(SCTK_REG_RIP);
#else
	rtdb_set_rbx_offset(0);
	rtdb_set_rbp_offset(1);
	rtdb_set_r12_offset(2);
	rtdb_set_r13_offset(3);
	rtdb_set_r14_offset(4);
	rtdb_set_r15_offset(5);
	rtdb_set_rsp_offset(6);
	rtdb_set_pc_offset(7);
#endif
#else
#error "Architecture not supported"
#endif

	rtdb_set_lock( (void *)&sctk_rtdb_lock,
	               (int (*)(void *) )mpc_common_spinlock_lock,
	               (int (*)(void *) )mpc_common_spinlock_unlock,
	               NULL);

	/* initialization of the main thread*/
	sctk_init_idle_thread_dbg(&sctk_ethread_main_thread, NULL);
	return 0;
}

/** ***************************************************************************** **/

int sctk_init_idle_thread_dbg(void *tid, void *start_fct)
{
	sctk_ethread_per_thread_t *  ttid;
	volatile tdb_thread_debug_t *thread;

	if(!sctk_use_rtdb)
	{
		return 0;
	}
	ttid = (sctk_ethread_per_thread_t *)tid;

	mpc_common_nodebug("-----------> IDLE TASK : %p", tid);

	rtdb_add_thread(ttid, &thread);
	rtdb_set_thread_stkbase(thread, ttid->stack);
	rtdb_set_thread_stksize(thread, ttid->stack_size);
	rtdb_set_thread_type(thread, TD_THR_SYSTEM);
	rtdb_log("start fct : %p", start_fct);
	rtdb_set_thread_startfunc(thread, start_fct);

#if defined(MPC_I686_ARCH) || defined(MPC_X86_64_ARCH)
#if SCTK_MCTX_MTH(mcsc)
	rtdb_set_thread_context(thread, ttid->ctx.uc.uc_mcontext.gregs);
#elif SCTK_MCTX_MTH(sjlj)
	rtdb_set_thread_context(thread, ttid->ctx.jb);
#elif SCTK_MCTX_MTH(libcontext)
	rtdb_set_thread_context(thread, ttid->ctx.uc.uc_mcontext.gregs);
#else
#error
#endif
#else
#error "Architecture not supported"
#endif

	ttid->debug_p = (tdb_thread_debug_t *)thread;
	mpc_common_nodebug("refresh idle");
	/*initialiaze variable parts of the data*/
	rtdb_update_thread_this_lid(thread);

	mpc_common_nodebug("creation of the idle task");
	rtdb_report_creation_event(thread);

	return 0;
}

int sctk_free_idle_thread_dbg(void *tid)
{
	if(!sctk_use_rtdb)
	{
		return 0;
	}

	rtdb_report_death_event_tid(tid);

	return 0;
}

int sctk_report_creation(void *tid)
{
	if(!sctk_use_rtdb)
	{
		return 0;
	}
	mpc_common_nodebug("creation of a task");
	rtdb_report_creation_event_tid(tid);

	return 0;
}

int sctk_report_death(void *tid)
{
	if(!sctk_use_rtdb)
	{
		return 0;
	}

	rtdb_report_death_event_tid(tid);

	return 0;
}

#endif /* SCTK_USE_THREAD_DEBUG */
