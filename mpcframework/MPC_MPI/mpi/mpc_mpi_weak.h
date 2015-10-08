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

#pragma weak MPI_Send = PMPI_Send
#pragma weak mpi_send_ = pmpi_send_
#pragma weak mpi_send__ = pmpi_send__
#pragma weak MPI_Recv = PMPI_Recv
#pragma weak mpi_recv_ = pmpi_recv_
#pragma weak mpi_recv__ = pmpi_recv__
#pragma weak MPI_Get_count = PMPI_Get_count
#pragma weak mpi_get_count_ = pmpi_get_count_
#pragma weak mpi_get_count__ = pmpi_get_count__
#pragma weak MPI_Bsend = PMPI_Bsend
#pragma weak mpi_bsend_ = pmpi_bsend_
#pragma weak mpi_bsend__ = pmpi_bsend__
#pragma weak MPI_Ssend = PMPI_Ssend
#pragma weak mpi_ssend_ = pmpi_ssend_
#pragma weak mpi_ssend__ = pmpi_ssend__
#pragma weak MPI_Rsend = PMPI_Rsend
#pragma weak mpi_rsend_ = pmpi_rsend_
#pragma weak mpi_rsend__ = pmpi_rsend__
#pragma weak MPI_Buffer_attach = PMPI_Buffer_attach
#pragma weak mpi_buffer_attach_ = pmpi_buffer_attach_
#pragma weak mpi_buffer_attach__ = pmpi_buffer_attach__
#pragma weak MPI_Buffer_detach = PMPI_Buffer_detach
#pragma weak mpi_buffer_detach_ = pmpi_buffer_detach_
#pragma weak mpi_buffer_detach__ = pmpi_buffer_detach__
#pragma weak MPI_Isend = PMPI_Isend
#pragma weak mpi_isend_ = pmpi_isend_
#pragma weak mpi_isend__ = pmpi_isend__
#pragma weak MPI_Ibsend = PMPI_Ibsend
#pragma weak mpi_ibsend_ = pmpi_ibsend_
#pragma weak mpi_ibsend__ = pmpi_ibsend__
#pragma weak MPI_Issend = PMPI_Issend
#pragma weak mpi_issend_ = pmpi_issend_
#pragma weak mpi_issend__ = pmpi_issend__
#pragma weak MPI_Irsend = PMPI_Irsend
#pragma weak mpi_irsend_ = pmpi_irsend_
#pragma weak mpi_irsend__ = pmpi_irsend__
#pragma weak MPI_Irecv = PMPI_Irecv
#pragma weak mpi_irecv_ = pmpi_irecv_
#pragma weak mpi_irecv__ = pmpi_irecv__
#pragma weak MPI_Wait = PMPI_Wait
#pragma weak mpi_wait_ = pmpi_wait_
#pragma weak mpi_wait__ = pmpi_wait__
#pragma weak MPI_Test = PMPI_Test
#pragma weak mpi_test_ = pmpi_test_
#pragma weak mpi_test__ = pmpi_test__
#pragma weak MPI_Request_free = PMPI_Request_free
#pragma weak mpi_request_free_ = pmpi_request_free_
#pragma weak mpi_request_free__ = pmpi_request_free__
#pragma weak MPI_Waitany = PMPI_Waitany
#pragma weak mpi_waitany_ = pmpi_waitany_
#pragma weak mpi_waitany__ = pmpi_waitany__
#pragma weak MPI_Testany = PMPI_Testany
#pragma weak mpi_testany_ = pmpi_testany_
#pragma weak mpi_testany__ = pmpi_testany__
#pragma weak MPI_Waitall = PMPI_Waitall
#pragma weak mpi_waitall_ = pmpi_waitall_
#pragma weak mpi_waitall__ = pmpi_waitall__
#pragma weak MPI_Testall = PMPI_Testall
#pragma weak mpi_testall_ = pmpi_testall_
#pragma weak mpi_testall__ = pmpi_testall__
#pragma weak MPI_Waitsome = PMPI_Waitsome
#pragma weak mpi_waitsome_ = pmpi_waitsome_
#pragma weak mpi_waitsome__ = pmpi_waitsome__
#pragma weak MPI_Testsome = PMPI_Testsome
#pragma weak mpi_testsome_ = pmpi_testsome_
#pragma weak mpi_testsome__ = pmpi_testsome__
#pragma weak MPI_Iprobe = PMPI_Iprobe
#pragma weak mpi_iprobe_ = pmpi_iprobe_
#pragma weak mpi_iprobe__ = pmpi_iprobe__
#pragma weak MPI_Probe = PMPI_Probe
#pragma weak mpi_probe_ = pmpi_probe_
#pragma weak mpi_probe__ = pmpi_probe__
#pragma weak MPI_Cancel = PMPI_Cancel
#pragma weak mpi_cancel_ = pmpi_cancel_
#pragma weak mpi_cancel__ = pmpi_cancel__
#pragma weak MPI_Send_init = PMPI_Send_init
#pragma weak mpi_send_init_ = pmpi_send_init_
#pragma weak mpi_send_init__ = pmpi_send_init__
#pragma weak MPI_Bsend_init = PMPI_Bsend_init
#pragma weak mpi_bsend_init_ = pmpi_bsend_init_
#pragma weak mpi_bsend_init__ = pmpi_bsend_init__
#pragma weak MPI_Ssend_init = PMPI_Ssend_init
#pragma weak mpi_ssend_init_ = pmpi_ssend_init_
#pragma weak mpi_ssend_init__ = pmpi_ssend_init__
#pragma weak MPI_Rsend_init = PMPI_Rsend_init
#pragma weak mpi_rsend_init_ = pmpi_rsend_init_
#pragma weak mpi_rsend_init__ = pmpi_rsend_init__
#pragma weak MPI_Recv_init = PMPI_Recv_init
#pragma weak mpi_recv_init_ = pmpi_recv_init_
#pragma weak mpi_recv_init__ = pmpi_recv_init__
#pragma weak MPI_Start = PMPI_Start
#pragma weak mpi_start_ = pmpi_start_
#pragma weak mpi_start__ = pmpi_start__
#pragma weak MPI_Startall = PMPI_Startall
#pragma weak mpi_startall_ = pmpi_startall_
#pragma weak mpi_startall__ = pmpi_startall__
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#pragma weak mpi_sendrecv_ = pmpi_sendrecv_
#pragma weak mpi_sendrecv__ = pmpi_sendrecv__
#pragma weak MPI_Sendrecv_replace = PMPI_Sendrecv_replace
#pragma weak mpi_sendrecv_replace_ = pmpi_sendrecv_replace_
#pragma weak mpi_sendrecv_replace__ = pmpi_sendrecv_replace__
#pragma weak MPI_Type_contiguous = PMPI_Type_contiguous
#pragma weak mpi_type_contiguous_ = pmpi_type_contiguous_
#pragma weak mpi_type_contiguous__ = pmpi_type_contiguous__
#pragma weak MPI_Type_vector = PMPI_Type_vector
#pragma weak mpi_type_vector_ = pmpi_type_vector_
#pragma weak mpi_type_vector__ = pmpi_type_vector__
#pragma weak MPI_Type_hvector = PMPI_Type_hvector
#pragma weak mpi_type_hvector_ = pmpi_type_hvector_
#pragma weak mpi_type_hvector__ = pmpi_type_hvector__
#pragma weak MPI_Type_create_hvector = PMPI_Type_create_hvector
#pragma weak mpi_type_create_hvector_ = pmpi_type_create_hvector_
#pragma weak mpi_type_create_hvector__ = pmpi_type_create_hvector__
#pragma weak MPI_Type_indexed = PMPI_Type_indexed
#pragma weak mpi_type_indexed_ = pmpi_type_indexed_
#pragma weak mpi_type_indexed__ = pmpi_type_indexed__
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#pragma weak mpi_type_hindexed_ = pmpi_type_hindexed_
#pragma weak mpi_type_hindexed__ = pmpi_type_hindexed__
#pragma weak MPI_Type_create_hindexed = PMPI_Type_create_hindexed
#pragma weak mpi_type_create_hindexed_ = pmpi_type_create_hindexed_
#pragma weak mpi_type_create_hindexed__ = pmpi_type_create_hindexed__
#pragma weak MPI_Type_struct = PMPI_Type_struct
#pragma weak mpi_type_struct_ = pmpi_type_struct_
#pragma weak mpi_type_struct__ = pmpi_type_struct__
#pragma weak MPI_Type_create_struct = PMPI_Type_create_struct
#pragma weak mpi_type_create_struct_ = pmpi_type_create_struct_
#pragma weak mpi_type_create_struct__ = pmpi_type_create_struct__
#pragma weak MPI_Address = PMPI_Address
#pragma weak mpi_address_ = pmpi_address_
#pragma weak mpi_address__ = pmpi_address__
#pragma weak MPI_Type_extent = PMPI_Type_extent
#pragma weak mpi_type_extent_ = pmpi_type_extent_
#pragma weak mpi_type_extent__ = pmpi_type_extent__
#pragma weak MPI_Type_size = PMPI_Type_size
#pragma weak mpi_type_size_ = pmpi_type_size_
#pragma weak mpi_type_size__ = pmpi_type_size__
#pragma weak MPI_Type_lb = PMPI_Type_lb
#pragma weak mpi_type_lb_ = pmpi_type_lb_
#pragma weak mpi_type_lb__ = pmpi_type_lb__
#pragma weak MPI_Type_ub = PMPI_Type_ub
#pragma weak mpi_type_ub_ = pmpi_type_ub_
#pragma weak mpi_type_ub__ = pmpi_type_ub__
#pragma weak MPI_Type_commit = PMPI_Type_commit
#pragma weak mpi_type_commit_ = pmpi_type_commit_
#pragma weak mpi_type_commit__ = pmpi_type_commit__
#pragma weak MPI_Type_free = PMPI_Type_free
#pragma weak mpi_type_free_ = pmpi_type_free_
#pragma weak mpi_type_free__ = pmpi_type_free__
#pragma weak MPI_Get_elements = PMPI_Get_elements
#pragma weak mpi_get_elements_ = pmpi_get_elements_
#pragma weak mpi_get_elements__ = pmpi_get_elements__
#pragma weak MPI_Pack = PMPI_Pack
#pragma weak mpi_pack_ = pmpi_pack_
#pragma weak mpi_pack__ = pmpi_pack__
#pragma weak MPI_Unpack = PMPI_Unpack
#pragma weak mpi_unpack_ = pmpi_unpack_
#pragma weak mpi_unpack__ = pmpi_unpack__
#pragma weak MPI_Pack_size = PMPI_Pack_size
#pragma weak mpi_pack_size_ = pmpi_pack_size_
#pragma weak mpi_pack_size__ = pmpi_pack_size__
#pragma weak MPI_Barrier = PMPI_Barrier
#pragma weak mpi_barrier_ = pmpi_barrier_
#pragma weak mpi_barrier__ = pmpi_barrier__
#pragma weak MPI_Bcast = PMPI_Bcast
#pragma weak mpi_bcast_ = pmpi_bcast_
#pragma weak mpi_bcast__ = pmpi_bcast__
#pragma weak MPI_Gather = PMPI_Gather
#pragma weak mpi_gather_ = pmpi_gather_
#pragma weak mpi_gather__ = pmpi_gather__
#pragma weak MPI_Gatherv = PMPI_Gatherv
#pragma weak mpi_gatherv_ = pmpi_gatherv_
#pragma weak mpi_gatherv__ = pmpi_gatherv__
#pragma weak MPI_Scatter = PMPI_Scatter
#pragma weak mpi_scatter_ = pmpi_scatter_
#pragma weak mpi_scatter__ = pmpi_scatter__
#pragma weak MPI_Scatterv = PMPI_Scatterv
#pragma weak mpi_scatterv_ = pmpi_scatterv_
#pragma weak mpi_scatterv__ = pmpi_scatterv__
#pragma weak MPI_Allgather = PMPI_Allgather
#pragma weak mpi_allgather_ = pmpi_allgather_
#pragma weak mpi_allgather__ = pmpi_allgather__
#pragma weak MPI_Allgatherv = PMPI_Allgatherv
#pragma weak mpi_allgatherv_ = pmpi_allgatherv_
#pragma weak mpi_allgatherv__ = pmpi_allgatherv__
#pragma weak MPI_Alltoall = PMPI_Alltoall
#pragma weak mpi_alltoall_ = pmpi_alltoall_
#pragma weak mpi_alltoall__ = pmpi_alltoall__
#pragma weak MPI_Alltoallv = PMPI_Alltoallv
#pragma weak mpi_alltoallv_ = pmpi_alltoallv_
#pragma weak mpi_alltoallv__ = pmpi_alltoallv__
#pragma weak MPI_Alltoallw = PMPI_Alltoallw
#pragma weak mpi_alltoallw_ = pmpi_alltoallw_
#pragma weak mpi_alltoallw__ = pmpi_alltoallw__

