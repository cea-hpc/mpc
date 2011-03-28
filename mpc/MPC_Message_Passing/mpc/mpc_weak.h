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
 
    /* PMPC Weak symbol instrumentation interface */
    
    /*Initialisation */
#pragma weak MPC_Init = PMPC_Init
#pragma weak MPC_Init_thread = PMPC_Init_thread
#pragma weak MPC_Initialized = PMPC_Initialized
#pragma weak MPC_Finalize = PMPC_Finalize
#pragma weak MPC_Abort = PMPC_Abort
  /*Topology informations */
#pragma weak MPC_Comm_rank = PMPC_Comm_rank
#pragma weak MPC_Comm_size = PMPC_Comm_size
#pragma weak MPC_Comm_remote_size = PMPC_Comm_remote_size
#pragma weak MPC_Node_rank = PMPC_Node_rank
#pragma weak MPC_Node_number = PMPC_Node_number
#pragma weak MPC_Processor_rank = PMPC_Processor_rank
#pragma weak MPC_Processor_number = PMPC_Processor_number
#pragma weak MPC_Process_rank = PMPC_Process_rank
#pragma weak MPC_Process_number = PMPC_Process_number
#pragma weak MPC_Local_process_rank = PMPC_Local_process_rank
#pragma weak MPC_Local_process_number = PMPC_Local_process_number
#pragma weak MPC_Get_version = PMPC_Get_version
#pragma weak MPC_Get_multithreading = PMPC_Get_multithreading
#pragma weak MPC_Get_networking = PMPC_Get_networking
#pragma weak MPC_Get_processor_name = PMPC_Get_processor_name
    /*Collective operations */
#pragma weak MPC_Barrier = PMPC_Barrier
#pragma weak MPC_Bcast = PMPC_Bcast
#pragma weak MPC_Allreduce = PMPC_Allreduce
#pragma weak MPC_Reduce = PMPC_Reduce
#pragma weak MPC_Op_create = PMPC_Op_create
#pragma weak MPC_Op_free = PMPC_Op_free
    /*P-t-P Communications */
#pragma weak MPC_Isend = PMPC_Isend
#pragma weak MPC_Ibsend = PMPC_Ibsend
#pragma weak MPC_Issend = PMPC_Issend
#pragma weak MPC_Irsend = PMPC_Irsend
#pragma weak MPC_Irecv = PMPC_Irecv
#pragma weak MPC_Wait = PMPC_Wait
#pragma weak MPC_Waitall = PMPC_Waitall
#pragma weak MPC_Waitsome = PMPC_Waitsome
#pragma weak MPC_Waitany = PMPC_Waitany
#pragma weak MPC_Wait_pending = PMPC_Wait_pending
#pragma weak MPC_Wait_pending_all_comm = PMPC_Wait_pending_all_comm
#pragma weak MPC_Test = PMPC_Test
#pragma weak MPC_Test_no_check = PMPC_Test_no_check
#pragma weak MPC_Test_check = PMPC_Test_check
#pragma weak MPC_Iprobe = PMPC_Iprobe
#pragma weak MPC_Probe = PMPC_Probe
#pragma weak MPC_Get_count = PMPC_Get_count
#pragma weak MPC_Send = PMPC_Send
#pragma weak MPC_Bsend = PMPC_Bsend
#pragma weak MPC_Ssend = PMPC_Ssend
#pragma weak MPC_Rsend = PMPC_Rsend
#pragma weak MPC_Recv = PMPC_Recv
#pragma weak MPC_Sendrecv = PMPC_Sendrecv
   /*Status */
#pragma weak MPC_Test_cancelled = PMPC_Test_cancelled
#pragma weak MPC_Cancel = PMPC_Cancel
    /*Gather */
#pragma weak MPC_Gather = PMPC_Gather
#pragma weak MPC_Gatherv = PMPC_Gatherv
#pragma weak MPC_Allgather = PMPC_Allgather
#pragma weak MPC_Allgatherv = PMPC_Allgatherv
   /*Scatter */
#pragma weak MPC_Scatter = PMPC_Scatter
#pragma weak MPC_Scatterv = PMPC_Scatterv
   /*Alltoall */
#pragma weak MPC_Alltoall = PMPC_Alltoall
#pragma weak MPC_Alltoallv = PMPC_Alltoallv
  /*Groups */
