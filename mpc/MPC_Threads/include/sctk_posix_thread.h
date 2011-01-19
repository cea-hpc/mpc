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

  struct _sctk_thread_cleanup_buffer
  {
    void (*__routine) (void *);	/* Function to call.  */
    void *__arg;		/* Its argument.  */
    struct _sctk_thread_cleanup_buffer *next;	/* Chaining of cleanup functions.  */
  };

#define sctk_thread_cleanup_push(routine,arg) \
  { struct _sctk_thread_cleanup_buffer _buffer;				      \
    _sctk_thread_cleanup_push (&_buffer, (routine), (arg));

  extern void _sctk_thread_cleanup_push (struct _sctk_thread_cleanup_buffer
					 *__buffer,
					 void (*__routine) (void *),
					 void *__arg);

/* Remove a cleanup handler installed by the matching sctk_thread_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */

#define sctk_thread_cleanup_pop(execute) \
    _sctk_thread_cleanup_pop (&_buffer, (execute)); }

  extern void _sctk_thread_cleanup_pop (struct _sctk_thread_cleanup_buffer
					*__buffer, int __execute);




#ifdef __cplusplus
}
#endif
#endif
