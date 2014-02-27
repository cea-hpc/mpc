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

TODO("Handle the case of MPI_Aint which are not int fortran")

void ffunc (mpi_send) (void *buf, int *count, MPI_Datatype * datatype,
		       int *dest, int *tag, MPI_Comm * comm, int *res);

void ffunc (mpi_recv) (void *buf, int *count, MPI_Datatype * datatype,
		       int *source, int *tag, MPI_Comm * comm,
		       MPI_Status * status, int *res);

void ffunc (mpi_get_count) (MPI_Status * status, MPI_Datatype * datatype,
			    int *count, int *res);

void ffunc (mpi_bsend) (void *buf, int *count, MPI_Datatype * datatype,
			int *dest, int *tag, MPI_Comm * comm, int *res);

void ffunc (mpi_ssend) (void *buf, int *count, MPI_Datatype * datatype,
			int *dest, int *tag, MPI_Comm * comm, int *res);

void ffunc (mpi_rsend) (void *buf, int *count, MPI_Datatype * datatype,
			int *dest, int *tag, MPI_Comm * comm, int *res);

void ffunc (mpi_buffer_attach) (void *buffer, int *size, int *res);

void ffunc (mpi_buffer_detach) (void *buffer, int *size, int *res);

void ffunc (mpi_isend) (void *buf, int *count, MPI_Datatype * datatype,
			int *dest, int *tag, MPI_Comm * comm,
			MPI_Request * request, int *res);

void ffunc (mpi_ibsend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm,
			 MPI_Request * request, int *res);

void ffunc (mpi_issend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm,
			 MPI_Request * request, int *res);

void ffunc (mpi_irsend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm,
			 MPI_Request * request, int *res);

void ffunc (mpi_irecv) (void *buf, int *count, MPI_Datatype * datatype,
			int *source, int *tag, MPI_Comm * comm,
			MPI_Request * request, int *res);

void ffunc (mpi_wait) (MPI_Request * request, MPI_Status * status, int *res);

void ffunc (mpi_test) (MPI_Request * request, int *flag, MPI_Status * status,
		       int *res);

void ffunc (mpi_request_free) (MPI_Request * request, int *res);

void ffunc (mpi_waitany) (int *count, MPI_Request array_of_requests[],
			  int *index, MPI_Status * status, int *res);

void ffunc (mpi_testany) (int *count, MPI_Request array_of_requests[],
			  int *index, int *flag, MPI_Status * status,
			  int *res);

void ffunc (mpi_waitall) (int *count, MPI_Request array_of_requests[],
			  MPI_Status array_of_statuses[], int *res);

void ffunc (mpi_testall) (int *count, MPI_Request array_of_requests[],
			  int *flag, MPI_Status array_of_statuses[],
			  int *res);

void ffunc (mpi_waitsome) (int *incount, MPI_Request array_of_requests[],
			   int *outcount, int array_of_indices[],
			   MPI_Status array_of_statuses[], int *res);

void ffunc (mpi_testsome) (int *incount, MPI_Request array_of_requests[],
			   int *outcount, int array_of_indices[],
			   MPI_Status array_of_statuses[], int *res);

void ffunc (mpi_iprobe) (int *source, int *tag, MPI_Comm * comm, int *flag,
			 MPI_Status * status, int *res);

void ffunc (mpi_probe) (int *source, int *tag, MPI_Comm * comm,
			MPI_Status * status, int *res);

void ffunc (mpi_cancel) (MPI_Request * request, int *res);

void ffunc (mpi_test_cancelled) (MPI_Status * status, int *flag, int *res);

void ffunc (mpi_send_init) (void *buf, int *count, MPI_Datatype * datatype,
			    int *dest, int *tag, MPI_Comm * comm,
			    MPI_Request * request, int *res);

void ffunc (mpi_bsend_init) (void *buf, int *count, MPI_Datatype * datatype,
			     int *dest, int *tag, MPI_Comm * comm,
			     MPI_Request * request, int *res);

void ffunc (mpi_ssend_init) (void *buf, int *count, MPI_Datatype * datatype,
			     int *dest, int *tag, MPI_Comm * comm,
			     MPI_Request * request, int *res);

void ffunc (mpi_rsend_init) (void *buf, int *count, MPI_Datatype * datatype,
			     int *dest, int *tag, MPI_Comm * comm,
			     MPI_Request * request, int *res);

void ffunc (mpi_recv_init) (void *buf, int *count, MPI_Datatype * datatype,
			    int *source, int *tag, MPI_Comm * comm,
			    MPI_Request * request, int *res);

void ffunc (mpi_start) (MPI_Request * request, int *res);

void ffunc (mpi_startall) (int *count, MPI_Request array_of_requests[],
			   int *res);

void ffunc (mpi_sendrecv) (void *sendbuf, int *sendcount,
			   MPI_Datatype * sendtype, int *dest, int *sendtag,
			   void *recvbuf, int *recvcount,
			   MPI_Datatype * recvtype, int *source,
			   int *recvtag, MPI_Comm * comm,
			   MPI_Status * status, int *res);

void ffunc (mpi_sendrecv_replace) (void *buf, int *count,
				   MPI_Datatype * datatype, int *dest,
				   int *sendtag, int *source, int *recvtag,
				   MPI_Comm * comm, MPI_Status * status,
				   int *res);

void ffunc (mpi_type_contiguous) (int *count,
				  MPI_Datatype * old_type,
				  MPI_Datatype * new_type_p, int *res);

void ffunc (mpi_type_vector) (int *count,
			      int *blocklength,
			      int *stride, MPI_Datatype * old_type,
			      MPI_Datatype * newtype_p, int *res);

void ffunc (mpi_type_hvector) (int *count,
			       int *blocklen,
			       MPI_Aint * stride,
			       MPI_Datatype * old_type,
			       MPI_Datatype * newtype_p, int *res);

void ffunc (mpi_type_create_hvector) (int *count,
			       int *blocklen,
			       MPI_Aint * stride,
			       MPI_Datatype * old_type,
			       MPI_Datatype * newtype_p, int *res);

void ffunc (mpi_type_indexed) (int *count,
			       int blocklens[],
			       int indices[],
			       MPI_Datatype * old_type,
			       MPI_Datatype * newtype, int *res);

void ffunc (mpi_type_hindexed) (int *count,
				int blocklens[],
				MPI_Aint indices[],
				MPI_Datatype * old_type,
				MPI_Datatype * newtype, int *res);

void ffunc (mpi_type_create_hindexed) (int *count,
				int blocklens[],
				MPI_Aint indices[],
				MPI_Datatype * old_type,
				MPI_Datatype * newtype, int *res);

