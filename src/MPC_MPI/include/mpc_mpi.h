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
#ifndef MPC_MPI_H_
#define MPC_MPI_H_

#include <mpc_mpi_comm_lib.h>

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief This Macro Informs the MPI Code that it is running in MPC
 */
#define MPC_MESSAGE_PASSING_INTERFACE 1

#define MPI_VERSION    3
#define MPI_SUBVERSION    1

#define MPI_CART (-2)
#define MPI_GRAPH (-3)
#define MPI_DIST_GRAPH (-4)

/* Results of the compare operations. */
#define MPI_IDENT     0
#define MPI_CONGRUENT 1
#define MPI_SIMILAR   2
#define MPI_UNEQUAL   3

#define MPI_LOCK_EXCLUSIVE  234
#define MPI_LOCK_SHARED     235

/* Other constants */
#define MPI_UNWEIGHTED ((void *) 2)
#define MPI_WEIGHTS_EMPTY ((void *) 3)

/************************************************************************/
/* MPI_* to MPC_* converters                                            */
/************************************************************************/

/* Misc */
#define MPI_MAX_PROCESSOR_NAME MPC_MAX_PROCESSOR_NAME
#define MPI_MAX_NAME_STRING MPC_MAX_OBJECT_NAME
#define MPI_MAX_OBJECT_NAME MPC_MAX_OBJECT_NAME
#define MPI_MAX_LIBRARY_VERSION_STRING MPC_MAX_LIBRARY_VERSION_STRING

/* Status Struct Members */
#define MPI_SOURCE MPC_SOURCE
#define MPI_TAG MPC_TAG
#define MPI_ERROR MPC_ERROR

/* Communication Parameters */
#define MPI_UNDEFINED MPC_UNDEFINED
#define MPI_REQUEST_NULL ((MPI_Request)-1)
#define MPI_COMM_WORLD MPC_COMM_WORLD
#define MPI_STATUS_IGNORE SCTK_STATUS_NULL
#define MPI_STATUSES_IGNORE SCTK_STATUS_NULL
#define MPI_ANY_TAG MPC_ANY_TAG
#define MPI_ANY_SOURCE MPC_ANY_SOURCE
#define MPI_PROC_NULL MPC_PROC_NULL
#define MPI_COMM_NULL MPC_COMM_NULL
#define MPI_ROOT MPC_ROOT
#define MPI_IN_PLACE MPC_IN_PLACE
#define MPI_BOTTOM ((void*)0)
#define MPI_COMM_SELF MPC_COMM_SELF

/* Error Handling */
#define MPI_SUCCESS SCTK_SUCCESS

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
#define MPI_ERR_RMA_FLAVOR MPC_ERR_RMA_FLAVOR
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

#define MPI_ERR_RMA_RANGE MPC_ERR_RMA_RANGE
#define MPI_ERR_RMA_ATTACH MPC_ERR_RMA_ATTACH
#define MPI_ERR_RMA_SHARED MPC_ERR_RMA_SHARED

#define MPI_ERR_LASTCODE MPC_ERR_LASTCODE
#define MPI_NOT_IMPLEMENTED MPC_NOT_IMPLEMENTED
#define MPIR_ERRORS_THROW_EXCEPTIONS MPCR_ERRORS_THROW_EXCEPTIONS
/* MPI_T Errors */
#define MPI_T_ERR_MEMORY MPC_T_ERR_MEMORY
#define MPI_T_ERR_NOT_INITIALIZED MPC_T_ERR_NOT_INITIALIZED
#define MPI_T_ERR_CANNOT_INIT MPC_T_ERR_CANNOT_INIT
#define MPI_T_ERR_INVALID_INDEX MPC_T_ERR_INVALID_INDEX
#define MPI_T_ERR_INVALID_ITEM MPC_T_ERR_INVALID_ITEM
#define MPI_T_ERR_INVALID_HANDLE MPC_T_ERR_INVALID_HANDLE
#define MPI_T_ERR_OUT_OF_HANDLES MPC_T_ERR_OUT_OF_HANDLES
#define MPI_T_ERR_OUT_OF_SESSIONS MPC_T_ERR_OUT_OF_SESSIONS
#define MPI_T_ERR_INVALID_SESSION MPC_T_ERR_INVALID_SESSION
#define MPI_T_ERR_CVAR_SET_NOT_NOW MPC_T_ERR_CVAR_SET_NOT_NOW
#define MPI_T_ERR_CVAR_SET_NEVER MPC_T_ERR_CVAR_SET_NEVER
#define MPI_T_ERR_PVAR_NO_STARTSTOP MPC_T_ERR_PVAR_NO_STARTSTOP
#define MPI_T_ERR_PVAR_NO_WRITE MPC_T_ERR_PVAR_NO_WRITE
#define MPI_T_ERR_PVAR_NO_ATOMIC MPC_T_ERR_PVAR_NO_ATOMIC
#define MPI_T_ERR_INVALID_NAME MPC_T_ERR_INVALID_NAME
#define MPI_T_ERR_INVALID MPC_T_ERR_INVALID

/* Data-type Handling */
#define MPI_DATATYPE_NULL MPC_DATATYPE_NULL
#define MPI_UB MPC_UB
#define MPI_LB MPC_LB
#define MPI_CHAR MPC_CHAR
#define MPI_BYTE MPC_BYTE
#define MPI_SHORT MPC_SHORT
#define MPI_INT MPC_INT

/* Support for MPI_INTEGER */
#define MPI_INTEGER MPC_INTEGER
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
#define MPI_REAL MPC_REAL
#define MPI_INT8_T MPC_INT8_T
#define MPI_UINT8_T MPC_UINT8_T
#define MPI_INT16_T MPC_INT16_T
#define MPI_UINT16_T MPC_UINT16_T
#define MPI_INT32_T MPC_INT32_T
#define MPI_UINT32_T MPC_UINT32_T
#define MPI_INT64_T MPC_INT64_T
#define MPI_UINT64_T MPC_UINT64_T
#define MPI_COMPLEX8 MPC_COMPLEX8
#define MPI_COMPLEX16 MPC_COMPLEX16
#define MPI_COMPLEX32 MPC_COMPLEX32
#define MPI_WCHAR MPC_WCHAR
#define MPI_INTEGER16 MPC_INTEGER16
#define MPI_AINT MPC_AINT
#define MPI_OFFSET MPC_OFFSET
#define MPI_COUNT MPC_COUNT
#define MPI_C_BOOL MPC_C_BOOL
#define MPI_C_COMPLEX MPC_C_COMPLEX
#define MPI_C_FLOAT_COMPLEX MPC_C_FLOAT_COMPLEX
#define MPI_C_DOUBLE_COMPLEX MPC_C_DOUBLE_COMPLEX
#define MPI_C_LONG_DOUBLE_COMPLEX MPC_C_LONG_DOUBLE_COMPLEX
#define MPI_CHARACTER MPC_CHARACTER
#define MPI_DOUBLE_PRECISION MPC_DOUBLE_PRECISION

#define MPI_2INTEGER MPC_2INTEGER
#define MPI_2REAL MPC_2REAL
/* Datatype decoders */

#define MPI_COMBINER_UNKNOWN MPC_COMBINER_UNKNOWN
#define MPI_COMBINER_NAMED MPC_COMBINER_NAMED
#define MPI_COMBINER_DUP MPC_COMBINER_DUP
#define MPI_COMBINER_CONTIGUOUS MPC_COMBINER_CONTIGUOUS
#define MPI_COMBINER_VECTOR MPC_COMBINER_VECTOR
#define MPI_COMBINER_HVECTOR MPC_COMBINER_HVECTOR
#define MPI_COMBINER_INDEXED MPC_COMBINER_INDEXED
#define MPI_COMBINER_HINDEXED MPC_COMBINER_HINDEXED
#define MPI_COMBINER_INDEXED_BLOCK MPC_COMBINER_INDEXED_BLOCK
#define MPI_COMBINER_HINDEXED_BLOCK MPC_COMBINER_HINDEXED_BLOCK
#define MPI_COMBINER_STRUCT MPC_COMBINER_STRUCT
#define MPI_COMBINER_SUBARRAY MPC_COMBINER_SUBARRAY
#define MPI_COMBINER_DARRAY MPC_COMBINER_DARRAY
#define MPI_COMBINER_F90_REAL MPC_COMBINER_F90_REAL
#define MPI_COMBINER_F90_COMPLEX MPC_COMBINER_F90_COMPLEX
#define MPI_COMBINER_F90_INTEGER MPC_COMBINER_F90_INTEGER
#define MPI_COMBINER_RESIZED MPC_COMBINER_RESIZED

/* Predefined MPI datatypes corresponding to both C and Fortran datatypes */

#define MPI_CXX_BOOL MPC_CXX_BOOL
#define MPI_CXX_FLOAT_COMPLEX MPC_CXX_FLOAT_COMPLEX
#define MPI_CXX_DOUBLE_COMPLEX MPC_CXX_DOUBLE_COMPLEX
#define MPI_CXX_LONG_DOUBLE_COMPLEX MPC_CXX_LONG_DOUBLE_COMPLEX

/* These are deprecated MPI 1.0 constants in MPI 3.0
 * however they are never returned by get envelope but as ROMIO uses them */

#define MPI_COMBINER_HINDEXED_INTEGER MPC_COMBINER_HINDEXED_INTEGER
#define MPI_COMBINER_STRUCT_INTEGER MPC_COMBINER_STRUCT_INTEGER
#define MPI_COMBINER_HVECTOR_INTEGER MPC_COMBINER_HVECTOR_INTEGER


/************************************************************************/
/* MPI_* Defines                                                        */
/************************************************************************/

/* Data-type classes */
#define MPI_TYPECLASS_INTEGER 1
#define MPI_TYPECLASS_REAL 2
#define MPI_TYPECLASS_COMPLEX 3

/* Threading Level */
#define MPI_THREAD_SINGLE MPC_THREAD_SINGLE
#define MPI_THREAD_FUNNELED MPC_THREAD_FUNNELED
#define MPI_THREAD_SERIALIZED MPC_THREAD_SERIALIZED
#define  MPI_THREAD_MULTIPLE MPC_THREAD_MULTIPLE

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
#define MPI_REPLACE 13
#define MPI_NO_OP 14
#define MAX_MPI_DEFINED_OP 13
#define MPI_OP_NULL ((MPI_Op)-1)


/* Group Handling */
#define MPI_GROUP_EMPTY ((MPI_Group)0)
#define MPI_GROUP_NULL ((MPI_Group)-1)

/* MPI_Info definitions */
/* Matches the one of MPI_INFO_NULL @ mpc_mpi.h:207 */
#define MPI_INFO_NULL (-1)
#define MPI_INFO_ENV (0)
/* Maximum length for keys and values
* they are both defined for MPC and MPI variants */
/*1 MB */
#define MPI_MAX_INFO_VAL 1024
#define MPI_MAX_INFO_KEY 255

/* Other Null Handles */
#define MPI_WIN_NULL ((MPI_Win)-1)
#define MPI_MESSAGE_NULL ((MPI_Message) MPI_REQUEST_NULL)
#define MPI_MESSAGE_NO_PROC -2

#ifdef ROMIO_COMP
#define MPI_FILE_NULL ((void *)0)
#endif

#define MPI_BSEND_OVERHEAD (2*sizeof(mpi_buffer_overhead_t))

#define MPI_ERRHANDLER_NULL MPC_ERRHANDLER_NULL
#define MPI_ERRORS_RETURN MPC_ERRORS_RETURN       /* 1234 in Fortran */
#define MPI_ERRORS_ARE_FATAL MPC_ERRORS_ARE_FATAL /* 1235 in Fortran */

#define MPI_KEYVAL_INVALID MPC_KEYVAL_INVALID

#define MPI_TAG_UB 0
#define MPI_HOST 1
#define MPI_IO 2
#define MPI_WTIME_IS_GLOBAL 3
#define MPI_APPNUM 4
#define MPI_UNIVERSE_SIZE 5
#define MPI_LASTUSEDCODE 6
#define MPI_MAX_KEY_DEFINED 7

/* In addition, there are 5 predefined window attributes that are
 *    defined for every window */
#define MPI_WIN_MODEL 0
#define MPI_WIN_BASE          1
#define MPI_WIN_SIZE          2
#define MPI_WIN_DISP_UNIT     3
#define MPI_WIN_CREATE_FLAVOR 4

/* Ordering defines */
#define MPI_DISTRIBUTE_DFLT_DARG 100
#define MPI_DISTRIBUTE_BLOCK 101
#define MPI_DISTRIBUTE_CYCLIC 102
#define MPI_DISTRIBUTE_NONE 1

#define MPI_ORDER_C 200
#define MPI_ORDER_FORTRAN 201

/* Halo */
#define MPI_HALO_NULL (-1)

/* for comm_split method */
#define MPI_COMM_TYPE_SHARED MPC_COMM_TYPE_SHARED
#define MPI_COMM_TYPE_SOCKET MPC_COMM_TYPE_SOCKET
#define MPI_COMM_TYPE_NUMA MPC_COMM_TYPE_NUMA
#define MPI_COMM_TYPE_UNIX_PROCESS MPC_COMM_TYPE_UNIX_PROCESS
#define MPI_COMM_TYPE_APP MPC_COMM_TYPE_APP
#define MPI_COMM_TYPE_HW_UNGUIDED MPC_COMM_TYPE_HW_UNGUIDED
#define MPI_COMM_TYPE_HW_SUBDOMAIN MPC_COMM_TYPE_HW_SUBDOMAIN
#define MPI_COMM_TYPE_NODE MPC_COMM_TYPE_NODE

/*
* * MPI-2 One-Sided Communications asserts
*/

/* asserts for one-sided communication */
#define MPI_MODE_NOCHECK      1
#define MPI_MODE_NOSTORE      2
#define MPI_MODE_NOPUT 4
#define MPI_MODE_NOPRECEDE 8
#define MPI_MODE_NOSUCCEED 16

#define MPI_WIN_FLAVOR_CREATE        1
#define MPI_WIN_FLAVOR_ALLOCATE      2
#define MPI_WIN_FLAVOR_DYNAMIC       3
#define MPI_WIN_FLAVOR_SHARED        4

#define MPI_WIN_UNIFIED              0
#define MPI_WIN_SEPARATE             1

/* Others definitions */
#define MPI_MAX_PORT_NAME      256
#define MPI_ARGV_NULL (char **)0
#define MPI_ARGVS_NULL (char ***)0
#define MPI_ERRCODES_IGNORE (int *)0

/* Error not implemented */
#define MPIX_ERR_PROC_FAILED          101 /* Process failure */
#define MPIX_ERR_PROC_FAILED_PENDING  102 /* A failure has caused this request
                                           * to be pending */
#define MPIX_ERR_REVOKED              103 /* The communciation object has been revoked */
#define MPICH_ERR_LAST_MPIX           103


/************************************************************************/
/*  Type Definitions                                                    */
/************************************************************************/

typedef mpc_lowcomm_datatype_t MPI_Datatype; /* unsigned int */
typedef mpc_lowcomm_communicator_t MPI_Comm; /* pointer */
typedef int MPI_Request;
typedef ssize_t MPI_Aint;
typedef ssize_t MPI_Count;
typedef int MPI_Errhandler;
typedef sctk_Op_User_function MPI_User_function;
typedef int MPI_Op;
typedef int MPI_Group;
typedef mpc_lowcomm_status_t MPI_Status;
typedef MPC_Handler_function MPI_Handler_function;
typedef int MPI_Fint;

/* MPI type combiner */
typedef MPC_Type_combiner MPI_Type_combiner;

