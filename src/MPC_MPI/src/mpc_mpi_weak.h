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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - JAEGER julien jaeger.julien@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */
// PROFILER MPI_POINT_TO_POINT MPI point to point
#pragma weak MPI_Send = PMPI_Send
#pragma weak MPI_Recv = PMPI_Recv
#pragma weak MPI_Get_count = PMPI_Get_count
#pragma weak MPI_Bsend = PMPI_Bsend
#pragma weak MPI_Ssend = PMPI_Ssend
#pragma weak MPI_Rsend = PMPI_Rsend
#pragma weak MPI_Buffer_attach = PMPI_Buffer_attach
#pragma weak MPI_Buffer_detach = PMPI_Buffer_detach
#pragma weak MPIX_Swap = PMPIX_Swap
#pragma weak MPIX_Exchange = PMPIX_Exchange
#pragma weak MPI_Isend = PMPI_Isend
#pragma weak MPI_Ibsend = PMPI_Ibsend
#pragma weak MPI_Issend = PMPI_Issend
#pragma weak MPI_Irsend = PMPI_Irsend
#pragma weak MPI_Irecv = PMPI_Irecv
// PROFILER MPI_WAIT MPI Waiting
#pragma weak MPI_Wait = PMPI_Wait
#pragma weak MPI_Test = PMPI_Test
#pragma weak MPI_Request_free = PMPI_Request_free
#pragma weak MPI_Waitany = PMPI_Waitany
#pragma weak MPI_Testany = PMPI_Testany
#pragma weak MPI_Waitall = PMPI_Waitall
#pragma weak MPI_Testall = PMPI_Testall
#pragma weak MPI_Waitsome = PMPI_Waitsome
#pragma weak MPI_Testsome = PMPI_Testsome
#pragma weak MPI_Iprobe = PMPI_Iprobe
#pragma weak MPI_Probe = PMPI_Probe
#pragma weak MPI_Cancel = PMPI_Cancel
// PROFILER MPI_PERSIST MPI Persitant communications
#pragma weak MPI_Send_init = PMPI_Send_init
#pragma weak MPI_Bsend_init = PMPI_Bsend_init
#pragma weak MPI_Ssend_init = PMPI_Ssend_init
#pragma weak MPI_Rsend_init = PMPI_Rsend_init
#pragma weak MPI_Recv_init = PMPI_Recv_init
#pragma weak MPI_Start = PMPI_Start
#pragma weak MPI_Startall = PMPI_Startall
// PROFILER MPI_SENDRECV MPI Sendrecv
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#pragma weak MPI_Sendrecv_replace = PMPI_Sendrecv_replace
// PROFILER MPI_TYPES MPI type related
#pragma weak MPI_Type_contiguous = PMPI_Type_contiguous
#pragma weak MPI_Type_vector = PMPI_Type_vector
#pragma weak MPI_Type_hvector = PMPI_Type_hvector
#pragma weak MPI_Type_create_hvector = PMPI_Type_create_hvector
#pragma weak MPI_Type_indexed = PMPI_Type_indexed
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#pragma weak MPI_Type_create_hindexed = PMPI_Type_create_hindexed
#pragma weak MPI_Type_struct = PMPI_Type_struct
#pragma weak MPI_Type_create_struct = PMPI_Type_create_struct
#pragma weak MPI_Address = PMPI_Address
#pragma weak MPI_Type_extent = PMPI_Type_extent
#pragma weak MPI_Type_size = PMPI_Type_size
#pragma weak MPI_Type_lb = PMPI_Type_lb
#pragma weak MPI_Type_ub = PMPI_Type_ub
#pragma weak MPI_Type_commit = PMPI_Type_commit
#pragma weak MPI_Type_free = PMPI_Type_free
// PROFILER MPI_PACK MPI Pack related
#pragma weak MPI_Pack = PMPI_Pack
#pragma weak MPI_Unpack = PMPI_Unpack
#pragma weak MPI_Pack_size = PMPI_Pack_size
// PROFILER MPI_COLLECTIVES MPI Collective communications
#pragma weak MPI_Barrier = PMPI_Barrier
#pragma weak MPI_Bcast = PMPI_Bcast
#pragma weak MPI_Gather = PMPI_Gather
#pragma weak MPI_Gatherv = PMPI_Gatherv
#pragma weak MPI_Scatter = PMPI_Scatter
#pragma weak MPI_Scatterv = PMPI_Scatterv
#pragma weak MPI_Allgather = PMPI_Allgather
#pragma weak MPI_Allgatherv = PMPI_Allgatherv
#pragma weak MPI_Alltoall = PMPI_Alltoall
#pragma weak MPI_Alltoallv = PMPI_Alltoallv
#pragma weak MPI_Alltoallw = PMPI_Alltoallw
/* Neighbor collectives */
#pragma weak MPI_Neighbor_allgather = PMPI_Neighbor_allgather
#pragma weak MPI_Neighbor_allgatherv = PMPI_Neighbor_allgatherv
#pragma weak MPI_Neighbor_alltoall = PMPI_Neighbor_alltoall
#pragma weak MPI_Neighbor_alltoallv = PMPI_Neighbor_alltoallv
#pragma weak MPI_Neighbor_alltoallw = PMPI_Neighbor_alltoallw
#pragma weak MPI_Reduce = PMPI_Reduce
#pragma weak MPI_Op_create = PMPI_Op_create
#pragma weak MPI_Op_commutative = PMPI_Op_commutative
#pragma weak MPI_Op_free = PMPI_Op_free
#pragma weak MPI_Allreduce = PMPI_Allreduce
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#pragma weak MPI_Reduce_scatter_block = PMPI_Reduce_scatter_block
#pragma weak MPI_Scan = PMPI_Scan
#pragma weak MPI_Exscan = PMPI_Exscan
// PROFILER MPI_GROUP MPI Group operation
#pragma weak MPI_Group_size = PMPI_Group_size
#pragma weak MPI_Group_rank = PMPI_Group_rank
#pragma weak MPI_Group_translate_ranks = PMPI_Group_translate_ranks
#pragma weak MPI_Group_compare = PMPI_Group_compare
#pragma weak MPI_Comm_group = PMPI_Comm_group
#pragma weak MPI_Group_union = PMPI_Group_union
#pragma weak MPI_Group_intersection = PMPI_Group_intersection
#pragma weak MPI_Group_difference = PMPI_Group_difference
#pragma weak MPI_Group_incl = PMPI_Group_incl
#pragma weak MPI_Group_excl = PMPI_Group_excl
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#pragma weak MPI_Group_range_excl = PMPI_Group_range_excl
#pragma weak MPI_Group_free = PMPI_Group_free
// PROFILER MPI_COMM MPI Communicator operation
#pragma weak MPI_Comm_size = PMPI_Comm_size
#pragma weak MPI_Comm_rank = PMPI_Comm_rank
#pragma weak MPI_Comm_compare = PMPI_Comm_compare
#pragma weak MPI_Comm_dup = PMPI_Comm_dup
#pragma weak MPI_Comm_create = PMPI_Comm_create
#pragma weak MPI_Comm_split = PMPI_Comm_split
#pragma weak MPI_Comm_free = PMPI_Comm_free
#pragma weak MPI_Comm_test_inter = PMPI_Comm_test_inter
#pragma weak MPI_Comm_remote_size = PMPI_Comm_remote_size
#pragma weak MPI_Comm_remote_group = PMPI_Comm_remote_group
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
// PROFILER MPI_KEYS MPI keys operations
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#pragma weak MPI_Attr_put = PMPI_Attr_put
#pragma weak MPI_Attr_get = PMPI_Attr_get
#pragma weak MPI_Attr_delete = PMPI_Attr_delete
// PROFILER MPI_TOPO MPI Topology operations
#pragma weak MPI_Topo_test = PMPI_Topo_test
#pragma weak MPI_Cart_create = PMPI_Cart_create
#pragma weak MPI_Dims_create = PMPI_Dims_create
#pragma weak MPI_Graph_create = PMPI_Graph_create
#pragma weak MPI_Graphdims_get = PMPI_Graphdims_get
#pragma weak MPI_Graph_get = PMPI_Graph_get
#pragma weak MPI_Cartdim_get = PMPI_Cartdim_get
#pragma weak MPI_Cart_get = PMPI_Cart_get
#pragma weak MPI_Cart_rank = PMPI_Cart_rank
#pragma weak MPI_Cart_coords = PMPI_Cart_coords
#pragma weak MPI_Graph_neighbors_count = PMPI_Graph_neighbors_count
#pragma weak MPI_Graph_neighbors = PMPI_Graph_neighbors
#pragma weak MPI_Cart_shift = PMPI_Cart_shift
#pragma weak MPI_Cart_sub = PMPI_Cart_sub
#pragma weak MPI_Cart_map = PMPI_Cart_map
#pragma weak MPI_Graph_map = PMPI_Graph_map
#pragma weak MPI_Get_processor_name = PMPI_Get_processor_name
#pragma weak MPI_Get_version = PMPI_Get_version
// PROFILER MPI_ERROR MPI Errors operations
#pragma weak MPI_Errhandler_create = PMPI_Errhandler_create
#pragma weak MPI_Errhandler_set = PMPI_Errhandler_set
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#pragma weak MPI_Errhandler_free = PMPI_Errhandler_free
#pragma weak MPI_Error_string = PMPI_Error_string
#pragma weak MPI_Error_class = PMPI_Error_class
// PROFILER MPI_TIME MPI_Timing operations
#pragma weak MPI_Wtime = PMPI_Wtime
#pragma weak MPI_Wtick = PMPI_Wtick
#pragma weak MPI_Init = PMPI_Init
// PROFILER MPI_INIT_FINALIZE MPI Env operations
#pragma weak MPI_Finalize = PMPI_Finalize
#pragma weak MPI_Finalized = PMPI_Finalized
#pragma weak MPI_Initialized = PMPI_Initialized
#pragma weak MPI_Abort = PMPI_Abort
#pragma weak MPI_Pcontrol = PMPI_Pcontrol
#pragma weak MPI_Comm_get_attr = PMPI_Attr_get
#pragma weak MPI_Comm_get_name = PMPI_Comm_get_name
#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name
#pragma weak MPI_Get_address = PMPI_Get_address
#pragma weak MPI_Init_thread = PMPI_Init_thread
#pragma weak MPI_Query_thread = PMPI_Query_thread
// PROFILER MPI_INFO MPI Info operations
#pragma weak MPI_Info_create = PMPI_Info_create
#pragma weak MPI_Info_delete = PMPI_Info_delete
#pragma weak MPI_Info_dup = PMPI_Info_dup
#pragma weak MPI_Info_free = PMPI_Info_free
#pragma weak MPI_Info_set = PMPI_Info_set
#pragma weak MPI_Info_get = PMPI_Info_get
#pragma weak MPI_Info_get_string = PMPI_Info_get_string
#pragma weak MPI_Info_get_nkeys = PMPI_Info_get_nkeys
#pragma weak MPI_Info_get_nthkey = PMPI_Info_get_nthkey
#pragma weak MPI_Info_get_valuelen = PMPI_Info_get_valuelen
// PROFILER MPI_GREQUEST MPI geral requests operations
#pragma weak MPI_Grequest_start = PMPI_Grequest_start
#pragma weak MPI_Grequest_complete = PMPI_Grequest_complete
#pragma weak MPIX_Grequest_start = PMPIX_Grequest_start
#pragma weak MPIX_Grequest_class_create = PMPIX_Grequest_class_create
#pragma weak MPIX_Grequest_class_allocate = PMPIX_Grequest_class_allocate
#pragma weak MPI_Status_set_elements = PMPI_Status_set_elements
#pragma weak MPI_Status_set_elements_x = PMPI_Status_set_elements_x
#pragma weak MPI_Status_set_cancelled = PMPI_Status_set_cancelled
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
// PROFILER MPI_OTHER MPI other operations
#pragma weak MPI_Test_cancelled = PMPI_Test_cancelled
#pragma weak MPI_Type_set_name = PMPI_Type_set_name
#pragma weak MPI_Type_get_name = PMPI_Type_get_name
#pragma weak MPI_Type_dup = PMPI_Type_dup
#pragma weak MPI_Type_get_envelope = PMPI_Type_get_envelope
#pragma weak MPI_Type_get_contents = PMPI_Type_get_contents
#pragma weak MPI_Type_get_extent = PMPI_Type_get_extent
#pragma weak MPI_Type_get_true_extent = PMPI_Type_get_true_extent
#pragma weak MPI_Type_create_resized = PMPI_Type_create_resized
#pragma weak MPI_Type_create_hindexed_block = PMPI_Type_create_hindexed_block
#pragma weak MPI_Type_create_indexed_block = PMPI_Type_create_indexed_block
#pragma weak MPI_Type_match_size = PMPI_Type_match_size
#pragma weak MPI_Type_size_x = PMPI_Type_size_x
#pragma weak MPI_Type_get_extent_x = PMPI_Type_get_extent_x
#pragma weak MPI_Type_get_true_extent_x = PMPI_Type_get_true_extent_x
#pragma weak MPI_Type_get_elements_x = PMPI_Type_get_elements_x
#pragma weak MPI_Type_get_elements = PMPI_Type_get_elements
#pragma weak MPI_Get_elements_x = PMPI_Get_elements_x
#pragma weak MPI_Get_elements = PMPI_Get_elements
#pragma weak MPI_Type_create_darray = PMPI_Type_create_darray
#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#pragma weak MPI_Pack_external_size = PMPI_Pack_external_size
#pragma weak MPI_Pack_external = PMPI_Pack_external
#pragma weak MPI_Unpack_external = PMPI_Unpack_external
#pragma weak MPI_Free_mem = PMPI_Free_mem
#pragma weak MPI_Alloc_mem = PMPI_Alloc_mem
#pragma weak MPI_Type_create_keyval = PMPI_Type_create_keyval
#pragma weak MPI_Type_set_attr = PMPI_Type_set_attr
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#pragma weak MPI_Type_free_keyval = PMPI_Type_free_keyval
#pragma weak  MPIX_Halo_cell_init = PMPIX_Halo_cell_init
#pragma weak  MPIX_Halo_cell_release = PMPIX_Halo_cell_release
#pragma weak  MPIX_Halo_cell_set = PMPIX_Halo_cell_set
#pragma weak  MPIX_Halo_cell_get = PMPIX_Halo_cell_get
#pragma weak  MPIX_Halo_exchange_init = PMPIX_Halo_exchange_init
#pragma weak  MPIX_Halo_exchange_release = PMPIX_Halo_exchange_release
#pragma weak  MPIX_Halo_exchange_commit = PMPIX_Halo_exchange_commit
#pragma weak  MPIX_Halo_exchange = PMPIX_Halo_exchange
#pragma weak  MPIX_Halo_iexchange = PMPIX_Halo_iexchange
#pragma weak  MPIX_Halo_iexchange_wait = PMPIX_Halo_iexchange_wait
#pragma weak  MPIX_Halo_cell_bind_local = PMPIX_Halo_cell_bind_local
#pragma weak  MPIX_Halo_cell_bind_remote = PMPIX_Halo_cell_bind_remote
/* checkpointing routines */
#pragma weak MPIX_Checkpoint = PMPIX_Checkpoint
// PROFILER MPI_ONE_SIDED MPI One-sided communications
/* Communicator Management */
// PROFILER MPI_COMM_MANAGE MPI Communicator Management
#pragma weak MPI_Comm_create_keyval = PMPI_Comm_create_keyval
#pragma weak MPI_Comm_delete_attr = PMPI_Comm_delete_attr
#pragma weak MPI_Comm_free_keyval = PMPI_Comm_free_keyval
#pragma weak MPI_Comm_set_attr = PMPI_Comm_set_attr
#pragma weak MPI_Comm_create_errhandler = PMPI_Comm_create_errhandler
#pragma weak MPI_Comm_dup_with_info = PMPI_Comm_dup_with_info
#pragma weak MPI_Comm_split_type = PMPI_Comm_split_type
#pragma weak MPI_Comm_set_info = PMPI_Comm_set_info
#pragma weak MPI_Comm_get_info = PMPI_Comm_get_info
#pragma weak MPI_Comm_idup = PMPI_Comm_idup
#pragma weak MPI_Comm_create_group = PMPI_Comm_create_group
#pragma weak MPI_Comm_get_errhandler = PMPI_Comm_get_errhandler
#pragma weak MPI_Comm_call_errhandler = PMPI_Comm_call_errhandler
#pragma weak MPI_Comm_set_errhandler = PMPI_Comm_set_errhandler
#pragma weak MPIX_Get_hwsubdomain_types = PMPIX_Get_hwsubdomain_types
// PROFILER MPI_TYPE_ENV MPI datatype handling
/* MPI environmental management */
// PROFILER MPI_ENV_MAN MPI environmental management
#pragma weak MPI_Add_error_class = PMPI_Add_error_class
#pragma weak MPI_Add_error_code = PMPI_Add_error_code
#pragma weak MPI_Add_error_string = PMPI_Add_error_string
#pragma weak MPI_Is_thread_main = PMPI_Is_thread_main
#pragma weak MPI_Get_library_version = PMPI_Get_library_version
/* Process creation and Management */
// PROFILER MPI_PROCESS_CREATE Process creation and Management
#pragma weak MPI_Close_port = PMPI_Close_port
#pragma weak MPI_Comm_accept = PMPI_Comm_accept
#pragma weak MPI_Comm_connect = PMPI_Comm_connect
#pragma weak MPI_Comm_disconnect = PMPI_Comm_disconnect
#pragma weak MPI_Comm_get_parent = PMPI_Comm_get_parent
#pragma weak MPI_Comm_join = PMPI_Comm_join
#pragma weak MPI_Comm_spawn = PMPI_Comm_spawn
#pragma weak MPI_Comm_spawn_multiple = PMPI_Comm_spawn_multiple
#pragma weak MPI_Lookup_name = PMPI_Lookup_name
#pragma weak MPI_Open_port = PMPI_Open_port
#pragma weak MPI_Publish_name = PMPI_Publish_name
#pragma weak MPI_Unpublish_name = PMPI_Unpublish_name
/* Dist graph operations */
// PROFILER MPI_DIST_GRAPH MPI Dist graph operations
#pragma weak MPI_Dist_graph_neighbors_count = PMPI_Dist_graph_neighbors_count
#pragma weak MPI_Dist_graph_neighbors = PMPI_Dist_graph_neighbors
#pragma weak MPI_Dist_graph_create = PMPI_Dist_graph_create
#pragma weak MPI_Dist_graph_create_adjacent = PMPI_Dist_graph_create_adjacent
/* collectives */
#pragma weak MPI_Reduce_local = PMPI_Reduce_local
/* FORTRAN TYPE */
#pragma weak MPI_Type_create_f90_complex = PMPI_Type_create_f90_complex
#pragma weak MPI_Type_create_f90_integer = PMPI_Type_create_f90_integer
#pragma weak MPI_Type_create_f90_real = PMPI_Type_create_f90_real

