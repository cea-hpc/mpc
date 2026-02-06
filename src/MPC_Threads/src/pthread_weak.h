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
/* #   - BEAULIEU Corentin corentin.beaulieu@cea.fr                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef PTHREAD_WEAK_H
#define PTHREAD_WEAK_H
#include "mpc_thread.h" // For symbol declaration (not mandatory)

#pragma weak sched_yield = mpc_thread_yield
#pragma weak raise       = mpc_thread_raise
#ifndef SCTK_DONOT_REDEFINE_KILL
#pragma weak kill        = mpc_thread_process_kill
#endif

#pragma weak sigpending  = mpc_thread_sigpending
#pragma weak sigsuspend  = mpc_thread_sigsuspend
#pragma weak sigwait     = mpc_thread_sigwait

#pragma weak sleep       = mpc_thread_sleep
#pragma weak usleep      = mpc_thread_usleep
#pragma weak nanosleep   = mpc_thread_nanosleep

#endif // PTHREAD_WEAK_H