#pragma weak MPC_Comm_group = PMPC_Comm_group
#pragma weak MPC_Comm_remote_group = PMPC_Comm_remote_group
#pragma weak MPC_Group_free = PMPC_Group_free
#pragma weak MPC_Group_incl = PMPC_Group_incl
#pragma weak MPC_Group_difference = PMPC_Group_difference
  /*Communicators */
#pragma weak MPC_Convert_to_intercomm = PMPC_Convert_to_intercomm
#pragma weak MPC_Comm_create_list = PMPC_Comm_create_list
#pragma weak MPC_Comm_create = PMPC_Comm_create
#pragma weak MPC_Comm_free = PMPC_Comm_free
#pragma weak MPC_Comm_dup = PMPC_Comm_dup
#pragma weak MPC_Comm_split = PMPC_Comm_split
  /*Error_handler */
#pragma weak MPC_Default_error = PMPC_Default_error
#pragma weak MPC_Return_error = PMPC_Return_error
#pragma weak MPC_Errhandler_create = PMPC_Errhandler_create
#pragma weak MPC_Errhandler_set = PMPC_Errhandler_set
#pragma weak MPC_Errhandler_get = PMPC_Errhandler_get
#pragma weak MPC_Errhandler_free = PMPC_Errhandler_free
#pragma weak MPC_Error_string = PMPC_Error_string
#pragma weak MPC_Error_class = PMPC_Error_class
  /*Timing */
#pragma weak MPC_Wtime = PMPC_Wtime
#pragma weak MPC_Wtick = PMPC_Wtick
  /*Types */
#pragma weak MPC_Type_size = PMPC_Type_size
#pragma weak MPC_Sizeof_datatype = PMPC_Sizeof_datatype
#pragma weak MPC_Type_free = PMPC_Type_free
#pragma weak MPC_Copy_in_buffer = PMPC_Copy_in_buffer
#pragma weak MPC_Copy_from_buffer = PMPC_Copy_from_buffer
#pragma weak MPC_Derived_datatype = PMPC_Derived_datatype
#pragma weak MPC_Is_derived_datatype = PMPC_Is_derived_datatype
#pragma weak MPC_Derived_use = PMPC_Derived_use
#pragma weak MPC_Get_keys = PMPC_Get_keys
#pragma weak MPC_Set_keys = PMPC_Set_keys
#pragma weak MPC_Get_requests = PMPC_Get_requests
#pragma weak MPC_Set_requests = PMPC_Set_requests
#pragma weak MPC_Get_groups = PMPC_Get_groups
#pragma weak MPC_Set_groups = PMPC_Set_groups
#pragma weak MPC_Get_errors = PMPC_Get_errors
#pragma weak MPC_Set_errors = PMPC_Set_errors
#pragma weak MPC_Set_buffers = PMPC_Set_buffers
#pragma weak MPC_Get_buffers = PMPC_Get_buffers
#pragma weak MPC_Get_comm_type = PMPC_Get_comm_type
#pragma weak MPC_Set_comm_type = PMPC_Set_comm_type
#pragma weak MPC_Get_op = PMPC_Get_op
#pragma weak MPC_Set_op = PMPC_Set_op
  /*Requests */
#pragma weak MPC_Request_free = PMPC_Request_free
  /*Scheduling */
#pragma weak MPC_Proceed = PMPC_Proceed
#pragma weak MPC_Checkpoint = PMPC_Checkpoint
#pragma weak MPC_Checkpoint_timed = PMPC_Checkpoint_timed
#pragma weak MPC_Migrate = PMPC_Migrate
#pragma weak MPC_Restart = PMPC_Restart
#pragma weak MPC_Restarted = PMPC_Restarted
#pragma weak MPC_Get_activity = PMPC_Get_activity
#pragma weak MPC_Move_to = PMPC_Move_to
  /*Packs */
#pragma weak MPC_Open_pack = PMPC_Open_pack
#pragma weak MPC_Default_pack = PMPC_Default_pack
#pragma weak MPC_Default_pack_absolute = PMPC_Default_pack_absolute
#pragma weak MPC_Add_pack = PMPC_Add_pack
#pragma weak MPC_Add_pack_absolute = PMPC_Add_pack_absolute
#pragma weak MPC_Add_pack_default = PMPC_Add_pack_default
#pragma weak MPC_Add_pack_default_absolute = PMPC_Add_pack_default_absolute
#pragma weak MPC_Isend_pack = PMPC_Isend_pack
#pragma weak MPC_Irecv_pack = PMPC_Irecv_pack