/* Neighbor collectives */
#pragma weak MPI_Neighbor_allgather = PMPI_Neighbor_allgather
#pragma weak mpi_neighbor_allgather_ = pmpi_neighbor_allgather_
#pragma weak mpi_neighbor_allgather__ = pmpi_neighbor_allgather__
#pragma weak MPI_Neighbor_allgatherv = PMPI_Neighbor_allgatherv
#pragma weak mpi_neighbor_allgatherv_ = pmpi_neighbor_allgatherv_
#pragma weak mpi_neighbor_allgatherv__ = pmpi_neighbor_allgatherv__
#pragma weak MPI_Neighbor_alltoall = PMPI_Neighbor_alltoall
#pragma weak mpi_neighbor_alltoall_ = pmpi_neighbor_alltoall_
#pragma weak mpi_neighbor_alltoall__ = pmpi_neighbor_alltoall__
#pragma weak MPI_Neighbor_alltoallv = PMPI_Neighbor_alltoallv
#pragma weak mpi_neighbor_alltoallv_ = pmpi_neighbor_alltoallv_
#pragma weak mpi_neighbor_alltoallv__ = pmpi_neighbor_alltoallv__
#pragma weak MPI_Neighbor_alltoallw = PMPI_Neighbor_alltoallw
#pragma weak mpi_neighbor_alltoallw_ = pmpi_neighbor_alltoallw_
#pragma weak mpi_neighbor_alltoallw__ = pmpi_neighbor_alltoallw__

