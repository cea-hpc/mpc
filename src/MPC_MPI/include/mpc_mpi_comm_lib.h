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

enum
{
/***********************************************
 *     CODES BELOW ARE SHARE WITH LOWCOMM      *
 * AND THEY SHOULD BE MAINTAINED FIRST IN LIST *
 ***********************************************/
	MPC_ERR_TRUNCATE = MPC_LOWCOMM_ERR_TRUNCATE,	/* Message truncated on receive */
	MPC_ERR_TYPE = MPC_LOWCOMM_ERR_TYPE,	/* Invalid datatype argument */
	MPC_ERR_PENDING = MPC_LOWCOMM_ERR_PENDING,	/* Pending request */
/***********************
 * END OF SHARED CODES *
 ***********************/

	/* Communication argument parameters */
	MPC_ERR_BUFFER,	/* Invalid buffer pointer */
	MPC_ERR_COUNT,	/* Invalid count argument */
	MPC_ERR_TAG,	/* Invalid tag argument */
	MPC_ERR_COMM,	/* Invalid communicator */
	MPC_ERR_RANK,	/* Invalid rank */
	MPC_ERR_ROOT,	/* Invalid root */
	MPC_ERR_NOT_SAME,	/* Collective argument not identical on all processess, \
								or collective routines called in a different order   \
	by different processes */

	/* MPC Objects (other than COMM) */
	MPC_ERR_GROUP,	/* Invalid group */
	MPC_ERR_OP,	/* Invalid operation */
	MPC_ERR_REQUEST,	/* Invalid mpc_request handle */

	/* Special topology argument parameters */
	MPC_ERR_TOPOLOGY,	/* Invalid topology */
	MPC_ERR_DIMS,	/* Invalid dimension argument */

	/* All other arguments.  This is a class with many kinds */
	MPC_ERR_ARG,	/* Invalid argument */
	MPC_ERR_PORT,	/* Invalid port name passed to MPI_COMM_CONNECT */
	MPC_ERR_SERVICE,	/* Invalid service name passed to MPI_UNPUBLISH_NAME */
	MPC_ERR_NAME,	/* Invalid service name passed to MPI_LOOKUP_NAME */
	MPC_ERR_WIN,	/* Invalid win argument */
	MPC_ERR_SIZE,	/* Invalid size argument */
	MPC_ERR_DISP,	/* Invalid disp argument */
	MPC_ERR_INFO,	/* Invalid info argument */
	MPC_ERR_LOCKTYPE,	/* Invalid locktype argument */
	MPC_ERR_ASSERT,	/* Invalid assert argument */

	/* Error on File handling*/
	MPC_ERR_FILE,	/* Invalid file handle */
	MPC_ERR_AMODE,	/* Error related to the amode passed to MPI_FILE_OPEN */
	MPC_ERR_UNSUPPORTED_DATAREP,	/* Unsupported datarep passed to MPI_FILE_SET_VIEW */
	MPC_ERR_NO_SUCH_FILE,	/* File does not exist */
	MPC_ERR_FILE_EXISTS,	/* File exists */
	MPC_ERR_BAD_FILE,	/* Invalid file name (e.g., path name too long) */
	MPC_ERR_ACCESS,	/* Permission denied */
	MPC_ERR_READ_ONLY,	/* Read-only file or file system */
	MPC_ERR_FILE_IN_USE,	/* File operation could not completed, as the file is \
	currently open by some process */
	MPC_ERR_IO,	/* Other I/O error */

	/* Multiple completion has two special error classes */
	MPC_ERR_IN_STATUS,	/* Look in status for error value */

	/* Error with comm attributes (keyval) */
	MPC_ERR_KEYVAL,	/* Invalid keyval has been passed */
	MPC_ERR_INFO_KEY,	/* Key onger than MPI_MAX_INFO_KEY */
	MPC_ERR_INFO_VALUE,	/* Value longer than MPI_MAX_INFO_VAL */
	MPC_ERR_INFO_NOKEY,	/* Invalid key passed to MPI_INFO_DELETE */

	/* Error with memory */
	MPC_ERR_NO_MEM,	/* MPI_ALLOC_MEM failed because memory is exhausted */
	MPC_ERR_BASE,	/* Invalid base passed to MPI_FREE_MEM */

