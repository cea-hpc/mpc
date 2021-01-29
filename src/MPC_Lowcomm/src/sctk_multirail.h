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
#include <mpc_common_spinlock.h>
#include <lowcomm_config.h>

#include "sctk_rail.h"


/************************************************************************/
/* _mpc_lowcomm_endpoint_list                                                   */
/************************************************************************/

typedef struct _mpc_lowcomm_endpoint_list_s
{
	int priority;
	_mpc_lowcomm_endpoint_t * endpoint;
	
	struct _mpc_lowcomm_config_struct_net_gate * gates;
	int gate_count;
	
	struct _mpc_lowcomm_endpoint_list_s * next;
	struct _mpc_lowcomm_endpoint_list_s * prev;
}_mpc_lowcomm_endpoint_list_t;

int _mpc_lowcomm_endpoint_list_init_entry( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_t * endpoint );
_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_new_entry( _mpc_lowcomm_endpoint_t * endpoint );

_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_push( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_t * endpoint );

_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_pop( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_list_t * topop );
_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_pop_endpoint( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_t * topop );

void _mpc_lowcomm_endpoint_list_free_entry( _mpc_lowcomm_endpoint_list_t * entry );
void _mpc_lowcomm_endpoint_list_release( _mpc_lowcomm_endpoint_list_t ** list );


/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/

typedef struct sctk_multirail_destination_table_entry_s
{
	UT_hash_handle hh;
	
	_mpc_lowcomm_endpoint_list_t * endpoints;
	mpc_common_rwlock_t endpoints_lock;
	
	int destination;
}sctk_multirail_destination_table_entry_t;

void sctk_multirail_destination_table_entry_init( sctk_multirail_destination_table_entry_t * entry, int destination );
void sctk_multirail_destination_table_entry_release( sctk_multirail_destination_table_entry_t * entry );
void sctk_multirail_destination_table_entry_free( sctk_multirail_destination_table_entry_t * entry );

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_entry_new(int destination);

void sctk_multirail_destination_table_entry_push_endpoint( sctk_multirail_destination_table_entry_t * entry, _mpc_lowcomm_endpoint_t * endpoint );
void sctk_multirail_destination_table_entry_pop_endpoint( sctk_multirail_destination_table_entry_t * entry, _mpc_lowcomm_endpoint_t * endpoint );

struct sctk_multirail_destination_table
{
	sctk_multirail_destination_table_entry_t * destinations;
	mpc_common_rwlock_t table_lock;
};

void sctk_multirail_destination_table_init();
void sctk_multirail_destination_table_release();

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_acquire_routes(int destination );
void sctk_multirail_destination_table_relax_routes( sctk_multirail_destination_table_entry_t * entry );

void sctk_multirail_destination_table_push_endpoint(_mpc_lowcomm_endpoint_t * endpoint );
void sctk_multirail_destination_table_pop_endpoint(_mpc_lowcomm_endpoint_t * topop );
void sctk_multirail_destination_table_route_to_process( int destination, int * new_destination );
void sctk_multirail_destination_table_prune(void);

/************************************************************************/
/* Pending on-demand connections                                        */
/************************************************************************/

typedef struct sctk_pending_on_demand_s
{
	sctk_rail_info_t * rail;	
	int dest;
	struct sctk_pending_on_demand_s * next;
}sctk_pending_on_demand_t;

void sctk_pending_on_demand_push( sctk_rail_info_t * rail, int dest );
void sctk_pending_on_demand_release( sctk_pending_on_demand_t * pod );
sctk_pending_on_demand_t * sctk_pending_on_demand_get();
void sctk_pending_on_demand_process();
void sctk_multirail_on_demand_connection(mpc_lowcomm_ptp_message_t*);
void sctk_multirail_on_demand_disconnection_process(int);
void sctk_multirail_on_demand_disconnection_rail(sctk_rail_info_t*);
void sctk_multirail_on_demand_disconnection_routes(_mpc_lowcomm_endpoint_type_t);

#endif /* SCTK_MULTIRAIL */