#pragma weak MPI_Reduce = PMPI_Reduce
#pragma weak mpi_reduce_ = pmpi_reduce_
#pragma weak mpi_reduce__ = pmpi_reduce__
#pragma weak MPI_Op_create = PMPI_Op_create
#pragma weak mpi_op_create_ = pmpi_op_create_
#pragma weak mpi_op_create__ = pmpi_op_create__
#pragma weak MPI_Op_free = PMPI_Op_free
#pragma weak mpi_op_free_ = pmpi_op_free_
#pragma weak mpi_op_free__ = pmpi_op_free__
#pragma weak MPI_Allreduce = PMPI_Allreduce
#pragma weak mpi_allreduce_ = pmpi_allreduce_
#pragma weak mpi_allreduce__ = pmpi_allreduce__
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#pragma weak mpi_reduce_scatter_ = pmpi_reduce_scatter_
#pragma weak mpi_reduce_scatter__ = pmpi_reduce_scatter__
#pragma weak MPI_Reduce_scatter_block = PMPI_Reduce_scatter_block
#pragma weak mpi_reduce_scatter_block_ = pmpi_reduce_scatter_block_
#pragma weak mpi_reduce_scatter_block__ = pmpi_reduce_scatter_block__
#pragma weak MPI_Scan = PMPI_Scan
#pragma weak mpi_scan_ = pmpi_scan_
#pragma weak mpi_scan__ = pmpi_scan__
#pragma weak MPI_Exscan = PMPI_Exscan
#pragma weak mpi_exscan_ = pmpi_exscan_
#pragma weak mpi_exscan__ = pmpi_exscan__
#pragma weak MPI_Group_size = PMPI_Group_size
#pragma weak mpi_group_size_ = pmpi_group_size_
#pragma weak mpi_group_size__ = pmpi_group_size__
#pragma weak MPI_Group_rank = PMPI_Group_rank
#pragma weak mpi_group_rank_ = pmpi_group_rank_
#pragma weak mpi_group_rank__ = pmpi_group_rank__
#pragma weak MPI_Group_translate_ranks = PMPI_Group_translate_ranks
#pragma weak mpi_group_translate_ranks_ = pmpi_group_translate_ranks_
#pragma weak mpi_group_translate_ranks__ = pmpi_group_translate_ranks__
#pragma weak MPI_Group_compare = PMPI_Group_compare
#pragma weak mpi_group_compare_ = pmpi_group_compare_
#pragma weak mpi_group_compare__ = pmpi_group_compare__
#pragma weak MPI_Comm_group = PMPI_Comm_group
#pragma weak mpi_comm_group_ = pmpi_comm_group_
#pragma weak mpi_comm_group__ = pmpi_comm_group__
#pragma weak MPI_Group_union = PMPI_Group_union
#pragma weak mpi_group_union_ = pmpi_group_union_
#pragma weak mpi_group_union__ = pmpi_group_union__
#pragma weak MPI_Group_intersection = PMPI_Group_intersection
#pragma weak mpi_group_intersection_ = pmpi_group_intersection_
#pragma weak mpi_group_intersection__ = pmpi_group_intersection__
#pragma weak MPI_Group_difference = PMPI_Group_difference
#pragma weak mpi_group_difference_ = pmpi_group_difference_
#pragma weak mpi_group_difference__ = pmpi_group_difference__
#pragma weak MPI_Group_incl = PMPI_Group_incl
#pragma weak mpi_group_incl_ = pmpi_group_incl_
#pragma weak mpi_group_incl__ = pmpi_group_incl__
#pragma weak MPI_Group_excl = PMPI_Group_excl
#pragma weak mpi_group_excl_ = pmpi_group_excl_
#pragma weak mpi_group_excl__ = pmpi_group_excl__
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#pragma weak mpi_group_range_incl_ = pmpi_group_range_incl_
#pragma weak mpi_group_range_incl__ = pmpi_group_range_incl__
#pragma weak MPI_Group_range_excl = PMPI_Group_range_excl
#pragma weak mpi_group_range_excl_ = pmpi_group_range_excl_
#pragma weak mpi_group_range_excl__ = pmpi_group_range_excl__
#pragma weak MPI_Group_free = PMPI_Group_free
#pragma weak mpi_group_free_ = pmpi_group_free_
#pragma weak mpi_group_free__ = pmpi_group_free__
#pragma weak MPI_Comm_size = PMPI_Comm_size
#pragma weak mpi_comm_size_ = pmpi_comm_size_
#pragma weak mpi_comm_size__ = pmpi_comm_size__
#pragma weak MPI_Comm_rank = PMPI_Comm_rank
#pragma weak mpi_comm_rank_ = pmpi_comm_rank_
#pragma weak mpi_comm_rank__ = pmpi_comm_rank__
#pragma weak MPI_Comm_compare = PMPI_Comm_compare
#pragma weak mpi_comm_compare_ = pmpi_comm_compare_
#pragma weak mpi_comm_compare__ = pmpi_comm_compare__
#pragma weak MPI_Comm_dup = PMPI_Comm_dup
#pragma weak mpi_comm_dup_ = pmpi_comm_dup_
#pragma weak mpi_comm_dup__ = pmpi_comm_dup__
#pragma weak MPI_Comm_create = PMPI_Comm_create
#pragma weak mpi_comm_create_ = pmpi_comm_create_
#pragma weak mpi_comm_create__ = pmpi_comm_create__
#pragma weak MPI_Comm_split = PMPI_Comm_split
#pragma weak mpi_comm_split_ = pmpi_comm_split_
#pragma weak mpi_comm_split__ = pmpi_comm_split__
#pragma weak MPI_Comm_free = PMPI_Comm_free
#pragma weak mpi_comm_free_ = pmpi_comm_free_
#pragma weak mpi_comm_free__ = pmpi_comm_free__
#pragma weak MPI_Comm_test_inter = PMPI_Comm_test_inter
#pragma weak mpi_comm_test_inter_ = pmpi_comm_test_inter_
#pragma weak mpi_comm_test_inter__ = pmpi_comm_test_inter__
#pragma weak MPI_Comm_remote_size = PMPI_Comm_remote_size
#pragma weak mpi_comm_remote_size_ = pmpi_comm_remote_size_
#pragma weak mpi_comm_remote_size__ = pmpi_comm_remote_size__
#pragma weak MPI_Comm_remote_group = PMPI_Comm_remote_group
#pragma weak mpi_comm_remote_group_ = pmpi_comm_remote_group_
#pragma weak mpi_comm_remote_group__ = pmpi_comm_remote_group__
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#pragma weak mpi_intercomm_create_ = pmpi_intercomm_create_
#pragma weak mpi_intercomm_create__ = pmpi_intercomm_create__
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
#pragma weak mpi_intercomm_merge_ = pmpi_intercomm_merge_
#pragma weak mpi_intercomm_merge__ = pmpi_intercomm_merge__
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#pragma weak mpi_keyval_create_ = pmpi_keyval_create_
#pragma weak mpi_keyval_create__ = pmpi_keyval_create__
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#pragma weak mpi_keyval_free_ = pmpi_keyval_free_
#pragma weak mpi_keyval_free__ = pmpi_keyval_free__
#pragma weak MPI_Attr_put = PMPI_Attr_put
#pragma weak mpi_attr_put_ = pmpi_attr_put_
#pragma weak mpi_attr_put__ = pmpi_attr_put__
#pragma weak MPI_Attr_get = PMPI_Attr_get
#pragma weak MPI_Attr_get_fortran = PMPI_Attr_get_fortran
#pragma weak mpi_attr_get_ = pmpi_attr_get_
#pragma weak mpi_attr_get__ = pmpi_attr_get__
#pragma weak MPI_Attr_delete = PMPI_Attr_delete
#pragma weak mpi_attr_delete_ = pmpi_attr_delete_
#pragma weak mpi_attr_delete__ = pmpi_attr_delete__
#pragma weak MPI_Topo_test = PMPI_Topo_test
#pragma weak mpi_topo_test_ = pmpi_topo_test_
#pragma weak mpi_topo_test__ = pmpi_topo_test__
#pragma weak MPI_Cart_create = PMPI_Cart_create
#pragma weak mpi_cart_create_ = pmpi_cart_create_
#pragma weak mpi_cart_create__ = pmpi_cart_create__
#pragma weak MPI_Dims_create = PMPI_Dims_create
#pragma weak mpi_dims_create_ = pmpi_dims_create_
#pragma weak mpi_dims_create__ = pmpi_dims_create__
#pragma weak MPI_Graph_create = PMPI_Graph_create
#pragma weak mpi_graph_create_ = pmpi_graph_create_
#pragma weak mpi_graph_create__ = pmpi_graph_create__
#pragma weak MPI_Graphdims_get = PMPI_Graphdims_get
#pragma weak mpi_graphdims_get_ = pmpi_graphdims_get_
#pragma weak mpi_graphdims_get__ = pmpi_graphdims_get__
#pragma weak MPI_Graph_get = PMPI_Graph_get
#pragma weak mpi_graph_get_ = pmpi_graph_get_
#pragma weak mpi_graph_get__ = pmpi_graph_get__
#pragma weak MPI_Cartdim_get = PMPI_Cartdim_get
#pragma weak mpi_cartdim_get_ = pmpi_cartdim_get_
#pragma weak mpi_cartdim_get__ = pmpi_cartdim_get__
#pragma weak MPI_Cart_get = PMPI_Cart_get
#pragma weak mpi_cart_get_ = pmpi_cart_get_
#pragma weak mpi_cart_get__ = pmpi_cart_get__
#pragma weak MPI_Cart_rank = PMPI_Cart_rank
#pragma weak mpi_cart_rank_ = pmpi_cart_rank_
#pragma weak mpi_cart_rank__ = pmpi_cart_rank__
#pragma weak MPI_Cart_coords = PMPI_Cart_coords
#pragma weak mpi_cart_coords_ = pmpi_cart_coords_
#pragma weak mpi_cart_coords__ = pmpi_cart_coords__
#pragma weak MPI_Graph_neighbors_count = PMPI_Graph_neighbors_count
#pragma weak mpi_graph_neighbors_count_ = pmpi_graph_neighbors_count_
#pragma weak mpi_graph_neighbors_count__ = pmpi_graph_neighbors_count__
#pragma weak MPI_Graph_neighbors = PMPI_Graph_neighbors
#pragma weak mpi_graph_neighbors_ = pmpi_graph_neighbors_
#pragma weak mpi_graph_neighbors__ = pmpi_graph_neighbors__
#pragma weak MPI_Cart_shift = PMPI_Cart_shift
#pragma weak mpi_cart_shift_ = pmpi_cart_shift_
#pragma weak mpi_cart_shift__ = pmpi_cart_shift__
#pragma weak MPI_Cart_sub = PMPI_Cart_sub
#pragma weak mpi_cart_sub_ = pmpi_cart_sub_
#pragma weak mpi_cart_sub__ = pmpi_cart_sub__
#pragma weak MPI_Cart_map = PMPI_Cart_map
#pragma weak mpi_cart_map_ = pmpi_cart_map_
#pragma weak mpi_cart_map__ = pmpi_cart_map__
#pragma weak MPI_Graph_map = PMPI_Graph_map
#pragma weak mpi_graph_map_ = pmpi_graph_map_
#pragma weak mpi_graph_map__ = pmpi_graph_map__
#pragma weak MPI_Get_processor_name = PMPI_Get_processor_name
#pragma weak mpi_get_processor_name_ = pmpi_get_processor_name_
#pragma weak mpi_get_processor_name__ = pmpi_get_processor_name__
#pragma weak MPI_Get_version = PMPI_Get_version
#pragma weak mpi_get_version_ = pmpi_get_version_
#pragma weak mpi_get_version__ = pmpi_get_version__
#pragma weak MPI_Errhandler_create = PMPI_Errhandler_create
#pragma weak mpi_errhandler_create_ = pmpi_errhandler_create_
#pragma weak mpi_errhandler_create__ = pmpi_errhandler_create__
#pragma weak MPI_Errhandler_set = PMPI_Errhandler_set
#pragma weak mpi_errhandler_set_ = pmpi_errhandler_set_
#pragma weak mpi_errhandler_set__ = pmpi_errhandler_set__
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#pragma weak mpi_errhandler_get_ = pmpi_errhandler_get_
#pragma weak mpi_errhandler_get__ = pmpi_errhandler_get__
#pragma weak MPI_Errhandler_free = PMPI_Errhandler_free
#pragma weak mpi_errhandler_free_ = pmpi_errhandler_free_
#pragma weak mpi_errhandler_free__ = pmpi_errhandler_free__
#pragma weak MPI_Error_string = PMPI_Error_string
#pragma weak mpi_error_string_ = pmpi_error_string_
#pragma weak mpi_error_string__ = pmpi_error_string__
#pragma weak MPI_Error_class = PMPI_Error_class
#pragma weak mpi_error_class_ = pmpi_error_class_
#pragma weak mpi_error_class__ = pmpi_error_class__
#pragma weak MPI_Wtime = PMPI_Wtime
#pragma weak mpi_wtime_ = pmpi_wtime_
#pragma weak mpi_wtime__ = pmpi_wtime__
#pragma weak MPI_Wtick = PMPI_Wtick
#pragma weak mpi_wtick_ = pmpi_wtick_
#pragma weak mpi_wtick__ = pmpi_wtick__
#pragma weak MPI_Init = PMPI_Init
#pragma weak mpi_init_ = pmpi_init_
#pragma weak mpi_init__ = pmpi_init__
#pragma weak MPI_Finalize = PMPI_Finalize
#pragma weak mpi_finalize_ = pmpi_finalize_
#pragma weak mpi_finalize__ = pmpi_finalize__
#pragma weak MPI_Finalized = PMPI_Finalized
#pragma weak mpi_finalized_ = pmpi_finalized_
#pragma weak mpi_finalized__ = pmpi_finalized__
#pragma weak MPI_Initialized = PMPI_Initialized
#pragma weak mpi_initialized_ = pmpi_initialized_
#pragma weak mpi_initialized__ = pmpi_initialized__
#pragma weak MPI_Abort = PMPI_Abort
#pragma weak mpi_abort_ = pmpi_abort_
#pragma weak mpi_abort__ = pmpi_abort__
#pragma weak MPI_Pcontrol = PMPI_Pcontrol
#pragma weak mpi_pcontrol_ = pmpi_pcontrol_
#pragma weak mpi_pcontrol__ = pmpi_pcontrol__