	/* Other errors that are not simply an invalid argument */
	MPC_ERR_SPAWN,	/* Error in spawning processes */
	MPC_ERR_RMA_CONFLICT,	/* Conflicting accesses to window */
	MPC_ERR_RMA_SYNC,	/* Wrong synchronization of RMA calls */
	MPC_ERR_UNSUPPORTED_OPERATION,	/* Unsupported operation, such as seeking on a file which \
	supports sequential access only*/
	MPC_ERR_NO_SPACE,	/* Not enough space */
	MPC_ERR_QUOTA,	/* Quota exceeded */
	MPC_ERR_DUP_DATAREP,	/* Conversion functions could not be regiestered because a \
															data representation identifier that was already defined \
	was passed to MPI_REGISTER_DATAREP */
	MPC_ERR_CONVERSION,	/* An error occured in a user supplied data conversion function */
	MPC_ERR_OTHER,	/* Other error; use Error_string */

	MPC_ERR_UNKNOWN,	/* Unknown error */
	MPC_ERR_INTERN,	/* Internal error code    */

	MPCR_ERRORS_THROW_EXCEPTIONS,

	MPC_NOT_IMPLEMENTED,

	MPC_ERR_RMA_FLAVOR,	/* Wrong type of RMA window */

	/* HOLE @,	*/

	MPC_T_ERR_MEMORY,	/* Out of memory */
	MPC_T_ERR_NOT_INITIALIZED,	/* Interface not initialized */
	MPC_T_ERR_CANNOT_INIT,    /* Interface not in the state to \
	be initialized */
	MPC_T_ERR_INVALID_INDEX,		  /* The index is invalid or \
	has been deleted  */
	MPC_T_ERR_INVALID_ITEM,	/* Item index queried is out of range */
	MPC_T_ERR_INVALID_HANDLE,	/* The handle is invalid */
	MPC_T_ERR_OUT_OF_HANDLES,	/* No more handles available */
	MPC_T_ERR_OUT_OF_SESSIONS,	/* No more sessions available */
	MPC_T_ERR_INVALID_SESSION,	/* Session argument is not valid */
	MPC_T_ERR_CVAR_SET_NOT_NOW,	/* Cvar can't be set at this moment */
	MPC_T_ERR_CVAR_SET_NEVER ,		   /* Cvar can't be set until \
	end of execution */
	MPC_T_ERR_PVAR_NO_STARTSTOP,	/* Pvar can't be started or stopped */
	MPC_T_ERR_PVAR_NO_WRITE,	/* Pvar can't be written or reset */
	MPC_T_ERR_PVAR_NO_ATOMIC,	/* Pvar can't be R/W atomically */
	MPC_T_ERR_INVALID_NAME,	/* Requested name is invalid */
	MPC_T_ERR_INVALID,	/* Item is invalid */

	MPC_ERR_RMA_RANGE,
	MPC_ERR_RMA_ATTACH,
	MPC_ERR_RMA_SHARED,
	MPC_ERR_LASTCODE
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

#define MPC_DATATYPE_NULL ( (mpc_lowcomm_datatype_t) -1 )
#define MPC_UB ( (mpc_lowcomm_datatype_t) -2 )
#define MPC_LB ( (mpc_lowcomm_datatype_t) -3 )


/* for comm_split method */
#define MPC_COMM_TYPE_SHARED 1
#define MPC_COMM_TYPE_SOCKET 2
#define MPC_COMM_TYPE_NUMA 3
#define MPC_COMM_TYPE_UNIX_PROCESS 4
#define MPC_COMM_TYPE_HW_UNGUIDED 5
#define MPC_COMM_TYPE_HW_SUBDOMAIN 6
#define MPC_COMM_TYPE_APP 7
#define MPC_COMM_TYPE_NODE 8

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

int mpc_mpi_cl_type_is_contiguous(mpc_lowcomm_datatype_t type);

/********************
 * THREAD MIGRATION *
 ********************/

int mpc_cl_move_to( int process, int cpuid );

#endif /* MPC_COMM_LIB_H_ */
