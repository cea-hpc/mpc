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

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_lock.h"
#include "mpcomp_intel_lock.h"
#include "mpcomp_intel_types.h"

void __kmpc_init_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_lock_t *user_mpcomp_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_lock = (omp_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_lock);
  omp_init_lock(user_mpcomp_lock);
}

void __kmpc_init_nest_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_nest_lock = (omp_nest_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_nest_lock);
  omp_init_nest_lock(user_mpcomp_nest_lock);
}

void __kmpc_destroy_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_lock_t *user_mpcomp_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_lock = (omp_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_lock);
  omp_destroy_lock(user_mpcomp_lock);
}

void __kmpc_destroy_nest_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_nest_lock = (omp_nest_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_nest_lock);
  omp_destroy_nest_lock(user_mpcomp_nest_lock);
}

void __kmpc_set_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_lock_t *user_mpcomp_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_lock = (omp_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_lock);
  omp_set_lock(user_mpcomp_lock);
}

void __kmpc_set_nest_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_nest_lock = (omp_nest_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_nest_lock);
  omp_set_nest_lock(user_mpcomp_nest_lock);
}

void __kmpc_unset_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_lock_t *user_mpcomp_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_lock = (omp_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_lock);
  omp_unset_lock(user_mpcomp_lock);
}

void __kmpc_unset_nest_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_nest_lock = (omp_nest_lock_t *)(&(user_iomp_lock->lk));
  sctk_error("[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__, user_iomp_lock,
             user_mpcomp_nest_lock);
  omp_unset_nest_lock(user_mpcomp_nest_lock);
}

int __kmpc_test_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_lock_t *user_mpcomp_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_lock = (omp_lock_t *)(&(user_iomp_lock->lk));
  const int ret = omp_test_lock(user_mpcomp_lock);
  sctk_error("[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
             user_iomp_lock, user_mpcomp_lock, ret);
  return ret;
}

int __kmpc_test_nest_lock(ident_t *loc, kmp_int32 gtid, void **user_lock) {
  omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
  iomp_lock_t *user_iomp_lock = (iomp_lock_t *)user_lock;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(user_iomp_lock);

  user_mpcomp_nest_lock = (omp_nest_lock_t *)(&(user_iomp_lock->lk));
  const int ret = omp_test_nest_lock(user_mpcomp_nest_lock);
  sctk_error("[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
             user_iomp_lock, user_mpcomp_nest_lock, ret);
  return ret;
}