#pragma weak MPI_Comm_get_attr = PMPI_Attr_get
#pragma weak mpi_comm_get_attr_ = pmpi_attr_get_
#pragma weak mpi_comm_get_attr_ = pmpi_attr_get__

#pragma weak MPI_Comm_get_name = PMPI_Comm_get_name
#pragma weak mpi_comm_get_name_ = pmpi_comm_get_name_
#pragma weak mpi_comm_get_name__ = pmpi_comm_get_name__

#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name
#pragma weak mpi_comm_set_name_ = pmpi_comm_set_name_
#pragma weak mpi_comm_set_name__ = pmpi_comm_set_name__

#pragma weak MPI_Get_address = PMPI_Get_address
#pragma weak mpi_get_address_ = pmpi_get_address_
#pragma weak mpi_get_address__ = pmpi_get_address__

#pragma weak MPI_Init_thread = PMPI_Init_thread
#pragma weak mpi_init_thread_ = pmpi_init_thread_
#pragma weak mpi_init_thread__ = pmpi_init_thread__
#pragma weak MPI_Query_thread = PMPI_Query_thread
#pragma weak mpi_query_thread_ = pmpi_query_thread_
#pragma weak mpi_query_thread__ = pmpi_query_thread__

#pragma weak MPI_Comm_f2c = PMPI_Comm_f2c
#pragma weak MPI_Comm_c2f = PMPI_Comm_c2f
#pragma weak MPI_Type_f2c = PMPI_Type_f2c
#pragma weak MPI_Type_c2f = PMPI_Type_c2f
#pragma weak MPI_Group_f2c = PMPI_Group_f2c
#pragma weak MPI_Group_c2f = PMPI_Group_c2f
#pragma weak MPI_Request_f2c = PMPI_Request_f2c
#pragma weak MPI_Request_c2f = PMPI_Request_c2f
#pragma weak MPI_Win_f2c = PMPI_Win_f2c
#pragma weak MPI_Win_c2f = PMPI_Win_c2f
#pragma weak MPI_Op_f2c = PMPI_Op_f2c
#pragma weak MPI_Op_c2f = PMPI_Op_c2f
#pragma weak MPI_Info_f2c = PMPI_Info_f2c
#pragma weak MPI_Info_c2f = PMPI_Info_c2f
#pragma weak MPI_Errhandler_f2c = PMPI_Errhandler_f2c
#pragma weak MPI_Errhandler_c2f = PMPI_Errhandler_c2f

#pragma weak MPI_Info_create = PMPI_Info_create
#pragma weak mpi_info_create_ = pmpi_info_create_
#pragma weak mpi_info_create__ = pmpi_info_create__

#pragma weak MPI_Info_delete = PMPI_Info_delete
#pragma weak mpi_info_delete_ = pmpi_info_delete_
#pragma weak mpi_info_delete__ = pmpi_info_delete__

#pragma weak MPI_Info_dup = PMPI_Info_dup
#pragma weak mpi_info_dup_ = pmpi_info_dup_
#pragma weak mpi_info_dup__ = pmpi_info_dup__

#pragma weak MPI_Info_free = PMPI_Info_free
#pragma weak mpi_info_free_ = pmpi_info_free_
#pragma weak mpi_info_free__ = pmpi_info_free__

#pragma weak MPI_Info_set = PMPI_Info_set
#pragma weak mpi_info_set_ = pmpi_info_set_
#pragma weak mpi_info_set__ = pmpi_info_set__

#pragma weak MPI_Info_get = PMPI_Info_get
#pragma weak mpi_info_get_ = pmpi_info_get_
#pragma weak mpi_info_get__ = pmpi_info_get__

#pragma weak MPI_Info_get_nkeys = PMPI_Info_get_nkeys
#pragma weak mpi_info_get_nkeys_ = pmpi_info_get_nkeys_
#pragma weak mpi_info_get_nkeys__ = pmpi_info_get_nkeys__

#pragma weak MPI_Info_get_nthkey = PMPI_Info_get_nthkey
#pragma weak mpi_info_get_nthkey_ = pmpi_info_get_nthkey_
#pragma weak mpi_info_get_nthkey__ = pmpi_info_get_nthkey__

#pragma weak MPI_Info_get_valuelen = PMPI_Info_get_valuelen
#pragma weak mpi_info_get_valuelen_ = pmpi_info_get_valuelen_
#pragma weak mpi_info_get_valuelen__ = pmpi_info_get_valuelen__

#pragma weak MPI_Grequest_start = PMPI_Grequest_start
#pragma weak mpi_grequest_start_ = pmpi_grequest_start_
#pragma weak mpi_grequest_start__ = pmpi_grequest_start__
#pragma weak MPI_Grequest_complete = PMPI_Grequest_complete
#pragma weak mpi_grequest_complete_ = pmpi_grequest_complete_
#pragma weak mpi_grequest_complete__ = pmpi_grequest_complete__
#pragma weak MPIX_Grequest_start = PMPIX_Grequest_start
#pragma weak mpix_grequest_start_ = pmpix_grequest_start_
#pragma weak mpix_grequest_start__ = pmpix_grequest_start__
#pragma weak MPIX_Grequest_class_create = PMPIX_Grequest_class_create
#pragma weak mpix_grequest_class_create_ = pmpix_grequest_class_create_
#pragma weak mpix_grequest_class_create__ = pmpix_grequest_class_create__
#pragma weak MPIX_Grequest_class_allocate = PMPIX_Grequest_class_allocate
#pragma weak mpix_grequest_class_allocate_ = pmpix_grequest_class_allocate_
#pragma weak mpix_grequest_class_allocate__ = pmpix_grequest_class_allocate__
#pragma weak MPI_Status_set_elements = PMPI_Status_set_elements
#pragma weak mpi_status_set_elements_ = pmpi_status_set_elements_
#pragma weak mpi_status_set_elements__ = pmpi_status_set_elements__
#pragma weak MPI_Status_set_elements_x = PMPI_Status_set_elements_x
#pragma weak mpi_status_set_elements_x_ = pmpi_status_set_elements_x_
#pragma weak mpi_status_set_elements_x__ = pmpi_status_set_elements_x__
#pragma weak MPI_Status_set_cancelled = PMPI_Status_set_cancelled
#pragma weak mpi_status_set_cancelled_ = pmpi_status_set_cancelled_
#pragma weak mpi_status_set_cancelled__ = pmpi_status_set_cancelled__
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
#pragma weak mpi_request_get_status_ = pmpi_request_get_status_
#pragma weak mpi_request_get_status__ = pmpi_request_get_status__

