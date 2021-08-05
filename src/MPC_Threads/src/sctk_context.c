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
#include <errno.h>
#include <string.h>
#include "mpc_common_debug.h"

#include "thread.h"
#include "sctk_context.h"
#include "mpc_common_spinlock.h"

#include "tls.h"
#include "sctk_alloc.h"

typedef struct
{
	void (*func)(void *);
	unsigned long offset;
} sctk_tls_entry_t;

#if defined(TLS_SUPPORT)

#if defined(MPC_USE_CUDA)
__thread void *sctk_cuda_ctx;
#endif

__thread char *mpc_user_tls_1 = NULL;

#if defined(MPC_OpenMP)
/* MPC OpenMP TLS */
__thread void *mpc_omp_tls;
#endif

#if defined(MPC_MPI)
struct mpc_mpi_cl_per_thread_ctx_s;
__thread struct mpc_mpi_cl_per_thread_ctx_s *___mpc_p_per_thread_comm_ctx;
#endif


unsigned long     mpc_user_tls_1_offset           = 0;
unsigned long     mpc_user_tls_1_entry_number     = 0;
unsigned long     mpc_user_tls_1_entry_number_max = 0;
sctk_tls_entry_t *mpc_user_tls_1_tab = NULL;

unsigned long sctk_tls_entry_add(unsigned long size,
                                 void (*func)(void *) )
{
	sctk_tls_entry_t *entry;

	size += 32;

	if(size % 32 != 0)
	{
		size += 32 - (size % 32);
	}

	assume(sctk_multithreading_initialised == 0);
	assume(size % 32 == 0);
	if(mpc_user_tls_1_entry_number >= mpc_user_tls_1_entry_number_max)
	{
		int           i;
		unsigned long old_size;
		old_size = mpc_user_tls_1_entry_number_max;
		mpc_user_tls_1_entry_number_max += 100;
		mpc_user_tls_1_tab =
		        sctk_realloc(mpc_user_tls_1_tab, mpc_user_tls_1_entry_number_max *
		                     sizeof(sctk_tls_entry_t) );
		for(i = 0; i < 100; i++)
		{
			mpc_user_tls_1_tab[old_size + i].func   = NULL;
			mpc_user_tls_1_tab[old_size + i].offset = 0;
		}
	}

	entry = mpc_user_tls_1_tab + mpc_user_tls_1_entry_number;

	entry->func   = func;
	entry->offset = mpc_user_tls_1_offset + 32;

	mpc_user_tls_1_offset += size;
#if 1
	{
		char *tmp;
		tmp = sctk_malloc(mpc_user_tls_1_offset);
		memset(tmp, 0, mpc_user_tls_1_offset);
		if(mpc_user_tls_1 != NULL)
		{
			memcpy(tmp, mpc_user_tls_1, mpc_user_tls_1_offset - size);
			free(mpc_user_tls_1);
		}
		mpc_user_tls_1 = tmp;
	}
#else
	mpc_user_tls_1 = realloc(mpc_user_tls_1, mpc_user_tls_1_offset);
#endif
	mpc_user_tls_1_entry_number++;

	mpc_common_nodebug("%p %p %p", entry->offset, mpc_user_tls_1,
	             mpc_user_tls_1_offset);
	return entry->offset;
}

/*Init specific key*/
void sctk_tls_init_key(unsigned long key,
                       void (*func)(void *) )
{
	int *done;

	done  = (int *)(mpc_user_tls_1 + key - 32);
	*done = 1;
	func(mpc_user_tls_1 + key);
}

void sctk_tls_init()
{
	assume(sctk_multithreading_initialised == 1);

	mpc_user_tls_1 = sctk_malloc(mpc_user_tls_1_offset);
	memset(mpc_user_tls_1, 0, mpc_user_tls_1_offset);
}

#else

void sctk_tls_init()
{
}

unsigned long sctk_tls_entry_add(unsigned long size,
                                 void (*func)(void *) )
{
	not_available();
	return 0;
}
#endif

#if SCTK_MCTX_MTH(mcsc)
static inline int sctk_mctx_save(sctk_mctx_t *mctx)
{
	(mctx)->error    = errno;
	(mctx)->restored = 0;
	getcontext(&( (mctx)->uc) );
	return (int)(mctx)->restored;
}

#elif SCTK_MCTX_MTH(libcontext)
static inline int sctk_mctx_save(sctk_mctx_t *mctx)
{
	(mctx)->error    = errno;
	(mctx)->restored = 0;
	mpc__getcontext(&( (mctx)->uc) );
	return (int)(mctx)->restored;
}

