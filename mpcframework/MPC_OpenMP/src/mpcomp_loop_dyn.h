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

#ifndef __MPCOMP_LOOP_DYN_H__
#define __MPCOMP_LOOP_DYN_H__

#include "sctk_bool.h"

void mpcomp_dynamic_loop_init( struct mpcomp_thread_s*, long, long, long, long );
void mpcomp_dynamic_loop_init_ull( struct mpcomp_thread_s*, unsigned long long, unsigned long long, unsigned long long, unsigned long long);

int mpcomp_dynamic_loop_begin(long, long, long, long, long*, long*);
int mpcomp_dynamic_loop_next(long*, long *);
int mpcomp_loop_ull_dynamic_begin(bool, unsigned long long , unsigned long long, unsigned long long, unsigned long long, unsigned long long*, unsigned long long*);
int mpcomp_loop_ull_dynamic_next (unsigned long long *, unsigned long long*);
void mpcomp_dynamic_loop_end ( void );
void mpcomp_dynamic_loop_end_nowait( void );
int mpcomp_dynamic_loop_next_ignore_nowait (long *, long *);

int mpcomp_ordered_dynamic_loop_begin (long, long, long, long, long*, long*);
int mpcomp_ordered_dynamic_loop_next(long *, long *);
int mpcomp_loop_ull_ordered_dynamic_begin (unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long*, unsigned long long*); 
int mpcomp_loop_ull_ordered_dynamic_next(unsigned long long*, unsigned long long*);
void mpcomp_ordered_dynamic_loop_end(void);
void mpcomp_ordered_dynamic_loop_end_nowait(void);

void mpcomp_for_dyn_coherency_end_barrier( void ); 
void mpcomp_for_dyn_coherency_end_parallel_region( struct mpcomp_instance_s* ); 

#endif /* __MPCOMP_LOOP_DYN_H__ */
