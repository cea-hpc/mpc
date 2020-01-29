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

#include <mpc_common_datastructure.h>

/**
 * @addtogroup _mpc_ht_cell_
 * @{
 */

/**
 * @brief Allocate a new internal cell for the MPC HT
 *
 * @param key The key of this cell
 * @param data The pointer to be stored
 * @param next The pointer to the next cell to the chained
 * @return struct _mpc_ht_cell*  New cell
 */
struct _mpc_ht_cell * _mpc_ht_cell_new( uint64_t key, void * data, struct _mpc_ht_cell * next );

/**
 * @brief Intialize the content of a cell
 *
 * @param cell The cell to be initialized
 * @param key The key of this cell
 * @param data The pointer to be stored
 * @param next The pointer to the next cell to the chained
 */
void _mpc_ht_cell_init( struct _mpc_ht_cell * cell , uint64_t key, void * data, struct _mpc_ht_cell * next );

/**
 * @brief Release an internal hash-table cell
 *
 * @param cell The cell to be released
 */
void _mpc_ht_cell_release( struct _mpc_ht_cell * cell );

/**
 * @brief Get a cell from an internal HT cell
 *
 * @param cell The cell-list to be searched
 * @param key the key to be searched
 * @return struct _mpc_ht_cell* the resulting cell (or NULL if not found)
 */
struct _mpc_ht_cell * _mpc_ht_cell_get( struct _mpc_ht_cell * cell, uint64_t key );

/**
 * @brief Remove a cell from an HT list
 *
 * @param head head of the cell-list
 * @param key the key to be removed
 * @return struct _mpc_ht_cell* the removed cell
 */
struct _mpc_ht_cell * _mpc_ht_cell_pop( struct _mpc_ht_cell * head, uint64_t key );

/**
 * @}
 */

#endif /* SCTK_HASH_TABLE */
