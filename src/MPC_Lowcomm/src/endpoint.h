/* ############################# MPC License ############################## */
/* # Fri Jan 29 12:37:09 CET 2021                                         # */
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
#ifndef _MPC_LOWCOMM_ENDPOINT_H_
#define _MPC_LOWCOMM_ENDPOINT_H_

#include <mpc_common_spinlock.h>
#include <mpc_common_types.h>
#include <mpc_common_datastructure.h>
#include <mpc_lowcomm_monitor.h>

/* Typedefs for this file are in the central
 * header to allow later references avoiding
 * circular refences */
#include "lowcomm_types_internal.h"

/* Driver definitions */
#include "driver.h"

/************************************************************************/
/* ENDPOINTS                                                            */
/************************************************************************/

/** @brief Endpoint type */
typedef enum
{
	_MPC_LOWCOMM_ENDPOINT_DYNAMIC = 111, /**< Endpoint has been created dynamically */
	_MPC_LOWCOMM_ENDPOINT_STATIC  = 222  /**< Endpoint has been created statically */
} _mpc_lowcomm_endpoint_type_t;

/** @brief Network dependent ROUTE informations */
typedef union
{
	_mpc_lowcomm_endpoint_info_tcp_t     tcp;    /**< TCP route info */
#ifdef MPC_USE_INFINIBAND
	_mpc_lowcomm_endpoint_info_ib_t      ib;     /**< IB route info */
#endif
	_mpc_lowcomm_endpoint_info_shm_t     shm;
#ifdef MPC_USE_PORTALS
	_mpc_lowcomm_endpoint_info_portals_t ptl;    /*< Portals route info */
#endif
#ifdef MPC_USE_OFI
	_mpc_lowcomm_endpoint_info_ofi_t     ofi;   /*< OFI-specific route info */
#endif
} _mpc_lowcomm_endpoint_info_t;

/** @brief State of the Route */
typedef enum
{
	_MPC_LOWCOMM_ENDPOINT_DECONNECTED    = 0,
	_MPC_LOWCOMM_ENDPOINT_CONNECTED      = 111,
	_MPC_LOWCOMM_ENDPOINT_FLUSHING       = 222,
	_MPC_LOWCOMM_ENDPOINT_FLUSHING_CHECK = 233,
	_MPC_LOWCOMM_ENDPOINT_FLUSHED        = 234,
	_MPC_LOWCOMM_ENDPOINT_CONNECTING     = 666,
	_MPC_LOWCOMM_ENDPOINT_RECONNECTING   = 444,
	_MPC_LOWCOMM_ENDPOINT_RESET          = 555,
	_MPC_LOWCOMM_ENDPOINT_RESIZING       = 777,
	_MPC_LOWCOMM_ENDPOINT_REQUESTING     = 888,
} _mpc_lowcomm_endpoint_state_t;

struct sctk_rail_info_s;

/** @brief Endpoint table entry
 *  Describes a network endpoint
 */
struct _mpc_lowcomm_endpoint_s
{
	mpc_lowcomm_peer_uid_t       dest;              /**< Target UID */
	_mpc_lowcomm_endpoint_info_t data;              /**< Rail level content */
	struct sctk_rail_info_s *    parent_rail;       /**< Pointer to the parent rail (called by default if present) */
	struct sctk_rail_info_s *    rail;              /**< Pointer to the rail owning this endpoint */
	int                          subrail_id;        /**< The id of the subrail (if applicable otherwise -1) */
	_mpc_lowcomm_endpoint_type_t type;              /**< Origin of the route entry: static or dynamic route */
	OPA_int_t                    state;             /**< State of the route \ref _mpc_lowcomm_endpoint_state_t */
	char                         is_initiator;      /**< Return if the process is the initiator of the remote creation.
	                                                 *   is set to CHAR_MAX if not set */
	mpc_common_spinlock_t        lock;              /**< Lock protecting the endpoint */
};

/**
 * @brief Lock and endpoint
 */
#define _MPC_LOWCOMM_ENDPOINT_LOCK(r)       mpc_common_spinlock_lock(&(r)->lock)

/**
 * @brief UnLock and endpoint
 */
#define _MPC_LOWCOMM_ENDPOINT_UNLOCK(r)     mpc_common_spinlock_unlock(&(r)->lock)

/**
 * @brief TryLock and endpoint
 */
#define _MPC_LOWCOMM_ENDPOINT_TRYLOCK(r)    mpc_common_spinlock_trylock(&(r)->lock)

/**
 * @brief Initialize and endpoint
 *
 * @param edp pointer to the endpoint to initialize
 * @param dest destination in comm_world of this endpoint
 * @param rail rail the endpoint belongs to
 * @param type type of the endpoint
 */
void _mpc_lowcomm_endpoint_init(_mpc_lowcomm_endpoint_t *edp,
                                mpc_lowcomm_peer_uid_t dest,
                                struct sctk_rail_info_s *rail,
                                _mpc_lowcomm_endpoint_type_t type);

/**
 * @brief Return true if the process is the one who initiated route connect
 *
 * @param edp endpoint
 * @return char 0 if not initator 1 otherwise
 */
char _mpc_lowcomm_endpoint_is_initiator(_mpc_lowcomm_endpoint_t *edp);


/**
 * @brief Set the state of a given endpoint
 *
 * @param edp endpoint to set the state of
 * @param state state to be set
 */
static inline void _mpc_lowcomm_endpoint_set_state(_mpc_lowcomm_endpoint_t *edp, _mpc_lowcomm_endpoint_state_t state)
{
	OPA_store_int(&edp->state, state);
}

