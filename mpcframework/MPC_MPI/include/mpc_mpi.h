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
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
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

#define MPI_VERSION    1
#define MPI_SUBVERSION    3

#define MPI_CART (-2)
#define MPI_GRAPH (-3)

/* Results of the compare operations. */
#define MPI_IDENT     0
#define MPI_CONGRUENT 1
#define MPI_SIMILAR   2
#define MPI_UNEQUAL   3

/************************************************************************/
/* MPI_* to MPC_* converters                                            */
/************************************************************************/

/* Misc */
#define MPI_MAX_PROCESSOR_NAME MPC_MAX_PROCESSOR_NAME
#define MPI_MAX_NAME_STRING MPC_MAX_OBJECT_NAME
#define MPI_MAX_OBJECT_NAME MPC_MAX_OBJECT_NAME

/* Communication Parameters */
#define MPI_SOURCE MPC_SOURCE
#define MPI_TAG MPC_TAG
#define MPI_UNDEFINED MPC_UNDEFINED
#define MPI_REQUEST_NULL ((MPI_Request)-1)
#define MPI_COMM_WORLD MPC_COMM_WORLD
#define MPI_STATUS_IGNORE MPC_STATUS_IGNORE
#define MPI_STATUSES_IGNORE MPC_STATUSES_IGNORE
#define MPI_ANY_TAG MPC_ANY_TAG
#define MPI_ANY_SOURCE MPC_ANY_SOURCE
#define MPI_PROC_NULL MPC_PROC_NULL
#define MPI_COMM_NULL MPC_COMM_NULL
#define MPI_ROOT MPC_ROOT
#define MPI_IN_PLACE MPC_IN_PLACE
#define MPI_BOTTOM MPC_BOTTOM
#define MPI_COMM_SELF MPC_COMM_SELF

/* Error Handling */
#define MPI_SUCCESS MPC_SUCCESS
#define MPI_ERROR MPC_ERROR
#define MPI_MAX_ERROR_STRING MPC_MAX_ERROR_STRING
#define MPI_ERR_BUFFER MPC_ERR_BUFFER
#define MPI_ERR_COUNT MPC_ERR_COUNT
#define MPI_ERR_TYPE MPC_ERR_TYPE
#define MPI_ERR_TAG MPC_ERR_TAG
#define MPI_ERR_COMM MPC_ERR_COMM
#define MPI_ERR_RANK MPC_ERR_RANK
#define MPI_ERR_REQUEST MPC_ERR_REQUEST
#define MPI_ERR_ROOT MPC_ERR_ROOT
#define MPI_ERR_GROUP MPC_ERR_GROUP
#define MPI_ERR_OP MPC_ERR_OP
#define MPI_ERR_TOPOLOGY MPC_ERR_TOPOLOGY
#define MPI_ERR_DIMS MPC_ERR_DIMS
#define MPI_ERR_ARG MPC_ERR_ARG
#define MPI_ERR_UNKNOWN MPC_ERR_UNKNOWN
#define MPI_ERR_TRUNCATE MPC_ERR_TRUNCATE
#define MPI_ERR_OTHER MPC_ERR_OTHER
#define MPI_ERR_INTERN MPC_ERR_INTERN
#define MPI_ERR_IN_STATUS MPC_ERR_IN_STATUS
#define MPI_ERR_PENDING MPC_ERR_PENDING
#define MPI_ERR_KEYVAL MPC_ERR_KEYVAL
#define MPI_ERR_NO_MEM MPC_ERR_NO_MEM
#define MPI_ERR_BASE MPC_ERR_BASE
#define MPI_ERR_INFO_KEY MPC_ERR_INFO_KEY
#define MPI_ERR_INFO_VALUE MPC_ERR_INFO_VALUE
#define MPI_ERR_INFO_NOKEY MPC_ERR_INFO_NOKEY
#define MPI_ERR_SPAWN MPC_ERR_SPAWN
#define MPI_ERR_PORT MPC_ERR_PORT
#define MPI_ERR_SERVICE MPC_ERR_SERVICE
#define MPI_ERR_NAME MPC_ERR_NAME
#define MPI_ERR_WIN MPC_ERR_WIN
#define MPI_ERR_SIZE MPC_ERR_SIZE
#define MPI_ERR_DISP MPC_ERR_DISP
#define MPI_ERR_INFO MPC_ERR_INFO
#define MPI_ERR_LOCKTYPE MPC_ERR_LOCKTYPE
#define MPI_ERR_ASSERT MPC_ERR_ASSERT
#define MPI_ERR_RMA_CONFLICT MPC_ERR_RMA_CONFLICT
#define MPI_ERR_RMA_SYNC MPC_ERR_RMA_SYNC
#define MPI_ERR_FILE MPC_ERR_FILE
#define MPI_ERR_NOT_SAME MPC_ERR_NOT_SAME
#define MPI_ERR_AMODE MPC_ERR_AMODE
#define MPI_ERR_UNSUPPORTED_DATAREP MPC_ERR_UNSUPPORTED_DATAREP
#define MPI_ERR_UNSUPPORTED_OPERATION MPC_ERR_UNSUPPORTED_OPERATION
#define MPI_ERR_NO_SUCH_FILE MPC_ERR_NO_SUCH_FILE
#define MPI_ERR_FILE_EXISTS MPC_ERR_FILE_EXISTS
#define MPI_ERR_BAD_FILE MPC_ERR_BAD_FILE
#define MPI_ERR_ACCESS MPC_ERR_ACCESS
#define MPI_ERR_NO_SPACE MPC_ERR_NO_SPACE
#define MPI_ERR_QUOTA MPC_ERR_QUOTA
#define MPI_ERR_READ_ONLY MPC_ERR_READ_ONLY
#define MPI_ERR_FILE_IN_USE MPC_ERR_FILE_IN_USE
#define MPI_ERR_DUP_DATAREP MPC_ERR_DUP_DATAREP
#define MPI_ERR_CONVERSION MPC_ERR_CONVERSION
#define MPI_ERR_IO MPC_ERR_IO
#define MPI_ERR_LASTCODE MPC_ERR_LASTCODE
#define MPI_NOT_IMPLEMENTED MPC_NOT_IMPLEMENTED

