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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_CONTEXT_H_
#define __SCTK_CONTEXT_H_

#include <stdlib.h>
#include <string.h>
#ifdef MPC_USE_EXTLS
	#include <extls.h>
#endif

#include <mpc_config.h>
#include "mpc_threads_config.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define SCTK_MCTX_MTH(which)    (SCTK_MCTX_MTH_use == (SCTK_MCTX_MTH_ ## which) )
#define SCTK_MCTX_DSP(which)    (SCTK_MCTX_DSP_use == (SCTK_MCTX_DSP_ ## which) )
#define SCTK_MCTX_STK(which)    (SCTK_MCTX_STK_use == (SCTK_MCTX_STK_ ## which) )
#define SCTK_MCTX_MTH_mcsc          1
#define SCTK_MCTX_MTH_sjlj          2
#define SCTK_MCTX_MTH_windows       3
#define SCTK_MCTX_MTH_libcontext    4
#define SCTK_MCTX_DSP_sc            1
#define SCTK_MCTX_DSP_sjlj          2
#define SCTK_MCTX_DSP_windows       3
#define SCTK_MCTX_DSP_libcontext    4


/*Context definition*/
#define SCTK_MCTX_MTH_use    SCTK_MCTX_MTH_mcsc
#define SCTK_MCTX_DSP_use    SCTK_MCTX_DSP_sc

#if defined(MPC_I686_ARCH)
#ifndef DONOTHAVE_CONTEXTS
#define SCTK_USE_CONTEXT_FOR_CREATION
#endif

#undef DONOTHAVE_CONTEXTS
#define DONOTHAVE_CONTEXTS
#endif

#if defined(MPC_X86_64_ARCH)
#if 1
#undef SCTK_MCTX_MTH_use
#undef SCTK_MCTX_DSP_use
#define SCTK_MCTX_MTH_use    SCTK_MCTX_MTH_libcontext
#define SCTK_MCTX_DSP_use    SCTK_MCTX_DSP_libcontext
#else

/*
 * Get a bug on new version of libs (centos6....), need to fix this. It produce segfault at
 * make install with --enable-debug. Seams to be impacted by -OX option.
 * Need to check this in more depth for futur version ( > 2.4.0-1).
 *
 * #ifndef DONOTHAVE_CONTEXTS
 * #if (defined(__linux__) && (defined(__GLIBC__) && ((__GLIBC__ >= 2) &&  (__GLIBC_MINOR__ >= 12)) ))
 * #define SCTK_USE_CONTEXT_FOR_CREATION
 * #endif
 * #endif
 */

#undef DONOTHAVE_CONTEXTS
#define DONOTHAVE_CONTEXTS

/*
 * we disable makecontext/swapcontext
 * */
#endif
#endif

#ifdef __MIC__
//#undef DONOTHAVE_CONTEXTS
//#define DONOTHAVE_CONTEXTS
#endif

#ifdef DONOTHAVE_CONTEXTS
#undef SCTK_MCTX_MTH_use
#undef SCTK_MCTX_DSP_use
#define SCTK_MCTX_MTH_use    SCTK_MCTX_MTH_sjlj
#define SCTK_MCTX_DSP_use    SCTK_MCTX_DSP_sjlj
#endif

#ifdef WINDOWS_SYS
#undef SCTK_MCTX_MTH_use
#undef SCTK_MCTX_DSP_use
#define SCTK_MCTX_MTH_use    SCTK_MCTX_MTH_windows
#define SCTK_MCTX_DSP_use    SCTK_MCTX_DSP_windows
#endif

/*Stack start according to stack direction*/
#define sctk_skaddr_sigstack(skaddr, sksize)            ( (skaddr) )
#define sctk_skaddr_sigaltstack(skaddr, sksize)         ( (skaddr) )
#define sctk_skaddr_makecontext(skaddr, sksize)         ( (skaddr) )
#define sctk_skaddr_mpc__makecontext(skaddr, sksize)    ( (skaddr) )