/* MPI_Info Definitions */
#define HAVE_MPI_INFO
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
typedef int MPIX_Grequest_class;
typedef int MPIX_Grequest_wait_fn(int count, void **array_of_states, double, MPI_Status *status);

/* Halo */
typedef int MPI_Halo;
typedef int MPI_Halo_exchange;

/* Checkpoint */
typedef mpc_lowcomm_checkpoint_state_t MPIX_Checkpoint_state;

/* NOT IMPLEMENTED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
typedef mpc_lowcomm_rdma_window_t MPI_Win;


typedef MPC_Message MPI_Message;

/* added in MPI-2.2 */
typedef void (MPI_File_errhandler_function)(void *, int *, ...);
/* names that were added in MPI-2.0 and deprecated in MPI-2.2 */
typedef MPI_File_errhandler_function MPI_File_errhandler_fn;
 
/* C functions */
//~ typedef void (MPC_Handler_function) ( MPI_Comm *, int *, ... );
typedef int (MPI_Comm_copy_attr_function)(MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm, int, void *, void *);

typedef MPC_Type_copy_attr_function MPI_Type_copy_attr_function;
typedef MPC_Type_delete_attr_function MPI_Type_delete_attr_function;

typedef int (MPI_Win_copy_attr_function)(MPI_Win, int, void *, void *, void *,int *);
typedef int (MPI_Win_delete_attr_function)(MPI_Win, int, void *, void *);
/* added in MPI-2.2 */
typedef void (MPI_Comm_errhandler_function)(MPI_Comm *, int *, ...);
typedef void (MPI_Win_errhandler_function)(MPI_Win *, int *, ...);

/* names that were added in MPI-2.0 and deprecated in MPI-2.2 */
typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
typedef MPI_Win_errhandler_function MPI_Win_errhandler_fn;
/* END OF NOT IMPLEMENTED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

typedef struct
{
	int size;
	MPI_Request request;
} mpi_buffer_overhead_t;


/* MPI File Support */
#define HAVE_MPI_OFFSET
typedef MPI_Count MPI_Offset;

#define MPI_FILE_DEFINED

typedef struct ADIOI_FileD * MPI_File; 

#define MPIIMPL_HAVE_MPI_COMBINER_SUBARRAY
#define MPIIMPL_HAVE_MPI_COMBINER_DUP
#define MPIIMPL_HAVE_MPI_COMBINER_DARRAY

/* Data-size conversion layer */
typedef int (MPI_Datarep_conversion_function)(void *, MPI_Datatype, int, void *, MPI_Offset, void *);
typedef int (MPI_Datarep_extent_function)(MPI_Datatype datatype, MPI_Aint *, void *);


/** MPI_T Verbosity levels */

/** MPI_T Enum */

#define MPI_T_ENUM_NULL ((MPI_T_enum)NULL)

typedef enum {

  MPC_T_VERBOSITY_NONE = 0,

  MPI_T_VERBOSITY_USER_BASIC,
  MPI_T_VERBOSITY_USER_DETAIL,
  MPI_T_VERBOSITY_USER_ALL,

  MPI_T_VERBOSITY_TUNER_BASIC,
  MPI_T_VERBOSITY_TUNER_DETAIL,
  MPI_T_VERBOSITY_TUNER_ALL,

  MPI_T_VERBOSITY_MPIDEV_BASIC,
  MPI_T_VERBOSITY_MPIDEV_DETAIL,
  MPI_T_VERBOSITY_MPIDEV_ALL

} MPC_T_verbosity;

/** MPI_T Binding to object */

typedef enum {

  MPI_T_BIND_NO_OBJECT = 0,
  MPI_T_BIND_MPI_COMM,
  MPI_T_BIND_MPI_DATATYPE,
  MPI_T_BIND_MPI_ERRHANDLER,
  MPI_T_BIND_MPI_FILE,
  MPI_T_BIND_MPI_GROUP,
  MPI_T_BIND_MPI_OP,
  MPI_T_BIND_MPI_REQUEST,
  MPI_T_BIND_MPI_WIN,
  MPI_T_BIND_MPI_MESSAGE,
  MPI_T_BIND_MPI_INFO
} MPC_T_binding;


/** Performance variables (PVAR) */

typedef enum {
  MPC_T_PVAR_CLASS_NONE,          /**<< Internal value to catch uninitialized */
  MPI_T_PVAR_CLASS_STATE,         /**<< A set of discrete states */
  MPI_T_PVAR_CLASS_LEVEL,         /**<< Utilization of a ressource */
  MPI_T_PVAR_CLASS_SIZE,          /**<< The size of a ressource */
  MPI_T_PVAR_CLASS_PERCENTAGE,    /**<< The percentage of a ressource use */
  MPI_T_PVAR_CLASS_HIGHWATERMARK, /**<< High use of a ressource */
  MPI_T_PVAR_CLASS_LOWWATERMARK,  /**<< Low use of a ressource */
  MPI_T_PVAR_CLASS_COUNTER,       /**<< Number of occurences of an event */
  MPI_T_PVAR_CLASS_AGGREGATE,     /**<< Aggregated value of ressource */
  MPI_T_PVAR_CLASS_TIMER,  /**<< Aggregate time spent executing something */
  MPI_T_PVAR_CLASS_GENERIC /**<< A generic class */
} MPC_T_pvar_class;

/** MPI Storage (just a pointer as no Fortran) */
typedef void *MPI_T_enum;


/** This defines the scope of a cvar */
typedef enum {
  MPI_T_SCOPE_CONSTANT, /*<< Readonly value does not change */
  MPI_T_SCOPE_READONLY, /*<< Readonly can change but cannot be changed */
  MPI_T_SCOPE_LOCAL,    /*<< Can be writable as a local operation */
  MPI_T_SCOPE_GROUP,    /*<< Can be writable in a group of processes */
  MPI_T_SCOPE_GROUP_EQ, /*<< Can be writable in a group of processes requiring
                           the same value */
  MPI_T_SCOPE_ALL,      /*<< Can be writable in all connected processes */
  MPI_T_SCOPE_ALL_EQ,   /*<< Can be writable in all connected processes with the
                           same value */
} MPC_T_cvar_scope;


/** Forward declaration of the VAR container */

/** CVAR Handles */

#define MPI_T_CVAR_HANDLE_NULL ((MPI_T_cvar_handle)-1)

typedef int MPI_T_cvar_handle;

/* Forward declaration of the session container */

#define MPI_T_PVAR_SESSION_NULL (-1)

typedef int MPI_T_pvar_session;

/** PVAR Handle allocation */

#define MPI_T_PVAR_ALL_HANDLES ((MPI_T_pvar_handle)-2)
#define MPI_T_PVAR_HANDLE_NULL ((MPI_T_pvar_handle)-1)

typedef int MPI_T_pvar_handle;

/* ######################################
   #  NULL delete handlers              #
   ######################################*/


/*****************/
/*MPI_COMM_DUP_FN*/
/*****************/



/**
 * @brief MPI function MPI_COMM_DUP_FN
 * 
 * @param oldcomm 
 * @param comm_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_COMM_DUP_FN(MPI_Comm oldcomm, int comm_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);
int PMPI_COMM_DUP_FN(MPI_Comm oldcomm, int comm_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/***********************/
/*MPI_COMM_NULL_COPY_FN*/
/***********************/



/**
 * @brief MPI function MPI_COMM_NULL_COPY_FN
 * 
 * @param oldcomm 
 * @param comm_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise
 */
int MPI_COMM_NULL_COPY_FN(MPI_Comm oldcomm, int comm_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);
int PMPI_COMM_NULL_COPY_FN(MPI_Comm oldcomm, int comm_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/*************************/
/*MPI_COMM_NULL_DELETE_FN*/
/*************************/



/**
 * @brief MPI function MPI_COMM_NULL_DELETE_FN
 * 
 * @param comm 
 * @param comm_keyval 
 * @param attribute_val 
 * @param extra_state 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_COMM_NULL_DELETE_FN(MPI_Comm comm, int comm_keyval, void *attribute_val, void *extra_state);
int PMPI_COMM_NULL_DELETE_FN(MPI_Comm comm, int comm_keyval, void *attribute_val, void *extra_state);


/************************/
/*MPI_CONVERSION_FN_NULL*/
/************************/



/**
 * @brief MPI function MPI_CONVERSION_FN_NULL
 * 
 * @param userbuf 
 * @param datatype 
 * @param count 
 * @param filebuf 
 * @param position 
 * @param extra_state 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_CONVERSION_FN_NULL(void *userbuf, MPI_Datatype datatype, int count, void *filebuf, MPI_Offset position, void *extra_state);
int PMPI_CONVERSION_FN_NULL(void *userbuf, MPI_Datatype datatype, int count, void *filebuf, MPI_Offset position, void *extra_state);


/************/
/*MPI_DUP_FN*/
/************/

/**
 * @brief MPI function MPI_DUP_FN
 * 
 * @param oldcomm 
 * @param keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_DUP_FN(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/******************/
/*MPI_NULL_COPY_FN*/
/******************/



/**
 * @brief MPI function MPI_NULL_COPY_FN
 * 
 * @param oldcomm 
 * @param keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_NULL_COPY_FN(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/********************/
/*MPI_NULL_DELETE_FN*/
/********************/



/**
 * @brief MPI function MPI_NULL_DELETE_FN
 * 
 * @param comm 
 * @param keyval 
 * @param attribute_val 
 * @param extra_state 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_NULL_DELETE_FN(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state);


/*****************/
/*MPI_TYPE_DUP_FN*/
/*****************/



/**
 * @brief MPI function MPI_TYPE_DUP_FN
 * 
 * @param oldtype 
 * @param type_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_TYPE_DUP_FN(MPI_Datatype oldtype, int type_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/***********************/
/*MPI_TYPE_NULL_COPY_FN*/
/***********************/



/**
 * @brief MPI function MPI_TYPE_NULL_COPY_FN
 * 
 * @param oldtype 
 * @param type_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_TYPE_NULL_COPY_FN(MPI_Datatype oldtype, int type_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/*************************/
/*MPI_TYPE_NULL_DELETE_FN*/
/*************************/



/**
 * @brief MPI function MPI_TYPE_NULL_DELETE_FN
 * 
 * @param datatype 
 * @param type_keyval 
 * @param attribute_val 
 * @param extra_state 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_TYPE_NULL_DELETE_FN(MPI_Datatype datatype, int type_keyval, void *attribute_val, void *extra_state);


/****************/
/*MPI_WIN_DUP_FN*/
/****************/



/**
 * @brief MPI function MPI_WIN_DUP_FN
 * 
 * @param oldwin 
 * @param win_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_WIN_DUP_FN(MPI_Win oldwin, int win_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/**********************/
/*MPI_WIN_NULL_COPY_FN*/
/**********************/

/**
 * @brief MPI function MPI_WIN_NULL_COPY_FN
 * 
 * @param oldwin 
 * @param win_keyval 
 * @param extra_state 
 * @param attribute_val_in 
 * @param attribute_val_out 
 * @param flag 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_WIN_NULL_COPY_FN(MPI_Win oldwin, int win_keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag);


/************************/
/*MPI_WIN_NULL_DELETE_FN*/
/************************/

/**
 * @brief MPI function MPI_WIN_NULL_DELETE_FN
 * 
 * @param win 
 * @param win_keyval 
 * @param attribute_val 
 * @param extra_state 
 * @param ierror 

 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_WIN_NULL_DELETE_FN(MPI_Win win, int win_keyval, void *attribute_val, void *extra_state);

/* ######################################
   #  MPI Interface                     #
   ######################################*/

void PMPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void PMPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void PMPI_Return_error (MPI_Comm * comm, int *error, ...);

void MPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void MPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file, int line);
void MPI_Return_error (MPI_Comm * comm, int *error, ...);





/*MPI_Abort*/

/**
 * @brief MPI function MPI_Abort
 * 
 * @param comm communicator of tasks to abort
 * @param errorcode error code to return to invoking environment
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Abort(MPI_Comm comm, int errorcode);
int PMPI_Abort(MPI_Comm comm, int errorcode);


/*MPI_Accumulate*/

/**
 * @brief MPI function MPI_Accumulate
 * 
 * @param origin_addr initial address of buffer
 * @param origin_count number of entries in buffer
 * @param origin_datatype datatype of each entry
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param op reduce operation
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);


/*MPI_Add_error_class*/

/**
 * @brief MPI function MPI_Add_error_class
 * 
 * @param errorclass value for the new error class
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Add_error_class(int *errorclass);
int PMPI_Add_error_class(int *errorclass);


/*MPI_Add_error_code*/

/**
 * @brief MPI function MPI_Add_error_code
 * 
 * @param errorclass 
 * @param errorcode new error code to be associated with errorclass
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Add_error_code(int errorclass, int *errorcode);
int PMPI_Add_error_code(int errorclass, int *errorcode);


/*MPI_Add_error_string*/

/**
 * @brief MPI function MPI_Add_error_string
 * 
 * @param errorcode error code or class
 * @param string text corresponding to errorcode
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Add_error_string(int errorcode, const char *string);
int PMPI_Add_error_string(int errorcode, const char *string);


/*MPI_Aint_add*/

/**
 * @brief MPI function MPI_Aint_add
 * 
 * @param base base address
 * @param disp displacement
 *
 * @return MPI_Aint 
 
 */
MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp);
MPI_Aint PMPI_Aint_add(MPI_Aint base, MPI_Aint disp);


/*MPI_Aint_diff*/

/**
 * @brief MPI function MPI_Aint_diff
 * 
 * @param addr1 minuend address
 * @param addr2 subtrahend address
 *
 * @return MPI_Aint 
 
 */
MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2);
MPI_Aint PMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2);


/*MPI_Allgather*/

/**
 * @brief MPI function MPI_Allgather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Allgather_init*/

/**
 * @brief MPI function MPI_Allgather_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Allgatherv*/

/**
 * @brief MPI function MPI_Allgatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Allgatherv_init*/

/**
 * @brief MPI function MPI_Allgatherv_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Alloc_mem*/

/**
 * @brief MPI function MPI_Alloc_mem
 * 
 * @param size size of memory segment in bytes
 * @param info 
 * @param baseptr pointer to beginning of memory segment allocated
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);
int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);


/*MPI_Allreduce*/

/**
 * @brief MPI function MPI_Allreduce
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op operation
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


/*MPI_Allreduce_init*/

/**
 * @brief MPI function MPI_Allreduce_init
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op operation
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Alltoall*/

/**
 * @brief MPI function MPI_Alltoall
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Alltoall_init*/

/**
 * @brief MPI function MPI_Alltoall_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Alltoallv*/

/**
 * @brief MPI function MPI_Alltoallv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls integer array (of length group size). Entry j specifies the displacement (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Alltoallv_init*/

/**
 * @brief MPI function MPI_Alltoallv_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls Integer array (of length group size). Entry j specifies the displacement (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Alltoallw*/

/**
 * @brief MPI function MPI_Alltoallw
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls integer array (of length group size). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtypes array of datatypes (of length group size). Entry j specifies the type of data to send to process j
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtypes array of datatypes (of length group size). Entry i specifies the type of data received from process i
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);


/*MPI_Alltoallw_init*/

/**
 * @brief MPI function MPI_Alltoallw_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls integer array (of length group size). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtypes Array of datatypes (of length group size). Entry j specifies the type of data to send to process j
 * @param recvbuf address of receive buffer
 * @param recvcounts integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtypes array of datatypes (of length group size). Entry i specifies the type of data received from process i
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Alltoallw_init(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallw_init(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Attr_delete*/

/**
 * @brief MPI function MPI_Attr_delete
 * 
 * @param comm communicator to which attribute is attached
 * @param keyval The key value of the deleted attribute
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Attr_delete(MPI_Comm comm, int keyval);
int PMPI_Attr_delete(MPI_Comm comm, int keyval);


/*MPI_Attr_get*/

