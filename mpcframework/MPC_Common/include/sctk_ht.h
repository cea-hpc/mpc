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
#ifndef SCTK_HASH_TABLE
#define SCTK_HASH_TABLE

#include <sctk_stdint.h>
#include <mpc_common_spinlock.h>

struct MPCHT_Cell
{
	char use_flag;
	sctk_uint64_t key;
	void * data;
	struct MPCHT_Cell * next;
};

struct MPCHT_Cell * MPCHT_Cell_new( sctk_uint64_t key, void * data, struct MPCHT_Cell * next );
void MPCHT_Cell_init( struct MPCHT_Cell * cell , sctk_uint64_t key, void * data, struct MPCHT_Cell * next );
void MPCHT_Cell_release( struct MPCHT_Cell * cell );
struct MPCHT_Cell * MPCHT_Cell_get( struct MPCHT_Cell * cell, sctk_uint64_t key );
struct MPCHT_Cell * MPCHT_Cell_pop( struct MPCHT_Cell * head, sctk_uint64_t key );


struct MPCHT
{
	struct MPCHT_Cell * cells;
	sctk_spin_rwlock_t * rwlocks;
	sctk_uint64_t table_size;
};

void MPCHT_init( struct MPCHT * ht, sctk_uint64_t size );
void MPCHT_release( struct MPCHT * ht );

void * MPCHT_get(  struct MPCHT * ht, sctk_uint64_t key );
void * MPCHT_get_or_create(  struct MPCHT * ht, sctk_uint64_t key , void * (create_entry)( sctk_uint64_t key ), int * did_create );
void MPCHT_set(  struct MPCHT * ht, sctk_uint64_t key, void * data );
void MPCHT_delete(  struct MPCHT * ht, sctk_uint64_t key );
int MPCHT_empty(struct MPCHT * ht);

/* Fine grained locking */
void MPCHT_lock_cell_read( struct MPCHT * ht , sctk_uint64_t key);
void MPCHT_unlock_cell_read( struct MPCHT * ht , sctk_uint64_t key);
void MPCHT_lock_cell_write( struct MPCHT * ht , sctk_uint64_t key);
void MPCHT_unlock_cell_write( struct MPCHT * ht , sctk_uint64_t key);


#define MPC_HT_ITER( ht, var )\
unsigned int ____i;\
for( ____i = 0 ; ____i < (ht)->table_size ; ____i++ )\
{\
	struct MPCHT_Cell * ____c = &( ht )->cells[____i];\
	if( ____c->use_flag == 0 )\
		continue;\
\
	while( ____c )\
	{\
		var = ____c->data;
		
#define MPC_HT_ITER_END \
		____c = ____c->next;\
	}\
}

#endif /* SCTK_HASH_TABLE */
