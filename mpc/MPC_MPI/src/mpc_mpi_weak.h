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
/* #                                                                      # */
/* ######################################################################## */

#pragma weak MPI_Send = PMPI_Send
#pragma weak MPI_Recv = PMPI_Recv
#pragma weak MPI_Get_count = PMPI_Get_count
#pragma weak MPI_Bsend = PMPI_Bsend
#pragma weak MPI_Ssend = PMPI_Ssend
#pragma weak MPI_Rsend = PMPI_Rsend
#pragma weak MPI_Buffer_attach = PMPI_Buffer_attach
#pragma weak MPI_Buffer_detach = PMPI_Buffer_detach
#pragma weak MPI_Isend = PMPI_Isend
#pragma weak MPI_Ibsend = PMPI_Ibsend
#pragma weak MPI_Issend = PMPI_Issend
#pragma weak MPI_Irsend = PMPI_Irsend
#pragma weak MPI_Irecv = PMPI_Irecv
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
#pragma weak MPI_Test_cancelled = PMPI_Test_cancelled
#pragma weak MPI_Send_init = PMPI_Send_init
#pragma weak MPI_Bsend_init = PMPI_Bsend_init
#pragma weak MPI_Ssend_init = PMPI_Ssend_init
#pragma weak MPI_Rsend_init = PMPI_Rsend_init
#pragma weak MPI_Recv_init = PMPI_Recv_init
#pragma weak MPI_Start = PMPI_Start
#pragma weak MPI_Startall = PMPI_Startall
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#pragma weak MPI_Sendrecv_replace = PMPI_Sendrecv_replace
#pragma weak MPI_Type_contiguous = PMPI_Type_contiguous
#pragma weak MPI_Type_vector = PMPI_Type_vector
#pragma weak MPI_Type_hvector = PMPI_Type_hvector
#pragma weak MPI_Type_indexed = PMPI_Type_indexed
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#pragma weak MPI_Type_struct = PMPI_Type_struct
#pragma weak MPI_Address = PMPI_Address
#pragma weak MPI_Type_extent = PMPI_Type_extent
#pragma weak MPI_Type_size = PMPI_Type_size
#pragma weak MPI_Type_lb = PMPI_Type_lb
#pragma weak MPI_Type_ub = PMPI_Type_ub
#pragma weak MPI_Type_commit = PMPI_Type_commit
#pragma weak MPI_Type_free = PMPI_Type_free
#pragma weak MPI_Get_elements = PMPI_Get_elements
#pragma weak MPI_Pack = PMPI_Pack
#pragma weak MPI_Unpack = PMPI_Unpack
#pragma weak MPI_Pack_size = PMPI_Pack_size
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
#pragma weak MPI_Reduce = PMPI_Reduce
#pragma weak MPI_Op_create = PMPI_Op_create
#pragma weak MPI_Op_free = PMPI_Op_free
#pragma weak MPI_Allreduce = PMPI_Allreduce
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#pragma weak MPI_Scan = PMPI_Scan
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
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#pragma weak MPI_Attr_put = PMPI_Attr_put
#pragma weak MPI_Attr_get = PMPI_Attr_get
#pragma weak MPI_Attr_delete = PMPI_Attr_delete
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
#pragma weak MPI_Errhandler_create = PMPI_Errhandler_create
#pragma weak MPI_Errhandler_set = PMPI_Errhandler_set
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#pragma weak MPI_Errhandler_free = PMPI_Errhandler_free
#pragma weak MPI_Error_string = PMPI_Error_string
#pragma weak MPI_Error_class = PMPI_Error_class
#pragma weak MPI_Wtime = PMPI_Wtime
#pragma weak MPI_Wtick = PMPI_Wtick
#pragma weak MPI_Init = PMPI_Init
#pragma weak MPI_Finalize = PMPI_Finalize
#pragma weak MPI_Initialized = PMPI_Initialized
#pragma weak MPI_Abort = PMPI_Abort
#pragma weak MPI_Pcontrol = PMPI_Pcontrol

#pragma weak MPI_Comm_get_name = PMPI_Comm_get_name


#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name


#pragma weak MPI_Init_thread = PMPI_Init_thread

