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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_INTEL_LOCK_H__
#define __MPCOMP_INTEL_LOCK_H__
    
#include "sctk_debug.h"
#include "mpcomp_intel_types.h"

/*
 * Lock structure
 */

typedef struct iomp_lock_s
{
  void* lk;
} iomp_lock_t;

void __kmpc_init_lock( ident_t *loc, kmp_int32 gtid,  void **user_lock );
void __kmpc_init_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_destroy_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_destroy_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_set_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_set_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_unset_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
void __kmpc_unset_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
int __kmpc_test_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );
int __kmpc_test_nest_lock( ident_t *loc, kmp_int32 gtid, void **user_lock );

#endif /* __MPCOMP_INTEL_LOCK_H__ */
