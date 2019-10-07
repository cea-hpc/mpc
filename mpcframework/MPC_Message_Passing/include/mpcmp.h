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
#ifndef __mpc__H
#define __mpc__H

#ifdef __cplusplus
extern "C"
{
#endif

/* #include "mpcthread.h" */
/* #include "mpcalloc.h" */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sctk_types.h>

#if defined(MPC_Fault_Tolerance) || defined(MPC_MODULE_MPC_Fault_Tolerance)
#include "sctk_ft_types.h"
#endif

#define MPC_THREAD_SINGLE 0
#define MPC_THREAD_FUNNELED 1
#define MPC_THREAD_SERIALIZED 2
#define  MPC_THREAD_MULTIPLE 3

#define SCTK_COMMON_DATA_TYPE_COUNT 70
#define SCTK_USER_DATA_TYPES_MAX 4000
#define SCTK_DERIVED_DATATYPE_BASE (SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX)

/** \brief Macro to obtain the total number of datatypes */
#define MPC_TYPE_COUNT                                                         \
  (SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX)

  typedef int MPC_Message;

  typedef mpc_mp_msg_count_t mpc_mp_msg_count_t;

  typedef unsigned int mpc_pack_indexes_t;
  typedef long mpc_pack_absolute_indexes_t;

  typedef struct {
    int task_nb;
    /* Task list rank are valid in COMM_WORLD  */
    int *task_list_in_global_ranks;
} _mpc_m_group_t;

extern const _mpc_m_group_t mpc_group_empty;
extern const _mpc_m_group_t mpc_group_null;

#define MPC_GROUP_EMPTY &mpc_group_empty
#define MPC_GROUP_NULL ((_mpc_m_group_t *)NULL)

/* #define MPC_STATUS_SIZE 80 */
/* #define MPC_REQUEST_SIZE 30 */


typedef void(MPC_Handler_function)(void *, int *, ...);
typedef int MPC_Errhandler;


struct sctk_thread_ptp_message_s;


/** Generalized requests functions **/
typedef sctk_Grequest_query_function MPC_Grequest_query_function;
typedef sctk_Grequest_cancel_function MPC_Grequest_cancel_function;
typedef sctk_Grequest_free_function MPC_Grequest_free_function;
/** Extended Generalized requests functions **/
typedef sctk_Grequest_poll_fn MPCX_Grequest_poll_fn;
typedef sctk_Grequest_wait_fn MPCX_Grequest_wait_fn;
/** Generalized Request classes **/
typedef sctk_Request_class MPCX_Request_class;

/** MPC Requests */
extern mpc_mp_request_t mpc_request_null;

#define MPC_REQUEST_NULL mpc_request_null

#define MPC_COMM_WORLD SCTK_COMM_WORLD
#define MPC_COMM_SELF SCTK_COMM_SELF
#define MPC_COMM_NULL SCTK_COMM_NULL
#define MPC_SUCCESS SCTK_SUCCESS

#define MPC_UNDEFINED (-1)
/* Communication argument parameters */
#define MPC_ERR_BUFFER                  1  /* Invalid buffer pointer */
#define MPC_ERR_COUNT                   2  /* Invalid count argument */
#define MPC_ERR_TYPE                    3  /* Invalid datatype argument */
#define MPC_ERR_TAG                     4  /* Invalid tag argument */
#define MPC_ERR_COMM                    5  /* Invalid communicator */
#define MPC_ERR_RANK                    6  /* Invalid rank */
#define MPC_ERR_ROOT                    8  /* Invalid root */
#define MPC_ERR_TRUNCATE               15  /* Message truncated on receive */
#define MPC_ERR_NOT_SAME               39  /* Collective argument not identical on all processess,
                                              or collective routines called in a different order
																							by different processes */

/* MPC Objects (other than COMM) */
#define MPC_ERR_GROUP                   9  /* Invalid group */
#define MPC_ERR_OP                     10  /* Invalid operation */
#define MPC_ERR_REQUEST                 7  /* Invalid mpc_request handle */

/* Special topology argument parameters */
#define MPC_ERR_TOPOLOGY               11  /* Invalid topology */
#define MPC_ERR_DIMS                   12	 /* Invalid dimension argument */

/* All other arguments.  This is a class with many kinds */
#define MPC_ERR_ARG                    13  /* Invalid argument */
#define MPC_ERR_PORT                   27  /* Invalid port name passed to MPI_COMM_CONNECT */
#define MPC_ERR_SERVICE                28  /* Invalid service name passed to MPI_UNPUBLISH_NAME */
#define MPC_ERR_NAME                   29  /* Invalid service name passed to MPI_LOOKUP_NAME */
#define MPC_ERR_WIN                    30  /* Invalid win argument */
#define MPC_ERR_SIZE                   31  /* Invalid size argument */
#define MPC_ERR_DISP                   32  /* Invalid disp argument */
#define MPC_ERR_INFO                   33  /* Invalid info argument */
#define MPC_ERR_LOCKTYPE               34  /* Invalid locktype argument */
#define MPC_ERR_ASSERT                 35  /* Invalid assert argument */

/* Error on File handling*/
#define MPC_ERR_FILE                   38  /* Invalid file handle */
#define MPC_ERR_AMODE                  40  /* Error related to the amode passed to MPI_FILE_OPEN */
#define MPC_ERR_UNSUPPORTED_DATAREP    41  /* Unsupported datarep passed to MPI_FILE_SET_VIEW */
#define MPC_ERR_NO_SUCH_FILE           43  /* File does not exist */
#define MPC_ERR_FILE_EXISTS            44  /* File exists */
#define MPC_ERR_BAD_FILE               45  /* Invalid file name (e.g., path name too long) */
#define MPC_ERR_ACCESS                 46  /* Permission denied */
#define MPC_ERR_READ_ONLY              49  /* Read-only file or file system */
#define MPC_ERR_FILE_IN_USE            50  /* File operation could not completed, as the file is
                                              currently open by some process */
#define MPC_ERR_IO                     53  /* Other I/O error */

/* Multiple completion has two special error classes */
#define MPC_ERR_IN_STATUS              18  /* Look in status for error value */
#define MPC_ERR_PENDING                19  /* Pending request */

/* Error with comm attributes (keyval) */
#define MPC_ERR_KEYVAL	               20  /* Invalid keyval has been passed */
#define MPC_ERR_INFO_KEY               23  /* Key onger than MPI_MAX_INFO_KEY */
#define MPC_ERR_INFO_VALUE             24  /* Value longer than MPI_MAX_INFO_VAL */
#define MPC_ERR_INFO_NOKEY             25  /* Invalid key passed to MPI_INFO_DELETE */

/* Error with memory */
#define MPC_ERR_NO_MEM 		             21  /* MPI_ALLOC_MEM failed because memory is exhausted */
#define MPC_ERR_BASE                   22  /* Invalid base passed to MPI_FREE_MEM */

/* Other errors that are not simply an invalid argument */
#define MPC_ERR_SPAWN                  26  /* Error in spawning processes */
#define MPC_ERR_RMA_CONFLICT               36  /* Conflicting accesses to window */
#define MPC_ERR_RMA_SYNC                   37  /* Wrong synchronization of RMA calls */
#define MPC_ERR_UNSUPPORTED_OPERATION  42  /* Unsupported operation, such as seeking on a file which
                                              supports sequential access only*/
#define MPC_ERR_NO_SPACE               47  /* Not enough space */
#define MPC_ERR_QUOTA                  48  /* Quota exceeded */
#define MPC_ERR_DUP_DATAREP            51  /* Conversion functions could not be regiestered because a
																							data representation identifier that was already defined
																							was passed to MPI_REGISTER_DATAREP */
#define MPC_ERR_CONVERSION             52  /* An error occured in a user supplied data conversion function */
#define MPC_ERR_OTHER                  16  /* Other error; use Error_string */

#define MPC_ERR_UNKNOWN                14  /* Unknown error */
#define MPC_ERR_INTERN                 17  /* Internal error code    */

#define MPCR_ERRORS_THROW_EXCEPTIONS   55

#define MPC_NOT_IMPLEMENTED            56

#define MPC_ERR_RMA_FLAVOR 57 /* Wrong type of RMA window */

/* HOLE @ 58 */

#define MPC_T_ERR_MEMORY 59          /* Out of memory */
#define MPC_T_ERR_NOT_INITIALIZED 60 /* Interface not initialized */
#define MPC_T_ERR_CANNOT_INIT                                                  \
  61 /* Interface not in the state to                                          \
        be initialized */
#define MPC_T_ERR_INVALID_INDEX                                                \
  62                                  /* The index is invalid or               \
                                         has been deleted  */
#define MPC_T_ERR_INVALID_ITEM 63     /* Item index queried is out of range */
#define MPC_T_ERR_INVALID_HANDLE 64   /* The handle is invalid */
#define MPC_T_ERR_OUT_OF_HANDLES 65   /* No more handles available */
#define MPC_T_ERR_OUT_OF_SESSIONS 66  /* No more sessions available */
#define MPC_T_ERR_INVALID_SESSION 67  /* Session argument is not valid */
#define MPC_T_ERR_CVAR_SET_NOT_NOW 68 /* Cvar can't be set at this moment */
#define MPC_T_ERR_CVAR_SET_NEVER                                               \
  69                                   /* Cvar can't be set until              \
                                           end of execution */
#define MPC_T_ERR_PVAR_NO_STARTSTOP 70 /* Pvar can't be started or stopped */
#define MPC_T_ERR_PVAR_NO_WRITE 71     /* Pvar can't be written or reset */
#define MPC_T_ERR_PVAR_NO_ATOMIC 72    /* Pvar can't be R/W atomically */
#define MPC_T_ERR_INVALID_NAME 73      /* Requested name is invalid */

#define MPC_ERR_RMA_RANGE 74
#define MPC_ERR_RMA_ATTACH 75
#define MPC_ERR_RMA_SHARED 76

#define MPC_ERR_LASTCODE 77

#define MPC_STATUS_IGNORE SCTK_STATUS_NULL
#define MPC_STATUSES_IGNORE NULL
#define MPC_ANY_TAG SCTK_ANY_TAG
#define MPC_ANY_SOURCE SCTK_ANY_SOURCE
#define MPC_PROC_NULL SCTK_PROC_NULL
#define MPC_MAX_PROCESSOR_NAME 255
#define MPC_ROOT -4
#define MPC_MAX_OBJECT_NAME 256
#define MPC_MAX_LIBRARY_VERSION_STRING 8192

/* Type Keyval Defines */

#define MPC_KEYVAL_INVALID -1

#define MPC_ERRHANDLER_NULL -1
#define MPC_ERRORS_RETURN -6
#define MPC_ERRORS_ARE_FATAL -7
#define MPC_TYPE_NULL_DELETE_FN (NULL)
#define MPC_TYPE_NULL_COPY_FN (NULL)
#define MPC_TYPE_NULL_DUP_FN ((void *)0x3)

/********************************************************************/
/*Special TAGS                                                      */
/********************************************************************/
#define MPC_GATHERV_TAG -2
#define MPC_GATHER_TAG -3
#define MPC_SCATTERV_TAG -4
#define MPC_SCATTER_TAG -5
#define MPC_ALLTOALL_TAG -6
#define MPC_ALLTOALLV_TAG -7
#define MPC_ALLTOALLW_TAG -8
#define MPC_BROADCAST_TAG -9
#define MPC_BARRIER_TAG -10
#define MPC_ALLGATHER_TAG -11
#define MPC_ALLGATHERV_TAG -12
#define MPC_REDUCE_TAG -13
#define MPC_ALLREDUCE_TAG -14
#define MPC_REDUCE_SCATTER_TAG -15
#define MPC_REDUCE_SCATTER_BLOCK_TAG -16
#define MPC_SCAN_TAG -17
#define MPC_EXSCAN_TAG -18
#define MPC_IBARRIER_TAG -19
#define MPC_IBCAST_TAG -20
#define MPC_IGATHER_TAG -21
#define MPC_IGATHERV_TAG -22
#define MPC_ISCATTER_TAG -23
#define MPC_ISCATTERV_TAG -24
#define MPC_IALLGATHER_TAG -25
#define MPC_IALLGATHERV_TAG -26
#define MPC_IALLTOALL_TAG -27
#define MPC_IALLTOALLV_TAG -28
#define MPC_IALLTOALLW_TAG -29
#define MPC_IREDUCE_TAG -30
#define MPC_IALLREDUCE_TAG -31
#define MPC_IREDUCE_SCATTER_TAG -32
#define MPC_IREDUCE_SCATTER_BLOCK_TAG -33
#define MPC_ISCAN_TAG -34
#define MPC_IEXSCAN_TAG -35
#define MPC_COPY_TAG -36
/* TAG of minimum value */
#define MPC_LAST_TAG -37



/* Here we define the type of an MPC_Info as an int
 * this type is alliased to MPI_Info at mpc_mpi.h:224
 */
typedef int MPC_Info;

#define MPC_CREATE_INTERN_FUNC(name) extern  const sctk_Op MPC_##name

MPC_CREATE_INTERN_FUNC (SUM);
MPC_CREATE_INTERN_FUNC (MAX);
MPC_CREATE_INTERN_FUNC (MIN);
MPC_CREATE_INTERN_FUNC (PROD);
MPC_CREATE_INTERN_FUNC (LAND);
MPC_CREATE_INTERN_FUNC (BAND);
MPC_CREATE_INTERN_FUNC (LOR);
MPC_CREATE_INTERN_FUNC (BOR);
MPC_CREATE_INTERN_FUNC (LXOR);
MPC_CREATE_INTERN_FUNC (BXOR);
MPC_CREATE_INTERN_FUNC (MINLOC);
MPC_CREATE_INTERN_FUNC (MAXLOC);

    /*TYPES*/
#define MPC_DATATYPE_NULL ((mpc_mp_datatype_t)-1)
#define MPC_UB ((mpc_mp_datatype_t)-2)
#define MPC_LB ((mpc_mp_datatype_t)-3)
#define MPC_PACKED 0
#define MPC_BYTE 1
#define MPC_SHORT 2
#define MPC_INT 3
#define MPC_LONG 4
#define MPC_FLOAT 5
#define MPC_DOUBLE 6
#define MPC_UNSIGNED_CHAR  7
#define MPC_UNSIGNED_SHORT 8
#define MPC_UNSIGNED  9
#define MPC_UNSIGNED_LONG  10
#define MPC_LONG_DOUBLE  11
#define MPC_LONG_LONG 12
#define MPC_CHAR 13
#define MPC_LOGICAL 22
#define MPC_INTEGER1 24
#define MPC_INTEGER2 25
#define MPC_INTEGER4 26
#define MPC_INTEGER8 27
#define MPC_REAL4 28
#define MPC_REAL8 29
#define MPC_REAL16 30
#define MPC_SIGNED_CHAR 31
#define MPC_UNSIGNED_LONG_LONG 33
#define MPC_INT8_T 34
#define MPC_UINT8_T 35
#define MPC_INT16_T 36
#define MPC_UINT16_T 37
#define MPC_INT32_T 38
#define MPC_UINT32_T 39
#define MPC_INT64_T 40
#define MPC_UINT64_T 41
#define MPC_WCHAR 45
#define MPC_INTEGER16 46
#define MPC_AINT 49
#define MPC_OFFSET 50
#define MPC_COUNT 51
#define MPC_LONG_LONG_INT 52
#define MPC_C_BOOL 53
#define MPC_CHARACTER 54
#define MPC_INTEGER 55
#define MPC_REAL 56
#define MPC_DOUBLE_PRECISION 57
#define MPC_UNSIGNED_LONG_LONG_INT  58

/* for comm_split method */
#define MPC_COMM_TYPE_SHARED 1
#define MPC_COMM_TYPE_SHARED_TR 2
#define MPC_COMM_TYPE_SOCKET 3
#define MPC_COMM_TYPE_SOCKET_TR 4
#define MPC_COMM_TYPE_NUMA 5
#define MPC_COMM_TYPE_NUMA_TR 6
#define MPC_COMM_TYPE_MPC_PROCESS 7
#define MPC_COMM_TYPE_MPC_PROCESS_TR 8

/* BE VERY CAREFUL HERE /!\/!\/!\/!\/!\/!\/!\/!\
 *  You have to note that these types
 *  are offseted of SCTK_DERIVED_DATATYPE_BASE
 *  actually making them derived ones.
 *  As common ones they are initialized in
 *  \ref sctk_common_datatype_init however
 *  they are derived ones !
 * 
 * See how the value is incremented and how MPC_STRUCT_DATATYPE_COUNT
 * is the last value plus 1.
 * 
 */
#define MPC_FLOAT_INT (SCTK_DERIVED_DATATYPE_BASE)
#define MPC_LONG_INT (SCTK_DERIVED_DATATYPE_BASE + 1)
#define MPC_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 2)
#define MPC_SHORT_INT  (SCTK_DERIVED_DATATYPE_BASE + 3)
#define MPC_2INT  (SCTK_DERIVED_DATATYPE_BASE + 4)
#define MPC_2FLOAT  (SCTK_DERIVED_DATATYPE_BASE + 5)
#define MPC_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 6)
#define MPC_2DOUBLE_PRECISION  (SCTK_DERIVED_DATATYPE_BASE + 7)
#define MPC_LONG_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 8)
#define MPC_COMPLEX8  (SCTK_DERIVED_DATATYPE_BASE + 9)
#define MPC_COMPLEX16  (SCTK_DERIVED_DATATYPE_BASE + 10)
#define MPC_COMPLEX32  (SCTK_DERIVED_DATATYPE_BASE + 11)
#define MPC_DOUBLE_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 12)
#define MPC_2INTEGER (SCTK_DERIVED_DATATYPE_BASE + 13)
#define MPC_2REAL (SCTK_DERIVED_DATATYPE_BASE + 14)

/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\
 * If you change anything in this section update MPC_STRUCT_DATATYPE_COUNT
 * otherwise the first derived datatype created
 * will overwrite your last datatype
 * /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\
 */
#define MPC_STRUCT_DATATYPE_COUNT 15

/* Aliased struct data-types */
#define MPC_C_COMPLEX MPC_COMPLEX
#define MPC_C_FLOAT_COMPLEX MPC_COMPLEX8
#define MPC_C_DOUBLE_COMPLEX MPC_COMPLEX16
#define MPC_C_LONG_DOUBLE_COMPLEX MPC_COMPLEX32

/* Predefined MPI datatypes corresponding to both C and Fortran datatypes */
#define MPC_CXX_BOOL 59
#define MPC_CXX_FLOAT_COMPLEX MPC_COMPLEX8
#define MPC_CXX_DOUBLE_COMPLEX MPC_COMPLEX16
#define MPC_CXX_LONG_DOUBLE_COMPLEX MPC_COMPLEX32

/* Datatype decoders */

typedef enum {
  MPC_COMBINER_UNKNOWN = 0,
  MPC_COMBINER_NAMED = 1,
  MPC_COMBINER_DUP = 2,
  MPC_COMBINER_CONTIGUOUS = 3,
  MPC_COMBINER_VECTOR = 4,
  MPC_COMBINER_HVECTOR = 5,
  MPC_COMBINER_INDEXED = 6,
  MPC_COMBINER_HINDEXED = 7,
  MPC_COMBINER_INDEXED_BLOCK = 8,
  MPC_COMBINER_HINDEXED_BLOCK = 9,
  MPC_COMBINER_STRUCT = 10,
  MPC_COMBINER_SUBARRAY = 11,
  MPC_COMBINER_DARRAY = 12,
  MPC_COMBINER_F90_REAL = 13,
  MPC_COMBINER_F90_COMPLEX = 14,
  MPC_COMBINER_F90_INTEGER = 15,
  MPC_COMBINER_RESIZED = 16,
  /* *_INTEGER COMBINER ARE DEPRECATED
   * in MPI 3.0 consequently they
   * are never returned by get_envelope */
  MPC_COMBINER_HINDEXED_INTEGER = 17,
  MPC_COMBINER_STRUCT_INTEGER = 18,
  MPC_COMBINER_HVECTOR_INTEGER = 19,
  MPC_COMBINER_DUMMY,
  MPC_COMBINER_COUNT__
} MPC_Type_combiner;

/* Note that this state is completed with other types in sctk_ft_types.h */

#define MPC_STATE_NO_SUPPORT 0

#if defined(MPC_Fault_Tolerance) || defined(MPC_MODULE_MPC_Fault_Tolerance)
/* Checkpoint */
typedef sctk_ft_state_t MPC_Checkpoint_state;
#else
typedef int MPC_Checkpoint_state;
#endif


/*Error_handler */
#define MPC_MAX_ERROR_STRING 512

  /* MPI Info management */
 
/* Matches the one of MPI_INFO_NULL @ mpc_mpi.h:207 */
#define MPC_INFO_NULL (-1)
/* Maximum length for keys and values
 * they are both defined for MPC and MPI variants */
/*1 MB */
#define MPC_MAX_INFO_VAL 1048576
#define MPC_MAX_INFO_KEY 256

  
  /*MPI compatibility*/

#define MPC_BOTTOM ((void*)0)



    /* Profiling Functions */

  typedef struct
  {
    int virtual_cpuid;
    double usage;
  } MPC_Activity_t;

  typedef int(MPC_Type_copy_attr_function)(mpc_mp_datatype_t old_type,
                                           int type_keyval, void *extra_state,
                                           void *attribute_val_in,
                                           void *attribute_val_out, int *flag);
  typedef int(MPC_Type_delete_attr_function)(mpc_mp_datatype_t datatype,
                                             int type_keyval,
                                             void *attribute_val,
                                             void *extra_state);



    /*Initialisation */
  int _mpc_m_init (int *argc, char ***argv);
  int _mpc_m_init_thread (int *argc, char ***argv, int required, int *provided);
  int _mpc_m_initialized (int *flag);
  int _mpc_m_finalize (void);
  int _mpc_m_abort (mpc_mp_communicator_t, int);
  int _mpc_m_query_thread (int *provided);

  /*Topology informations */
  int _mpc_m_comm_rank (mpc_mp_communicator_t comm, int *rank);
  int _mpc_m_comm_size (mpc_mp_communicator_t comm, int *size);
  int _mpc_m_comm_remote_size (mpc_mp_communicator_t comm, int *size);
  int _mpc_m_node_rank (int *rank);
  int _mpc_m_get_processor_name (char *name, int *resultlen);


  /*Collective operations */
  int _mpc_m_barrier( mpc_mp_communicator_t comm );
  int _mpc_m_bcast( void *buffer, mpc_mp_msg_count_t count,
				            mpc_mp_datatype_t datatype, int root, mpc_mp_communicator_t comm);
  int _mpc_m_gather(void *sendbuf, mpc_mp_msg_count_t sendcnt,
		mpc_mp_datatype_t sendtype, void *recvbuf,
		mpc_mp_msg_count_t recvcount, mpc_mp_datatype_t recvtype,
		int root, mpc_mp_communicator_t comm);
  int _mpc_m_allgather(void *sendbuf, mpc_mp_msg_count_t sendcount,
		mpc_mp_datatype_t sendtype, void *recvbuf,
		mpc_mp_msg_count_t recvcount,
		mpc_mp_datatype_t recvtype, mpc_mp_communicator_t comm);

  int _mpc_m_op_create (sctk_Op_User_function *, int, sctk_Op *);
  int _mpc_m_op_free (sctk_Op *);



  /*P-t-P Communications */
  int _mpc_m_isend (void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype,
		 int dest, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t * request);
  int _mpc_m_ibsend (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		  mpc_mp_request_t *);
  int _mpc_m_issend (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		  mpc_mp_request_t *);
  int _mpc_m_irsend (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		  mpc_mp_request_t *);

  int _mpc_m_irecv (void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype,
		 int source, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t * request);

  int _mpc_m_wait (mpc_mp_request_t * request, mpc_mp_status_t * status);
  int _mpc_m_waitall (mpc_mp_msg_count_t, mpc_mp_request_t *, mpc_mp_status_t *);
  int _mpc_m_waitsome (mpc_mp_msg_count_t, mpc_mp_request_t *, mpc_mp_msg_count_t *,
		    mpc_mp_msg_count_t *, mpc_mp_status_t *);
  int _mpc_m_waitany (mpc_mp_msg_count_t count, mpc_mp_request_t array_of_requests[],
		   mpc_mp_msg_count_t * index, mpc_mp_status_t * status);
  int _mpc_m_wait_pending (mpc_mp_communicator_t comm);
  int _mpc_m_wait_pending_all_comm (void);


  int _mpc_m_test( mpc_mp_request_t *request, int *flag, mpc_mp_status_t *status );

  int PMPC_Iprobe (int, int, mpc_mp_communicator_t, int *, mpc_mp_status_t *);
  int PMPC_Probe (int, int, mpc_mp_communicator_t, mpc_mp_status_t *);
  int _mpc_m_status_get_count (mpc_mp_status_t *, mpc_mp_datatype_t, mpc_mp_msg_count_t *);

  int  _mpc_m_send(void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t);

  int _mpc_m_ssend (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t);

  int _mpc_m_recv (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		mpc_mp_status_t *);

  int recv (void *, mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, void *,
		    mpc_mp_msg_count_t, mpc_mp_datatype_t, int, int, mpc_mp_communicator_t,
		    mpc_mp_status_t *);

  /*Status */
  int mpc_mp_comm_request_cancel (mpc_mp_request_t *);


  /*Groups */
  int _mpc_m_comm_group (mpc_mp_communicator_t, _mpc_m_group_t **);
  int _mpc_m_comm_remote_group (mpc_mp_communicator_t, _mpc_m_group_t **);
  int _mpc_m_group_free (_mpc_m_group_t **);
  int _mpc_m_group_incl (_mpc_m_group_t *, int, int *, _mpc_m_group_t **);
  int _mpc_m_group_difference (_mpc_m_group_t *, _mpc_m_group_t *, _mpc_m_group_t **);

  /*Communicators */
  int _mpc_m_comm_create (mpc_mp_communicator_t, _mpc_m_group_t *, mpc_mp_communicator_t *);
  int _mpc_m_intercomm_create (mpc_mp_communicator_t local_comm, int local_leader, mpc_mp_communicator_t peer_comm, int remote_leader, int tag, mpc_mp_communicator_t * newintercomm);
  int _mpc_m_comm_create_from_intercomm (mpc_mp_communicator_t, _mpc_m_group_t *, mpc_mp_communicator_t *);
  int _mpc_m_comm_free (mpc_mp_communicator_t *);
  int _mpc_m_comm_dup (mpc_mp_communicator_t, mpc_mp_communicator_t *);
  int _mpc_m_comm_split (mpc_mp_communicator_t, int, int, mpc_mp_communicator_t *);

  /*Error_handler */
  void _mpc_m_default_error (mpc_mp_communicator_t * comm, int *error, char *msg, char *file, int line);
  void _mpc_m_return_error (mpc_mp_communicator_t * comm, int *error, ...);

  int _mpc_m_errhandler_create (MPC_Handler_function *, MPC_Errhandler *);
  int _mpc_m_errhandler_set (mpc_mp_communicator_t, MPC_Errhandler);
  int _mpc_m_errhandler_get (mpc_mp_communicator_t, MPC_Errhandler *);
  int _mpc_m_errhandler_free (MPC_Errhandler *);
  int _mpc_m_error_string (int, char *, int *);
  int _mpc_m_error_class (int, int *);

  /*Timing */
  double _mpc_m_wtime (void);
  double _mpc_m_wtick (void);

  /*Types */
  int _mpc_m_type_size (mpc_mp_datatype_t, size_t *);
  int _mpc_m_type_is_allocated (mpc_mp_datatype_t datatype, int * flag );
  int _mpc_m_type_flag_padded(mpc_mp_datatype_t datatype);
  int _mpc_m_type_hcontiguous (mpc_mp_datatype_t *outtype, size_t count, mpc_mp_datatype_t *data_in);
  int _mpc_m_type_free (mpc_mp_datatype_t * datatype);
  int _mpc_m_type_dup( mpc_mp_datatype_t old_type, mpc_mp_datatype_t * new_type );
  int _mpc_m_type_set_name( mpc_mp_datatype_t datatype, char *name );
  int _mpc_m_type_get_name( mpc_mp_datatype_t datatype, char *name, int * resultlen );
  int _mpc_m_type_get_envelope(mpc_mp_datatype_t datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);
  int _mpc_m_type_get_contents( mpc_mp_datatype_t datatype,
                                int max_integers,
                                int max_addresses,
                                int max_datatypes,
                                int array_of_integers[],
                                size_t array_of_addresses[],
                                mpc_mp_datatype_t array_of_datatypes[]);
  int _mpc_m_type_commit( mpc_mp_datatype_t * type );

  /* Types Keyval handling */
  int _mpc_m_type_free_keyval(int *type_keyval);
  int _mpc_m_type_create_keyval(MPC_Type_copy_attr_function *copy,
                              MPC_Type_delete_attr_function *deletef,
                              int *type_keyval, void *extra_state);
  int _mpc_m_type_delete_attr(mpc_mp_datatype_t datatype, int type_keyval);
  int _mpc_m_type_set_attr(mpc_mp_datatype_t datatype, int type_keyval,
                         void *attribute_val);
  int _mpc_m_type_get_attr(mpc_mp_datatype_t datatype, int attribute_val,
                         void *type_keyval, int *flag);

  struct Datatype_External_context;
  int _mpc_m_derived_datatype (mpc_mp_datatype_t * datatype,
			    mpc_pack_absolute_indexes_t * begins,
			    mpc_pack_absolute_indexes_t * ends,
			    mpc_mp_datatype_t * types,
			    unsigned long count,
			    mpc_pack_absolute_indexes_t lb, int is_lb,
			    mpc_pack_absolute_indexes_t ub, int is_ub,
			    struct Datatype_External_context *ectx );

  int _mpc_m_type_get_true_extent(mpc_mp_datatype_t datatype, size_t *true_lb, size_t *true_extent);
  int _mpc_m_type_convert_to_derived( mpc_mp_datatype_t in_datatype, mpc_mp_datatype_t * out_datatype );
  int _mpc_m_type_use (mpc_mp_datatype_t datatype);

  /*Requests */
  int mpc_mp_comm_request_free (mpc_mp_request_t *);

  /*Scheduling */
  int _mpc_m_checkpoint (MPC_Checkpoint_state* st);
  int _mpc_m_get_activity (int nb_item, MPC_Activity_t * tab,	double *process_act);

  /*Packs */
  int _mpc_m_open_pack (mpc_mp_request_t * request);

  int _mpc_m_add_pack (void *buf, mpc_mp_msg_count_t count,
		    mpc_pack_indexes_t * begins,
		    mpc_pack_indexes_t * ends, mpc_mp_datatype_t datatype,
		    mpc_mp_request_t * request);
  int _mpc_m_add_pack_absolute (void *buf, mpc_mp_msg_count_t count,
			   mpc_pack_absolute_indexes_t * begins,
			   mpc_pack_absolute_indexes_t * ends,
			   mpc_mp_datatype_t datatype, mpc_mp_request_t * request);

  int _mpc_m_isend_pack (int dest, int tag, mpc_mp_communicator_t comm,
		      mpc_mp_request_t * request);
  int _mpc_m_irecv_pack (int source, int tag, mpc_mp_communicator_t comm,
		      mpc_mp_request_t * request);


  /* MPI Info management */

  int _mpc_m_info_set( MPC_Info, const char *, const char * );
  int _mpc_m_info_free( MPC_Info * );
  int _mpc_m_info_create( MPC_Info * );
  int _mpc_m_info_delete( MPC_Info , const char * );
  int _mpc_m_info_get(MPC_Info , const char *, int , char *, int *);
  int _mpc_m_info_dup( MPC_Info , MPC_Info * );
  int _mpc_m_info_get_nkeys (MPC_Info, int *);
  int _mpc_m_info_get_count (MPC_Info, int, char *);
  int _mpc_m_info_get_valuelen (MPC_Info, char *, int *, int *);

  /* Generalized Requests */

  int _mpc_m_grequest_start( MPC_Grequest_query_function *query_fr, MPC_Grequest_free_function * free_fn,
						  MPC_Grequest_cancel_function * cancel_fn, void *extra_state, mpc_mp_request_t * request);

  int _mpc_m_grequest_complete(  mpc_mp_request_t request);

  /* Extended Generalized Requests */

  int _mpc_m_egrequest_start(MPC_Grequest_query_function *query_fn,
                          MPC_Grequest_free_function * free_fn,
                          MPC_Grequest_cancel_function * cancel_fn,
                          MPCX_Grequest_poll_fn * poll_fn,
                          void *extra_state,
                          mpc_mp_request_t * request);

  /* Extended Generalized Requests Classes */
  int _mpc_m_grequest_class_create( MPC_Grequest_query_function * query_fn,
				   MPC_Grequest_cancel_function * cancel_fn,
				   MPC_Grequest_free_function * free_fn,
				   MPCX_Grequest_poll_fn * poll_fn,
				   MPCX_Grequest_wait_fn * wait_fn,
				   MPCX_Request_class * new_class );

  int _mpc_m_grequest_class_allocate( MPCX_Request_class target_class, void *extra_state, mpc_mp_request_t *request );


int _mpc_m_request_get_status( mpc_mp_request_t request, int *flag, mpc_mp_status_t *status );
int _mpc_m_status_set_elements_x( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, size_t count );
int _mpc_m_status_set_elements( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, int count );

#ifdef __cplusplus
}
#endif
#endif