/* Data-type Handling */
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

/* Basic data-types */
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
#define MPI_UNSIGNED_LONG_LONG_INT MPC_UNSIGNED_LONG_LONG_INT
#define MPI_UNSIGNED_LONG_LONG MPC_UNSIGNED_LONG_LONG
#define MPI_PACKED MPC_PACKED
#define MPI_FLOAT_INT MPC_FLOAT_INT
#define MPI_DOUBLE_INT MPC_DOUBLE_INT
#define MPI_LONG_DOUBLE_INT MPC_LONG_DOUBLE_INT
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
#define MPI_SIGNED_CHAR MPC_SIGNED_CHAR
#define MPI_LONG_DOUBLE_INT MPC_LONG_DOUBLE_INT
#define MPI_REAL MPC_FLOAT
#define MPI_UINT64_T MPC_UINT64_T

/************************************************************************/
/* MPI_* Defines                                                        */
/************************************************************************/

/* Threading Level */
#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define  MPI_THREAD_MULTIPLE 3

/* Basic Ops */
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

/* Group Handling */
#define MPI_GROUP_EMPTY ((MPI_Group)0)
#define MPI_GROUP_NULL ((MPI_Group)-1)

/* MPI_Info definitions */
/* Matches the one of MPI_INFO_NULL @ mpc_mpi.h:207 */
#define MPI_INFO_NULL (-1)
/* Maximum length for keys and values
* they are both defined for MPC and MPI variants */
/*1 MB */
#define MPI_MAX_INFO_VAL 1048576
#define MPI_MAX_INFO_KEY 255

/* Other Null Handles */
#define MPI_FILE_NULL ((MPI_File)-1)
#define MPI_WIN_NULL ((MPI_Win)-1)

#define MPI_BSEND_OVERHEAD (2*sizeof(mpi_buffer_overhead_t))

#define MPI_ERRHANDLER_NULL ((MPI_Errhandler)0)
#define MPI_ERRORS_RETURN 1
#define MPI_ERRORS_ARE_FATAL 2

#define MPI_KEYVAL_INVALID -1

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

#define MPI_MAX_KEY_DEFINED 7
#define MPI_TAG_UB 0
#define MPI_HOST 1
#define MPI_IO 2
#define MPI_WTIME_IS_GLOBAL 3
#define MPI_UNIVERSE_SIZE MPI_KEYVAL_INVALID
#define MPI_LASTUSEDCODE MPI_KEYVAL_INVALID
#define MPI_APPNUM MPI_KEYVAL_INVALID