/**
 * @brief MPI function MPI_Attr_get
 * 
 * @param comm communicator to which attribute is attached
 * @param keyval key value
 * @param attribute_val attribute value, unless flag = false
 * @param flag true if an attribute value was extracted; false if no attribute is associated with the key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);


/*MPI_Attr_put*/

/**
 * @brief MPI function MPI_Attr_put
 * 
 * @param comm communicator to which attribute will be attached
 * @param keyval key value, as returned by MPI_KEYVAL_CREATE
 * @param attribute_val attribute value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);


/*MPI_Barrier*/

/**
 * @brief MPI function MPI_Barrier
 * 
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Barrier(MPI_Comm comm);
int PMPI_Barrier(MPI_Comm comm);


/*MPI_Barrier_init*/

/**
 * @brief MPI function MPI_Barrier_init
 * 
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Bcast*/

/**
 * @brief MPI function MPI_Bcast
 * 
 * @param buffer starting address of buffer
 * @param count number of entries in buffer
 * @param datatype data type of buffer
 * @param root rank of broadcast root
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);


/*MPI_Bcast_init*/

/**
 * @brief MPI function MPI_Bcast_init
 * 
 * @param buffer starting address of buffer
 * @param count number of entries in buffer
 * @param datatype data type of buffer
 * @param root rank of broadcast root
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Bsend*/

/**
 * @brief MPI function MPI_Bsend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);


/*MPI_Bsend_init*/

/**
 * @brief MPI function MPI_Bsend_init
 * 
 * @param buf initial address of send buffer
 * @param count number of elements sent
 * @param datatype type of each element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Buffer_attach*/

/**
 * @brief MPI function MPI_Buffer_attach
 * 
 * @param buffer initial buffer address
 * @param size buffer size, in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Buffer_attach(void *buffer, int size);
int PMPI_Buffer_attach(void *buffer, int size);


/*MPI_Buffer_detach*/

/**
 * @brief MPI function MPI_Buffer_detach
 * 
 * @param buffer_addr initial buffer address
 * @param size buffer size, in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Buffer_detach(void *buffer_addr, int *size);
int PMPI_Buffer_detach(void *buffer_addr, int *size);


/*MPI_Cancel*/

/**
 * @brief MPI function MPI_Cancel
 * 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cancel(MPI_Request *request);
int PMPI_Cancel(MPI_Request *request);


/*MPI_Cart_coords*/

/**
 * @brief MPI function MPI_Cart_coords
 * 
 * @param comm communicator with Cartesian structure
 * @param rank rank of a process within group of comm
 * @param maxdims length of vector  coords in the calling program
 * @param coords integer array (of size maxdims) containing the Cartesian coordinates of specified process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);
int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);


/*MPI_Cart_create*/

/**
 * @brief MPI function MPI_Cart_create
 * 
 * @param comm_old input communicator
 * @param ndims number of dimensions of Cartesian grid
 * @param dims integer array of size ndims specifying the number of processes in each dimension
 * @param periods logical array of size ndims specifying whether the grid is periodic (true) or not (false) in each dimension
 * @param reorder ranking may be reordered (true) or not (false)
 * @param comm_cart communicator with new Cartesian topology
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart);
int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart);


/*MPI_Cart_get*/

/**
 * @brief MPI function MPI_Cart_get
 * 
 * @param comm communicator with Cartesian structure
 * @param maxdims length of vectors dims, periods, and coords in the calling program
 * @param dims number of processes for each Cartesian dimension
 * @param periods periodicity (true/false) for each Cartesian dimension
 * @param coords coordinates of calling process in Cartesian structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);
int PMPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);


/*MPI_Cart_map*/

/**
 * @brief MPI function MPI_Cart_map
 * 
 * @param comm input communicator
 * @param ndims number of dimensions of Cartesian structure
 * @param dims integer array of size ndims specifying the number of processes in each coordinate direction
 * @param periods logical array of size ndims specifying the periodicity specification in each coordinate direction
 * @param newrank reordered rank of the calling process; MPI_UNDEFINED if calling process does not belong to grid
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank);
int PMPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank);


/*MPI_Cart_rank*/

/**
 * @brief MPI function MPI_Cart_rank
 * 
 * @param comm communicator with Cartesian structure
 * @param coords integer array (of size ndims) specifying the Cartesian coordinates of a process
 * @param rank rank of specified process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank);
int PMPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank);


/*MPI_Cart_shift*/

/**
 * @brief MPI function MPI_Cart_shift
 * 
 * @param comm communicator with Cartesian structure
 * @param direction coordinate dimension of shift
 * @param disp displacement ($> 0$: upwards shift, $< 0$: downwards shift)
 * @param rank_source rank of source process
 * @param rank_dest rank of destination process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);
int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);


/*MPI_Cart_sub*/

/**
 * @brief MPI function MPI_Cart_sub
 * 
 * @param comm communicator with Cartesian structure
 * @param remain_dims the i-th entry of remain_dims specifies whether the i-th dimension is kept in the subgrid (true) or is dropped (false)
 * @param newcomm communicator containing the subgrid that includes the calling process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);
int PMPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);


/*MPI_Cartdim_get*/

/**
 * @brief MPI function MPI_Cartdim_get
 * 
 * @param comm communicator with Cartesian structure
 * @param ndims number of dimensions of the Cartesian structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Cartdim_get(MPI_Comm comm, int *ndims);
int PMPI_Cartdim_get(MPI_Comm comm, int *ndims);


/*MPI_Close_port*/

/**
 * @brief MPI function MPI_Close_port
 * 
 * @param port_name a port
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Close_port(const char *port_name);
int PMPI_Close_port(const char *port_name);


/*MPI_Comm_accept*/

/**
 * @brief MPI function MPI_Comm_accept
 * 
 * @param port_name port name
 * @param info implementation-dependent information
 * @param root rank in comm of root node
 * @param comm intracommunicator over which call is collective
 * @param newcomm intercommunicator with client as remote group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);


/*MPI_Comm_call_errhandler*/

/**
 * @brief MPI function MPI_Comm_call_errhandler
 * 
 * @param comm communicator with error handler
 * @param errorcode 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);


/*MPI_Comm_compare*/

/**
 * @brief MPI function MPI_Comm_compare
 * 
 * @param comm1 first communicator
 * @param comm2 second communicator
 * @param result result
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);


/*MPI_Comm_connect*/

/**
 * @brief MPI function MPI_Comm_connect
 * 
 * @param port_name network address
 * @param info implementation-dependent information
 * @param root rank in comm of root node
 * @param comm intracommunicator over which call is collective
 * @param newcomm intercommunicator with server as remote group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);


/*MPI_Comm_create*/

/**
 * @brief MPI function MPI_Comm_create
 * 
 * @param comm communicator
 * @param group group, which is a subset of the group of comm
 * @param newcomm new communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);


/*MPI_Comm_create_errhandler*/

/**
 * @brief MPI function MPI_Comm_create_errhandler
 * 
 * @param comm_errhandler_fn user defined error handling procedure
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Errhandler *errhandler);


/*MPI_Comm_create_group*/

/**
 * @brief MPI function MPI_Comm_create_group
 * 
 * @param comm intracommunicator
 * @param group group, which is a subset of the group of comm
 * @param tag tag
 * @param newcomm new communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm);
int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm);


/*MPI_Comm_create_keyval*/

/**
 * @brief MPI function MPI_Comm_create_keyval
 * 
 * @param comm_copy_attr_fn copy callback function for comm_keyval
 * @param comm_delete_attr_fn delete callback function for comm_keyval
 * @param comm_keyval key value for future access
 * @param extra_state extra state for callback function
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state);
int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state);


/*MPI_Comm_delete_attr*/

/**
 * @brief MPI function MPI_Comm_delete_attr
 * 
 * @param comm communicator from which the attribute is deleted
 * @param comm_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);
int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);


/*MPI_Comm_disconnect*/

/**
 * @brief MPI function MPI_Comm_disconnect
 * 
 * @param comm 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_disconnect(MPI_Comm *comm);
int PMPI_Comm_disconnect(MPI_Comm *comm);


/*MPI_Comm_dup*/

/**
 * @brief MPI function MPI_Comm_dup
 * 
 * @param comm communicator
 * @param newcomm copy of comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);


/*MPI_Comm_dup_with_info*/

/**
 * @brief MPI function MPI_Comm_dup_with_info
 * 
 * @param comm communicator
 * @param info info object
 * @param newcomm copy of comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);


/*MPI_Comm_free*/

/**
 * @brief MPI function MPI_Comm_free
 * 
 * @param comm communicator to be destroyed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_free(MPI_Comm *comm);
int PMPI_Comm_free(MPI_Comm *comm);


/*MPI_Comm_free_keyval*/

/**
 * @brief MPI function MPI_Comm_free_keyval
 * 
 * @param comm_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_free_keyval(int *comm_keyval);
int PMPI_Comm_free_keyval(int *comm_keyval);


/*MPI_Comm_get_attr*/

/**
 * @brief MPI function MPI_Comm_get_attr
 * 
 * @param comm communicator to which the attribute is attached
 * @param comm_keyval key value
 * @param attribute_val attribute value, unless flag = false
 * @param flag false if no attribute is associated with the key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);


/*MPI_Comm_get_errhandler*/

/**
 * @brief MPI function MPI_Comm_get_errhandler
 * 
 * @param comm 
 * @param errhandler error handler currently associated with communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler);
int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler);


/*MPI_Comm_get_info*/

/**
 * @brief MPI function MPI_Comm_get_info
 * 
 * @param comm communicator object
 * @param info_used new info object
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used);
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used);


/*MPI_Comm_get_name*/

/**
 * @brief MPI function MPI_Comm_get_name
 * 
 * @param comm communicator whose name is to be returned
 * @param comm_name the name previously stored on the communicator, or an empty string if no such name exists
 * @param resultlen length of returned name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);


/*MPI_Comm_get_parent*/

/**
 * @brief MPI function MPI_Comm_get_parent
 * 
 * @param parent the parent communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_get_parent(MPI_Comm *parent);
int PMPI_Comm_get_parent(MPI_Comm *parent);


/*MPI_Comm_group*/

/**
 * @brief MPI function MPI_Comm_group
 * 
 * @param comm communicator
 * @param group group corresponding to comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_group(MPI_Comm comm, MPI_Group *group);
int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group);


/*MPI_Comm_idup*/

/**
 * @brief MPI function MPI_Comm_idup
 * 
 * @param comm communicator
 * @param newcomm copy of comm
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);
int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);


/*MPI_Comm_idup_with_info*/

/**
 * @brief MPI function MPI_Comm_idup_with_info
 * 
 * @param comm communicator
 * @param info info object
 * @param newcomm copy of comm
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request);
int PMPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request);


/*MPI_Comm_join*/

/**
 * @brief MPI function MPI_Comm_join
 * 
 * @param fd socket file descriptor
 * @param intercomm new intercommunicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_join(int fd, MPI_Comm *intercomm);
int PMPI_Comm_join(int fd, MPI_Comm *intercomm);


/*MPI_Comm_rank*/

/**
 * @brief MPI function MPI_Comm_rank
 * 
 * @param comm communicator
 * @param rank rank of the calling process in group of comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int PMPI_Comm_rank(MPI_Comm comm, int *rank);


/*MPI_Comm_remote_group*/

/**
 * @brief MPI function MPI_Comm_remote_group
 * 
 * @param comm inter-communicator
 * @param group remote group corresponding to comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);
int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);


/*MPI_Comm_remote_size*/

/**
 * @brief MPI function MPI_Comm_remote_size
 * 
 * @param comm inter-communicator
 * @param size number of processes in the remote group of comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_remote_size(MPI_Comm comm, int *size);
int PMPI_Comm_remote_size(MPI_Comm comm, int *size);


/*MPI_Comm_set_attr*/

/**
 * @brief MPI function MPI_Comm_set_attr
 * 
 * @param comm communicator to which attribute will be attached
 * @param comm_keyval key value
 * @param attribute_val attribute value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);


/*MPI_Comm_set_errhandler*/

/**
 * @brief MPI function MPI_Comm_set_errhandler
 * 
 * @param comm 
 * @param errhandler new error handler for communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);


/*MPI_Comm_set_info*/

/**
 * @brief MPI function MPI_Comm_set_info
 * 
 * @param comm communicator
 * @param info info object
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info);
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info);


/*MPI_Comm_set_name*/

/**
 * @brief MPI function MPI_Comm_set_name
 * 
 * @param comm communicator whose identifier is to be set
 * @param comm_name the character string which is remembered as the name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name);
int PMPI_Comm_set_name(MPI_Comm comm, const char *comm_name);


/*MPI_Comm_size*/

/**
 * @brief MPI function MPI_Comm_size
 * 
 * @param comm communicator
 * @param size number of processes in the group of comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_size(MPI_Comm comm, int *size);
int PMPI_Comm_size(MPI_Comm comm, int *size);


/*MPI_Comm_spawn*/

/**
 * @brief MPI function MPI_Comm_spawn
 * 
 * @param command name of program to be spawned
 * @param argv arguments to command
 * @param maxprocs maximum number of processes to start
 * @param info a set of key-value pairs telling the runtime system where and how to start the processes
 * @param root rank of process in which previous arguments are examined
 * @param comm intracommunicator containing group of spawning processes
 * @param intercomm intercommunicator between original group and the newly spawned group
 * @param array_of_errcodes one code per process (array of integer)
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int PMPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);


/*MPI_Comm_spawn_multiple*/

/**
 * @brief MPI function MPI_Comm_spawn_multiple
 * 
 * @param count number of commands
 * @param array_of_commands programs to be executed
 * @param array_of_argv arguments for commands
 * @param array_of_maxprocs maximum number of processes to start for each command
 * @param array_of_info info objects telling the runtime system where and how to start processes
 * @param root rank of process in which previous arguments are examined
 * @param comm intracommunicator containing group of spawning processes
 * @param intercomm intercommunicator between original group and the newly spawned group
 * @param array_of_errcodes one error code per process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);


/*MPI_Comm_split*/

/**
 * @brief MPI function MPI_Comm_split
 * 
 * @param comm communicator
 * @param color control of subset assignment
 * @param key control of rank assignment
 * @param newcomm new communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);


/*MPI_Comm_split_type*/

/**
 * @brief MPI function MPI_Comm_split_type
 * 
 * @param comm communicator
 * @param split_type type of processes to be grouped together
 * @param key control of rank assignment
 * @param info info argument
 * @param newcomm new communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm);
int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm);

/*MPIX_Get_hwsubdomain_types*/

/**
 * @brief MPI function MPIX_Get_hwsubdomain_types
 * 
 * @param value split level available separated with comma
 *
 */
void MPIX_Get_hwsubdomain_types(char *value);
void PMPIX_Get_hwsubdomain_types(char *value);

/*MPI_Comm_test_inter*/

/**
 * @brief MPI function MPI_Comm_test_inter
 * 
 * @param comm communicator
 * @param flag 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Comm_test_inter(MPI_Comm comm, int *flag);
int PMPI_Comm_test_inter(MPI_Comm comm, int *flag);


/*MPI_Compare_and_swap*/

/**
 * @brief MPI function MPI_Compare_and_swap
 * 
 * @param origin_addr initial address of buffer
 * @param compare_addr initial address of compare buffer
 * @param result_addr initial address of result buffer
 * @param datatype datatype of the element in all buffers
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);
int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);


/*MPI_Dims_create*/