#elif SCTK_MCTX_MTH(sjlj)
static inline int sctk_mctx_save(sctk_mctx_t *mctx)
{
	(mctx)->error    = errno;
	(mctx)->restored = 0;
	sctk_setjmp( ( (mctx)->jb) );
	return (int)(mctx)->restored;
}

#elif SCTK_MCTX_MTH(windows)
static inline int sctk_mctx_save(sctk_mctx_t *mctx)
{
	(mctx)->error    = errno;
	(mctx)->restored = 0;
	sctk_setjmp( ( (mctx)->jb) );
	return (int)(mctx)->restored;
}

#else
#error "unknown mctx method"
#endif

#if SCTK_MCTX_MTH(mcsc)
static inline int sctk_mctx_restore(sctk_mctx_t *mctx)
{
	errno            = (mctx)->error;
	(mctx)->restored = 1;
	setcontext(&( (mctx)->uc) );
	return (mctx)->restored;
}

#elif SCTK_MCTX_MTH(libcontext)
static inline int sctk_mctx_restore(sctk_mctx_t *mctx)
{
	errno            = (mctx)->error;
	(mctx)->restored = 1;
	mpc__setcontext(&( (mctx)->uc) );
	return (mctx)->restored;
}

#elif SCTK_MCTX_MTH(sjlj)
static inline int sctk_mctx_restore(sctk_mctx_t *mctx)
{
	errno            = (mctx)->error;
	(mctx)->restored = 1;
	sctk_longjmp( ( (mctx)->jb), 1);
	return (mctx)->restored;
}

#elif SCTK_MCTX_MTH(windows)
static inline int sctk_mctx_restore(sctk_mctx_t *mctx)
{
	errno            = (mctx)->error;
	(mctx)->restored = 1;
	sctk_longjmp( ( (mctx)->jb), 1);
	return (mctx)->restored;
}

#else
#error "unknown mctx method"
#endif