//-----
//-----
/* MPIX methods */
#pragma weak MPIX_Comm_failure_ack = PMPIX_Comm_failure_ack
#pragma weak MPIX_Comm_failure_get_acked = PMPIX_Comm_failure_get_acked
#pragma weak MPIX_Comm_agree = PMPIX_Comm_agree
#pragma weak MPIX_Comm_revoke = PMPIX_Comm_revoke
#pragma weak MPIX_Comm_shrink = PMPIX_Comm_shrink
/* probe and cancel */
// PROFILER MPI_PROBE_CANCEL MPI probe and cancel
#pragma weak MPI_Mprobe = PMPI_Mprobe
#pragma weak MPI_Mrecv = PMPI_Mrecv
#pragma weak MPI_Improbe = PMPI_Improbe
#pragma weak MPI_Imrecv = PMPI_Imrecv
/* One-Sided Communications */
#pragma weak MPI_Win_set_attr = PMPI_Win_set_attr
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#pragma weak MPI_Win_free_keyval = PMPI_Win_free_keyval
#pragma weak MPI_Win_delete_attr = PMPI_Win_delete_attr
#pragma weak MPI_Win_create_keyval = PMPI_Win_create_keyval
#pragma weak MPI_Win_create = PMPI_Win_create
#pragma weak MPI_Win_free = PMPI_Win_free
#pragma weak MPI_Win_wait = PMPI_Win_wait
#pragma weak MPI_Accumulate = PMPI_Accumulate
#pragma weak MPI_Get = PMPI_Get
#pragma weak MPI_Put = PMPI_Put
#pragma weak MPI_Win_complete = PMPI_Win_complete
#pragma weak MPI_Win_fence = PMPI_Win_fence
#pragma weak MPI_Win_get_group = PMPI_Win_get_group
#pragma weak MPI_Win_lock = PMPI_Win_lock
#pragma weak MPI_Win_post = PMPI_Win_post
#pragma weak MPI_Win_start = PMPI_Win_start
#pragma weak MPI_Win_test = PMPI_Win_test
#pragma weak MPI_Win_unlock = PMPI_Win_unlock
#pragma weak MPI_Win_allocate = PMPI_Win_allocate
#pragma weak MPI_Win_set_name = PMPI_Win_set_name
#pragma weak MPI_Win_get_name = PMPI_Win_get_name
#pragma weak MPI_Win_set_errhandler = PMPI_Win_set_errhandler
#pragma weak MPI_Win_get_errhandler = PMPI_Win_get_errhandler
#pragma weak MPI_Win_call_errhandler = PMPI_Win_call_errhandler
#pragma weak MPI_Win_allocate_shared = PMPI_Win_allocate_shared
#pragma weak MPI_Win_create_errhandler = PMPI_Win_create_errhandler
#pragma weak MPI_Win_create_dynamic = PMPI_Win_create_dynamic
#pragma weak MPI_Win_shared_query = PMPI_Win_shared_query
#pragma weak MPI_Win_lock_all = PMPI_Win_lock_all
#pragma weak MPI_Win_unlock_all = PMPI_Win_unlock_all
#pragma weak MPI_Win_sync = PMPI_Win_sync
#pragma weak MPI_Win_attach = PMPI_Win_attach
#pragma weak MPI_Win_detach = PMPI_Win_detach
#pragma weak MPI_Win_flush = PMPI_Win_flush
#pragma weak MPI_Win_flush_all = PMPI_Win_flush_all
#pragma weak MPI_Win_flush_local_all = PMPI_Win_flush_local_all
#pragma weak MPI_Win_flush_local = PMPI_Win_flush_local
#pragma weak MPI_Win_set_info = PMPI_Win_set_info
#pragma weak MPI_Win_get_info = PMPI_Win_get_info
#pragma weak MPI_Get_accumulate = PMPI_Get_accumulate
#pragma weak MPI_Fetch_and_op = PMPI_Fetch_and_op
#pragma weak MPI_Compare_and_swap = PMPI_Compare_and_swap
#pragma weak MPI_Rput = PMPI_Rput
#pragma weak MPI_Rget = PMPI_Rget
#pragma weak MPI_Raccumulate = PMPI_Raccumulate
#pragma weak MPI_Rget_accumulate = PMPI_Rget_accumulate
#pragma weak MPI_File_create_errhandler = PMPI_File_create_errhandler
#pragma weak MPI_File_set_errhandler = PMPI_File_set_errhandler
#pragma weak MPI_File_get_errhandler = PMPI_File_get_errhandler
#pragma weak MPI_File_call_errhandler = PMPI_File_call_errhandler