/**
 * @brief MPI function MPI_Dims_create
 * 
 * @param nnodes number of nodes in a grid
 * @param ndims number of Cartesian dimensions
 * @param dims integer array of size ndims specifying the number of nodes in each dimension
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Dims_create(int nnodes, int ndims, int dims[]);
int PMPI_Dims_create(int nnodes, int ndims, int dims[]);


/*MPI_Dist_graph_create*/

/**
 * @brief MPI function MPI_Dist_graph_create
 * 
 * @param comm_old input communicator
 * @param n number of source nodes for which this process specifies edges
 * @param sources array containing the n source nodes for which this process specifies edges
 * @param degrees array specifying the number of destinations for each source node in the source node array
 * @param destinations destination nodes for the source nodes in the source node array
 * @param weights weights for source to destination edges
 * @param info hints on optimization and interpretation of weights
 * @param reorder the ranks may be reordered (true) or not~(\const{false})
 * @param comm_dist_graph communicator with distributed graph topology added
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);


/*MPI_Dist_graph_create_adjacent*/

/**
 * @brief MPI function MPI_Dist_graph_create_adjacent
 * 
 * @param comm_old input communicator
 * @param indegree size of sources and sourceweights arrays
 * @param sources ranks of processes for which the calling process is a destination
 * @param sourceweights weights of the edges into the calling process
 * @param outdegree size of destinations and destweights arrays
 * @param destinations ranks of processes for which the calling process is a source
 * @param destweights weights of the edges out of the calling process
 * @param info hints on optimization and interpretation of weights
 * @param reorder the ranks may be reordered (true) or not~(\const{false})
 * @param comm_dist_graph communicator with distributed graph topology
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);


/*MPI_Dist_graph_neighbors*/

/**
 * @brief MPI function MPI_Dist_graph_neighbors
 * 
 * @param comm communicator with distributed graph topology
 * @param maxindegree size of sources and sourceweights arrays
 * @param sources processes for which the calling process is a destination
 * @param sourceweights weights of the edges into the calling process
 * @param maxoutdegree size of destinations and destweights arrays
 * @param destinations processes for which the calling process is a source
 * @param destweights weights of the edges out of the calling process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[]);
int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[]);


/*MPI_Dist_graph_neighbors_count*/

/**
 * @brief MPI function MPI_Dist_graph_neighbors_count
 * 
 * @param comm communicator with distributed graph topology
 * @param indegree number of edges into this process
 * @param outdegree number of edges out of this process
 * @param weighted false if MPI_UNWEIGHTED was supplied during creation, true otherwise
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted);
int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted);

/*MPI_Errhandler_create (DEPRECATED) */

/**
 * @brief MPI function MPI_Errhandler_create
 * 
 * @param function Function to use as errhandler
 * @param errhandler new errhandler
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler);
int PMPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler);

/*MPI_Errhandler_set (DEPRECATED) */

/**
 * @brief MPI function MPI_Errhandler_set
 * 
 * @param Comm to set the errhandler on
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

/*MPI_Errhandler_get (DEPRECATED) */

/**
 * @brief MPI function MPI_Errhandler_get
 * 
 * @param Comm to get the errhandler from
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);


/*MPI_Errhandler_free*/

/**
 * @brief MPI function MPI_Errhandler_free
 * 
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Errhandler_free(MPI_Errhandler *errhandler);
int PMPI_Errhandler_free(MPI_Errhandler *errhandler);


/*MPI_Error_class*/

/**
 * @brief MPI function MPI_Error_class
 * 
 * @param errorcode Error code returned by an \MPI/ routine
 * @param errorclass Error class associated with errorcode
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Error_class(int errorcode, int *errorclass);
int PMPI_Error_class(int errorcode, int *errorclass);


/*MPI_Error_string*/

/**
 * @brief MPI function MPI_Error_string
 * 
 * @param errorcode Error code returned by an \MPI/ routine
 * @param string Text that corresponds to the errorcode
 * @param resultlen Length (in printable characters) of the result returned in string
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Error_string(int errorcode, char *string, int *resultlen);
int PMPI_Error_string(int errorcode, char *string, int *resultlen);


/*MPI_Exscan*/

/**
 * @brief MPI function MPI_Exscan
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm intracommunicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


/*MPI_Exscan_init*/

/**
 * @brief MPI function MPI_Exscan_init
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm intracommunicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Exscan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Exscan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Fetch_and_op*/

/**
 * @brief MPI function MPI_Fetch_and_op
 * 
 * @param origin_addr initial address of buffer
 * @param result_addr initial address of result buffer
 * @param datatype datatype of the entry in origin, result, and target buffers
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param op reduce operation
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);
int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);


/*MPI_Finalize*/

/**
 * @brief MPI function MPI_Finalize
 * 
 *
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Finalize();
int PMPI_Finalize();


/*MPI_Finalized*/

/**
 * @brief MPI function MPI_Finalized
 * 
 * @param flag true if \mpi/ was finalized
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Finalized(int *flag);
int PMPI_Finalized(int *flag);


/*MPI_Free_mem*/

/**
 * @brief MPI function MPI_Free_mem
 * 
 * @param base initial address of memory segment allocated by MPI_ALLOC_MEM
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Free_mem(void *base);
int PMPI_Free_mem(void *base);


/*MPI_Gather*/

/**
 * @brief MPI function MPI_Gather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements for any single receive
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);


/*MPI_Gather_init*/

/**
 * @brief MPI function MPI_Gather_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements for any single receive
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Gatherv*/

/**
 * @brief MPI function MPI_Gatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);


/*MPI_Gatherv_init*/

/**
 * @brief MPI function MPI_Gatherv_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);

/*MPI_Get*/

/**
 * @brief MPI function MPI_Get
 * 
 * @param origin_addr initial address of origin buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param target_rank rank of target
 * @param target_disp displacement from window start to the beginning of the target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param win window object used for communication
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);


/*MPI_Get_accumulate*/

/**
 * @brief MPI function MPI_Get_accumulate
 * 
 * @param origin_addr initial address of buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param result_addr initial address of result buffer
 * @param result_count number of entries in result buffer
 * @param result_datatype datatype of each entry in result buffer
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param op reduce operation
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);


/*MPI_Get_address*/

/**
 * @brief MPI function MPI_Get_address
 * 
 * @param location location in caller memory
 * @param address address of location
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_address(const void *location, MPI_Aint *address);
int PMPI_Get_address(const void *location, MPI_Aint *address);


/*MPI_Address (DEPRECATED) */

/**
 * @brief MPI function MPI_Address
 * 
 * @param location location in caller memory
 * @param address address of location
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Address(const void *location, MPI_Aint *address);
int PMPI_Address(const void *location, MPI_Aint *address);


/*MPI_Get_count*/

/**
 * @brief MPI function MPI_Get_count
 * 
 * @param status return status of receive operation
 * @param datatype datatype of each receive buffer entry
 * @param count number of received entries
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);
int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);


/*MPI_Get_library_version*/

/**
 * @brief MPI function MPI_Get_library_version
 * 
 * @param version version number
 * @param resultlen Length (in printable characters) of the result returned in version
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_library_version(char *version, int *resultlen);
int PMPI_Get_library_version(char *version, int *resultlen);


/*MPI_Get_processor_name*/

/**
 * @brief MPI function MPI_Get_processor_name
 * 
 * @param name A unique specifier for the actual (as opposed to virtual) node.
 * @param resultlen Length (in printable characters) of the result returned in name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_processor_name(char *name, int *resultlen);
int PMPI_Get_processor_name(char *name, int *resultlen);


/*MPI_Get_version*/

/**
 * @brief MPI function MPI_Get_version
 * 
 * @param version version number
 * @param subversion subversion number
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_version(int *version, int *subversion);
int PMPI_Get_version(int *version, int *subversion);


/*MPI_Graph_create*/

/**
 * @brief MPI function MPI_Graph_create
 * 
 * @param comm_old input communicator
 * @param nnodes number of nodes in graph
 * @param index array of integers describing node degrees (see below)
 * @param edges array of integers describing graph edges (see below)
 * @param reorder ranking may be reordered (true) or not (false)
 * @param comm_graph communicator with graph topology added
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int index[], const int edges[], int reorder, MPI_Comm *comm_graph);
int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, const int index[], const int edges[], int reorder, MPI_Comm *comm_graph);


/*MPI_Graph_get*/

/**
 * @brief MPI function MPI_Graph_get
 * 
 * @param comm communicator with graph structure
 * @param maxindex length of vector index in the calling program
 * @param maxedges length of vector edges in the calling program
 * @param index array of integers containing the graph structure (for details see the definition of MPI_GRAPH_CREATE)
 * @param edges array of integers containing the graph structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[]);
int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[]);


/*MPI_Graph_map*/

/**
 * @brief MPI function MPI_Graph_map
 * 
 * @param comm input communicator
 * @param nnodes number of graph nodes
 * @param index integer array specifying the graph structure, see MPI_GRAPH_CREATE
 * @param edges integer array specifying the graph structure
 * @param newrank reordered rank of the calling process; MPI_UNDEFINED if the calling process does not belong to graph
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graph_map(MPI_Comm comm, int nnodes, const int index[], const int edges[], int *newrank);
int PMPI_Graph_map(MPI_Comm comm, int nnodes, const int index[], const int edges[], int *newrank);


/*MPI_Graph_neighbors*/

/**
 * @brief MPI function MPI_Graph_neighbors
 * 
 * @param comm communicator with graph topology
 * @param rank rank of process in group of comm
 * @param maxneighbors size of array neighbors
 * @param neighbors ranks of processes that are neighbors to specified process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]);
int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]);


/*MPI_Graph_neighbors_count*/

/**
 * @brief MPI function MPI_Graph_neighbors_count
 * 
 * @param comm communicator with graph topology
 * @param rank rank of process in group of comm
 * @param nneighbors number of neighbors of specified process
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);


/*MPI_Graphdims_get*/

/**
 * @brief MPI function MPI_Graphdims_get
 * 
 * @param comm communicator for group with graph structure
 * @param nnodes number of nodes in graph (same as number of processes in the group)
 * @param nedges number of edges in graph
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);


/*MPI_Grequest_complete*/

/**
 * @brief MPI function MPI_Grequest_complete
 * 
 * @param request generalized request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Grequest_complete(MPI_Request request);
int PMPI_Grequest_complete(MPI_Request request);


/*MPI_Grequest_start*/

/**
 * @brief MPI function MPI_Grequest_start
 * 
 * @param query_fn callback function invoked when request status is queried
 * @param free_fn callback function invoked when request is freed
 * @param cancel_fn callback function invoked when request is cancelled
 * @param extra_state extra state
 * @param request generalized request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request);
int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request);


/*MPI_Group_compare*/

/**
 * @brief MPI function MPI_Group_compare
 * 
 * @param group1 first group
 * @param group2 second group
 * @param result result
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);


/*MPI_Group_difference*/

/**
 * @brief MPI function MPI_Group_difference
 * 
 * @param group1 first group
 * @param group2 second group
 * @param newgroup difference group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);


/*MPI_Group_excl*/

/**
 * @brief MPI function MPI_Group_excl
 * 
 * @param group group
 * @param n number of elements in array ranks
 * @param ranks array of integer ranks of processes in group not to appear in newgroup
 * @param newgroup new group derived from above, preserving the order defined by group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int PMPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);


/*MPI_Group_free*/

/**
 * @brief MPI function MPI_Group_free
 * 
 * @param group group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_free(MPI_Group *group);
int PMPI_Group_free(MPI_Group *group);


/*MPI_Group_incl*/

/**
 * @brief MPI function MPI_Group_incl
 * 
 * @param group group
 * @param n number of elements in array ranks (and size of newgroup)
 * @param ranks ranks of processes in group to appear in newgroup
 * @param newgroup new group derived from above, in the order defined by ranks
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int PMPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);


/*MPI_Group_intersection*/

/**
 * @brief MPI function MPI_Group_intersection
 * 
 * @param group1 first group
 * @param group2 second group
 * @param newgroup intersection group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);


/*MPI_Group_range_excl*/

/**
 * @brief MPI function MPI_Group_range_excl
 * 
 * @param group group
 * @param n number of triplets in array ranges
 * @param ranges a one-dimensional array of integer triplets, of the form (first rank, last rank, stride) indicating ranks in group of processes to be excluded from the output group newgroup
 * @param newgroup new group derived from above, preserving the order in group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);


/*MPI_Group_range_incl*/

/**
 * @brief MPI function MPI_Group_range_incl
 * 
 * @param group group
 * @param n number of triplets in array ranges
 * @param ranges a one-dimensional array of integer triplets, of the form (first rank, last rank, stride) indicating ranks in group of processes to be included in newgroup
 * @param newgroup new group derived from above, in the order defined by ranges
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);


/*MPI_Group_rank*/

/**
 * @brief MPI function MPI_Group_rank
 * 
 * @param group group
 * @param rank rank of the calling process in group, or MPI_UNDEFINED if the process is not a member
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_rank(MPI_Group group, int *rank);
int PMPI_Group_rank(MPI_Group group, int *rank);


/*MPI_Group_size*/

/**
 * @brief MPI function MPI_Group_size
 * 
 * @param group group
 * @param size number of processes in the group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_size(MPI_Group group, int *size);
int PMPI_Group_size(MPI_Group group, int *size);


/*MPI_Group_translate_ranks*/

/**
 * @brief MPI function MPI_Group_translate_ranks
 * 
 * @param group1 group1
 * @param n number of ranks in ranks1 and ranks2 arrays
 * @param ranks1 array of zero or more valid ranks in group1
 * @param group2 group2
 * @param ranks2 array of corresponding ranks in group2, MPI_UNDEFINED when no correspondence exists.
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]);
int PMPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]);


/*MPI_Group_union*/

/**
 * @brief MPI function MPI_Group_union
 * 
 * @param group1 first group
 * @param group2 second group
 * @param newgroup union group
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);


/*MPI_Iallgather*/

/**
 * @brief MPI function MPI_Iallgather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Iallgatherv*/

/**
 * @brief MPI function MPI_Iallgatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Iallreduce*/

/**
 * @brief MPI function MPI_Iallreduce
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op operation
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);


/*MPI_Ialltoall*/

/**
 * @brief MPI function MPI_Ialltoall
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ialltoallv*/

/**
 * @brief MPI function MPI_Ialltoallv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls integer array (of length group size). Entry j specifies the displacement (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ialltoallw*/

/**
 * @brief MPI function MPI_Ialltoallw
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts integer array (of length group size) specifying the number of elements to send to each rank
 * @param sdispls integer array (of length group size). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for process j
 * @param sendtypes array of datatypes (of length group size). Entry j specifies the type of data to send to process j
 * @param recvbuf address of receive buffer
 * @param recvcounts integer array (of length group size) specifying the number of elements that can be received from each rank
 * @param rdispls integer array (of length group size). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from process i
 * @param recvtypes array of datatypes (of length group size). Entry i specifies the type of data received from process i
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);


/*MPI_Ibarrier*/

/**
 * @brief MPI function MPI_Ibarrier
 * 
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request);
int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request);


/*MPI_Ibcast*/

/**
 * @brief MPI function MPI_Ibcast
 * 
 * @param buffer starting address of buffer
 * @param count number of entries in buffer
 * @param datatype data type of buffer
 * @param root rank of broadcast root
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Ibsend*/

/**
 * @brief MPI function MPI_Ibsend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Iexscan*/

/**
 * @brief MPI function MPI_Iexscan
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm intracommunicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);


/*MPI_Igather*/

/**
 * @brief MPI function MPI_Igather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements for any single receive
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Igatherv*/

/**
 * @brief MPI function MPI_Igatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) containing the number of elements that are received from each process
 * @param displs integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Improbe*/

