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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_COMM_LIB_H_
#define MPC_COMM_LIB_H_

#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>


/************************************************************************/
/* Per thread context                                                   */
/************************************************************************/

struct mpc_mpi_cl_per_thread_ctx_s;

void mpc_mpi_cl_per_thread_ctx_init();
void mpc_mpi_cl_per_thread_ctx_release();

/************************************************************************/
/* Per Process context                                                  */
/************************************************************************/

struct mpc_mpi_cl_per_mpi_process_ctx_s;

struct mpc_mpi_cl_per_mpi_process_ctx_s *mpc_cl_per_mpi_process_ctx_get();

void mpc_mpi_cl_per_mpi_process_ctx_reinit ( struct mpc_mpi_cl_per_mpi_process_ctx_s *tmp );
int mpc_mpi_cl_per_mpi_process_ctx_at_exit_register( void ( *function )( void ) );

/*******************
 * PACK MANAGEMENT *
 *******************/

int mpc_mpi_cl_open_pack( mpc_lowcomm_request_t *request );

int mpc_mpi_cl_add_pack( void *buf, mpc_lowcomm_msg_count_t count,
                      unsigned long *begins,
                      unsigned long *ends, mpc_lowcomm_datatype_t datatype,
                      mpc_lowcomm_request_t *request );

int mpc_mpi_cl_add_pack_absolute( void *buf, mpc_lowcomm_msg_count_t count,
                               long *begins,
                               long *ends,
                               mpc_lowcomm_datatype_t datatype, mpc_lowcomm_request_t *request );

int mpc_mpi_cl_isend_pack( int dest, int tag, mpc_lowcomm_communicator_t comm,
                        mpc_lowcomm_request_t *request );

int mpc_mpi_cl_irecv_pack( int source, int tag, mpc_lowcomm_communicator_t comm,
                        mpc_lowcomm_request_t *request );

/*******************
 * WAIT OPERATIONS *
 *******************/

int mpc_mpi_cl_wait_pending( mpc_lowcomm_communicator_t comm );

int mpc_mpi_cl_wait_pending_all_comm( void );

/*************************
 * COLLECTIVE OPERATIONS *
 *************************/

#define MPC_CREATE_INTERN_FUNC( name ) extern const sctk_Op MPC_##name

MPC_CREATE_INTERN_FUNC( SUM );
MPC_CREATE_INTERN_FUNC( MAX );
MPC_CREATE_INTERN_FUNC( MIN );
MPC_CREATE_INTERN_FUNC( PROD );
MPC_CREATE_INTERN_FUNC( LAND );
MPC_CREATE_INTERN_FUNC( BAND );
MPC_CREATE_INTERN_FUNC( LOR );
MPC_CREATE_INTERN_FUNC( BOR );
MPC_CREATE_INTERN_FUNC( LXOR );
MPC_CREATE_INTERN_FUNC( BXOR );
MPC_CREATE_INTERN_FUNC( MINLOC );
MPC_CREATE_INTERN_FUNC( MAXLOC );


/************************
 * TIMING AND PROFILING *
 ************************/

typedef struct
{
	int virtual_cpuid;
	double usage;
} mpc_mpi_cl_activity_t;

int mpc_mpi_cl_get_activity( int nb_item, mpc_mpi_cl_activity_t *tab, double *process_act );


/****************
 * MISC DEFINES *
 ****************/

#define MPC_UNDEFINED ( -1 )
#define MPC_ROOT -4
#define MPC_KEYVAL_INVALID -1

/****************
 * STRING SIZES *
 ****************/

#define MPC_MAX_PROCESSOR_NAME 255
#define MPC_MAX_OBJECT_NAME 256
#define MPC_MAX_LIBRARY_VERSION_STRING 8192

/*****************
 * THREAD LEVELS *
 *****************/

#define MPC_THREAD_SINGLE 0
#define MPC_THREAD_FUNNELED 1
#define MPC_THREAD_SERIALIZED 2
#define MPC_THREAD_MULTIPLE 3

/******************
 * ERROR HANDLING *
 ******************/

typedef void( MPC_Handler_function )( void *, int *, ... );
typedef int MPC_Errhandler;

#define MPC_MAX_ERROR_STRING 512

#define MPC_ERRHANDLER_NULL -1
#define MPC_ERRORS_RETURN -6
#define MPC_ERRORS_ARE_FATAL -7

/* Communication argument parameters */
#define MPC_ERR_BUFFER 1	/* Invalid buffer pointer */
#define MPC_ERR_COUNT 2		/* Invalid count argument */
#define MPC_ERR_TYPE 3		/* Invalid datatype argument */
#define MPC_ERR_TAG 4		/* Invalid tag argument */
#define MPC_ERR_COMM 5		/* Invalid communicator */
#define MPC_ERR_RANK 6		/* Invalid rank */
#define MPC_ERR_ROOT 8		/* Invalid root */
#define MPC_ERR_TRUNCATE 15 /* Message truncated on receive */
#define MPC_ERR_NOT_SAME 39 /* Collective argument not identical on all processess, \
							   or collective routines called in a different order   \