/**
 * @brief Get the state of a given endpoint
 *
 * @param edp endpoint to get the state of
 * @return _mpc_lowcomm_endpoint_state_t state of the endpoint
 */
static inline _mpc_lowcomm_endpoint_state_t _mpc_lowcomm_endpoint_get_state(_mpc_lowcomm_endpoint_t *edp)
{
	if(!edp)
	{
		return _MPC_LOWCOMM_ENDPOINT_DECONNECTED;
	}

	return ( _mpc_lowcomm_endpoint_state_t )OPA_load_int(&edp->state);
}

/**
 * @brief Get endpoint type (STATIC or Dynamic)
 *
 * @param edp endpoint to get type of
 * @return _mpc_lowcomm_endpoint_type_t type of the given endpoint
 */
static inline _mpc_lowcomm_endpoint_type_t _mpc_lowcomm_endpoint_get_type(_mpc_lowcomm_endpoint_t *tmp)
{
	return tmp->type;
}

/************************************************************************/
/* Endpoint Table                                                          */
/************************************************************************/

/**
 * @brief This table is used to store routes inside the rails
 *
 */
struct _mpc_lowcomm_endpoint_table_s
{
	/* Dynamic Endpoints */
	struct mpc_common_hashtable dynamic_route_table;      /** Here are stored the dynamic routes (hash table) */
	mpc_common_rwlock_t         dynamic_route_table_lock; /** This is the dynamic route lock */
	/* Static Endpoints */
	struct mpc_common_hashtable static_route_table;       /** Here are stored static routes (hash table)
	                                                       *  -- no lock as they are read only */
};

/**
 * @brief Create a new endpoint table (to be destroyed later on)
 *
 * @return _mpc_lowcomm_endpoint_table_t* newly allocated endpoint table
 */
_mpc_lowcomm_endpoint_table_t *_mpc_lowcomm_endpoint_table_new();

/**
 * @brief Check if an endpoint table is empty
 *
 * @param table table to check for emptyness
 * @return int 1 if table is empty
 */
int _mpc_lowcomm_endpoint_table_has_routes(_mpc_lowcomm_endpoint_table_t *table);

/**
 * @brief Free endpoint table
 *
 * @param table endpoint table to be freed
 */
void _mpc_lowcomm_endpoint_table_free(_mpc_lowcomm_endpoint_table_t **table);

/**
 * @brief Clear the endpoint table (same as free + new)
 *
 * @param table endpoint table to be cleared
 */
void _mpc_lowcomm_endpoint_table_clear(_mpc_lowcomm_endpoint_table_t **table);

/* Functions for adding a route */

/**
 * @brief Add a static endpoint to the table
 *
 * @param table the endpoint table to add to
 * @param edp the new endpoint
 */
void _mpc_lowcomm_endpoint_table_add_static_route(_mpc_lowcomm_endpoint_table_t *table,
                                                  _mpc_lowcomm_endpoint_t *edp);

/**
 * @brief Add a dynamic endpoint to the table
 *
 * @param table the endpoint table to add to
 * @param edp the new endpoint
 */
void _mpc_lowcomm_endpoint_table_add_dynamic_route(_mpc_lowcomm_endpoint_table_t *table,
                                                   _mpc_lowcomm_endpoint_t *edp);

/**
 * @brief Add a dynamic endpoint to the table (version without lock)
 *
 * @param table the endpoint table to add to
 * @param edp the new endpoint
 */
void _mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock(_mpc_lowcomm_endpoint_table_t *table,
                                                           _mpc_lowcomm_endpoint_t *edp);

/* Functions for getting a route */
_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_static_route(_mpc_lowcomm_endpoint_table_t *table, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_dynamic_route(_mpc_lowcomm_endpoint_table_t *table, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *_mpc_lowcomm_endpoint_table_get_dynamic_route_no_lock(_mpc_lowcomm_endpoint_table_t *table, mpc_lowcomm_peer_uid_t dest);

/**
 * @brief Call a function on all dynamic endpoints
 *
 * @param table endpoint table to walk
 * @param func function to be called on each endpoint
 * @param arg argument to be passed to the called function
 */
void _mpc_lowcomm_endpoint_table_walk_dynamic(_mpc_lowcomm_endpoint_table_t *table,
                                              void (*func)(_mpc_lowcomm_endpoint_t *endpoint, void *arg),
                                              void *arg);

/**
 * @brief Call a function on all static endpoints
 *
 * @param table endpoint table to walk
 * @param func function to be called on each endpoint
 * @param arg argument to be passed to the called function
 */
void _mpc_lowcomm_endpoint_table_walk_static(_mpc_lowcomm_endpoint_table_t *table,
                                             void (*func)(_mpc_lowcomm_endpoint_t *endpoint, void *arg),
                                             void *arg);

/**
 * @brief Call a function on all endpoints
 *
 * @param table endpoint table to walk
 * @param func function to be called on each endpoint
 * @param arg argument to be passed to the called function
 */
static inline void _mpc_lowcomm_endpoint_table_walk(_mpc_lowcomm_endpoint_table_t *table,
                                                    void (*func)(_mpc_lowcomm_endpoint_t *endpoint, void *arg),
                                                    void *arg)
{
	_mpc_lowcomm_endpoint_table_walk_static(table, func, arg);
	_mpc_lowcomm_endpoint_table_walk_dynamic(table, func, arg);
}

#endif /* _MPC_LOWCOMM_ENDPOINT_H_ */