#pragma weak MPI_Test_cancelled = PMPI_Test_cancelled
#pragma weak mpi_test_cancelled_ = pmpi_test_cancelled_
#pragma weak mpi_test_cancelled__ = pmpi_test_cancelled__

#pragma weak MPI_Type_set_name = PMPI_Type_set_name
#pragma weak mpi_type_set_name_ = pmpi_type_set_name_
#pragma weak mpi_type_set_name__ = pmpi_type_set_name__

#pragma weak MPI_Type_get_name = PMPI_Type_get_name
#pragma weak mpi_type_get_name_ = pmpi_type_get_name_
#pragma weak mpi_type_get_name__ = pmpi_type_get_name__

#pragma weak MPI_Type_dup = PMPI_Type_dup
#pragma weak mpi_type_dup_ = pmpi_type_dup_
#pragma weak mpi_type_dup__ = pmpi_type_dup__

#pragma weak MPI_Type_get_envelope = PMPI_Type_get_envelope
#pragma weak mpi_type_get_envelope_ = pmpi_type_get_envelope_
#pragma weak mpi_type_get_envelope__ = pmpi_type_get_envelope__

#pragma weak MPI_Type_get_contents = PMPI_Type_get_contents
#pragma weak mpi_type_get_contents_ = pmpi_type_get_contents_
#pragma weak mpi_type_get_contents__ = pmpi_type_get_contents__

#pragma weak MPI_Type_get_extent = PMPI_Type_get_extent
#pragma weak mpi_type_get_extent_ = pmpi_type_get_extent_
#pragma weak mpi_type_get_extent__ = pmpi_type_get_extent__

#pragma weak MPI_Type_get_true_extent = PMPI_Type_get_true_extent
#pragma weak mpi_type_get_true_extent_ = pmpi_type_get_true_extent_
#pragma weak mpi_type_get_true_extent__ = pmpi_type_get_true_extent__

#pragma weak MPI_Type_create_resized = PMPI_Type_create_resized
#pragma weak mpi_type_create_resized_ = pmpi_type_create_resized_
#pragma weak mpi_type_create_resized__ = pmpi_type_create_resized__

#pragma weak MPI_Type_create_hindexed_block = PMPI_Type_create_hindexed_block
#pragma weak mpi_type_create_hindexed_block_ = pmpi_type_create_hindexed_block_
#pragma weak mpi_type_create_hindexed_block__ = pmpi_type_create_hindexed_block__

#pragma weak MPI_Type_create_indexed_block = PMPI_Type_create_indexed_block
#pragma weak mpi_type_create_indexed_block_ = pmpi_type_create_indexed_block_
#pragma weak mpi_type_create_indexed_block__ = pmpi_type_create_indexed_block__

#pragma weak MPI_Type_match_size = PMPI_Type_match_size
#pragma weak MPI_Type_size_x = PMPI_Type_size_x
#pragma weak MPI_Type_get_extent_x = PMPI_Type_get_extent_x
#pragma weak MPI_Type_get_true_extent_x = PMPI_Type_get_true_extent_x
#pragma weak MPI_Get_elements_x = PMPI_Get_elements_x
#pragma weak MPI_Type_create_darray = PMPI_Type_create_darray

#pragma weak mpi_type_match_size_ = pmpi_type_match_size_
#pragma weak mpi_type_size_x_ = pmpi_type_size_x_
#pragma weak mpi_type_get_extent_x_ = pmpi_type_get_extent_x_
#pragma weak mpi_type_get_true_extent_x_ = pmpi_type_get_true_extent_x_
#pragma weak mpi_get_elements_x_ = pmpi_get_elements_x_
#pragma weak mpi_type_create_darray_ = pmpi_type_create_darray_

#pragma weak mpi_type_match_size__ = pmpi_type_match_size__
#pragma weak mpi_type_size_x__ = pmpi_type_size_x__
#pragma weak mpi_type_get_extent_x__ = pmpi_type_get_extent_x__
#pragma weak mpi_type_get_true_extent_x__ = pmpi_type_get_true_extent_x__
#pragma weak mpi_get_elements_x__ = pmpi_get_elements_x__
#pragma weak mpi_type_create_darray__ = pmpi_type_create_darray__

#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#pragma weak mpi_type_create_subarray_ = pmpi_type_create_subarray_
#pragma weak mpi_type_create_subarray__ = pmpi_type_create_subarray__

#pragma weak MPI_Pack_external_size = PMPI_Pack_external_size
#pragma weak mpi_pack_external_size_ = pmpi_pack_external_size_
#pragma weak mpi_pack_external_size__ = pmpi_pack_external_size__

#pragma weak MPI_Pack_external = PMPI_Pack_external
#pragma weak mpi_pack_external_ = pmpi_pack_external_
#pragma weak mpi_pack_external__ = pmpi_pack_external__

#pragma weak MPI_Unpack_external = PMPI_Unpack_external
#pragma weak mpi_unpack_external_ = pmpi_unpack_external_
#pragma weak mpi_unpack_external__ = pmpi_unpack_external__

#pragma weak  MPI_Free_mem = PMPI_Free_mem
#pragma weak  mpi_free_mem_ = pmpi_free_mem_
#pragma weak  mpi_free_mem__ = pmpi_free_mem__

#pragma weak  MPI_Alloc_mem = PMPI_Alloc_mem
#pragma weak  mpi_alloc_mem_ = pmpi_alloc_mem_
#pragma weak  mpi_alloc_mem__ = pmpi_alloc_mem__

/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/

/* One-sided communications */
#pragma weak MPI_Win_set_attr = PMPI_Win_set_attr
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#pragma weak MPI_Win_free_keyval = PMPI_Win_free_keyval
#pragma weak MPI_Win_delete_attr = PMPI_Win_delete_attr
#pragma weak MPI_Win_create_keyval = PMPI_Win_create_keyval
#pragma weak MPI_Win_create = PMPI_Win_create
#pragma weak MPI_Win_free = PMPI_Win_free
#pragma weak MPI_Win_fence = PMPI_Win_fence
#pragma weak MPI_Win_start = PMPI_Win_start
#pragma weak MPI_Win_complete = PMPI_Win_complete
#pragma weak MPI_Win_lock = PMPI_Win_lock
#pragma weak MPI_Win_unlock = PMPI_Win_unlock
#pragma weak MPI_Win_post = PMPI_Win_post
#pragma weak MPI_Win_wait = PMPI_Win_wait
#pragma weak MPI_Win_allocate = PMPI_Win_allocate
#pragma weak MPI_Win_test = PMPI_Win_test
#pragma weak MPI_Win_set_name = PMPI_Win_set_name
#pragma weak MPI_Win_get_name = PMPI_Win_get_name
#pragma weak MPI_Win_create_errhandler = PMPI_Win_create_errhandler
#pragma weak MPI_Win_set_errhandler = PMPI_Win_set_errhandler
#pragma weak MPI_Win_get_errhandler = PMPI_Win_get_errhandler
#pragma weak MPI_Win_get_group = PMPI_Win_get_group
#pragma weak MPI_Win_call_errhandler = PMPI_Win_call_errhandler
#pragma weak MPI_Win_allocate_shared = PMPI_Win_allocate_shared
#pragma weak MPI_Win_create_dynamic = PMPI_Win_create_dynamic
#pragma weak MPI_Win_shared_query = PMPI_Win_shared_query
#pragma weak MPI_Win_lock_all = PMPI_Win_lock_all
#pragma weak MPI_Win_unlock_all = PMPI_Win_unlock_all
#pragma weak MPI_Win_sync = PMPI_Win_sync
#pragma weak MPI_Win_attach = PMPI_Win_attach
#pragma weak MPI_Win_detach = PMPI_Win_detach
#pragma weak MPI_Win_flush = PMPI_Win_flush
#pragma weak MPI_Win_flush_all = PMPI_Win_flush_all
#pragma weak MPI_Win_set_info = PMPI_Win_set_info
#pragma weak MPI_Win_get_info = PMPI_Win_get_info
#pragma weak MPI_Get_accumulate = PMPI_Get_accumulate
#pragma weak MPI_Fetch_and_op = PMPI_Fetch_and_op
#pragma weak MPI_Compare_and_swap = PMPI_Compare_and_swap
#pragma weak MPI_Rput = PMPI_Rput
#pragma weak MPI_Rget = PMPI_Rget
#pragma weak MPI_Raccumulate = PMPI_Raccumulate
#pragma weak MPI_Rget_accumulate = PMPI_Rget_accumulate
#pragma weak MPI_Accumulate = PMPI_Accumulate
#pragma weak MPI_Get = PMPI_Get
#pragma weak MPI_Put = PMPI_Put