by different processes */

/* MPC Objects (other than COMM) */
#define MPC_ERR_GROUP 9   /* Invalid group */
#define MPC_ERR_OP 10	 /* Invalid operation */
#define MPC_ERR_REQUEST 7 /* Invalid mpc_request handle */

/* Special topology argument parameters */
#define MPC_ERR_TOPOLOGY 11 /* Invalid topology */
#define MPC_ERR_DIMS 12		/* Invalid dimension argument */

/* All other arguments.  This is a class with many kinds */
#define MPC_ERR_ARG 13		/* Invalid argument */
#define MPC_ERR_PORT 27		/* Invalid port name passed to MPI_COMM_CONNECT */
#define MPC_ERR_SERVICE 28  /* Invalid service name passed to MPI_UNPUBLISH_NAME */
#define MPC_ERR_NAME 29		/* Invalid service name passed to MPI_LOOKUP_NAME */
#define MPC_ERR_WIN 30		/* Invalid win argument */
#define MPC_ERR_SIZE 31		/* Invalid size argument */
#define MPC_ERR_DISP 32		/* Invalid disp argument */
#define MPC_ERR_INFO 33		/* Invalid info argument */
#define MPC_ERR_LOCKTYPE 34 /* Invalid locktype argument */
#define MPC_ERR_ASSERT 35   /* Invalid assert argument */

/* Error on File handling*/
#define MPC_ERR_FILE 38				   /* Invalid file handle */
#define MPC_ERR_AMODE 40			   /* Error related to the amode passed to MPI_FILE_OPEN */
#define MPC_ERR_UNSUPPORTED_DATAREP 41 /* Unsupported datarep passed to MPI_FILE_SET_VIEW */
#define MPC_ERR_NO_SUCH_FILE 43		   /* File does not exist */
#define MPC_ERR_FILE_EXISTS 44		   /* File exists */
#define MPC_ERR_BAD_FILE 45			   /* Invalid file name (e.g., path name too long) */
#define MPC_ERR_ACCESS 46			   /* Permission denied */
#define MPC_ERR_READ_ONLY 49		   /* Read-only file or file system */
#define MPC_ERR_FILE_IN_USE 50		   /* File operation could not completed, as the file is \
currently open by some process */
#define MPC_ERR_IO 53				   /* Other I/O error */

/* Multiple completion has two special error classes */
#define MPC_ERR_IN_STATUS 18 /* Look in status for error value */
#define MPC_ERR_PENDING 19   /* Pending request */

/* Error with comm attributes (keyval) */
#define MPC_ERR_KEYVAL 20	 /* Invalid keyval has been passed */
#define MPC_ERR_INFO_KEY 23   /* Key onger than MPI_MAX_INFO_KEY */
#define MPC_ERR_INFO_VALUE 24 /* Value longer than MPI_MAX_INFO_VAL */
#define MPC_ERR_INFO_NOKEY 25 /* Invalid key passed to MPI_INFO_DELETE */

/* Error with memory */
#define MPC_ERR_NO_MEM 21 /* MPI_ALLOC_MEM failed because memory is exhausted */
#define MPC_ERR_BASE 22   /* Invalid base passed to MPI_FREE_MEM */

/* Other errors that are not simply an invalid argument */
#define MPC_ERR_SPAWN 26				 /* Error in spawning processes */
#define MPC_ERR_RMA_CONFLICT 36			 /* Conflicting accesses to window */
#define MPC_ERR_RMA_SYNC 37				 /* Wrong synchronization of RMA calls */
#define MPC_ERR_UNSUPPORTED_OPERATION 42 /* Unsupported operation, such as seeking on a file which \
supports sequential access only*/
#define MPC_ERR_NO_SPACE 47				 /* Not enough space */
#define MPC_ERR_QUOTA 48				 /* Quota exceeded */
#define MPC_ERR_DUP_DATAREP 51			 /* Conversion functions could not be regiestered because a \
                                                        data representation identifier that was already defined \
was passed to MPI_REGISTER_DATAREP */
#define MPC_ERR_CONVERSION 52			 /* An error occured in a user supplied data conversion function */
#define MPC_ERR_OTHER 16				 /* Other error; use Error_string */

#define MPC_ERR_UNKNOWN 14 /* Unknown error */
#define MPC_ERR_INTERN 17  /* Internal error code    */

#define MPCR_ERRORS_THROW_EXCEPTIONS 55

#define MPC_NOT_IMPLEMENTED 56

#define MPC_ERR_RMA_FLAVOR 57 /* Wrong type of RMA window */

/* HOLE @ 58 */

