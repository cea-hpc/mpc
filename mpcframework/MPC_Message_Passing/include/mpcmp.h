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

/* #include "mpcthread.h" */
/* #include "mpcalloc.h" */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

  int PMPC_Main (int argc, char **argv);
  int PMPC_User_Main (int argc, char **argv);

#define MPC_THREAD_SINGLE 0
#define MPC_THREAD_FUNNELED 1
#define MPC_THREAD_SERIALIZED 2
#define  MPC_THREAD_MULTIPLE 3

  typedef int mpc_msg_count;

  typedef unsigned int mpc_pack_indexes_t;
  typedef unsigned long mpc_pack_absolute_indexes_t;

  typedef int MPC_Comm;
  typedef struct
  {
    int task_nb;
    /* Task list rank are valid in COMM_WORLD  */
    int *task_list_in_global_ranks;
  } MPC_Group_t;
  typedef MPC_Group_t *MPC_Group;

  extern const MPC_Group_t mpc_group_empty;
  extern const MPC_Group_t mpc_group_null;

#define MPC_GROUP_EMPTY &mpc_group_empty
#define MPC_GROUP_NULL ((MPC_Group)NULL)

/* #define MPC_STATUS_SIZE 80 */
/* #define MPC_REQUEST_SIZE 30 */

/*   typedef unsigned long MPC_Datatype; */
  typedef unsigned int MPC_Datatype;
  typedef void (*MPC_Op_f) (const void *, void *, size_t, MPC_Datatype);
  typedef void (MPC_User_function) (void *, void *, int *, MPC_Datatype *);

  typedef void (MPC_Handler_function) (MPC_Comm *, int *, ...);
  typedef MPC_Handler_function *MPC_Errhandler;

  typedef struct
  {
    MPC_Op_f func;
    MPC_User_function *u_func;
  } MPC_Op;
#define MPC_OP_INIT {NULL,NULL}

  typedef struct MPC_Header{
    int source;
    int destination;
    int glob_destination;
    int glob_source;
    int message_tag;
    MPC_Comm communicator;
    mpc_msg_count msg_size;
  }MPC_Header;

  struct sctk_thread_ptp_message_s;
  typedef struct
  {
    MPC_Header header;
    volatile int completion_flag;
    struct sctk_thread_ptp_message_s* msg;
    int is_null;
    int need_check_in_wait;
    int request_type;
    int truncated;
  } MPC_Request;

  extern MPC_Request mpc_request_null;
#define MPC_REQUEST_NULL mpc_request_null
#define MPC_COMM_WORLD 0
#define MPC_COMM_SELF 1

#define MPC_SUCCESS 0
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

#define MPC_ERR_LASTCODE               55
#define MPC_NOT_IMPLEMENTED            56

#define MPC_STATUS_IGNORE NULL
#define MPC_STATUSES_IGNORE NULL
#define MPC_ANY_TAG -1
#define MPC_ANY_SOURCE -1
#define MPC_PROC_NULL -2
#define MPC_COMM_NULL ((MPC_Comm)(-1))
#define MPC_MAX_PROCESSOR_NAME 255
#define MPC_ROOT -4

/********************************************************************/
/*Special TAGS                                                      */
/********************************************************************/
#define MPC_ANY_TAG -1
#define MPC_GATHERV_TAG -2
#define MPC_GATHER_TAG -3
#define MPC_SCATTERV_TAG -4
#define MPC_SCATTER_TAG -5
#define MPC_ALLTOALL_TAG -6
#define MPC_ALLTOALLV_TAG -7
#define MPC_BROADCAST_INTERCOMM_TAG -8

  typedef struct
  {
    int MPC_SOURCE;
    int MPC_TAG;
    int MPC_ERROR;
    int cancelled;
    mpc_msg_count count;
  } MPC_Status;
#define MPC_STATUS_INIT {MPC_ANY_SOURCE,MPC_ANY_TAG,MPC_SUCCESS,0,0}