void ffunc (mpi_type_struct) (int *count,
			      int blocklens[],
			      MPI_Aint indices[],
			      MPI_Datatype old_types[],
			      MPI_Datatype * newtype, int *res);
void ffunc (mpi_type_create_struct) (int *count,
			      int blocklens[],
			      MPI_Aint indices[],
			      MPI_Datatype old_types[],
			      MPI_Datatype * newtype, int *res);

void ffunc (mpi_address) (void *location, MPI_Aint * address, int *res);

  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
void ffunc (mpi_type_extent) (MPI_Datatype * datatype, MPI_Aint * extent,
			      int *res);

  /* See the 1.1 version of the Standard.  The standard made an 
     unfortunate choice here, however, it is the standard.  The size returned 
     by MPI_Type_size is specified as an int, not an MPI_Aint */
void ffunc (mpi_type_size) (MPI_Datatype * datatype, int *size, int *res);

  /* MPI_Type_count was withdrawn in MPI 1.1 */
void ffunc (mpi_type_lb) (MPI_Datatype * datatype, MPI_Aint * displacement,
			  int *res);

void ffunc (mpi_type_ub) (MPI_Datatype * datatype, MPI_Aint * displacement,
			  int *res);

void ffunc (mpi_type_commit) (MPI_Datatype * datatype, int *res);

void ffunc (mpi_type_free) (MPI_Datatype * datatype, int *res);

void ffunc (mpi_get_elements) (MPI_Status * status, MPI_Datatype * datatype,
			       int *elements, int *res);

void ffunc (mpi_pack) (void *inbuf,
		       int *incount,
		       MPI_Datatype * datatype,
		       void *outbuf, int *outcount, int *position,
		       MPI_Comm * comm, int *res);

void ffunc (mpi_unpack) (void *inbuf,
			 int *insize,
			 int *position,
			 void *outbuf, int *outcount,
			 MPI_Datatype * datatype, MPI_Comm * comm, int *res);

void ffunc (mpi_pack_size) (int *incount, MPI_Datatype * datatype,
			    MPI_Comm * comm, int *size, int *res);

void ffunc (mpi_barrier) (MPI_Comm * comm, int *res);

void ffunc (mpi_bcast) (void *buffer, int *count, MPI_Datatype * datatype,
			int *root, MPI_Comm * comm, int *res);

void ffunc (mpi_gather) (void *sendbuf, int *sendcnt,
			 MPI_Datatype * sendtype, void *recvbuf,
			 int *recvcnt, MPI_Datatype * recvtype, int *root,
			 MPI_Comm * comm, int *res);

void ffunc (mpi_gatherv) (void *sendbuf, int *sendcnt,
			  MPI_Datatype * sendtype, void *recvbuf,
			  int *recvcnts, int *displs,
			  MPI_Datatype * recvtype, int *root,
			  MPI_Comm * comm, int *res);

void ffunc (mpi_scatter) (void *sendbuf, int *sendcnt,
			  MPI_Datatype * sendtype, void *recvbuf,
			  int *recvcnt, MPI_Datatype * recvtype, int *root,
			  MPI_Comm * comm, int *res);

void ffunc (mpi_scatterv) (void *sendbuf, int *sendcnts, int *displs,
			   MPI_Datatype * sendtype, void *recvbuf,
			   int *recvcnt, MPI_Datatype * recvtype, int *root,
			   MPI_Comm * comm, int *res);

void ffunc (mpi_allgather) (void *sendbuf, int *sendcount,
			    MPI_Datatype * sendtype, void *recvbuf,
			    int *recvcount, MPI_Datatype * recvtype,
			    MPI_Comm * comm, int *res);

void ffunc (mpi_allgatherv) (void *sendbuf, int *sendcount,
			     MPI_Datatype * sendtype, void *recvbuf,
			     int *recvcounts, int *displs,
			     MPI_Datatype * recvtype, MPI_Comm * comm,
			     int *res);

void ffunc (mpi_alltoall) (void *sendbuf, int *sendcount,
			   MPI_Datatype * sendtype, void *recvbuf,
			   int *recvcount, MPI_Datatype * recvtype,
			   MPI_Comm * comm, int *res);

void ffunc (mpi_alltoallv) (void *sendbuf, int *sendcnts, int *sdispls,
			    MPI_Datatype * sendtype, void *recvbuf,
			    int *recvcnts, int *rdispls,
			    MPI_Datatype * recvtype, MPI_Comm * comm,
			    int *res);

void ffunc (mpi_reduce) (void *sendbuf, void *recvbuf, int *count,
			 MPI_Datatype * datatype, MPI_Op * op, int *root,
			 MPI_Comm * comm, int *res);

void ffunc (mpi_op_create) (MPI_User_function * function, int *commute,
			    MPI_Op * op, int *res);

void ffunc (mpi_op_free) (MPI_Op * op, int *res);

void ffunc (mpi_allreduce) (void *sendbuf, void *recvbuf, int *count,
			    MPI_Datatype * datatype, MPI_Op * op,
			    MPI_Comm * comm, int *res);

void ffunc (mpi_reduce_scatter) (void *sendbuf, void *recvbuf, int *recvcnts,
				 MPI_Datatype * datatype, MPI_Op * op,
				 MPI_Comm * comm, int *res);

void ffunc (mpi_scan) (void *sendbuf, void *recvbuf, int *count,
		       MPI_Datatype * datatype, MPI_Op * op, MPI_Comm * comm,
		       int *res);

void ffunc (mpi_exscan) (void *sendbuf, void *recvbuf, int *count,
		       MPI_Datatype * datatype, MPI_Op * op, MPI_Comm * comm,
		       int *res);

void ffunc (mpi_group_size) (MPI_Group * group, int *size, int *res);

void ffunc (mpi_group_rank) (MPI_Group * group, int *rank, int *res);

void ffunc (mpi_group_translate_ranks) (MPI_Group * group1, int *n,
					int *ranks1, MPI_Group * group2,
					int *ranks2, int *res);

void ffunc (mpi_group_compare) (MPI_Group * group1, MPI_Group * group2,
				int *result, int *res);

void ffunc (mpi_comm_group) (MPI_Comm * comm, MPI_Group * group, int *res);

void ffunc (mpi_group_union) (MPI_Group * group1, MPI_Group * group2,
			      MPI_Group * newgroup, int *res);

void ffunc (mpi_group_intersection) (MPI_Group * group1, MPI_Group * group2,
				     MPI_Group * newgroup, int *res);