/**
 * @brief MPI function MPI_Improbe
 * 
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param flag flag
 * @param message returned message
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status);
int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status);


/*MPI_Imrecv*/

/**
 * @brief MPI function MPI_Imrecv
 * 
 * @param buf initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param message message
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);


/*MPI_Ineighbor_allgather*/

/**
 * @brief MPI function MPI_Ineighbor_allgather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ineighbor_allgatherv*/

/**
 * @brief MPI function MPI_Ineighbor_allgatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) containing the number of elements that are received from each neighbor
 * @param displs integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ineighbor_alltoall*/

/**
 * @brief MPI function MPI_Ineighbor_alltoall
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ineighbor_alltoallv*/

/**
 * @brief MPI function MPI_Ineighbor_alltoallv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement (relative to sendbuf) from which send the outgoing data to neighbor j
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);


/*MPI_Ineighbor_alltoallw*/

/**
 * @brief MPI function MPI_Ineighbor_alltoallw
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for neighbor j
 * @param sendtypes array of datatypes (of length outdegree). Entry j specifies the type of data to send to neighbor j
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtypes array of datatypes (of length indegree). Entry i specifies the type of data received from neighbor i
 * @param comm communicator with topology structure
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);


/*MPI_Info_create*/

/**
 * @brief MPI function MPI_Info_create
 * 
 * @param info info object created
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_create(MPI_Info *info);
int PMPI_Info_create(MPI_Info *info);


/*MPI_Info_delete*/

/**
 * @brief MPI function MPI_Info_delete
 * 
 * @param info info object
 * @param key key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_delete(MPI_Info info, const char *key);
int PMPI_Info_delete(MPI_Info info, const char *key);


/*MPI_Info_dup*/

/**
 * @brief MPI function MPI_Info_dup
 * 
 * @param info info object
 * @param newinfo info object
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo);
int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo);


/*MPI_Info_free*/

/**
 * @brief MPI function MPI_Info_free
 * 
 * @param info info object
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_free(MPI_Info *info);
int PMPI_Info_free(MPI_Info *info);


/*MPI_Info_get*/

/**
 * @brief MPI function MPI_Info_get
 * 
 * @param info info object
 * @param key key
 * @param valuelen length of value arg
 * @param value value
 * @param flag true if key defined, false if not
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag);
int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag);


/*MPI_Info_get_nkeys*/

/**
 * @brief MPI function MPI_Info_get_nkeys
 * 
 * @param info info object
 * @param nkeys number of defined keys
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys);
int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys);


/*MPI_Info_get_nthkey*/

/**
 * @brief MPI function MPI_Info_get_nthkey
 * 
 * @param info info object
 * @param n key number
 * @param key key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key);
int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key);


/*MPI_Info_get_string*/

/**
 * @brief MPI function MPI_Info_get_string
 * 
 * @param info info object
 * @param key key
 * @param buflen length of buffer
 * @param value value
 * @param flag true if key defined, false if not
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag);
int PMPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag);


/*MPI_Info_get_valuelen*/

/**
 * @brief MPI function MPI_Info_get_valuelen
 * 
 * @param info info object
 * @param key key
 * @param valuelen length of value arg
 * @param flag true if key defined, false if not
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag);
int PMPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag);


/*MPI_Info_set*/

/**
 * @brief MPI function MPI_Info_set
 * 
 * @param info info object
 * @param key key
 * @param value value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Info_set(MPI_Info info, const char *key, const char *value);
int PMPI_Info_set(MPI_Info info, const char *key, const char *value);


/*MPI_Init*/

/**
 * @brief MPI function MPI_Init
 * 
 * @param argc 
 * @param argv 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Init(int *argc, char ***argv);
int PMPI_Init(int *argc, char ***argv);


/*MPI_Init_thread*/

/**
 * @brief MPI function MPI_Init_thread
 * 
 * @param argc 
 * @param argv 
 * @param required desired level of thread support
 * @param provided provided level of thread support
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided);
int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided);


/*MPI_Initialized*/

/**
 * @brief MPI function MPI_Initialized
 * 
 * @param flag Flag is true if MPI_INIT has been called and false otherwise
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Initialized(int *flag);
int PMPI_Initialized(int *flag);


/*MPI_Intercomm_create*/

/**
 * @brief MPI function MPI_Intercomm_create
 * 
 * @param local_comm local intra-communicator
 * @param local_leader rank of local group leader in local_comm
 * @param peer_comm ``peer'' communicator; significant only at the local_leader
 * @param remote_leader rank of remote group leader in peer_comm; significant only at the local_leader
 * @param tag tag
 * @param newintercomm new inter-communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);


/*MPI_Intercomm_merge*/

/**
 * @brief MPI function MPI_Intercomm_merge
 * 
 * @param intercomm Inter-Communicator
 * @param high 
 * @param newintracomm new intra-communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);


/*MPI_Iprobe*/

/**
 * @brief MPI function MPI_Iprobe
 * 
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param flag 
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);


/*MPI_Irecv*/

/**
 * @brief MPI function MPI_Irecv
 * 
 * @param buf initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Ireduce*/

/**
 * @brief MPI function MPI_Ireduce
 * 
 * @param sendbuf address of send buffer
 * @param recvbuf address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op reduce operation
 * @param root rank of root process
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Ireduce_scatter*/

/**
 * @brief MPI function MPI_Ireduce_scatter
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array specifying the number of elements in result distributed to each process. This array must be identical on all calling processes.
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);


/*MPI_Ireduce_scatter_block*/

/**
 * @brief MPI function MPI_Ireduce_scatter_block
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcount element count per block
 * @param datatype data type of elements of send and receive buffers
 * @param op operation
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);


/*MPI_Irsend*/

/**
 * @brief MPI function MPI_Irsend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Is_thread_main*/

/**
 * @brief MPI function MPI_Is_thread_main
 * 
 * @param flag true if calling thread is main thread, false otherwise
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Is_thread_main(int *flag);
int PMPI_Is_thread_main(int *flag);


/*MPI_Iscan*/

/**
 * @brief MPI function MPI_Iscan
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);


/*MPI_Iscatter*/

/**
 * @brief MPI function MPI_Iscatter
 * 
 * @param sendbuf address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Iscatterv*/

/**
 * @brief MPI function MPI_Iscatterv
 * 
 * @param sendbuf address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to sendbuf) from which to take the outgoing data to process i
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);


/*MPI_Isend*/

/**
 * @brief MPI function MPI_Isend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Issend*/

/**
 * @brief MPI function MPI_Issend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Keyval_create*/

/**
 * @brief MPI function MPI_Keyval_create
 * 
 * @param copy_fn Copy callback function for keyval
 * @param delete_fn Delete callback function for keyval
 * @param keyval key value for future access
 * @param extra_state Extra state for callback functions
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
int PMPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);


/*MPI_Keyval_free*/

/**
 * @brief MPI function MPI_Keyval_free
 * 
 * @param keyval Frees the integer key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Keyval_free(int *keyval);
int PMPI_Keyval_free(int *keyval);


/*MPI_Lookup_name*/

/**
 * @brief MPI function MPI_Lookup_name
 * 
 * @param service_name a service name
 * @param info implementation-specific information
 * @param port_name a port name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name);
int PMPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name);


/*MPI_Mprobe*/

/**
 * @brief MPI function MPI_Mprobe
 * 
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param message returned message
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);


/*MPI_Mrecv*/

/**
 * @brief MPI function MPI_Mrecv
 * 
 * @param buf initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param message message
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);


/*MPI_Neighbor_allgather*/

/**
 * @brief MPI function MPI_Neighbor_allgather
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Neighbor_allgather_init*/

/**
 * @brief MPI function MPI_Neighbor_allgather_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Neighbor_allgatherv*/

/**
 * @brief MPI function MPI_Neighbor_allgatherv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) containing the number of elements that are received from each neighbor
 * @param displs integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Neighbor_allgatherv_init*/

/**
 * @brief MPI function MPI_Neighbor_allgatherv_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) containing the number of elements that are received from each neighbor
 * @param displs integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Neighbor_alltoall*/

/**
 * @brief MPI function MPI_Neighbor_alltoall
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Neighbor_alltoall_init*/

/**
 * @brief MPI function MPI_Neighbor_alltoall_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each neighbor
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcount number of elements received from each neighbor
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Neighbor_alltoallv*/

/**
 * @brief MPI function MPI_Neighbor_alltoallv
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement (relative to sendbuf) from which to send the outgoing data to neighbor j
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);


/*MPI_Neighbor_alltoallv_init*/

/**
 * @brief MPI function MPI_Neighbor_alltoallv_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement (relative to sendbuf) from which send the outgoing data to neighbor j
 * @param sendtype data type of send buffer elements
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtype data type of receive buffer elements
 * @param comm communicator with topology structure
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Neighbor_alltoallw*/

/**
 * @brief MPI function MPI_Neighbor_alltoallw
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for neighbor j
 * @param sendtypes array of datatypes (of length outdegree). Entry j specifies the type of data to send to neighbor j
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtypes array of datatypes (of length indegree). Entry i specifies the type of data received from neighbor i
 * @param comm communicator with topology structure
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);


/*MPI_Neighbor_alltoallw_init*/

/**
 * @brief MPI function MPI_Neighbor_alltoallw_init
 * 
 * @param sendbuf starting address of send buffer
 * @param sendcounts non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
 * @param sdispls integer array (of length outdegree). Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for neighbor j
 * @param sendtypes array of datatypes (of length outdegree). Entry j specifies the type of data to send to neighbor j
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
 * @param rdispls integer array (of length indegree). Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from neighbor i
 * @param recvtypes array of datatypes (of length indegree). Entry i specifies the type of data received from neighbor i
 * @param comm communicator with topology structure
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Op_commutative*/

/**
 * @brief MPI function MPI_Op_commutative
 * 
 * @param op operation
 * @param commute true if op is commutative, false otherwise
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Op_commutative(MPI_Op op, int *commute);
int PMPI_Op_commutative(MPI_Op op, int *commute);


/*MPI_Op_create*/

/**
 * @brief MPI function MPI_Op_create
 * 
 * @param user_fn user defined function
 * @param commute true if commutative; false otherwise.
 * @param op operation
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);
int PMPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);


/*MPI_Op_free*/

/**
 * @brief MPI function MPI_Op_free
 * 
 * @param op operation
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Op_free(MPI_Op *op);
int PMPI_Op_free(MPI_Op *op);


/*MPI_Open_port*/

/**
 * @brief MPI function MPI_Open_port
 * 
 * @param info implementation-specific information on how to establish an address
 * @param port_name newly established port
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Open_port(MPI_Info info, char *port_name);
int PMPI_Open_port(MPI_Info info, char *port_name);


/*MPI_Pack*/

/**
 * @brief MPI function MPI_Pack
 * 
 * @param inbuf input buffer start
 * @param incount number of input data items
 * @param datatype datatype of each input data item
 * @param outbuf output buffer start
 * @param outsize output buffer size, in bytes
 * @param position current position in buffer, in bytes
 * @param comm communicator for packed message
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm);
int PMPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm);


/*MPI_Pack_external*/

/**
 * @brief MPI function MPI_Pack_external
 * 
 * @param datarep data representation
 * @param inbuf input buffer start
 * @param incount number of input data items
 * @param datatype datatype of each input data item
 * @param outbuf output buffer start
 * @param outsize output buffer size, in bytes
 * @param position current position in buffer, in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Pack_external(const char datarep[], const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);
int PMPI_Pack_external(const char datarep[], const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);


/*MPI_Pack_external_size*/

/**
 * @brief MPI function MPI_Pack_external_size
 * 
 * @param datarep data representation
 * @param incount number of input data items
 * @param datatype datatype of each input data item
 * @param size output buffer size, in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Pack_external_size(const char datarep[], int incount, MPI_Datatype datatype, MPI_Aint *size);
int PMPI_Pack_external_size(const char datarep[], int incount, MPI_Datatype datatype, MPI_Aint *size);


/*MPI_Pack_size*/

/**
 * @brief MPI function MPI_Pack_size
 * 
 * @param incount count argument to packing call
 * @param datatype datatype argument to packing call
 * @param comm communicator argument to packing call
 * @param size upper bound on size of packed message, in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);
int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);


/*MPI_Pcontrol*/

/**
 * @brief MPI function MPI_Pcontrol
 * 
 * @param level Profiling level
 * @param  
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Pcontrol(const int level, ... );
int PMPI_Pcontrol(const int level, ... );


/*MPI_Probe*/

/**
 * @brief MPI function MPI_Probe
 * 
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);


/*MPI_Publish_name*/

/**
 * @brief MPI function MPI_Publish_name
 * 
 * @param service_name a service name to associate with the port
 * @param info implementation-specific information
 * @param port_name a port name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name);
int PMPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name);


/*MPI_Put*/

/**
 * @brief MPI function MPI_Put
 * 
 * @param origin_addr initial address of origin buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param win window object used for communication
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);


/*MPI_Query_thread*/

/**
 * @brief MPI function MPI_Query_thread
 * 
 * @param provided provided level of thread support
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Query_thread(int *provided);
int PMPI_Query_thread(int *provided);


/*MPI_Raccumulate*/

/**
 * @brief MPI function MPI_Raccumulate
 * 
 * @param origin_addr initial address of buffer
 * @param origin_count number of entries in buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param op reduce operation
 * @param win 
 * @param request RMA request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);


/*MPI_Recv*/

/**
 * @brief MPI function MPI_Recv
 * 
 * @param buf initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);


/*MPI_Recv_init*/

/**
 * @brief MPI function MPI_Recv_init
 * 
 * @param buf initial address of receive buffer
 * @param count number of elements received
 * @param datatype type of each element
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Reduce*/

/**
 * @brief MPI function MPI_Reduce
 * 
 * @param sendbuf address of send buffer
 * @param recvbuf address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op reduce operation
 * @param root rank of root process
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);


/*MPI_Reduce_init*/

/**
 * @brief MPI function MPI_Reduce_init
 * 
 * @param sendbuf address of send buffer
 * @param recvbuf address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op reduce operation
 * @param root rank of root process
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Reduce_local*/

/**
 * @brief MPI function MPI_Reduce_local
 * 
 * @param inbuf input buffer
 * @param inoutbuf combined input and output buffer
 * @param count number of elements in inbuf and inoutbuf buffers
 * @param datatype data type of elements of inbuf and inoutbuf buffers
 * @param op operation
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);
int PMPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);


/*MPI_Reduce_scatter*/

/**
 * @brief MPI function MPI_Reduce_scatter
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array (of length group size) specifying the number of elements of the result distributed to each process.
 * @param datatype data type of elements of send and receive buffers
 * @param op operation
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


/*MPI_Reduce_scatter_block*/

/**
 * @brief MPI function MPI_Reduce_scatter_block
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcount element count per block
 * @param datatype data type of elements of send and receive buffers
 * @param op operation
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


/*MPI_Reduce_scatter_block_init*/

/**
 * @brief MPI function MPI_Reduce_scatter_block_init
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcount element count per block
 * @param datatype data type of elements of send and receive buffers
 * @param op operation
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Reduce_scatter_init*/

/**
 * @brief MPI function MPI_Reduce_scatter_init
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcounts non-negative integer array specifying the number of elements in result distributed to each process. This array must be identical on all calling processes.
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Register_datarep*/

/**
 * @brief MPI function MPI_Register_datarep
 * 
 * @param datarep data representation identifier
 * @param read_conversion_fn function invoked to convert from file representation to native representation
 * @param write_conversion_fn function invoked to convert from native representation to file representation
 * @param dtype_file_extent_fn function invoked to get the extent of a datatype as represented in the file
 * @param extra_state extra state
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Register_datarep(const char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);
int PMPI_Register_datarep(const char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);


/*MPI_Request_free*/

