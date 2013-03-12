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
#ifndef __SCTK_MPC_MPI_H_
#define __SCTK_MPC_MPI_H_

#include <stdlib.h>
#include <string.h>
#include <mpcmp.h>

#ifndef MPC_NO_AUTO_MAIN_REDEF
#undef main
#ifdef __cplusplus
#define main long mpc_user_main_dummy__ (); extern "C" int mpc_user_main__
#else
#define main mpc_user_main__
#endif
#endif


#ifdef __cplusplus
extern "C"
{
#endif

  /*
     #define MPI_ MPC_
   */
#define MPI_SOURCE MPC_SOURCE
#define MPI_TAG MPC_TAG
#define MPI_ERROR MPC_ERROR

#define MPI_SUCCESS MPC_SUCCESS
#define MPI_UNDEFINED MPC_UNDEFINED
#define MPI_REQUEST_NULL (-1)
#define MPI_COMM_WORLD MPC_COMM_WORLD
#define MPI_CART (-2)
#define MPI_GRAPH (-3)

/* Results of the compare operations. */
#define MPI_IDENT     0
#define MPI_CONGRUENT 1
#define MPI_SIMILAR   2
#define MPI_UNEQUAL   3

#define MPI_STATUS_IGNORE MPC_STATUS_IGNORE
#define MPI_STATUSES_IGNORE MPC_STATUS_IGNORE
#define MPI_ANY_TAG MPC_ANY_TAG
#define MPI_ANY_SOURCE MPC_ANY_SOURCE
#define MPI_PROC_NULL MPC_PROC_NULL
#define MPI_COMM_NULL MPC_COMM_NULL
#define MPI_MAX_PROCESSOR_NAME MPC_MAX_PROCESSOR_NAME
#define MPI_MAX_NAME_STRING 256

/* Communication argument parameters */
#define MPI_ERR_BUFFER MPC_ERR_BUFFER
#define MPI_ERR_COUNT MPC_ERR_COUNT
#define MPI_ERR_TYPE MPC_ERR_TYPE
#define MPI_ERR_TAG MPC_ERR_TAG
#define MPI_ERR_COMM MPC_ERR_COMM
#define MPI_ERR_RANK MPC_ERR_RANK
#define MPI_ERR_ROOT MPC_ERR_ROOT
#define MPI_ERR_TRUNCATE MPC_ERR_TRUNCATE
#define MPI_ERR_GROUP MPC_ERR_GROUP
#define MPI_ERR_OP MPC_ERR_OP
#define MPI_ERR_REQUEST MPC_ERR_REQUEST
#define MPI_ERR_TOPOLOGY MPC_ERR_TOPOLOGY
#define MPI_ERR_DIMS MPC_ERR_DIMS
#define MPI_ERR_ARG MPC_ERR_ARG
#define MPI_ERR_OTHER MPC_ERR_OTHER
#define MPI_ERR_UNKNOWN MPC_ERR_UNKNOWN
#define MPI_ERR_INTERN MPC_ERR_INTERN
#define MPI_ERR_KEYVAL MPC_ERR_KEYVAL
#define MPI_ERR_IN_STATUS MPC_ERR_IN_STATUS
#define MPI_ERR_PENDING MPC_ERR_PENDING
#define MPI_ERR_LASTCODE MPC_ERR_LASTCODE
#define MPI_NOT_IMPLEMENTED MPC_NOT_IMPLEMENTED

#define MPI_DATATYPE_NULL MPC_DATATYPE_NULL
#define MPI_UB MPC_UB
#define MPI_LB MPC_LB
#define MPI_CHAR MPC_CHAR
#define MPI_BYTE MPC_BYTE
#define MPI_SHORT MPC_SHORT
#define MPI_INT MPC_INT
/* Support for MPI_INTEGER */
#define MPI_INTEGER MPC_INT
#ifndef NOHAVE_ASSERT_H
	#define MPI_INTEGER1 MPC_INTEGER1
	#define MPI_INTEGER2 MPC_INTEGER2
	#define MPI_INTEGER4 MPC_INTEGER4
	#define MPI_INTEGER8 MPC_INTEGER8
#endif
#define MPI_LONG MPC_LONG
#define MPI_LONG_INT MPC_LONG_INT
#define MPI_FLOAT MPC_FLOAT
#define MPI_DOUBLE MPC_DOUBLE
#define MPI_UNSIGNED_CHAR MPC_UNSIGNED_CHAR
#define MPI_UNSIGNED_SHORT MPC_UNSIGNED_SHORT
#define MPI_UNSIGNED MPC_UNSIGNED
#define MPI_UNSIGNED_LONG MPC_UNSIGNED_LONG
#define MPI_LONG_DOUBLE MPC_LONG_DOUBLE
#define MPI_LONG_LONG_INT MPC_LONG_LONG_INT
#define MPI_LONG_LONG MPC_LONG_LONG
#define MPI_PACKED MPC_PACKED
#define MPI_FLOAT_INT MPC_FLOAT_INT
#define MPI_DOUBLE_INT MPC_DOUBLE_INT
#define MPI_SHORT_INT MPC_SHORT_INT
#define MPI_2INT MPC_2INT
#define MPI_2FLOAT MPC_2FLOAT
#define MPI_COMPLEX MPC_COMPLEX
#define MPI_DOUBLE_COMPLEX MPC_DOUBLE_COMPLEX
#define MPI_2DOUBLE_PRECISION MPC_2DOUBLE_PRECISION
#define MPI_LOGICAL MPC_LOGICAL
#define MPI_REAL4 MPC_REAL4
#define MPI_REAL8 MPC_REAL8
#define MPI_REAL16 MPC_REAL16


#define MPI_SUM 0
#define MPI_MAX 1
#define MPI_MIN 2
#define MPI_PROD 3
#define MPI_LAND 4
#define MPI_BAND 5
#define MPI_LOR 6
#define MPI_BOR 7
#define MPI_LXOR 8
#define MPI_BXOR 9
#define MPI_MINLOC 10
#define MPI_MAXLOC 11
#define MPI_OP_NULL ((MPI_Op)-1)


#define MPI_BOTTOM MPC_BOTTOM


#define MPI_GROUP_EMPTY 0
#define MPI_GROUP_NULL -1

  typedef MPC_Datatype MPI_Datatype;	/* unsigned int */
  typedef MPC_Comm MPI_Comm;	/* unsigned int */
  typedef int MPI_Request;
  typedef MPC_Aint MPI_Aint;	/* long */
  typedef int MPI_Errhandler;
  typedef MPC_User_function MPI_User_function;
  typedef int MPI_Op;
  typedef int MPI_Group;
  typedef MPC_Status MPI_Status;
  typedef MPC_Handler_function MPI_Handler_function;
  typedef int MPI_Fint;

  typedef struct
  {
    int size;
    MPI_Request request;
  } mpi_buffer_overhead_t;

#define MPI_BSEND_OVERHEAD (sizeof(mpi_buffer_overhead_t))

#define MPI_MAX_ERROR_STRING MPC_MAX_ERROR_STRING
  void MPI_Default_error (MPI_Comm * comm, int *error, char *msg,
			  char *file, int line);
  void MPI_Return_error (MPI_Comm * comm, int *error, ...);

#define MPI_ERRHANDLER_NULL 0
#define MPI_ERRORS_RETURN 1
#define MPI_ERRORS_ARE_FATAL 2

#define MPI_KEYVAL_INVALID -1

typedef int (MPI_Copy_function) (MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Delete_function) (MPI_Comm, int, void *, void *);

#define MPI_NULL_DELETE_FN ((MPI_Delete_function *)MPC_Mpi_null_delete_fn)
#define MPI_NULL_COPY_FN ((MPI_Copy_function *)MPC_Mpi_null_copy_fn)
#define MPI_DUP_FN ((MPI_Copy_function *)MPC_Mpi_dup_fn)

#define MPI_TYPE_NULL_DELETE_FN ((MPI_Delete_function *)MPC_Mpi_type_null_delete_fn)
#define MPI_TYPE_NULL_COPY_FN ((MPI_Copy_function *)MPC_Mpi_type_null_copy_fn)
#define MPI_TYPE_DUP_FN ((MPI_Copy_function *)MPC_Mpi_type_dup_fn)

#define MPI_COMM_NULL_DELETE_FN ((MPI_Delete_function *)MPC_Mpi_comm_null_delete_fn)
#define MPI_COMM_NULL_COPY_FN ((MPI_Copy_function *)MPC_Mpi_comm_null_copy_fn)
#define MPI_COMM_DUP_FN ((MPI_Copy_function *)MPC_Mpi_comm_dup_fn)

#define MPI_WIN_NULL_DELETE_FN ((MPI_Delete_function *)MPC_Mpi_win_null_delete_fn)
#define MPI_WIN_NULL_COPY_FN ((MPI_Copy_function *)MPC_Mpi_win_null_copy_fn)
#define MPI_WIN_DUP_FN ((MPI_Copy_function *)MPC_Mpi_win_dup_fn)

  extern const int MPI_TAG_UB;
  extern const int MPI_HOST;
  extern const int MPI_IO;
  extern const int MPI_WTIME_IS_GLOBAL;
  extern const int MPI_UNIVERSE_SIZE;
  extern const int MPI_LASTUSEDCODE;
  extern const int MPI_APPNUM;
  extern const MPI_Comm MPI_COMM_SELF;

	int MPC_Mpi_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state );
	int MPC_Mpi_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );
	int MPC_Mpi_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );

	/* type */
	int MPC_Mpi_type_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state );
	int MPC_Mpi_type_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );
	int MPC_Mpi_type_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );

	/* comm */
	int MPC_Mpi_comm_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state );
	int MPC_Mpi_comm_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );
	int MPC_Mpi_comm_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );

	/* win */
	int MPC_Mpi_win_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state );
	int MPC_Mpi_win_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );
	int MPC_Mpi_win_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag );
	
  int MPI_Send (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int MPI_Recv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
  int MPI_Get_count (MPI_Status *, MPI_Datatype, int *);
  int MPI_Bsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int MPI_Ssend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int MPI_Rsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int MPI_Buffer_attach (void *, int);
  int MPI_Buffer_detach (void *, int *);
  int MPI_Isend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		 MPI_Request *);
  int MPI_Ibsend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		  MPI_Request *);
  int MPI_Issend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		  MPI_Request *);
  int MPI_Irsend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		  MPI_Request *);
  int MPI_Irecv (void *, int, MPI_Datatype, int, int, MPI_Comm,
		 MPI_Request *);
  int MPI_Wait (MPI_Request *, MPI_Status *);
  int MPI_Test (MPI_Request *, int *, MPI_Status *);
  int MPI_Request_free (MPI_Request *);
  int MPI_Waitany (int, MPI_Request *, int *, MPI_Status *);
  int MPI_Testany (int, MPI_Request *, int *, int *, MPI_Status *);
  int MPI_Waitall (int, MPI_Request *, MPI_Status *);
  int MPI_Testall (int, MPI_Request *, int *, MPI_Status *);
  int MPI_Waitsome (int, MPI_Request *, int *, int *, MPI_Status *);
  int MPI_Testsome (int, MPI_Request *, int *, int *, MPI_Status *);
  int MPI_Iprobe (int, int, MPI_Comm, int *, MPI_Status *);
  int MPI_Probe (int, int, MPI_Comm, MPI_Status *);
  int MPI_Cancel (MPI_Request *);
  int MPI_Test_cancelled (MPI_Status *, int *);
  int MPI_Send_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		     MPI_Request *);
  int MPI_Bsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		      MPI_Request *);
  int MPI_Ssend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		      MPI_Request *);
  int MPI_Rsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		      MPI_Request *);
  int MPI_Recv_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		     MPI_Request *);
  int MPI_Start (MPI_Request *);
  int MPI_Startall (int, MPI_Request *);
  int MPI_Sendrecv (void *, int, MPI_Datatype, int, int, void *, int,
		    MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
  int MPI_Sendrecv_replace (void *, int, MPI_Datatype, int, int, int, int,
			    MPI_Comm, MPI_Status *);
  int MPI_Type_contiguous (int, MPI_Datatype, MPI_Datatype *);
  int MPI_Type_vector (int, int, int, MPI_Datatype, MPI_Datatype *);
  int MPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
  int MPI_Type_indexed (int, int *, int *, MPI_Datatype, MPI_Datatype *);
  int MPI_Type_hindexed (int, int *, MPI_Aint *, MPI_Datatype,
			 MPI_Datatype *);
  int MPI_Type_struct (int, int *, MPI_Aint *, MPI_Datatype *,
		       MPI_Datatype *);
  int MPI_Address (void *, MPI_Aint *);
  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
  int MPI_Type_extent (MPI_Datatype, MPI_Aint *);
  /* See the 1.1 version of the Standard.  The standard made an
     unfortunate choice here, however, it is the standard.  The size returned
     by MPI_Type_size is specified as an int, not an MPI_Aint */
  int MPI_Type_size (MPI_Datatype, int *);
  /* MPI_Type_count was withdrawn in MPI 1.1 */
  int MPI_Type_lb (MPI_Datatype, MPI_Aint *);
  int MPI_Type_ub (MPI_Datatype, MPI_Aint *);
  int MPI_Type_commit (MPI_Datatype *);
  int MPI_Type_free (MPI_Datatype *);
  int MPI_Get_elements (MPI_Status *, MPI_Datatype, int *);
  int MPI_Pack (void *, int, MPI_Datatype, void *, int, int *, MPI_Comm);
  int MPI_Unpack (void *, int, int *, void *, int, MPI_Datatype, MPI_Comm);
  int MPI_Pack_size (int, MPI_Datatype, MPI_Comm, int *);
  int MPI_Barrier (MPI_Comm);
  int MPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
  int MPI_Gather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
		  MPI_Comm);
  int MPI_Gatherv (void *, int, MPI_Datatype, void *, int *, int *,
		   MPI_Datatype, int, MPI_Comm);
  int MPI_Scatter (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
		   MPI_Comm);
  int MPI_Scatterv (void *, int *, int *, MPI_Datatype, void *, int,
		    MPI_Datatype, int, MPI_Comm);
  int MPI_Allgather (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
		     MPI_Comm);
  int MPI_Allgatherv (void *, int, MPI_Datatype, void *, int *, int *,
		      MPI_Datatype, MPI_Comm);
  int MPI_Alltoall (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
		    MPI_Comm);
  int MPI_Alltoallv (void *, int *, int *, MPI_Datatype, void *, int *, int *,
		     MPI_Datatype, MPI_Comm);
  int MPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
  int MPI_Op_create (MPI_User_function *, int, MPI_Op *);
  int MPI_Op_free (MPI_Op *);
  int MPI_Allreduce (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
  int MPI_Reduce_scatter (void *, void *, int *, MPI_Datatype, MPI_Op,
			  MPI_Comm);
  int MPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
  int MPI_Group_size (MPI_Group, int *);
  int MPI_Group_rank (MPI_Group, int *);
  int MPI_Group_translate_ranks (MPI_Group, int, int *, MPI_Group, int *);
  int MPI_Group_compare (MPI_Group, MPI_Group, int *);
  int MPI_Comm_group (MPI_Comm, MPI_Group *);
  int MPI_Group_union (MPI_Group, MPI_Group, MPI_Group *);
  int MPI_Group_intersection (MPI_Group, MPI_Group, MPI_Group *);
  int MPI_Group_difference (MPI_Group, MPI_Group, MPI_Group *);
  int MPI_Group_incl (MPI_Group, int, int *, MPI_Group *);
  int MPI_Group_excl (MPI_Group, int, int *, MPI_Group *);
  int MPI_Group_range_incl (MPI_Group, int, int[][3], MPI_Group *);
  int MPI_Group_range_excl (MPI_Group, int, int[][3], MPI_Group *);
  int MPI_Group_free (MPI_Group *);
  int MPI_Comm_size (MPI_Comm, int *);
  int MPI_Comm_rank (MPI_Comm, int *);
  int MPI_Comm_compare (MPI_Comm, MPI_Comm, int *);
  int MPI_Comm_dup (MPI_Comm, MPI_Comm *);
  int MPI_Comm_create (MPI_Comm, MPI_Group, MPI_Comm *);
  int MPI_Comm_split (MPI_Comm, int, int, MPI_Comm *);
  int MPI_Comm_free (MPI_Comm *);
  int MPI_Comm_test_inter (MPI_Comm, int *);
  int MPI_Comm_remote_size (MPI_Comm, int *);
  int MPI_Comm_remote_group (MPI_Comm, MPI_Group *);
  int MPI_Intercomm_create (MPI_Comm, int, MPI_Comm, int, int, MPI_Comm *);
  int MPI_Intercomm_merge (MPI_Comm, int, MPI_Comm *);
  int MPI_Keyval_create (MPI_Copy_function *, MPI_Delete_function *, int *,
			 void *);
  int MPI_Keyval_free (int *);
  int MPI_Attr_put (MPI_Comm, int, void *);
  int MPI_Attr_get (MPI_Comm, int, void *, int *);
  int MPI_Attr_delete (MPI_Comm, int);
  int MPI_Topo_test (MPI_Comm, int *);
  int MPI_Cart_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
  int MPI_Dims_create (int, int, int *);
  int MPI_Graph_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
  int MPI_Graphdims_get (MPI_Comm, int *, int *);
  int MPI_Graph_get (MPI_Comm, int, int, int *, int *);
  int MPI_Cartdim_get (MPI_Comm, int *);
  int MPI_Cart_get (MPI_Comm, int, int *, int *, int *);
  int MPI_Cart_rank (MPI_Comm, int *, int *);
  int MPI_Cart_coords (MPI_Comm, int, int, int *);
  int MPI_Graph_neighbors_count (MPI_Comm, int, int *);
  int MPI_Graph_neighbors (MPI_Comm, int, int, int *);
  int MPI_Cart_shift (MPI_Comm, int, int, int *, int *);
  int MPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
  int MPI_Cart_map (MPI_Comm, int, int *, int *, int *);
  int MPI_Graph_map (MPI_Comm, int, int *, int *, int *);
  int MPI_Get_processor_name (char *, int *);
  int MPI_Get_version (int *, int *);
  int MPI_Errhandler_create (MPI_Handler_function *, MPI_Errhandler *);
  int MPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
  int MPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
  int MPI_Errhandler_free (MPI_Errhandler *);
  int MPI_Error_string (int, char *, int *);
  int MPI_Error_class (int, int *);
  double MPI_Wtime (void);
  double MPI_Wtick (void);
  int MPI_Init (int *, char ***);
  int MPI_Finalize (void);
  int MPI_Initialized (int *);
  int MPI_Abort (MPI_Comm, int);

  /* Note that we may need to define a @PCONTROL_LIST@ depending on whether
     stdargs are supported */
  int MPI_Pcontrol (const int, ...);

  /* MPI-2 functions */
  int MPI_Comm_get_name (MPI_Comm, char *, int *);
  int MPI_Comm_set_name (MPI_Comm, char *);

/* Here are the bindings of the profiling routines */
#if !defined(MPI_BUILD_PROFILING)
  int PMPI_Send (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int PMPI_Recv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
  int PMPI_Get_count (MPI_Status *, MPI_Datatype, int *);
  int PMPI_Bsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int PMPI_Ssend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int PMPI_Rsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
  int PMPI_Buffer_attach (void *, int);
  int PMPI_Buffer_detach (void *, int *);
  int PMPI_Isend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		  MPI_Request *);
  int PMPI_Ibsend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		   MPI_Request *);
  int PMPI_Issend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		   MPI_Request *);
  int PMPI_Irsend (void *, int, MPI_Datatype, int, int, MPI_Comm,
		   MPI_Request *);
  int PMPI_Irecv (void *, int, MPI_Datatype, int, int, MPI_Comm,
		  MPI_Request *);
  int PMPI_Wait (MPI_Request *, MPI_Status *);
  int PMPI_Test (MPI_Request *, int *, MPI_Status *);
  int PMPI_Request_free (MPI_Request *);
  int PMPI_Waitany (int, MPI_Request *, int *, MPI_Status *);
  int PMPI_Testany (int, MPI_Request *, int *, int *, MPI_Status *);
  int PMPI_Waitall (int, MPI_Request *, MPI_Status *);
  int PMPI_Testall (int, MPI_Request *, int *, MPI_Status *);
  int PMPI_Waitsome (int, MPI_Request *, int *, int *, MPI_Status *);
  int PMPI_Testsome (int, MPI_Request *, int *, int *, MPI_Status *);
  int PMPI_Iprobe (int, int, MPI_Comm, int *, MPI_Status *);
  int PMPI_Probe (int, int, MPI_Comm, MPI_Status *);
  int PMPI_Cancel (MPI_Request *);
  int PMPI_Test_cancelled (MPI_Status *, int *);
  int PMPI_Send_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		      MPI_Request *);
  int PMPI_Bsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		       MPI_Request *);
  int PMPI_Ssend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		       MPI_Request *);
  int PMPI_Rsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		       MPI_Request *);
  int PMPI_Recv_init (void *, int, MPI_Datatype, int, int, MPI_Comm,
		      MPI_Request *);
  int PMPI_Start (MPI_Request *);
  int PMPI_Startall (int, MPI_Request *);
  int PMPI_Sendrecv (void *, int, MPI_Datatype, int, int, void *, int,
		     MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
  int PMPI_Sendrecv_replace (void *, int, MPI_Datatype, int, int, int, int,
			     MPI_Comm, MPI_Status *);
  int PMPI_Type_contiguous (int, MPI_Datatype, MPI_Datatype *);
  int PMPI_Type_vector (int, int, int, MPI_Datatype, MPI_Datatype *);
  int PMPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
  int PMPI_Type_indexed (int, int *, int *, MPI_Datatype, MPI_Datatype *);
  int PMPI_Type_hindexed (int, int *, MPI_Aint *, MPI_Datatype,
			  MPI_Datatype *);
  int PMPI_Type_struct (int, int *, MPI_Aint *, MPI_Datatype *,
			MPI_Datatype *);
  int PMPI_Address (void *, MPI_Aint *);
  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
  int PMPI_Type_extent (MPI_Datatype, MPI_Aint *);
  /* See the 1.1 version of the Standard.  The standard made an
     unfortunate choice here, however, it is the standard.  The size returned
     by MPI_Type_size is specified as an int, not an MPI_Aint */
  int PMPI_Type_size (MPI_Datatype, int *);
  /* MPI_Type_count was withdrawn in MPI 1.1 */
  int PMPI_Type_lb (MPI_Datatype, MPI_Aint *);
  int PMPI_Type_ub (MPI_Datatype, MPI_Aint *);
  int PMPI_Type_commit (MPI_Datatype *);
  int PMPI_Type_free (MPI_Datatype *);
  int PMPI_Get_elements (MPI_Status *, MPI_Datatype, int *);
  int PMPI_Pack (void *, int, MPI_Datatype, void *, int, int *, MPI_Comm);
  int PMPI_Unpack (void *, int, int *, void *, int, MPI_Datatype, MPI_Comm);
  int PMPI_Pack_size (int, MPI_Datatype, MPI_Comm, int *);
  int PMPI_Barrier (MPI_Comm);
  int PMPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
  int PMPI_Gather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
		   MPI_Comm);
  int PMPI_Gatherv (void *, int, MPI_Datatype, void *, int *, int *,
		    MPI_Datatype, int, MPI_Comm);
  int PMPI_Scatter (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
		    MPI_Comm);
  int PMPI_Scatterv (void *, int *, int *, MPI_Datatype, void *, int,
		     MPI_Datatype, int, MPI_Comm);
  int PMPI_Allgather (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
		      MPI_Comm);
  int PMPI_Allgatherv (void *, int, MPI_Datatype, void *, int *, int *,
		       MPI_Datatype, MPI_Comm);
  int PMPI_Alltoall (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
		     MPI_Comm);
  int PMPI_Alltoallv (void *, int *, int *, MPI_Datatype, void *, int *,
		      int *, MPI_Datatype, MPI_Comm);
  int PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
  int PMPI_Op_create (MPI_User_function *, int, MPI_Op *);
  int PMPI_Op_free (MPI_Op *);
  int PMPI_Allreduce (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
  int PMPI_Reduce_scatter (void *, void *, int *, MPI_Datatype, MPI_Op,
			   MPI_Comm);
  int PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
  int PMPI_Group_size (MPI_Group, int *);
  int PMPI_Group_rank (MPI_Group, int *);
  int PMPI_Group_translate_ranks (MPI_Group, int, int *, MPI_Group, int *);
  int PMPI_Group_compare (MPI_Group, MPI_Group, int *);
  int PMPI_Comm_group (MPI_Comm, MPI_Group *);
  int PMPI_Group_union (MPI_Group, MPI_Group, MPI_Group *);
  int PMPI_Group_intersection (MPI_Group, MPI_Group, MPI_Group *);
  int PMPI_Group_difference (MPI_Group, MPI_Group, MPI_Group *);
  int PMPI_Group_incl (MPI_Group, int, int *, MPI_Group *);
  int PMPI_Group_excl (MPI_Group, int, int *, MPI_Group *);
  int PMPI_Group_range_incl (MPI_Group, int, int[][3], MPI_Group *);
  int PMPI_Group_range_excl (MPI_Group, int, int[][3], MPI_Group *);
  int PMPI_Group_free (MPI_Group *);
  int PMPI_Comm_size (MPI_Comm, int *);
  int PMPI_Comm_rank (MPI_Comm, int *);
  int PMPI_Comm_compare (MPI_Comm, MPI_Comm, int *);
  int PMPI_Comm_dup (MPI_Comm, MPI_Comm *);
  int PMPI_Comm_create (MPI_Comm, MPI_Group, MPI_Comm *);
  int PMPI_Comm_split (MPI_Comm, int, int, MPI_Comm *);
  int PMPI_Comm_free (MPI_Comm *);
  int PMPI_Comm_test_inter (MPI_Comm, int *);
  int PMPI_Comm_remote_size (MPI_Comm, int *);
  int PMPI_Comm_remote_group (MPI_Comm, MPI_Group *);
  int PMPI_Intercomm_create (MPI_Comm, int, MPI_Comm, int, int, MPI_Comm *);
  int PMPI_Intercomm_merge (MPI_Comm, int, MPI_Comm *);
  int PMPI_Keyval_create (MPI_Copy_function *, MPI_Delete_function *, int *,
			  void *);
  int PMPI_Keyval_free (int *);
  int PMPI_Attr_put (MPI_Comm, int, void *);
  int PMPI_Attr_get (MPI_Comm, int, void *, int *);
  int PMPI_Attr_delete (MPI_Comm, int);
  int PMPI_Topo_test (MPI_Comm, int *);
  int PMPI_Cart_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
  int PMPI_Dims_create (int, int, int *);
  int PMPI_Graph_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
  int PMPI_Graphdims_get (MPI_Comm, int *, int *);
  int PMPI_Graph_get (MPI_Comm, int, int, int *, int *);
  int PMPI_Cartdim_get (MPI_Comm, int *);
  int PMPI_Cart_get (MPI_Comm, int, int *, int *, int *);
  int PMPI_Cart_rank (MPI_Comm, int *, int *);
  int PMPI_Cart_coords (MPI_Comm, int, int, int *);
  int PMPI_Graph_neighbors_count (MPI_Comm, int, int *);
  int PMPI_Graph_neighbors (MPI_Comm, int, int, int *);
  int PMPI_Cart_shift (MPI_Comm, int, int, int *, int *);
  int PMPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
  int PMPI_Cart_map (MPI_Comm, int, int *, int *, int *);
  int PMPI_Graph_map (MPI_Comm, int, int *, int *, int *);
  int PMPI_Get_processor_name (char *, int *);
  int PMPI_Get_version (int *, int *);
  int PMPI_Errhandler_create (MPI_Handler_function *, MPI_Errhandler *);
  int PMPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
  int PMPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
  int PMPI_Errhandler_free (MPI_Errhandler *);
  int PMPI_Error_string (int, char *, int *);
  int PMPI_Error_class (int, int *);
  double PMPI_Wtime (void);
  double PMPI_Wtick (void);
  int PMPI_Init (int *, char ***);
  int PMPI_Init_thread (int *, char ***, int, int *);
  int PMPI_Finalize (void);
  int PMPI_Initialized (int *);
  int PMPI_Abort (MPI_Comm, int);

  /* Note that we may need to define a @PCONTROL_LIST@ depending on whether
     stdargs are supported */
  int PMPI_Pcontrol (const int, ...);

  /* MPI-2 functions */
  int PMPI_Comm_get_name (MPI_Comm, char *, int *);
  int PMPI_Comm_set_name (MPI_Comm, char *);

#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define  MPI_THREAD_MULTIPLE 3
  int MPI_Init_thread (int *, char ***, int, int *);

#endif				/* MPI_BUILD_PROFILING */
/* End of MPI bindings */

  /* MPI-2 functions */
#if 0

  /* Process Creation and Management */
  int MPI_Close_port (char *);
  int MPI_Comm_accept (char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
  int MPI_Comm_connect (char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
  int MPI_Comm_disconnect (MPI_Comm *);
  int MPI_Comm_get_parent (MPI_Comm *);
  int MPI_Comm_join (int, MPI_Comm *);
  int MPI_Comm_spawn (char *, char *[], int, MPI_Info, int, MPI_Comm,
		      MPI_Comm *, int[]);
  int MPI_Comm_spawn_multiple (int, char *[], char **[], int[], MPI_Info[],
			       int, MPI_Comm, MPI_Comm *, int[]);
  int MPI_Lookup_name (char *, MPI_Info, char *);
  int MPI_Open_port (MPI_Info, char *);
  int MPI_Publish_name (char *, MPI_Info, char *);
  int MPI_Unpublish_name (char *, MPI_Info, char *);

  /* One-Sided Communications */
  int MPI_Accumulate (void *, int, MPI_Datatype, int, MPI_Aint, int,
		      MPI_Datatype, MPI_Op, MPI_Win);
  int MPI_Get (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
	       MPI_Win);
  int MPI_Put (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
	       MPI_Win);
  int MPI_Win_complete (MPI_Win);
  int MPI_Win_create (void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
  int MPI_Win_fence (int, MPI_Win);
  int MPI_Win_free (MPI_Win *);
  int MPI_Win_get_group (MPI_Win, MPI_Group *);
  int MPI_Win_lock (int, int, int, MPI_Win);
  int MPI_Win_post (MPI_Group, int, MPI_Win);
  int MPI_Win_start (MPI_Group, int, MPI_Win);
  int MPI_Win_test (MPI_Win, int *);
  int MPI_Win_unlock (int, MPI_Win);
  int MPI_Win_wait (MPI_Win);

  /* Extended Collective Operations */
  int MPI_Alltoallw (void *, int[], int[], MPI_Datatype[], void *, int[],
		     int[], MPI_Datatype[], MPI_Comm);
  int MPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

  /* External Interfaces */
  int MPI_Add_error_class (int *);
  int MPI_Add_error_code (int, int *);
  int MPI_Add_error_string (int, char *);
  int MPI_Comm_call_errhandler (MPI_Comm, int);
  int MPI_Comm_create_keyval (MPI_Comm_copy_attr_function *,
			      MPI_Comm_delete_attr_function *, int *, void *);
  int MPI_Comm_delete_attr (MPI_Comm, int);
  int MPI_Comm_free_keyval (int *);
  int MPI_Comm_get_attr (MPI_Comm, int, void *, int *);
  int MPI_Comm_set_attr (MPI_Comm, int, void *);
  int MPI_File_call_errhandler (MPI_File, int);
  int MPI_Grequest_complete (MPI_Request);
  int MPI_Grequest_start (MPI_Grequest_query_function *,
			  MPI_Grequest_free_function *,
			  MPI_Grequest_cancel_function *, void *,
			  MPI_Request *);
  int MPI_Is_thread_main (int *);
  int MPI_Query_thread (int *);
  int MPI_Status_set_cancelled (MPI_Status *, int);
  int MPI_Status_set_elements (MPI_Status *, MPI_Datatype, int);
  int MPI_Type_create_keyval (MPI_Type_copy_attr_function *,
			      MPI_Type_delete_attr_function *, int *, void *);
  int MPI_Type_delete_attr (MPI_Datatype, int);
  int MPI_Type_dup (MPI_Datatype, MPI_Datatype *);
  int MPI_Type_free_keyval (int *);
  int MPI_Type_get_attr (MPI_Datatype, int, void *, int *);
  int MPI_Type_get_contents (MPI_Datatype, int, int, int, int[], MPI_Aint[],
			     MPI_Datatype[]);
  int MPI_Type_get_envelope (MPI_Datatype, int *, int *, int *, int *);
  int MPI_Type_get_name (MPI_Datatype, char *, int *);
  int MPI_Type_set_attr (MPI_Datatype, int, void *);
  int MPI_Type_set_name (MPI_Datatype, char *);
  int MPI_Type_match_size (int, int, MPI_Datatype *);
  int MPI_Win_call_errhandler (MPI_Win, int);
  int MPI_Win_create_keyval (MPI_Win_copy_attr_function *,
			     MPI_Win_delete_attr_function *, int *, void *);
  int MPI_Win_delete_attr (MPI_Win, int);
  int MPI_Win_free_keyval (int *);
  int MPI_Win_get_attr (MPI_Win, int, void *, int *);
  int MPI_Win_get_name (MPI_Win, char *, int *);
  int MPI_Win_set_attr (MPI_Win, int, void *);
  int MPI_Win_set_name (MPI_Win, char *);

  int MPI_Alloc_mem (MPI_Aint, MPI_Info info, void *baseptr);
  int MPI_Comm_create_errhandler (MPI_Comm_errhandler_fn *, MPI_Errhandler *);
  int MPI_Comm_get_errhandler (MPI_Comm, MPI_Errhandler *);
  int MPI_Comm_set_errhandler (MPI_Comm, MPI_Errhandler);
  int MPI_File_create_errhandler (MPI_File_errhandler_fn *, MPI_Errhandler *);
  int MPI_File_get_errhandler (MPI_File, MPI_Errhandler *);
  int MPI_File_set_errhandler (MPI_File, MPI_Errhandler);
  int MPI_Finalized (int *);
  int MPI_Free_mem (void *);
  int MPI_Get_address (void *, MPI_Aint *);
  int MPI_Info_create (MPI_Info *);
  int MPI_Info_delete (MPI_Info, char *);
  int MPI_Info_dup (MPI_Info, MPI_Info *);
  int MPI_Info_free (MPI_Info * info);
  int MPI_Info_get (MPI_Info, char *, int, char *, int *);
  int MPI_Info_get_nkeys (MPI_Info, int *);
  int MPI_Info_get_nthkey (MPI_Info, int, char *);
  int MPI_Info_get_valuelen (MPI_Info, char *, int *, int *);
  int MPI_Info_set (MPI_Info, char *, char *);
  int MPI_Pack_external (char *, void *, int, MPI_Datatype, void *, MPI_Aint,
			 MPI_Aint *);
  int MPI_Pack_external_size (char *, int, MPI_Datatype, MPI_Aint *);
  int MPI_Request_get_status (MPI_Request, int *, MPI_Status *);
  int MPI_Status_c2f (MPI_Status *, MPI_Fint *);
  int MPI_Status_f2c (MPI_Fint *, MPI_Status *);
  int MPI_Type_create_darray (int, int, int, int[], int[], int[], int[], int,
			      MPI_Datatype, MPI_Datatype *);
  int MPI_Type_create_hindexed (int, int[], MPI_Aint[], MPI_Datatype,
				MPI_Datatype *);
  int MPI_Type_create_hvector (int, int, MPI_Aint, MPI_Datatype,
			       MPI_Datatype *);
  int MPI_Type_create_indexed_block (int, int, int[], MPI_Datatype,
				     MPI_Datatype *);
  int MPI_Type_create_resized (MPI_Datatype, MPI_Aint, MPI_Aint,
			       MPI_Datatype *);
  int MPI_Type_create_struct (int, int[], MPI_Aint[], MPI_Datatype[],
			      MPI_Datatype *);
  int MPI_Type_create_subarray (int, int[], int[], int[], int, MPI_Datatype,
				MPI_Datatype *);
  int MPI_Type_get_extent (MPI_Datatype, MPI_Aint *, MPI_Aint *);
  int MPI_Type_get_true_extent (MPI_Datatype, MPI_Aint *, MPI_Aint *);
  int MPI_Unpack_external (char *, void *, MPI_Aint, MPI_Aint *, void *, int,
			   MPI_Datatype);
  int MPI_Win_create_errhandler (MPI_Win_errhandler_fn *, MPI_Errhandler *);
  int MPI_Win_get_errhandler (MPI_Win, MPI_Errhandler *);
  int MPI_Win_set_errhandler (MPI_Win, MPI_Errhandler);
#endif

#ifdef __cplusplus
}
#endif
#endif