void ffunc (mpi_group_difference) (MPI_Group * group1, MPI_Group * group2,
				   MPI_Group * newgroup, int *res);

void ffunc (mpi_group_incl) (MPI_Group * group, int *n, int *ranks,
			     MPI_Group * newgroup, int *res);

void ffunc (mpi_group_excl) (MPI_Group * group, int *n, int *ranks,
			     MPI_Group * newgroup, int *res);

void ffunc (mpi_group_range_incl) (MPI_Group * group, int *n,
				   int ranges[][3], MPI_Group * newgroup,
				   int *res);

void ffunc (mpi_group_range_excl) (MPI_Group * group, int *n,
				   int ranges[][3], MPI_Group * newgroup,
				   int *res);

void ffunc (mpi_group_free) (MPI_Group * group, int *res);

void ffunc (mpi_comm_size) (MPI_Comm * comm, int *size, int *res);

void ffunc (mpi_comm_rank) (MPI_Comm * comm, int *rank, int *res);

void ffunc (mpi_comm_compare) (MPI_Comm * comm1, MPI_Comm * comm2,
			       int *result, int *res);

void ffunc (mpi_comm_dup) (MPI_Comm * comm, MPI_Comm * newcomm, int *res);

void ffunc (mpi_comm_create) (MPI_Comm * comm, MPI_Group * group,
			      MPI_Comm * newcomm, int *res);

void ffunc (mpi_comm_split) (MPI_Comm * comm, int *color, int *key,
			     MPI_Comm * newcomm, int *res);

void ffunc (mpi_comm_free) (MPI_Comm * comm, int *res);

void ffunc (mpi_comm_test_inter) (MPI_Comm * comm, int *flag, int *res);

void ffunc (mpi_comm_remote_size) (MPI_Comm * comm, int *size, int *res);

void ffunc (mpi_comm_remote_group) (MPI_Comm * comm, MPI_Group * group,
				    int *res);

void ffunc (mpi_intercomm_create) (MPI_Comm * local_comm, int *local_leader,
				   MPI_Comm * peer_comm, int *remote_leader,
				   int *tag, MPI_Comm * newintercomm,
				   int *res);

void ffunc (mpi_intercomm_merge) (MPI_Comm * intercomm, int *high,
				  MPI_Comm * newintracomm, int *res);

void ffunc (mpi_keyval_create) (MPI_Copy_function * copy_fn,
				MPI_Delete_function * delete_fn,
				int *keyval, void *extra_state, int *res);

void ffunc (mpi_keyval_free) (int *keyval, int *res);

void ffunc (mpi_attr_put) (MPI_Comm * comm, int *keyval, void *attr_value,
			   int *res);

void ffunc (mpi_attr_get) (MPI_Comm * comm, int *keyval, void *attr_value,
			   int *flag, int *res);

void ffunc (mpi_attr_delete) (MPI_Comm * comm, int *keyval, int *res);

void ffunc (mpi_topo_test) (MPI_Comm * comm, int *topo_type, int *res);

void ffunc (mpi_cart_create) (MPI_Comm * comm_old, int *ndims, int *dims,
			      int *periods, int *reorder,
			      MPI_Comm * comm_cart, int *res);

void ffunc (mpi_dims_create) (int *nnodes, int *ndims, int *dims, int *res);

void ffunc (mpi_graph_create) (MPI_Comm * comm_old, int *nnodes, int *index,
			       int *edges, int *reorder,
			       MPI_Comm * comm_graph, int *res);

void ffunc (mpi_graphdims_get) (MPI_Comm * comm, int *nnodes, int *nedges,
				int *res);

void ffunc (mpi_graph_get) (MPI_Comm * comm, int *maxindex, int *maxedges,
			    int *index, int *edges, int *res);

void ffunc (mpi_cartdim_get) (MPI_Comm * comm, int *ndims, int *res);

void ffunc (mpi_cart_get) (MPI_Comm * comm, int *maxdims, int *dims,
			   int *periods, int *coords, int *res);

void ffunc (mpi_cart_rank) (MPI_Comm * comm, int *coords, int *rank,
			    int *res);

void ffunc (mpi_cart_coords) (MPI_Comm * comm, int *rank, int *maxdims,
			      int *coords, int *res);

void ffunc (mpi_graph_neighbors_count) (MPI_Comm * comm, int *rank,
					int *nneighbors, int *res);

void ffunc (mpi_graph_neighbors) (MPI_Comm * comm, int *rank,
				  int *maxneighbors, int *neighbors,
				  int *res);

void ffunc (mpi_cart_shift) (MPI_Comm * comm, int *direction, int *displ,
			     int *source, int *dest, int *res);

void ffunc (mpi_cart_sub) (MPI_Comm * comm, int *remain_dims,
			   MPI_Comm * comm_new, int *res);

void ffunc (mpi_cart_map) (MPI_Comm * comm_old, int *ndims, int *dims,
			   int *periods, int *newrank, int *res);

void ffunc (mpi_graph_map) (MPI_Comm * comm_old, int *nnodes, int *index,
			    int *edges, int *newrank, int *res);

void ffunc (mpi_get_processor_name) (char *name, int *resultlen, int *res);

void ffunc (mpi_get_version) (int *version, int *subversion, int *res);

void ffunc (mpi_errhandler_create) (MPI_Handler_function * function,
				    MPI_Errhandler * errhandler, int *res);

void ffunc (mpi_errhandler_set) (MPI_Comm * comm,
				 MPI_Errhandler * errhandler, int *res);

void ffunc (mpi_errhandler_get) (MPI_Comm * comm,
				 MPI_Errhandler * errhandler, int *res);

void ffunc (mpi_errhandler_free) (MPI_Errhandler * errhandler, int *res);

void ffunc (mpi_error_string) (int *errorcode, char *string, int *resultlen,
			       int *res);

void ffunc (mpi_error_class) (int *errorcode, int *errorclass, int *res);

double ffunc (mpi_wtime) ();

double ffunc (mpi_wtick) ();

void ffunc (mpi_init) (int *res);

void ffunc (mpi_init_thread) (int *required, int *provide, int *res);

void ffunc (mpi_finalize) (int *res);

void ffunc (mpi_initialized) (int *flag, int *res);

void ffunc (mpi_abort) (MPI_Comm * comm, int *errorcode, int *res);

void ffunc (mpi_pcontrol) (const int level, ...);

void ffunc (mpi_comm_get_name) (MPI_Comm * a, char *b SCTK_CHAR_MIXED (size),
				int *c, int *res SCTK_CHAR_END (size));