/**
 * @brief MPI function MPI_Request_free
 * 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Request_free(MPI_Request *request);
int PMPI_Request_free(MPI_Request *request);


/*MPI_Request_get_status*/

/**
 * @brief MPI function MPI_Request_get_status
 * 
 * @param request request
 * @param flag boolean flag, same as from MPI_TEST
 * @param status status object if flag is true
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status);
int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status);


/*MPI_Rget*/

/**
 * @brief MPI function MPI_Rget
 * 
 * @param origin_addr initial address of origin buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param target_rank rank of target
 * @param target_disp displacement from window start to the beginning of the target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param win window object used for communication
 * @param request RMA request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);


/*MPI_Rget_accumulate*/

/**
 * @brief MPI function MPI_Rget_accumulate
 * 
 * @param origin_addr initial address of buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param result_addr initial address of result buffer
 * @param result_count number of entries in result buffer
 * @param result_datatype datatype of entries in result buffer
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to beginning of target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param op reduce operation
 * @param win 
 * @param request RMA request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);


/*MPI_Rput*/

/**
 * @brief MPI function MPI_Rput
 * 
 * @param origin_addr initial address of origin buffer
 * @param origin_count number of entries in origin buffer
 * @param origin_datatype datatype of each entry in origin buffer
 * @param target_rank rank of target
 * @param target_disp displacement from start of window to target buffer
 * @param target_count number of entries in target buffer
 * @param target_datatype datatype of each entry in target buffer
 * @param win window object used for communication
 * @param request RMA request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);


/*MPI_Rsend*/

/**
 * @brief MPI function MPI_Rsend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);


/*MPI_Rsend_init*/

/**
 * @brief MPI function MPI_Rsend_init
 * 
 * @param buf initial address of send buffer
 * @param count number of elements sent
 * @param datatype type of each element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Scan*/

/**
 * @brief MPI function MPI_Scan
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


/*MPI_Scan_init*/

/**
 * @brief MPI function MPI_Scan_init
 * 
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in input buffer
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Scatter*/

/**
 * @brief MPI function MPI_Scatter
 * 
 * @param sendbuf address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);


/*MPI_Scatter_init*/

/**
 * @brief MPI function MPI_Scatter_init
 * 
 * @param sendbuf address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Scatterv*/

/**
 * @brief MPI function MPI_Scatterv
 * 
 * @param sendbuf address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to sendbuf) from which to take the outgoing data to process i
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);


/*MPI_Scatterv_init*/

/**
 * @brief MPI function MPI_Scatterv_init
 * 
 * @param sendbuf address of send buffer
 * @param sendcounts non-negative integer array (of length group size) specifying the number of elements to send to each rank
 * @param displs integer array (of length group size). Entry i specifies the displacement (relative to sendbuf) from which to take the outgoing data to process i
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 * @param info info argument
 * @param request communication request
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Scatterv_init(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatterv_init(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);


/*MPI_Send*/

/**
 * @brief MPI function MPI_Send
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);


/*MPI_Send_init*/

/**
 * @brief MPI function MPI_Send_init
 * 
 * @param buf initial address of send buffer
 * @param count number of elements sent
 * @param datatype type of each element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Sendrecv*/

/**
 * @brief MPI function MPI_Sendrecv
 * 
 * @param sendbuf initial address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype type of elements in send buffer
 * @param dest rank of destination
 * @param sendtag send tag
 * @param recvbuf initial address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype type of elements receive buffer element
 * @param source rank of source or MPI_ANY_SOURCE
 * @param recvtag receive tag or MPI_ANY_TAG
 * @param comm 
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);


/*MPI_Sendrecv_replace*/

/**
 * @brief MPI function MPI_Sendrecv_replace
 * 
 * @param buf initial address of send and receive buffer
 * @param count number of elements in send and receive buffer
 * @param datatype type of elements in send and receive buffer
 * @param dest rank of destination
 * @param sendtag send message tag
 * @param source rank of source or MPI_ANY_SOURCE
 * @param recvtag receive message tag or MPI_ANY_TAG
 * @param comm 
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);


/*MPI_Ssend*/

/**
 * @brief MPI function MPI_Ssend
 * 
 * @param buf initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);


/*MPI_Ssend_init*/

/**
 * @brief MPI function MPI_Ssend_init
 * 
 * @param buf initial address of send buffer
 * @param count number of elements sent
 * @param datatype type of each element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);


/*MPI_Start*/

/**
 * @brief MPI function MPI_Start
 * 
 * @param request 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Start(MPI_Request *request);
int PMPI_Start(MPI_Request *request);


/*MPI_Startall*/

/**
 * @brief MPI function MPI_Startall
 * 
 * @param count list length
 * @param array_of_requests array of requests
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Startall(int count, MPI_Request array_of_requests[]);
int PMPI_Startall(int count, MPI_Request array_of_requests[]);


/*MPI_Status_set_cancelled*/

/**
 * @brief MPI function MPI_Status_set_cancelled
 * 
 * @param status status with which to associate cancel flag
 * @param flag if true, indicates request was cancelled
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Status_set_cancelled(MPI_Status *status, int flag);
int PMPI_Status_set_cancelled(MPI_Status *status, int flag);


/*MPI_Status_set_elements*/

/**
 * @brief MPI function MPI_Status_set_elements
 * 
 * @param status status with which to associate count
 * @param datatype datatype associated with count
 * @param count number of elements to associate with status
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count);
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count);


/*MPI_Status_set_elements_x*/

/**
 * @brief MPI function MPI_Status_set_elements_x
 * 
 * @param status status with which to associate count
 * @param datatype datatype associated with count
 * @param count number of elements to associate with status
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);


/*MPI_T_category_changed*/

/**
 * @brief MPI function MPI_T_category_changed
 * 
 * @param stamp a virtual time stamp to indicate the last change to the categories
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_changed(int *stamp);
int PMPI_T_category_changed(int *stamp);


/*MPI_T_category_get_categories*/

/**
 * @brief MPI function MPI_T_category_get_categories
 * 
 * @param cat_index index of the category to be queried, in the range $0$ and $num_cat-1$
 * @param len the length of the indices array
 * @param indices an integer array of size len, indicating category indices
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_categories(int cat_index, int len, int indices[]);
int PMPI_T_category_get_categories(int cat_index, int len, int indices[]);


/*MPI_T_category_get_cvars*/

/**
 * @brief MPI function MPI_T_category_get_cvars
 * 
 * @param cat_index index of the category to be queried, in the range $0$ and $num_cat-1$
 * @param len the length of the indices array
 * @param indices an integer array of size len, indicating control variable indices
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_cvars(int cat_index, int len, int indices[]);
int PMPI_T_category_get_cvars(int cat_index, int len, int indices[]);


/*MPI_T_category_get_index*/

/**
 * @brief MPI function MPI_T_category_get_index
 * 
 * @param name the name of the category
 * @param cat_index the index of the category
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_index(const char *name, int *cat_index);
int PMPI_T_category_get_index(const char *name, int *cat_index);


/*MPI_T_category_get_info*/

/**
 * @brief MPI function MPI_T_category_get_info
 * 
 * @param cat_index index of the category to be queried
 * @param name buffer to return the string containing the name of the category
 * @param name_len length of the string and/or buffer for name
 * @param desc buffer to return the string containing the description of the category
 * @param desc_len length of the string and/or buffer for desc
 * @param num_cvars number of control variables in the category
 * @param num_pvars number of performance variables in the category
 * @param num_categories number of  categories contained in the category
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_cvars, int *num_pvars, int *num_categories);
int PMPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_cvars, int *num_pvars, int *num_categories);


/*MPI_T_category_get_num*/

/**
 * @brief MPI function MPI_T_category_get_num
 * 
 * @param num_cat current number of categories
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_num(int *num_cat);
int PMPI_T_category_get_num(int *num_cat);


/*MPI_T_category_get_pvars*/

/**
 * @brief MPI function MPI_T_category_get_pvars
 * 
 * @param cat_index index of the category to be queried, in the range $0$ and $num_cat-1$
 * @param len the length of the indices array
 * @param indices an integer array of size len, indicating performance variable indices
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_category_get_pvars(int cat_index, int len, int indices[]);
int PMPI_T_category_get_pvars(int cat_index, int len, int indices[]);


/*MPI_T_cvar_get_index*/

/**
 * @brief MPI function MPI_T_cvar_get_index
 * 
 * @param name name of the control variable
 * @param cvar_index index of the control variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_get_index(const char *name, int *cvar_index);
int PMPI_T_cvar_get_index(const char *name, int *cvar_index);


/*MPI_T_cvar_get_info*/

/**
 * @brief MPI function MPI_T_cvar_get_info
 * 
 * @param cvar_index index of the control variable to be queried, value between $0$ and $num_cvar-1$
 * @param name buffer to return the string containing the name of the control variable
 * @param name_len length of the string and/or buffer for name
 * @param verbosity verbosity level of this variable
 * @param datatype \mpi/ datatype of the information stored in the control variable
 * @param enumtype optional descriptor for enumeration information
 * @param desc buffer to return the string containing a description of the control variable
 * @param desc_len length of the string and/or buffer for desc
 * @param bind type of \mpi/ object to which this variable must be bound
 * @param scope scope of when changes to this variable are possible
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *scope);
int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *scope);


/*MPI_T_cvar_get_num*/

/**
 * @brief MPI function MPI_T_cvar_get_num
 * 
 * @param num_cvar returns number of control variables
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_get_num(int *num_cvar);
int PMPI_T_cvar_get_num(int *num_cvar);


/*MPI_T_cvar_handle_alloc*/

/**
 * @brief MPI function MPI_T_cvar_handle_alloc
 * 
 * @param cvar_index index of control variable for which handle is to be allocated
 * @param obj_handle reference to a handle of the \mpi/ object to which this variable is supposed to be bound
 * @param handle allocated handle
 * @param count number of elements used to represent this variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle, int *count);
int PMPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle, int *count);


/*MPI_T_cvar_handle_free*/

/**
 * @brief MPI function MPI_T_cvar_handle_free
 * 
 * @param handle handle to be freed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle);
int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle);


/*MPI_T_cvar_read*/

/**
 * @brief MPI function MPI_T_cvar_read
 * 
 * @param handle handle to the control variable to be read
 * @param buf initial address of storage location for variable value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf);
int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf);


/*MPI_T_cvar_write*/

/**
 * @brief MPI function MPI_T_cvar_write
 * 
 * @param handle handle to the control variable to be written
 * @param buf initial address of storage location for variable value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf);
int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf);


/*MPI_T_enum_get_info*/

/**
 * @brief MPI function MPI_T_enum_get_info
 * 
 * @param enumtype enumeration to be queried
 * @param num number of discrete values represented by this enumeration
 * @param name buffer to return the string containing the name of the enumeration item
 * @param name_len length of the string and/or buffer for name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len);
int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len);


/*MPI_T_enum_get_item*/

/**
 * @brief MPI function MPI_T_enum_get_item
 * 
 * @param enumtype enumeration to be queried
 * @param index number of the value to be queried in this enumeration
 * @param value variable value
 * @param name buffer to return the string containing the name of the enumeration item
 * @param name_len length of the string and/or buffer for name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value, char *name, int *name_len);
int PMPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value, char *name, int *name_len);


/*MPI_T_finalize*/

/**
 * @brief MPI function MPI_T_finalize
 * 
 *
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_finalize();
int PMPI_T_finalize();


/*MPI_T_init_thread*/

/**
 * @brief MPI function MPI_T_init_thread
 * 
 * @param required desired level of thread support
 * @param provided provided level of thread support
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_init_thread(int required, int *provided);
int PMPI_T_init_thread(int required, int *provided);


/*MPI_T_pvar_get_index*/

/**
 * @brief MPI function MPI_T_pvar_get_index
 * 
 * @param name the name of the performance variable
 * @param var_class the class of the performance variable
 * @param pvar_index the index of the performance variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index);
int PMPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index);


/*MPI_T_pvar_get_info*/

/**
 * @brief MPI function MPI_T_pvar_get_info
 * 
 * @param pvar_index index of the performance variable to be queried between $0$ and $num_pvar-1$
 * @param name buffer to return the string containing the name of the performance variable
 * @param name_len length of the string and/or buffer for name
 * @param verbosity verbosity level of this variable
 * @param var_class class of performance variable
 * @param datatype \mpi/ datatype of the information stored in the performance variable
 * @param enumtype optional descriptor for enumeration information
 * @param desc buffer to return the string containing a description of the performance variable
 * @param desc_len length of the string and/or buffer for desc
 * @param bind type of \mpi/ object to which this variable must be bound
 * @param readonly flag indicating whether the variable can be written/reset
 * @param continuous flag indicating whether the variable can be started and stopped or is continuously active
 * @param atomic flag indicating whether the variable can be atomically read and reset
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *readonly, int *continuous, int *atomic);
int PMPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *readonly, int *continuous, int *atomic);


/*MPI_T_pvar_get_num*/

/**
 * @brief MPI function MPI_T_pvar_get_num
 * 
 * @param num_pvar returns number of performance variables
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_get_num(int *num_pvar);
int PMPI_T_pvar_get_num(int *num_pvar);


/*MPI_T_pvar_handle_alloc*/

/**
 * @brief MPI function MPI_T_pvar_handle_alloc
 * 
 * @param session identifier of performance experiment session
 * @param pvar_index index of performance variable for which handle is to be allocated
 * @param obj_handle reference to a handle of the \mpi/ object to which this variable is supposed to be bound
 * @param handle allocated handle
 * @param count number of elements used to represent this variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle, MPI_T_pvar_handle *handle, int *count);
int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle, MPI_T_pvar_handle *handle, int *count);


/*MPI_T_pvar_handle_free*/

/**
 * @brief MPI function MPI_T_pvar_handle_free
 * 
 * @param session identifier of performance experiment session
 * @param handle handle to be freed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);
int PMPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);


/*MPI_T_pvar_read*/

/**
 * @brief MPI function MPI_T_pvar_read
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 * @param buf initial address of storage location for variable value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);


/*MPI_T_pvar_readreset*/

/**
 * @brief MPI function MPI_T_pvar_readreset
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 * @param buf initial address of storage location for variable value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int PMPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);


/*MPI_T_pvar_reset*/

/**
 * @brief MPI function MPI_T_pvar_reset
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle);


/*MPI_T_pvar_session_create*/

/**
 * @brief MPI function MPI_T_pvar_session_create
 * 
 * @param session identifier of performance session
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_session_create(MPI_T_pvar_session *session);
int PMPI_T_pvar_session_create(MPI_T_pvar_session *session);


/*MPI_T_pvar_session_free*/

/**
 * @brief MPI function MPI_T_pvar_session_free
 * 
 * @param session identifier of performance experiment session
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_session_free(MPI_T_pvar_session *session);
int PMPI_T_pvar_session_free(MPI_T_pvar_session *session);


/*MPI_T_pvar_start*/

/**
 * @brief MPI function MPI_T_pvar_start
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle);


/*MPI_T_pvar_stop*/

/**
 * @brief MPI function MPI_T_pvar_stop
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle);


/*MPI_T_pvar_write*/

/**
 * @brief MPI function MPI_T_pvar_write
 * 
 * @param session identifier of performance experiment session
 * @param handle handle of a performance variable
 * @param buf initial address of storage location for variable value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf);
int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf);


/*MPI_Test*/

/**
 * @brief MPI function MPI_Test
 * 
 * @param request 
 * @param flag true if operation completed
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status);


/*MPI_Test_cancelled*/