#if SCTK_MCTX_MTH(mcsc)
static inline int sctk_mctx_set(sctk_mctx_t *mctx,
                                void (*func)(void *),
                                char *sk_addr_lo,
                                char *sk_addr_hi,
                                void *arg)
{
	if(getcontext(&(mctx->uc) ) != 0)
	{
		return FALSE;
	}

	mctx->uc.uc_link = NULL;

	mctx->uc.uc_stack.ss_sp =
	        sctk_skaddr(makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	mctx->uc.uc_stack.ss_size =
	        sctk_sksize(makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	mctx->uc.uc_stack.ss_flags = 0;

	(mctx)->restored = 0;
	makecontext(&(mctx->uc), (void (*)(void) )func, 1 + 1, arg);

	return TRUE;
}

#elif SCTK_MCTX_MTH(libcontext)
static inline int sctk_mctx_set(sctk_mctx_t *mctx,
                                void (*func)(void *),
                                char *sk_addr_lo,
                                char *sk_addr_hi,
                                void *arg)
{
	if(mpc__getcontext(&(mctx->uc) ) != 0)
	{
		return FALSE;
	}

	mctx->uc.uc_link = NULL;

	mctx->uc.uc_stack.ss_sp =
	        sctk_skaddr(mpc__makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	mctx->uc.uc_stack.ss_size =
	        sctk_sksize(mpc__makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	mctx->uc.uc_stack.ss_flags = 0;

	(mctx)->restored = 0;
	mpc__makecontext(&(mctx->uc), (void (*)(void) )func, 1 + 1, arg);

	return TRUE;
}

#elif SCTK_MCTX_MTH(sjlj)
#ifdef SCTK_USE_CONTEXT_FOR_CREATION
#include <ucontext.h>

static inline void sctk_bootstrap_func(void (*func)(void *),
                                       void *arg,
                                       ucontext_t *root_uc,
                                       sctk_mctx_t *mctx)
{
	if(sctk_setjmp( (mctx->jb) ) == 0)
	{
		setcontext(root_uc);
	}
	else
	{
		func(arg);
	}
}

static inline int sctk_mctx_set(sctk_mctx_t *mctx,
                                void (*func)(void *),
                                char *sk_addr_lo,
                                char *sk_addr_hi,
                                void *arg)
{
	ucontext_t uc;
	ucontext_t root_uc;

	if(getcontext(&(uc) ) != 0)
	{
		return FALSE;
	}

	uc.uc_link = NULL;

	uc.uc_stack.ss_sp =
	        sctk_skaddr(makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	uc.uc_stack.ss_size =
	        sctk_sksize(makecontext, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	uc.uc_stack.ss_flags = 0;

	(mctx)->restored = 0;
	makecontext(&(uc), (void (*)(void) )sctk_bootstrap_func, 1 + 4, (void *)func, arg,
	            &root_uc, mctx);

	swapcontext(&(root_uc), &(uc) );
	return TRUE;
}

#else
static mpc_common_spinlock_t sjlj_spinlock = SCTK_SPINLOCK_INITIALIZER;
static jmp_buf mctx_trampoline;

static sctk_mctx_t           mctx_caller;
static volatile sig_atomic_t mctx_called;

static sctk_mctx_t *volatile mctx_creating;
static void(*volatile mctx_creating_func)(void *);
static volatile sigset_t mctx_creating_sigs;
static volatile void *   sctk_create_arg;

static void sctk_mctx_set_trampoline(int);
static void sctk_mctx_set_bootstrap(void);

/* initialize a machine state */
static inline int sctk_mctx_set(sctk_mctx_t *mctx,
                                void (*func)(void *),
                                char *sk_addr_lo,
                                char *sk_addr_hi,
                                void *arg)
{
	struct sigaction   sa;
	struct sigaction   osa;
	struct sigaltstack ss;
	struct sigaltstack oss;
	sigset_t           osigs;
	sigset_t           sigs;

	mpc_common_spinlock_lock(&sjlj_spinlock);

	sigemptyset(&sigs);
	sigaddset(&sigs, SIGUSR1);

	sa.sa_handler = sctk_mctx_set_trampoline;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK;

	if(sigaction(SIGUSR1, &sa, &osa) != 0)
	{
		mpc_common_debug_abort();
	}

	ss.ss_sp    = sctk_skaddr(sigaltstack, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	ss.ss_size  = sctk_sksize(sigaltstack, sk_addr_lo, sk_addr_hi - sk_addr_lo);
	ss.ss_flags = 0;
	if(sigaltstack(&ss, &oss) < 0)
	{
		mpc_common_debug_abort();
	}

	mpc_common_nodebug("Structures initalized %p ", mctx);

	mctx_called = FALSE;

	raise(SIGUSR1);

	sigfillset(&sigs);
	sigdelset(&sigs, SIGUSR1);

	mpc_common_nodebug("Main returned %d", mctx_called);

	sigaltstack(NULL, &ss);
	ss.ss_flags = SS_DISABLE;
	if(sigaltstack(&ss, NULL) < 0)
	{
		mpc_common_debug_abort();
	}
	sigaltstack(NULL, &ss);
	if(!(ss.ss_flags & SS_DISABLE) )
	{
		mpc_common_debug_abort();
	}
	if(!(oss.ss_flags & SS_DISABLE) )
	{
		sigaltstack(&oss, NULL);
	}

	sigaction(SIGUSR1, &osa, NULL);

	sigemptyset(&mctx->sigs);
	mctx->error = 0;

	sctk_create_arg    = arg;
	mctx_creating      = mctx;
	mctx_creating_func = func;
	memcpy( (void *)&mctx_creating_sigs, &osigs, sizeof(sigset_t) );

	mctx_caller.error    = errno;
	mctx_caller.restored = 0;

	mpc_common_nodebug("retore new thread %p ", &mctx_caller);

	if(sctk_setjmp(mctx_caller.jb) == 0)
	{
		sctk_longjmp(mctx_trampoline, 1);
	}

	sctk_create_arg    = NULL;
	mctx_creating      = NULL;
	mctx_creating_func = NULL;
	memset(&mctx_trampoline, 0, sizeof(jmp_buf) );

	mpc_common_nodebug("Creation ended");
	mpc_common_spinlock_unlock(&sjlj_spinlock);
	return TRUE;
}

static void sctk_mctx_set_trampoline(int sig)
{
	if(sig != SIGUSR1)
	{
		return;
	}
	mpc_common_nodebug("In trampoline");
	if(sctk_setjmp(mctx_trampoline) == 0)
	{
		mctx_called = TRUE;
		mpc_common_nodebug("In trampoline return");
		return;
	}

	sctk_mctx_set_bootstrap();
}

static void sctk_mctx_set_bootstrap(void)
{
	sctk_mctx_t volatile *mctx_starting;

	void(*volatile mctx_starting_func)(void *);
	volatile void *args;

	mpc_common_nodebug("In bootstrap");
	mctx_starting      = mctx_creating;
	mctx_starting_func = mctx_creating_func;
	args = sctk_create_arg;

	mpc_common_nodebug("Restore main %p %p", mctx_starting, &mctx_caller);
	sctk_swapcontext( (sctk_mctx_t *)mctx_starting,
	                  (sctk_mctx_t *)&mctx_caller);

	mpc_common_nodebug("Start func");
	mctx_starting_func( (void *)args);
	abort();
}
#endif
#elif SCTK_MCTX_MTH(windows)
#error "not implemented yet"
#else
#error "unknown mctx method"
#endif

int sctk_getcontext(sctk_mctx_t *ucp)
{
	mpc_common_nodebug("Save %d", sizeof(sctk_mctx_t) );
	sctk_mctx_save(ucp);
	sctk_context_save_tls(ucp);
	return 0;
}

int sctk_setcontext(sctk_mctx_t *ucp)
{
	sctk_context_restore_tls(ucp);
	sctk_mctx_restore(ucp);
	return 0;
}

int sctk_swapcontext(sctk_mctx_t *oucp, sctk_mctx_t *ucp)
{
	sctk_context_save_tls(oucp);
	sctk_context_restore_tls(ucp);

	oucp->error    = errno;
	oucp->restored = 0;
	errno          = ucp->error;
	ucp->restored  = 1;

#if SCTK_MCTX_MTH(mcsc)
	swapcontext(&(oucp->uc), &(ucp->uc) );
#elif SCTK_MCTX_MTH(libcontext)
	mpc__swapcontext(&(oucp->uc), &(ucp->uc) );
#elif SCTK_MCTX_MTH(sjlj)
	mpc_common_nodebug("swap %p to %p", oucp, ucp);
	if(sctk_setjmp( (oucp->jb) ) == 0)
	{
		sctk_longjmp( (ucp->jb), 1);
	}
#elif SCTK_MCTX_MTH(windows)
	mpc_common_nodebug("swap %p to %p", oucp, ucp);
	if(sctk_setjmp( (oucp->jb) ) == 0)
	{
		sctk_longjmp( (ucp->jb), 1);
	}
#else
#error "unknown mctx method"
#endif
	return 0;
}

int sctk_makecontext(sctk_mctx_t *ucp,
                     void *arg,
                     void (*func)(void *),
                     char *stack,
                     size_t stack_size)
{
	int         res;
	sctk_mctx_t lucp;

	/* force temporary ctx to be inited to avoid unconsistent pointers during the
	 * save() */
	sctk_context_init_tls(&lucp);
	sctk_context_save_tls(&lucp);

	sctk_context_init_tls(ucp);
	sctk_context_restore_tls(ucp);

#ifdef SCTK_USE_VALGRIND
	VALGRIND_STACK_REGISTER(stack, ( ( (char *)stack) + stack_size) );
#endif
	mpc_common_nodebug("new stack %p-%p", stack, ( ( (char *)stack) + stack_size) );
	res = sctk_mctx_set(ucp, func, stack, stack + stack_size, arg);

	sctk_context_restore_tls(&lucp);
	return res;
}

int
sctk_setcontext_no_tls (sctk_mctx_t * ucp)
{
    sctk_mctx_restore (ucp);
    return 0;
}

int
sctk_makecontext_no_tls(
        sctk_mctx_t * ucp,
        void *arg,
        void (*func) (void *), char *stack, size_t stack_size)
{
#ifdef SCTK_USE_VALGRIND
    VALGRIND_STACK_REGISTER (stack, (((char *) stack) + stack_size));
#endif
    mpc_common_nodebug ("new stack %p-%p", stack, (((char *) stack) + stack_size));
    return sctk_mctx_set (ucp, func, stack, stack + stack_size, arg);
}

int
sctk_swapcontext_no_tls (sctk_mctx_t * oucp, sctk_mctx_t * ucp)
{
    oucp->error = errno;
    oucp->restored = 0;
    errno = ucp->error;
    ucp->restored = 1;

#if SCTK_MCTX_MTH(mcsc)
    swapcontext (&(oucp->uc), &(ucp->uc));
#elif SCTK_MCTX_MTH(libcontext)
    mpc__swapcontext (&(oucp->uc), &(ucp->uc));
#elif SCTK_MCTX_MTH(sjlj)
    sctk_nodebug ("swap %p to %p", oucp, ucp);
    if (sctk_setjmp ((oucp->jb)) == 0)
    {
        sctk_longjmp ((ucp->jb), 1);
    }
#elif SCTK_MCTX_MTH(windows)
    sctk_nodebug ("swap %p to %p", oucp, ucp);
    if (sctk_setjmp ((oucp->jb)) == 0)
        sctk_longjmp ((ucp->jb), 1);
#else
#error "unknown mctx method"
#endif
    return 0;
}