void ffunc (mpi_comm_set_name) (MPI_Comm * a, char *b SCTK_CHAR_MIXED (size),
				int *res SCTK_CHAR_END (size));

/*Fortran binding*/
void ffunc (pmpi_send) (void *buf, int *count, MPI_Datatype * datatype,
			int *dest, int *tag, MPI_Comm * comm, int *res)
{
  *res = MPI_Send (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (pmpi_recv) (void *buf, int *count, MPI_Datatype * datatype,
			int *source, int *tag, MPI_Comm * comm,
			MPI_Status * status, int *res)
{
  *res = MPI_Recv (buf, *count, *datatype, *source, *tag, *comm, status);
}

void ffunc (pmpi_get_count) (MPI_Status * status, MPI_Datatype * datatype,
			     int *count, int *res)
{
  *res = MPI_Get_count (status, *datatype, count);
}

void ffunc (pmpi_bsend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm, int *res)
{
  *res = MPI_Bsend (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (pmpi_ssend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm, int *res)
{
  *res = MPI_Ssend (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (pmpi_rsend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm, int *res)
{
  *res = MPI_Rsend (buf, *count, *datatype, *dest, *tag, *comm);
}

void ffunc (pmpi_buffer_attach) (void *buffer, int *size, int *res)
{
  *res = MPI_Buffer_attach (buffer, *size);
}

void ffunc (pmpi_buffer_detach) (void *buffer, int *size, int *res)
{
  *res = MPI_Buffer_detach (buffer, size);
}

void ffunc (pmpi_isend) (void *buf, int *count, MPI_Datatype * datatype,
			 int *dest, int *tag, MPI_Comm * comm,
			 MPI_Request * request, int *res)
{
  *res = MPI_Isend (buf, *count, *datatype, *dest, *tag, *comm, request);
}

void ffunc (pmpi_ibsend) (void *buf, int *count, MPI_Datatype * datatype,
			  int *dest, int *tag, MPI_Comm * comm,
			  MPI_Request * request, int *res)
{
  *res = MPI_Ibsend (buf, *count, *datatype, *dest, *tag, *comm, request);
}

void ffunc (pmpi_issend) (void *buf, int *count, MPI_Datatype * datatype,
			  int *dest, int *tag, MPI_Comm * comm,
			  MPI_Request * request, int *res)
{
  *res = MPI_Issend (buf, *count, *datatype, *dest, *tag, *comm, request);
}

void ffunc (pmpi_irsend) (void *buf, int *count, MPI_Datatype * datatype,
			  int *dest, int *tag, MPI_Comm * comm,
			  MPI_Request * request, int *res)
{
  *res = MPI_Irsend (buf, *count, *datatype, *dest, *tag, *comm, request);
}

void ffunc (pmpi_irecv) (void *buf, int *count, MPI_Datatype * datatype,
			 int *source, int *tag, MPI_Comm * comm,
			 MPI_Request * request, int *res)
{
  *res = MPI_Irecv (buf, *count, *datatype, *source, *tag, *comm, request);
}

void ffunc (pmpi_wait) (MPI_Request * request, MPI_Status * status, int *res)
{
  *res = MPI_Wait (request, status);
}

void ffunc (pmpi_test) (MPI_Request * request, int *flag, MPI_Status * status,
			int *res)
{
  *res = MPI_Test (request, flag, status);

}

void ffunc (pmpi_request_free) (MPI_Request * request, int *res)
{
  *res = MPI_Request_free (request);

}

void ffunc (pmpi_waitany) (int *count, MPI_Request array_of_requests[],
			   int *index, MPI_Status * status, int *res)
{
  *res = MPI_Waitany (*count, array_of_requests, index, status);
  *index = (*index) + 1;

}

void ffunc (pmpi_testany) (int *count, MPI_Request array_of_requests[],
			   int *index, int *flag, MPI_Status * status,
			   int *res)
{
  *res = MPI_Testany (*count, array_of_requests, index, flag, status);
  *index = (*index) + 1;

}

void ffunc (pmpi_waitall) (int *count, MPI_Request array_of_requests[],
			   MPI_Status array_of_statuses[], int *res)
{
  *res = MPI_Waitall (*count, array_of_requests, array_of_statuses);
}

void ffunc (pmpi_testall) (int *count, MPI_Request array_of_requests[],
			   int *flag, MPI_Status array_of_statuses[],
			   int *res)
{
  *res = MPI_Testall (*count, array_of_requests, flag, array_of_statuses);

}

void ffunc (pmpi_waitsome) (int *incount, MPI_Request array_of_requests[],
			    int *outcount, int array_of_indices[],
			    MPI_Status array_of_statuses[], int *res)
{
  int i;
  *res =
    MPI_Waitsome (*incount, array_of_requests, outcount, array_of_indices,
		   array_of_statuses);
  for (i = 0; i < *outcount; i++)
    {
      array_of_indices[i]++;
    }
}

void ffunc (pmpi_testsome) (int *incount, MPI_Request array_of_requests[],
			    int *outcount, int array_of_indices[],
			    MPI_Status array_of_statuses[], int *res)
{
  int i;
  *res =
    MPI_Testsome (*incount, array_of_requests, outcount, array_of_indices,
		   array_of_statuses);
  for (i = 0; i < *outcount; i++)
    {
      array_of_indices[i]++;
    }

}

void ffunc (pmpi_iprobe) (int *source, int *tag, MPI_Comm * comm, int *flag,
			  MPI_Status * status, int *res)
{
  *res = MPI_Iprobe (*source, *tag, *comm, flag, status);
}

void ffunc (pmpi_probe) (int *source, int *tag, MPI_Comm * comm,
			 MPI_Status * status, int *res)
{
  *res = MPI_Probe (*source, *tag, *comm, status);
}

void ffunc (pmpi_cancel) (MPI_Request * request, int *res)
{
  *res = MPI_Cancel (request);

}

void ffunc (pmpi_test_cancelled) (MPI_Status * status, int *flag, int *res)
{
  *res = MPI_Test_cancelled (status, flag);

}

void ffunc (pmpi_send_init) (void *buf, int *count, MPI_Datatype * datatype,
			     int *dest, int *tag, MPI_Comm * comm,
			     MPI_Request * request, int *res)
{
  *res = MPI_Send_init (buf, *count, *datatype, *dest, *tag, *comm, request);

}

void ffunc (pmpi_bsend_init) (void *buf, int *count, MPI_Datatype * datatype,
			      int *dest, int *tag, MPI_Comm * comm,
			      MPI_Request * request, int *res)
{
  *res =
    MPI_Bsend_init (buf, *count, *datatype, *dest, *tag, *comm, request);

}

void ffunc (pmpi_ssend_init) (void *buf, int *count, MPI_Datatype * datatype,
			      int *dest, int *tag, MPI_Comm * comm,
			      MPI_Request * request, int *res)
{
  *res =
    MPI_Ssend_init (buf, *count, *datatype, *dest, *tag, *comm, request);

}

void ffunc (pmpi_rsend_init) (void *buf, int *count, MPI_Datatype * datatype,
			      int *dest, int *tag, MPI_Comm * comm,
			      MPI_Request * request, int *res)
{
  *res =
    MPI_Rsend_init (buf, *count, *datatype, *dest, *tag, *comm, request);

}

void ffunc (pmpi_recv_init) (void *buf, int *count, MPI_Datatype * datatype,
			     int *source, int *tag, MPI_Comm * comm,
			     MPI_Request * request, int *res)
{
  *res =
    MPI_Recv_init (buf, *count, *datatype, *source, *tag, *comm, request);

}

void ffunc (pmpi_start) (MPI_Request * request, int *res)
{
  *res = MPI_Start (request);

}

void ffunc (pmpi_startall) (int *count, MPI_Request array_of_requests[],
			    int *res)
{
  *res = MPI_Startall (*count, array_of_requests);

}

void ffunc (pmpi_sendrecv) (void *sendbuf, int *sendcount,
			    MPI_Datatype * sendtype, int *dest, int *sendtag,
			    void *recvbuf, int *recvcount,
			    MPI_Datatype * recvtype, int *source,
			    int *recvtag, MPI_Comm * comm,
			    MPI_Status * status, int *res)
{
  *res =
    MPI_Sendrecv (sendbuf, *sendcount, *sendtype, *dest, *sendtag, recvbuf,
		   *recvcount, *recvtype, *source, *recvtag, *comm, status);

}

void ffunc (pmpi_sendrecv_replace) (void *buf, int *count,
				    MPI_Datatype * datatype, int *dest,
				    int *sendtag, int *source, int *recvtag,
				    MPI_Comm * comm, MPI_Status * status,
				    int *res)
{
  *res =
    MPI_Sendrecv_replace (buf, *count, *datatype, *dest, *sendtag, *source,
			   *recvtag, *comm, status);

}

void ffunc (pmpi_type_contiguous) (int *count,
				   MPI_Datatype * old_type,
				   MPI_Datatype * new_type_p, int *res)
{
  *res = MPI_Type_contiguous (*count, *old_type, new_type_p);

}

void ffunc (pmpi_type_vector) (int *count,
			       int *blocklength,
			       int *stride, MPI_Datatype * old_type,
			       MPI_Datatype * newtype_p, int *res)
{
  *res =
    MPI_Type_vector (*count, *blocklength, *stride, *old_type, newtype_p);

}

void ffunc (pmpi_type_create_hvector) (int *count,
				int *blocklen,
				MPI_Aint * stride,
				MPI_Datatype * old_type,
				MPI_Datatype * newtype_p, int *res)
{
  *res = MPI_Type_create_hvector (*count, *blocklen, *stride, *old_type, newtype_p);

}

void ffunc (pmpi_type_hvector) (int *count,
				int *blocklen,
				MPI_Aint * stride,
				MPI_Datatype * old_type,
				MPI_Datatype * newtype_p, int *res)
{
  *res = MPI_Type_hvector (*count, *blocklen, *stride, *old_type, newtype_p);

}

void ffunc (pmpi_type_indexed) (int *count,
				int blocklens[],
				int indices[],
				MPI_Datatype * old_type,
				MPI_Datatype * newtype, int *res)
{
  *res = MPI_Type_indexed (*count, blocklens, indices, *old_type, newtype);

}

void ffunc (pmpi_type_hindexed) (int *count,
				 int blocklens[],
				 MPI_Aint indices[],
				 MPI_Datatype * old_type,
				 MPI_Datatype * newtype, int *res)
{
  *res = MPI_Type_hindexed (*count, blocklens, indices, *old_type, newtype);

}

void ffunc (pmpi_type_create_hindexed) (int *count,
				 int blocklens[],
				 MPI_Aint indices[],
				 MPI_Datatype * old_type,
				 MPI_Datatype * newtype, int *res)
{
  *res = MPI_Type_create_hindexed (*count, blocklens, indices, *old_type, newtype);

}

void ffunc (pmpi_type_struct) (int *count,
			       int blocklens[],
			       MPI_Aint indices[],
			       MPI_Datatype old_types[],
			       MPI_Datatype * newtype, int *res)
{
  *res = MPI_Type_struct (*count, blocklens, indices, old_types, newtype);

}

void ffunc (pmpi_type_create_struct) (int *count,
			       int blocklens[],
			       MPI_Aint indices[],
			       MPI_Datatype old_types[],
			       MPI_Datatype * newtype, int *res)
{
  *res = MPI_Type_create_struct (*count, blocklens, indices, old_types, newtype);

}

void ffunc (pmpi_address) (void *location, MPI_Aint * address, int *res)
{
  *res = MPI_Address (location, address);

}

  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
void ffunc (pmpi_type_extent) (MPI_Datatype * datatype, MPI_Aint * extent,
			       int *res)
{
  *res = MPI_Type_extent (*datatype, extent);

}

  /* See the 1.1 version of the Standard.  The standard made an 
     unfortunate choice here, however, it is the standard.  The size returned 
     by MPI_Type_size is specified as an int, not an MPI_Aint */
void ffunc (pmpi_type_size) (MPI_Datatype * datatype, int *size, int *res)
{
  *res = MPI_Type_size (*datatype, size);

}

  /* MPI_Type_count was withdrawn in MPI 1.1 */
void ffunc (pmpi_type_lb) (MPI_Datatype * datatype, MPI_Aint * displacement,
			   int *res)
{
  *res = MPI_Type_lb (*datatype, displacement);

}

void ffunc (pmpi_type_ub) (MPI_Datatype * datatype, MPI_Aint * displacement,
			   int *res)
{
  *res = MPI_Type_ub (*datatype, displacement);

}

void ffunc (pmpi_type_commit) (MPI_Datatype * datatype, int *res)
{
  *res = MPI_Type_commit (datatype);

}

void ffunc (pmpi_type_free) (MPI_Datatype * datatype, int *res)
{
  *res = MPI_Type_free (datatype);

}

void ffunc (pmpi_get_elements) (MPI_Status * status, MPI_Datatype * datatype,
				int *elements, int *res)
{
  *res = MPI_Get_elements (status, *datatype, elements);

}

void ffunc (pmpi_pack) (void *inbuf,
			int *incount,
			MPI_Datatype * datatype,
			void *outbuf, int *outcount, int *position,
			MPI_Comm * comm, int *res)
{
  *res =
    MPI_Pack (inbuf, *incount, *datatype, outbuf, *outcount, position,
	       *comm);

}

void ffunc (pmpi_unpack) (void *inbuf,
			  int *insize,
			  int *position,
			  void *outbuf, int *outcount,
			  MPI_Datatype * datatype, MPI_Comm * comm, int *res)
{
  *res =
    MPI_Unpack (inbuf, *insize, position, outbuf, *outcount, *datatype,
		 *comm);

}

void ffunc (pmpi_pack_size) (int *incount, MPI_Datatype * datatype,
			     MPI_Comm * comm, int *size, int *res)
{
  *res = MPI_Pack_size (*incount, *datatype, *comm, size);

}

void ffunc (pmpi_barrier) (MPI_Comm * comm, int *res)
{
  *res = MPI_Barrier (*comm);

}

void ffunc (pmpi_bcast) (void *buffer, int *count, MPI_Datatype * datatype,
			 int *root, MPI_Comm * comm, int *res)
{
  *res = MPI_Bcast (buffer, *count, *datatype, *root, *comm);

}

void ffunc (pmpi_gather) (void *sendbuf, int *sendcnt,
			  MPI_Datatype * sendtype, void *recvbuf,
			  int *recvcnt, MPI_Datatype * recvtype, int *root,
			  MPI_Comm * comm, int *res)
{
  *res =
    MPI_Gather (sendbuf, *sendcnt, *sendtype, recvbuf, *recvcnt, *recvtype,
		 *root, *comm);

}

void ffunc (pmpi_gatherv) (void *sendbuf, int *sendcnt,
			   MPI_Datatype * sendtype, void *recvbuf,
			   int *recvcnts, int *displs,
			   MPI_Datatype * recvtype, int *root,
			   MPI_Comm * comm, int *res)
{
  *res =
    MPI_Gatherv (sendbuf, *sendcnt, *sendtype, recvbuf, recvcnts, displs,
		  *recvtype, *root, *comm);

}

void ffunc (pmpi_scatter) (void *sendbuf, int *sendcnt,
			   MPI_Datatype * sendtype, void *recvbuf,
			   int *recvcnt, MPI_Datatype * recvtype, int *root,
			   MPI_Comm * comm, int *res)
{
  *res =
    MPI_Scatter (sendbuf, *sendcnt, *sendtype, recvbuf, *recvcnt, *recvtype,
		  *root, *comm);

}

void ffunc (pmpi_scatterv) (void *sendbuf, int *sendcnts, int *displs,
			    MPI_Datatype * sendtype, void *recvbuf,
			    int *recvcnt, MPI_Datatype * recvtype, int *root,
			    MPI_Comm * comm, int *res)
{
  *res =
    MPI_Scatterv (sendbuf, sendcnts, displs, *sendtype, recvbuf, *recvcnt,
		   *recvtype, *root, *comm);

}

void ffunc (pmpi_allgather) (void *sendbuf, int *sendcount,
			     MPI_Datatype * sendtype, void *recvbuf,
			     int *recvcount, MPI_Datatype * recvtype,
			     MPI_Comm * comm, int *res)
{
  *res =
    MPI_Allgather (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
		    *recvtype, *comm);

}

void ffunc (pmpi_allgatherv) (void *sendbuf, int *sendcount,
			      MPI_Datatype * sendtype, void *recvbuf,
			      int *recvcounts, int *displs,
			      MPI_Datatype * recvtype, MPI_Comm * comm,
			      int *res)
{
  *res =
    MPI_Allgatherv (sendbuf, *sendcount, *sendtype, recvbuf, recvcounts,
		     displs, *recvtype, *comm);
}

void ffunc (pmpi_alltoall) (void *sendbuf, int *sendcount,
			    MPI_Datatype * sendtype, void *recvbuf,
			    int *recvcount, MPI_Datatype * recvtype,
			    MPI_Comm * comm, int *res)
{
  *res =
    MPI_Alltoall (sendbuf, *sendcount, *sendtype, recvbuf, *recvcount,
		   *recvtype, *comm);

}

void ffunc (pmpi_alltoallv) (void *sendbuf, int *sendcnts, int *sdispls,
			     MPI_Datatype * sendtype, void *recvbuf,
			     int *recvcnts, int *rdispls,
			     MPI_Datatype * recvtype, MPI_Comm * comm,
			     int *res)
{
  *res =
    MPI_Alltoallv (sendbuf, sendcnts, sdispls, *sendtype, recvbuf, recvcnts,
		    rdispls, *recvtype, *comm);

}

void ffunc (pmpi_reduce) (void *sendbuf, void *recvbuf, int *count,
			  MPI_Datatype * datatype, MPI_Op * op, int *root,
			  MPI_Comm * comm, int *res)
{
  *res = MPI_Reduce (sendbuf, recvbuf, *count, *datatype, *op, *root, *comm);

}

void ffunc (pmpi_op_create) (MPI_User_function * function, int *commute,
			     MPI_Op * op, int *res)
{
  *res = MPI_Op_create (function, *commute, op);

}

void ffunc (pmpi_op_free) (MPI_Op * op, int *res)
{
  *res = MPI_Op_free (op);

}

void ffunc (pmpi_allreduce) (void *sendbuf, void *recvbuf, int *count,
			     MPI_Datatype * datatype, MPI_Op * op,
			     MPI_Comm * comm, int *res)
{
  *res = MPI_Allreduce (sendbuf, recvbuf, *count, *datatype, *op, *comm);

}

void ffunc (pmpi_reduce_scatter) (void *sendbuf, void *recvbuf, int *recvcnts,
				  MPI_Datatype * datatype, MPI_Op * op,
				  MPI_Comm * comm, int *res)
{
  *res =
    MPI_Reduce_scatter (sendbuf, recvbuf, recvcnts, *datatype, *op, *comm);

}

void ffunc (pmpi_scan) (void *sendbuf, void *recvbuf, int *count,
			MPI_Datatype * datatype, MPI_Op * op, MPI_Comm * comm,
			int *res)
{
  *res = MPI_Scan (sendbuf, recvbuf, *count, *datatype, *op, *comm);

}

void ffunc (pmpi_exscan) (void *sendbuf, void *recvbuf, int *count,
			MPI_Datatype * datatype, MPI_Op * op, MPI_Comm * comm,
			int *res)
{
  *res = MPI_Scan (sendbuf, recvbuf, *count, *datatype, *op, *comm);

}

void ffunc (pmpi_group_size) (MPI_Group * group, int *size, int *res)
{
  *res = MPI_Group_size (*group, size);

}

void ffunc (pmpi_group_rank) (MPI_Group * group, int *rank, int *res)
{
  *res = MPI_Group_rank (*group, rank);

}

void ffunc (pmpi_group_translate_ranks) (MPI_Group * group1, int *n,
					 int *ranks1, MPI_Group * group2,
					 int *ranks2, int *res)
{
  *res = MPI_Group_translate_ranks (*group1, *n, ranks1, *group2, ranks2);

}

void ffunc (pmpi_group_compare) (MPI_Group * group1, MPI_Group * group2,
				 int *result, int *res)
{
  *res = MPI_Group_compare (*group1, *group2, result);

}

void ffunc (pmpi_comm_group) (MPI_Comm * comm, MPI_Group * group, int *res)
{
  *res = MPI_Comm_group (*comm, group);

}

void ffunc (pmpi_group_union) (MPI_Group * group1, MPI_Group * group2,
			       MPI_Group * newgroup, int *res)
{
  *res = MPI_Group_union (*group1, *group2, newgroup);

}

void ffunc (pmpi_group_intersection) (MPI_Group * group1, MPI_Group * group2,
				      MPI_Group * newgroup, int *res)
{
  *res = MPI_Group_intersection (*group1, *group2, newgroup);

}

void ffunc (pmpi_group_difference) (MPI_Group * group1, MPI_Group * group2,
				    MPI_Group * newgroup, int *res)
{
  *res = MPI_Group_difference (*group1, *group2, newgroup);

}

void ffunc (pmpi_group_incl) (MPI_Group * group, int *n, int *ranks,
			      MPI_Group * newgroup, int *res)
{
  *res = MPI_Group_incl (*group, *n, ranks, newgroup);

}

void ffunc (pmpi_group_excl) (MPI_Group * group, int *n, int *ranks,
			      MPI_Group * newgroup, int *res)
{
  *res = MPI_Group_excl (*group, *n, ranks, newgroup);

}

void ffunc (pmpi_group_range_incl) (MPI_Group * group, int *n,
				    int ranges[][3], MPI_Group * newgroup,
				    int *res)
{
  *res = MPI_Group_range_incl (*group, *n, ranges, newgroup);

}

void ffunc (pmpi_group_range_excl) (MPI_Group * group, int *n,
				    int ranges[][3], MPI_Group * newgroup,
				    int *res)
{
  *res = MPI_Group_range_excl (*group, *n, ranges, newgroup);

}

void ffunc (pmpi_group_free) (MPI_Group * group, int *res)
{
  *res = MPI_Group_free (group);

}

void ffunc (pmpi_comm_size) (MPI_Comm * comm, int *size, int *res)
{
  *res = MPI_Comm_size (*comm, size);
}

void ffunc (pmpi_comm_rank) (MPI_Comm * comm, int *rank, int *res)
{
  *res = MPI_Comm_rank (*comm, rank);
}

void ffunc (pmpi_comm_compare) (MPI_Comm * comm1, MPI_Comm * comm2,
				int *result, int *res)
{
  *res = MPI_Comm_compare (*comm1, *comm2, result);

}

void ffunc (pmpi_comm_dup) (MPI_Comm * comm, MPI_Comm * newcomm, int *res)
{
  *res = MPI_Comm_dup (*comm, newcomm);
}

void ffunc (pmpi_comm_create) (MPI_Comm * comm, MPI_Group * group,
			       MPI_Comm * newcomm, int *res)
{
  *res = MPI_Comm_create (*comm, *group, newcomm);

}

void ffunc (pmpi_comm_split) (MPI_Comm * comm, int *color, int *key,
			      MPI_Comm * newcomm, int *res)
{
  *res = MPI_Comm_split (*comm, *color, *key, newcomm);

}

void ffunc (pmpi_comm_free) (MPI_Comm * comm, int *res)
{
  *res = MPI_Comm_free (comm);

}

void ffunc (pmpi_comm_test_inter) (MPI_Comm * comm, int *flag, int *res)
{
  *res = MPI_Comm_test_inter (*comm, flag);

}

void ffunc (pmpi_comm_remote_size) (MPI_Comm * comm, int *size, int *res)
{
  *res = MPI_Comm_remote_size (*comm, size);

}

void ffunc (pmpi_comm_remote_group) (MPI_Comm * comm, MPI_Group * group,
				     int *res)
{
  *res = MPI_Comm_remote_group (*comm, group);

}

void ffunc (pmpi_intercomm_create) (MPI_Comm * local_comm, int *local_leader,
				    MPI_Comm * peer_comm, int *remote_leader,
				    int *tag, MPI_Comm * newintercomm,
				    int *res)
{
  *res =
    MPI_Intercomm_create (*local_comm, *local_leader, *peer_comm,
			   *remote_leader, *tag, newintercomm);

}

void ffunc (pmpi_intercomm_merge) (MPI_Comm * intercomm, int *high,
				   MPI_Comm * newintracomm, int *res)
{
  *res = MPI_Intercomm_merge (*intercomm, *high, newintracomm);

}

void ffunc (pmpi_keyval_create) (MPI_Copy_function * copy_fn,
				 MPI_Delete_function * delete_fn,
				 int *keyval, void *extra_state, int *res)
{
  *res = MPI_Keyval_create (copy_fn, delete_fn, keyval, extra_state);
  __INTERNAL__PMPI_Attr_set_fortran (*keyval);
}

void ffunc (pmpi_keyval_free) (int *keyval, int *res)
{
  *res = MPI_Keyval_free (keyval);

}

void ffunc (pmpi_attr_put) (MPI_Comm * comm, int *keyval, void *attr_value,
			    int *res)
{
  *res = MPI_Attr_put (*comm, *keyval, attr_value);

}

void ffunc (pmpi_attr_get) (MPI_Comm * comm, int *keyval, void *attr_value,
			    int *flag, int *res)
{
  *res = MPI_Attr_get (*comm, *keyval, attr_value, flag);

}

void ffunc (pmpi_attr_delete) (MPI_Comm * comm, int *keyval, int *res)
{
  *res = MPI_Attr_delete (*comm, *keyval);

}

void ffunc (pmpi_topo_test) (MPI_Comm * comm, int *topo_type, int *res)
{
  *res = MPI_Topo_test (*comm, topo_type);

}

void ffunc (pmpi_cart_create) (MPI_Comm * comm_old, int *ndims, int *dims,
			       int *periods, int *reorder,
			       MPI_Comm * comm_cart, int *res)
{
  *res =
    MPI_Cart_create (*comm_old, *ndims, dims, periods, *reorder, comm_cart);

}

void ffunc (pmpi_dims_create) (int *nnodes, int *ndims, int *dims, int *res)
{
  *res = MPI_Dims_create (*nnodes, *ndims, dims);

}

void ffunc (pmpi_graph_create) (MPI_Comm * comm_old, int *nnodes, int *index,
				int *edges, int *reorder,
				MPI_Comm * comm_graph, int *res)
{
  *res =
    MPI_Graph_create (*comm_old, *nnodes, index, edges, *reorder,
		       comm_graph);

}

void ffunc (pmpi_graphdims_get) (MPI_Comm * comm, int *nnodes, int *nedges,
				 int *res)
{
  *res = MPI_Graphdims_get (*comm, nnodes, nedges);

}

void ffunc (pmpi_graph_get) (MPI_Comm * comm, int *maxindex, int *maxedges,
			     int *index, int *edges, int *res)
{
  *res = MPI_Graph_get (*comm, *maxindex, *maxedges, index, edges);

}

void ffunc (pmpi_cartdim_get) (MPI_Comm * comm, int *ndims, int *res)
{
  *res = MPI_Cartdim_get (*comm, ndims);

}

void ffunc (pmpi_cart_get) (MPI_Comm * comm, int *maxdims, int *dims,
			    int *periods, int *coords, int *res)
{
  *res = MPI_Cart_get (*comm, *maxdims, dims, periods, coords);

}

void ffunc (pmpi_cart_rank) (MPI_Comm * comm, int *coords, int *rank,
			     int *res)
{
  *res = MPI_Cart_rank (*comm, coords, rank);

}

void ffunc (pmpi_cart_coords) (MPI_Comm * comm, int *rank, int *maxdims,
			       int *coords, int *res)
{
  *res = MPI_Cart_coords (*comm, *rank, *maxdims, coords);

}

void ffunc (pmpi_graph_neighbors_count) (MPI_Comm * comm, int *rank,
					 int *nneighbors, int *res)
{
  *res = MPI_Graph_neighbors_count (*comm, *rank, nneighbors);

}

void ffunc (pmpi_graph_neighbors) (MPI_Comm * comm, int *rank,
				   int *maxneighbors, int *neighbors,
				   int *res)
{
  *res = MPI_Graph_neighbors (*comm, *rank, *maxneighbors, neighbors);

}

void ffunc (pmpi_cart_shift) (MPI_Comm * comm, int *direction, int *displ,
			      int *source, int *dest, int *res)
{
  *res = MPI_Cart_shift (*comm, *direction, *displ, source, dest);

}

void ffunc (pmpi_cart_sub) (MPI_Comm * comm, int *remain_dims,
			    MPI_Comm * comm_new, int *res)
{
  *res = MPI_Cart_sub (*comm, remain_dims, comm_new);

}

void ffunc (pmpi_cart_map) (MPI_Comm * comm_old, int *ndims, int *dims,
			    int *periods, int *newrank, int *res)
{
  *res = MPI_Cart_map (*comm_old, *ndims, dims, periods, newrank);

}

void ffunc (pmpi_graph_map) (MPI_Comm * comm_old, int *nnodes, int *index,
			     int *edges, int *newrank, int *res)
{
  *res = MPI_Graph_map (*comm_old, *nnodes, index, edges, newrank);

}

void ffunc (pmpi_get_processor_name) (char *name, int *resultlen, int *res)
{
  *res = MPI_Get_processor_name (name, resultlen);

}

void ffunc (pmpi_get_version) (int *version, int *subversion, int *res)
{
  *res = MPI_Get_version (version, subversion);

}

void ffunc (pmpi_errhandler_create) (MPI_Handler_function * function,
				     MPI_Errhandler * errhandler, int *res)
{
  *res = MPI_Errhandler_create (function, errhandler);

}

void ffunc (pmpi_errhandler_set) (MPI_Comm * comm,
				  MPI_Errhandler * errhandler, int *res)
{
  *res = MPI_Errhandler_set (*comm, *errhandler);

}

void ffunc (pmpi_errhandler_get) (MPI_Comm * comm,
				  MPI_Errhandler * errhandler, int *res)
{
  *res = MPI_Errhandler_get (*comm, errhandler);

}

void ffunc (pmpi_errhandler_free) (MPI_Errhandler * errhandler, int *res)
{
  *res = MPI_Errhandler_free (errhandler);

}

void ffunc (pmpi_error_string) (int *errorcode, char *string, int *resultlen,
				int *res)
{
  *res = MPI_Error_string (*errorcode, string, resultlen);

}

void ffunc (pmpi_error_class) (int *errorcode, int *errorclass, int *res)
{
  *res = MPI_Error_class (*errorcode, errorclass);

}

double ffunc (pmpi_wtime) ()
{
  return MPI_Wtime ();
}

double ffunc (pmpi_wtick) ()
{
  return MPI_Wtick ();
}

void ffunc (pmpi_init) (int *res)
{
  *res = MPI_Init (NULL, NULL);
}

void ffunc (pmpi_init_thread) (int *required, int *provide, int *res)
{
  *res = MPI_Init_thread (NULL, NULL, *required, provide);
}

void ffunc (pmpi_finalized) (int *flag, int *res)
{
  *res = MPI_Finalized (flag);
}

void ffunc (pmpi_finalize) (int *res)
{
  *res = MPI_Finalize ();
}


void ffunc (pmpi_initialized) (int *flag, int *res)
{
  *res = MPI_Initialized (flag);
}

void ffunc (pmpi_abort) (MPI_Comm * comm, int *errorcode, int *res)
{
  *res = MPI_Abort (*comm, *errorcode);
}

void ffunc (pmpi_pcontrol) (const int level, ...)
{
  not_implemented ();
}

void ffunc (pmpi_comm_get_name) (MPI_Comm * a, char *b SCTK_CHAR_MIXED (size),
				 int *c, int *res SCTK_CHAR_END (size))
{
  *res = MPI_Comm_get_name (*a, b, c);
  sctk_char_c_to_fortran (b, size);
}

void ffunc (pmpi_comm_set_name) (MPI_Comm * a, char *b SCTK_CHAR_MIXED (size),
				 int *res SCTK_CHAR_END (size))
{
  char *tmp;
  tmp = sctk_char_fortran_to_c (b, size);
  *res = MPI_Comm_set_name (*a, tmp);
  sctk_free (tmp);
}