/**
 * @brief MPI function MPI_Test_cancelled
 * 
 * @param status 
 * @param flag 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Test_cancelled(const MPI_Status *status, int *flag);
int PMPI_Test_cancelled(const MPI_Status *status, int *flag);


/*MPI_Testall*/

/**
 * @brief MPI function MPI_Testall
 * 
 * @param count lists length
 * @param array_of_requests array of requests
 * @param flag 
 * @param array_of_statuses array of status objects
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]);
int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]);


/*MPI_Testany*/

/**
 * @brief MPI function MPI_Testany
 * 
 * @param count list length
 * @param array_of_requests array of requests
 * @param index index of operation that completed or MPI_UNDEFINED if none completed
 * @param flag true if one of the operations is complete
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status);
int PMPI_Testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status);


/*MPI_Testsome*/

/**
 * @brief MPI function MPI_Testsome
 * 
 * @param incount length of array_of_requests
 * @param array_of_requests array of requests
 * @param outcount number of completed requests
 * @param array_of_indices array of indices of operations that completed
 * @param array_of_statuses array of status objects for operations that completed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);


/*MPI_Topo_test*/

/**
 * @brief MPI function MPI_Topo_test
 * 
 * @param comm communicator
 * @param status topology type of communicator comm
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Topo_test(MPI_Comm comm, int *status);
int PMPI_Topo_test(MPI_Comm comm, int *status);


/*MPI_Type_commit*/

/**
 * @brief MPI function MPI_Type_commit
 * 
 * @param datatype datatype that is committed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_commit(MPI_Datatype *datatype);
int PMPI_Type_commit(MPI_Datatype *datatype);


/*MPI_Type_contiguous*/

/**
 * @brief MPI function MPI_Type_contiguous
 * 
 * @param count replication count
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_create_darray*/

/**
 * @brief MPI function MPI_Type_create_darray
 * 
 * @param size size of process group
 * @param rank rank in process group
 * @param ndims number of array dimensions as well as process grid dimensions
 * @param array_of_gsizes number of elements of type oldtype in each dimension of global array
 * @param array_of_distribs distribution of array in each dimension
 * @param array_of_dargs distribution argument in each dimension
 * @param array_of_psizes size of process grid in each dimension
 * @param order array storage order flag
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_create_f90_complex*/

/**
 * @brief MPI function MPI_Type_create_f90_complex
 * 
 * @param p precision, in decimal digits
 * @param r decimal exponent range
 * @param newtype the requested \MPI/ datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);
int PMPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);


/*MPI_Type_create_f90_integer*/

/**
 * @brief MPI function MPI_Type_create_f90_integer
 * 
 * @param r decimal exponent range, i.e., number of decimal digits
 * @param newtype the requested \MPI/ datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);
int PMPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);


/*MPI_Type_create_f90_real*/

/**
 * @brief MPI function MPI_Type_create_f90_real
 * 
 * @param p precision, in decimal digits
 * @param r decimal exponent range
 * @param newtype the requested \MPI/ datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);
int PMPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);


/*MPI_Type_create_hindexed*/

/**
 * @brief MPI function MPI_Type_create_hindexed
 * 
 * @param count number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements in each block
 * @param array_of_displacements byte displacement of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);

/*MPI_Type_hindexed (deprecated) */

/**
 * @brief MPI function MPI_Type_hindexed
 * 
 * @param count number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements in each block
 * @param array_of_displacements byte displacement of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_hindexed(int count,
                      const int array_of_blocklengths[],
                      const MPI_Aint array_of_displacements[],
                      MPI_Datatype oldtype, MPI_Datatype * newtype);
int PMPI_Type_hindexed(int count,
                       const int array_of_blocklengths[],
                       const MPI_Aint array_of_displacements[],
                       MPI_Datatype oldtype, MPI_Datatype * newtype);

/*MPI_Type_create_hindexed_block*/

/**
 * @brief MPI function MPI_Type_create_hindexed_block
 * 
 * @param count length of array of displacements
 * @param blocklength size of block
 * @param array_of_displacements byte displacement of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);

/*MPI_Type_hvector*/

/**
 * @brief MPI function MPI_Type_hvector
 * 
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of bytes between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);

/*MPI_Type_create_hvector*/

/**
 * @brief MPI function MPI_Type_create_hvector
 * 
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of bytes between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_create_indexed_block*/

/**
 * @brief MPI function MPI_Type_create_indexed_block
 * 
 * @param count length of array of displacements
 * @param blocklength size of block
 * @param array_of_displacements array of displacements
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_create_keyval*/

/**
 * @brief MPI function MPI_Type_create_keyval
 * 
 * @param type_copy_attr_fn copy callback function for type_keyval
 * @param type_delete_attr_fn delete callback function for type_keyval
 * @param type_keyval key value for future access
 * @param extra_state extra state for callback function
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state);
int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state);


/*MPI_Type_create_resized*/

/**
 * @brief MPI function MPI_Type_create_resized
 * 
 * @param oldtype input datatype
 * @param lb new lower bound of datatype
 * @param extent new extent of datatype
 * @param newtype output datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype);
int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype);


/*MPI_Type_create_struct*/

/**
 * @brief MPI function MPI_Type_create_struct
 * 
 * @param count number of blocks also number of entries in arrays array_of_types, array_of_displacements, and array_of_blocklengths
 * @param array_of_blocklengths number of elements in each block
 * @param array_of_displacements byte displacement of each block
 * @param array_of_types types of elements in each block
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_struct(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int PMPI_Type_create_struct(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);


/*PMPI_Type_lb (DEPRECATED)*/

/**
 * @brief MPI function MPI_Type_lb
 * 
 * @param datatype type to set LB on
 * @param value of LB
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement);
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement);

/*PMPI_Type_ub (DEPRECATED)) */

/**
 * @brief MPI function MPI_Type_ub
 * 
 * @param datatype type to set UB on
 * @param value of IB
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement);
int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement);

/*MPI_Type_struct (DEPRECATED)*/

/**
 * @brief MPI function MPI_Type_struct
 * 
 * @param count number of blocks also number of entries in arrays array_of_types, array_of_displacements, and array_of_blocklengths
 * @param array_of_blocklengths number of elements in each block
 * @param array_of_displacements byte displacement of each block
 * @param array_of_types types of elements in each block
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_struct(int count,
                   const int *array_of_blocklengths,
                   const MPI_Aint *array_of_displacements,
                   const MPI_Datatype *array_of_types,
                   MPI_Datatype *newtype);

int PMPI_Type_struct(int count,
                     const int *array_of_blocklengths,
                     const MPI_Aint *array_of_displacements,
                     const MPI_Datatype *array_of_types,
                     MPI_Datatype *newtype);

/*MPI_Type_create_subarray*/

/**
 * @brief MPI function MPI_Type_create_subarray
 * 
 * @param ndims number of array dimensions
 * @param array_of_sizes number of elements of type oldtype in each dimension of the full array
 * @param array_of_subsizes number of elements of type oldtype in each dimension of the subarray
 * @param array_of_starts starting coordinates of the subarray in each dimension
 * @param order array storage order flag
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_delete_attr*/

/**
 * @brief MPI function MPI_Type_delete_attr
 * 
 * @param datatype datatype from which the attribute is deleted
 * @param type_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval);
int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval);


/*MPI_Type_dup*/

/**
 * @brief MPI function MPI_Type_dup
 * 
 * @param oldtype datatype
 * @param newtype copy of oldtype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_free*/

/**
 * @brief MPI function MPI_Type_free
 * 
 * @param datatype datatype that is freed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_free(MPI_Datatype *datatype);
int PMPI_Type_free(MPI_Datatype *datatype);


/*MPI_Type_free_keyval*/

/**
 * @brief MPI function MPI_Type_free_keyval
 * 
 * @param type_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_free_keyval(int *type_keyval);
int PMPI_Type_free_keyval(int *type_keyval);


/*MPI_Type_get_attr*/

/**
 * @brief MPI function MPI_Type_get_attr
 * 
 * @param datatype datatype to which the attribute is attached
 * @param type_keyval key value
 * @param attribute_val attribute value, unless flag = false
 * @param flag false if no attribute is associated with the key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag);
int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag);


/*MPI_Type_get_contents*/

/**
 * @brief MPI function MPI_Type_get_contents
 * 
 * @param datatype datatype to access
 * @param max_integers number of elements in array_of_integers
 * @param max_addresses number of elements in array_of_addresses
 * @param max_datatypes number of elements in array_of_datatypes
 * @param array_of_integers contains integer arguments used in constructing datatype
 * @param array_of_addresses contains address arguments used in constructing datatype
 * @param array_of_datatypes contains datatype arguments used in constructing datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]);
int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]);


/* MPI_Get_elements_x */

/**
 * @brief MPI function MPI_Get_elements_x
 * 
 * @param status status to query
 * @param datatype type used to compute element size
 * @param elements number of type entry in the corresponding status
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *elements);
int PMPI_Get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *elements);

/* MPI_Get_elements */

/**
 * @brief MPI function MPI_Get_elements
 * 
 * @param status status to query
 * @param datatype type used to compute element size
 * @param elements number of type entry in the corresponding status
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements);
int PMPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements);

/*MPI_Type_get_elements*/

/**
 * @brief MPI function MPI_Type_get_elements
 * 
 * @param status return status of receive operation
 * @param datatype datatype used by receive operation
 * @param count number of received basic elements
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_elements(MPI_Status *status, MPI_Datatype datatype, int *count);
int PMPI_Type_get_elements(MPI_Status *status, MPI_Datatype datatype, int *count);


/*MPI_Type_get_elements_x*/

/**
 * @brief MPI function MPI_Type_get_elements_x
 * 
 * @param status return status of receive operation
 * @param datatype datatype used by receive operation
 * @param count number of received basic elements
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int PMPI_Type_get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);


/*MPI_Type_get_envelope*/

/**
 * @brief MPI function MPI_Type_get_envelope
 * 
 * @param datatype datatype to access
 * @param num_integers number of input integers used in call constructing combiner
 * @param num_addresses number of input addresses used in call constructing combiner
 * @param num_datatypes number of input datatypes used in call constructing combiner
 * @param combiner combiner
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);
int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);


/*MPI_Type_get_extent*/

/**
 * @brief MPI function MPI_Type_get_extent
 * 
 * @param datatype datatype to get information on
 * @param lb lower bound of datatype
 * @param extent extent of datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);


/*MPI_Type_extent (DEPRECATED)*/

/**
 * @brief MPI function MPI_Type_extent
 * 
 * @param datatype datatype to get information on
 * @param extent extent of datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent);
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent);

/*MPI_Type_get_extent_x*/

/**
 * @brief MPI function MPI_Type_get_extent_x
 * 
 * @param datatype datatype to get information on
 * @param lb lower bound of datatype
 * @param extent extent of datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);


/*MPI_Type_get_name*/

/**
 * @brief MPI function MPI_Type_get_name
 * 
 * @param datatype datatype whose name is to be returned
 * @param type_name the name previously stored on the datatype, or an empty string if no such name exists
 * @param resultlen length of returned name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen);
int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen);


/*MPI_Type_get_true_extent*/

/**
 * @brief MPI function MPI_Type_get_true_extent
 * 
 * @param datatype datatype to get information on
 * @param true_lb true lower bound of datatype
 * @param true_extent true size of datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);
int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);


/*MPI_Type_get_true_extent_x*/

/**
 * @brief MPI function MPI_Type_get_true_extent_x
 * 
 * @param datatype datatype to get information on
 * @param true_lb true lower bound of datatype
 * @param true_extent true size of datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);


/*MPI_Type_indexed*/

/**
 * @brief MPI function MPI_Type_indexed
 * 
 * @param count number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements per block
 * @param array_of_displacements displacement for each block, in multiples of oldtype
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);


/*MPI_Type_match_size*/

/**
 * @brief MPI function MPI_Type_match_size
 * 
 * @param typeclass generic type specifier
 * @param size size, in bytes, of representation
 * @param datatype datatype with correct type, size
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype);
int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype);


/*MPI_Type_set_attr*/

/**
 * @brief MPI function MPI_Type_set_attr
 * 
 * @param datatype datatype to which attribute will be attached
 * @param type_keyval key value
 * @param attribute_val attribute value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val);


/*MPI_Type_set_name*/

/**
 * @brief MPI function MPI_Type_set_name
 * 
 * @param datatype datatype whose identifier is to be set
 * @param type_name the character string which is remembered as the name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_set_name(MPI_Datatype datatype, const char *type_name);
int PMPI_Type_set_name(MPI_Datatype datatype, const char *type_name);


/*MPI_Type_size*/

/**
 * @brief MPI function MPI_Type_size
 * 
 * @param datatype datatype
 * @param size datatype size
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_size(MPI_Datatype datatype, int *size);
int PMPI_Type_size(MPI_Datatype datatype, int *size);


/*MPI_Type_size_x*/

/**
 * @brief MPI function MPI_Type_size_x
 * 
 * @param datatype datatype
 * @param size datatype size
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size);
int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size);


/*MPI_Type_vector*/

/**
 * @brief MPI function MPI_Type_vector
 * 
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of elements between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);

/*MPI_Type_hvector*/

/**
 * @brief MPI function MPI_Type_hvector
 * 
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of elements between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */

int MPI_Type_hvector( 
        int count, 
        int blocklen, 
        MPI_Aint stride, 
        MPI_Datatype old_type, 
        MPI_Datatype *newtype );
int PMPI_Type_hvector( 
        int count, 
        int blocklen, 
        MPI_Aint stride, 
        MPI_Datatype old_type, 
        MPI_Datatype *newtype );

/*MPI_Unpack*/

/**
 * @brief MPI function MPI_Unpack
 * 
 * @param inbuf input buffer start
 * @param insize size of input buffer, in bytes
 * @param position current position in bytes
 * @param outbuf output buffer start
 * @param outcount number of items to be unpacked
 * @param datatype datatype of each output data item
 * @param comm communicator for packed message
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm);
int PMPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm);


/*MPI_Unpack_external*/

/**
 * @brief MPI function MPI_Unpack_external
 * 
 * @param datarep data representation
 * @param inbuf input buffer start
 * @param insize input buffer size, in bytes
 * @param position current position in buffer, in bytes
 * @param outbuf output buffer start
 * @param outcount number of output data items
 * @param datatype datatype of output data item
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);
int PMPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);


/*MPI_Unpublish_name*/

/**
 * @brief MPI function MPI_Unpublish_name
 * 
 * @param service_name a service name
 * @param info implementation-specific information
 * @param port_name a port name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name);
int PMPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name);


/*MPI_Wait*/

/**
 * @brief MPI function MPI_Wait
 * 
 * @param request request
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int PMPI_Wait(MPI_Request *request, MPI_Status *status);


/*MPI_Waitall*/

/**
 * @brief MPI function MPI_Waitall
 * 
 * @param count lists length
 * @param array_of_requests array of requests
 * @param array_of_statuses array of status objects
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);
int PMPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);


/*MPI_Waitany*/

/**
 * @brief MPI function MPI_Waitany
 * 
 * @param count list length
 * @param array_of_requests array of requests
 * @param index index of handle for operation that completed
 * @param status 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status);
int PMPI_Waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status);


/*MPI_Waitsome*/

/**
 * @brief MPI function MPI_Waitsome
 * 
 * @param incount length of array_of_requests
 * @param array_of_requests array of requests
 * @param outcount number of completed requests
 * @param array_of_indices array of indices of operations that completed
 * @param array_of_statuses array of status objects for operations that completed
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
int PMPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);


/*MPI_Win_allocate*/

