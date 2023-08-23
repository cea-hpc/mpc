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
#include <mpc_common_types.h>
#include <mpc_lowcomm_group.h>

/************************************************************************/
/* Per thread context                                                   */
/************************************************************************/

void mpc_mpi_cl_per_thread_ctx_init(void);
void mpc_mpi_cl_per_thread_ctx_release(void);

/************************************************************************/
/* Per Process context                                                  */
/************************************************************************/

struct mpc_mpi_cl_per_mpi_process_ctx_s;

struct mpc_mpi_cl_per_mpi_process_ctx_s *mpc_cl_per_mpi_process_ctx_get(void);

int mpc_mpi_cl_per_mpi_process_ctx_at_exit_register( void ( *function )( void ) );

/*******************
 * PACK MANAGEMENT *
 *******************/

int mpc_mpi_cl_open_pack( mpc_lowcomm_request_t *request );

int mpc_mpi_cl_add_pack( void *buf, mpc_lowcomm_msg_count_t count,
                      unsigned long *begins,
                      unsigned long *ends, mpc_lowcomm_datatype_t datatype,
                      mpc_lowcomm_request_t *request );

int mpc_mpi_cl_add_pack_absolute(const void *buf, mpc_lowcomm_msg_count_t count,
                               long *begins,
                               long *ends,
                               mpc_lowcomm_datatype_t datatype, mpc_lowcomm_request_t *request );

int mpc_mpi_cl_isend_pack( int dest, int tag, mpc_lowcomm_communicator_t comm,
                        mpc_lowcomm_request_t *request );

int mpc_mpi_cl_irecv_pack( int source, int tag, mpc_lowcomm_communicator_t comm,
                        mpc_lowcomm_request_t *request );


/*******************
 * RANK CONVERSION *
 *******************/

int mpc_mpi_cl_world_rank( mpc_lowcomm_communicator_t comm, int rank);

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

/* Propagate changes in \ref romio_ctx.c */
#define MPC_KEYVAL_INVALID -7

/****************
 * STRING SIZES *
 ****************/

#define MPC_MAX_PROCESSOR_NAME 256
#define MPC_MAX_OBJECT_NAME 128
#define MPC_MAX_LIBRARY_VERSION_STRING 8192

/* 4.0 new maximum size variables */
#define MPC_MAX_STRINGTAG_LEN 256
#define MPC_MAX_PSET_NAME_LEN 256

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
typedef struct MPI_ABI_Errhandler *MPC_Errhandler;

#define MPC_ERRHANDLER_NULL   ((MPC_Errhandler) 0)
#define MPC_ERRORS_ARE_FATAL  ((MPC_Errhandler) 1)
#define MPC_ERRORS_RETURN     ((MPC_Errhandler) 2)
#define MPC_ERRORS_ABORT      ((MPC_Errhandler) 3)

#define MPC_MAX_ERROR_STRING 512

/* Error Classes */
enum
{
/***********************************************
 *     CODES BELOW ARE SHARE WITH LOWCOMM      *
 * AND THEY SHOULD BE MAINTAINED FIRST IN LIST *
 ***********************************************/
	MPC_ERR_TRUNCATE = MPC_LOWCOMM_ERR_TRUNCATE,	/* Message truncated on receive */
	MPC_ERR_TYPE = MPC_LOWCOMM_ERR_TYPE,	        /* Invalid datatype argument */
/***********************
 * END OF SHARED CODES *
 ***********************/

    /* MPC specific error classes */
    MPCR_ERRORS_THROW_EXCEPTIONS = -2,
    MPC_NOT_IMPLEMENTED = -1,

    /******************************************
     *        ABI compliant declaration       *
     *      The order is the standard one     *
     ******************************************/

    MPC_SUCCESS = MPC_LOWCOMM_SUCCESS,

	MPC_ERR_BUFFER,	/* Invalid buffer pointer */
	MPC_ERR_COUNT,	/* Invalid count argument */
	MPC_ERR_TAG = MPC_ERR_TYPE + 1,	/* Invalid tag argument */
	MPC_ERR_COMM,	/* Invalid communicator */
	MPC_ERR_RANK,	/* Invalid rank */
	MPC_ERR_REQUEST,	/* Invalid mpc_request handle */
	MPC_ERR_ROOT,	/* Invalid root */

