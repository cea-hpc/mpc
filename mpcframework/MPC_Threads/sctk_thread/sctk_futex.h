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
#ifndef SCTK_FUTEX_H
#define SCTK_FUTEX_H

#include "sctk_buffered_fifo.h"
#include "sctk_ht.h"
#include "sctk_spinlock.h"
#include "sctk_atomics.h"

/************************************************************************/
/* Futex Queues                                                         */
/************************************************************************/

struct futex_queue
{
	OPA_int_t queue_is_wake_tainted;
	struct Buffered_FIFO wait_list;
	int * futex_key;
};

struct futex_queue * futex_queue_new( int * futex_key );
int futex_queue_release( struct futex_queue * fq );
int ** futex_queue_push( struct futex_queue * fq );
int futex_queue_wake( struct futex_queue * fq, int count );

/************************************************************************/
/* Futex HT                                                             */
/************************************************************************/

struct futex_queue_HT
{
	OPA_int_t queue_table_is_being_manipulated;
	
	struct MPCHT queue_hash_table;
	unsigned int queue_count;
	unsigned int queue_cleanup_ratio;
};

int futex_queue_HT_init( struct futex_queue_HT * ht );
int futex_queue_HT_release( struct futex_queue_HT * ht );
int * futex_queue_HT_register_thread( struct futex_queue_HT * ht , int * futex_key );
int futex_queue_HT_wake_threads( struct futex_queue_HT * ht , int * futex_key, int count );


/************************************************************************/
/* Futex Context                                                        */
/************************************************************************/

void sctk_futex_context_init();
void sctk_futex_context_release();

/************************************************************************/
/* Futex Ops                                               	            */
/************************************************************************/

int sctk_futex(void *addr1, int op, int val1, 
               struct timespec *timeout, void *addr2, int val3);


/************************************************************************/
/* Futex Syscall Trampoline                                             */
/************************************************************************/

int dyn_SYS_futex();


#endif /* SCTK_FUTEX_H */