#define MPC_CREATE_INTERN_FUNC(name) extern  const MPC_Op MPC_##name

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
#define MPC_DATATYPE_NULL ((MPC_Datatype)-1)
#define MPC_UB ((MPC_Datatype)-2)
#define MPC_LB ((MPC_Datatype)-3)
#define MPC_CHAR 0
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
#define MPC_LONG_LONG_INT  12
#define MPC_LONG_LONG 12
#define MPC_PACKED 13
#define MPC_FLOAT_INT 14
/*    struct { float, int } */
#define MPC_LONG_INT 15
/*    struct { long, int } */
#define MPC_DOUBLE_INT 16
/*    struct { double, int } */
#define MPC_SHORT_INT 17
/*    struct { short, int } */
#define MPC_2INT 18
/*    struct { int, int }*/
#define MPC_2FLOAT 19
#define MPC_COMPLEX 20
/*    struct { float, float }*/
#define MPC_2DOUBLE_PRECISION 21
/*    struct { double, double }*/
#define MPC_LOGICAL 22
#define MPC_DOUBLE_COMPLEX 23

#define MPC_INTEGER1 24
#define MPC_INTEGER2 25
#define MPC_INTEGER4 26
#define MPC_INTEGER8 27
#define MPC_REAL4 28
#define MPC_REAL8 29
#define MPC_REAL16 30
#define MPC_SIGNED_CHAR 31
#define MPC_LONG_DOUBLE_INT 32
#define MPC_UNSIGNED_LONG_LONG_INT 33
#define MPC_UNSIGNED_LONG_LONG 33
/* for the datatype decoders */
enum MPIR_Combiner_enum {
    MPI_COMBINER_NAMED            = 1,
    MPI_COMBINER_DUP              = 2,
    MPI_COMBINER_CONTIGUOUS       = 3,
    MPI_COMBINER_VECTOR           = 4,
    MPI_COMBINER_HVECTOR_INTEGER  = 5,
    MPI_COMBINER_HVECTOR          = 6,
    MPI_COMBINER_INDEXED          = 7,
    MPI_COMBINER_HINDEXED_INTEGER = 8,
    MPI_COMBINER_HINDEXED         = 9,
    MPI_COMBINER_INDEXED_BLOCK    = 10,
    MPI_COMBINER_HINDEXED_BLOCK   = 11,
    MPI_COMBINER_STRUCT_INTEGER   = 12,
    MPI_COMBINER_STRUCT           = 13,
    MPI_COMBINER_SUBARRAY         = 14,
    MPI_COMBINER_DARRAY           = 15,
    MPI_COMBINER_F90_REAL         = 16,
    MPI_COMBINER_F90_COMPLEX      = 17,
    MPI_COMBINER_F90_INTEGER      = 18,
    MPI_COMBINER_RESIZED          = 19
};

  /*Initialisation */
  int MPC_Init (int *argc, char ***argv);
  int MPC_Init_thread (int *argc, char ***argv, int required, int *provided);
  int MPC_Query_thread(int *provided);
  int MPC_Initialized (int *flag);
  int MPC_Finalize (void);
  int MPC_Abort (MPC_Comm, int);

  typedef enum {MPC_HAVE_OPTION_HLS,MPC_HAVE_OPTION_ETLS_COW,MPC_HAVE_OPTION_ETLS_OPTIMIZED,MPC_HAVE_OPTION_END} MPC_Config_Status_t;

  int MPC_Config_Status(MPC_Config_Status_t option);
  void MPC_Config_Status_Print(FILE* fd);
  char* MPC_Config_Status_Name(MPC_Config_Status_t option);

  /* MPI Topology informations */
  int MPC_Comm_rank (MPC_Comm comm, int *rank);
  int MPC_Comm_size (MPC_Comm comm, int *size);
  int MPC_Comm_remote_size (MPC_Comm comm, int *size);
  int MPC_Comm_test_inter (MPC_Comm comm, int *flag);

  /* Node topology */
  int MPC_Node_rank (int *rank);
  int MPC_Node_number (int *number);

  /* Processors topology */
  int MPC_Processor_rank (int *rank);
  int MPC_Processor_number (int *number);

  /* Process global numbering */
  int MPC_Process_rank (int *rank);
  int MPC_Process_number (int *number);

  /* Process local numbering */
  int MPC_Local_process_rank (int *rank);
  int MPC_Local_process_number (int *number);

  /* Task global topology */
  int MPC_Task_rank( int *rank );
  int MPC_Task_number( int *number );

  /* Task local topology */
  int MPC_Local_task_rank( int *rank );
  int MPC_Local_task_number( int *number );

  int MPC_Get_version (int *version, int *subversion);
  int MPC_Get_multithreading (char *name, int size);
  int MPC_Get_networking (char *name, int size);
  int MPC_Get_processor_name (char *name, int *resultlen);


  /*Collective operations */
  int MPC_Barrier (MPC_Comm comm);
  int MPC_Bcast (void *buffer, mpc_msg_count count, MPC_Datatype datatype,
		 int root, MPC_Comm comm);
  int MPC_Allreduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		     MPC_Datatype datatype, MPC_Op op, MPC_Comm comm);
  int MPC_Reduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		  MPC_Datatype datatype, MPC_Op op, int root, MPC_Comm comm);
  int MPC_Op_create (MPC_User_function *, int, MPC_Op *);
  int MPC_Op_free (MPC_Op *);



  /*P-t-P Communications */
  int MPC_Isend (void *buf, mpc_msg_count count, MPC_Datatype datatype,
		 int dest, int tag, MPC_Comm comm, MPC_Request * request);
  int MPC_Ibsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);
  int MPC_Issend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);
  int MPC_Irsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);

  int MPC_Irecv (void *buf, mpc_msg_count count, MPC_Datatype datatype,
		 int source, int tag, MPC_Comm comm, MPC_Request * request);

  int MPC_Wait (MPC_Request * request, MPC_Status * status);
  int MPC_Waitall (mpc_msg_count, MPC_Request *, MPC_Status *);
  int MPC_Waitsome (mpc_msg_count, MPC_Request *, mpc_msg_count *,
		    mpc_msg_count *, MPC_Status *);
  int MPC_Waitany (mpc_msg_count count, MPC_Request array_of_requests[],
		   mpc_msg_count * index, MPC_Status * status);
  int MPC_Wait_pending (MPC_Comm comm);
  int MPC_Wait_pending_all_comm (void);

  int MPC_Test (MPC_Request *, int *, MPC_Status *);
  int MPC_Test_no_check (MPC_Request *, int *, MPC_Status *);
  int MPC_Test_check (MPC_Request *, int *, MPC_Status *);

  int MPC_Iprobe (int, int, MPC_Comm, int *, MPC_Status *);
  int MPC_Probe (int, int, MPC_Comm, MPC_Status *);
  int MPC_Get_count (MPC_Status *, MPC_Datatype, mpc_msg_count *);

  int MPC_Send (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int MPC_Bsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int MPC_Ssend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int MPC_Rsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int MPC_Recv (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		MPC_Status *);

  int MPC_Sendrecv (void *, mpc_msg_count, MPC_Datatype, int, int, void *,
		    mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		    MPC_Status *);

  /*Status */
  int MPC_Test_cancelled (MPC_Status *, int *);
  int MPC_Cancel (MPC_Request *);

  /*Gather */
  int MPC_Gather (void *, mpc_msg_count, MPC_Datatype, void *,
		  mpc_msg_count, MPC_Datatype, int, MPC_Comm);
  int MPC_Gatherv (void *, mpc_msg_count, MPC_Datatype, void *,
		   mpc_msg_count *, mpc_msg_count *, MPC_Datatype, int,
		   MPC_Comm);
  int MPC_Allgather (void *, mpc_msg_count, MPC_Datatype, void *,
		     mpc_msg_count, MPC_Datatype, MPC_Comm);
  int MPC_Allgatherv (void *, mpc_msg_count, MPC_Datatype, void *,
		      mpc_msg_count *, mpc_msg_count *, MPC_Datatype,
		      MPC_Comm);

  /*Scatter */
  int MPC_Scatter (void *, mpc_msg_count, MPC_Datatype, void *,
		   mpc_msg_count, MPC_Datatype, int, MPC_Comm);
  int MPC_Scatterv (void *, mpc_msg_count *, mpc_msg_count *,
		    MPC_Datatype, void *, mpc_msg_count, MPC_Datatype,
		    int, MPC_Comm);

  /*Alltoall */
  int MPC_Alltoall (void *, mpc_msg_count, MPC_Datatype, void *,
		    mpc_msg_count, MPC_Datatype, MPC_Comm);
  int MPC_Alltoallv (void *, mpc_msg_count *, mpc_msg_count *,
		     MPC_Datatype, void *, mpc_msg_count *,
		     mpc_msg_count *, MPC_Datatype, MPC_Comm);

  /*Informations */
  int MPC_Get_processor_name (char *, int *);

  /*Groups */
  int MPC_Comm_group (MPC_Comm, MPC_Group *);
  int MPC_Comm_remote_group (MPC_Comm, MPC_Group *);
  int MPC_Group_free (MPC_Group *);
  int MPC_Group_incl (MPC_Group, int, int *, MPC_Group *);
  int MPC_Group_difference (MPC_Group, MPC_Group, MPC_Group *);

  /*Communicators */
  int MPC_Convert_to_intercomm (MPC_Comm comm, MPC_Group group);
  int MPC_Comm_create_list (MPC_Comm, int *list, int nb_elem, MPC_Comm *);
  int MPC_Comm_create (MPC_Comm, MPC_Group, MPC_Comm *);
  int MPC_Comm_free (MPC_Comm *);
  int MPC_Comm_dup (MPC_Comm, MPC_Comm *);
  int MPC_Comm_split (MPC_Comm, int, int, MPC_Comm *);

  /*Error_handler */
#define MPC_MAX_ERROR_STRING 512
  void MPC_Default_error (MPC_Comm * comm, int *error, char *msg,
			  char *file, int line);
  void MPC_Return_error (MPC_Comm * comm, int *error, ...);
  int MPC_Errhandler_create (MPC_Handler_function *, MPC_Errhandler *);
  int MPC_Errhandler_set (MPC_Comm, MPC_Errhandler);
  int MPC_Errhandler_get (MPC_Comm, MPC_Errhandler *);
  int MPC_Errhandler_free (MPC_Errhandler *);
  int MPC_Error_string (int, char *, int *);
  int MPC_Error_class (int, int *);

  /*Timing */
  double MPC_Wtime (void);
  double MPC_Wtick (void);

  /*Types */
  int MPC_Type_size (MPC_Datatype, size_t *);
  int MPC_Sizeof_datatype (MPC_Datatype *, size_t);
  int MPC_Type_free (MPC_Datatype * datatype);

  /*MPC specific function */
  int MPC_Copy_in_buffer (void *inbuffer, void *outbuffer, int count,
			  MPC_Datatype datatype);
  int MPC_Copy_from_buffer (void *inbuffer, void *outbuffer, int count,
			    MPC_Datatype datatype);

  int MPC_Derived_datatype (MPC_Datatype * datatype,
			    mpc_pack_absolute_indexes_t * begins,
			    mpc_pack_absolute_indexes_t * ends,
			    unsigned long count,
			    mpc_pack_absolute_indexes_t lb, int is_lb,
			    mpc_pack_absolute_indexes_t ub, int is_ub);

  int MPC_Is_derived_datatype (MPC_Datatype datatype, int *res,
			       mpc_pack_absolute_indexes_t ** begins,
			       mpc_pack_absolute_indexes_t ** ends,
			       unsigned long *count,
			       mpc_pack_absolute_indexes_t * lb, int *is_lb,
			       mpc_pack_absolute_indexes_t * ub, int *is_ub);

  int MPC_Derived_use (MPC_Datatype datatype);
/*   int MPC_Get_keys (void **keys); */
/*   int MPC_Set_keys (void *keys); */
/*   int MPC_Get_requests (void **requests); */
/*   int MPC_Set_requests (void *requests); */
/*   int MPC_Get_groups (void **groups); */
/*   int MPC_Set_groups (void *groups); */
/*   int MPC_Get_errors (void **errors); */
/*   int MPC_Set_errors (void *errors); */
/*   int MPC_Set_buffers (void *buffers); */
/*   int MPC_Get_buffers (void **buffers); */
/*   int MPC_Get_comm_type (void **comm_type); */
/*   int MPC_Set_comm_type (void *comm_type); */
/*   int MPC_Get_op (void **op); */
/*   int MPC_Set_op (void *op); */


  /*Requests */
  int MPC_Request_free (MPC_Request *);

  /*Scheduling */
  int MPC_Proceed (void);
  int MPC_Checkpoint (void);
  int MPC_Checkpoint_timed (unsigned int sec, MPC_Comm comm);
  int MPC_Migrate (void);
  int MPC_Restart (int rank);
  int MPC_Restarted (int *flag);

  typedef struct
  {
    int virtual_cpuid;
    double usage;
  } MPC_Activity_t;

  int MPC_Get_activity (int nb_item, MPC_Activity_t * tab,
			double *process_act);
  int MPC_Move_to (int process, int cpuid);

  /*Packs */
  int MPC_Open_pack (MPC_Request * request);
  int MPC_Default_pack (mpc_msg_count count,
			mpc_pack_indexes_t * begins,
			mpc_pack_indexes_t * ends, MPC_Request * request);
  int
    MPC_Default_pack_absolute (mpc_msg_count count,
			       mpc_pack_absolute_indexes_t * begins,
			       mpc_pack_absolute_indexes_t * ends,
			       MPC_Request * request);
  int MPC_Add_pack (void *buf, mpc_msg_count count,
		    mpc_pack_indexes_t * begins,
		    mpc_pack_indexes_t * ends, MPC_Datatype datatype,
		    MPC_Request * request);
  int
    MPC_Add_pack_absolute (void *buf, mpc_msg_count count,
			   mpc_pack_absolute_indexes_t * begins,
			   mpc_pack_absolute_indexes_t * ends,
			   MPC_Datatype datatype, MPC_Request * request);

  int MPC_Add_pack_default (void *buf, MPC_Datatype datatype,
			    MPC_Request * request);
  int
    MPC_Add_pack_default_absolute (void *buf, MPC_Datatype datatype,
				   MPC_Request * request);
  int MPC_Isend_pack (int dest, int tag, MPC_Comm comm,
		      MPC_Request * request);
  int MPC_Irecv_pack (int source, int tag, MPC_Comm comm,
		      MPC_Request * request);
  void *tmp_malloc (size_t size);

  /*MPI compatibility*/
#define MPC_BSEND_OVERHEAD 0
#define MPC_BOTTOM ((void*)0)
#define MPC_IN_PLACE ((void*)-1)

  typedef long MPC_Aint;


    /* Profiling Functions */

    /*Initialisation */
  int PMPC_Init (int *argc, char ***argv);
  int PMPC_Init_thread (int *argc, char ***argv, int required, int *provided);
  int MPC_Init_thread (int *argc, char ***argv, int required, int *provided);
  int PMPC_Initialized (int *flag);
  int PMPC_Finalize (void);
  int PMPC_Abort (MPC_Comm, int);
  int PMPC_Query_thread (int *provided);

  /*Topology informations */
  int PMPC_Comm_rank (MPC_Comm comm, int *rank);
  int PMPC_Comm_size (MPC_Comm comm, int *size);
  int PMPC_Comm_remote_size (MPC_Comm comm, int *size);
  int PMPC_Comm_test_inter (MPC_Comm comm, int *flag);
  int PMPC_Node_rank (int *rank);
  int PMPC_Node_number (int *number);
  int PMPC_Processor_rank (int *rank);
  int PMPC_Processor_number (int *number);
  int PMPC_Process_rank (int *rank);
  int PMPC_Process_number (int *number);
  int PMPC_Local_process_rank (int *rank);
  int PMPC_Local_process_number (int *rank);
  int PMPC_Get_version (int *version, int *subversion);
  int PMPC_Get_multithreading (char *name, int size);
  int PMPC_Get_networking (char *name, int size);
  int PMPC_Get_processor_name (char *name, int *resultlen);


  /*Collective operations */
  int PMPC_Barrier (MPC_Comm comm);
  int PMPC_Bcast (void *buffer, mpc_msg_count count, MPC_Datatype datatype,
		 int root, MPC_Comm comm);
  int PMPC_Allreduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		     MPC_Datatype datatype, MPC_Op op, MPC_Comm comm);
  int PMPC_Reduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		  MPC_Datatype datatype, MPC_Op op, int root, MPC_Comm comm);
  int PMPC_Op_create (MPC_User_function *, int, MPC_Op *);
  int PMPC_Op_free (MPC_Op *);



  /*P-t-P Communications */
  int PMPC_Isend (void *buf, mpc_msg_count count, MPC_Datatype datatype,
		 int dest, int tag, MPC_Comm comm, MPC_Request * request);
  int PMPC_Ibsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);
  int PMPC_Issend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);
  int PMPC_Irsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		  MPC_Request *);

  int PMPC_Irecv (void *buf, mpc_msg_count count, MPC_Datatype datatype,
		 int source, int tag, MPC_Comm comm, MPC_Request * request);

  int PMPC_Wait (MPC_Request * request, MPC_Status * status);
  int PMPC_Waitall (mpc_msg_count, MPC_Request *, MPC_Status *);
  int PMPC_Waitsome (mpc_msg_count, MPC_Request *, mpc_msg_count *,
		    mpc_msg_count *, MPC_Status *);
  int PMPC_Waitany (mpc_msg_count count, MPC_Request array_of_requests[],
		   mpc_msg_count * index, MPC_Status * status);
  int PMPC_Wait_pending (MPC_Comm comm);
  int PMPC_Wait_pending_all_comm (void);

  int PMPC_Test (MPC_Request *, int *, MPC_Status *);
  int PMPC_Test_no_check (MPC_Request *, int *, MPC_Status *);
  int PMPC_Test_check (MPC_Request *, int *, MPC_Status *);

  int PMPC_Iprobe (int, int, MPC_Comm, int *, MPC_Status *);
  int PMPC_Probe (int, int, MPC_Comm, MPC_Status *);
  int PMPC_Get_count (MPC_Status *, MPC_Datatype, mpc_msg_count *);

  int PMPC_Send (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int PMPC_Bsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int PMPC_Ssend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int PMPC_Rsend (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm);
  int PMPC_Recv (void *, mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		MPC_Status *);

  int PMPC_Sendrecv (void *, mpc_msg_count, MPC_Datatype, int, int, void *,
		    mpc_msg_count, MPC_Datatype, int, int, MPC_Comm,
		    MPC_Status *);

  /*Status */
  int PMPC_Test_cancelled (MPC_Status *, int *);
  int PMPC_Cancel (MPC_Request *);

  /*Gather */
  int PMPC_Gather (void *, mpc_msg_count, MPC_Datatype, void *,
		  mpc_msg_count, MPC_Datatype, int, MPC_Comm);
  int PMPC_Gatherv (void *, mpc_msg_count, MPC_Datatype, void *,
		   mpc_msg_count *, mpc_msg_count *, MPC_Datatype, int,
		   MPC_Comm);
  int PMPC_Allgather (void *, mpc_msg_count, MPC_Datatype, void *,
		     mpc_msg_count, MPC_Datatype, MPC_Comm);
  int PMPC_Allgatherv (void *, mpc_msg_count, MPC_Datatype, void *,
		      mpc_msg_count *, mpc_msg_count *, MPC_Datatype,
		      MPC_Comm);

  /*Scatter */
  int PMPC_Scatter (void *, mpc_msg_count, MPC_Datatype, void *,
		   mpc_msg_count, MPC_Datatype, int, MPC_Comm);
  int PMPC_Scatterv (void *, mpc_msg_count *, mpc_msg_count *,
		    MPC_Datatype, void *, mpc_msg_count, MPC_Datatype,
		    int, MPC_Comm);

  /*Alltoall */
  int PMPC_Alltoall (void *, mpc_msg_count, MPC_Datatype, void *,
		    mpc_msg_count, MPC_Datatype, MPC_Comm);
  int PMPC_Alltoallv (void *, mpc_msg_count *, mpc_msg_count *,
		     MPC_Datatype, void *, mpc_msg_count *,
		     mpc_msg_count *, MPC_Datatype, MPC_Comm);
  int PMPC_Alltoallw (const void *, const mpc_msg_count *, const mpc_msg_count *,
		     const MPC_Datatype *, void *, const mpc_msg_count *,
		     const mpc_msg_count *, const MPC_Datatype *, MPC_Comm);

  /*Groups */
  int PMPC_Comm_group (MPC_Comm, MPC_Group *);
  int PMPC_Comm_remote_group (MPC_Comm, MPC_Group *);
  int PMPC_Group_free (MPC_Group *);
  int PMPC_Group_incl (MPC_Group, int, int *, MPC_Group *);
  int PMPC_Group_difference (MPC_Group, MPC_Group, MPC_Group *);

  /*Communicators */
  int PMPC_Convert_to_intercomm (MPC_Comm comm, MPC_Group group);
  int PMPC_Comm_create_list (MPC_Comm, int *list, int nb_elem, MPC_Comm *);
  int PMPC_Comm_create (MPC_Comm, MPC_Group, MPC_Comm *);
  int PMPC_Intercomm_create (MPC_Comm local_comm, int local_leader, MPC_Comm peer_comm, int remote_leader, int tag, MPC_Comm * newintercomm);
  int PMPC_Comm_create_from_intercomm (MPC_Comm, MPC_Group, MPC_Comm *);
  int PMPC_Comm_free (MPC_Comm *);
  int PMPC_Comm_dup (MPC_Comm, MPC_Comm *);
  int PMPC_Comm_split (MPC_Comm, int, int, MPC_Comm *);

  /*Error_handler */
  void PMPC_Default_error (MPC_Comm * comm, int *error, char *msg,
			  char *file, int line);
  void PMPC_Return_error (MPC_Comm * comm, int *error, ...);
#define MPC_ERRHANDLER_NULL ((MPC_Handler_function *)PMPC_Default_error)
#define MPC_ERRORS_RETURN ((MPC_Handler_function *)PMPC_Return_error)
#define MPC_ERRORS_ARE_FATAL ((MPC_Handler_function *)PMPC_Default_error)
  int PMPC_Errhandler_create (MPC_Handler_function *, MPC_Errhandler *);
  int PMPC_Errhandler_set (MPC_Comm, MPC_Errhandler);
  int PMPC_Errhandler_get (MPC_Comm, MPC_Errhandler *);
  int PMPC_Errhandler_free (MPC_Errhandler *);
  int PMPC_Error_string (int, char *, int *);
  int PMPC_Error_class (int, int *);

  /*Timing */
  double PMPC_Wtime (void);
  double PMPC_Wtick (void);

  /*Types */
  int PMPC_Type_size (MPC_Datatype, size_t *);
  int PMPC_Sizeof_datatype (MPC_Datatype *, size_t, size_t count, MPC_Datatype *data_in);
  int __MPC_Barrier (MPC_Comm comm);
  int PMPC_Type_free (MPC_Datatype * datatype);

  /*MPC specific function */
  int PMPC_Copy_in_buffer (void *inbuffer, void *outbuffer, int count,
			  MPC_Datatype datatype);
  int PMPC_Copy_from_buffer (void *inbuffer, void *outbuffer, int count,
			    MPC_Datatype datatype);

  int PMPC_Derived_datatype (MPC_Datatype * datatype,
			    mpc_pack_absolute_indexes_t * begins,
			    mpc_pack_absolute_indexes_t * ends,
			    unsigned long count,
			    mpc_pack_absolute_indexes_t lb, int is_lb,
			    mpc_pack_absolute_indexes_t ub, int is_ub);

  int PMPC_Is_derived_datatype (MPC_Datatype datatype, int *res,
			       mpc_pack_absolute_indexes_t ** begins,
			       mpc_pack_absolute_indexes_t ** ends,
			       unsigned long *count,
			       mpc_pack_absolute_indexes_t * lb, int *is_lb,
			       mpc_pack_absolute_indexes_t * ub, int *is_ub);

  int PMPC_Derived_use (MPC_Datatype datatype);
/*   int PMPC_Get_keys (void **keys); */
/*   int PMPC_Set_keys (void *keys); */
/*   int PMPC_Get_requests (void **requests); */
/*   int PMPC_Set_requests (void *requests); */
/*   int PMPC_Get_groups (void **groups); */
/*   int PMPC_Set_groups (void *groups); */
/*   int PMPC_Get_errors (void **errors); */
/*   int PMPC_Set_errors (void *errors); */
/*   int PMPC_Set_buffers (void *buffers); */
/*   int PMPC_Get_buffers (void **buffers); */
/*   int PMPC_Get_comm_type (void **comm_type); */
/*   int PMPC_Set_comm_type (void *comm_type); */
/*   int PMPC_Get_op (void **op); */
/*   int PMPC_Set_op (void *op); */


  /*Requests */
  int PMPC_Request_free (MPC_Request *);

  /*Scheduling */
  int PMPC_Proceed (void);
  int PMPC_Checkpoint (void);
  int PMPC_Checkpoint_timed (unsigned int sec, MPC_Comm comm);
  int PMPC_Migrate (void);
  int PMPC_Restart (int rank);
  int PMPC_Restarted (int *flag);

  int PMPC_Get_activity (int nb_item, MPC_Activity_t * tab,
			double *process_act);
  int PMPC_Move_to (int process, int cpuid);

  /*Packs */
  int PMPC_Open_pack (MPC_Request * request);
  int PMPC_Default_pack (mpc_msg_count count,
			mpc_pack_indexes_t * begins,
			mpc_pack_indexes_t * ends, MPC_Request * request);
  int PMPC__Default_pack_absolute (mpc_msg_count count,
			       mpc_pack_absolute_indexes_t * begins,
			       mpc_pack_absolute_indexes_t * ends,
			       MPC_Request * request);
  int PMPC_Add_pack (void *buf, mpc_msg_count count,
		    mpc_pack_indexes_t * begins,
		    mpc_pack_indexes_t * ends, MPC_Datatype datatype,
		    MPC_Request * request);
  int PMPC_Add_pack_absolute (void *buf, mpc_msg_count count,
			   mpc_pack_absolute_indexes_t * begins,
			   mpc_pack_absolute_indexes_t * ends,
			   MPC_Datatype datatype, MPC_Request * request);

  int PMPC_Add_pack_default (void *buf, MPC_Datatype datatype,
			    MPC_Request * request);
  int PMPC__Add_pack_default_absolute (void *buf, MPC_Datatype datatype,
				   MPC_Request * request);
  int PMPC_Isend_pack (int dest, int tag, MPC_Comm comm,
		      MPC_Request * request);
  int PMPC_Irecv_pack (int source, int tag, MPC_Comm comm,
		      MPC_Request * request);

  /* Send/Recv message using the signalization network */
  void MPC_Send_signalization_network(int dest_process, int tag, void *buff, size_t size);
  void MPC_Recv_signalization_network(int src_process, int tag, void *buff, size_t size);

#ifdef __cplusplus
}
#endif
#endif