	MPC_ERR_GROUP,	/* Invalid group */
	MPC_ERR_OP,	/* Invalid operation */

	MPC_ERR_TOPOLOGY,	/* Invalid topology */
	MPC_ERR_DIMS,	/* Invalid dimension argument */

	MPC_ERR_ARG,	/* Invalid argument */
	MPC_ERR_UNKNOWN,	/* Unknown error */

	MPC_ERR_OTHER = MPC_ERR_TRUNCATE + 1,	/* Other error; use Error_string */
	MPC_ERR_INTERN,	/* Internal error code */
	MPC_ERR_PENDING,	/* Pending request */
	MPC_ERR_IN_STATUS,	/* Look in status for error value */

	MPC_ERR_ACCESS,	/* Permission denied */
	MPC_ERR_AMODE,	/* Error related to the amode passed to MPI_FILE_OPEN */
	MPC_ERR_ASSERT,	/* Invalid assert argument */

	MPC_ERR_BAD_FILE,	/* Invalid file name (e.g., path name too long) */
	MPC_ERR_BASE,	/* Invalid base passed to MPI_FREE_MEM */
	MPC_ERR_CONVERSION,	/* An error occured in a user supplied data conversion function */
	MPC_ERR_DISP,	/* Invalid disp argument */
	MPC_ERR_DUP_DATAREP,	/* Conversion functions could not be regiestered because a \
	data representation identifier that was already defined \
	was passed to MPI_REGISTER_DATAREP */

    //MPC_ERR_ERRHANDLER, /* New constant introduced in the 4.1 standard */

	MPC_ERR_FILE_EXISTS,	/* File exists */
	MPC_ERR_FILE_IN_USE,	/* File operation could not completed, as the file is \
	currently open by some process */
	MPC_ERR_FILE,	/* Invalid file handle */

	MPC_ERR_INFO_KEY,	/* Key onger than MPI_MAX_INFO_KEY */
	MPC_ERR_INFO_NOKEY,	/* Invalid key passed to MPI_INFO_DELETE */
	MPC_ERR_INFO_VALUE,	/* Value longer than MPI_MAX_INFO_VAL */
	MPC_ERR_INFO,	/* Invalid info argument */
	MPC_ERR_IO,	/* Other I/O error */
	MPC_ERR_KEYVAL,	/* Invalid keyval has been passed */
	MPC_ERR_LOCKTYPE,	/* Invalid locktype argument */

	MPC_ERR_NAME,	/* Invalid service name passed to MPI_LOOKUP_NAME */
	MPC_ERR_NO_MEM,	/* MPI_ALLOC_MEM failed because memory is exhausted */
	MPC_ERR_NOT_SAME,	/* Collective argument not identical on all processess, \
								or collective routines called in a different order   \
	by different processes */
	MPC_ERR_NO_SPACE,	/* Not enough space */
	MPC_ERR_NO_SUCH_FILE,	/* File does not exist */

	MPC_ERR_PORT,	/* Invalid port name passed to MPI_COMM_CONNECT */
    MPC_ERR_PROC_ABORTED, /* Operation failed due to the abortion of a peer process */
	MPC_ERR_QUOTA,	/* Quota exceeded */
	MPC_ERR_READ_ONLY,	/* Read-only file or file system */

	MPC_ERR_RMA_ATTACH, /* Memory cannot be attached */
	MPC_ERR_RMA_CONFLICT,	/* Conflicting accesses to window */
	MPC_ERR_RMA_RANGE, /* Target memory is not part of the window */
	MPC_ERR_RMA_SHARED, /* Memory cannot be shared */
	MPC_ERR_RMA_SYNC,	/* Wrong synchronization of RMA calls */
	MPC_ERR_RMA_FLAVOR,	/* Wrong type of RMA window */

