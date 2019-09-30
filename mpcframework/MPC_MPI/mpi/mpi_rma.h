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
#ifndef MPC_MPI_RMA_H
#define MPC_MPI_RMA_H

#include "mpc_mpi.h"
#include "mpc_common_asm.h"
#include "sctk_buffered_fifo.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_types.h"
#include "sctk_window.h"

#include "mpi_rma_win.h"

/************************************************************************/
/* INTERNAL MPI_Get                                                     */
/************************************************************************/

int mpc_MPI_Get(void *origin_addr, int origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, int target_count,
                MPI_Datatype target_data_type, MPI_Win win);

int mpc_MPI_Rget(void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_data_type, MPI_Win win,
                 MPI_Request *request);

/************************************************************************/
/* INTERNAL MPI_Put                                                     */
/************************************************************************/

int mpc_MPI_Put(const void *origin_addr, int origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, int target_count,
                MPI_Datatype target_datatype, MPI_Win win);

int mpc_MPI_Rput(const void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_datatype, MPI_Win win,
                 MPI_Request *request);

/************************************************************************/
/* INTERNAL MPI_Accumulate                                              */
/************************************************************************/

int mpc_MPI_Accumulate(const void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);

int mpc_MPI_Raccumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, int target_rank,
                        MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                        MPI_Request *request);

/************************************************************************/
/* INTERNAL MPI_Get_accumulate                                          */
/************************************************************************/

int mpc_MPI_Get_accumulate(const void *origin_addr, int origin_count,
                           MPI_Datatype origin_datatype, void *result_addr,
                           int result_count, MPI_Datatype result_datatype,
                           int target_rank, MPI_Aint target_disp,
                           int target_count, MPI_Datatype target_datatype,
                           MPI_Op op, MPI_Win win);

int mpc_MPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr,
                         int result_count, MPI_Datatype result_datatype,
                         int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win, MPI_Request *request);

/************************************************************************/
/* INTERNAL MPI_Fetch_and_op                                            */
/************************************************************************/

int mpc_MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                         MPI_Datatype datatype, int target_rank,
                         MPI_Aint target_disp, MPI_Op op, MPI_Win win);

/************************************************************************/
/* INTERNAL MPI_Compare_and_swap                                        */
/************************************************************************/

int mpc_MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                             void *result_addr, MPI_Datatype datatype,
                             int target_rank, MPI_Aint target_disp,
                             MPI_Win win);

#endif /* MPC_MPI_RMA_H */