#pragma weak mpi_win_set_attr_ = pmpi_win_set_attr_
#pragma weak mpi_win_get_attr_ = pmpi_win_get_attr_
#pragma weak mpi_win_free_keyval_ = pmpi_win_free_keyval_
#pragma weak mpi_win_delete_attr_ = pmpi_win_delete_attr_
#pragma weak mpi_win_create_keyval_ = pmpi_win_create_keyval_
#pragma weak mpi_win_create_ = pmpi_win_create_
#pragma weak mpi_win_free_ = pmpi_win_free_
#pragma weak mpi_win_fence_ = pmpi_win_fence_
#pragma weak mpi_win_start_ = pmpi_win_start_
#pragma weak mpi_win_complete_ = pmpi_win_complete_
#pragma weak mpi_win_lock_ = pmpi_win_lock_
#pragma weak mpi_win_unlock_ = pmpi_win_unlock_
#pragma weak mpi_win_post_ = pmpi_win_post_
#pragma weak mpi_win_wait_ = pmpi_win_wait_
#pragma weak mpi_win_allocate_= pmpi_win_allocate_
#pragma weak mpi_win_test_ = pmpi_win_test_
#pragma weak mpi_win_set_name_ = pmpi_win_set_name_
#pragma weak mpi_win_get_name_ = pmpi_win_get_name_
#pragma weak mpi_win_create_errhandler_ = pmpi_win_create_errhandler_
#pragma weak mpi_win_set_errhandler_ = pmpi_win_set_errhandler_
#pragma weak mpi_win_get_errhandler_ = pmpi_win_get_errhandler_
#pragma weak mpi_win_get_group_ = pmpi_win_get_group_
#pragma weak mpi_win_call_errhandler_ = pmpi_win_call_errhandler_
#pragma weak mpi_win_allocate_shared_ = pmpi_win_allocate_shared_
#pragma weak mpi_win_create_dynamic_ = pmpi_win_create_dynamic_
#pragma weak mpi_win_shared_query_ = pmpi_win_shared_query_
#pragma weak mpi_win_lock_all_ = pmpi_win_lock_all_
#pragma weak mpi_win_unlock_all_ = pmpi_win_unlock_all_
#pragma weak mpi_win_sync_ = pmpi_win_sync_
#pragma weak mpi_win_attach_ = pmpi_win_attach_
#pragma weak mpi_win_detach_ = pmpi_win_detach_
#pragma weak mpi_win_flush_ = pmpi_win_flush_
#pragma weak mpi_win_flush_all_ = pmpi_win_flush_all_
#pragma weak mpi_win_set_info_ = pmpi_win_set_info_
#pragma weak mpi_win_get_info_ = pmpi_win_get_info_
#pragma weak mpi_get_accumulate_ = pmpi_get_accumulate_
#pragma weak mpi_fetch_and_op_ = pmpi_fetch_and_op_
#pragma weak mpi_compare_and_swap_ = pmpi_compare_and_swap_
#pragma weak mpi_rput_ = pmpi_rput_
#pragma weak mpi_rget_ = pmpi_rget_
#pragma weak mpi_raccumulate_ = pmpi_raccumulate_
#pragma weak mpi_rget_accumulate_ = pmpi_rget_accumulate_
#pragma weak mpi_accumulate_ = pmpi_accumulate_
#pragma weak mpi_get_ = pmpi_get_
#pragma weak mpi_put_ = pmpi_put_

#pragma weak mpi_win_set_attr__ = pmpi_win_set_attr__
#pragma weak mpi_win_get_attr__ = pmpi_win_get_attr__
#pragma weak mpi_win_free_keyval__ = pmpi_win_free_keyval__
#pragma weak mpi_win_delete_attr__ = pmpi_win_delete_attr__
#pragma weak mpi_win_create_keyval__ = pmpi_win_create_keyval__
#pragma weak mpi_win_create__ = pmpi_win_create__
#pragma weak mpi_win_free__ = pmpi_win_free__
#pragma weak mpi_win_fence__ = pmpi_win_fence__
#pragma weak mpi_win_start__ = pmpi_win_start__
#pragma weak mpi_win_complete__ = pmpi_win_complete__
#pragma weak mpi_win_lock__ = pmpi_win_lock__
#pragma weak mpi_win_unlock__ = pmpi_win_unlock__
#pragma weak mpi_win_post__ = pmpi_win_post__
#pragma weak mpi_win_wait__ = pmpi_win_wait__
#pragma weak mpi_win_allocate__= pmpi_win_allocate__
#pragma weak mpi_win_test__ = pmpi_win_test__
#pragma weak mpi_win_set_name__ = pmpi_win_set_name__
#pragma weak mpi_win_get_name__ = pmpi_win_get_name__
#pragma weak mpi_win_create_errhandler__ = pmpi_win_create_errhandler__
#pragma weak mpi_win_set_errhandler__ = pmpi_win_set_errhandler__
#pragma weak mpi_win_get_errhandler__ = pmpi_win_get_errhandler__
#pragma weak mpi_win_get_group_ = pmpi_win_get_group_
#pragma weak mpi_win_call_errhandler__ = pmpi_win_call_errhandler__
#pragma weak mpi_win_allocate_shared__ = pmpi_win_allocate_shared__
#pragma weak mpi_win_create_dynamic__ = pmpi_win_create_dynamic_
#pragma weak mpi_win_shared_query__ = pmpi_win_shared_query__
#pragma weak mpi_win_lock_all__ = pmpi_win_lock_all__
#pragma weak mpi_win_unlock_all__ = pmpi_win_unlock_all__
#pragma weak mpi_win_sync__ = pmpi_win_sync__
#pragma weak mpi_win_attach__ = pmpi_win_attach__
#pragma weak mpi_win_detach__ = pmpi_win_detach__
#pragma weak mpi_win_flush__ = pmpi_win_flush__
#pragma weak mpi_win_flush_all__ = pmpi_win_flush_all__
#pragma weak mpi_win_set_info__ = pmpi_win_set_info__
#pragma weak mpi_win_get_info__ = pmpi_win_get_info__
#pragma weak mpi_get_accumulate__ = pmpi_get_accumulate__
#pragma weak mpi_fetch_and_op__ = pmpi_fetch_and_op__
#pragma weak mpi_compare_and_swap__ = pmpi_compare_and_swap__
#pragma weak mpi_rput__ = pmpi_rput__
#pragma weak mpi_rget__ = pmpi_rget__
#pragma weak mpi_raccumulate__ = pmpi_raccumulate__
#pragma weak mpi_rget_accumulate__ = pmpi_rget_accumulate__
#pragma weak mpi_accumulate__ = pmpi_accumulate__
#pragma weak mpi_get__ = pmpi_get__
#pragma weak mpi_put__ = pmpi_put__

/* Communicator Management */
#pragma weak MPI_Comm_create_keyval = PMPI_Comm_create_keyval
#pragma weak MPI_Comm_delete_attr  = PMPI_Comm_delete_attr
#pragma weak MPI_Comm_free_keyval  = PMPI_Comm_free_keyval
#pragma weak MPI_Comm_set_attr  = PMPI_Comm_set_attr
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

#pragma weak mpi_comm_create_keyval_ = pmpi_comm_create_keyval_
#pragma weak mpi_comm_delete_attr_  = pmpi_comm_delete_attr_
#pragma weak mpi_comm_free_keyval_  = pmpi_comm_free_keyval_
#pragma weak mpi_comm_set_attr_  = pmpi_comm_set_attr_
#pragma weak mpi_comm_create_errhandler_ = pmpi_comm_create_errhandler_
#pragma weak mpi_comm_dup_with_info_ = pmpi_comm_dup_with_info_
#pragma weak mpi_comm_split_type_ = pmpi_comm_split_type_
#pragma weak mpi_comm_set_info_ = pmpi_comm_set_info_
#pragma weak mpi_comm_get_info_ = pmpi_comm_get_info_
#pragma weak mpi_comm_idup_ = pmpi_comm_idup_
#pragma weak mpi_comm_create_group_ = pmpi_comm_create_group_
#pragma weak mpi_comm_get_errhandler_ = pmpi_comm_get_errhandler_
#pragma weak mpi_comm_call_errhandler_ = pmpi_comm_call_errhandler_
#pragma weak mpi_comm_set_errhandler_ = pmpi_comm_set_errhandler_

