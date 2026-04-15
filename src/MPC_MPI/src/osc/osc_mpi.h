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
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_MPI_OSC_H
#define MPC_MPI_OSC_H

#include <mpc_mpi.h>                  // For MPI_MAX_OBJECT_NAME, MPI_Info...
#include "mpc_mpi_internal.h"         // For MPI_internal_request_t

typedef struct mpc_mpi_win_s mpc_mpi_win_t;

int mpc_mpi_osc_win_create(void **base, size_t size, int disp_unit, MPI_Info info,
                           mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p);
int mpc_mpi_osc_win_allocate(void **base, size_t size, int disp_unit, MPI_Info info,
                             mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p);
int mpc_mpi_osc_win_create_dynamic(MPI_Info info, mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p);

int mpc_mpi_osc_win_free(mpc_mpi_win_t *win);

int mpc_mpi_osc_win_attach(mpc_mpi_win_t *win, void *base, size_t len);
int mpc_mpi_osc_win_detach(mpc_mpi_win_t *win, const void *base);

/* Communication. */
int mpc_mpi_osc_put(const void *origin_addr, int origin_count,
                    _mpc_lowcomm_general_datatype_t *origin_dt,
                    int target, ptrdiff_t target_disp, int target_count,
                    _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win);
int mpc_mpi_osc_get(void *origin_addr, int origin_count,
                    _mpc_lowcomm_general_datatype_t *origin_dt,
                    int target, ptrdiff_t target_disp, int target_count,
                    _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win);
int mpc_mpi_osc_accumulate(const void *origin_addr, int origin_count,
                           _mpc_lowcomm_general_datatype_t *origin_dt,
                           int target, ptrdiff_t target_disp, int target_count,
                           _mpc_lowcomm_general_datatype_t *target_dt,
                           MPI_Op op, mpc_mpi_win_t *win);
int mpc_mpi_osc_compare_and_swap(const void *origin_addr, const void *compare_addr,
                                 void *result_addr, _mpc_lowcomm_general_datatype_t *dt,
                                 int target, ptrdiff_t target_disp,
                                 mpc_mpi_win_t *win);
int mpc_mpi_osc_fetch_and_op(const void *origin_addr, void *result_addr,
                             _mpc_lowcomm_general_datatype_t *dt, int target,
                             ptrdiff_t target_disp, MPI_Op op,
                             mpc_mpi_win_t *win);
int mpc_mpi_osc_get_accumulate(const void *origin_addr, int origin_count,
                               _mpc_lowcomm_general_datatype_t *origin_datatype,
                               void *result_addr, int result_count,
                               _mpc_lowcomm_general_datatype_t *result_datatype,
                               int target_rank, ptrdiff_t target_disp,
                               int target_count, _mpc_lowcomm_general_datatype_t *target_datatype,
                               MPI_Op op, mpc_mpi_win_t *win);
int mpc_mpi_osc_rput(const void *origin_addr, int origin_count,
                     _mpc_lowcomm_general_datatype_t *origin_dt,
                     int target, ptrdiff_t target_disp, int target_count,
                     _mpc_lowcomm_general_datatype_t *target_dt,
                     mpc_mpi_win_t *win, MPI_internal_request_t **req);
int mpc_mpi_osc_rget(void *origin_addr, int origin_count,
                     _mpc_lowcomm_general_datatype_t *origin_dt,
                     int target, ptrdiff_t target_disp, int target_count,
                     _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win,
                     MPI_internal_request_t **req);
int mpc_mpi_osc_raccumulate(const void *origin_addr, int origin_count,
                            _mpc_lowcomm_general_datatype_t *origin_dt,
                            int target, ptrdiff_t target_disp, int target_count,
                            _mpc_lowcomm_general_datatype_t *target_dt, MPI_Op op,
                            mpc_mpi_win_t *win, MPI_internal_request_t **request);
int mpc_mpi_osc_rget_accumulate(const void *origin_addr, int origin_count,
                                _mpc_lowcomm_general_datatype_t *origin_datatype,
                                void *result_addr, int result_count,
                                _mpc_lowcomm_general_datatype_t *result_datatype,
                                int target_rank, ptrdiff_t target_disp, int target_count,
                                _mpc_lowcomm_general_datatype_t *target_datatype,
                                MPI_Op op, mpc_mpi_win_t *win,
                                MPI_internal_request_t **request);

/* Passive target. */
int mpc_mpi_osc_lock(int lock_type, int target, int mpi_assert, mpc_mpi_win_t *win);
int mpc_mpi_osc_unlock(int target, mpc_mpi_win_t *win);
int mpc_mpi_osc_lock_all(int mpi_assert, mpc_mpi_win_t *win);
int mpc_mpi_osc_unlock_all(mpc_mpi_win_t *win);
int mpc_mpi_osc_sync(mpc_mpi_win_t *win);
int mpc_mpi_osc_flush(int target, mpc_mpi_win_t *win);
int mpc_mpi_osc_flush_all(mpc_mpi_win_t *win);

/* Active target. */
int mpc_mpi_osc_fence(int mpi_assert, mpc_mpi_win_t *win);
int mpc_mpi_osc_start(mpc_lowcomm_group_t *group, int mpi_assert, mpc_mpi_win_t *win);
int mpc_mpi_osc_complete(mpc_mpi_win_t *win);
int mpc_mpi_osc_post(mpc_lowcomm_group_t *group, int mpi_assert, mpc_mpi_win_t *win);
int mpc_mpi_osc_wait(mpc_mpi_win_t *win);
int mpc_mpi_osc_test(mpc_mpi_win_t *win, int *flag);


// Attributes
struct mpc_mpi_osc_win_keyval * mpc_mpi_osc_win_keyval_init(MPI_Win_copy_attr_function *copy_fn,
                                                            MPI_Win_delete_attr_function *delete_fn,
                                                            void *extra_state);
struct mpc_mpi_osc_win_attr * mpc_mpi_osc_win_attr_init(int keyval,
                                                        void *value,
                                                        struct mpc_mpi_osc_win_keyval *keyval_pl,
                                                        mpc_mpi_win_t *win);
int mpc_mpi_osc_win_set_attr(mpc_mpi_win_t *win, int keyval, void *attr_val);
int mpc_mpi_osc_win_get_attr(mpc_mpi_win_t *win, int keyval, void *attr_val, int *flag);
int mpc_mpi_osc_win_delete_attr(mpc_mpi_win_t *win, int keyval);
int mpc_mpi_osc_win_create_keyval(MPI_Win_copy_attr_function *copy_fn, MPI_Win_delete_attr_function *delete_fn,
                                  int *keyval, void *extra_state);
int mpc_mpi_osc_win_free_keyval(const int keyval);


/* Special */
MPI_Comm mpc_mpi_osc_win_get_comm(const mpc_mpi_win_t *const win);
MPI_Group mpc_mpi_osc_win_get_group(const mpc_mpi_win_t *const win);

#endif // MPC_MPI_OSC_H