/************************************************************************/
/*  Type Definitions                                                    */
/************************************************************************/

typedef MPC_Datatype MPI_Datatype; /* unsigned int */
typedef MPC_Comm MPI_Comm; /* unsigned int */
typedef int MPI_Request;
typedef MPC_Aint MPI_Aint; /* long */
typedef int MPI_Errhandler;
typedef MPC_User_function MPI_User_function;
typedef int MPI_Op;
typedef int MPI_Group;
typedef MPC_Status MPI_Status;
typedef MPC_Handler_function MPI_Handler_function;
typedef int MPI_Fint;
typedef int MPI_File;

/* MPI_Info Definitions */
typedef MPC_Info MPI_Info;

/* Copy Functions */
typedef int (MPI_Copy_function) (MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Delete_function) (MPI_Comm, int, void *, void *);

/* Generalized requests functions */
typedef int MPI_Grequest_query_function( void * extra_state, MPI_Status *status );
typedef int MPI_Grequest_cancel_function( void * extra_state, int complete );
typedef int MPI_Grequest_free_function( void * extra_state );

/* Extended Generalized Requests Functions */
typedef int MPIX_Grequest_poll_fn(void * extra_arg, MPI_Status *status);

/* Extended Generalized Request Class */
typedef int MPIX_Request_class;
typedef int MPIX_Grequest_wait_fn(int count, void **array_of_states, double, MPI_Status *status);

/* NOT IMPLEMENTED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
typedef int MPI_Win;
typedef long MPI_Count;
typedef long MPI_Offset;
/* C functions */
//~ typedef void (MPC_Handler_function) ( MPI_Comm *, int *, ... );
typedef int (MPI_Comm_copy_attr_function)(MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm, int, void *, void *);
typedef int (MPI_Type_copy_attr_function)(MPI_Datatype, int, void *, void *, void *, int *);
typedef int (MPI_Type_delete_attr_function)(MPI_Datatype, int, void *, void *);
typedef int (MPI_Win_copy_attr_function)(MPI_Win, int, void *, void *, void *,int *);
typedef int (MPI_Win_delete_attr_function)(MPI_Win, int, void *, void *);
/* added in MPI-2.2 */
typedef void (MPI_Comm_errhandler_function)(MPI_Comm *, int *, ...);
//~ typedef void (MPI_File_errhandler_function)(MPI_File *, int *, ...);
typedef void (MPI_Win_errhandler_function)(MPI_Win *, int *, ...);
/* names that were added in MPI-2.0 and deprecated in MPI-2.2 */
typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
//~ typedef MPI_File_errhandler_function MPI_File_errhandler_fn;
typedef MPI_Win_errhandler_function MPI_Win_errhandler_fn;
/* END OF NOT IMPLEMENTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

typedef struct
{
	int size;
	MPI_Request request;
} mpi_buffer_overhead_t;

/************************************************************************/
/*  MPI Interface                                                       */
/*                                                                      */  
/* /!\ Be careful to update the PMPI interface if                       */
/*      you are adding or updating something here                       */
/*      you also have to define the weak symbol                         */
/*      bindings in mpi_weak.h                                          */
/************************************************************************/



/* Init and Finalize */
int MPI_Init_thread (int *, char ***, int, int *);  
int MPI_Init (int *, char ***);
int MPI_Finalize (void);
int MPI_Finalized (int *);
int MPI_Initialized (int *);

/* Error Handling */
void MPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void MPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void MPI_Return_error (MPI_Comm * comm, int *error, ...);
int MPI_Errhandler_create (MPI_Handler_function *, MPI_Errhandler *);
int MPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
int MPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
int MPI_Errhandler_free (MPI_Errhandler *);
int MPI_Error_string (int, char *, int *);
int MPI_Error_class (int, int *);
int MPI_Abort (MPI_Comm, int);

