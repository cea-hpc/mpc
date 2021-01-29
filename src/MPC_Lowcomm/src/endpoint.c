/* ############################# MPC License ############################## */
/* # Fri Jan 29 12:37:11 CET 2021                                         # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.fr                       # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_debug.h>
#include <limits.h>
#include <sctk_alloc.h>

#include "endpoint.h"

/* Return if the process is the initiator of the connexion or not */
char _mpc_lowcomm_endpoint_is_initiator(_mpc_lowcomm_endpoint_t *edp)
{
	assume(edp);
	int is_initiator;

	_MPC_LOWCOMM_ENDPOINT_LOCK(edp);
	is_initiator = edp->is_initiator;
	_MPC_LOWCOMM_ENDPOINT_UNLOCK(edp);

	return is_initiator;
}

void _mpc_lowcomm_endpoint_init(_mpc_lowcomm_endpoint_t *edp, int dest, sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_type_t type)
{
	memset(edp, 0, sizeof(_mpc_lowcomm_endpoint_t) );
	edp->dest        = dest;
	edp->rail        = rail;
	edp->parent_rail = NULL;
	edp->subrail_id  = -1;

	_mpc_lowcomm_endpoint_set_state(edp, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);

	edp->is_initiator = CHAR_MAX;
	mpc_common_spinlock_init(&edp->lock, 0);

	edp->type = type;
}

/************************************************************************/
/* Route Table                                                          */
/************************************************************************/

_mpc_lowcomm_endpoint_table_t *_mpc_lowcomm_endpoint_table_new()
{
	_mpc_lowcomm_endpoint_table_t *ret = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_table_t) );

	assume(ret != NULL);

	mpc_common_rwlock_t lck = SCTK_SPIN_RWLOCK_INITIALIZER;
	ret->dynamic_route_table_lock = lck;

	mpc_common_hashtable_init(&ret->dynamic_route_table, 128);
	mpc_common_hashtable_init(&ret->static_route_table, 128);


	return ret;
}

void _mpc_lowcomm_endpoint_table_free(_mpc_lowcomm_endpoint_table_t **table)
{
	mpc_common_spinlock_write_lock(&(*table)->dynamic_route_table_lock);
	mpc_common_hashtable_release(&(*table)->dynamic_route_table);
	mpc_common_hashtable_release(&(*table)->static_route_table);
	mpc_common_spinlock_write_unlock(&(*table)->dynamic_route_table_lock);

	sctk_free(*table);
	*table = NULL;
}

void _mpc_lowcomm_endpoint_table_clear(_mpc_lowcomm_endpoint_table_t **table)
{
	_mpc_lowcomm_endpoint_table_free(table);
	*table = _mpc_lowcomm_endpoint_table_new();
}

int _mpc_lowcomm_endpoint_table_has_routes(_mpc_lowcomm_endpoint_table_t *table)
{
	return mpc_common_hashtable_empty(&table->dynamic_route_table) && mpc_common_hashtable_empty(&table->static_route_table);
}

/************************************************************************/
/* Route Table setters                                                  */
/************************************************************************/

void _mpc_lowcomm_endpoint_table_add_static_route(_mpc_lowcomm_endpoint_table_t *table, _mpc_lowcomm_endpoint_t *edp)
{
	mpc_common_nodebug("New static route to %d", edp->dest);

	assume(edp->type == _MPC_LOWCOMM_ENDPOINT_STATIC);

	mpc_common_hashtable_set(&table->static_route_table, edp->dest, edp);
}

void _mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock(_mpc_lowcomm_endpoint_table_t *table, _mpc_lowcomm_endpoint_t *edp)
{
	assume(edp->type == _MPC_LOWCOMM_ENDPOINT_DYNAMIC);
	mpc_common_hashtable_set(&table->dynamic_route_table, edp->dest, edp);
}

void _mpc_lowcomm_endpoint_table_add_dynamic_route(_mpc_lowcomm_endpoint_table_t *table, _mpc_lowcomm_endpoint_t *edp)
{
	assume(edp->type == _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

	mpc_common_spinlock_write_lock(&table->dynamic_route_table_lock);
	_mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock(table, edp);
	mpc_common_spinlock_write_unlock(&table->dynamic_route_table_lock);
}

/************************************************************************/
/* Route Table Getters                                                  */
/************************************************************************/

_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_static_route(_mpc_lowcomm_endpoint_table_t *table, int dest)
{
	_mpc_lowcomm_endpoint_t *tmp = NULL;

	tmp = mpc_common_hashtable_get(&table->static_route_table, dest);

	mpc_common_nodebug("Get static route for %d -> %p", dest, tmp);

	return tmp;
}

_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_dynamic_route_no_lock(_mpc_lowcomm_endpoint_table_t *table, int dest)
{
	_mpc_lowcomm_endpoint_t *tmp = NULL;

	tmp = mpc_common_hashtable_get(&table->dynamic_route_table, dest);

	return tmp;
}

_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_dynamic_route(_mpc_lowcomm_endpoint_table_t *table, int dest)
{
	_mpc_lowcomm_endpoint_t *tmp;

	mpc_common_spinlock_read_lock(&table->dynamic_route_table_lock);
	tmp = _mpc_lowcomm_endpoint_table_get_dynamic_route_no_lock(table, dest);
	mpc_common_spinlock_read_unlock(&table->dynamic_route_table_lock);

	return tmp;
}

/************************************************************************/
/* Route Walking                                                        */
/************************************************************************/

/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void _mpc_lowcomm_endpoint_table_walk_dynamic(_mpc_lowcomm_endpoint_table_t *table, void (*func)(_mpc_lowcomm_endpoint_t *endpoint, void *arg), void *arg)
{
	_mpc_lowcomm_endpoint_t *current_route;

	if(table == NULL)
	{
		return;           /* no routes */
	}

	/* Do not walk on static routes */
	MPC_HT_ITER(&table->dynamic_route_table, current_route)
	func(current_route, arg);
	MPC_HT_ITER_END
}

/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void _mpc_lowcomm_endpoint_table_walk_static(_mpc_lowcomm_endpoint_table_t *table, void (*func)(_mpc_lowcomm_endpoint_t *endpoint, void *arg), void *arg)
{
	_mpc_lowcomm_endpoint_t *current_route;

	if(table == NULL)
	{
		return;          /* no route */
	}

	MPC_HT_ITER(&table->static_route_table, current_route)
	func(current_route, arg);
	MPC_HT_ITER_END
}