#pragma weak mpi_comm_create_keyval__ = pmpi_comm_create_keyval__
#pragma weak mpi_comm_delete_attr__  = pmpi_comm_delete_attr__
#pragma weak mpi_comm_free_keyval__  = pmpi_comm_free_keyval__
#pragma weak mpi_comm_set_attr__  = pmpi_comm_set_attr__
#pragma weak mpi_comm_create_errhandler__ = pmpi_comm_create_errhandler__
#pragma weak mpi_comm_dup_with_info__ = pmpi_comm_dup_with_info__
#pragma weak mpi_comm_split_type__ = pmpi_comm_split_type__
#pragma weak mpi_comm_set_info__ = pmpi_comm_set_info__
#pragma weak mpi_comm_get_info__ = pmpi_comm_get_info__
#pragma weak mpi_comm_idup__ = pmpi_comm_idup__
#pragma weak mpi_comm_create_group__ = pmpi_comm_create_group__
#pragma weak mpi_comm_get_errhandler__ = pmpi_comm_get_errhandler__
#pragma weak mpi_comm_call_errhandler__ = pmpi_comm_call_errhandler__
#pragma weak mpi_comm_set_errhandler__ = pmpi_comm_set_errhandler__

/* datatype handling */
#pragma weak MPI_Type_create_keyval = PMPI_Type_create_keyval
#pragma weak MPI_Type_set_attr = PMPI_Type_set_attr
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#pragma weak MPI_Type_free_keyval = PMPI_Type_free_keyval

#pragma weak mpi_type_create_keyval_ = pmpi_type_create_keyval_
#pragma weak mpi_type_set_attr_ = pmpi_type_set_attr_
#pragma weak mpi_type_get_attr_ = pmpi_type_get_attr_
#pragma weak mpi_type_delete_attr_ = pmpi_type_delete_attr_
#pragma weak mpi_type_free_keyval_ = pmpi_type_free_keyval_

#pragma weak mpi_type_create_keyval__ = pmpi_type_create_keyval__
#pragma weak mpi_type_set_attr__ = pmpi_type_set_attr__
#pragma weak mpi_type_get_attr__ = pmpi_type_get_attr__
#pragma weak mpi_type_delete_attr__ = pmpi_type_delete_attr__
#pragma weak mpi_type_free_keyval__ = pmpi_type_free_keyval__

/* MPI environmental management */
#pragma weak MPI_Add_error_class = PMPI_Add_error_class
#pragma weak MPI_Add_error_code = PMPI_Add_error_code
#pragma weak MPI_Add_error_string = PMPI_Add_error_string
#pragma weak MPI_Is_thread_main = PMPI_Is_thread_main
#pragma weak MPI_Get_library_version = PMPI_Get_library_version

#pragma weak mpi_add_error_class_ = pmpi_add_error_class_
#pragma weak mpi_add_error_code_ = pmpi_add_error_code_
#pragma weak mpi_add_error_string_ = pmpi_add_error_string_
#pragma weak mpi_is_thread_main_ = pmpi_is_thread_main_
#pragma weak mpi_get_library_version_ = pmpi_get_library_version_

#pragma weak mpi_add_error_class__ = pmpi_add_error_class__
#pragma weak mpi_add_error_code__ = pmpi_add_error_code__
#pragma weak mpi_add_error_string__ = pmpi_add_error_string__
#pragma weak mpi_is_thread_main__ = pmpi_is_thread_main__
#pragma weak mpi_get_library_version__ = pmpi_get_library_version__

/* Process creation and Management */
#pragma weak MPI_Close_port = PMPI_Close_port
#pragma weak MPI_Comm_accept = PMPI_Comm_accept
#pragma weak MPI_Comm_connect = PMPI_Comm_connect
#pragma weak MPI_Comm_disconnect = PMPI_Comm_disconnect
#pragma weak MPI_Comm_get_parent = PMPI_Comm_get_parent
#pragma weak MPI_Comm_join = PMPI_Comm_join
#pragma weak MPI_Comm_spawn  = PMPI_Comm_spawn
#pragma weak MPI_Comm_spawn_multiple = PMPI_Comm_spawn_multiple
#pragma weak MPI_Lookup_name = PMPI_Lookup_name
#pragma weak MPI_Open_port = PMPI_Open_port
#pragma weak MPI_Publish_name = PMPI_Publish_name
#pragma weak MPI_Unpublish_name = PMPI_Unpublish_name

#pragma weak mpi_close_port_ = pmpi_close_port_
#pragma weak mpi_comm_accept_ = pmpi_comm_accept_
#pragma weak mpi_comm_connect_ = pmpi_comm_connect_
#pragma weak mpi_comm_disconnect_ = pmpi_comm_disconnect_
#pragma weak mpi_comm_get_parent_ = pmpi_comm_get_parent_
#pragma weak mpi_comm_join_ = pmpi_comm_join_
#pragma weak mpi_comm_spawn_ = pmpi_comm_spawn_
#pragma weak mpi_comm_spawn_multiple_ = pmpi_comm_spawn_multiple_
#pragma weak mpi_lookup_name_ = pmpi_lookup_name_
#pragma weak mpi_open_port_ = pmpi_open_port_
#pragma weak mpi_publish_name_ = pmpi_publish_name_
#pragma weak mpi_unpublish_name_ = pmpi_unpublish_name_

#pragma weak mpi_close_port__ = pmpi_close_port__
#pragma weak mpi_comm_accept__ = pmpi_comm_accept__
#pragma weak mpi_comm_connect__ = pmpi_comm_connect__
#pragma weak mpi_comm_disconnect__ = pmpi_comm_disconnect__
#pragma weak mpi_comm_get_parent__ = pmpi_comm_get_parent__
#pragma weak mpi_comm_join__ = pmpi_comm_join__
#pragma weak mpi_comm_spawn__ = pmpi_comm_spawn__
#pragma weak mpi_comm_spawn_multiple__ = pmpi_comm_spawn_multiple__
#pragma weak mpi_lookup_name__ = pmpi_lookup_name__
#pragma weak mpi_open_port__ = pmpi_open_port__
#pragma weak mpi_publish_name__ = pmpi_publish_name__
#pragma weak mpi_unpublish_name__ = pmpi_unpublish_name__


/* Dist graph operations */
#pragma weak MPI_Dist_graph_neighbors_count = PMPI_Dist_graph_neighbors_count
#pragma weak MPI_Dist_graph_neighbors = PMPI_Dist_graph_neighbors
#pragma weak MPI_Dist_graph_create = PMPI_Dist_graph_create
#pragma weak MPI_Dist_graph_create_adjacent = PMPI_Dist_graph_create_adjacent

#pragma weak mpi_dist_graph_neighbors_count_ = pmpi_dist_graph_neighbors_count_
#pragma weak mpi_dist_graph_neighbors_ = pmpi_dist_graph_neighbors_
#pragma weak mpi_dist_graph_create_ = pmpi_dist_graph_create_
#pragma weak mpi_dist_graph_create_adjacent_ = pmpi_dist_graph_create_adjacent_

#pragma weak mpi_dist_graph_neighbors_count__ = pmpi_dist_graph_neighbors_count__
#pragma weak mpi_dist_graph_neighbors__ = pmpi_dist_graph_neighbors__
#pragma weak mpi_dist_graph_create__ = pmpi_dist_graph_create__
#pragma weak mpi_dist_graph_create_adjacent__ = pmpi_dist_graph_create_adjacent__

/* collectives */
#pragma weak MPI_Reduce_local = PMPI_Reduce_local
#pragma weak mpi_reduce_local_ = pmpi_reduce_local_
#pragma weak mpi_reduce_local__ = pmpi_reduce_local__

/* Error handling */
#pragma weak MPI_File_create_errhandler = PMPI_File_create_errhandler
#pragma weak MPI_File_call_errhandler = PMPI_File_call_errhandler

#pragma weak mpi_file_create_errhandler_ = pmpi_file_create_errhandler_
#pragma weak mpi_file_call_errhandler_ = pmpi_file_call_errhandler_

#pragma weak mpi_file_create_errhandler__ = pmpi_file_create_errhandler__
#pragma weak mpi_file_call_errhandler__ = pmpi_file_call_errhandler__

/* MPI_T methods */
#pragma weak MPI_T_init_thread = PMPI_T_init_thread
#pragma weak MPI_T_finalize = PMPI_T_finalize

