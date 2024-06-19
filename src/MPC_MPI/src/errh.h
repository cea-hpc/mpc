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
#ifndef MPC_MPI_ERROR_H
#define MPC_MPI_ERROR_H


#include <stdio.h>
#include <stdbool.h>

/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

#define MAX_ERROR_HANDLERS    32 /**< 4 predefined + 8 user */

typedef void (*_mpc_mpi_generic_errhandler_func_t)(void *phandle, int *error_code, ...);
typedef struct MPI_ABI_Errhandler
{
	_mpc_mpi_generic_errhandler_func_t eh;
	unsigned                           ref_count;
} *_mpc_mpi_errhandler_t;

/** @brief Retrieves the errhandler at the given index
 *
 *  @param idx  The index of the errhandler to retrieve
 *  @return     A valid errhandler if the index is correct, the entry otherwise
 */
_mpc_mpi_errhandler_t _mpc_mpi_errhandler_from_idx(const long idx);

/** @brief Retrieves the index of a given errhandler
 *
 *  @param errh  The errhandler of which we want the index in the global table
 *  @return      A valid index
 */
long _mpc_mpi_errhandler_to_idx(const _mpc_mpi_errhandler_t errh);

/** @brief Checks if an errhandler is valid (registered)
 *
 *  @param errh    The errhandler to check
 *  @return        true if the errhandler is valid, false otherwise
 */
bool _mpc_mpi_errhandler_is_valid(const _mpc_mpi_errhandler_t errh);

int _mpc_mpi_errhandler_register(_mpc_mpi_generic_errhandler_func_t eh, _mpc_mpi_errhandler_t *errh);

int _mpc_mpi_errhandler_register_on_slot(_mpc_mpi_generic_errhandler_func_t eh,
                                         const int slot);

_mpc_mpi_generic_errhandler_func_t _mpc_mpi_errhandler_resolve(_mpc_mpi_errhandler_t errh);

int _mpc_mpi_errhandler_free(_mpc_mpi_errhandler_t errh);

/************************************************************************/
/* MPI Handles                                                          */
/************************************************************************/

#define SCTK_BOOKED_HANDLES    (10000)

typedef size_t sctk_handle;

struct _mpc_mpi_handle_ctx_s
{
	sctk_handle           id;
	_mpc_mpi_errhandler_t handler;
};

typedef enum
{
	_MPC_MPI_HANDLE_GLOBAL,
	_MPC_MPI_HANDLE_COMM,
	_MPC_MPI_HANDLE_DATATYPE,
	_MPC_MPI_HANDLE_WIN,
	_MPC_MPI_HANDLE_FILE,
	_MPC_MPI_HANDLE_ERRHANDLER,
	_MPC_MPI_HANDLE_OP,
	_MPC_MPI_HANDLE_GROUP,
	_MPC_MPI_HANDLE_REQUEST,
	_MPC_MPI_HANDLE_MESSAGE,
	_MPC_MPI_HANDLE_INFO,
	_MPC_MPI_HANDLE_SESSION
} sctk_handle_type;

void mpc_mpi_err_init();

struct _mpc_mpi_handle_ctx_s *_mpc_mpi_handle_ctx_new(sctk_handle id);

int _mpc_mpi_handle_ctx_release(struct _mpc_mpi_handle_ctx_s *hctx);

sctk_handle _mpc_mpi_handle_new_from_id(sctk_handle previous_id, sctk_handle_type type);

sctk_handle _mpc_mpi_handle_new(sctk_handle_type type);

int _mpc_mpi_handle_free(sctk_handle id, sctk_handle_type type);


_mpc_mpi_errhandler_t _mpc_mpi_handle_get_errhandler(sctk_handle id,
                                                     sctk_handle_type type);

int _mpc_mpi_handle_set_errhandler(sctk_handle id, sctk_handle_type type,
                                   _mpc_mpi_errhandler_t errh);

#endif /* MPC_MPI_ERROR_H */