#define MPC_T_ERR_MEMORY 59			 /* Out of memory */
#define MPC_T_ERR_NOT_INITIALIZED 60 /* Interface not initialized */
#define MPC_T_ERR_CANNOT_INIT           \
	61 /* Interface not in the state to \
be initialized */
#define MPC_T_ERR_INVALID_INDEX                                  \
	62								  /* The index is invalid or \
has been deleted  */
#define MPC_T_ERR_INVALID_ITEM 63	 /* Item index queried is out of range */
#define MPC_T_ERR_INVALID_HANDLE 64   /* The handle is invalid */
#define MPC_T_ERR_OUT_OF_HANDLES 65   /* No more handles available */
#define MPC_T_ERR_OUT_OF_SESSIONS 66  /* No more sessions available */
#define MPC_T_ERR_INVALID_SESSION 67  /* Session argument is not valid */
#define MPC_T_ERR_CVAR_SET_NOT_NOW 68 /* Cvar can't be set at this moment */
#define MPC_T_ERR_CVAR_SET_NEVER                                  \
	69								   /* Cvar can't be set until \
end of execution */
#define MPC_T_ERR_PVAR_NO_STARTSTOP 70 /* Pvar can't be started or stopped */
#define MPC_T_ERR_PVAR_NO_WRITE 71	 /* Pvar can't be written or reset */
#define MPC_T_ERR_PVAR_NO_ATOMIC 72	/* Pvar can't be R/W atomically */
#define MPC_T_ERR_INVALID_NAME 73	  /* Requested name is invalid */

#define MPC_ERR_RMA_RANGE 74
#define MPC_ERR_RMA_ATTACH 75
#define MPC_ERR_RMA_SHARED 76

#define MPC_ERR_LASTCODE 77

/*************************
 * MESSAGING LAYER TYPES *
 *************************/

typedef int MPC_Info;
typedef int MPC_Message;

/*************
 * DATATYPES *
 *************/

#define MPC_DATATYPE_NULL ( (mpc_lowcomm_datatype_t) -1 )
#define MPC_UB ( (mpc_lowcomm_datatype_t) -2 )
#define MPC_LB ( (mpc_lowcomm_datatype_t) -3 )
#define MPC_PACKED 0
#define MPC_BYTE 1
#define MPC_SHORT 2
#define MPC_INT 3
#define MPC_LONG 4
#define MPC_FLOAT 5
#define MPC_DOUBLE 6
#define MPC_UNSIGNED_CHAR 7
#define MPC_UNSIGNED_SHORT 8
#define MPC_UNSIGNED 9
#define MPC_UNSIGNED_LONG 10
#define MPC_LONG_DOUBLE 11
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
#define MPC_UNSIGNED_LONG_LONG_INT 58

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
 *  \ref __mpc_common_types_init however
 *  they are derived ones !
 *
 * See how the value is incremented and how MPC_STRUCT_DATATYPE_COUNT
 * is the last value plus 1.
 *
 */
#define MPC_FLOAT_INT ( SCTK_DERIVED_DATATYPE_BASE )
#define MPC_LONG_INT ( SCTK_DERIVED_DATATYPE_BASE + 1 )
#define MPC_DOUBLE_INT ( SCTK_DERIVED_DATATYPE_BASE + 2 )
#define MPC_SHORT_INT ( SCTK_DERIVED_DATATYPE_BASE + 3 )
#define MPC_2INT ( SCTK_DERIVED_DATATYPE_BASE + 4 )
#define MPC_2FLOAT ( SCTK_DERIVED_DATATYPE_BASE + 5 )
#define MPC_COMPLEX ( SCTK_DERIVED_DATATYPE_BASE + 6 )
#define MPC_2DOUBLE_PRECISION ( SCTK_DERIVED_DATATYPE_BASE + 7 )
#define MPC_LONG_DOUBLE_INT ( SCTK_DERIVED_DATATYPE_BASE + 8 )
#define MPC_COMPLEX8 ( SCTK_DERIVED_DATATYPE_BASE + 9 )
#define MPC_COMPLEX16 ( SCTK_DERIVED_DATATYPE_BASE + 10 )
#define MPC_COMPLEX32 ( SCTK_DERIVED_DATATYPE_BASE + 11 )
#define MPC_DOUBLE_COMPLEX ( SCTK_DERIVED_DATATYPE_BASE + 12 )
#define MPC_2INTEGER ( SCTK_DERIVED_DATATYPE_BASE + 13 )
#define MPC_2REAL ( SCTK_DERIVED_DATATYPE_BASE + 14 )

/* /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\
 * If you change anything in this section update MPC_STRUCT_DATATYPE_COUNT
 * otherwise the first derived datatype created
 * will overwrite your last datatype
 * /!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\/!\
 */
#define MPC_STRUCT_DATATYPE_COUNT 15
#define SCTK_COMMON_DATA_TYPE_COUNT 70
#define SCTK_USER_DATA_TYPES_MAX 4000
#define SCTK_DERIVED_DATATYPE_BASE ( SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX )

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

typedef enum
{
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

typedef int( MPC_Type_copy_attr_function )( mpc_lowcomm_datatype_t old_type,
        int type_keyval, void *extra_state,
        void *attribute_val_in,
        void *attribute_val_out, int *flag );
typedef int( MPC_Type_delete_attr_function )( mpc_lowcomm_datatype_t datatype,
        int type_keyval,
        void *attribute_val,
        void *extra_state );

#endif /* MPC_COMM_LIB_H_ */
