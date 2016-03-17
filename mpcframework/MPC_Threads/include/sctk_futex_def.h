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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_FUTEX_DEF_H
#define SCTK_FUTEX_DEF_H

/************************************************************************/
/* Internal MPC Futex Codes                                             */
/************************************************************************/


#define SCTK_FUTEX_WAIT 21210
#define SCTK_FUTEX_WAKE 21211
#define SCTK_FUTEX_REQUEUE 21212
#define SCTK_FUTEX_CMP_REQUEUE 21213
#define SCTK_FUTEX_WAKE_OP 21214
#define SCTK_FUTEX_WAIT_BITSET 21215
#define SCTK_FUTEX_WAKE_BITSET 21216
#define SCTK_FUTEX_LOCK_PI 21217
#define SCTK_FUTEX_TRYLOCK_PI 21218
#define SCTK_FUTEX_UNLOCK_PI 21219
#define SCTK_FUTEX_CMP_REQUEUE_PI 21220
#define SCTK_FUTEX_WAIT_REQUEUE_PI 21221

/* WAITERS */
#define SCTK_FUTEX_WAITERS (1 << 31)

/* OPS */

#define SCTK_FUTEX_OP_SET 0
#define SCTK_FUTEX_OP_ADD 1 
#define SCTK_FUTEX_OP_OR 2 
#define SCTK_FUTEX_OP_ANDN 3
#define SCTK_FUTEX_OP_XOR 4  
#define SCTK_FUTEX_OP_ARG_SHIFT 8

/* CMP */

#define SCTK_FUTEX_OP_CMP_EQ 0
#define SCTK_FUTEX_OP_CMP_NE 1
#define SCTK_FUTEX_OP_CMP_LT 2
#define SCTK_FUTEX_OP_CMP_LE 3
#define SCTK_FUTEX_OP_CMP_GT 4
#define SCTK_FUTEX_OP_CMP_GE 5

/************************************************************************/
/* Futex Ops                                               	            */
/************************************************************************/

int sctk_futex(void *addr1, int op, int val1, 
               struct timespec *timeout, void *addr2, int val3);

#endif /* SCTK_FUTEX_DEF_H */
