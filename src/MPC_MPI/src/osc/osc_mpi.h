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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPI_WIN_H 
#define MPI_WIN_H 

#include <mpc_mpi.h>
#include <mpc_mpi_comm_lib.h>
#include <mpc_lowcomm_communicator.h> //NOTE: for lowcomm_communicator_t
#include <lcp.h>

#include "osc_module.h"

#define MAX_WIN_NAME 64

typedef struct MPI_ABI_Win {
        int comm_size;
        int comm_rank;
        mpc_lowcomm_communicator_t comm;
        mpc_lowcomm_group_t *group;
        MPI_Info info;

        char win_name[MAX_WIN_NAME];

        int flavor; 
        int model;
        size_t size;

        mpc_osc_module_t win_module;
} mpc_win_t;

int mpc_win_create(void *base, size_t size, 
                   int disp_unit, MPI_Info info,
                   mpc_lowcomm_communicator_t comm, 
                   mpc_win_t **win_p);
int mpc_win_allocate(void *base, size_t size, 
                     int disp_unit, MPI_Info info,
                     mpc_lowcomm_communicator_t comm, 
                     mpc_win_t **win_p);

int mpc_win_free(mpc_win_t *win);

int mpc_osc_win_attach(mpc_win_t *win, void *base, size_t len);
int mpc_osc_win_detach(mpc_win_t *win, const void *base);
int mpc_osc_free(mpc_win_t *win);

/* Communication. */
int mpc_osc_put(const void *origin_addr, int origin_count,
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win);
int mpc_osc_get(void *origin_addr, int origin_count, 
                _mpc_lowcomm_general_datatype_t *origin_dt,
                int target, ptrdiff_t target_disp, int target_count,
                _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win);
int mpc_osc_accumulate(const void *origin_addr, int origin_count, 
                       _mpc_lowcomm_general_datatype_t *origin_dt,
                       int target, ptrdiff_t target_disp, int target_count,
                       _mpc_lowcomm_general_datatype_t *target_dt,
                       MPI_Op op, mpc_win_t *win);
int mpc_osc_compare_and_swap(const void *origin_addr, const void *compare_addr,
                             void *result_addr, _mpc_lowcomm_general_datatype_t *dt,
                             int target, ptrdiff_t target_disp,
                             mpc_win_t *win);        
int mpc_osc_fetch_and_op(const void *origin_addr, void *result_addr,
                         _mpc_lowcomm_general_datatype_t *dt, int target,
                         ptrdiff_t target_disp, MPI_Op op,
                         mpc_win_t *win);        
int mpc_osc_get_accumulate(const void *origin_addr, int origin_count, 
                           _mpc_lowcomm_general_datatype_t *origin_datatype,
                           void *result_addr, int result_count, 
                           _mpc_lowcomm_general_datatype_t *result_datatype,
                           int target_rank, ptrdiff_t target_disp,
                           int target_count, _mpc_lowcomm_general_datatype_t *target_datatype,
                           MPI_Op op, mpc_win_t *win);
int mpc_osc_rput(const void *origin_addr, int origin_count, 
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target, ptrdiff_t target_disp, int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt,
                 mpc_win_t *win, MPI_Request *req);
int mpc_osc_rget(void *origin_addr, int origin_count, 
                 _mpc_lowcomm_general_datatype_t *origin_dt,
                 int target, ptrdiff_t target_disp, int target_count,
                 _mpc_lowcomm_general_datatype_t *target_dt, mpc_win_t *win,
                 MPI_Request *req);
int mpc_osc_raccumulate(const void *origin_addr, int origin_count, 
                        _mpc_lowcomm_general_datatype_t *origin_dt,
                        int target, ptrdiff_t target_disp, int target_count,
                        _mpc_lowcomm_general_datatype_t *target_dt, MPI_Op op,
                        mpc_win_t *win, MPI_Request *request);
int mpc_osc_rget_accumulate(const void *origin_addr, int origin_count, 
                                _mpc_lowcomm_general_datatype_t *origin_datatype,
                                void *result_addr, int result_count, 
                                _mpc_lowcomm_general_datatype_t *result_datatype,
                                int target_rank, ptrdiff_t target_disp, int target_count,
                                _mpc_lowcomm_general_datatype_t *target_datatype,
                                MPI_Op op, mpc_win_t *win,
                                MPI_Request *request);

/* Passive target. */
int mpc_osc_lock(int lock_type, int target, int mpi_assert, mpc_win_t *win);
int mpc_osc_unlock(int target, mpc_win_t *win);
int mpc_osc_lock_all(int mpi_assert, mpc_win_t *win);
int mpc_osc_unlock_all(mpc_win_t *win);
int mpc_osc_sync(mpc_win_t *win);
int mpc_osc_flush(int target, mpc_win_t *win);
int mpc_osc_flush_all(mpc_win_t *win);
int mpc_osc_flush_local(int target, mpc_win_t *win);
int mpc_osc_flush_local_all(mpc_win_t *win);

/* Active target. */
int mpc_osc_fence(int mpi_assert, mpc_win_t *win);
int mpc_osc_start(mpc_lowcomm_group_t *group, int mpi_assert, mpc_win_t *win);
int mpc_osc_complete(mpc_win_t *win);
int mpc_osc_post(mpc_lowcomm_group_t *group, int mpi_assert, mpc_win_t *win);
int mpc_osc_wait(mpc_win_t *win);
int mpc_osc_test(mpc_win_t *win, int *flag);

int osc_module_fini();
#endif
