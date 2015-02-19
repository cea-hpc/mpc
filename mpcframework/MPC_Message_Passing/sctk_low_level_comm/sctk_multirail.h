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
#ifndef SCTK_MULTIRAIL
#define SCTK_MULTIRAIL

#include <uthash.h>

#include <sctk_route.h>
#include <sctk_spinlock.h>
#include <sctk_runtime_config_struct.h>

/************************************************************************/
/* sctk_endpoint_list                                                   */
/************************************************************************/

typedef struct sctk_endpoint_list_s
{
	int priority;
	sctk_endpoint_t * endpoint;
	
	struct sctk_runtime_config_struct_net_gate * gates;
	int gate_count;
	
	struct sctk_endpoint_list_s * next;
	struct sctk_endpoint_list_s * prev;
}sctk_endpoint_list_t;

int sctk_endpoint_list_init_entry( sctk_endpoint_list_t * list, sctk_endpoint_t * endpoint );
sctk_endpoint_list_t * sctk_endpoint_list_new_entry( sctk_endpoint_t * endpoint );

sctk_endpoint_list_t * sctk_endpoint_list_push( sctk_endpoint_list_t * list, sctk_endpoint_t * endpoint );

sctk_endpoint_list_t * sctk_endpoint_list_pop( sctk_endpoint_list_t * list, sctk_endpoint_list_t * topop );
sctk_endpoint_list_t * sctk_endpoint_list_pop_endpoint( sctk_endpoint_list_t * list, sctk_endpoint_t * topop );

void sctk_endpoint_list_free_entry( sctk_endpoint_list_t * entry );
void sctk_endpoint_list_release( sctk_endpoint_list_t ** list );


/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/

typedef struct sctk_multirail_destination_table_entry_s
{
	UT_hash_handle hh;
	
	sctk_endpoint_list_t * endpoints;
	sctk_spin_rwlock_t endpoints_lock;
	
	int destination;
}sctk_multirail_destination_table_entry_t;

void sctk_multirail_destination_table_entry_init( sctk_multirail_destination_table_entry_t * entry, int destination );
void sctk_multirail_destination_table_entry_release( sctk_multirail_destination_table_entry_t * entry );
void sctk_multirail_destination_table_entry_free( sctk_multirail_destination_table_entry_t * entry );

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_entry_new(int destination);

void sctk_multirail_destination_table_entry_push_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint );
void sctk_multirail_destination_table_entry_pop_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint );

struct sctk_multirail_destination_table
{
	sctk_multirail_destination_table_entry_t * destinations;
	sctk_spin_rwlock_t table_lock;
};

void sctk_multirail_destination_table_init();
void sctk_multirail_destination_table_release();

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_acquire_routes(int destination );
void sctk_multirail_destination_table_relax_routes( sctk_multirail_destination_table_entry_t * entry );

void sctk_multirail_destination_table_push_endpoint(sctk_endpoint_t * endpoint );
void sctk_multirail_destination_table_pop_endpoint(sctk_endpoint_t * topop );
void sctk_multirail_destination_table_route_to_process( int destination, int * new_destination );


#endif /* SCTK_MULTIRAIL */