/* NOT IMPLEMENTED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
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
/* END OF NOT IMPLEMENTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

/* Point to Point communications */
int MPI_Send (void *, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Recv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int MPI_Get_count (MPI_Status *, MPI_Datatype, int *);
int MPI_Bsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Ssend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Rsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Buffer_attach (void *, int);
int MPI_Buffer_detach (void *, int *);
int MPI_Isend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Ibsend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Issend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Irsend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Irecv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
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
int MPI_Send_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int MPI_Bsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int MPI_Ssend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int MPI_Rsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Recv_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int MPI_Start (MPI_Request *);
int MPI_Startall (int, MPI_Request *);
int MPI_Sendrecv (void *, int, MPI_Datatype, int, int, void *, int,  MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int MPI_Sendrecv_replace (void *, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);
  
/* Status Modification and query */
int MPI_Status_set_elements(MPI_Status *, MPI_Datatype , int );
int MPI_Status_set_elements_x(MPI_Status *, MPI_Datatype , MPI_Count );
int MPI_Status_set_cancelled (MPI_Status *, int);
int MPI_Request_get_status (MPI_Request, int *, MPI_Status *);
int MPI_Test_cancelled (MPI_Status *, int *);

/* Datatype Management & Packs */
int MPI_Type_contiguous (int, MPI_Datatype, MPI_Datatype *);
int MPI_Type_vector (int, int, int, MPI_Datatype, MPI_Datatype *);
int MPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
int MPI_Type_create_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
int MPI_Type_indexed (int, int *, int *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_hindexed (int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_create_hindexed (int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_struct (int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
int MPI_Type_create_struct (int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
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
int MPI_Type_set_name( MPI_Datatype datatype, char *name );
int MPI_Type_get_name( MPI_Datatype datatype, char *name, int * resultlen );

/* Collective Operations */
int MPI_Barrier (MPI_Comm);
int MPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
int MPI_Gather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int MPI_Gatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, int, MPI_Comm);
int MPI_Scatter (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int MPI_Scatterv (void *, int *, int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int MPI_Allgather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int MPI_Allgatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int MPI_Alltoall (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int MPI_Alltoallv (void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int MPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int MPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
int MPI_Allreduce (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int MPI_Reduce_scatter (void *, void *, int *, MPI_Datatype, MPI_Op, MPI_Comm);
int MPI_Reduce_scatter_block(void *, void *, int , MPI_Datatype , MPI_Op , MPI_Comm );
int MPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

/* Operations */
int MPI_Op_create (MPI_User_function *, int, MPI_Op *);
int MPI_Op_free (MPI_Op *);

/* Group Management */
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
  
/* Communicators and Intercommunicators */
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
  
/* Keyval and Attr */
int MPI_Keyval_create (MPI_Copy_function *, MPI_Delete_function *, int *, void *);
int MPI_Keyval_free (int *);
int MPI_Attr_put (MPI_Comm, int, void *);
int MPI_Attr_get (MPI_Comm, int, void *, int *);
int MPI_Attr_delete (MPI_Comm, int);
  
/* Topology Management */
int MPI_Topo_test (MPI_Comm, int *);

int MPI_Dims_create (int, int, int *);

int MPI_Graph_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
int MPI_Graphdims_get (MPI_Comm, int *, int *);
int MPI_Graph_get (MPI_Comm, int, int, int *, int *);
int MPI_Graph_neighbors_count (MPI_Comm, int, int *);
int MPI_Graph_neighbors (MPI_Comm, int, int, int *);
int MPI_Graph_map (MPI_Comm, int, int *, int *, int *);  

int MPI_Cart_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
int MPI_Cart_get (MPI_Comm, int, int *, int *, int *);
int MPI_Cart_rank (MPI_Comm, int *, int *);
int MPI_Cart_coords (MPI_Comm, int, int, int *);
int MPI_Cart_shift (MPI_Comm, int, int, int *, int *);
int MPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
int MPI_Cart_map (MPI_Comm, int, int *, int *, int *);
int MPI_Cartdim_get (MPI_Comm, int *);

/* Getters */
int MPI_Get_processor_name (char *, int *);
int MPI_Get_version (int *, int *);
double MPI_Wtime (void);
double MPI_Wtick (void);
int MPI_Query_thread (int *);

/* Profiling */
/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
stdargs are supported */
int MPI_Pcontrol (const int, ...);

/* Communicator Naming (MPI-2 functions) */
int MPI_Comm_get_name (MPI_Comm, char *, int *);
int MPI_Comm_set_name (MPI_Comm, char *);

/* Fortran Converters */
MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
MPI_Fint MPI_Comm_c2f(MPI_Comm comm);
MPI_Datatype MPI_Type_f2c(MPI_Fint datatype);
MPI_Fint MPI_Type_c2f(MPI_Datatype datatype);
MPI_Group MPI_Group_f2c(MPI_Fint group);
MPI_Fint MPI_Group_c2f(MPI_Group group);
MPI_Request MPI_Request_f2c(MPI_Fint request);
MPI_Fint MPI_Request_c2f(MPI_Request request);
MPI_File MPI_File_f2c(MPI_Fint file);
MPI_Fint MPI_File_c2f(MPI_File file);
MPI_Win MPI_Win_f2c(MPI_Fint win);
MPI_Fint MPI_Win_c2f(MPI_Win win);
MPI_Op MPI_Op_f2c(MPI_Fint op);
MPI_Fint MPI_Op_c2f(MPI_Op op);
MPI_Info MPI_Info_f2c(MPI_Fint info);
MPI_Fint MPI_Info_c2f(MPI_Info info);
MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler);
  
/* Neighbors collectives */
int MPI_Neighbor_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoallv(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoallw(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm);

/* MPI Info Management */
int MPI_Info_set( MPI_Info, const char *, const char * );
int MPI_Info_free( MPI_Info * );
int MPI_Info_create( MPI_Info * );
int MPI_Info_delete( MPI_Info , const char * );
int MPI_Info_get(MPI_Info , const char *, int , char *, int *);
int MPI_Info_dup( MPI_Info , MPI_Info * );
int MPI_Info_get_nkeys (MPI_Info, int *);
int MPI_Info_get_nthkey (MPI_Info, int, char *);
int MPI_Info_get_valuelen (MPI_Info, char *, int *, int *);

/* Generalized Requests */
int MPI_Grequest_start( MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn,
			void *extra_state,
			MPI_Request * request);
int MPI_Grequest_complete(  MPI_Request request); 

/* Extended Generalized Requests */
int MPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn, 
			MPIX_Grequest_poll_fn * poll_fn, 
			void *extra_state, 
			MPI_Request * request);

/* Extended Generalized Request Class */
int MPIX_GRequest_class_create( MPI_Grequest_query_function * query_fn,
				MPI_Grequest_cancel_function * cancel_fn,
				MPI_Grequest_free_function * free_fn,
				MPIX_Grequest_poll_fn * poll_fn,
				MPIX_Grequest_wait_fn * wait_fn,
				MPIX_Request_class * new_class );
int MPIX_Grequest_class_allocate( MPIX_Request_class target_class, void *extra_state, MPI_Request *request );





/************************************************************************/
/*  PMPI Profiling Interface                                            */
/************************************************************************/
#if !defined(MPI_BUILD_PROFILING)
/* Init and Finalize */
int PMPI_Init_thread (int *, char ***, int, int *);  
int PMPI_Init (int *, char ***);
int PMPI_Finalize (void);
int PMPI_Finalized (int *);
int PMPI_Initialized (int *);

/* Error Handling */
void PMPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void PMPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void PMPI_Return_error (MPI_Comm * comm, int *error, ...);
int PMPI_Errhandler_create (MPI_Handler_function *, MPI_Errhandler *);
int PMPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
int PMPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
int PMPI_Errhandler_free (MPI_Errhandler *);
int PMPI_Error_string (int, char *, int *);
int PMPI_Error_class (int, int *);
int PMPI_Abort (MPI_Comm, int);

/* Point to Point communications */
int PMPI_Send (void *, int, MPI_Datatype, int, int, MPI_Comm);
int PMPI_Recv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int PMPI_Get_count (MPI_Status *, MPI_Datatype, int *);
int PMPI_Bsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int PMPI_Ssend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int PMPI_Rsend (void *, int, MPI_Datatype, int, int, MPI_Comm);
int PMPI_Buffer_attach (void *, int);
int PMPI_Buffer_detach (void *, int *);
int PMPI_Isend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int PMPI_Ibsend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int PMPI_Issend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int PMPI_Irsend (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int PMPI_Irecv (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
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
int PMPI_Test_cancelled (MPI_Status *, int *);
int PMPI_Send_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int PMPI_Bsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int PMPI_Ssend_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int PMPI_Rsend_init (void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int PMPI_Recv_init (void *, int, MPI_Datatype, int, int, MPI_Comm,  MPI_Request *);
int PMPI_Start (MPI_Request *);
int PMPI_Startall (int, MPI_Request *);
int PMPI_Sendrecv (void *, int, MPI_Datatype, int, int, void *, int,  MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int PMPI_Sendrecv_replace (void *, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);

/* Status Modification and query */
int PMPI_Status_set_elements(MPI_Status *, MPI_Datatype , int );
int PMPI_Status_set_elements_x(MPI_Status *, MPI_Datatype , MPI_Count );
int PMPI_Status_set_cancelled (MPI_Status *, int);
int PMPI_Request_get_status (MPI_Request, int *, MPI_Status *);
int PMPI_Test_cancelled (MPI_Status *, int *);

/* Datatype Management & Packs */
int PMPI_Type_contiguous (int, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_vector (int, int, int, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_create_hvector (int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_indexed (int, int *, int *, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_hindexed (int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_create_hindexed (int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
int PMPI_Type_struct (int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
int PMPI_Type_create_struct (int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
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
int PMPI_Type_set_name( MPI_Datatype datatype, char *name );
int PMPI_Type_get_name( MPI_Datatype datatype, char *name, int * resultlen );

/* Collective Operations */
int PMPI_Barrier (MPI_Comm);
int PMPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
int PMPI_Gather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int PMPI_Gatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, int, MPI_Comm);
int PMPI_Scatter (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int PMPI_Scatterv (void *, int *, int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int PMPI_Allgather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int PMPI_Allgatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int PMPI_Alltoall (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int PMPI_Alltoallv (void *, int *, int *, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int PMPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
int PMPI_Allreduce (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int PMPI_Reduce_scatter (void *, void *, int *, MPI_Datatype, MPI_Op, MPI_Comm);
int PMPI_Reduce_scatter_block(void *, void *, int , MPI_Datatype , MPI_Op , MPI_Comm );
int PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

/* Operations */
int PMPI_Op_create (MPI_User_function *, int, MPI_Op *);
int PMPI_Op_free (MPI_Op *);

/* Group Management */
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
  
/* Communicators and Intercommunicators */
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
  
/* Keyval and Attr */
int PMPI_Keyval_create (MPI_Copy_function *, MPI_Delete_function *, int *, void *);
int PMPI_Keyval_free (int *);
int PMPI_Attr_put (MPI_Comm, int, void *);
int PMPI_Attr_get (MPI_Comm, int, void *, int *);
int PMPI_Attr_delete (MPI_Comm, int);
  
/* Topology Management */
int PMPI_Topo_test (MPI_Comm, int *);

int PMPI_Dims_create (int, int, int *);

int PMPI_Graph_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
int PMPI_Graphdims_get (MPI_Comm, int *, int *);
int PMPI_Graph_get (MPI_Comm, int, int, int *, int *);
int PMPI_Graph_neighbors_count (MPI_Comm, int, int *);
int PMPI_Graph_neighbors (MPI_Comm, int, int, int *);
int PMPI_Graph_map (MPI_Comm, int, int *, int *, int *);  

int PMPI_Cart_create (MPI_Comm, int, int *, int *, int, MPI_Comm *);
int PMPI_Cart_get (MPI_Comm, int, int *, int *, int *);
int PMPI_Cart_rank (MPI_Comm, int *, int *);
int PMPI_Cart_coords (MPI_Comm, int, int, int *);
int PMPI_Cart_shift (MPI_Comm, int, int, int *, int *);
int PMPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
int PMPI_Cart_map (MPI_Comm, int, int *, int *, int *);
int PMPI_Cartdim_get (MPI_Comm, int *);

/* Getters */
int PMPI_Get_processor_name (char *, int *);
int PMPI_Get_version (int *, int *);
double PMPI_Wtime (void);
double PMPI_Wtick (void);
int PMPI_Query_thread (int *);

/* Profiling */
/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
stdargs are supported */
int PMPI_Pcontrol (const int, ...);

/* Communicator Naming (MPI-2 functions) */
int PMPI_Comm_get_name (MPI_Comm, char *, int *);
int PMPI_Comm_set_name (MPI_Comm, char *);

/* Fortran Converters */
MPI_Comm PMPI_Comm_f2c(MPI_Fint comm);
MPI_Fint PMPI_Comm_c2f(MPI_Comm comm);
MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype);
MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype);
MPI_Group PMPI_Group_f2c(MPI_Fint group);
MPI_Fint PMPI_Group_c2f(MPI_Group group);
MPI_Request PMPI_Request_f2c(MPI_Fint request);
MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_File PMPI_File_f2c(MPI_Fint file);
MPI_Fint PMPI_File_c2f(MPI_File file);
MPI_Win PMPI_Win_f2c(MPI_Fint win);
MPI_Fint PMPI_Win_c2f(MPI_Win win);
MPI_Op PMPI_Op_f2c(MPI_Fint op);
MPI_Fint PMPI_Op_c2f(MPI_Op op);
MPI_Info PMPI_Info_f2c(MPI_Fint info);
MPI_Fint PMPI_Info_c2f(MPI_Info info);
MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler);
  
/* Neighbors collectives */
int PMPI_Neighbor_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoallv(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoallw(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm);

/* MPI Info Management */
int PMPI_Info_set( MPI_Info, const char *, const char * );
int PMPI_Info_free( MPI_Info * );
int PMPI_Info_create( MPI_Info * );
int PMPI_Info_delete( MPI_Info , const char * );
int PMPI_Info_get(MPI_Info , const char *, int , char *, int *);
int PMPI_Info_dup( MPI_Info , MPI_Info * );
int PMPI_Info_get_nkeys (MPI_Info, int *);
int PMPI_Info_get_nthkey (MPI_Info, int, char *);
int PMPI_Info_get_valuelen (MPI_Info, char *, int *, int *);

/* Generalized Requests */
int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn,
			void *extra_state,
			MPI_Request * request);
int PMPI_Grequest_complete(  MPI_Request request); 

/* Extended Generalized Requests */
int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn, 
			MPIX_Grequest_poll_fn * poll_fn, 
			void *extra_state, 
			MPI_Request * request);

/* Extended Generalized Request Class */
int PMPIX_GRequest_class_create( MPI_Grequest_query_function * query_fn,
				MPI_Grequest_cancel_function * cancel_fn,
				MPI_Grequest_free_function * free_fn,
				MPIX_Grequest_poll_fn * poll_fn,
				MPIX_Grequest_wait_fn * wait_fn,
				MPIX_Request_class * new_class );
int PMPIX_Grequest_class_allocate( MPIX_Request_class target_class, void *extra_state, MPI_Request *request );


#endif /* MPI_BUILD_PROFILING */

#if 0
/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/

/* One-Sided Communications */
int MPI_Alloc_mem (MPI_Aint, MPI_Info , void *);
int MPI_Free_mem (void *);
int MPI_Win_set_attr(MPI_Win , int , void *);
int MPI_Win_get_attr(MPI_Win , int , void *, int *);
int MPI_Win_free_keyval(int *);
int MPI_Win_delete_attr(MPI_Win , int );
int MPI_Win_create_keyval(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *);
int MPI_Win_create (void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
int MPI_Win_free (MPI_Win *);
int MPI_Win_wait (MPI_Win);
int MPI_Accumulate (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win);
int MPI_Get (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
int MPI_Put (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
int MPI_Win_complete (MPI_Win);
int MPI_Win_fence (int, MPI_Win);
int MPI_Win_get_group (MPI_Win, MPI_Group *);
int MPI_Win_lock (int, int, int, MPI_Win);
int MPI_Win_post (MPI_Group, int, MPI_Win);
int MPI_Win_start (MPI_Group, int, MPI_Win);
int MPI_Win_test (MPI_Win, int *);
int MPI_Win_unlock (int, MPI_Win);


/* Datatype Handling & Packs */
int MPI_Type_dup(MPI_Datatype , MPI_Datatype *);
int MPI_Type_get_name(MPI_Datatype , char *, int *);
int MPI_Type_set_name(MPI_Datatype , const char *);
int MPI_Type_create_resized (MPI_Datatype, MPI_Aint , MPI_Aint , MPI_Datatype *);
int MPI_Type_get_true_extent(MPI_Datatype , MPI_Aint *, MPI_Aint *);
int MPI_Type_get_extent(MPI_Datatype , MPI_Aint *, MPI_Aint *);
int MPI_Type_free_keyval (int *);
int MPI_Type_create_keyval (MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *);
int MPI_Type_delete_attr (MPI_Datatype, int);
int MPI_Type_set_attr (MPI_Datatype, int, void *);
int MPI_Type_get_attr (MPI_Datatype, int, void *, int *);
int MPI_Type_create_indexed_block (int, int, int *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_get_envelope (MPI_Datatype, int *, int *, int *, int *);
int MPI_Type_get_contents (MPI_Datatype, int, int, int, int *, MPI_Aint *, MPI_Datatype *);
int MPI_Type_create_darray (int, int, int, int[], int[], int[], int[], int, MPI_Datatype, MPI_Datatype *);
int MPI_Get_address (void *, MPI_Aint *);
int MPI_Type_size_x(MPI_Datatype , MPI_Count *);
int MPI_Type_get_extent_x(MPI_Datatype , MPI_Count *, MPI_Count *);
int MPI_Type_get_true_extent_x(MPI_Datatype , MPI_Count *, MPI_Count *);
int MPI_Get_elements_x(const MPI_Status *, MPI_Datatype , MPI_Count *);
int MPI_Type_create_hindexed_block(int , int , const MPI_Aint *, MPI_Datatype , MPI_Datatype * );
int MPI_Type_match_size (int, int, MPI_Datatype *);
int MPI_Type_create_subarray (int, int[], int[], int[], int, MPI_Datatype, MPI_Datatype *);
int MPI_Pack_external_size (char *, int, MPI_Datatype, MPI_Aint *);
int MPI_Pack_external (char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *);
int MPI_Unpack_external (char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype);

/* Communicator Management */
int MPI_Comm_set_errhandler(MPI_Comm , MPI_Errhandler );
int MPI_Comm_free_keyval (int *);
int MPI_Comm_delete_attr (MPI_Comm, int);
int MPI_Comm_call_errhandler (MPI_Comm, int);
int MPI_Comm_create_keyval (MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *);
int MPI_Comm_delete_attr (MPI_Comm, int);
int MPI_Comm_get_attr (MPI_Comm, int, void *, int *);
int MPI_Comm_set_attr (MPI_Comm, int, void *);
int MPI_Comm_dup_with_info(MPI_Comm , MPI_Info , MPI_Comm * );
int MPI_Comm_split_type(MPI_Comm , int , int , MPI_Info , MPI_Comm * );
int MPI_Comm_set_info(MPI_Comm , MPI_Info );
int MPI_Comm_get_info(MPI_Comm , MPI_Info * );
int MPI_Comm_call_errhandler (MPI_Comm, int);
int MPI_Comm_create_errhandler (MPI_Comm_errhandler_fn *, MPI_Errhandler *);

/* Process Creation and Management */
int MPI_Close_port (char *);
int MPI_Comm_accept (char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
int MPI_Comm_connect (char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
int MPI_Comm_disconnect (MPI_Comm *);
int MPI_Comm_get_parent (MPI_Comm *);
int MPI_Comm_join (int, MPI_Comm *);
int MPI_Comm_spawn (char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int[]);
int MPI_Comm_spawn_multiple (int, char *[], char **[], int[], MPI_Info[], int, MPI_Comm, MPI_Comm *, int[]);
int MPI_Lookup_name (char *, MPI_Info, char *);
int MPI_Open_port (MPI_Info, char *);
int MPI_Publish_name (char *, MPI_Info, char *);
int MPI_Unpublish_name (char *, MPI_Info, char *);
  
/* Error Management */
int MPI_Add_error_class (int *);
int MPI_Add_error_code (int, int *);
int MPI_Add_error_string (int, char *);

/* Getters */
int MPI_Is_thread_main (int *);
int MPI_Query_thread (int *);
int MPI_Get_library_version(char *, int *);
  
/* Collectives */
int MPI_Reduce_scatter_block(void *, void *, int , MPI_Datatype , MPI_Op , MPI_Comm );

/* Extended Collective Operations */
int MPI_Alltoallw (void *, int[], int[], MPI_Datatype[], void *, int[], int[], MPI_Datatype[], MPI_Comm);
  
#endif /* Not Implemented */





#ifdef __cplusplus
}
#endif
#endif /* __SCTK_MPC_MPI_H_ */
