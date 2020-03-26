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
#ifndef __SCTK_POSIX_THREAD_H_
#define __SCTK_POSIX_THREAD_H_
#ifdef __cplusplus
extern "C"
{
#endif



unsigned long mpc_thread_atomic_add(volatile unsigned long
                                    *ptr, unsigned long val);

unsigned long mpc_thread_tls_entry_add(unsigned long size,
                                       void (*func)(void *) );



/* Futexes */

long  mpc_thread_futex(int sysop, void *addr1, int op, int val1,
                       struct timespec *timeout, void *addr2, int val3);
long  mpc_thread_futex_with_vaargs(int sysop, ...);


/* MPC_MPI Trampolines */

void mpc_thread_per_mpi_task_atexit_set_trampoline(int (*trampoline)(void (*func)(void) ) );

struct mpc_mpi_cl_per_mpi_process_ctx_s;
void mpc_thread_get_mpi_process_ctx_set_trampoline(struct mpc_mpi_cl_per_mpi_process_ctx_s * (*trampoline)(void) );

struct _sctk_thread_cleanup_buffer
{
	void                                (*__routine) (void *); /* Function to call.  */
	void *                              __arg;                 /* Its argument.  */
	struct _sctk_thread_cleanup_buffer *next;                  /* Chaining of cleanup functions.  */
};


void _sctk_thread_cleanup_push(struct _sctk_thread_cleanup_buffer *__buffer,
                               void (*__routine)(void *),
                               void *__arg);


#define sctk_thread_cleanup_push(routine, arg)        \
	{ struct _sctk_thread_cleanup_buffer _buffer; \
	  _sctk_thread_cleanup_push(&_buffer, (routine), (arg) );

void _sctk_thread_cleanup_pop(struct _sctk_thread_cleanup_buffer *__buffer, int __execute);



#define sctk_thread_cleanup_pop(execute) \
	_sctk_thread_cleanup_pop(&_buffer, (execute) ); }



#ifdef __cplusplus
}
#endif
#endif