/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/

#pragma weak MPI_Neighbor_allgatherv_init = PMPI_Neighbor_allgatherv_init
#pragma weak MPI_Ineighbor_alltoallw = PMPI_Ineighbor_alltoallw
#pragma weak MPI_Ineighbor_alltoallv = PMPI_Ineighbor_alltoallv
#pragma weak MPI_Ineighbor_alltoall = PMPI_Ineighbor_alltoall
#pragma weak MPI_Ineighbor_allgatherv = PMPI_Ineighbor_allgatherv
#pragma weak MPI_Ineighbor_allgather = PMPI_Ineighbor_allgather
#pragma weak MPI_Barrier_init = PMPI_Barrier_init
#pragma weak MPI_Register_datarep = PMPI_Register_datarep
#pragma weak MPI_Comm_idup_with_info = PMPI_Comm_idup_with_info
#pragma weak MPI_Aint_diff = PMPI_Aint_diff
#pragma weak MPI_Aint_add = PMPI_Aint_add
#pragma weak MPI_Neighbor_alltoallv_init = PMPI_Neighbor_alltoallv_init
#pragma weak MPI_Neighbor_alltoallw_init = PMPI_Neighbor_alltoallw_init
#pragma weak MPI_Neighbor_alltoall_init = PMPI_Neighbor_alltoall_init
#pragma weak MPI_Neighbor_allgather_init = PMPI_Neighbor_allgather_init
#pragma weak MPI_Reduce_scatter_init = PMPI_Reduce_scatter_init

