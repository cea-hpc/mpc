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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

/* Syntax : PROBE( UNIQUE_KEY, PARENT_KEY, "Short Desc", MPI_T_VAR_HITS,
 * MPI_T_VAR_VALUE, MPI_T_VAR_LOWW, MPI_T_VAR_HIGHW )
 * use NO_PARENT if a given probe has no paren */

PROBE(MPI, NO_PARENT, MPI Interface, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

#include "sctk_profiler_keys_mpi.h"
#include "sctk_profiler_keys_mpi_nbc.h"

PROBE(MPC, NO_PARENT, MPC Interface, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(MPC_POINT_TO_POINT, MPC, MPC point to point, MPC_POINT_TO_POINT_CALL,
      MPC_POINT_TO_POINT_TIME, MPC_POINT_TO_POINT_TIME_HW,
      MPC_POINT_TO_POINT_TIME_LW)
PROBE(MPC_Iprobe, MPC_POINT_TO_POINT, MPC_Iprobe, MPC_Iprobe_CALL,
      MPC_Iprobe_TIME, MPC_Iprobe_TIME_HW, MPC_Iprobe_TIME_LW)
PROBE(MPC_Probe, MPC_POINT_TO_POINT, MPC_Probe, MPC_Probe_CALL, MPC_Probe_TIME,
      MPC_Probe_TIME_HW, MPC_Probe_TIME_LW)
PROBE(MPC_Irecv, MPC_POINT_TO_POINT, MPC_Irecv, MPC_Irecv_CALL, MPC_Irecv_TIME,
      MPC_Irecv_TIME_HW, MPC_Irecv_TIME_LW)
PROBE(MPC_Irecv_pack, MPC_POINT_TO_POINT, MPC_Irecv_pack, MPC_Irecv_pack_CALL,
      MPC_Irecv_pack_TIME, MPC_Irecv_pack_TIME_HW, MPC_Irecv_pack_TIME_LW)
PROBE(MPC_Irsend, MPC_POINT_TO_POINT, MPC_Irsend, MPC_Irsend_CALL,
      MPC_Irsend_TIME, MPC_Irsend_TIME_HW, MPC_Irsend_TIME_LW)
PROBE(MPC_Isend, MPC_POINT_TO_POINT, MPC_Isend, MPC_Isend_CALL, MPC_Isend_TIME,
      MPC_Isend_TIME_HW, MPC_Isend_TIME_LW)
PROBE(MPC_Isend_pack, MPC_POINT_TO_POINT, MPC_Isend_pack, MPC_Isend_pack_CALL,
      MPC_Isend_pack_TIME, MPC_Isend_pack_TIME_HW, MPC_Isend_pack_TIME_LW)
PROBE(MPC_Issend, MPC_POINT_TO_POINT, MPC_Issend, MPC_Issend_CALL,
      MPC_Issend_TIME, MPC_Issend_TIME_HW, MPC_Issend_TIME_LW)
PROBE(MPC_Recv, MPC_POINT_TO_POINT, MPC_Recv, MPC_Recv_CALL, MPC_Recv_TIME,
      MPC_Recv_TIME_HW, MPC_Recv_TIME_LW)
PROBE(MPC_Rsend, MPC_POINT_TO_POINT, MPC_Rsend, MPC_Rsend_CALL, MPC_Rsend_TIME,
      MPC_Rsend_TIME_HW, MPC_Rsend_TIME_LW)
PROBE(MPC_Send, MPC_POINT_TO_POINT, MPC_Send, MPC_Send_CALL, MPC_Send_TIME,
      MPC_Send_TIME_HW, MPC_Send_TIME_LW)
PROBE(MPC_Sendrecv, MPC_POINT_TO_POINT, MPC_Sendrecv, MPC_Sendrecv_CALL,
      MPC_Sendrecv_TIME, MPC_Sendrecv_TIME_HW, MPC_Sendrecv_TIME_LW)
PROBE(MPC_Ssend, MPC_POINT_TO_POINT, MPC_Ssend, MPC_Ssend_CALL, MPC_Ssend_TIME,
      MPC_Ssend_TIME_HW, MPC_Ssend_TIME_LW)
PROBE(MPC_Ibsend, MPC_POINT_TO_POINT, MPC_Ibsend, MPC_Ibsend_CALL,
      MPC_Ibsend_TIME, MPC_Ibsend_TIME_HW, MPC_Ibsend_TIME_LW)
PROBE(MPC_Test, MPC_POINT_TO_POINT, MPC_Test, MPC_Test_CALL, MPC_Test_TIME,
      MPC_Test_TIME_HW, MPC_Test_TIME_LW)
PROBE(MPC_Test_no_check, MPC_POINT_TO_POINT, MPC_Test_no_check,
      MPC_Test_no_check_CALL, MPC_Test_no_check_TIME, MPC_Test_no_check_TIME_HW,
      MPC_Test_no_check_TIME_LW)
PROBE(MPC_Test_check, MPC_POINT_TO_POINT, MPC_Test_check, MPC_Test_check_CALL,
      MPC_Test_check_TIME, MPC_Test_check_TIME_HW, MPC_Test_check_TIME_LW)
PROBE(MPC_Proceed, MPC_POINT_TO_POINT, MPC_Proceed, MPC_Proceed_CALL,
      MPC_Proceed_TIME, MPC_Proceed_TIME_HW, MPC_Proceed_TIME_LW)
PROBE(MPC_WAIT, MPC, MPC Waiting, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(MPC_Wait, MPC_WAIT, MPC_Wait, MPC_Wait_CALL, MPC_Wait_TIME,
      MPC_Wait_TIME_HW, MPC_Wait_TIME_LW)
PROBE(MPC_Wait_pending, MPC_WAIT, MPC_Wait_pending, MPC_Wait_pending_CALL,
      MPC_Wait_pending_TIME, MPC_Wait_pending_TIME_HW, MPC_Wait_pending_TIME_LW)
PROBE(MPC_Wait_pending_all_comm, MPC_WAIT, MPC_Wait_pending_all_comm,
      MPC_Wait_pending_all_comm_CALL, MPC_Wait_pending_all_comm_TIME,
      MPC_Wait_pending_all_comm_TIME_HW, MPC_Wait_pending_all_comm_TIME_LW)
PROBE(MPC_Waitall, MPC_WAIT, MPC_Waitall, MPC_Waitall_CALL, MPC_Waitall_TIME,
      MPC_Waitall_TIME_HW, MPC_Waitall_TIME_LW)
PROBE(MPC_Waitany, MPC_WAIT, MPC_Waitany, MPC_Waitany_CALL, MPC_Waitany_TIME,
      MPC_Waitany_TIME_HW, MPC_Waitany_TIME_LW)
PROBE(MPC_Waitsome, MPC_WAIT, MPC_Waitsome, MPC_Waitsome_CALL,
      MPC_Waitsome_TIME, MPC_Waitsome_TIME_HW, MPC_Waitsome_TIME_LW)
PROBE(MPC_COLL, MPC, MPC collectives, MPC_COLL_CALL, MPC_COLL_TIME,
      MPC_COLL_TIME_HW, MPC_COLL_TIME_LW)
PROBE(MPC_Allgather, MPC_COLL, MPC_Allgather, MPC_Allgather_CALL,
      MPC_Allgather_TIME, MPC_Allgather_TIME_HW, MPC_Allgather_TIME_LW)
PROBE(MPC_Allgatherv, MPC_COLL, MPC_Allgatherv, MPC_Allgatherv_CALL,
      MPC_Allgatherv_TIME, MPC_Allgatherv_TIME_HW, MPC_Allgatherv_TIME_LW)
PROBE(MPC_Allreduce, MPC_COLL, MPC_Allreduce, MPC_Allreduce_CALL,
      MPC_Allreduce_TIME, MPC_Allreduce_TIME_HW, MPC_Allreduce_TIME_LW)
PROBE(MPC_Alltoall, MPC_COLL, MPC_Alltoall, MPC_Alltoall_CALL,
      MPC_Alltoall_TIME, MPC_Alltoall_TIME_HW, MPC_Alltoall_TIME_LW)
PROBE(MPC_Alltoallv, MPC_COLL, MPC_Alltoallv, MPC_Alltoallv_CALL,
      MPC_Alltoallv_TIME, MPC_Alltoallv_TIME_HW, MPC_Alltoallv_TIME_LW)
PROBE(MPC_Barrier, MPC_COLL, MPC_Barrier, MPC_Barrier_CALL, MPC_Barrier_TIME,
      MPC_Barrier_TIME_HW, MPC_Barrier_TIME_LW)
PROBE(MPC_Bcast, MPC_COLL, MPC_Bcast, MPC_Bcast_CALL, MPC_Bcast_TIME,
      MPC_Bcast_TIME_HW, MPC_Bcast_TIME_LW)
PROBE(MPC_Bsend, MPC_COLL, MPC_Bsend, MPC_Bsend_CALL, MPC_Bsend_TIME,
      MPC_Bsend_TIME_HW, MPC_Bsend_TIME_LW)
PROBE(MPC_Gather, MPC_COLL, MPC_Gather, MPC_Gather_CALL, MPC_Gather_TIME,
      MPC_Gather_TIME_HW, MPC_Gather_TIME_LW)
PROBE(MPC_Gatherv, MPC_COLL, MPC_Gatherv, MPC_Gatherv_CALL, MPC_Gatherv_TIME,
      MPC_Gatherv_TIME_HW, MPC_Gatherv_TIME_LW)
PROBE(MPC_Reduce, MPC_COLL, MPC_Reduce, MPC_Reduce_CALL, MPC_Reduce_TIME,
      MPC_Reduce_TIME_HW, MPC_Reduce_TIME_LW)
PROBE(MPC_Scatter, MPC_COLL, MPC_Scatter, MPC_Scatter_CALL, MPC_Scatter_TIME,
      MPC_Scatter_TIME_HW, MPC_Scatter_TIME_LW)
PROBE(MPC_Scatterv, MPC_COLL, MPC_Scatterv, MPC_Scatterv_CALL,
      MPC_Scatterv_TIME, MPC_Scatterv_TIME_HW, MPC_Scatterv_TIME_LW)
PROBE(MPC_COMMUNICATORS, MPC, MPC Communicator, MPC_COMMUNICATORS_CALL,
      MPC_COMMUNICATORS_TIME, MPC_COMMUNICATORS_TIME_HW,
      MPC_COMMUNICATORS_TIME_LW)
PROBE(MPC_Comm_create, MPC_COMMUNICATORS, MPC_Comm_create, MPC_Comm_create_CALL,
      MPC_Comm_create_TIME, MPC_Comm_create_TIME_HW, MPC_Comm_create_TIME_LW)
PROBE(MPC_Comm_create_list, MPC_COMMUNICATORS, MPC_Comm_create_list,
      MPC_Comm_create_list_CALL, MPC_Comm_create_list_TIME,
      MPC_Comm_create_list_TIME_HW, MPC_Comm_create_list_TIME_LW)
PROBE(MPC_Comm_dup, MPC_COMMUNICATORS, MPC_Comm_dup, MPC_Comm_dup_CALL,
      MPC_Comm_dup_TIME, MPC_Comm_dup_TIME_HW, MPC_Comm_dup_TIME_LW)
PROBE(MPC_Comm_free, MPC_COMMUNICATORS, MPC_Comm_free, MPC_Comm_free_CALL,
      MPC_Comm_free_TIME, MPC_Comm_free_TIME_HW, MPC_Comm_free_TIME_LW)
PROBE(MPC_Comm_split, MPC_COMMUNICATORS, MPC_Comm_split, MPC_Comm_split_CALL,
      MPC_Comm_split_TIME, MPC_Comm_split_TIME_HW, MPC_Comm_split_TIME_LW)
PROBE(MPC_GROUPS, MPC, MPC Group, MPC_GROUPS_CALL, MPC_GROUPS_TIME,
      MPC_GROUPS_TIME_HW, MPC_GROUPS_TIME_LW)
PROBE(MPC_Comm_group, MPC_GROUPS, MPC_Comm_group, MPC_Comm_group_CALL,
      MPC_Comm_group_TIME, MPC_Comm_group_TIME_HW, MPC_Comm_group_TIME_LW)
PROBE(MPC_Group_difference, MPC_GROUPS, MPC_Group_difference,
      MPC_Group_difference_CALL, MPC_Group_difference_TIME,
      MPC_Group_difference_TIME_HW, MPC_Group_difference_TIME_LW)
PROBE(MPC_Group_free, MPC_GROUPS, MPC_Group_free, MPC_Group_free_CALL,
      MPC_Group_free_TIME, MPC_Group_free_TIME_HW, MPC_Group_free_TIME_LW)
PROBE(MPC_Group_incl, MPC_GROUPS, MPC_Group_incl, MPC_Group_incl_CALL,
      MPC_Group_incl_TIME, MPC_Group_incl_TIME_HW, MPC_Group_incl_TIME_LW)
PROBE(MPC_PACK, MPC, MPC Group, MPC_PACK_CALL, MPC_PACK_TIME, MPC_PACK_TIME_HW,
      MPC_PACK_TIME_LW)
PROBE(MPC_Add_pack, MPC_PACK, MPC_Add_pack, MPC_Add_pack_CALL,
      MPC_Add_pack_TIME, MPC_Add_pack_TIME_HW, MPC_Add_pack_TIME_LW)
PROBE(MPC_Add_pack_absolute, MPC_PACK, MPC_Add_pack_absolute,
      MPC_Add_pack_absolute_CALL, MPC_Add_pack_absolute_TIME,
      MPC_Add_pack_absolute_TIME_HW, MPC_Add_pack_absolute_TIME_LW)
PROBE(MPC_Add_pack_default, MPC_PACK, MPC_Add_pack_default,
      MPC_Add_pack_default_CALL, MPC_Add_pack_default_TIME,
      MPC_Add_pack_default_TIME_HW, MPC_Add_pack_default_TIME_LW)
PROBE(MPC_Default_pack, MPC_PACK, MPC_Default_pack, MPC_Default_pack_CALL,
      MPC_Default_pack_TIME, MPC_Default_pack_TIME_HW, MPC_Default_pack_TIME_LW)
PROBE(MPC_Open_pack, MPC_PACK, MPC_Open_pack, MPC_Open_pack_CALL,
      MPC_Open_pack_TIME, MPC_Open_pack_TIME_HW, MPC_Open_pack_TIME_LW)
PROBE(MPC_RANK, MPC, MPC rank and context, MPC_RANK_CALL, MPC_RANK_TIME,
      MPC_RANK_TIME_HW, MPC_RANK_TIME_LW)
PROBE(MPC_Node_number, MPC_RANK, MPC_Node_number, MPC_Node_number_CALL,
      MPC_Node_number_TIME, MPC_Node_number_TIME_HW, MPC_Node_number_TIME_LW)
PROBE(MPC_Node_rank, MPC_RANK, MPC_Node_rank, MPC_Node_rank_CALL,
      MPC_Node_rank_TIME, MPC_Node_rank_TIME_HW, MPC_Node_rank_TIME_LW)
PROBE(MPC_Process_number, MPC_RANK, MPC_Process_number, MPC_Process_number_CALL,
      MPC_Process_number_TIME, MPC_Process_number_TIME_HW,
      MPC_Process_number_TIME_LW)
PROBE(MPC_Process_rank, MPC_RANK, MPC_Process_rank, MPC_Process_rank_CALL,
      MPC_Process_rank_TIME, MPC_Process_rank_TIME_HW, MPC_Process_rank_TIME_LW)
PROBE(MPC_Processor_number, MPC_RANK, MPC_Processor_number,
      MPC_Processor_number_CALL, MPC_Processor_number_TIME,
      MPC_Processor_number_TIME_HW, MPC_Processor_number_TIME_LW)
PROBE(MPC_Processor_rank, MPC_RANK, MPC_Processor_rank, MPC_Processor_rank_CALL,
      MPC_Processor_rank_TIME, MPC_Processor_rank_TIME_HW,
      MPC_Processor_rank_TIME_LW)
PROBE(MPC_Comm_rank, MPC_RANK, MPC_Comm_rank, MPC_Comm_rank_CALL,
      MPC_Comm_rank_TIME, MPC_Comm_rank_TIME_HW, MPC_Comm_rank_TIME_LW)
PROBE(MPC_Comm_size, MPC_RANK, MPC_Comm_size, MPC_Comm_size_CALL,
      MPC_Comm_size_TIME, MPC_Comm_size_TIME_HW, MPC_Comm_size_TIME_LW)
PROBE(MPC_Get_processor_name, MPC_RANK, MPC_Get_processor_name,
      MPC_Get_processor_name_CALL, MPC_Get_processor_name_TIME,
      MPC_Get_processor_name_TIME_HW, MPC_Get_processor_name_TIME_LW)
PROBE(MPC_Get_count, MPC_RANK, MPC_Get_count, MPC_Get_count_CALL,
      MPC_Get_count_TIME, MPC_Get_count_TIME_HW, MPC_Get_count_TIME_LW)
PROBE(MPC_Wtime, MPC_RANK, MPC_Wtime, MPC_Wtime_CALL, MPC_Wtime_TIME,
      MPC_Wtime_TIME_HW, MPC_Wtime_TIME_LW)
PROBE(MPC_Local_process_rank, MPC_RANK, MPC_Local_process_rank,
      MPC_Local_process_rank_CALL, MPC_Local_process_rank_TIME,
      MPC_Local_process_rank_TIME_HW, MPC_Local_process_rank_TIME_LW)
PROBE(MPC_Local_process_number, MPC_RANK, MPC_Local_process_number,
      MPC_Local_process_number_CALL, MPC_Local_process_number_TIME,
      MPC_Local_process_number_TIME_HW, MPC_Local_process_number_TIME_LW)
PROBE(MPC_Task_rank, MPC_RANK, MPC_Task_rank, MPC_Task_rank_CALL,
      MPC_Task_rank_TIME, MPC_Task_rank_TIME_HW, MPC_Task_rank_TIME_LW)
PROBE(MPC_Task_number, MPC_RANK, MPC_Task_number, MPC_Task_number_CALL,
      MPC_Task_number_TIME, MPC_Task_number_TIME_HW, MPC_Task_number_TIME_LW)
PROBE(MPC_ERROR, MPC, MPC error related, MPC_ERROR_CALL, MPC_ERROR_TIME,
      MPC_ERROR_TIME_HW, MPC_ERROR_TIME_LW)
PROBE(MPC_Errhandler_create, MPC_ERROR, MPC_Errhandler_create,
      MPC_Errhandler_create_CALL, MPC_Errhandler_create_TIME,
      MPC_Errhandler_create_TIME_HW, MPC_Errhandler_create_TIME_LW)
PROBE(MPC_Errhandler_free, MPC_ERROR, MPC_Errhandler_free,
      MPC_Errhandler_free_CALL, MPC_Errhandler_free_TIME,
      MPC_Errhandler_free_TIME_HW, MPC_Errhandler_free_TIME_LW)
PROBE(MPC_Errhandler_get, MPC_ERROR, MPC_Errhandler_get,
      MPC_Errhandler_get_CALL, MPC_Errhandler_get_TIME,
      MPC_Errhandler_get_TIME_HW, MPC_Errhandler_get_TIME_LW)
PROBE(MPC_Errhandler_set, MPC_ERROR, MPC_Errhandler_set,
      MPC_Errhandler_set_CALL, MPC_Errhandler_set_TIME,
      MPC_Errhandler_set_TIME_HW, MPC_Errhandler_set_TIME_LW)
PROBE(MPC_Error_class, MPC_ERROR, MPC_Error_class, MPC_Error_class_CALL,
      MPC_Error_class_TIME, MPC_Error_class_TIME_HW, MPC_Error_class_TIME_LW)
PROBE(MPC_Error_string, MPC_ERROR, MPC_Error_string, MPC_Error_string_CALL,
      MPC_Error_string_TIME, MPC_Error_string_TIME_HW, MPC_Error_string_TIME_LW)
PROBE(MPC_Abort, MPC_ERROR, MPC_Abort, MPC_Abort_CALL, MPC_Abort_TIME,
      MPC_Abort_TIME_HW, MPC_Abort_TIME_LW)
PROBE(MPC_TYPES, MPC, MPC types related, MPC_TYPES_CALL, MPC_TYPES_TIME,
      MPC_TYPES_TIME_HW, MPC_TYPES_TIME_LW)
PROBE(MPC_Type_hcontiguous, MPC_TYPES, MPC_Type_hcontiguous,
      MPC_Type_hcontiguous_CALL, MPC_Type_hcontiguous_TIME,
      MPC_Type_hcontiguous_TIME_HW, MPC_Type_hcontiguous_TIME_LW)
PROBE(MPC_Type_free, MPC_TYPES, MPC_Type_free, MPC_Type_free_CALL,
      MPC_Type_free_TIME, MPC_Type_free_TIME_HW, MPC_Type_free_TIME_LW)
PROBE(MPC_Type_size, MPC_TYPES, MPC_Type_size, MPC_Type_size_CALL,
      MPC_Type_size_TIME, MPC_Type_size_TIME_HW, MPC_Type_size_TIME_LW)
PROBE(MPC_Derived_datatype, MPC_TYPES, MPC_Derived_datatype,
      MPC_Derived_datatype_CALL, MPC_Derived_datatype_TIME,
      MPC_Derived_datatype_TIME_HW, MPC_Derived_datatype_TIME_LW)
PROBE(MPC_OP, MPC, MPC Operation related, MPC_OP_CALL, MPC_OP_TIME,
      MPC_OP_TIME_HW, MPC_OP_TIME_LW)
PROBE(MPC_Op_create, MPC_OP, MPC_Op_create, MPC_Op_create_CALL,
      MPC_Op_create_TIME, MPC_Op_create_TIME_HW, MPC_Op_create_TIME_LW)
PROBE(MPC_Op_free, MPC_OP, MPC_Op_free, MPC_Op_free_CALL, MPC_Op_free_TIME,
      MPC_Op_free_TIME_HW, MPC_Op_free_TIME_LW)
PROBE(MPC_SETUP, MPC, MPC Init and release, MPC_SETUP_CALL, MPC_SETUP_TIME,
      MPC_SETUP_TIME_HW, MPC_SETUP_TIME_LW)
PROBE(MPC_Finalize, MPC_SETUP, MPC_Finalize, MPC_Finalize_CALL,
      MPC_Finalize_TIME, MPC_Finalize_TIME_HW, MPC_Finalize_TIME_LW)
PROBE(MPC_Init, MPC_SETUP, MPC_Init, MPC_Init_CALL, MPC_Init_TIME,
      MPC_Init_TIME_HW, MPC_Init_TIME_LW)
PROBE(MPC_Initialized, MPC_SETUP, MPC_Initialized, MPC_Initialized_CALL,
      MPC_Initialized_TIME, MPC_Initialized_TIME_HW, MPC_Initialized_TIME_LW)
PROBE(MPC_INTERNAL, MPC, Internals, MPC_INTERNAL_CALL, MPC_INTERNAL_TIME,
      MPC_INTERNAL_TIME_HW, MPC_INTERNAL_TIME_LW)
PROBE(MPC_Copy_message, MPC_INTERNAL, MPC_Copy_message, MPC_Copy_message_CALL,
      MPC_Copy_message_TIME, MPC_Copy_message_TIME_HW, MPC_Copy_message_TIME_LW)
PROBE(MPC_Test_message_search_matching, MPC_INTERNAL,
      MPC_Test_message_search_matching, MPC_Test_message_search_matching_CALL,
      MPC_Test_message_search_matching_TIME,
      MPC_Test_message_search_matching_TIME_HW,
      MPC_Test_message_search_matching_TIME_LW)
PROBE(MPC_Test_message_pair_locked, MPC_INTERNAL, MPC_Test_message_pair_locked,
      MPC_Test_message_pair_locked_CALL, MPC_Test_message_pair_locked_TIME,
      MPC_Test_message_pair_locked_TIME_HW,
      MPC_Test_message_pair_locked_TIME_LW)
PROBE(MPC_Test_message_pair, MPC_INTERNAL, MPC_Test_message_pair,
      MPC_Test_message_pair_CALL, MPC_Test_message_pair_TIME,
      MPC_Test_message_pair_TIME_HW, MPC_Test_message_pair_TIME_LW)
PROBE(MPC_Test_message_pair_try, MPC_INTERNAL, MPC_Test_message_pair_try,
      MPC_Test_message_pair_try_CALL, MPC_Test_message_pair_try_TIME,
      MPC_Test_message_pair_try_TIME_HW, MPC_Test_message_pair_try_TIME_LW)
PROBE(MPC_Test_message_pair_try_lock, MPC_INTERNAL,
      MPC_Test_message_pair_try_lock, MPC_Test_message_pair_try_lock_CALL,
      MPC_Test_message_pair_try_lock_TIME,
      MPC_Test_message_pair_try_lock_TIME_HW,
      MPC_Test_message_pair_try_lock_TIME_LW)
PROBE(MPC_Recv_init_message, MPC_INTERNAL, MPC_Recv_init_message,
      MPC_Recv_init_message_CALL, MPC_Recv_init_message_TIME,
      MPC_Recv_init_message_TIME_HW, MPC_Recv_init_message_TIME_LW)
PROBE(sctk_mpc_wait_message, MPC_INTERNAL, sctk_mpc_wait_message,
      sctk_mpc_wait_message_CALL, sctk_mpc_wait_message_TIME,
      sctk_mpc_wait_message_TIME_HW, sctk_mpc_wait_message_TIME_LW)
PROBE(ALLOCATOR, NO_PARENT, MPC Allocator, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(POSIX_ALLOC, ALLOCATOR, Posix allocator Interface, POSIX_ALLOC_CALL,
      POSIX_ALLOC_TIME, POSIX_ALLOC_TIME_HW, POSIX_ALLOC_TIME_LW)
SIZE_COUNTER(malloc_size, POSIX_ALLOC, size allocated by malloc,
             malloc_size_CALL, malloc_size_TIME, malloc_size_TIME_HW,
             malloc_size_TIME_LW)
PROBE(malloc, POSIX_ALLOC, malloc, malloc_CALL, malloc_TIME, malloc_TIME_HW,
      malloc_TIME_LW)
PROBE(calloc, POSIX_ALLOC, calloc, calloc_CALL, calloc_TIME, calloc_TIME_HW,
      calloc_TIME_LW)
PROBE(realloc, POSIX_ALLOC, realloc, realloc_CALL, realloc_TIME,
      realloc_TIME_HW, realloc_TIME_LW)
PROBE(free, POSIX_ALLOC, free, free_CALL, free_TIME, free_TIME_HW, free_TIME_LW)
PROBE(cfree, POSIX_ALLOC, cfree, cfree_CALL, cfree_TIME, cfree_TIME_HW,
      cfree_TIME_LW)
PROBE(memalign, POSIX_ALLOC, memalign, memalign_CALL, memalign_TIME,
      memalign_TIME_HW, memalign_TIME_LW)
PROBE(posix_memalign, POSIX_ALLOC, posix_memalign, posix_memalign_CALL,
      posix_memalign_TIME, posix_memalign_TIME_HW, posix_memalign_TIME_LW)
PROBE(MPC_ALLOC, ALLOCATOR, MPC internal allocator Interface, MPC_ALLOC_CALL,
      MPC_ALLOC_TIME, MPC_ALLOC_TIME_HW, MPC_ALLOC_TIME_LW)
PROBE(sctk_malloc, MPC_ALLOC, sctk_malloc, sctk_malloc_CALL, sctk_malloc_TIME,
      sctk_malloc_TIME_HW, sctk_malloc_TIME_LW)
SIZE_COUNTER(sctk_malloc_size, MPC_ALLOC, size allocated by sctk_malloc,
             sctk_malloc_size_CALL, sctk_malloc_size_TIME,
             sctk_malloc_size_TIME_HW, sctk_malloc_size_TIME_LW)
PROBE(sctk_calloc, MPC_ALLOC, sctk_calloc, sctk_calloc_CALL, sctk_calloc_TIME,
      sctk_calloc_TIME_HW, sctk_calloc_TIME_LW)
PROBE(sctk_memalign, MPC_ALLOC, sctk_memalign, sctk_memalign_CALL,
      sctk_memalign_TIME, sctk_memalign_TIME_HW, sctk_memalign_TIME_LW)
PROBE(sctk_posix_memalign, MPC_ALLOC, sctk_posix_memalign,
      sctk_posix_memalign_CALL, sctk_posix_memalign_TIME,
      sctk_posix_memalign_TIME_HW, sctk_posix_memalign_TIME_LW)
PROBE(sctk_realloc_inter_chain, MPC_ALLOC, sctk_realloc_inter_chain,
      sctk_realloc_inter_chain_CALL, sctk_realloc_inter_chain_TIME,
      sctk_realloc_inter_chain_TIME_HW, sctk_realloc_inter_chain_TIME_LW)
PROBE(sctk_alloc_posix_numa_migrate, MPC_ALLOC, sctk_alloc_posix_numa_migrate,
      sctk_alloc_posix_numa_migrate_CALL, sctk_alloc_posix_numa_migrate_TIME,
      sctk_alloc_posix_numa_migrate_TIME_HW,
      sctk_alloc_posix_numa_migrate_TIME_LW)
PROBE(sctk_alloc_posix_setup_tls_chain, MPC_ALLOC,
      sctk_alloc_posix_setup_tls_chain, sctk_alloc_posix_setup_tls_chain_CALL,
      sctk_alloc_posix_setup_tls_chain_TIME,
      sctk_alloc_posix_setup_tls_chain_TIME_HW,
      sctk_alloc_posix_setup_tls_chain_TIME_LW)
PROBE(sctk_realloc, MPC_ALLOC, sctk_realloc, sctk_realloc_CALL,
      sctk_realloc_TIME, sctk_realloc_TIME_HW, sctk_realloc_TIME_LW)
PROBE(sctk_free, MPC_ALLOC, sctk_free, sctk_free_CALL, sctk_free_TIME,
      sctk_free_TIME_HW, sctk_free_TIME_LW)
PROBE(sctk_sbrk, MPC_ALLOC, sctk_sbrk, sctk_sbrk_CALL, sctk_sbrk_TIME,
      sctk_sbrk_TIME_HW, sctk_sbrk_TIME_LW)
PROBE(sctk_sbrk_try_to_find_page, MPC_ALLOC, sctk_sbrk_try_to_find_page,
      sctk_sbrk_try_to_find_page_CALL, sctk_sbrk_try_to_find_page_TIME,
      sctk_sbrk_try_to_find_page_TIME_HW, sctk_sbrk_try_to_find_page_TIME_LW)
PROBE(sctk_sbrk_mmap, MPC_ALLOC, sctk_sbrk_mmap, sctk_sbrk_mmap_CALL,
      sctk_sbrk_mmap_TIME, sctk_sbrk_mmap_TIME_HW, sctk_sbrk_mmap_TIME_LW)
PROBE(sctk_free_distant, MPC_ALLOC, sctk_free_distant, sctk_free_distant_CALL,
      sctk_free_distant_TIME, sctk_free_distant_TIME_HW,
      sctk_free_distant_TIME_LW)
PROBE(sctk_mmap, MPC_ALLOC, sctk_mmap, sctk_mmap_CALL, sctk_mmap_TIME,
      sctk_mmap_TIME_HW, sctk_mmap_TIME_LW)
PROBE(sctk_alloc_ptr_small_serach_chunk, MPC_ALLOC,
      sctk_alloc_ptr_small_serach_chunk, sctk_alloc_ptr_small_serach_chunk_CALL,
      sctk_alloc_ptr_small_serach_chunk_TIME,
      sctk_alloc_ptr_small_serach_chunk_TIME_HW,
      sctk_alloc_ptr_small_serach_chunk_TIME_LW)
PROBE(sctk_alloc_ptr_big, MPC_ALLOC, sctk_alloc_ptr_big,
      sctk_alloc_ptr_big_CALL, sctk_alloc_ptr_big_TIME,
      sctk_alloc_ptr_big_TIME_HW, sctk_alloc_ptr_big_TIME_LW)
PROBE(sctk_alloc_ptr_small, MPC_ALLOC, sctk_alloc_ptr_small,
      sctk_alloc_ptr_small_CALL, sctk_alloc_ptr_small_TIME,
      sctk_alloc_ptr_small_TIME_HW, sctk_alloc_ptr_small_TIME_LW)
PROBE(sctk_free_small, MPC_ALLOC, sctk_free_small, sctk_free_small_CALL,
      sctk_free_small_TIME, sctk_free_small_TIME_HW, sctk_free_small_TIME_LW)
PROBE(sctk_free_big, MPC_ALLOC, sctk_free_big, sctk_free_big_CALL,
      sctk_free_big_TIME, sctk_free_big_TIME_HW, sctk_free_big_TIME_LW)
PROBE(THREAD, NO_PARENT, MPI Thread, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(sctk_thread_mutex_lock, THREAD, sctk_thread_mutex_lock,
      sctk_thread_mutex_lock_CALL, sctk_thread_mutex_lock_TIME,
      sctk_thread_mutex_lock_TIME_HW, sctk_thread_mutex_lock_TIME_LW)
PROBE(sctk_thread_mutex_unlock, THREAD, sctk_thread_mutex_unlock,
      sctk_thread_mutex_unlock_CALL, sctk_thread_mutex_unlock_TIME,
      sctk_thread_mutex_unlock_TIME_HW, sctk_thread_mutex_unlock_TIME_LW)
PROBE(sctk_thread_mutex_spinlock, THREAD, sctk_thread_mutex_spinlock,
      sctk_thread_mutex_spinlock_CALL, sctk_thread_mutex_spinlock_TIME,
      sctk_thread_mutex_spinlock_TIME_HW, sctk_thread_mutex_spinlock_TIME_LW)
PROBE(sctk_thread_wait_for_value_and_poll, THREAD,
      sctk_thread_wait_for_value_and_poll,
      sctk_thread_wait_for_value_and_poll_CALL,
      sctk_thread_wait_for_value_and_poll_TIME,
      sctk_thread_wait_for_value_and_poll_TIME_HW,
      sctk_thread_wait_for_value_and_poll_TIME_LW)
PROBE(sctk_thread_wait_for_value, THREAD, sctk_thread_wait_for_value,
      sctk_thread_wait_for_value_CALL, sctk_thread_wait_for_value_TIME,
      sctk_thread_wait_for_value_TIME_HW, sctk_thread_wait_for_value_TIME_LW)
PROBE(MESSAGE_PASSING_INT, NO_PARENT, MPC Message passing internals,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(sctk_elan_barrier, MESSAGE_PASSING_INT, sctk_elan_barrier,
      sctk_elan_barrier_CALL, sctk_elan_barrier_TIME, sctk_elan_barrier_TIME_HW,
      sctk_elan_barrier_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver, sctk_net_send_ptp_message_driver_CALL,
      sctk_net_send_ptp_message_driver_TIME,
      sctk_net_send_ptp_message_driver_TIME_HW,
      sctk_net_send_ptp_message_driver_TIME_LW)
PROBE(sctk_net_get_new_header, MESSAGE_PASSING_INT, sctk_net_get_new_header,
      sctk_net_get_new_header_CALL, sctk_net_get_new_header_TIME,
      sctk_net_get_new_header_TIME_HW, sctk_net_get_new_header_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver_inf, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver_inf,
      sctk_net_send_ptp_message_driver_inf_CALL,
      sctk_net_send_ptp_message_driver_inf_TIME,
      sctk_net_send_ptp_message_driver_inf_TIME_HW,
      sctk_net_send_ptp_message_driver_inf_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver_sup, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver_sup,
      sctk_net_send_ptp_message_driver_sup_CALL,
      sctk_net_send_ptp_message_driver_sup_TIME,
      sctk_net_send_ptp_message_driver_sup_TIME_HW,
      sctk_net_send_ptp_message_driver_sup_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver_sup_write, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver_sup_write,
      sctk_net_send_ptp_message_driver_sup_write_CALL,
      sctk_net_send_ptp_message_driver_sup_write_TIME,
      sctk_net_send_ptp_message_driver_sup_write_TIME_HW,
      sctk_net_send_ptp_message_driver_sup_write_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver_split_header, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver_split_header,
      sctk_net_send_ptp_message_driver_split_header_CALL,
      sctk_net_send_ptp_message_driver_split_header_TIME,
      sctk_net_send_ptp_message_driver_split_header_TIME_HW,
      sctk_net_send_ptp_message_driver_split_header_TIME_LW)
PROBE(sctk_net_send_ptp_message_driver_send_header, MESSAGE_PASSING_INT,
      sctk_net_send_ptp_message_driver_send_header,
      sctk_net_send_ptp_message_driver_send_header_CALL,
      sctk_net_send_ptp_message_driver_send_header_TIME,
      sctk_net_send_ptp_message_driver_send_header_TIME_HW,
      sctk_net_send_ptp_message_driver_send_header_TIME_LW)
COUNTER(signalization_endpoints, MESSAGE_PASSING_INT, signalization_endpoints,
        signalization_endpoints_CALL, signalization_endpoints_TIME,
        signalization_endpoints_TIME_HW, signalization_endpoints_TIME_LW)
PROBE(INFINIBAND, MESSAGE_PASSING_INT, Infiniband module, INFINIBAND_CALL,
      INFINIBAND_TIME, INFINIBAND_TIME_HW, INFINIBAND_TIME_LW)
PROBE(ib_time_steals, INFINIBAND, ib_time_steals, ib_time_steals_CALL,
      ib_time_steals_TIME, ib_time_steals_TIME_HW, ib_time_steals_TIME_LW)
PROBE(ib_time_own, INFINIBAND, ib_time_steals, ib_time_own_CALL,
      ib_time_own_TIME, ib_time_own_TIME_HW, ib_time_own_TIME_LW)
PROBE(ib_time_ptp, INFINIBAND, ib_time_ptp, ib_time_ptp_CALL, ib_time_ptp_TIME,
      ib_time_ptp_TIME_HW, ib_time_ptp_TIME_LW)
PROBE(ib_time_coll, INFINIBAND, ib_time_coll, ib_time_coll_CALL,
      ib_time_coll_TIME, ib_time_coll_TIME_HW, ib_time_coll_TIME_LW)
PROBE(ib_time_send, INFINIBAND, ib_time_send, ib_time_send_CALL,
      ib_time_send_TIME, ib_time_send_TIME_HW, ib_time_send_TIME_LW)
PROBE(ib_poll_send, INFINIBAND, ib_poll_send, ib_poll_send_CALL,
      ib_poll_send_TIME, ib_poll_send_TIME_HW, ib_poll_send_TIME_LW)
PROBE(ib_poll_recv, INFINIBAND, ib_poll_recv, ib_poll_recv_CALL,
      ib_poll_recv_TIME, ib_poll_recv_TIME_HW, ib_poll_recv_TIME_LW)
PROBE(ib_post_send_lock, INFINIBAND, ib_post_send_lock, ib_post_send_lock_CALL,
      ib_post_send_lock_TIME, ib_post_send_lock_TIME_HW,
      ib_post_send_lock_TIME_LW)
PROBE(ib_post_send, INFINIBAND, ib_post_send, ib_post_send_CALL,
      ib_post_send_TIME, ib_post_send_TIME_HW, ib_post_send_TIME_LW)
PROBE(ib_resize_rdma, INFINIBAND, ib_resize_rdma, ib_resize_rdma_CALL,
      ib_resize_rdma_TIME, ib_resize_rdma_TIME_HW, ib_resize_rdma_TIME_LW)
PROBE(ib_get_numa, INFINIBAND, ib_get_numa, ib_get_numa_CALL, ib_get_numa_TIME,
      ib_get_numa_TIME_HW, ib_get_numa_TIME_LW)
PROBE(IBUF, INFINIBAND, Infiniband ibuf, IBUF_CALL, IBUF_TIME, IBUF_TIME_HW,
      IBUF_TIME_LW)
PROBE(ib_pick_send, IBUF, ib_pick_send, ib_pick_send_CALL, ib_pick_send_TIME,
      ib_pick_send_TIME_HW, ib_pick_send_TIME_LW)
PROBE(ib_pick_send_sr, IBUF, ib_pick_send_sr, ib_pick_send_sr_CALL,
      ib_pick_send_sr_TIME, ib_pick_send_sr_TIME_HW, ib_pick_send_sr_TIME_LW)
PROBE(ib_pick_recv, IBUF, ib_pick_recv, ib_pick_recv_CALL, ib_pick_recv_TIME,
      ib_pick_recv_TIME_HW, ib_pick_recv_TIME_LW)
PROBE(ib_ibuf_sr_send_release, IBUF, ib_ibuf_sr_send_release,
      ib_ibuf_sr_send_release_CALL, ib_ibuf_sr_send_release_TIME,
      ib_ibuf_sr_send_release_TIME_HW, ib_ibuf_sr_send_release_TIME_LW)
PROBE(ib_ibuf_sr_srq_release, IBUF, ib_ibuf_sr_srq_release,
      ib_ibuf_sr_srq_release_CALL, ib_ibuf_sr_srq_release_TIME,
      ib_ibuf_sr_srq_release_TIME_HW, ib_ibuf_sr_srq_release_TIME_LW)
PROBE(ib_ibuf_rdma_release, IBUF, ib_ibuf_rdma_release,
      ib_ibuf_rdma_release_CALL, ib_ibuf_rdma_release_TIME,
      ib_ibuf_rdma_release_TIME_HW, ib_ibuf_rdma_release_TIME_LW)
PROBE(ib_poll_recv_ibuf, INFINIBAND, ib_poll_recv_ibuf, ib_poll_recv_ibuf_CALL,
      ib_poll_recv_ibuf_TIME, ib_poll_recv_ibuf_TIME_HW,
      ib_poll_recv_ibuf_TIME_LW)
COUNTER(ib_poll_steals, INFINIBAND, ib_poll_steals, ib_poll_steals_CALL,
        ib_poll_steals_TIME, ib_poll_steals_TIME_HW, ib_poll_steals_TIME_LW)
COUNTER(ib_poll_steals_failed, INFINIBAND, ib_poll_steals_failed,
        ib_poll_steals_failed_CALL, ib_poll_steals_failed_TIME,
        ib_poll_steals_failed_TIME_HW, ib_poll_steals_failed_TIME_LW)
COUNTER(ib_poll_steals_success, INFINIBAND, ib_poll_steals_success,
        ib_poll_steals_success_CALL, ib_poll_steals_success_TIME,
        ib_poll_steals_success_TIME_HW, ib_poll_steals_success_TIME_LW)
COUNTER(ib_poll_steals_same_node, INFINIBAND, ib_poll_steals_same_node,
        ib_poll_steals_same_node_CALL, ib_poll_steals_same_node_TIME,
        ib_poll_steals_same_node_TIME_HW, ib_poll_steals_same_node_TIME_LW)
COUNTER(ib_poll_steals_other_node, INFINIBAND, ib_poll_steals_other_node,
        ib_poll_steals_other_node_CALL, ib_poll_steals_other_node_TIME,
        ib_poll_steals_other_node_TIME_HW, ib_poll_steals_other_node_TIME_LW)
COUNTER(ib_poll_own, INFINIBAND, ib_poll_own, ib_poll_own_CALL,
        ib_poll_own_TIME, ib_poll_own_TIME_HW, ib_poll_own_TIME_LW)
COUNTER(ib_poll_own_failed, INFINIBAND, ib_poll_own_failed,
        ib_poll_own_failed_CALL, ib_poll_own_failed_TIME,
        ib_poll_own_failed_TIME_HW, ib_poll_own_failed_TIME_LW)
COUNTER(ib_poll_own_success, INFINIBAND, ib_poll_own_success,
        ib_poll_own_success_CALL, ib_poll_own_success_TIME,
        ib_poll_own_success_TIME_HW, ib_poll_own_success_TIME_LW)
COUNTER(ib_call_to_polling, INFINIBAND, ib_call_to_polling,
        ib_call_to_polling_CALL, ib_call_to_polling_TIME,
        ib_call_to_polling_TIME_HW, ib_call_to_polling_TIME_LW)
COUNTER(ib_cp_matched, INFINIBAND, ib_cp_matched, ib_cp_matched_CALL,
        ib_cp_matched_TIME, ib_cp_matched_TIME_HW, ib_cp_matched_TIME_LW)
COUNTER(ib_cp_not_matched, INFINIBAND, ib_cp_not_matched,
        ib_cp_not_matched_CALL, ib_cp_not_matched_TIME,
        ib_cp_not_matched_TIME_HW, ib_cp_not_matched_TIME_LW)
COUNTER(ib_poll_found, INFINIBAND, ib_poll_found, ib_poll_found_CALL,
        ib_poll_found_TIME, ib_poll_found_TIME_HW, ib_poll_found_TIME_LW)
COUNTER(ib_poll_not_found, INFINIBAND, ib_poll_not_found,
        ib_poll_not_found_CALL, ib_poll_not_found_TIME,
        ib_poll_not_found_TIME_HW, ib_poll_not_found_TIME_LW)
COUNTER(ib_alloc_mem, INFINIBAND, ib_alloc_mem, ib_alloc_mem_CALL,
        ib_alloc_mem_TIME, ib_alloc_mem_TIME_HW, ib_alloc_mem_TIME_LW)
COUNTER(ib_free_mem, INFINIBAND, ib_free_mem, ib_free_mem_CALL,
        ib_free_mem_TIME, ib_free_mem_TIME_HW, ib_free_mem_TIME_LW)
COUNTER(ib_qp_created, INFINIBAND, ib_qp_created, ib_qp_created_CALL,
        ib_qp_created_TIME, ib_qp_created_TIME_HW, ib_qp_created_TIME_LW)
COUNTER(ib_eager_nb, INFINIBAND, ib_eager_nb, ib_eager_nb_CALL,
        ib_eager_nb_TIME, ib_eager_nb_TIME_HW, ib_eager_nb_TIME_LW)
COUNTER(ib_buffered_nb, INFINIBAND, ib_buffered_nb, ib_buffered_nb_CALL,
        ib_buffered_nb_TIME, ib_buffered_nb_TIME_HW, ib_buffered_nb_TIME_LW)
COUNTER(ib_rdma_nb, INFINIBAND, ib_rdma_nb, ib_rdma_nb_CALL, ib_rdma_nb_TIME,
        ib_rdma_nb_TIME_HW, ib_rdma_nb_TIME_LW)
COUNTER(ib_ibuf_sr_nb, INFINIBAND, ib_ibuf_sr_nb, ib_ibuf_sr_nb_CALL,
        ib_ibuf_sr_nb_TIME, ib_ibuf_sr_nb_TIME_HW, ib_ibuf_sr_nb_TIME_LW)
COUNTER(ib_ibuf_sr_size, INFINIBAND, ib_ibuf_sr_size, ib_ibuf_sr_size_CALL,
        ib_ibuf_sr_size_TIME, ib_ibuf_sr_size_TIME_HW, ib_ibuf_sr_size_TIME_LW)
COUNTER(ib_ibuf_rdma_nb, INFINIBAND, ib_ibuf_rdma_nb, ib_ibuf_rdma_nb_CALL,
        ib_ibuf_rdma_nb_TIME, ib_ibuf_rdma_nb_TIME_HW, ib_ibuf_rdma_nb_TIME_LW)
COUNTER(ib_ibuf_rdma_size, INFINIBAND, ib_ibuf_rdma_size,
        ib_ibuf_rdma_size_CALL, ib_ibuf_rdma_size_TIME,
        ib_ibuf_rdma_size_TIME_HW, ib_ibuf_rdma_size_TIME_LW)
COUNTER(ib_ibuf_rdma_hits_nb, INFINIBAND, ib_ibuf_rdma_hits_nb,
        ib_ibuf_rdma_hits_nb_CALL, ib_ibuf_rdma_hits_nb_TIME,
        ib_ibuf_rdma_hits_nb_TIME_HW, ib_ibuf_rdma_hits_nb_TIME_LW)
COUNTER(ib_ibuf_rdma_miss_nb, INFINIBAND, ib_ibuf_rdma_miss_nb,
        ib_ibuf_rdma_miss_nb_CALL, ib_ibuf_rdma_miss_nb_TIME,
        ib_ibuf_rdma_miss_nb_TIME_HW, ib_ibuf_rdma_miss_nb_TIME_LW)
COUNTER(ib_rdma_connection, INFINIBAND, ib_rdma_connection,
        ib_rdma_connection_CALL, ib_rdma_connection_TIME,
        ib_rdma_connection_TIME_HW, ib_rdma_connection_TIME_LW)
COUNTER(ib_rdma_resizing, INFINIBAND, ib_rdma_resizing, ib_rdma_resizing_CALL,
        ib_rdma_resizing_TIME, ib_rdma_resizing_TIME_HW,
        ib_rdma_resizing_TIME_LW)
COUNTER(ib_rdma_deconnection, INFINIBAND, ib_rdma_deconnection,
        ib_rdma_deconnection_CALL, ib_rdma_deconnection_TIME,
        ib_rdma_deconnection_TIME_HW, ib_rdma_deconnection_TIME_LW)
COUNTER(matching_found, INFINIBAND, matching_found, matching_found_CALL,
        matching_found_TIME, matching_found_TIME_HW, matching_found_TIME_LW)
COUNTER(matching_not_found, INFINIBAND, matching_not_found,
        matching_not_found_CALL, matching_not_found_TIME,
        matching_not_found_TIME_HW, matching_not_found_TIME_LW)
COUNTER(matching_locked, INFINIBAND, matching_locked, matching_locked_CALL,
        matching_locked_TIME, matching_locked_TIME_HW, matching_locked_TIME_LW)
PROBE(send_mmu_register, INFINIBAND, send_mmu_register, send_mmu_register_CALL,
      send_mmu_register_TIME, send_mmu_register_TIME_HW,
      send_mmu_register_TIME_LW)
PROBE(recv_mmu_register, INFINIBAND, recv_mmu_register, recv_mmu_register_CALL,
      recv_mmu_register_TIME, recv_mmu_register_TIME_HW,
      recv_mmu_register_TIME_LW)
PROBE(ib_buffered_memcpy, INFINIBAND, buffered_memcpy, ib_buffered_memcpy_CALL,
      ib_buffered_memcpy_TIME, ib_buffered_memcpy_TIME_HW,
      ib_buffered_memcpy_TIME_LW)
PROBE(ib_send_message, INFINIBAND, ib_send_message, ib_send_message_CALL,
      ib_send_message_TIME, ib_send_message_TIME_HW, ib_send_message_TIME_LW)
PROBE(ib_ibuf_srq_post, INFINIBAND, ib_ibuf_srq_post, ib_ibuf_srq_post_CALL,
      ib_ibuf_srq_post_TIME, ib_ibuf_srq_post_TIME_HW, ib_ibuf_srq_post_TIME_LW)
COUNTER(mmu_cache_hit, INFINIBAND, mmu_cache_hit, mmu_cache_hit_CALL,
        mmu_cache_hit_TIME, mmu_cache_hit_TIME_HW, mmu_cache_hit_TIME_LW)
COUNTER(mmu_cache_miss, INFINIBAND, mmu_cache_miss, mmu_cache_miss_CALL,
        mmu_cache_miss_TIME, mmu_cache_miss_TIME_HW, mmu_cache_miss_TIME_LW)
PROBE(ib_ibuf_send_polled, INFINIBAND, ib_ibuf_send_polled,
      ib_ibuf_send_polled_CALL, ib_ibuf_send_polled_TIME,
      ib_ibuf_send_polled_TIME_HW, ib_ibuf_send_polled_TIME_LW)
PROBE(ib_ibuf_recv_polled, INFINIBAND, ib_ibuf_recv_polled,
      ib_ibuf_recv_polled_CALL, ib_ibuf_recv_polled_TIME,
      ib_ibuf_recv_polled_TIME_HW, ib_ibuf_recv_polled_TIME_LW)
PROBE(ib_ibuf_memcpy, INFINIBAND, ib_ibuf_memcpy, ib_ibuf_memcpy_CALL,
      ib_ibuf_memcpy_TIME, ib_ibuf_memcpy_TIME_HW, ib_ibuf_memcpy_TIME_LW)
PROBE(ib_rendezvous_wait_recv_ack, INFINIBAND, ib_rendezvous_wait_recv_ack,
      ib_rendezvous_wait_recv_ack_CALL, ib_rendezvous_wait_recv_ack_TIME,
      ib_rendezvous_wait_recv_ack_TIME_HW, ib_rendezvous_wait_recv_ack_TIME_LW)
PROBE(ib_rendezvous_wait_send_ack, INFINIBAND, ib_rendezvous_wait_send_ack,
      ib_rendezvous_wait_send_ack_CALL, ib_rendezvous_wait_send_ack_TIME,
      ib_rendezvous_wait_send_ack_TIME_HW, ib_rendezvous_wait_send_ack_TIME_LW)
PROBE(ib_rendezvous_wait_send_msg, INFINIBAND, ib_rendezvous_wait_send_msg,
      ib_rendezvous_wait_send_msg_CALL, ib_rendezvous_wait_send_msg_TIME,
      ib_rendezvous_wait_send_msg_TIME_HW, ib_rendezvous_wait_send_msg_TIME_LW)
PROBE(ib_rendezvous_wait_sender_done, INFINIBAND,
      ib_rendezvous_wait_sender_done, ib_rendezvous_wait_sender_done_CALL,
      ib_rendezvous_wait_sender_done_TIME,
      ib_rendezvous_wait_sender_done_TIME_HW,
      ib_rendezvous_wait_sender_done_TIME_LW)
PROBE(ib_rendezvous_wait_receiver_done, INFINIBAND,
      ib_rendezvous_wait_receiver_done, ib_rendezvous_wait_receiver_done_CALL,
      ib_rendezvous_wait_receiver_done_TIME,
      ib_rendezvous_wait_receiver_done_TIME_HW,
      ib_rendezvous_wait_receiver_done_TIME_LW)
PROBE(ib_rendezvous_recv_req, INFINIBAND, ib_rendezvous_recv_req,
      ib_rendezvous_recv_req_CALL, ib_rendezvous_recv_req_TIME,
      ib_rendezvous_recv_req_TIME_HW, ib_rendezvous_recv_req_TIME_LW)
PROBE(ib_rendezvous_recv_ack, INFINIBAND, ib_rendezvous_recv_ack,
      ib_rendezvous_recv_ack_CALL, ib_rendezvous_recv_ack_TIME,
      ib_rendezvous_recv_ack_TIME_HW, ib_rendezvous_recv_ack_TIME_LW)
PROBE(ib_rendezvous_recv_done_local, INFINIBAND, ib_rendezvous_recv_done_local,
      ib_rendezvous_recv_done_local_CALL, ib_rendezvous_recv_done_local_TIME,
      ib_rendezvous_recv_done_local_TIME_HW,
      ib_rendezvous_recv_done_local_TIME_LW)
PROBE(ib_rendezvous_recv_done_remote, INFINIBAND,
      ib_rendezvous_recv_done_remote, ib_rendezvous_recv_done_remote_CALL,
      ib_rendezvous_recv_done_remote_TIME,
      ib_rendezvous_recv_done_remote_TIME_HW,
      ib_rendezvous_recv_done_remote_TIME_LW)
PROBE(ib_rendezvous_matching_done, INFINIBAND, ib_rendezvous_matching_done,
      ib_rendezvous_matching_done_CALL, ib_rendezvous_matching_done_TIME,
      ib_rendezvous_matching_done_TIME_HW, ib_rendezvous_matching_done_TIME_LW)
PROBE(ib_rdma_idle, INFINIBAND, ib_rdma_idle, ib_rdma_idle_CALL,
      ib_rdma_idle_TIME, ib_rdma_idle_TIME_HW, ib_rdma_idle_TIME_LW)
PROBE(ib_poll_cq, INFINIBAND, ib_poll_cq, ib_poll_cq_CALL, ib_poll_cq_TIME,
      ib_poll_cq_TIME_HW, ib_poll_cq_TIME_LW)
PROBE(ib_perform_all, INFINIBAND, ib_perform_all, ib_perform_all_CALL,
      ib_perform_all_TIME, ib_perform_all_TIME_HW, ib_perform_all_TIME_LW)
PROBE(ib_cp_poll, INFINIBAND, ib_cp_poll, ib_cp_poll_CALL, ib_cp_poll_TIME,
      ib_cp_poll_TIME_HW, ib_cp_poll_TIME_LW)
PROBE(waitany_idle, INFINIBAND, waitany_idle, waitany_idle_CALL,
      waitany_idle_TIME, waitany_idle_TIME_HW, waitany_idle_TIME_LW)

PROBE(COLLABORATIVE_POLLING, INFINIBAND, Collaboraive Polling, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

PROBE(cp_time_own, COLLABORATIVE_POLLING, Time own, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
COUNTER(cp_counter_own, COLLABORATIVE_POLLING, Counter own, MPI_T_PVAR_NULL,
        MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
PROBE(cp_time_steal, COLLABORATIVE_POLLING, Time steal, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
COUNTER(cp_counter_steal_same_node, COLLABORATIVE_POLLING,
        Counter steal same node, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
        MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
COUNTER(cp_counter_steal_other_node, COLLABORATIVE_POLLING,
        Counter steal other node, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL,
        MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

PROBE(OPEN_MP, NO_PARENT, MPC Message passing internals, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

PROBE(__sctk_microthread_parallel_exec__last_barrier, OPEN_MP,
      __sctk_microthread_parallel_exec__last_barrier, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

PROBE(__mpcomp_start_parallel_region, OPEN_MP, __mpcomp_start_parallel_region,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)

PROBE(__mpcomp_start_parallel_region__creation, OPEN_MP,
      __mpcomp_start_parallel_region__creation, MPI_T_PVAR_NULL,
      MPI_T_PVAR_NULL, MPI_T_PVAR_NULL, MPI_T_PVAR_NULL)
