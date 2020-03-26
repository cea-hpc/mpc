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

#include <mpcthread.h>
#include "sctk_futex.h"
#include "sctk_thread.h"

#include <fcntl.h>


unsigned long mpc_thread_atomic_add(volatile unsigned long
                                    *ptr, unsigned long val)
{
	return sctk_thread_atomic_add(ptr, val);
}

unsigned long mpc_thread_tls_entry_add(unsigned long size,
                                       void (*func)(void *) )
{
	return sctk_tls_entry_add(size, func);
}



/* Futexes */

long  mpc_thread_futex(__UNUSED__ int sysop, void *addr1, int op, int val1,
                       struct timespec *timeout, void *addr2, int val2)
{
	sctk_futex_context_init();
	return sctk_thread_futex(addr1, op, val1, timeout, addr2, val2);
}

long mpc_thread_futex_with_vaargs(int sysop, ...)
{
	va_list          ap;
	void *           addr1, *addr2;
	int              op, val1, val2;
	struct timespec *timeout;

	va_start(ap, sysop);
	addr1   = va_arg(ap, void *);
	op      = va_arg(ap, int);
	val1    = va_arg(ap, int);
	timeout = va_arg(ap, struct timespec *);
	addr2   = va_arg(ap, void *);
	val2    = va_arg(ap, int);
	va_end(ap);

	return mpc_thread_futex(sysop, addr1, op, val1, timeout, addr2, val2);
}