/**
 * @brief MPI function MPI_Win_allocate
 * 
 * @param size size of window in bytes
 * @param disp_unit local unit size for displacements, in bytes
 * @param info 
 * @param comm intra-communicator
 * @param baseptr initial address of window
 * @param win window object returned by call
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);


/*MPI_Win_allocate_shared*/

/**
 * @brief MPI function MPI_Win_allocate_shared
 * 
 * @param size size of local window in bytes
 * @param disp_unit local unit size for displacements, in bytes
 * @param info 
 * @param comm intra-communicator
 * @param baseptr address of local allocated window segment
 * @param win window object returned by the call
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);


/*MPI_Win_attach*/

/**
 * @brief MPI function MPI_Win_attach
 * 
 * @param win 
 * @param base initial address of memory to be attached
 * @param size size of memory to be attached in bytes
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);
int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);


/*MPI_Win_call_errhandler*/

/**
 * @brief MPI function MPI_Win_call_errhandler
 * 
 * @param win window with error handler
 * @param errorcode 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_call_errhandler(MPI_Win win, int errorcode);
int PMPI_Win_call_errhandler(MPI_Win win, int errorcode);


/*MPI_Win_complete*/

/**
 * @brief MPI function MPI_Win_complete
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_complete(MPI_Win win);
int PMPI_Win_complete(MPI_Win win);


/*MPI_Win_create*/

/**
 * @brief MPI function MPI_Win_create
 * 
 * @param base initial address of window
 * @param size size of window in bytes
 * @param disp_unit local unit size for displacements, in bytes
 * @param info 
 * @param comm intra-communicator
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);


/*MPI_Win_create_dynamic*/

/**
 * @brief MPI function MPI_Win_create_dynamic
 * 
 * @param info 
 * @param comm intra-communicator
 * @param win window object returned by the call
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);
int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);


/*MPI_Win_create_errhandler*/

/**
 * @brief MPI function MPI_Win_create_errhandler
 * 
 * @param win_errhandler_fn user defined error handling procedure
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Errhandler *errhandler);


/*MPI_Win_create_keyval*/

/**
 * @brief MPI function MPI_Win_create_keyval
 * 
 * @param win_copy_attr_fn copy callback function for win_keyval
 * @param win_delete_attr_fn delete callback function for win_keyval
 * @param win_keyval key value for future access
 * @param extra_state extra state for callback function
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state);
int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state);


/*MPI_Win_delete_attr*/

/**
 * @brief MPI function MPI_Win_delete_attr
 * 
 * @param win window from which the attribute is deleted
 * @param win_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_delete_attr(MPI_Win win, int win_keyval);
int PMPI_Win_delete_attr(MPI_Win win, int win_keyval);


/*MPI_Win_detach*/

/**
 * @brief MPI function MPI_Win_detach
 * 
 * @param win 
 * @param base initial address of memory to be detached
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_detach(MPI_Win win, const void *base);
int PMPI_Win_detach(MPI_Win win, const void *base);


/*MPI_Win_fence*/

/**
 * @brief MPI function MPI_Win_fence
 * 
 * @param assert 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_fence(int assert, MPI_Win win);
int PMPI_Win_fence(int assert, MPI_Win win);


/*MPI_Win_flush*/

/**
 * @brief MPI function MPI_Win_flush
 * 
 * @param rank rank of target window
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_flush(int rank, MPI_Win win);
int PMPI_Win_flush(int rank, MPI_Win win);


/*MPI_Win_flush_all*/

/**
 * @brief MPI function MPI_Win_flush_all
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_flush_all(MPI_Win win);
int PMPI_Win_flush_all(MPI_Win win);


/*MPI_Win_flush_local*/

/**
 * @brief MPI function MPI_Win_flush_local
 * 
 * @param rank rank of target window
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_flush_local(int rank, MPI_Win win);
int PMPI_Win_flush_local(int rank, MPI_Win win);


/*MPI_Win_flush_local_all*/

/**
 * @brief MPI function MPI_Win_flush_local_all
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_flush_local_all(MPI_Win win);
int PMPI_Win_flush_local_all(MPI_Win win);


/*MPI_Win_free*/

/**
 * @brief MPI function MPI_Win_free
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_free(MPI_Win *win);
int PMPI_Win_free(MPI_Win *win);


/*MPI_Win_free_keyval*/

/**
 * @brief MPI function MPI_Win_free_keyval
 * 
 * @param win_keyval key value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_free_keyval(int *win_keyval);
int PMPI_Win_free_keyval(int *win_keyval);


/*MPI_Win_get_attr*/

/**
 * @brief MPI function MPI_Win_get_attr
 * 
 * @param win window to which the attribute is attached
 * @param win_keyval key value
 * @param attribute_val attribute value, unless flag = false
 * @param flag false if no attribute is associated with the key
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);


/*MPI_Win_get_errhandler*/

/**
 * @brief MPI function MPI_Win_get_errhandler
 * 
 * @param win 
 * @param errhandler error handler currently associated with window
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);


/*MPI_Win_get_group*/

/**
 * @brief MPI function MPI_Win_get_group
 * 
 * @param win 
 * @param group group of processes which share access to the window
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_get_group(MPI_Win win, MPI_Group *group);
int PMPI_Win_get_group(MPI_Win win, MPI_Group *group);


/*MPI_Win_get_info*/

/**
 * @brief MPI function MPI_Win_get_info
 * 
 * @param win 
 * @param info_used new info object
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used);
int PMPI_Win_get_info(MPI_Win win, MPI_Info *info_used);


/*MPI_Win_get_name*/

/**
 * @brief MPI function MPI_Win_get_name
 * 
 * @param win window whose name is to be returned
 * @param win_name the name previously stored on the window, or an empty string if no such name exists
 * @param resultlen length of returned name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);
int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);


/*MPI_Win_lock*/

/**
 * @brief MPI function MPI_Win_lock
 * 
 * @param lock_type either MPI_LOCK_EXCLUSIVE or MPI_LOCK_SHARED
 * @param rank rank of locked window
 * @param assert 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);
int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);


/*MPI_Win_lock_all*/

/**
 * @brief MPI function MPI_Win_lock_all
 * 
 * @param assert 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_lock_all(int assert, MPI_Win win);
int PMPI_Win_lock_all(int assert, MPI_Win win);


/*MPI_Win_post*/

/**
 * @brief MPI function MPI_Win_post
 * 
 * @param group group of origin processes
 * @param assert 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win);
int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win);


/*MPI_Win_set_attr*/

/**
 * @brief MPI function MPI_Win_set_attr
 * 
 * @param win window to which attribute will be attached
 * @param win_keyval key value
 * @param attribute_val attribute value
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);


/*MPI_Win_set_errhandler*/

/**
 * @brief MPI function MPI_Win_set_errhandler
 * 
 * @param win 
 * @param errhandler new error handler for window
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);
int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);


/*MPI_Win_set_info*/

/**
 * @brief MPI function MPI_Win_set_info
 * 
 * @param win 
 * @param info 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_set_info(MPI_Win win, MPI_Info info);
int PMPI_Win_set_info(MPI_Win win, MPI_Info info);


/*MPI_Win_set_name*/

/**
 * @brief MPI function MPI_Win_set_name
 * 
 * @param win window whose identifier is to be set
 * @param win_name the character string which is remembered as the name
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_set_name(MPI_Win win, const char *win_name);
int PMPI_Win_set_name(MPI_Win win, const char *win_name);


/*MPI_Win_shared_query*/

/**
 * @brief MPI function MPI_Win_shared_query
 * 
 * @param win shared memory window object
 * @param rank rank in the group of window win or MPI_PROC_NULL
 * @param size size of the window segment
 * @param disp_unit local unit size for displacements, in bytes
 * @param baseptr address for load/store access to window segment
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);
int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);


/*MPI_Win_start*/

/**
 * @brief MPI function MPI_Win_start
 * 
 * @param group group of target processes
 * @param assert 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_start(MPI_Group group, int assert, MPI_Win win);
int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win);


/*MPI_Win_sync*/

/**
 * @brief MPI function MPI_Win_sync
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_sync(MPI_Win win);
int PMPI_Win_sync(MPI_Win win);


/*MPI_Win_test*/

/**
 * @brief MPI function MPI_Win_test
 * 
 * @param win 
 * @param flag success flag
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_test(MPI_Win win, int *flag);
int PMPI_Win_test(MPI_Win win, int *flag);


/*MPI_Win_unlock*/

/**
 * @brief MPI function MPI_Win_unlock
 * 
 * @param rank rank of window
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_unlock(int rank, MPI_Win win);
int PMPI_Win_unlock(int rank, MPI_Win win);


/*MPI_Win_unlock_all*/

/**
 * @brief MPI function MPI_Win_unlock_all
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_unlock_all(MPI_Win win);
int PMPI_Win_unlock_all(MPI_Win win);


/*MPI_Win_wait*/

/**
 * @brief MPI function MPI_Win_wait
 * 
 * @param win 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_Win_wait(MPI_Win win);
int PMPI_Win_wait(MPI_Win win);


/*MPI_Wtick*/

/**
 * @brief MPI function MPI_Wtick
 * 
 *
 *
 * @return double 
 
 */
double MPI_Wtick();
double PMPI_Wtick();


/*MPI_Wtime*/

/**
 * @brief MPI function MPI_Wtime
 * 
 *
 *
 * @return double 
 
 */
double MPI_Wtime();
double PMPI_Wtime();


/*MPI_File_call_errhandler*/

/**
 * @brief MPI function MPI_File_call_errhandler
 * 
 * @param fh file with error handler
 * @param errorcode 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_File_call_errhandler(MPI_File fh, int errorcode);
int PMPI_File_call_errhandler(MPI_File fh, int errorcode);


/*MPI_File_create_errhandler*/

/**
 * @brief MPI function MPI_File_create_errhandler
 * 
 * @param file_errhandler_fn user defined error handling procedure
 * @param errhandler 
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler);


/*MPI_File_get_errhandler*/

/**
 * @brief MPI function MPI_File_get_errhandler
 * 
 * @param file 
 * @param errhandler error handler currently associated with file
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler);
int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler);


/*MPI_File_set_errhandler*/

/**
 * @brief MPI function MPI_File_set_errhandler
 * 
 * @param file 
 * @param errhandler new error handler for file
 *
 * @return int MPI_SUCCESS on success other MPI_* error code otherwise 
 */
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler);
int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler);


/******************************
 * EXTENSIONS TO THE STANDARD *
 ******************************/

/* Checkpointing */
int MPIX_Checkpoint(MPIX_Checkpoint_state* state);
int PMPIX_Checkpoint(MPIX_Checkpoint_state* state);

/* Extended Generalized Requests */
int MPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn, 
			MPIX_Grequest_poll_fn * poll_fn, 
			void *extra_state, 
			MPI_Request * request);
int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
			MPI_Grequest_free_function * free_fn,
			MPI_Grequest_cancel_function * cancel_fn, 
			MPIX_Grequest_poll_fn * poll_fn, 
			void *extra_state, 
			MPI_Request * request);

/* Extended Generalized Request Class */
int MPIX_Grequest_class_create( MPI_Grequest_query_function * query_fn,
				MPI_Grequest_free_function * free_fn,
				MPI_Grequest_cancel_function * cancel_fn,
				MPIX_Grequest_poll_fn * poll_fn,
				MPIX_Grequest_wait_fn * wait_fn,
				MPIX_Grequest_class * new_class );
int PMPIX_Grequest_class_create( MPI_Grequest_query_function * query_fn,
				MPI_Grequest_free_function * free_fn,
				MPI_Grequest_cancel_function * cancel_fn,
				MPIX_Grequest_poll_fn * poll_fn,
				MPIX_Grequest_wait_fn * wait_fn,
				MPIX_Grequest_class * new_class );

int MPIX_Grequest_class_allocate( MPIX_Grequest_class  target_class, void *extra_state, MPI_Request *request );
int PMPIX_Grequest_class_allocate( MPIX_Grequest_class  target_class, void *extra_state, MPI_Request *request );

/* Halo Cells */

int MPIX_Swap(void **sendrecv_buf , int remote_rank, MPI_Count size , MPI_Comm comm);
int PMPIX_Swap(void **sendrecv_buf , int remote_rank, MPI_Count size , MPI_Comm comm);

int MPIX_Exchange(void **send_buf , void **recvbuff, int remote_rank, MPI_Count size , MPI_Comm comm);
int PMPIX_Exchange(void **send_buf , void **recvbuff, int remote_rank, MPI_Count size , MPI_Comm comm);


int MPIX_Halo_cell_init( MPI_Halo * halo, char * label, MPI_Datatype type, int count );
int PMPIX_Halo_cell_init( MPI_Halo * halo, char * label, MPI_Datatype type, int count );

int MPIX_Halo_cell_release( MPI_Halo * halo );
int PMPIX_Halo_cell_release( MPI_Halo * halo );


int MPIX_Halo_cell_set( MPI_Halo halo, void * ptr );
int PMPIX_Halo_cell_set( MPI_Halo halo, void * ptr );


int MPIX_Halo_cell_get( MPI_Halo halo, void ** ptr );
int PMPIX_Halo_cell_get( MPI_Halo halo, void ** ptr );

int MPIX_Halo_exchange_init( MPI_Halo_exchange * ex );
int PMPIX_Halo_exchange_init( MPI_Halo_exchange * ex );

int MPIX_Halo_exchange_release( MPI_Halo_exchange * ex );
int PMPIX_Halo_exchange_release( MPI_Halo_exchange * ex );

int MPIX_Halo_exchange_commit( MPI_Halo_exchange ex );
int PMPIX_Halo_exchange_commit( MPI_Halo_exchange ex );

int MPIX_Halo_exchange( MPI_Halo_exchange ex );
int PMPIX_Halo_exchange( MPI_Halo_exchange ex );

int MPIX_Halo_iexchange( MPI_Halo_exchange ex );
int PMPIX_Halo_iexchange( MPI_Halo_exchange ex );

int MPIX_Halo_iexchange_wait( MPI_Halo_exchange ex );
int PMPIX_Halo_iexchange_wait( MPI_Halo_exchange ex );

int MPIX_Halo_cell_bind_local( MPI_Halo_exchange ex, MPI_Halo halo );
int PMPIX_Halo_cell_bind_local( MPI_Halo_exchange ex, MPI_Halo halo );

int MPIX_Halo_cell_bind_remote( MPI_Halo_exchange ex, MPI_Halo halo, int remote, char * remote_label );
int PMPIX_Halo_cell_bind_remote( MPI_Halo_exchange ex, MPI_Halo halo, int remote, char * remote_label );

/* ULFM agreement FT */

int MPIX_Comm_failure_ack( MPI_Comm  );
int PMPIX_Comm_failure_ack( MPI_Comm  );

int MPIX_Comm_failure_get_acked( MPI_Comm , MPI_Group * );
int PMPIX_Comm_failure_get_acked( MPI_Comm , MPI_Group * );

int MPIX_Comm_agree(MPI_Comm , int *);
int PMPIX_Comm_agree(MPI_Comm , int *);

int MPIX_Comm_revoke(MPI_Comm );
int PMPIX_Comm_revoke(MPI_Comm );

int MPIX_Comm_shrink(MPI_Comm , MPI_Comm *);
int PMPIX_Comm_shrink(MPI_Comm , MPI_Comm *);

/********************
 * OTHER INTERFACES *
 ********************/

#define MPICH_ATTR_POINTER_WITH_TYPE_TAG(a,b) 
#define MPI_AINT_FMT_HEX_SPEC "%X"

#ifdef MPC_MPIIO_ENABLED
#include <mpio.h>
#endif

#ifdef MPC_FORTRAN_ENABLED
#include <mpc_mpi_fortran.h>
#endif


#ifdef __cplusplus
}
#endif
#endif /* MPC_MPI_H_ */