#pragma weak MPI_T_pvar_read = PMPI_T_pvar_read
#pragma weak MPI_T_pvar_write = PMPI_T_pvar_write
#pragma weak MPI_T_pvar_reset = PMPI_T_pvar_reset
#pragma weak MPI_T_pvar_get_num = PMPI_T_pvar_get_num
#pragma weak MPI_T_pvar_get_info = PMPI_T_pvar_get_info
#pragma weak MPI_T_pvar_session_create = PMPI_T_pvar_session_create
#pragma weak MPI_T_pvar_session_free = PMPI_T_pvar_session_free
#pragma weak MPI_T_pvar_handle_alloc = PMPI_T_pvar_handle_alloc
#pragma weak MPI_T_pvar_handle_free = PMPI_T_pvar_handle_free
#pragma weak MPI_T_pvar_start = PMPI_T_pvar_start
#pragma weak MPI_T_pvar_stop = PMPI_T_pvar_stop

#pragma weak MPI_T_cvar_read = PMPI_T_cvar_read
#pragma weak MPI_T_cvar_write = PMPI_T_cvar_write
#pragma weak MPI_T_cvar_get_num = PMPI_T_cvar_get_num
#pragma weak MPI_T_cvar_get_info = PMPI_T_cvar_get_info
#pragma weak MPI_T_cvar_handle_alloc = PMPI_T_cvar_handle_alloc
#pragma weak MPI_T_cvar_handle_free = PMPI_T_cvar_handle_free

#pragma weak MPI_T_category_get_pvars = PMPI_T_category_get_pvars
#pragma weak MPI_T_category_get_num = PMPI_T_category_get_num
#pragma weak MPI_T_category_get_categories = PMPI_T_category_get_categories
#pragma weak MPI_T_category_get_info = PMPI_T_category_get_info
#pragma weak MPI_T_category_get_cvars = PMPI_T_category_get_pvars

#pragma weak MPI_T_enum_get_info = PMPI_T_enum_get_info
//-----
#pragma weak mpi_t_init_thread_ = pmpi_t_init_thread_
#pragma weak mpi_t_finalize_ = pmpi_t_finalize_

#pragma weak mpi_t_pvar_read_ = pmpi_t_pvar_read_
#pragma weak mpi_t_pvar_write_ = pmpi_t_pvar_write_
#pragma weak mpi_t_pvar_reset_ = pmpi_t_pvar_reset_
#pragma weak mpi_t_pvar_get_num_ = pmpi_t_pvar_get_num_
#pragma weak mpi_t_pvar_get_info_ = pmpi_t_pvar_get_info_
#pragma weak mpi_t_pvar_session_create_ = pmpi_t_pvar_session_create_
#pragma weak mpi_t_pvar_session_free_ = pmpi_t_pvar_session_free_
#pragma weak mpi_t_pvar_handle_alloc_ = pmpi_t_pvar_handle_alloc_
#pragma weak mpi_t_pvar_handle_free_ = pmpi_t_pvar_handle_free_
#pragma weak mpi_t_pvar_start_ = pmpi_t_pvar_start_
#pragma weak mpi_t_pvar_stop_ = pmpi_t_pvar_stop_

#pragma weak mpi_t_cvar_read_ = pmpi_t_cvar_read_
#pragma weak mpi_t_cvar_write_ = pmpi_t_cvar_write_
#pragma weak mpi_t_cvar_get_num_ = pmpi_t_cvar_get_num_
#pragma weak mpi_t_cvar_get_info_ = pmpi_t_cvar_get_info_
#pragma weak mpi_t_cvar_handle_alloc_ = pmpi_t_cvar_handle_alloc_
#pragma weak mpi_t_cvar_handle_free_ = pmpi_t_cvar_handle_free_

#pragma weak mpi_t_category_get_pvars_ = pmpi_t_category_get_pvars_
#pragma weak mpi_t_category_get_num_ = pmpi_t_category_get_num_
#pragma weak mpi_t_category_get_categories_ = pmpi_t_category_get_categories_
#pragma weak mpi_t_category_get_info_ = pmpi_t_category_get_info_
#pragma weak mpi_t_category_get_cvars_ = pmpi_t_category_get_pvars_

#pragma weak mpi_t_enum_get_info_ = pmpi_t_enum_get_info_
//-----
#pragma weak mpi_t_init_thread__ = pmpi_t_init_thread__
#pragma weak mpi_t_finalize__ = pmpi_t_finalize__

#pragma weak mpi_t_pvar_read__ = pmpi_t_pvar_read__
#pragma weak mpi_t_pvar_write__ = pmpi_t_pvar_write__
#pragma weak mpi_t_pvar_reset__ = pmpi_t_pvar_reset__
#pragma weak mpi_t_pvar_get_num__ = pmpi_t_pvar_get_num__
#pragma weak mpi_t_pvar_get_info__ = pmpi_t_pvar_get_info__
#pragma weak mpi_t_pvar_session_create__ = pmpi_t_pvar_session_create__
#pragma weak mpi_t_pvar_session_free__ = pmpi_t_pvar_session_free__
#pragma weak mpi_t_pvar_handle_alloc__ = pmpi_t_pvar_handle_alloc__
#pragma weak mpi_t_pvar_handle_free__ = pmpi_t_pvar_handle_free__
#pragma weak mpi_t_pvar_start__ = pmpi_t_pvar_start__
#pragma weak mpi_t_pvar_stop__ = pmpi_t_pvar_stop__

#pragma weak mpi_t_cvar_read__ = pmpi_t_cvar_read__
#pragma weak mpi_t_cvar_write__ = pmpi_t_cvar_write__
#pragma weak mpi_t_cvar_get_num__ = pmpi_t_cvar_get_num__
#pragma weak mpi_t_cvar_get_info__ = pmpi_t_cvar_get_info__
#pragma weak mpi_t_cvar_handle_alloc__ = pmpi_t_cvar_handle_alloc__
#pragma weak mpi_t_cvar_handle_free__ = pmpi_t_cvar_handle_free__

#pragma weak mpi_t_category_get_pvars__ = pmpi_t_category_get_pvars__
#pragma weak mpi_t_category_get_num__ = pmpi_t_category_get_num__
#pragma weak mpi_t_category_get_categories__ = pmpi_t_category_get_categories__
#pragma weak mpi_t_category_get_info__ = pmpi_t_category_get_info__
#pragma weak mpi_t_category_get_cvars__ = pmpi_t_category_get_pvars__

#pragma weak mpi_t_enum_get_info__ = pmpi_t_enum_get_info__

/* MPIX methods */
#pragma weak MPIX_Comm_failure_ack = PMPIX_Comm_failure_ack
#pragma weak MPIX_Comm_failure_get_acked = PMPIX_Comm_failure_get_acked
#pragma weak MPIX_Comm_agree = PMPIX_Comm_agree
#pragma weak MPIX_Comm_revoke = PMPIX_Comm_revoke
#pragma weak MPIX_Comm_shrink = PMPIX_Comm_shrink

#pragma weak mpix_comm_failure_ack_ = pmpix_comm_failure_ack_
#pragma weak mpix_comm_failure_get_acked_ = pmpix_comm_failure_get_acked_
#pragma weak mpix_comm_agree_ = pmpix_comm_agree_
#pragma weak mpix_comm_revoke_ = pmpix_comm_revoke_
#pragma weak mpix_comm_shrink_ = pmpix_comm_shrink_

#pragma weak mpix_comm_failure_ack__ = pmpix_comm_failure_ack__
#pragma weak mpix_comm_failure_get_acked__ = pmpix_comm_failure_get_acked__
#pragma weak mpix_comm_agree__ = pmpix_comm_agree__
#pragma weak mpix_comm_revoke__ = pmpix_comm_revoke__
#pragma weak mpix_comm_shrink__ = pmpix_comm_shrink__

/* probe and cancel */
#pragma weak MPI_Mprobe = PMPI_Mprobe
#pragma weak MPI_Mrecv = PMPI_Mrecv
#pragma weak MPI_Improbe = PMPI_Improbe
#pragma weak MPI_Imrecv = PMPI_Imrecv

#pragma weak mpi_mprobe_ = pmpi_mprobe_
#pragma weak mpi_mrecv_ = pmpi_mrecv_
#pragma weak mpi_improbe_ = pmpi_improbe_
#pragma weak mpi_imrecv_ = pmpi_imrecv_

#pragma weak mpi_mprobe__ = pmpi_mprobe__
#pragma weak mpi_mrecv__ = pmpi_mrecv__
#pragma weak mpi_improbe__ = pmpi_improbe__
#pragma weak mpi_imrecv__ = pmpi_imrecv__
/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/