/*Stack size*/
#define sctk_sksize_sigstack(skaddr, sksize)            ( (sksize) )
#define sctk_sksize_sigaltstack(skaddr, sksize)         ( (sksize) )
#define sctk_sksize_makecontext(skaddr, sksize)         ( (sksize) )
#define sctk_sksize_mpc__makecontext(skaddr, sksize)    ( (sksize) )

#if !defined(AIX_SYS)
#define FALSE    1
#define TRUE     0
#endif

#define sctk_skaddr(func, skaddr, sksize)    sctk_skaddr_ ## func(skaddr, sksize)
#define sctk_sksize(func, skaddr, sksize)    sctk_sksize_ ## func(skaddr, sksize)


/*
 * machine context state structure
 *
 * In `jb' the CPU registers, the program counter, the stack
 * pointer and (usually) the signals mask is stored. When the
 * signal mask cannot be implicitly stored in `jb', it's
 * alternatively stored explicitly in `sigs'. The `error' stores
 * the value of `errno'.
 */


#if SCTK_MCTX_MTH(mcsc)
#include <ucontext.h>
#endif

#if SCTK_MCTX_MTH(libcontext)
#include <mpc_arch_context.h>
#endif

#if SCTK_MCTX_MTH(sjlj)
#include <signal.h>
#include <setjmp.h>

#define sctk_longjmp    longjmp
#define sctk_setjmp     setjmp

#ifdef SCTK_NOT_USE_SIGACTION
struct sigaction
{
	void     ( *sa_handler ) (int);
	sigset_t sa_mask;
	int      sa_flags;
};
#define sigaction(a, b, c)    signal( (a), (b)->sa_handler)
#endif
#endif
#if SCTK_MCTX_MTH(windows)
#include <signal.h>
#include <setjmp.h>
#endif

#ifdef MPC_MPI
struct mpc_mpi_cl_per_thread_ctx_s;
#endif

typedef struct sctk_mctx_st
{
#if SCTK_MCTX_MTH(mcsc)
	ucontext_t                          uc;
#elif SCTK_MCTX_MTH(sjlj)
	jmp_buf                             jb;
#elif SCTK_MCTX_MTH(windows)
	jmp_buf                             jb;
#elif SCTK_MCTX_MTH(libcontext)
	sctk_ucontext_t                     uc;
#else
#error "unknown mctx method"
#endif
	volatile int                        restored;
	sigset_t                            sigs;
	int                                 error;
	void *                              thread_lib;
	void *                              sctk_tls_trace_local;
	void *                              mpc_user_tls_1;
#ifdef MPC_USE_EXTLS
	extls_ctx_t *                       tls_ctx;
#endif

#ifdef MPC_MPI
	struct mpc_mpi_cl_per_thread_ctx_s *___mpc_p_per_thread_comm_ctx;
#endif

#ifdef MPC_MPIIO
	struct mpc_thread_romio_ctx_s * mpc_thread_romio_ctx_storage;
#endif

#ifdef MPC_Lowcomm
	int                                 __mpc_task_rank;
#endif

#if defined(MPC_USE_CUDA)
	/* MPC CUDA context */
	void *                              sctk_cuda_ctx;
#endif

#if defined (MPC_OpenMP)
	/* MPC OpenMP TLS */
	void *                              mpc_omp_tls;
#endif

#if defined MPC_USE_DMTCP
	int                                 sctk_ft_critical_section;
#endif

} sctk_mctx_t;

int sctk_getcontext(sctk_mctx_t *ucp);
int sctk_setcontext(sctk_mctx_t *ucp);
int sctk_swapcontext(sctk_mctx_t *oucp, sctk_mctx_t *ucp);
int sctk_makecontext(sctk_mctx_t *ucp,
                     void *arg,
                     void (*func)(void *),
                     char *stack, size_t stack_size);

void sctk_tls_init(void);

int sctk_getcontext_no_tls(sctk_mctx_t *ucp);
int sctk_setcontext_no_tls(sctk_mctx_t *ucp);
int sctk_swapcontext_no_tls(sctk_mctx_t *oucp, sctk_mctx_t *ucp);
int sctk_makecontext_no_tls(sctk_mctx_t *ucp,
                     void *arg,
                     void (*func)(void *),
                     char *stack, size_t stack_size);

#ifdef __cplusplus
}
#endif
#endif