	MPC_ERR_SERVICE,	/* Invalid service name passed to MPI_UNPUBLISH_NAME */
    MPC_ERR_SESSION, /* Invalid session argument */
	MPC_ERR_SIZE,	/* Invalid size argument */
	MPC_ERR_SPAWN,	/* Error in spawning processes */
	MPC_ERR_UNSUPPORTED_DATAREP,	/* Unsupported datarep passed to MPI_FILE_SET_VIEW */
	MPC_ERR_UNSUPPORTED_OPERATION,	/* Unsupported operation, such as seeking on a file which \
	supports sequential access only*/
    MPC_ERR_VALUE_TOO_LARGE, /* Value is too large to store */
	MPC_ERR_WIN,	/* Invalid win argument */

    /* MPC_T_ERR */
	MPC_T_ERR_CANNOT_INIT,    /* Interface not in the state to \
	be initialized */
    MPC_T_ERR_NOT_ACCESSIBLE, /* User may not access this function at this time */
	MPC_T_ERR_NOT_INITIALIZED,	/* Interface not initialized */
    MPC_T_ERR_NOT_SUPPORTED, /* Requested functionnality is not supported */
	MPC_T_ERR_MEMORY,	/* Out of memory */
	MPC_T_ERR_INVALID,	/* Item is invalid */
	MPC_T_ERR_INVALID_INDEX,		  /* The index is invalid or \
	has been deleted  */
	MPC_T_ERR_INVALID_ITEM,	/* Item index queried is out of range */
	MPC_T_ERR_INVALID_SESSION,	/* Session argument is not valid */
	MPC_T_ERR_INVALID_HANDLE,	/* The handle is invalid */
	MPC_T_ERR_INVALID_NAME,	/* Requested name is invalid */
	MPC_T_ERR_OUT_OF_HANDLES,	/* No more handles available */
	MPC_T_ERR_OUT_OF_SESSIONS,	/* No more sessions available */

	MPC_T_ERR_CVAR_SET_NOT_NOW,	/* Cvar can't be set at this moment */
	MPC_T_ERR_CVAR_SET_NEVER,		   /* Cvar can't be set until \
	end of execution */

	MPC_T_ERR_PVAR_NO_WRITE,	/* Pvar can't be written or reset */
	MPC_T_ERR_PVAR_NO_STARTSTOP,	/* Pvar can't be started or stopped */
	MPC_T_ERR_PVAR_NO_ATOMIC,	/* Pvar can't be R/W atomically */

	MPC_ERR_LASTCODE /* Last error code: must stay the last of this enum */
};


int mpc_mpi_cl_error_string( int code, char *str, int *len );

/*************************
 * MESSAGING LAYER TYPES *
 *************************/

typedef int MPC_Info;
typedef int MPC_Message;

/*************
 * DATATYPES *
 *************/

#define MPC_DATATYPE_NULL MPC_LOWCOMM_DATATYPE_NULL
#define MPC_UB ( (mpc_lowcomm_datatype_t) -2 ) /* Removed from the standard at the 3.0 */
#define MPC_LB ( (mpc_lowcomm_datatype_t) -3 ) /* Removed from the standard at the 3.0 */


/* for comm_split method */
#define MPC_COMM_TYPE_SHARED -100
#define MPC_COMM_TYPE_HW_UNGUIDED -101
#define MPC_COMM_TYPE_HW_GUIDED -102       /* Not implemented */
#define MPC_COMM_TYPE_RESOURCE_GUIDED -103 /* Not implemented */
#define MPC_COMM_TYPE_SOCKET -104
#define MPC_COMM_TYPE_NUMA -105
#define MPC_COMM_TYPE_UNIX_PROCESS -106
#define MPC_COMM_TYPE_HW_SUBDOMAIN -107
#define MPC_COMM_TYPE_APP -108
#define MPC_COMM_TYPE_NODE -109

/************************
 * Common derived types *
 ************************/

/* C Types */
#define MPC_C_COMPLEX             MPC_LOWCOMM_C_COMPLEX
#define MPC_C_FLOAT_COMPLEX       MPC_LOWCOMM_C_FLOAT_COMPLEX
#define MPC_C_DOUBLE_COMPLEX      MPC_LOWCOMM_C_DOUBLE_COMPLEX
#define MPC_C_LONG_DOUBLE_COMPLEX MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX

/* C++ Types */
#define MPC_CXX_FLOAT_COMPLEX       MPC_LOWCOMM_CXX_FLOAT_COMPLEX
#define MPC_CXX_DOUBLE_COMPLEX      MPC_LOWCOMM_CXX_DOUBLE_COMPLEX
#define MPC_CXX_LONG_DOUBLE_COMPLEX MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX

/* FORTRAN Types */
#define MPC_COMPLEX        MPC_LOWCOMM_COMPLEX
#define MPC_DOUBLE_COMPLEX MPC_LOWCOMM_DOUBLE_COMPLEX

/* F08 Types */
#define MPC_COMPLEX4  MPC_LOWCOMM_COMPLEX4
#define MPC_COMPLEX8  MPC_LOWCOMM_COMPLEX8
#define MPC_COMPLEX16 MPC_LOWCOMM_COMPLEX16
#define MPC_COMPLEX32 MPC_LOWCOMM_COMPLEX32

/* C Struct Types */
#define MPC_FLOAT_INT         MPC_LOWCOMM_FLOAT_INT
#define MPC_DOUBLE_INT        MPC_LOWCOMM_DOUBLE_INT
#define MPC_LONG_INT          MPC_LOWCOMM_LONG_INT
#define MPC_2INT              MPC_LOWCOMM_2INT
#define MPC_SHORT_INT         MPC_LOWCOMM_SHORT_INT
#define MPC_LONG_DOUBLE_INT   MPC_LOWCOMM_LONG_DOUBLE_INT

/* Fortran Struct Types */
#define MPC_2REAL             MPC_LOWCOMM_2REAL
#define MPC_2INTEGER          MPC_LOWCOMM_2INTEGER
#define MPC_2DOUBLE_PRECISION MPC_LOWCOMM_2DOUBLE_PRECISION

/* MPC Types */
#define MPC_2FLOAT MPC_LOWCOMM_2FLOAT


#define MPC_CXX_BOOL MPC_LOWCOMM_CXX_BOOL

/* Common and user types max counts */
#define SCTK_COMMON_DATA_TYPE_COUNT MPC_LOWCOMM_TYPE_COMMON_LIMIT
#define SCTK_USER_DATA_TYPES_MAX 4000

/* Datatype decoders */
typedef enum
{
	MPC_COMBINER_CONTIGUOUS = 0,
	MPC_COMBINER_DARRAY,
	MPC_COMBINER_DUP,
	MPC_COMBINER_F90_COMPLEX,
	MPC_COMBINER_F90_INTEGER,
	MPC_COMBINER_F90_REAL,
	MPC_COMBINER_HINDEXED,
	MPC_COMBINER_HVECTOR,
	MPC_COMBINER_INDEXED_BLOCK,
	MPC_COMBINER_HINDEXED_BLOCK,
	MPC_COMBINER_INDEXED,
	MPC_COMBINER_NAMED,
	MPC_COMBINER_RESIZED,
	MPC_COMBINER_STRUCT,
	MPC_COMBINER_SUBARRAY,
	MPC_COMBINER_VECTOR,

	/* *_INTEGER COMBINER ARE DEPRECATED
     * They have been removed
	 * in MPI 3.0 consequently they
	 * are never returned by get_envelope */
	MPC_COMBINER_HINDEXED_INTEGER,
	MPC_COMBINER_STRUCT_INTEGER,
	MPC_COMBINER_HVECTOR_INTEGER,

    /* Special combiners */
	MPC_COMBINER_UNKNOWN,
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

bool mpc_mpi_cl_type_is_contiguous(mpc_lowcomm_datatype_t type);

mpc_lowcomm_datatype_t _mpc_cl_per_mpi_process_ctx_user_datatype_array_get(void);

/********************
 * THREAD MIGRATION *
 ********************/

int mpc_cl_move_to( int process, int cpuid );

#endif /* MPC_COMM_LIB_H_ */
