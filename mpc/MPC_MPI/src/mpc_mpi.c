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
#include <mpc.h>
#include <mpc_mpi.h>
#include <sctk_debug.h>
#include "mpc_reduction.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "mpcthread.h"
#include "sctk_inter_thread_comm.h"
#include "mpc_common.h"
#include "sctk_spinlock.h"
#include "sctk_communicator.h"
#include "sctk_alloc.h"
#include <string.h>

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_mpi_weak.h"
#endif

static int __INTERNAL__PMPI_Attr_set_fortran (int keyval);

static char *
sctk_char_fortran_to_c (char *buf, long int size)
{
  char *tmp;
  long int i;
  tmp = sctk_malloc (size + 1);

  for (i = 0; i < size; i++)
    {
      tmp[i] = buf[i];
    }
  tmp[i] = '\0';
  return tmp;
}

static void
sctk_char_c_to_fortran (char *buf, long int size)
{
  long int i;
  for (i = strlen (buf); i < size; i++)
    {
      buf[i] = ' ';
    }
}

#if defined(SCTK_USE_CHAR_MIXED)
#define SCTK_CHAR_END(size)
#define SCTK_CHAR_MIXED(size)  ,long int size
#else
#define SCTK_CHAR_END(size) ,long int size
#define SCTK_CHAR_MIXED(size)
#endif

#undef ffunc
#define ffunc(a) a##_
#include <mpc_mpi_fortran.h>

#undef ffunc
#define ffunc(a) a##__
#include <mpc_mpi_fortran.h>

#undef ffunc
/*
  INTERNAL FUNCTIONS
*/
static int __INTERNAL__PMPI_Send (void *, int, MPI_Datatype, int, int,
				  MPI_Comm);
static int __INTERNAL__PMPI_Recv (void *, int, MPI_Datatype, int, int,
				  MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Get_count (MPI_Status *, MPI_Datatype, int *);
static int __INTERNAL__PMPI_Bsend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Ssend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Rsend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm);
static int __INTERNAL__PMPI_Buffer_attach (void *, int);
static int __INTERNAL__PMPI_Buffer_detach (void *, int *);
static int __INTERNAL__PMPI_Isend (void *, int, MPI_Datatype, int, int,
				   MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Ibsend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Issend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Irsend (void *, int, MPI_Datatype, int, int,
				    MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Irecv (void *, int, MPI_Datatype, int, int,
				   MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Wait (MPI_Request *, MPI_Status *);
static int __INTERNAL__PMPI_Test (MPI_Request *, int *, MPI_Status *);
static int __INTERNAL__PMPI_Request_free (MPI_Request *);
static int __INTERNAL__PMPI_Waitany (int, MPI_Request *, int *, MPI_Status *);
static int __INTERNAL__PMPI_Testany (int, MPI_Request *, int *, int *,
				     MPI_Status *);
static int __INTERNAL__PMPI_Waitall (int, MPI_Request *, MPI_Status *);
static int __INTERNAL__PMPI_Testall (int, MPI_Request *, int *, MPI_Status *);
static int __INTERNAL__PMPI_Waitsome (int, MPI_Request *, int *, int *,
				      MPI_Status *);
static int __INTERNAL__PMPI_Testsome (int, MPI_Request *, int *, int *,
				      MPI_Status *);
static int __INTERNAL__PMPI_Iprobe (int, int, MPI_Comm, int *, MPI_Status *);
static int __INTERNAL__PMPI_Probe (int, int, MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Cancel (MPI_Request *);
static int __INTERNAL__PMPI_Test_cancelled (MPI_Status *, int *);
static int __INTERNAL__PMPI_Send_init (void *, int, MPI_Datatype, int, int,
				       MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Bsend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Ssend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Rsend_init (void *, int, MPI_Datatype, int, int,
					MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Recv_init (void *, int, MPI_Datatype, int, int,
				       MPI_Comm, MPI_Request *);
static int __INTERNAL__PMPI_Start (MPI_Request *);
static int __INTERNAL__PMPI_Startall (int, MPI_Request *);
static int __INTERNAL__PMPI_Sendrecv (void *, int, MPI_Datatype, int, int,
				      void *, int, MPI_Datatype, int, int,
				      MPI_Comm, MPI_Status *);
static int __INTERNAL__PMPI_Sendrecv_replace (void *, int, MPI_Datatype, int,
					      int, int, int, MPI_Comm,
					      MPI_Status *);
static int __INTERNAL__PMPI_Type_contiguous (int, MPI_Datatype,
					     MPI_Datatype *);
static int __INTERNAL__PMPI_Type_vector (int, int, int, MPI_Datatype,
					 MPI_Datatype *);
static int __INTERNAL__PMPI_Type_hvector (int, int, MPI_Aint, MPI_Datatype,
					  MPI_Datatype *);
static int __INTERNAL__PMPI_Type_indexed (int, int *, int *, MPI_Datatype,
					  MPI_Datatype *);
static int __INTERNAL__PMPI_Type_hindexed (int, int *, MPI_Aint *,
					   MPI_Datatype, MPI_Datatype *);
static int __INTERNAL__PMPI_Type_struct (int, int *, MPI_Aint *,
					 MPI_Datatype *, MPI_Datatype *);
static int __INTERNAL__PMPI_Address (void *, MPI_Aint *);
static int __INTERNAL__PMPI_Type_extent (MPI_Datatype, MPI_Aint *);
static int __INTERNAL__PMPI_Type_size (MPI_Datatype, int *);
static int __INTERNAL__PMPI_Type_lb (MPI_Datatype, MPI_Aint *);
static int __INTERNAL__PMPI_Type_ub (MPI_Datatype, MPI_Aint *);
static int __INTERNAL__PMPI_Type_commit (MPI_Datatype *);
static int __INTERNAL__PMPI_Type_free (MPI_Datatype *);
static int __INTERNAL__PMPI_Get_elements (MPI_Status *, MPI_Datatype, int *);
static int __INTERNAL__PMPI_Pack (void *, int, MPI_Datatype, void *, int,
				  int *, MPI_Comm);
static int __INTERNAL__PMPI_Unpack (void *, int, int *, void *, int,
				    MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Pack_size (int, MPI_Datatype, MPI_Comm, int *);
static int __INTERNAL__PMPI_Barrier (MPI_Comm);
static int __INTERNAL__PMPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
static int __INTERNAL__PMPI_Gather (void *, int, MPI_Datatype, void *, int,
				    MPI_Datatype, int, MPI_Comm);
static int __INTERNAL__PMPI_Gatherv (void *, int, MPI_Datatype, void *, int *,
				     int *, MPI_Datatype, int, MPI_Comm);
static int __INTERNAL__PMPI_Scatter (void *, int, MPI_Datatype, void *, int,
				     MPI_Datatype, int, MPI_Comm);
static int __INTERNAL__PMPI_Scatterv (void *, int *, int *, MPI_Datatype,
				      void *, int, MPI_Datatype, int,
				      MPI_Comm);
static int __INTERNAL__PMPI_Allgather (void *, int, MPI_Datatype, void *, int,
				       MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Allgatherv (void *, int, MPI_Datatype, void *,
					int *, int *, MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Alltoall (void *, int, MPI_Datatype, void *, int,
				      MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Alltoallv (void *, int *, int *, MPI_Datatype,
				       void *, int *, int *, MPI_Datatype,
				       MPI_Comm);
static int __INTERNAL__PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op,
				    int, MPI_Comm);
static int __INTERNAL__PMPI_Op_create (MPI_User_function *, int, MPI_Op *);
static int __INTERNAL__PMPI_Op_free (MPI_Op *);
static int __INTERNAL__PMPI_Allreduce (void *, void *, int, MPI_Datatype,
				       MPI_Op, MPI_Comm);
static int __INTERNAL__PMPI_Reduce_scatter (void *, void *, int *,
					    MPI_Datatype, MPI_Op, MPI_Comm);
static int __INTERNAL__PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op,
				  MPI_Comm);
static int __INTERNAL__PMPI_Group_size (MPI_Group, int *);
static int __INTERNAL__PMPI_Group_rank (MPI_Group, int *);
static int __INTERNAL__PMPI_Group_translate_ranks (MPI_Group, int, int *,
						   MPI_Group, int *);
static int __INTERNAL__PMPI_Group_compare (MPI_Group, MPI_Group, int *);
static int __INTERNAL__PMPI_Comm_group (MPI_Comm, MPI_Group *);
static int __INTERNAL__PMPI_Group_union (MPI_Group, MPI_Group, MPI_Group *);
static int __INTERNAL__PMPI_Group_intersection (MPI_Group, MPI_Group,
						MPI_Group *);
static int __INTERNAL__PMPI_Group_difference (MPI_Group, MPI_Group,
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_incl (MPI_Group, int, int *, MPI_Group *);
static int __INTERNAL__PMPI_Group_excl (MPI_Group, int, int *, MPI_Group *);
static int __INTERNAL__PMPI_Group_range_incl (MPI_Group, int, int[][3],
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_range_excl (MPI_Group, int, int[][3],
					      MPI_Group *);
static int __INTERNAL__PMPI_Group_free (MPI_Group *);
static int __INTERNAL__PMPI_Comm_size (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_rank (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_compare (MPI_Comm, MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_dup (MPI_Comm, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_create (MPI_Comm, MPI_Group, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_split (MPI_Comm, int, int, MPI_Comm *);
static int __INTERNAL__PMPI_Comm_free (MPI_Comm *);
static int __INTERNAL__PMPI_Comm_test_inter (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_remote_size (MPI_Comm, int *);
static int __INTERNAL__PMPI_Comm_remote_group (MPI_Comm, MPI_Group *);
static int __INTERNAL__PMPI_Intercomm_create (MPI_Comm, int, MPI_Comm, int,
					      int, MPI_Comm *);
static int __INTERNAL__PMPI_Intercomm_merge (MPI_Comm, int, MPI_Comm *);
static int __INTERNAL__PMPI_Keyval_create (MPI_Copy_function *,
					   MPI_Delete_function *, int *,
					   void *);
static int __INTERNAL__PMPI_Keyval_free (int *);
static int __INTERNAL__PMPI_Attr_put (MPI_Comm, int, void *);
static int __INTERNAL__PMPI_Attr_get (MPI_Comm, int, void *, int *);
static int __INTERNAL__PMPI_Attr_delete (MPI_Comm, int);
static int __INTERNAL__PMPI_Topo_test (MPI_Comm, int *);
static int __INTERNAL__PMPI_Cart_create (MPI_Comm, int, int *, int *, int,
					 MPI_Comm *);
static int __INTERNAL__PMPI_Dims_create (int, int, int *);
static int __INTERNAL__PMPI_Graph_create (MPI_Comm, int, int *, int *, int,
					  MPI_Comm *);
static int __INTERNAL__PMPI_Graphdims_get (MPI_Comm, int *, int *);
static int __INTERNAL__PMPI_Graph_get (MPI_Comm, int, int, int *, int *);
static int __INTERNAL__PMPI_Cartdim_get (MPI_Comm, int *);
static int __INTERNAL__PMPI_Cart_get (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Cart_rank (MPI_Comm, int *, int *, int);
static int __INTERNAL__PMPI_Cart_coords (MPI_Comm, int, int, int *, int);
static int __INTERNAL__PMPI_Graph_neighbors_count (MPI_Comm, int, int *);
static int __INTERNAL__PMPI_Graph_neighbors (MPI_Comm, int, int, int *);
static int __INTERNAL__PMPI_Cart_shift (MPI_Comm, int, int, int *, int *);
static int __INTERNAL__PMPI_Cart_sub (MPI_Comm, int *, MPI_Comm *);
static int __INTERNAL__PMPI_Cart_map (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Graph_map (MPI_Comm, int, int *, int *, int *);
static int __INTERNAL__PMPI_Get_processor_name (char *, int *);
static int __INTERNAL__PMPI_Get_version (int *, int *);
static int __INTERNAL__PMPI_Errhandler_create (MPI_Handler_function *,
					       MPI_Errhandler *);
static int __INTERNAL__PMPI_Errhandler_set (MPI_Comm, MPI_Errhandler);
static int __INTERNAL__PMPI_Errhandler_get (MPI_Comm, MPI_Errhandler *);
static int __INTERNAL__PMPI_Errhandler_free (MPI_Errhandler *);
static int __INTERNAL__PMPI_Error_string (int, char *, int *);
static int __INTERNAL__PMPI_Error_class (int, int *);
static double __INTERNAL__PMPI_Wtime (void);
static double __INTERNAL__PMPI_Wtick (void);
static int __INTERNAL__PMPI_Init (int *, char ***);
static int __INTERNAL__PMPI_Finalize (void);
static int __INTERNAL__PMPI_Initialized (int *);
static int __INTERNAL__PMPI_Abort (MPI_Comm, int);

  /* MPI-2 functions */
static int __INTERNAL__PMPI_Comm_get_name (MPI_Comm, char *, int *);
static int __INTERNAL__PMPI_Comm_set_name (MPI_Comm, char *);
static int __INTERNAL__PMPI_Init_thread (int *, char ***, int, int *);

static inline int
sctk_is_derived_type (MPI_Datatype data_in)
{
  if ((data_in >= sctk_user_data_types + sctk_user_data_types_max) &&
      (data_in < sctk_user_data_types + 2 * sctk_user_data_types_max))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

/*
  ERRORS HANDELING
*/
typedef struct
{
  MPI_Handler_function *func;
  int status;
} MPI_Handler_user_function_t;

typedef struct
{
  MPI_Handler_function *func[SCTK_MAX_COMMUNICATOR_NUMBER];
  int func_ident[SCTK_MAX_COMMUNICATOR_NUMBER];
  mpc_thread_mutex_t lock;
  MPI_Handler_user_function_t *user_func;
  int user_func_nb;
} mpi_errors_handler_t;

static inline int
SCTK__MPI_ERROR_REPORT__ (MPC_Comm comm, int error, char *message, char *file,
			  int line)
{
  MPI_Comm comm_id;
  int error_id;
  MPI_Handler_function *func;
  mpi_errors_handler_t *task_specific;
  PMPC_Get_errors ((void **) &task_specific);

  sctk_thread_mutex_lock (&(task_specific->lock));
  comm_id = comm;
  if (!(comm_id < SCTK_MAX_COMMUNICATOR_NUMBER))
    comm_id = MPI_COMM_WORLD;
  error_id = error;
  func = task_specific->func[comm_id];
  func (&comm_id, &error_id, message, file, line);
  sctk_thread_mutex_unlock (&(task_specific->lock));

  return error;
}

static void
__sctk_init_mpi_errors ()
{
  int i;
  mpi_errors_handler_t *task_specific;
  task_specific = sctk_malloc (sizeof (mpi_errors_handler_t));
  sctk_thread_mutex_init (&(task_specific->lock), NULL);
  task_specific->user_func =
    sctk_malloc (3 * sizeof (MPI_Handler_user_function_t));
  task_specific->user_func_nb = 3;

  task_specific->user_func[MPI_ERRHANDLER_NULL].func =
    (MPI_Handler_function *) MPI_Default_error;
  task_specific->user_func[MPI_ERRHANDLER_NULL].status = 1;
  task_specific->user_func[MPI_ERRORS_RETURN].func =
    (MPI_Handler_function *) MPI_Return_error;
  task_specific->user_func[MPI_ERRORS_RETURN].status = 1;
  task_specific->user_func[MPI_ERRORS_ARE_FATAL].func =
    (MPI_Handler_function *) MPI_Default_error;
  task_specific->user_func[MPI_ERRORS_ARE_FATAL].status = 1;

  for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
    {
      task_specific->func[i] =
	task_specific->user_func[MPI_ERRORS_ARE_FATAL].func;
      task_specific->func_ident[i] = MPI_ERRORS_ARE_FATAL;
    }


  PMPC_Set_errors (task_specific);
}

#define MPI_ERROR_REPORT(comm, error,message) return SCTK__MPI_ERROR_REPORT__(comm, error,message,__FILE__, __LINE__)

#define MPI_ERROR_SUCESS() return MPI_SUCCESS

#define mpi_check_type(datatype,comm)		\
  if ((datatype >= sctk_user_data_types_max) && (sctk_is_derived_type(datatype) != 1)) \
    MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "");

#define mpi_check_comm(com,comm)		\
  if(!((com < SCTK_MAX_COMMUNICATOR_NUMBER)))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"")

#define mpi_check_status(status,comm)		\
  if(status == MPI_STATUS_IGNORE)	\
    MPI_ERROR_REPORT(comm,MPI_ERR_IN_STATUS,"")

#define mpi_check_buf(buf,comm)					\
  if((buf == NULL) && (buf != MPI_BOTTOM))					\
    MPI_ERROR_REPORT(comm,MPI_ERR_BUFFER,"")

#define mpi_check_count(count,comm)				\
  if(count < 0)							\
    MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"")

#define mpi_check_rank(task,max_rank,comm)		\
  if(((task < 0) || (task >= max_rank)) && (task != MPI_ANY_SOURCE) && (task != MPI_PROC_NULL))		\
    MPI_ERROR_REPORT(comm,MPI_ERR_RANK,"")

#define mpi_check_tag(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)) && (tag != MPI_ANY_TAG))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"")

#define mpi_check_tag_send(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"")

static int SCTK__MPI_Attr_clean_communicator (MPI_Comm comm);
static int SCTK__MPI_Attr_communicator_dup (MPI_Comm old, MPI_Comm new);

const MPI_Comm MPI_COMM_SELF = MPC_COMM_SELF;

/*
  Requests
*/

typedef enum
{ Send_init, Bsend_init, Rsend_init, Ssend_init,
  Recv_init
} MPI_Persistant_op_t;

typedef struct MPI_Persistant_s
{
  void *buf;
  int count;
  MPI_Datatype datatype;
  int dest_source;
  int tag;
  MPI_Comm comm;
  MPI_Persistant_op_t op;
} MPI_Persistant_t;

typedef struct MPI_internal_request_s
{
  MPC_Request req;
  int used;
  volatile struct MPI_internal_request_s *next;
  int rank;

  /*Persitant */
  MPI_Persistant_t persistant;
  int freeable;
  int is_active;

  /*Datatypes */
  void *saved_datatype;
} MPI_internal_request_t;

typedef struct
{
  sctk_spinlock_t lock;
  /*Number of resquests*/
  int max_size;
  MPI_internal_request_t **tab;
  volatile MPI_internal_request_t *free_list;
  sctk_alloc_buffer_t buf;
} MPI_request_struct_t;


static inline void
__sctk_init_mpc_request ()
{
  static sctk_thread_mutex_t sctk_request_lock =
    SCTK_THREAD_MUTEX_INITIALIZER;
  MPI_request_struct_t *requests;
  PMPC_Get_requests ((void *) &requests);
  assume (requests == NULL);

  sctk_thread_mutex_lock (&sctk_request_lock);
  PMPC_Get_requests ((void *) &requests);
  requests = sctk_malloc (sizeof (MPI_request_struct_t));

  /*Init request struct */
  requests->lock = 0;
  requests->tab = NULL;
  requests->free_list = NULL;
  requests->max_size = 0;

  sctk_buffered_alloc_create (&(requests->buf),
			      sizeof (MPI_internal_request_t));

  PMPC_Set_requests (requests);
  sctk_thread_mutex_unlock (&sctk_request_lock);
}

static inline MPI_internal_request_t *
__sctk_new_mpc_request_internal (MPI_Request * req)
{
  MPI_request_struct_t *requests;
  MPI_internal_request_t *tmp;

  PMPC_Get_requests ((void *) &requests);

  sctk_spinlock_lock (&(requests->lock));
  if (requests->free_list == NULL)
    {
      int old_size;
      int i;
      old_size = requests->max_size;
      requests->max_size += 10;
      requests->tab =
	sctk_realloc (requests->tab,
		      requests->max_size * sizeof (MPI_internal_request_t *));
      assume (requests->tab != NULL);
      for (i = old_size; i < requests->max_size; i++)
	{
	  MPI_internal_request_t *tmp;
	  sctk_nodebug ("%lu", i);
/* 	  tmp = sctk_malloc(sizeof (MPI_internal_request_t)); */
	  tmp =
	    sctk_buffered_malloc (&(requests->buf),
				  sizeof (MPI_internal_request_t));
	  assume (tmp != NULL);
	  requests->tab[i] = tmp;
	  tmp->used = 0;
	  tmp->next = requests->free_list;
	  requests->free_list = tmp;
	  tmp->rank = i;
	  tmp->saved_datatype = NULL;
	}
    }
  tmp = (MPI_internal_request_t *) requests->free_list;
  tmp->used = 1;
  tmp->freeable = 1;
  tmp->is_active = 1;
  tmp->saved_datatype = NULL;
  requests->free_list = tmp->next;
  sctk_spinlock_unlock (&(requests->lock));
  *req = tmp->rank;
  sctk_nodebug ("Create request %d", *req);

  memset (&(tmp->req), 0, sizeof (MPC_Request));

  return tmp;
}

static inline MPC_Request *
__sctk_new_mpc_request (MPI_Request * req)
{
  MPI_internal_request_t *tmp;
  tmp = __sctk_new_mpc_request_internal (req);
  return &(tmp->req);
}

static inline MPI_internal_request_t *
__sctk_convert_mpc_request_internal (MPI_Request * req)
{
  MPI_internal_request_t *tmp;
  MPI_request_struct_t *requests;
  int int_req;

  int_req = *req;
  if (int_req == MPI_REQUEST_NULL)
    {
      return NULL;
    }


  PMPC_Get_requests ((void *) &requests);

  assume (requests != NULL);

  sctk_spinlock_lock (&(requests->lock));
  sctk_nodebug ("Convert request %d", *req);
  assume (((int_req) >= 0) && ((int_req) < requests->max_size));
  tmp = requests->tab[int_req];
  assume (tmp->rank == *req);
  assume (tmp->used);
  sctk_spinlock_unlock (&(requests->lock));
  return tmp;
}
static inline MPC_Request *
__sctk_convert_mpc_request (MPI_Request * req)
{
  MPI_internal_request_t *tmp;

  tmp = __sctk_convert_mpc_request_internal (req);
  if ((tmp == NULL) || (tmp->is_active == 0))
    {
      return &MPC_REQUEST_NULL;
    }
  return &(tmp->req);
}

static inline void
__sctk_add_in_mpc_request (MPI_Request * req, void *t)
{
  MPI_internal_request_t *tmp;
  tmp = __sctk_convert_mpc_request_internal (req);
  tmp->saved_datatype = t;
}

static inline void
__sctk_delete_mpc_request (MPI_Request * req)
{
  MPI_request_struct_t *requests;
  MPI_internal_request_t *tmp;
  int int_req;

  int_req = *req;
  if (int_req == MPI_REQUEST_NULL)
    {
      return;
    }

  PMPC_Get_requests ((void *) &requests);
  assume (requests != NULL);

  sctk_nodebug ("Delete request %d", *req);
  sctk_spinlock_lock (&(requests->lock));
  assume (((*req) >= 0) && ((*req) < requests->max_size));
  tmp = requests->tab[*req];
  tmp->is_active = 0;
  if (tmp->freeable == 1)
    {
      assume (tmp->rank == *req);
      tmp->used = 0;
      sctk_free (tmp->saved_datatype);
      tmp->saved_datatype = NULL;
      tmp->next = requests->free_list;
      requests->free_list = tmp;
      *req = MPI_REQUEST_NULL;
    }
  sctk_spinlock_unlock (&(requests->lock));
}

static int
__INTERNAL__PMPI_Send (void *buf, int count, MPI_Datatype datatype, int dest,
		       int tag, MPI_Comm comm)
{
  sctk_nodebug ("SEND buf %p", buf);
  if (sctk_is_derived_type (datatype))
    {
      int res;

      sctk_nodebug ("Send derived type");

      if (count > 1)
	{
	  MPI_Datatype new_datatype;
	  res =
	    __INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_commit (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Send (buf, 1, new_datatype, dest, tag, comm);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_free (&new_datatype);
	  return res;
	}
      else
	{
	  mpc_pack_absolute_indexes_t *begins;
	  mpc_pack_absolute_indexes_t *ends;
	  unsigned long slots_count;
	  MPC_Request request;
	  MPC_Status status;
	  mpc_pack_absolute_indexes_t lb;
	  int is_lb;
	  mpc_pack_absolute_indexes_t ub;
	  int is_ub;

	  res =
	    PMPC_Is_derived_datatype (datatype, &res, &begins, &ends,
				     &slots_count, &lb, &is_lb, &ub, &is_ub);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = PMPC_Open_pack (&request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res =
	    PMPC_Add_pack_absolute (buf, slots_count, begins, ends, MPC_CHAR,
				   &request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res = PMPC_Isend_pack (dest, tag, comm, &request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = PMPC_Wait (&request, &status);
	  return res;
	}
    }
  else
    {
      sctk_nodebug ("Send contiguous type");
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
      return PMPC_Send (buf, count, datatype, dest, tag, comm);
    }
}
static int
__INTERNAL__PMPI_Recv (void *buf, int count, MPI_Datatype datatype,
		       int source, int tag, MPI_Comm comm,
		       MPI_Status * status)
{
  sctk_nodebug ("RECV buf %p", buf);
  if (sctk_is_derived_type (datatype))
    {
      sctk_nodebug ("Recv derived type");
      int res;

      if (count > 1)
	{
	  MPI_Datatype new_datatype;
	  res =
	    __INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_commit (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res =
	    __INTERNAL__PMPI_Recv (buf, 1, new_datatype, source, tag, comm,
				   status);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_free (&new_datatype);
	  return res;
	}
      else
	{
	  mpc_pack_absolute_indexes_t *begins;
	  mpc_pack_absolute_indexes_t *ends;
	  unsigned long slots_count;
	  MPC_Request request;
	  mpc_pack_absolute_indexes_t lb;
	  int is_lb;
	  mpc_pack_absolute_indexes_t ub;
	  int is_ub;

	  res =
	    PMPC_Is_derived_datatype (datatype, &res, &begins, &ends,
				     &slots_count, &lb, &is_lb, &ub, &is_ub);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = PMPC_Open_pack (&request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res =
	    PMPC_Add_pack_absolute (buf, slots_count, begins, ends, MPC_CHAR,
				   &request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res = PMPC_Irecv_pack (source, tag, comm, &request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = PMPC_Wait (&request, status);
	  return res;
	}
    }
  else
    {
      sctk_nodebug ("Recv contiguous type");
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
      return PMPC_Recv (buf, count, datatype, source, tag, comm, status);
    }
}
static int
__INTERNAL__PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype,
			    int *count)
{
  int res = MPI_SUCCESS;
  unsigned long size;
  int data_size;

  if (status == MPC_STATUS_IGNORE)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_IN_STATUS, "Invalid status");
    }

  res = __INTERNAL__PMPI_Type_size (datatype, &data_size);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  if (data_size != 0)
    {
      size = status->count;
      sctk_nodebug ("%d count %d data_size", size, data_size);
      if (size % data_size == 0)
	{
	  size = size / data_size;
	  *count = size;
	}
      else
	{
	  *count = MPI_UNDEFINED;
	}
    }
  else
    {
      *count = 0;
    }

  return res;
}

static int
__INTERNAL__PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  int res;
  MPI_Request request;
  res =
    __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, &request);
  return res;
}
static int
__INTERNAL__PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_is_derived_type (datatype))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
      return PMPC_Ssend (buf, count, datatype, dest, tag, comm);
    }
}
static int
__INTERNAL__PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_is_derived_type (datatype))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
      return PMPC_Rsend (buf, count, datatype, dest, tag, comm);
    }
}

/*
  BUFFERS
*/
typedef struct
{
  void *buffer;
  int size;
  sctk_spinlock_t lock;
} mpi_buffer_t;

static inline void
__sctk_init_mpi_buffer ()
{
  mpi_buffer_t *tmp;
  tmp = sctk_malloc (sizeof (mpi_buffer_t));
  tmp->size = 0;
  tmp->buffer = NULL;
  tmp->lock = 0;
  PMPC_Set_buffers (tmp);
}

static inline mpi_buffer_overhead_t *
SCTK__buffer_next_header (mpi_buffer_overhead_t * head, mpi_buffer_t * tmp)
{
  mpi_buffer_overhead_t *head_next;

  head_next =
    (mpi_buffer_overhead_t *) ((char *) head +
			       (sizeof (mpi_buffer_overhead_t) + head->size));
  if ((unsigned long) head_next >= (unsigned long) tmp->buffer + tmp->size)
    {
      head_next = NULL;
    }

  return head_next;
}

static inline mpi_buffer_overhead_t *
SCTK__MPI_Compact_buffer (int size, mpi_buffer_t * tmp)
{
  mpi_buffer_overhead_t *found = NULL;
  mpi_buffer_overhead_t *head;
  mpi_buffer_overhead_t *head_next;

  int flag;
  head = (mpi_buffer_overhead_t *) (tmp->buffer);

  while (head != NULL)
    {
      if (head->request != MPI_REQUEST_NULL)
	{
	  __INTERNAL__PMPI_Test (&(head->request), &flag, MPI_STATUS_IGNORE);
	}
      if (head->request == MPI_REQUEST_NULL)
	{
	  sctk_nodebug ("%p freed", head);
	  head_next = SCTK__buffer_next_header (head, tmp);

	  /*Compact from head */
	  while (head_next != NULL)
	    {
	      if (head->request != MPI_REQUEST_NULL)
		{
		  __INTERNAL__PMPI_Test (&(head_next->request), &flag,
					 MPI_STATUS_IGNORE);
		}
	      if (head_next->request == MPI_REQUEST_NULL)
		{
		  sctk_nodebug ("%p freed", head_next);
		  head->size =
		    head->size + head_next->size +
		    sizeof (mpi_buffer_overhead_t);
		  sctk_nodebug
		    ("MERGE Create new buffer of size %d (%d + %d)",
		     head->size, head_next->size,
		     head->size - head_next->size -
		     sizeof (mpi_buffer_overhead_t));
		}
	      else
		{
		  break;
		}

	      head_next = SCTK__buffer_next_header (head, tmp);
	    }
	}

      if ((head->request == MPI_REQUEST_NULL) && (head->size >= size)
	  && (found == NULL))
	{
	  found = head;
	}

      head_next = SCTK__buffer_next_header (head, tmp);
      head = head_next;
    }
  return found;
}

static int
__INTERNAL__PMPI_Buffer_attach (void *buf, int count)
{
  mpi_buffer_t *tmp;
  mpi_buffer_overhead_t *head;
  PMPC_Get_buffers ((void **) &tmp);

  sctk_spinlock_lock (&(tmp->lock));
  assume (tmp->buffer == NULL);
  tmp->buffer = buf;
  tmp->size = count;
  sctk_nodebug ("Buffer size %d", count);
  head = (mpi_buffer_overhead_t *) (tmp->buffer);

  head->size = tmp->size - sizeof (mpi_buffer_overhead_t);
  head->request = MPI_REQUEST_NULL;
  sctk_spinlock_unlock (&(tmp->lock));

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Buffer_detach (void *bufferptr, int *size)
{
  mpi_buffer_t *tmp;
  PMPC_Get_buffers ((void **) &tmp);
  sctk_spinlock_lock (&(tmp->lock));

  /* Flush pending messages */
  {
    mpi_buffer_overhead_t *head;
    mpi_buffer_overhead_t *head_next;
    int flag;
    int pending;
    do
      {
	pending = 0;
	head = (mpi_buffer_overhead_t *) (tmp->buffer);

	while (head != NULL)
	  {
	    if (head->request != MPI_REQUEST_NULL)
	      {
		__INTERNAL__PMPI_Test (&(head->request), &flag,
				       MPI_STATUS_IGNORE);
	      }
	    if (head->request != MPI_REQUEST_NULL)
	      {
		pending++;
	      }
	    head_next = SCTK__buffer_next_header (head, tmp);
	    head = head_next;
	  }
	sctk_thread_yield ();
      }
    while (pending != 0);
  }

  *((void **) bufferptr) = tmp->buffer;
  *size = tmp->size;
  tmp->size = 0;
  tmp->buffer = NULL;
  sctk_spinlock_unlock (&(tmp->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Isend_test_req (void *buf, int count, MPI_Datatype datatype,
				 int dest, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request);

static int
__INTERNAL__PMPI_Ibsend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request)
{
  mpi_buffer_t *tmp;
  int size;
  int res;
  mpi_buffer_overhead_t *head;
  mpi_buffer_overhead_t *head_next;
  void *head_buf;
  mpi_buffer_overhead_t *found = NULL;

  res = __INTERNAL__PMPI_Pack_size (count, datatype, comm, &size);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  if (size % sizeof (mpi_buffer_overhead_t))
    {
      size +=
	sizeof (mpi_buffer_overhead_t) -
	(size % sizeof (mpi_buffer_overhead_t));
    }
  assume (size % sizeof (mpi_buffer_overhead_t) == 0);

  sctk_nodebug ("MSG size %d", size);

  PMPC_Get_buffers ((void **) &tmp);
  sctk_spinlock_lock (&(tmp->lock));

  if (tmp->buffer == NULL)
    {
      sctk_spinlock_unlock (&(tmp->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "No buffer available");
    }

  found = SCTK__MPI_Compact_buffer (size, tmp);

  if (found)
    {
      int position = 0;
      head = found;
      head_buf = (char *) head + sizeof (mpi_buffer_overhead_t);

      assume (head->request == MPI_REQUEST_NULL);

      if (head->size >= size + (int)sizeof (mpi_buffer_overhead_t))
	{
	  int old_size;
	  old_size = head->size;
	  head->size = size;
	  head_next = SCTK__buffer_next_header (head, tmp);
	  head_next->size = old_size - size - sizeof (mpi_buffer_overhead_t);
	  head_next->request = MPI_REQUEST_NULL;
	  sctk_nodebug ("SPLIT Create new buffer of size %d old %d",
			head_next->size, old_size);
	}

      sctk_nodebug ("CHUNK %p<%p-%p<%p", tmp->buffer, head,
		    SCTK__buffer_next_header (head, tmp),
		    (void *) ((unsigned long) tmp->buffer + tmp->size));

      res =
	__INTERNAL__PMPI_Pack (buf, count, datatype, head_buf, head->size,
			       &position, comm);
      if (res != MPI_SUCCESS)
	{
	  sctk_spinlock_unlock (&(tmp->lock));
	  return res;
	}
      assume (position <= size);

      res =
	__INTERNAL__PMPI_Isend_test_req (head_buf, position, MPI_PACKED, dest,
					 tag, comm, &(head->request), 0);
      if (res != MPI_SUCCESS)
	{
	  sctk_spinlock_unlock (&(tmp->lock));
	  return res;
	}


      if (is_valid_request)
	{
	  __sctk_delete_mpc_request (request);
	}

    }
  else
    {
      sctk_spinlock_unlock (&(tmp->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "No space left in buffer");
    }

  sctk_spinlock_unlock (&(tmp->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ibsend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Ibsend_test_req (buf, count, datatype, dest, tag,
					   comm, request, 0);
}

static int
__INTERNAL__PMPI_Isend_test_req (void *buf, int count, MPI_Datatype datatype,
				 int dest, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request)
{
  if (sctk_is_derived_type (datatype))
    {
      int res;

      if (count > 1)
	{
	  MPI_Datatype new_datatype;
	  res =
	    __INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_commit (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res =
	    __INTERNAL__PMPI_Isend_test_req (buf, 1, new_datatype, dest, tag,
					     comm, request, is_valid_request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_free (&new_datatype);
	  return res;
	}
      else
	{
	  mpc_pack_absolute_indexes_t *begins;
	  mpc_pack_absolute_indexes_t *ends;
	  unsigned long slots_count;
	  mpc_pack_absolute_indexes_t lb;
	  int is_lb;
	  mpc_pack_absolute_indexes_t ub;
	  int is_ub;

	  res =
	    PMPC_Is_derived_datatype (datatype, &res, &begins, &ends,
				     &slots_count, &lb, &is_lb, &ub, &is_ub);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }


	  if (is_valid_request)
	    {
	      res = PMPC_Open_pack (__sctk_convert_mpc_request (request));
	    }
	  else
	    {
	      res = PMPC_Open_pack (__sctk_new_mpc_request (request));
	    }
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  {
	    mpc_pack_absolute_indexes_t *tmp;
	    tmp =
	      sctk_malloc (slots_count * 2 *
			   sizeof (mpc_pack_absolute_indexes_t));
	    __sctk_add_in_mpc_request (request, tmp);
	    memcpy (tmp, begins,
		    slots_count * sizeof (mpc_pack_absolute_indexes_t));
	    memcpy (&(tmp[slots_count]), ends,
		    slots_count * sizeof (mpc_pack_absolute_indexes_t));
	    begins = tmp;
	    ends = &(tmp[slots_count]);
	  }

	  res =
	    PMPC_Add_pack_absolute (buf, slots_count, begins, ends, MPC_CHAR,
				   __sctk_convert_mpc_request (request));
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res =
	    PMPC_Isend_pack (dest, tag, comm,
			    __sctk_convert_mpc_request (request));
	  return res;
	}
    }
  else
    {
      if (is_valid_request)
	{
	  return PMPC_Isend (buf, count, datatype, dest, tag, comm,
			    __sctk_convert_mpc_request (request));
	}
      else
	{
	  return PMPC_Isend (buf, count, datatype, dest, tag, comm,
			    __sctk_new_mpc_request (request));
	}
    }
}
static int
__INTERNAL__PMPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm, MPI_Request * request)
{
  return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					  comm, request, 0);
}
static int
__INTERNAL__PMPI_Issend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request)
{
  if (sctk_is_derived_type (datatype))
    {
      return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					      comm, request,
					      is_valid_request);
    }
  else
    {
      if (is_valid_request)
	{
	  return PMPC_Issend (buf, count, datatype, dest, tag, comm,
			     __sctk_convert_mpc_request (request));
	}
      else
	{
	  return PMPC_Issend (buf, count, datatype, dest, tag, comm,
			     __sctk_new_mpc_request (request));
	}
    }
}
static int
__INTERNAL__PMPI_Issend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Issend_test_req (buf, count, datatype, dest, tag,
					   comm, request, 0);
}
static int
__INTERNAL__PMPI_Irsend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request)
{
  if (sctk_is_derived_type (datatype))
    {
      return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					      comm, request,
					      is_valid_request);
    }
  else
    {
      if (is_valid_request)
	{
	  return PMPC_Irsend (buf, count, datatype, dest, tag, comm,
			     __sctk_convert_mpc_request (request));
	}
      else
	{
	  return PMPC_Irsend (buf, count, datatype, dest, tag, comm,
			     __sctk_new_mpc_request (request));
	}
    }
}
static int
__INTERNAL__PMPI_Irsend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Irsend_test_req (buf, count, datatype, dest, tag,
					   comm, request, 0);
}
static int
__INTERNAL__PMPI_Irecv_test_req (void *buf, int count, MPI_Datatype datatype,
				 int source, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request)
{
  if (sctk_is_derived_type (datatype))
    {
      int res;

      if (count > 1)
	{
	  MPI_Datatype new_datatype;
	  res =
	    __INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_commit (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res =
	    __INTERNAL__PMPI_Irecv_test_req (buf, 1, new_datatype, source,
					     tag, comm, request,
					     is_valid_request);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_free (&new_datatype);
	  return res;
	}
      else
	{
	  mpc_pack_absolute_indexes_t *begins;
	  mpc_pack_absolute_indexes_t *ends;
	  unsigned long slots_count;
	  mpc_pack_absolute_indexes_t lb;
	  int is_lb;
	  mpc_pack_absolute_indexes_t ub;
	  int is_ub;

	  res =
	    PMPC_Is_derived_datatype (datatype, &res, &begins, &ends,
				     &slots_count, &lb, &is_lb, &ub, &is_ub);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  if (is_valid_request)
	    {
	      res = PMPC_Open_pack (__sctk_convert_mpc_request (request));
	    }
	  else
	    {
	      res = PMPC_Open_pack (__sctk_new_mpc_request (request));
	    }
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  {
	    mpc_pack_absolute_indexes_t *tmp;
	    tmp =
	      sctk_malloc (slots_count * 2 *
			   sizeof (mpc_pack_absolute_indexes_t));
	    __sctk_add_in_mpc_request (request, tmp);
	    memcpy (tmp, begins,
		    slots_count * sizeof (mpc_pack_absolute_indexes_t));
	    memcpy (&(tmp[slots_count]), ends,
		    slots_count * sizeof (mpc_pack_absolute_indexes_t));
	    begins = tmp;
	    ends = &(tmp[slots_count]);
	  }

	  res =
	    PMPC_Add_pack_absolute (buf, slots_count, begins, ends, MPC_CHAR,
				   __sctk_convert_mpc_request (request));
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  res =
	    PMPC_Irecv_pack (source, tag, comm,
			    __sctk_convert_mpc_request (request));
	  return res;
	}
    }
  else
    {
      if (is_valid_request)
	{
	  return PMPC_Irecv (buf, count, datatype, source, tag, comm,
			    __sctk_convert_mpc_request (request));
	}
      else
	{
	  return PMPC_Irecv (buf, count, datatype, source, tag, comm,
			    __sctk_new_mpc_request (request));
	}
    }
}
static int
__INTERNAL__PMPI_Irecv (void *buf, int count, MPI_Datatype datatype,
			int source, int tag, MPI_Comm comm,
			MPI_Request * request)
{
  return __INTERNAL__PMPI_Irecv_test_req (buf, count, datatype, source, tag,
					  comm, request, 0);
}
static int
__INTERNAL__PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
  int res;
  res = PMPC_Wait (__sctk_convert_mpc_request (request), status);
  __sctk_delete_mpc_request (request);
  return res;
}

static int
__INTERNAL__PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
  int res;
  res = PMPC_Test (__sctk_convert_mpc_request (request), flag, status);
  if (*flag)
    {
      __sctk_delete_mpc_request (request);
    }
  else
    {
      mpc_thread_yield ();
    }
  return res;
}

static int
__INTERNAL__PMPI_Request_free (MPI_Request * request)
{
  int res = MPI_SUCCESS;
  MPI_internal_request_t *tmp;

  tmp = __sctk_convert_mpc_request_internal (request);
  if (tmp)
    {
      tmp->freeable = 1;
      res = PMPC_Request_free (&(tmp->req));
      __sctk_delete_mpc_request (request);
    }
  return res;
}

static int
__INTERNAL__PMPI_Waitany (int count, MPI_Request * array_of_requests,
			  int *index, MPI_Status * status)
{
  int flag = 0;
  while (!flag)
    {
      __INTERNAL__PMPI_Testany (count, array_of_requests, index, &flag,
				status);
      sctk_thread_yield ();
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Testany (int count, MPI_Request * array_of_requests,
			  int *index, int *flag, MPI_Status * status)
{
  int i;
  *index = MPI_UNDEFINED;
  *flag = 0;
  for (i = 0; i < count; i++)
    {
      int tmp;
      tmp =
	PMPC_Test (__sctk_convert_mpc_request (&(array_of_requests[i])), flag,
		  status);
      if (tmp != MPI_SUCCESS)
	{
	  return tmp;
	}
      if (*flag)
	{
	  __sctk_delete_mpc_request (&(array_of_requests[i]));
	  *index = i;
	  return tmp;
	}

    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Waitall (int count, MPI_Request * array_of_requests,
			  MPI_Status * array_of_statuses)
{
  int flag = 0;
  while (!flag)
    {
      int i;
      int done = 0;
      int loc_flag;
      for (i = 0; i < count; i++)
	{
	  int tmp;

	  if (array_of_requests[i] == MPI_REQUEST_NULL)
	    {
	      done++;
	      loc_flag = 0;
	      tmp = MPI_SUCCESS;
	    }
	  else
	    {
	      MPC_Request *req;
	      req = __sctk_convert_mpc_request (&(array_of_requests[i]));
	      if (req == &MPC_REQUEST_NULL)
		{
		  done++;
		  loc_flag = 0;
		  tmp = MPI_SUCCESS;
		}
	      else
		{
		  if( array_of_statuses != MPI_STATUSES_IGNORE )
		    tmp = PMPC_Test (req, &loc_flag, &(array_of_statuses[i]));
                  else
                    tmp = PMPC_Test (req, &loc_flag, MPC_STATUS_IGNORE);
		}
	    }
	  if (loc_flag)
	    {
	      __sctk_delete_mpc_request (&(array_of_requests[i]));
	      done++;
	    }
	  if (tmp != MPI_SUCCESS)
	    {
	      return tmp;
	    }
	}
      sctk_nodebug ("done %d tot %d", done, count);
      flag = (done == count);
      sctk_thread_yield ();
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Testall (int count, MPI_Request array_of_requests[],
			  int *flag, MPI_Status array_of_statuses[])
{
  int i;
  int done = 0;
  int loc_flag;
  *flag = 0;
  for (i = 0; i < count; i++)
    {
      int tmp;

      if (array_of_requests[i] == MPI_REQUEST_NULL)
	{
	  done++;
	  loc_flag = 0;
	  tmp = MPI_SUCCESS;
	}
      else
	{
	  MPC_Request *req;
	  req = __sctk_convert_mpc_request (&(array_of_requests[i]));
	  if (req == &MPC_REQUEST_NULL)
	    {
	      done++;
	      loc_flag = 0;
	      tmp = MPI_SUCCESS;
	    }
	  else
	    {
	       if( array_of_statuses != MPI_STATUSES_IGNORE )
		  tmp = PMPC_Test (req, &loc_flag, &(array_of_statuses[i]));
               else
                  tmp = PMPC_Test (req, &loc_flag, MPC_STATUS_IGNORE);
	    }
	}
      if (loc_flag)
	{
	  done++;
	}
      if (tmp != MPI_SUCCESS)
	{
	  return tmp;
	}
    }

  if (done == count)
    {
      for (i = 0; i < count; i++)
	{
	  if (array_of_requests[i] != MPI_REQUEST_NULL)
	    {
	      __sctk_delete_mpc_request (&(array_of_requests[i]));
	    }
	}
    }
  sctk_nodebug ("done %d tot %d", done, count);
  *flag = (done == count);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Waitsome (int incount, MPI_Request * array_of_requests,
			   int *outcount, int *array_of_indices,
			   MPI_Status * array_of_statuses)
{
  int flag = 0;
  while (!flag)
    {
      __INTERNAL__PMPI_Testsome (incount, array_of_requests, outcount,
				 array_of_indices, array_of_statuses);
      flag = *outcount;
      sctk_thread_yield ();
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Testsome (int incount, MPI_Request * array_of_requests,
			   int *outcount, int *array_of_indices,
			   MPI_Status * array_of_statuses)
{
  int i;
  int done = 0;
  int no_active_done = 0;
  int loc_flag;
  for (i = 0; i < incount; i++)
    {
      if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	  int tmp;
	  MPC_Request *req;
	  req = __sctk_convert_mpc_request (&(array_of_requests[i]));
	  if (req == &MPC_REQUEST_NULL)
	    {
	      loc_flag = 0;
	      tmp = MPI_SUCCESS;
	      no_active_done++;
	    }
	  else
	    {
	      if( array_of_statuses != MPI_STATUSES_IGNORE )
		 tmp = PMPC_Test (req, &loc_flag, &(array_of_statuses[done]));
              else
                 tmp = PMPC_Test (req, &loc_flag, MPC_STATUS_IGNORE);

	      array_of_indices[done] = i;
	    }
	  if (loc_flag)
	    {
	      __sctk_delete_mpc_request (&(array_of_requests[i]));
	      done++;
	    }
	  if (tmp != MPI_SUCCESS)
	    {
	      return tmp;
	    }
	}
      else
	{
	  no_active_done++;
	}
    }
  *outcount = done;
  if (no_active_done == incount)
    {
      *outcount = MPI_UNDEFINED;
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
			 MPI_Status * status)
{
  int res;
  res = PMPC_Iprobe (source, tag, comm, flag, status);
  if (!(*flag))
    sctk_thread_yield ();
  return res;
}

static int
__INTERNAL__PMPI_Probe (int source, int tag, MPI_Comm comm,
			MPI_Status * status)
{
  return PMPC_Probe (source, tag, comm, status);
}

static int
__INTERNAL__PMPI_Cancel (MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  MPI_internal_request_t *req;
  req = __sctk_convert_mpc_request_internal (request);
  if (req->is_active == 1)
    {
      res = PMPC_Cancel (__sctk_convert_mpc_request (request));
    }
  else
    {
      res = MPI_ERR_REQUEST;
    }
  return res;
}

static int
__INTERNAL__PMPI_Test_cancelled (MPI_Status * status, int *flag)
{
  PMPC_Test_cancelled (status, flag);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Send_init (void *buf, int count, MPI_Datatype datatype,
			    int dest, int tag, MPI_Comm comm,
			    MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request);
  req->freeable = 0;
  req->is_active = 0;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = dest;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Send_init;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Bsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request);
  req->freeable = 0;
  req->is_active = 0;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = dest;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Bsend_init;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ssend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request);
  req->freeable = 0;
  req->is_active = 0;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = dest;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Ssend_init;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Rsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request);
  req->freeable = 0;
  req->is_active = 0;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = dest;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Rsend_init;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Recv_init (void *buf, int count, MPI_Datatype datatype,
			    int source, int tag, MPI_Comm comm,
			    MPI_Request * request)
{
  MPI_internal_request_t *req;
  req = __sctk_new_mpc_request_internal (request);
  req->freeable = 0;
  req->is_active = 0;

  req->persistant.buf = buf;
  req->persistant.count = count;
  req->persistant.datatype = datatype;
  req->persistant.dest_source = source;
  req->persistant.tag = tag;
  req->persistant.comm = comm;
  req->persistant.op = Recv_init;

  return MPI_SUCCESS;
}

static inline int
____INTERNAL__PMPI_Start (MPI_Request * request)
{
  int res = 0;
  MPI_internal_request_t *req;

  req = __sctk_convert_mpc_request_internal (request);
  req->is_active = 1;

  switch (req->persistant.op)
    {
    case Send_init:
      res =
	__INTERNAL__PMPI_Isend_test_req (req->persistant.buf,
					 req->persistant.count,
					 req->persistant.datatype,
					 req->persistant.dest_source,
					 req->persistant.tag,
					 req->persistant.comm, request, 1);
      break;
    case Bsend_init:
      res =
	__INTERNAL__PMPI_Ibsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Rsend_init:
      res =
	__INTERNAL__PMPI_Irsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Ssend_init:
      res =
	__INTERNAL__PMPI_Issend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Recv_init:
      res =
	__INTERNAL__PMPI_Irecv_test_req (req->persistant.buf,
					 req->persistant.count,
					 req->persistant.datatype,
					 req->persistant.dest_source,
					 req->persistant.tag,
					 req->persistant.comm, request, 1);
      break;
    default:
      not_reachable ();
    }

  return res;
}

static int
__INTERNAL__PMPI_Start (MPI_Request * request)
{
  return ____INTERNAL__PMPI_Start (request);
}

static int
__INTERNAL__PMPI_Startall (int count, MPI_Request * array_of_requests)
{
  int i;
  for (i = 0; i < count; i++)
    {
      int res;
      res = ____INTERNAL__PMPI_Start (&(array_of_requests[i]));
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Sendrecv (void *sendbuf, int sendcount,
			   MPI_Datatype sendtype, int dest, int sendtag,
			   void *recvbuf, int recvcount,
			   MPI_Datatype recvtype, int source, int recvtag,
			   MPI_Comm comm, MPI_Status * status)
{
  int res;
  MPI_Request s_request;
  MPI_Request r_request;
  MPI_Status s_status;

  sctk_nodebug ("__INTERNAL__PMPI_Sendrecv TYPE %d %d", sendtype, recvtype);

  res =
    __INTERNAL__PMPI_Isend (sendbuf, sendcount, sendtype, dest, sendtag,
			    comm, &s_request);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  res =
    __INTERNAL__PMPI_Irecv (recvbuf, recvcount, recvtype, source, recvtag,
			    comm, &r_request);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&s_request, &s_status);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&r_request, status);
  sctk_nodebug ("__INTERNAL__PMPI_Sendrecv TYPE %d %d done", sendtype,
		recvtype);
  return res;
}
static int
__INTERNAL__PMPI_Sendrecv_replace (void *buf, int count,
				   MPI_Datatype datatype, int dest,
				   int sendtag, int source, int recvtag,
				   MPI_Comm comm, MPI_Status * status)
{
  int type_size;
  int res;
  char *tmp;
  MPI_Request sendreq;
  MPI_Request recvreq;
  int position = 0;

  sctk_nodebug ("__INTERNAL__PMPI_Sendrecv_replace TYPE %d", datatype);

  res = __INTERNAL__PMPI_Pack_size (count, datatype, comm, &type_size);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  tmp = sctk_malloc (type_size);
  if (tmp == NULL)
    {
      return MPI_ERR_INTERN;
    }

  res =
    __INTERNAL__PMPI_Pack (buf, count, datatype, tmp, type_size, &position,
			   comm);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  sctk_nodebug ("position %d, %d", position, type_size);

  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, recvtag, comm,
			    &recvreq);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  res =
    __INTERNAL__PMPI_Isend (tmp, position, MPI_PACKED, dest, sendtag, comm,
			    &sendreq);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&sendreq, status);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  res = __INTERNAL__PMPI_Wait (&recvreq, status);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }

  sctk_free (tmp);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Type_contiguous (int count, MPI_Datatype data_in,
				  MPI_Datatype * data_out)
{
  if (sctk_is_derived_type (data_in))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      unsigned long i;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;

      unsigned long extent;

      PMPC_Is_derived_datatype (data_in, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);
      __INTERNAL__PMPI_Type_extent (data_in, (MPI_Aint *) & extent);

      count_out = count_in * count;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      for (i = 0; i < count_out; i++)
	{
	  begins_out[i] = begins_in[i % count_in] + extent * (i / count_in);
	  ends_out[i] = ends_in[i % count_in] + extent * (i / count_in);
	  sctk_nodebug ("%d , %lu-%lu <- %lu-%lu", i, begins_out[i],
			ends_out[i], begins_in[i % count_in],
			ends_in[i % count_in]);
	}

      ub += extent * (count - 1);

      PMPC_Derived_datatype (data_out, begins_out, ends_out, count_out, lb,
			    is_lb, ub, is_ub);

      sctk_free (begins_out);
      sctk_free (ends_out);
      return MPI_SUCCESS;
    }
  else
    {
      size_t size;
      PMPC_Type_size (data_in, &size);
      size = size * count;
      return PMPC_Sizeof_datatype (data_out, size);
    }
}
static int
__INTERNAL__PMPI_Type_vector (int count,
			      int blocklen,
			      int stride, MPI_Datatype old_type,
			      MPI_Datatype * newtype_p)
{
  unsigned long stride_t;
  stride_t = (unsigned long) stride;
  if (sctk_is_derived_type (old_type))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      int i;
      int j;
      unsigned long k;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;

      unsigned long extent;

      PMPC_Is_derived_datatype (old_type, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);
      __INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

      sctk_nodebug ("extend %lu, count %d, blocklen %d, stide %lu", extent,
		    count, blocklen, stride);


      count_out = count_in * count * blocklen;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));


      for (i = 0; i < count; i++)
	{
	  for (j = 0; j < blocklen; j++)
	    {
	      for (k = 0; k < count_in; k++)
		{
		  begins_out[(i * blocklen + j) * count_in + k] =
		    begins_in[k] + ((stride_t * i + j) * extent);
		  ends_out[(i * blocklen + j) * count_in + k] =
		    ends_in[k] + ((stride_t * i + j) * extent);
		}
	    }
	}

      ub += extent * stride * (count - 1) + extent * (blocklen - 1);

      PMPC_Derived_datatype (newtype_p, begins_out, ends_out, count_out, lb,
			    is_lb, ub, is_ub);

      sctk_free (begins_out);
      sctk_free (ends_out);
/*       { */
/* 	size_t deb_size; */
/* 	PMPC_Type_size(*newtype_p,&deb_size); */
/* 	sctk_nodebug("type %d, size %lu",*newtype_p,deb_size); */
/*       } */
      return MPI_SUCCESS;
    }
  else
    {
      int res;
      MPI_Datatype data_out;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      size_t tmp;

      count_out = 1;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      begins_out[0] = 0;
      PMPC_Type_size (old_type, &(tmp));
      ends_out[0] = tmp;
      ends_out[0]--;

      PMPC_Derived_datatype (&data_out, begins_out, ends_out, count_out, 0, 0,
			    0, 0);
      res =
	__INTERNAL__PMPI_Type_vector (count, blocklen, (MPI_Aint) stride_t,
				      data_out, newtype_p);
      __INTERNAL__PMPI_Type_free (&data_out);
      sctk_free (begins_out);
      sctk_free (ends_out);
      return res;
    }
}

static int
__INTERNAL__PMPI_Type_hvector (int count,
			       int blocklen,
			       MPI_Aint stride,
			       MPI_Datatype old_type,
			       MPI_Datatype * newtype_p)
{
  unsigned long stride_t;
  stride_t = (unsigned long) stride;
  if (sctk_is_derived_type (old_type))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      int i;
      int j;
      unsigned long k;

      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;
      unsigned long extent;

      PMPC_Is_derived_datatype (old_type, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);
      __INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

      sctk_nodebug ("extend %lu, count %d, blocklen %d, stide %lu", extent,
		    count, blocklen, stride);
/*       { */
/* 	size_t deb_size; */
/* 	PMPC_Type_size(old_type,&deb_size); */
/* 	sctk_nodebug("type %d, size %lu",old_type,deb_size); */
/*       } */

      count_out = count_in * count * blocklen;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));


      for (i = 0; i < count; i++)
	{
	  for (j = 0; j < blocklen; j++)
	    {
	      for (k = 0; k < count_in; k++)
		{
		  begins_out[(i * blocklen + j) * count_in + k] =
		    begins_in[k] + (stride_t * i) + (j * extent);
		  sctk_nodebug ("start %d %lu",
				(i * blocklen + j) * count_in + k,
				begins_out[(i * blocklen + j) * count_in +
					   k]);
		  ends_out[(i * blocklen + j) * count_in + k] =
		    ends_in[k] + (stride_t * i) + (j * extent);
		}
	    }
	}
      ub += stride * (count - 1) + extent * (blocklen - 1);
      PMPC_Derived_datatype (newtype_p, begins_out, ends_out, count_out, lb,
			    is_lb, ub, is_ub);

      sctk_free (begins_out);
      sctk_free (ends_out);
/*       { */
/* 	size_t deb_size; */
/* 	PMPC_Type_size(*newtype_p,&deb_size); */
/* 	sctk_nodebug("type %d, size %lu",*newtype_p,deb_size); */
/*       } */
      return MPI_SUCCESS;
    }
  else
    {
      int res;
      MPI_Datatype data_out;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      size_t tmp;

      count_out = 1;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      begins_out[0] = 0;
      PMPC_Type_size (old_type, &(tmp));
      ends_out[0] = tmp;
      ends_out[0]--;

      PMPC_Derived_datatype (&data_out, begins_out, ends_out, count_out, 0, 0,
			    0, 0);
      res =
	__INTERNAL__PMPI_Type_hvector (count, blocklen, (MPI_Aint) stride_t,
				       data_out, newtype_p);
      __INTERNAL__PMPI_Type_free (&data_out);
      sctk_free (begins_out);
      sctk_free (ends_out);
      return res;
    }
}

static int
__INTERNAL__PMPI_Type_indexed (int count,
			       int blocklens[],
			       int indices[],
			       MPI_Datatype old_type, MPI_Datatype * newtype)
{
  if (sctk_is_derived_type (old_type))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      int i;
      int j;
      unsigned long k;
      unsigned long step = 0;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;

      unsigned long extent;

      PMPC_Is_derived_datatype (old_type, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);
      __INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

      count_out = 0;
      for (i = 0; i < count; i++)
	{
	  count_out += count_in * blocklens[i];
	  sctk_nodebug ("%d %d %d", i, count_in, blocklens[i]);
	}

      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      for (i = 0; i < count; i++)
	{
	  unsigned long stride_t;
	  stride_t = (unsigned long) (indices[i]);
	  for (j = 0; j < blocklens[i]; j++)
	    {
	      for (k = 0; k < count_in; k++)
		{
		  begins_out[step] = begins_in[k] + (indices[i] + j) * extent;
		  ends_out[step] = ends_in[k] + (indices[i] + j) * extent;
		  step++;
		}
	    }
	  if (is_ub)
	    {
	      long int new_b;
	      new_b = ub + extent * indices[i] + extent * (blocklens[i] - 1);
	      sctk_nodebug ("cur ub %d", new_b);
	      if ((long int) new_b > (long int) ub)
		{
		  ub = new_b;
		}
	      sctk_nodebug ("cur ub %d", ub);
	    }
	  if (is_lb)
	    {
	      long int new_b;
	      new_b = lb + extent * indices[i];
	      if ((long int) new_b < (long int) lb)
		{
		  lb = new_b;
		}
	    }
	}

      PMPC_Derived_datatype (newtype, begins_out, ends_out, count_out, lb,
			    is_lb, ub, is_ub);

      sctk_free (begins_out);
      sctk_free (ends_out);
      return MPI_SUCCESS;
    }
  else
    {
      int res;
      MPI_Datatype data_out;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      size_t tmp;

      count_out = 1;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      begins_out[0] = 0;
      PMPC_Type_size (old_type, &(tmp));
      ends_out[0] = begins_out[0] + tmp - 1;

      PMPC_Derived_datatype (&data_out, begins_out, ends_out, count_out, 0, 0,
			    0, 0);
      res =
	__INTERNAL__PMPI_Type_indexed (count, blocklens, indices, data_out,
				       newtype);
      __INTERNAL__PMPI_Type_free (&data_out);
      sctk_free (begins_out);
      sctk_free (ends_out);
      return res;
    }
}

static int
__INTERNAL__PMPI_Type_hindexed (int count,
				int blocklens[],
				MPI_Aint indices[],
				MPI_Datatype old_type, MPI_Datatype * newtype)
{
  if (sctk_is_derived_type (old_type))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      int i;
      int j;
      unsigned long k;
      unsigned long step = 0;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;

      unsigned long extent;

      PMPC_Is_derived_datatype (old_type, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);
      __INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

      count_out = 0;
      for (i = 0; i < count; i++)
	{
	  count_out += count_in * blocklens[i];
	}

      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      sctk_nodebug ("%lu extent", extent);

      for (i = 0; i < count; i++)
	{
	  unsigned long stride_t;
	  stride_t = (unsigned long) indices[i];
	  for (j = 0; j < blocklens[i]; j++)
	    {
	      for (k = 0; k < count_in; k++)
		{
		  begins_out[step] = begins_in[k] + indices[i] + j * extent;
		  ends_out[step] = ends_in[k] + indices[i] + j * extent;
		  step++;
		}
	    }
	  if (is_ub)
	    {
	      int new_b;
	      new_b = ub + indices[i] + extent * (blocklens[i] - 1);
	      if ((long int) new_b > (long int) ub)
		{
		  ub = new_b;
		}
	    }
	  if (is_lb)
	    {
	      int new_b;
	      new_b = lb + indices[i];
	      if ((long int) new_b < (long int) lb)
		{
		  lb = new_b;
		}
	    }
	}

      PMPC_Derived_datatype (newtype, begins_out, ends_out, count_out, lb,
			    is_lb, ub, is_ub);

      sctk_free (begins_out);
      sctk_free (ends_out);
      return MPI_SUCCESS;
    }
  else
    {
      int res;
      MPI_Datatype data_out;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      size_t tmp;

      count_out = 1;
      begins_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
      ends_out =
	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));

      begins_out[0] = 0;
      PMPC_Type_size (old_type, &(tmp));
      ends_out[0] = begins_out[0] + tmp - 1;

      PMPC_Derived_datatype (&data_out, begins_out, ends_out, count_out, 0, 0,
			    0, 0);
      res =
	__INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, data_out,
					newtype);
      __INTERNAL__PMPI_Type_free (&data_out);
      sctk_free (begins_out);
      sctk_free (ends_out);
      return res;
    }
}

static int
__INTERNAL__PMPI_Type_struct (int count,
			      int blocklens[],
			      MPI_Aint indices[],
			      MPI_Datatype old_types[],
			      MPI_Datatype * newtype)
{
  int i;
  mpc_pack_absolute_indexes_t *begins_out = NULL;
  mpc_pack_absolute_indexes_t *ends_out = NULL;
  unsigned long count_out = 0;
  unsigned long glob_count_out = 0;
  mpc_pack_absolute_indexes_t new_lb = (mpc_pack_absolute_indexes_t) (-1);
  int new_is_lb = 0;
  mpc_pack_absolute_indexes_t new_ub = 0;
  int new_is_ub = 0;

  for (i = 0; i < count; i++)
    {
      if ((old_types[i] != MPI_UB) && (old_types[i] != MPI_LB))
	{
	  if (sctk_is_derived_type (old_types[i]))
	    {
	      int res;
	      mpc_pack_absolute_indexes_t *begins_in;
	      mpc_pack_absolute_indexes_t *ends_in;
	      unsigned long count_in;
	      unsigned long extent;
	      unsigned long local_count_out = 0;
	      unsigned long prev_count_out = 0;
	      int j;
	      unsigned long k;
	      unsigned long stride_t;
	      mpc_pack_absolute_indexes_t lb;
	      int is_lb;
	      mpc_pack_absolute_indexes_t ub;
	      int is_ub;

	      PMPC_Is_derived_datatype (old_types[i], &res, &begins_in,
				       &ends_in, &count_in, &lb, &is_lb, &ub,
				       &is_ub);
	      __INTERNAL__PMPI_Type_extent (old_types[i],
					    (MPI_Aint *) & extent);
	      sctk_nodebug ("Extend %lu", extent);

	      prev_count_out = count_out;
	      local_count_out = count_in * blocklens[i];
	      count_out += local_count_out;

	      begins_out =
		sctk_realloc (begins_out,
			      count_out *
			      sizeof (mpc_pack_absolute_indexes_t));
	      ends_out =
		sctk_realloc (ends_out,
			      count_out *
			      sizeof (mpc_pack_absolute_indexes_t));


	      stride_t = (unsigned long) indices[i];
	      for (j = 0; j < blocklens[i]; j++)
		{
		  for (k = 0; k < count_in; k++)
		    {
		      begins_out[glob_count_out] =
			begins_in[k] + stride_t + (extent * j);
		      ends_out[glob_count_out] =
			ends_in[k] + stride_t + (extent * j);
		      sctk_nodebug (" begins_out[%lu] = %lu", glob_count_out,
				    begins_out[glob_count_out]);
		      sctk_nodebug (" ends_out[%lu] = %lu", glob_count_out,
				    ends_out[glob_count_out]);

		      glob_count_out++;
		    }
		}
	      sctk_nodebug ("derived type %d new_lb %d new_ub %d before", i,
			    new_lb, new_ub);
	      if (is_ub)
		{
		  mpc_pack_absolute_indexes_t new_b;
		  new_b = ub + indices[i] + extent * (blocklens[i] - 1);
		  sctk_nodebug ("cur ub %d", new_b);
		  if ((long int) new_b > (long int) new_ub || new_is_ub == 0)
		    {
		      new_ub = new_b;
		    }
		  new_is_ub = 1;
		}
	      if (is_lb)
		{
		  mpc_pack_absolute_indexes_t new_b;
		  new_b = lb + indices[i];
		  sctk_nodebug ("cur lb %d", new_b);
		  if ((long int) new_b < (long int) new_lb || new_is_lb == 0)
		    {
		      new_lb = new_b;
		    }
		  new_is_lb = 1;
		}
	      sctk_nodebug ("derived type %d new_lb %d new_ub %d after ", i,
			    new_lb, new_ub);
	    }
	  else
	    {
	      size_t tmp;
	      unsigned long local_count_out = 0;
	      unsigned long prev_count_out = 0;
	      unsigned long stride_t;
	      mpc_pack_absolute_indexes_t begins_in[1];
	      mpc_pack_absolute_indexes_t ends_in[1];
	      unsigned long count_in;
	      unsigned long extent;
	      int j;
	      unsigned long k;

	      begins_in[0] = 0;
	      PMPC_Type_size (old_types[i], &(tmp));
	      ends_in[0] = begins_in[0] + tmp - 1;
	      count_in = 1;
	      sctk_nodebug ("Type size %lu", tmp);

	      __INTERNAL__PMPI_Type_extent (old_types[i],
					    (MPI_Aint *) & extent);

	      prev_count_out = count_out;
	      local_count_out = count_in * blocklens[i];
	      count_out += local_count_out;

	      begins_out =
		sctk_realloc (begins_out,
			      count_out *
			      sizeof (mpc_pack_absolute_indexes_t));
	      ends_out =
		sctk_realloc (ends_out,
			      count_out *
			      sizeof (mpc_pack_absolute_indexes_t));

	      stride_t = (unsigned long) indices[i];
	      for (j = 0; j < blocklens[i]; j++)
		{
		  for (k = 0; k < count_in; k++)
		    {
		      begins_out[glob_count_out] =
			begins_in[k] + stride_t + (extent * j);


		      ends_out[glob_count_out] =
			ends_in[k] + stride_t + (extent * j);

		      sctk_nodebug (" begins_out[%lu] = %lu", glob_count_out,
				    begins_out[glob_count_out]);
		      sctk_nodebug (" ends_out[%lu] = %lu", glob_count_out,
				    ends_out[glob_count_out]);

		      glob_count_out++;
		    }
		}
	      sctk_nodebug ("simple type %d new_lb %d new_ub %d", i, new_lb,
			    new_ub);
	    }
	}
      else
	{
	  if (old_types[i] == MPI_UB)
	    {
	      new_is_ub = 1;
	      new_ub = indices[i];
	    }
	  else
	    {
	      new_is_lb = 1;
	      new_lb = indices[i];
	    }
	  sctk_nodebug ("hard bound %d new_lb %d new_ub %d", i, new_lb,
			new_ub);
	}
      sctk_nodebug ("%d new_lb %d new_ub %d", i, new_lb, new_ub);
    }


  PMPC_Derived_datatype (newtype, begins_out, ends_out, glob_count_out, new_lb,
			new_is_lb, new_ub, new_is_ub);

/*   sctk_debug("new_type %d",* newtype); */
/*   sctk_debug("final new_lb %d,%d new_ub %d %d",new_lb,new_is_lb,new_ub,new_is_ub); */
/*   { */
/*     int i ;  */
/*     for(i = 0; i < glob_count_out; i++){ */
/*       sctk_debug("%d begins %lu ends %lu",i,begins_out[i],ends_out[i]); */
/*     } */
/*   } */

  sctk_free (begins_out);
  sctk_free (ends_out);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Address (void *location, MPI_Aint * address)
{
  *address = (MPI_Aint) location;
  return MPI_SUCCESS;
}

/* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
static int
__INTERNAL__PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
  MPI_Aint UB;
  MPI_Aint LB;

  __INTERNAL__PMPI_Type_lb (datatype, &LB);
  __INTERNAL__PMPI_Type_ub (datatype, &UB);

  *extent = (MPI_Aint) ((unsigned long) UB - (unsigned long) LB);
  return MPI_SUCCESS;
}

/* See the 1.1 version of the Standard.  The standard made an 
   unfortunate choice here, however, it is the standard.  The size returned 
   by MPI_Type_size is specified as an int, not an MPI_Aint */
static int
__INTERNAL__PMPI_Type_size (MPI_Datatype datatype, int *size)
{
  size_t tmp_size;
  int real_val;
  int res;
  res = PMPC_Type_size (datatype, &tmp_size);
  real_val = tmp_size;
  *size = real_val;
  return res;
}

/* MPI_Type_count was withdrawn in MPI 1.1 */
static int
__INTERNAL__PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
  int res;
  mpc_pack_absolute_indexes_t *begins_in;
  mpc_pack_absolute_indexes_t *ends_in;
  unsigned long count_in;
  unsigned long i;
  mpc_pack_absolute_indexes_t lb;
  int is_lb;
  mpc_pack_absolute_indexes_t ub;
  int is_ub;
  PMPC_Is_derived_datatype (datatype, &res, &begins_in, &ends_in, &count_in,
			   &lb, &is_lb, &ub, &is_ub);
  if (res)
    {
      if (is_lb == 0)
	{
	  *displacement = (MPI_Aint) begins_in[0];
	  for (i = 0; i < count_in; i++)
	    {
	      if ((mpc_pack_absolute_indexes_t) * displacement > begins_in[i])
		{
		  *displacement = (MPI_Aint) begins_in[i];
		}
	    }
	}
      else
	{
	  *displacement = lb;
	}
    }
  else
    {
      *displacement = 0;
    }
  sctk_nodebug ("%lu lb", *displacement);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Type_ub (MPI_Datatype datatype, MPI_Aint * displacement)
{
  int res;
  mpc_pack_absolute_indexes_t *begins_in;
  mpc_pack_absolute_indexes_t *ends_in;
  unsigned long count_in;
  unsigned long i;
  mpc_pack_absolute_indexes_t lb;
  int is_lb;
  mpc_pack_absolute_indexes_t ub;
  int is_ub;
  int e;
  PMPC_Is_derived_datatype (datatype, &res, &begins_in, &ends_in, &count_in,
			   &lb, &is_lb, &ub, &is_ub);
  if (res)
    {
      if (is_ub == 0)
	{
	  *displacement = (MPI_Aint) ends_in[0];
	  for (i = 0; i < count_in; i++)
	    {
	      if ((mpc_pack_absolute_indexes_t) * displacement < ends_in[i])
		{
		  *displacement = (MPI_Aint) ends_in[i];
		}
	      sctk_nodebug ("Current ub %lu, %lu", ends_in[i], *displacement);
	    }
	  e = 1;
	  *displacement = (MPI_Aint) ((unsigned long) (*displacement) + e);
	}
      else
	{
	  *displacement = ub;
	}
    }
  else
    {
      size_t tmp;
      PMPC_Type_size (datatype, &tmp);
      *displacement = (MPI_Aint) tmp;
    }
  sctk_nodebug ("%lu ub", *displacement);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Type_commit (MPI_Datatype * datatype)
{
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Type_free (MPI_Datatype * datatype)
{
  return PMPC_Type_free (datatype);
}

static int
__INTERNAL__PMPI_Get_elements (MPI_Status * status, MPI_Datatype datatype,
			       int *elements)
{
  int res = MPI_SUCCESS;
  unsigned long size;
  int data_size;

  if (status == MPC_STATUS_IGNORE)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_IN_STATUS, "Invalid status");
    }

  res = __INTERNAL__PMPI_Type_size (datatype, &data_size);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  if (data_size != 0)
    {
      size = status->count;
      sctk_nodebug ("%d count %d data_size", size, data_size);
      if (size % data_size == 0)
	{
	  size = size / data_size;
	  *elements = size;
	}
      else
	{
	  *elements = MPI_UNDEFINED;
	}
    }
  else
    {
      *elements = 0;
    }

#warning "behave as get_count"

  return res;
}

static int
__INTERNAL__PMPI_Pack (void *inbuf,
		       int incount,
		       MPI_Datatype datatype,
		       void *outbuf, int outcount, int *position,
		       MPI_Comm comm)
{
  int copied = 0;

  if (sctk_is_derived_type (datatype))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      unsigned long i;
      int j;
      unsigned long extent;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;
      __INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

      PMPC_Is_derived_datatype (datatype, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);

      for (j = 0; j < incount; j++)
	{
	  for (i = 0; i < count_in; i++)
	    {
	      unsigned long size;
	      size = ends_in[i] - begins_in[i] + 1;
	      sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu",
			    begins_in[i] + extent * j,
			    ends_in[i] + extent * j, *position, size);
	      if (size != 0)
		{
		  memcpy (&(((char *) outbuf)[*position]),
			  &(((char *) inbuf)[begins_in[i]]), size);
		}
	      copied += size;
	      sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done",
			    begins_in[i] + extent * j,
			    ends_in[i] + extent * j, *position, size);
	      *position = *position + size;
	      assume (copied <= outcount);
	    }
	  inbuf = (char *) inbuf + extent;
	}

      sctk_nodebug ("%lu <= %lu", copied, outcount);
      assume (copied <= outcount);
      return MPI_SUCCESS;
    }
  else
    {
      size_t size;
      PMPC_Type_size (datatype, &size);
      sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu", 0, size * incount,
		    *position, size * incount);
      memcpy (&(((char *) outbuf)[*position]), inbuf, size * incount);
      sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done", 0, size * incount,
		    *position, size * incount);
      *position = *position + size * incount;
      copied += size * incount;
      sctk_nodebug ("%lu == %lu", copied, outcount);
      assume (copied <= outcount);
      return MPI_SUCCESS;
    }
}

static int
__INTERNAL__PMPI_Unpack (void *inbuf,
			 int insize,
			 int *position,
			 void *outbuf, int outcount, MPI_Datatype datatype,
			 MPI_Comm comm)
{
  int copied = 0;

  if (sctk_is_derived_type (datatype))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_out;
      mpc_pack_absolute_indexes_t *ends_out;
      unsigned long count_out;
      unsigned long i;
      int j;
      unsigned long extent;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;
      __INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

      PMPC_Is_derived_datatype (datatype, &res, &begins_out, &ends_out,
			       &count_out, &lb, &is_lb, &ub, &is_ub);
      for (j = 0; j < outcount; j++)
	{
	  for (i = 0; i < count_out; i++)
	    {
	      size_t size;
	      size = ends_out[i] - begins_out[i] + 1;
	      copied += size;
	      sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position, size,
			    begins_out[i] + extent * j,
			    ends_out[i] + extent * j);
	      if (size != 0)
		{
		  memcpy (&(((char *) outbuf)[begins_out[i]]),
			  &(((char *) inbuf)[*position]), size);
		}
	      sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position,
			    size, begins_out[i] + extent * j,
			    ends_out[i] + extent * j);
	      *position = *position + size;
	      assume (copied <= insize);
	    }
	  outbuf = (char *) outbuf + extent;
	}

      sctk_nodebug ("%lu <= %lu", copied, insize);
      assume (copied <= insize);
      return MPI_SUCCESS;
    }
  else
    {
      size_t size;
      PMPC_Type_size (datatype, &size);
      sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position,
		    size * outcount, 0, size * outcount);
      memcpy (outbuf, &(((char *) inbuf)[*position]), size * outcount);
      sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position,
		    size * outcount, 0, size * outcount);
      *position = *position + size * outcount;
      copied += size * outcount;
      assume (copied <= insize);
      return MPI_SUCCESS;
    }
}

static int
__INTERNAL__PMPI_Pack_size (int incount, MPI_Datatype datatype, MPI_Comm comm,
			    int *size)
{
  *size = 0;
  sctk_nodebug ("incount size %d", incount);
  if (sctk_is_derived_type (datatype))
    {
      int res;
      mpc_pack_absolute_indexes_t *begins_in;
      mpc_pack_absolute_indexes_t *ends_in;
      unsigned long count_in;
      unsigned long i;
      mpc_pack_absolute_indexes_t lb;
      int is_lb;
      mpc_pack_absolute_indexes_t ub;
      int is_ub;
      int j;
      PMPC_Is_derived_datatype (datatype, &res, &begins_in, &ends_in,
			       &count_in, &lb, &is_lb, &ub, &is_ub);

      for (j = 0; j < incount; j++)
	{
	  for (i = 0; i < count_in; i++)
	    {
	      size_t sizet;
	      sizet = ends_in[i] - begins_in[i] + 1;
	      *size = *size + sizet;
	      sctk_nodebug ("PACK derived part size %d, %lu-%lu", sizet,
			    begins_in[i], ends_in[i]);
	    }
	}
      sctk_nodebug ("PACK derived size %d", *size);
      return MPI_SUCCESS;
    }
  else
    {
      size_t size_t;
      PMPC_Type_size (datatype, &size_t);
      *size = size_t * incount;
      sctk_nodebug ("PACK contiguous size %d", *size);
      return MPI_SUCCESS;
    }
}

static int
__INTERNAL__PMPI_Barrier (MPI_Comm comm)
{
  return PMPC_Barrier (comm);
}

static int
__INTERNAL__PMPI_Bcast (void *buffer, int count, MPI_Datatype datatype,
			int root, MPI_Comm comm)
{
  int res;
  sctk_nodebug ("BCAST with root %d, datatype %d count %d", root, datatype,
		count);

  if (sctk_is_derived_type (datatype))
    {
      int rank;
      int size;
      int offset = 0;
      void *tmp_buf;


      __INTERNAL__PMPI_Pack_size (count, datatype, comm, &size);
      tmp_buf = sctk_malloc (size);
      assume (tmp_buf != NULL);

      __INTERNAL__PMPI_Comm_rank (comm, &rank);
      sctk_nodebug ("I am %d root %d", rank, root);
      if (root == rank)
	{
	  __INTERNAL__PMPI_Pack (buffer, count, datatype, tmp_buf, size,
				 &offset, comm);
	  sctk_nodebug ("SEND size %d offset %d", size, offset);
	  sctk_nodebug ("BEFORE SEND %d %d", ((int *) tmp_buf)[0],
			((int *) buffer)[0]);
	  res = PMPC_Bcast (tmp_buf, size, MPI_PACKED, root, comm);
	  sctk_nodebug ("AFTER SEND %d %d", ((int *) tmp_buf)[0],
			((int *) buffer)[0]);
	}
      else
	{
	  res = PMPC_Bcast (tmp_buf, size, MPI_PACKED, root, comm);
	  sctk_nodebug ("BEFORE RECV %d %d res %d", ((int *) tmp_buf)[0],
			((int *) buffer)[0], res);
	  __INTERNAL__PMPI_Unpack (tmp_buf, size, &offset, buffer, count,
				   datatype, comm);
	  sctk_nodebug ("RECV size %d offset %d res %d", size, offset, res);
	  sctk_nodebug ("RECV %d %d", ((int *) tmp_buf)[0],
			((int *) buffer)[0]);
	}

      sctk_free (tmp_buf);
      sctk_nodebug ("res after free %d", res);
    }
  else
    {
      res = PMPC_Bcast (buffer, count, datatype, root, comm);
    }
  sctk_nodebug ("BCAST with root %d DONE res %d", root, res);
  return res;
}

#define MPI_MAX_CONCURENT 10
static int
__INTERNAL__PMPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
			 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
			 int root, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      MPI_Aint dsize;
      int size;
      int rank;
      MPI_Request request;
      MPI_Request recvrequest[MPI_MAX_CONCURENT];


      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);
      __INTERNAL__PMPI_Isend (sendbuf, sendcnt, sendtype, root, -2, comm,
			      &request);

      if (rank == root)
	{
	  int i = 0;
	  int j;
/* 	  __INTERNAL__PMPI_Pack_size(1,recvtype,comm,&dsize); */
	  __INTERNAL__PMPI_Type_extent (recvtype, &dsize);
	  while (i < size)
	    {
	      for (j = 0; (i < size) && (j < MPI_MAX_CONCURENT);)
		{
		  __INTERNAL__PMPI_Irecv (((char *) recvbuf) +
					  (i * recvcnt * dsize),
					  recvcnt, recvtype, i, -2, comm,
					  &(recvrequest[j]));
		  i++;
		  j++;
		}
	      j--;
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(recvrequest[j]),
					 MPI_STATUS_IGNORE);
		}
	    }
	}

      __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);

      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Gather (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			 recvtype, root, comm);
    }
}
static int
__INTERNAL__PMPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
			  void *recvbuf, int *recvcnts, int *displs,
			  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      MPI_Aint dsize;
      int size;
      int rank;
      MPI_Request request;
      MPI_Request recvrequest[MPI_MAX_CONCURENT];


      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);
      __INTERNAL__PMPI_Isend (sendbuf, sendcnt, sendtype, root, -2, comm,
			      &request);

      if (rank == root)
	{
	  int i = 0;
	  int j;
	  /*      __INTERNAL__PMPI_Pack_size(1,recvtype,comm,&dsize); */
	  __INTERNAL__PMPI_Type_extent (recvtype, &dsize);
	  while (i < size)
	    {
	      for (j = 0; (i < size) && (j < MPI_MAX_CONCURENT);)
		{
		  __INTERNAL__PMPI_Irecv (((char *) recvbuf) +
					  (displs[i] * dsize),
					  recvcnts[i], recvtype, i, -2, comm,
					  &(recvrequest[j]));
		  i++;
		  j++;
		}
	      j--;
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(recvrequest[j]),
					 MPI_STATUS_IGNORE);
		}
	    }
	}

      __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);

      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Gatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
			  displs, recvtype, root, comm);
    }
}
static int
__INTERNAL__PMPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
			  void *recvbuf, int recvcnt, MPI_Datatype recvtype,
			  int root, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int size;
      int rank;
      MPI_Aint dsize;
      MPI_Request request;
      MPI_Request sendrequest[MPI_MAX_CONCURENT];
      int i;
      int j;

      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);


      __INTERNAL__PMPI_Irecv (recvbuf, recvcnt, recvtype, root, -2, comm,
			      &request);

      if (rank == root)
	{
	  i = 0;
	  /*      __INTERNAL__PMPI_Pack_size(1,sendtype,comm,&dsize); */
	  __INTERNAL__PMPI_Type_extent (sendtype, &dsize);
	  while (i < size)
	    {
	      for (j = 0; (i < size) && (j < MPI_MAX_CONCURENT);)
		{
		  __INTERNAL__PMPI_Isend (((char *) sendbuf) +
					  (i * sendcnt * dsize),
					  sendcnt, sendtype, i, -2, comm,
					  &(sendrequest[j]));
		  i++;
		  j++;
		}
	      j--;
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(sendrequest[j]),
					 MPI_STATUS_IGNORE);
		}
	    }
	}

      __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Scatter (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			  recvtype, root, comm);
    }
}
static int
__INTERNAL__PMPI_Scatterv (void *sendbuf, int *sendcnts, int *displs,
			   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
			   MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int size;
      int rank;
      MPI_Request request;
      MPI_Request sendrequest[MPI_MAX_CONCURENT];
      int i;
      int j;
      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);

      __INTERNAL__PMPI_Irecv (recvbuf, recvcnt, recvtype, root, -2, comm,
			      &request);


      if (rank == root)
	{
	  MPI_Aint send_type_size;

/* 	  __INTERNAL__PMPI_Pack_size(1,sendtype,comm,&send_type_size); */
	  __INTERNAL__PMPI_Type_extent (sendtype, &send_type_size);
	  i = 0;
	  while (i < size)
	    {
	      for (j = 0; (i < size) && (j < MPI_MAX_CONCURENT);)
		{
		  __INTERNAL__PMPI_Isend (((char *) sendbuf) +
					  (displs[i] * send_type_size),
					  sendcnts[i], sendtype, i, -2, comm,
					  &(sendrequest[j]));
		  i++;
		  j++;
		}
	      j--;
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(sendrequest[j]),
					 MPI_STATUS_IGNORE);
		}
	    }
	}

      __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Scatterv (sendbuf, sendcnts, displs, sendtype, recvbuf,
			   recvcnt, recvtype, root, comm);
    }
}
static int
__INTERNAL__PMPI_Allgather (void *sendbuf, int sendcount,
			    MPI_Datatype sendtype, void *recvbuf,
			    int recvcount, MPI_Datatype recvtype,
			    MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int root = 0;
      int size;
      __INTERNAL__PMPI_Comm_size (comm, &size);

      __INTERNAL__PMPI_Gather (sendbuf, sendcount, sendtype, recvbuf,
			       recvcount, recvtype, root, comm);
      __INTERNAL__PMPI_Bcast (recvbuf, size * recvcount, recvtype, root,
			      comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Allgather (sendbuf, sendcount, sendtype, recvbuf, recvcount,
			    recvtype, comm);
    }
}
static int
__INTERNAL__PMPI_Allgatherv (void *sendbuf, int sendcount,
			     MPI_Datatype sendtype, void *recvbuf,
			     int *recvcounts, int *displs,
			     MPI_Datatype recvtype, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int size;
      int root = 0;
      MPI_Aint dsize;

      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Gatherv (sendbuf, sendcount, sendtype,
				recvbuf, recvcounts, displs, recvtype, root,
				comm);
      size--;
/*       __INTERNAL__PMPI_Pack_size(1,recvtype,comm,&dsize); */
      __INTERNAL__PMPI_Type_extent (recvtype, &dsize);
      for (; size >= 0; size--)
	{
	  __INTERNAL__PMPI_Bcast (((char *) recvbuf) +
				  (displs[size] * dsize),
				  recvcounts[size], recvtype, root, comm);
	}
      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Allgatherv (sendbuf, sendcount, sendtype, recvbuf,
			     recvcounts, displs, recvtype, comm);
    }
}
static int
__INTERNAL__PMPI_Alltoall (void *sendbuf, int sendcount,
			   MPI_Datatype sendtype, void *recvbuf,
			   int recvcount, MPI_Datatype recvtype,
			   MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int i;
      int size;
      int rank;
      MPI_Request sendrequest;
      MPI_Request recvrequest;
      MPI_Aint d_send;
      MPI_Aint d_recv;

      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);

/*       __INTERNAL__PMPI_Pack_size(1,sendtype,comm,&d_send); */
      __INTERNAL__PMPI_Type_extent (sendtype, &d_send);
/*       __INTERNAL__PMPI_Pack_size(1,recvtype,comm,&d_recv); */
      __INTERNAL__PMPI_Type_extent (recvtype, &d_recv);
      for (i = 0; i < size; i++)
	{
	  __INTERNAL__PMPI_Isend (((char *) sendbuf) +
				  (i * sendcount * d_send),
				  sendcount, sendtype, i, -2, comm,
				  &sendrequest);
	  __INTERNAL__PMPI_Irecv (((char *) recvbuf) +
				  (i * recvcount * d_recv), recvcount,
				  recvtype, i, -2, comm, &recvrequest);

	  __INTERNAL__PMPI_Wait (&(sendrequest), MPI_STATUS_IGNORE);
	  __INTERNAL__PMPI_Wait (&(recvrequest), MPI_STATUS_IGNORE);
	}

      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Alltoall (sendbuf, sendcount, sendtype, recvbuf, recvcount,
			   recvtype, comm);
    }
}
static int
__INTERNAL__PMPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls,
			    MPI_Datatype sendtype, void *recvbuf,
			    int *recvcnts, int *rdispls,
			    MPI_Datatype recvtype, MPI_Comm comm)
{
  if (sctk_is_derived_type (sendtype) || sctk_is_derived_type (recvtype))
    {
      int i;
      int size;
      int rank;
      MPI_Request sendrequest;
      MPI_Request recvrequest;
      MPI_Aint d_send;
      MPI_Aint d_recv;

      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Comm_rank (comm, &rank);

/*       __INTERNAL__PMPI_Pack_size(1,sendtype,comm,&d_send); */
      __INTERNAL__PMPI_Type_extent (sendtype, &d_send);
/*       __INTERNAL__PMPI_Pack_size(1,recvtype,comm,&d_recv); */
      __INTERNAL__PMPI_Type_extent (recvtype, &d_recv);
      for (i = 0; i < size; i++)
	{
	  __INTERNAL__PMPI_Isend (((char *) sendbuf) +
				  (sdispls[i] * d_send),
				  sendcnts[i], sendtype, i, -2, comm,
				  &sendrequest);
	  __INTERNAL__PMPI_Irecv (((char *) recvbuf) + (rdispls[i] * d_recv),
				  recvcnts[i], recvtype, i, -2, comm,
				  &recvrequest);

	  __INTERNAL__PMPI_Wait (&(sendrequest), MPI_STATUS_IGNORE);
	  __INTERNAL__PMPI_Wait (&(recvrequest), MPI_STATUS_IGNORE);
	}
      __INTERNAL__PMPI_Barrier (comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Alltoallv (sendbuf, sendcnts, sdispls, sendtype, recvbuf,
			    recvcnts, rdispls, recvtype, comm);
    }
}

typedef struct
{
  MPC_Op op;
  int used;
  int commute;
} sctk_op_t;

typedef struct
{
  sctk_op_t *ops;
  int size;
  sctk_spinlock_t lock;
} sctk_mpi_ops_t;

#define MAX_MPI_DEFINED_OP 12

static sctk_op_t defined_op[MAX_MPI_DEFINED_OP];

#define sctk_add_op(operation) defined_op[MPI_##operation].op = MPC_##operation;defined_op[MPI_##operation].used = 1;defined_op[MPI_##operation].commute = 1

static void
__sctk_init_mpi_op ()
{
  sctk_mpi_ops_t *ops;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  static volatile int done = 0;
  int i;

  ops = (sctk_mpi_ops_t *) sctk_malloc (sizeof (sctk_mpi_ops_t));
  ops->size = 0;
  ops->ops = NULL;
  ops->lock = 0;

  sctk_thread_mutex_lock (&lock);
  if (done == 0)
    {
      for (i = 0; i < MAX_MPI_DEFINED_OP; i++)
	{
	  defined_op[i].used = 0;
	}

      sctk_add_op (SUM);
      sctk_add_op (MAX);
      sctk_add_op (MIN);
      sctk_add_op (PROD);
      sctk_add_op (LAND);
      sctk_add_op (BAND);
      sctk_add_op (LOR);
      sctk_add_op (BOR);
      sctk_add_op (LXOR);
      sctk_add_op (BXOR);
      sctk_add_op (MINLOC);
      sctk_add_op (MAXLOC);
      done = 1;
    }
  sctk_thread_mutex_unlock (&lock);
#ifdef MPC_Allocator
  sctk_add_global_var (defined_op, sizeof (defined_op));
#endif

  PMPC_Set_op (ops);
}

static sctk_op_t *
sctk_convert_to_mpc_op (MPI_Op op)
{
  sctk_mpi_ops_t *ops;
  sctk_op_t *tmp;
  if (op < MAX_MPI_DEFINED_OP)
    {
      assume (defined_op[op].used == 1);
      return &(defined_op[op]);
    }
  PMPC_Get_op ((void **) &ops);
  sctk_spinlock_lock (&(ops->lock));

  op -= MAX_MPI_DEFINED_OP;
  assume (op < ops->size);
  assume (ops->ops[op].used == 1);

  tmp = &(ops->ops[op]);

  sctk_spinlock_unlock (&(ops->lock));
  return tmp;
}

static int
__INTERNAL__PMPI_Reduce (void *sendbuf, void *recvbuf, int count,
			 MPI_Datatype datatype, MPI_Op op, int root,
			 MPI_Comm comm)
{
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;

  if (sctk_is_derived_type (datatype) || (mpi_op->commute == 0))
    {
      int size;
      int i;
      int rank;
      MPI_Request request;
      int dsize;
      void *tmp;
      int res;
      MPI_Datatype new_datatype;
      __INTERNAL__PMPI_Comm_rank (comm, &rank);
      __INTERNAL__PMPI_Comm_size (comm, &size);


      __INTERNAL__PMPI_Isend (sendbuf, count, datatype, root, -3, comm,
			      &request);

      if (rank == root)
	{
	  res =
	    __INTERNAL__PMPI_Type_contiguous (count, datatype, &new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Type_commit (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  res = __INTERNAL__PMPI_Pack_size (1, new_datatype, comm, &dsize);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }

	  tmp = sctk_malloc (dsize);
	  assume (tmp != NULL);

	  res =
	    __INTERNAL__PMPI_Recv (recvbuf, count, datatype, 0, -3, comm,
				   MPI_STATUS_IGNORE);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	  for (i = 1; i < size; i++)
	    {
	      sctk_nodebug ("RECV %d", i);
	      res =
		__INTERNAL__PMPI_Recv (tmp, count, datatype, i, -3, comm,
				       MPI_STATUS_IGNORE);
	      if (res != MPI_SUCCESS)
		{
		  return res;
		}
	      if (mpc_op.u_func != NULL)
		{
		  mpc_op.u_func (recvbuf, tmp, &count, &datatype);
		}
	      else
		{
		  not_reachable ();
		}

	      if ((i + 1) < size)
		{
		  int offset = 0;
		  __INTERNAL__PMPI_Unpack (tmp, dsize, &offset, recvbuf,
					   count, datatype, comm);
		}

	    }
	  sctk_free (tmp);
	  res = __INTERNAL__PMPI_Type_free (&new_datatype);
	  if (res != MPI_SUCCESS)
	    {
	      return res;
	    }
	}

      res = __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Barrier (comm);
      return res;
    }
  else
    {
      return PMPC_Reduce (sendbuf, recvbuf, count, datatype, mpc_op, root,
			 comm);
    }
}
static int
__INTERNAL__PMPI_Op_create (MPI_User_function * function, int commute,
			    MPI_Op * op)
{
  MPC_Op *mpc_op = NULL;
  sctk_mpi_ops_t *ops;
  int i;

  PMPC_Get_op ((void **) &ops);
  sctk_spinlock_lock (&(ops->lock));

  for (i = 0; i < ops->size; i++)
    {
      if (ops->ops[i].used == 0)
	{
	  mpc_op = &(ops->ops[i].op);
	  break;
	}
    }

  if (mpc_op == NULL)
    {
      i = ops->size;
      ops->size += 10;
      ops->ops = sctk_realloc (ops->ops, ops->size * sizeof (sctk_op_t));
      assume (ops->ops != NULL);
    }

  *op = i + MAX_MPI_DEFINED_OP;
  ops->ops[i].used = 1;
  mpc_op = &(ops->ops[i].op);

  ops->ops[i].commute = commute;

  if (commute)
    {
      PMPC_Op_create (function, commute, mpc_op);
    }
  else
    {
      mpc_op->u_func = function;
    }

  sctk_spinlock_unlock (&(ops->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Op_free (MPI_Op * op)
{
  MPC_Op *mpc_op = NULL;
  sctk_mpi_ops_t *ops;

  PMPC_Get_op ((void **) &ops);
  sctk_spinlock_lock (&(ops->lock));

  assume (*op >= MAX_MPI_DEFINED_OP);

  *op -= MAX_MPI_DEFINED_OP;

  assume (*op < ops->size);
  assume (ops->ops[*op].used == 1);

  mpc_op = &(ops->ops[*op].op);

  PMPC_Op_free (mpc_op);
  *op = MPI_OP_NULL;
  sctk_spinlock_unlock (&(ops->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
			    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;
  if (sctk_is_derived_type (datatype) || (mpi_op->commute == 0))
    {
      __INTERNAL__PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, 0,
			       comm);
      __INTERNAL__PMPI_Bcast (recvbuf, count, datatype, 0, comm);

      return MPI_SUCCESS;
    }
  else
    {

      return PMPC_Allreduce (sendbuf, recvbuf, count, datatype, mpc_op, comm);
    }
}
static int
__INTERNAL__PMPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts,
				 MPI_Datatype datatype, MPI_Op op,
				 MPI_Comm comm)
{
  int res;
  int i;
  MPI_Aint dsize;
  int size;
  int acc = 0;

  __INTERNAL__PMPI_Comm_size (comm, &size);
  __INTERNAL__PMPI_Type_extent (datatype, &dsize);

  for (i = 0; i < size; i++)
    {
      res =
	__INTERNAL__PMPI_Reduce (((char *) sendbuf) + (acc * dsize), recvbuf,
				 recvcnts[i], datatype, op, i, comm);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      acc += recvcnts[i];
    }

  res = __INTERNAL__PMPI_Barrier (comm);

  return res;
}

MPC_Op_f sctk_get_common_function (MPC_Datatype datatype, MPC_Op op);
static int
__INTERNAL__PMPI_Scan (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;
  int size;
  int rank;
  MPI_Request request;
  int dsize;
  void *tmp;
  int res;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;

  __INTERNAL__PMPI_Comm_rank (comm, &rank);
  __INTERNAL__PMPI_Comm_size (comm, &size);


  __INTERNAL__PMPI_Isend (sendbuf, count, datatype, rank, -3, comm, &request);

  res = __INTERNAL__PMPI_Pack_size (count, datatype, comm, &dsize);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  tmp = sctk_malloc (dsize);

  if (rank == 0)
    {
      res =
	__INTERNAL__PMPI_Recv (recvbuf, count, datatype, 0, -3, comm,
			       MPI_STATUS_IGNORE);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  else
    {
      res =
	__INTERNAL__PMPI_Recv (recvbuf, count, datatype, rank, -3, comm,
			       MPI_STATUS_IGNORE);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res =
	__INTERNAL__PMPI_Recv (tmp, count, datatype, rank - 1, -3, comm,
			       MPI_STATUS_IGNORE);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      if (mpc_op.u_func != NULL)
	{
	  mpc_op.u_func (tmp, recvbuf, &count, &datatype);
	}
      else
	{
	  MPC_Op_f func;
	  func = sctk_get_common_function (datatype, mpc_op);
	  func (tmp, recvbuf, count, datatype);
	}
    }

  if (rank + 1 < size)
    {
      res =
	__INTERNAL__PMPI_Send (recvbuf, count, datatype, rank + 1, -3, comm);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }

  sctk_free (tmp);
  res = __INTERNAL__PMPI_Wait (&(request), MPI_STATUS_IGNORE);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  res = __INTERNAL__PMPI_Barrier (comm);

  return res;
}

typedef struct MPI_internal_group_s
{
  MPC_Group group;
  int used;
  volatile struct MPI_internal_group_s *next;
  int rank;
} MPI_internal_group_t;

typedef struct
{
  sctk_spinlock_t lock;
  long max_size;
  MPI_internal_group_t **tab;
  volatile MPI_internal_group_t *free_list;
  sctk_alloc_buffer_t buf;
} MPI_group_struct_t;


static inline MPI_internal_group_t *
__sctk_new_mpc_group_internal (MPI_Group * group)
{
  MPI_group_struct_t *groups;
  MPI_internal_group_t *tmp;

  PMPC_Get_groups ((void *) &groups);

  sctk_spinlock_lock (&(groups->lock));
  if (groups->free_list == NULL)
    {
      long old_size;
      long i;
      old_size = groups->max_size;
      groups->max_size += 10;
      groups->tab =
	sctk_realloc (groups->tab,
		      groups->max_size * sizeof (MPI_internal_group_t *));
      assume (groups->tab != NULL);
      for (i = groups->max_size - 1; i >= old_size; i--)
	{
	  MPI_internal_group_t *tmp;
	  sctk_nodebug ("%lu", i);
/* 	  tmp = sctk_malloc(sizeof (MPI_internal_group_t)); */
	  tmp =
	    sctk_buffered_malloc (&(groups->buf),
				  sizeof (MPI_internal_group_t));
	  assume (tmp != NULL);
	  groups->tab[i] = tmp;
	  tmp->used = 0;
	  tmp->next = groups->free_list;
	  groups->free_list = tmp;
	  tmp->rank = i;
	}
    }
  tmp = (MPI_internal_group_t *) groups->free_list;
  tmp->used = 1;
  groups->free_list = tmp->next;
  sctk_spinlock_unlock (&(groups->lock));
  *group = tmp->rank;
  sctk_nodebug ("Create group %d", *group);
  return tmp;
}

static inline MPC_Group
__sctk_new_mpc_group (MPI_Group * group)
{
  MPI_internal_group_t *tmp;
  tmp = __sctk_new_mpc_group_internal (group);
  return tmp->group;
}

static inline void
__sctk_init_mpc_group ()
{
  MPI_group_struct_t *groups;
  MPI_Group EMPTY;
  static MPC_Group_t mpc_mpi_group_empty;

  groups = sctk_malloc (sizeof (MPI_group_struct_t));

  groups->lock = 0;
  groups->tab = NULL;
  groups->free_list = NULL;
  groups->max_size = 0;
  sctk_buffered_alloc_create (&(groups->buf), sizeof (MPI_internal_group_t));

  PMPC_Set_groups (groups);

  /* Init group empty */
  memcpy (&mpc_mpi_group_empty, &mpc_group_empty, sizeof (MPC_Group_t));
  __sctk_new_mpc_group_internal (&EMPTY)->group = &mpc_mpi_group_empty;
  assume (EMPTY == MPI_GROUP_EMPTY);

}

static inline MPI_internal_group_t *
__sctk_convert_mpc_group_internal (MPI_Group group)
{
  MPI_internal_group_t *tmp;
  MPI_group_struct_t *groups;
  int int_group;

  int_group = group;
  if (int_group == MPI_GROUP_NULL)
    {
      return NULL;
    }


  PMPC_Get_groups ((void *) &groups);

  assume (groups != NULL);

  sctk_spinlock_lock (&(groups->lock));
  assume (((int_group) >= 0) && ((int_group) < groups->max_size));
  tmp = groups->tab[int_group];
  assume (tmp->rank == group);
  assume (tmp->used);
  sctk_spinlock_unlock (&(groups->lock));
  assume (tmp != NULL);
  return tmp;
}
static inline MPC_Group
__sctk_convert_mpc_group (MPI_Group group)
{
  MPI_internal_group_t *tmp;

  tmp = __sctk_convert_mpc_group_internal (group);
  if ((tmp == NULL))
    {
      return MPC_GROUP_NULL;
    }
  assume (tmp->group != NULL);
  return tmp->group;
}

static inline void
__sctk_delete_mpc_group (MPI_Group * group)
{
  MPI_group_struct_t *groups;
  MPI_internal_group_t *tmp;
  int int_group;

  int_group = *group;
  if (int_group == MPI_GROUP_NULL)
    {
      return;
    }

  PMPC_Get_groups ((void *) &groups);
  assume (groups != NULL);

  sctk_nodebug ("Delete group %d", *group);
  sctk_spinlock_lock (&(groups->lock));
  assume (((*group) >= 0) && ((*group) < groups->max_size));
  tmp = groups->tab[*group];
  assume (tmp->rank == *group);
  tmp->used = 0;
  tmp->next = groups->free_list;
  groups->free_list = tmp;
  *group = MPI_GROUP_NULL;
  sctk_spinlock_unlock (&(groups->lock));
}


static int
__INTERNAL__PMPI_Group_size (MPI_Group group, int *size)
{
  *size = __sctk_convert_mpc_group (group)->task_nb;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_rank (MPI_Group mpi_group, int *rank)
{
  int i;
  int grank;
  MPC_Group group;
  group = __sctk_convert_mpc_group (mpi_group);
  __INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &grank);
  *rank = MPI_UNDEFINED;
  for (i = 0; i < group->task_nb; i++)
    {
      if (group->task_list[i] == grank)
	{
	  *rank = i;
	  return MPI_SUCCESS;
	}
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_translate_ranks (MPI_Group mpi_group1, int n,
					int *ranks1, MPI_Group mpi_group2,
					int *ranks2)
{
  int j;
  MPC_Group group1;
  MPC_Group group2;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  for (j = 0; j < n; j++)
    {
      int i;
      int grank;
      grank = group1->task_list[ranks1[j]];
      ranks2[j] = MPI_UNDEFINED;
      for (i = 0; i < group2->task_nb; i++)
	{
	  if (group2->task_list[i] == grank)
	    {
	      ranks2[j] = i;
	      sctk_nodebug ("%d is %d", ranks1[j], i);
	    }
	}
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_compare (MPI_Group mpi_group1, MPI_Group mpi_group2,
				int *result)
{
  int i;
  int is_ident = 1;
  int is_similar = 1;
  MPC_Group group1;
  MPC_Group group2;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  *result = MPI_UNEQUAL;

  if (group1->task_nb != group2->task_nb)
    {
      *result = MPI_UNEQUAL;
      return MPI_SUCCESS;
    }

  for (i = 0; i < group1->task_nb; i++)
    {
      if (group1->task_list[i] != group2->task_list[i])
	{
	  is_ident = 0;
	  break;
	}
    }

  if (is_ident)
    {
      *result = MPI_IDENT;
      return MPI_SUCCESS;
    }

  for (i = 0; i < group1->task_nb; i++)
    {
      int j;
      int found = 0;
      for (j = 0; j < group2->task_nb; j++)
	{
	  if (group1->task_list[i] == group2->task_list[j])
	    {
	      found = 1;
	      break;
	    }
	}
      if (found == 0)
	{
	  is_similar = 0;
	  break;
	}
    }

  if (is_similar)
    {
      *result = MPI_SIMILAR;
      return MPI_SUCCESS;
    }

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Comm_group (MPI_Comm comm, MPI_Group * mpi_newgroup)
{
  MPI_internal_group_t *newgroup;
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  return PMPC_Comm_group (comm, &(newgroup->group));
}

static int
__INTERNAL__PMPI_Group_union (MPI_Group mpi_group1, MPI_Group mpi_group2,
			      MPI_Group * mpi_newgroup)
{
  int size;
  int j;
  int i;
  MPC_Group group1;
  MPC_Group group2;
  MPC_Group newgroup;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);

  size = group1->task_nb + group2->task_nb;
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  memcpy ((newgroup)->task_list, group1->task_list,
	  group1->task_nb * sizeof (int));
  memcpy (&((newgroup)->task_list[group1->task_nb]), group2->task_list,
	  group2->task_nb * sizeof (int));

  for (i = 0; i < group1->task_nb; i++)
    {
      for (j = 0; j < group2->task_nb; j++)
	{
	  if (group1->task_list[i] == group2->task_list[j])
	    {
	      (&((newgroup)->task_list[group1->task_nb]))[j] = -1;
	      size--;
	    }
	}
    }

  for (i = 0; i < (newgroup)->task_nb; i++)
    {
      if ((newgroup)->task_list[i] == -1)
	{
	  for (j = i; j < (newgroup)->task_nb - 1; j++)
	    {
	      (newgroup)->task_list[j] = (newgroup)->task_list[j + 1];
	    }
	}
    }

  (newgroup)->task_nb = size;

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_intersection (MPI_Group mpi_group1,
				     MPI_Group mpi_group2,
				     MPI_Group * mpi_newgroup)
{
  int i;
  int size;
  MPC_Group group1;
  MPC_Group group2;
  MPC_Group newgroup;
  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  size = group1->task_nb;

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  size = 0;
  for (i = 0; i < group1->task_nb; i++)
    {
      int j;
      for (j = 0; j < group2->task_nb; j++)
	{
	  if (group1->task_list[i] == group2->task_list[j])
	    {
	      (newgroup)->task_list[size] = group1->task_list[i];
	      size++;
	      break;
	    }
	}
    }
  (newgroup)->task_nb = size;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_difference (MPI_Group mpi_group1, MPI_Group mpi_group2,
				   MPI_Group * mpi_newgroup)
{
  MPC_Group group1;
  MPC_Group group2;
  MPI_internal_group_t *newgroup;

  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  return PMPC_Group_difference (group1, group2, &(newgroup->group));
}

static int
__INTERNAL__PMPI_Group_incl (MPI_Group mpi_group, int n, int *ranks,
			     MPI_Group * mpi_newgroup)
{
  int res;
  MPC_Group group;
  MPI_internal_group_t *newgroup;

  group = __sctk_convert_mpc_group (mpi_group);
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);

  res = PMPC_Group_incl (group, n, ranks, &(newgroup->group));

  return res;
}

static int
__INTERNAL__PMPI_Group_excl (MPI_Group mpi_group, int n, int *ranks,
			     MPI_Group * mpi_newgroup)
{
  int i;
  int j;
  int size;
  MPC_Group group;
  MPC_Group newgroup;

  group = __sctk_convert_mpc_group (mpi_group);
  newgroup = __sctk_new_mpc_group (mpi_newgroup);
  size = group->task_nb;

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  for (i = 0; i < group->task_nb; i++)
    {
      (newgroup)->task_list[i] = group->task_list[i];
    }

  for (i = 0; i < n; i++)
    {
      (newgroup)->task_list[ranks[i]] = -1;
    }

  (newgroup)->task_nb = group->task_nb;
  for (i = 0; i < (newgroup)->task_nb; i++)
    {
      if ((newgroup)->task_list[i] == -1)
	{
	  size--;
	  for (j = i; j < (newgroup)->task_nb - 1; j++)
	    {
	      (newgroup)->task_list[j] = (newgroup)->task_list[j + 1];
	    }
	}
    }

  (newgroup)->task_nb = size;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_range_excl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
  int i;
  int j;
  int size;
  MPC_Group group;
  MPC_Group newgroup;

  group = __sctk_convert_mpc_group (mpi_group);
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  size = group->task_nb;

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  for (i = 0; i < size; i++)
    {
      (newgroup)->task_list[i] = group->task_list[i];
    }

  for (i = 0; i < n; i++)
    {
      int first;
      int last;
      int stride;

      first = ranges[i][0];
      last = ranges[i][1];
      stride = ranges[i][2];

      sctk_nodebug ("first %d last %d stride %d", first, last, stride);

      for (j = 0; j <= ((last - first) / (stride)); j++)
	{
	  assume (first + j * stride < group->task_nb);
	  (newgroup)->task_list[first + j * stride] = -1;
	  sctk_nodebug ("remove rank %d", first + j * stride);
	  size--;
	}
    }

  for (i = 0; i < (newgroup)->task_nb; i++)
    {
      if ((newgroup)->task_list[i] == -1)
	{
	  for (j = i; j < (newgroup)->task_nb - 1; j++)
	    {
	      (newgroup)->task_list[j] = (newgroup)->task_list[j + 1];
	    }
	}
    }

  (newgroup)->task_nb = size;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_range_incl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
  int i;
  int j;
  int size;
  MPC_Group group;
  MPC_Group newgroup;

  group = __sctk_convert_mpc_group (mpi_group);
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  size = group->task_nb;

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  for (i = 0; i < size; i++)
    {
      (newgroup)->task_list[i] = -1;
    }
  size = 0;

  for (i = 0; i < n; i++)
    {
      int first;
      int last;
      int stride;

      first = ranges[i][0];
      last = ranges[i][1];
      stride = ranges[i][2];

      sctk_nodebug ("first %d last %d stride %d", first, last, stride);

      for (j = 0; j <= ((last - first) / (stride)); j++)
	{
	  (newgroup)->task_list[i * n + j] = first + j * stride;
	  sctk_nodebug ("[%d] = %d", i * n + j,
			(newgroup)->task_list[i * n + j]);
	  size++;
	  assume (size <= group->task_nb);
	}
    }

  (newgroup)->task_nb = size;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_free (MPI_Group * mpi_group)
{
  int res;
  MPC_Group group;
  group = __sctk_convert_mpc_group (*mpi_group);

  res = PMPC_Group_free (&group);

  __sctk_delete_mpc_group (mpi_group);

  return res;
}

static int
__INTERNAL__PMPI_Comm_size (MPI_Comm comm, int *size)
{
  return PMPC_Comm_size (comm, size);
}

static int
__INTERNAL__PMPI_Comm_rank (MPI_Comm comm, int *rank)
{
  return PMPC_Comm_rank (comm, rank);
}

static int
__INTERNAL__PMPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  *result = MPI_UNEQUAL;
  if (comm1 == comm2)
    {
      *result = MPI_IDENT;
      return MPI_SUCCESS;
    }
  else
    {
      MPI_Group comm_group1;
      MPI_Group comm_group2;

      __INTERNAL__PMPI_Comm_group (comm1, &comm_group1);
      __INTERNAL__PMPI_Comm_group (comm2, &comm_group2);

      __INTERNAL__PMPI_Group_compare (comm_group1, comm_group2, result);

      if (*result == MPI_IDENT)
	{
	  *result = MPI_CONGRUENT;
	}
    }
  if (sctk_is_inter_comm (comm1) != sctk_is_inter_comm (comm2))
    {
      *result = MPI_UNEQUAL;
    }

  return MPI_SUCCESS;
}

static int SCTK__MPI_Comm_communicator_dup (MPI_Comm comm, MPI_Comm newcomm);
static int SCTK__MPI_Comm_communicator_free (MPI_Comm comm);


static int
__INTERNAL__PMPI_Comm_dup (MPI_Comm comm, MPI_Comm * newcomm)
{
  int res;
  MPI_Errhandler err;
  res = PMPC_Comm_dup (comm, newcomm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  res = __INTERNAL__PMPI_Errhandler_get (comm, &err);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  res = __INTERNAL__PMPI_Errhandler_set (*newcomm, err);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  res = SCTK__MPI_Attr_communicator_dup (comm, *newcomm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  res = SCTK__MPI_Comm_communicator_dup (comm, *newcomm);
  return res;
}

static int
__INTERNAL__PMPI_Comm_create (MPI_Comm comm, MPI_Group group,
			      MPI_Comm * newcomm)
{
  return PMPC_Comm_create (comm, __sctk_convert_mpc_group (group), newcomm);
}

static int
__INTERNAL__PMPI_Comm_split (MPI_Comm comm, int color, int key,
			     MPI_Comm * newcomm)
{
  int res;
  res = PMPC_Comm_split (comm, color, key, newcomm);

  return res;
}

static int
__INTERNAL__PMPI_Comm_free (MPI_Comm * comm)
{
  int res;
  res = SCTK__MPI_Attr_clean_communicator (*comm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  SCTK__MPI_Comm_communicator_free (*comm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  return PMPC_Comm_free (comm);
}

static int
__INTERNAL__PMPI_Comm_test_inter (MPI_Comm comm, int *flag)
{
  int res = MPI_SUCCESS;
  *flag = sctk_is_inter_comm (comm);
  return res;
}

static int
__INTERNAL__PMPI_Comm_remote_size (MPI_Comm comm, int *size)
{
  return PMPC_Comm_remote_size (comm, size);
}

static int
__INTERNAL__PMPI_Comm_remote_group (MPI_Comm comm, MPI_Group * mpi_newgroup)
{
  MPI_internal_group_t *newgroup;
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  return PMPC_Comm_remote_group (comm, &(newgroup->group));
}
static int
__INTERNAL__PMPI_Intercomm_create (MPI_Comm local_comm, int local_leader,
				   MPI_Comm peer_comm, int remote_leader,
				   int tag, MPI_Comm * newintercomm)
{
  MPI_Group remote_group;

  __INTERNAL__PMPI_Barrier (local_comm);

  __INTERNAL__PMPI_Comm_group (peer_comm, &remote_group);

  __INTERNAL__PMPI_Comm_dup (local_comm, newintercomm);

  PMPC_Convert_to_intercomm (*newintercomm,
			    __sctk_convert_mpc_group (remote_group));

  __INTERNAL__PMPI_Barrier (local_comm);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Intercomm_merge (MPI_Comm intercomm, int high,
				  MPI_Comm * newintracomm)
{
  MPI_Group local_group;
  MPI_Group remote_group;
  MPI_Group new_group;

  __INTERNAL__PMPI_Comm_group (intercomm, &local_group);
  __INTERNAL__PMPI_Comm_remote_group (intercomm, &remote_group);

  if (high)
    {
      __INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
    }
  else
    {
      __INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
    }
  return __INTERNAL__PMPI_Comm_create (intercomm, new_group, newintracomm);
}

/*
  Attributes
*/
typedef struct
{
  MPI_Copy_function *copy_fn;
  MPI_Delete_function *delete_fn;
  void *extra_state;
  volatile int used;
  volatile int fortran_key;
} MPI_Caching_key_t;

typedef struct
{
  void *attr;
  int flag;
} MPI_Caching_key_value_t;

typedef struct
{
  volatile MPI_Caching_key_t *attrs_fn;
  volatile MPI_Caching_key_value_t *value[SCTK_MAX_COMMUNICATOR_NUMBER];
  sctk_thread_mutex_t lock;
  volatile int number;
  volatile int max_number;
} MPI_Caching_struct_t;

#define MPI_MAX_KEY_DEFINED 7
const int MPI_TAG_UB = 0;
const int MPI_HOST = 1;
const int MPI_IO = 2;
const int MPI_WTIME_IS_GLOBAL = 3;
const int MPI_UNIVERSE_SIZE = MPI_KEYVAL_INVALID;
const int MPI_LASTUSEDCODE = MPI_KEYVAL_INVALID;
const int MPI_APPNUM = MPI_KEYVAL_INVALID;

static int MPI_TAG_UB_VALUE = 32767;
static char *MPI_HOST_VALUE[4096];
static int MPI_IO_VALUE = 0;
static int MPI_WTIME_IS_GLOBAL_VALUE = 0;

typedef int (MPI_Copy_function_fortran) (MPI_Comm *, int *, int *, int *,
					 int *, int *, int *);
typedef int (MPI_Delete_function_fortran) (MPI_Comm *, int *, int *, int *,
					   int *);

static void *defines_attr_tab[MPI_MAX_KEY_DEFINED] = {
  (void *) &MPI_TAG_UB_VALUE /*MPI_TAG_UB */ ,
  &MPI_HOST_VALUE,
  &MPI_IO_VALUE,
  &MPI_WTIME_IS_GLOBAL_VALUE
};

static sctk_thread_mutex_t sctk_attr_lock = SCTK_THREAD_MUTEX_INITIALIZER;
static int
__INTERNAL__PMPI_Keyval_create (MPI_Copy_function * copy_fn,
				MPI_Delete_function * delete_fn,
				int *keyval, void *extra_state)
{
  MPI_Caching_struct_t *keys;
  int i;
  sctk_thread_mutex_lock (&sctk_attr_lock);
  PMPC_Get_keys ((void *) &keys);
  if (keys == NULL)
    {
      keys = sctk_malloc (sizeof (MPI_Caching_struct_t));
      PMPC_Set_keys (keys);
      keys->number = 0;
      keys->max_number = 0;
      sctk_thread_mutex_init (&(keys->lock), NULL);
      for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
	{
	  keys->value[i] = NULL;
	}
      keys->attrs_fn = NULL;
    }
  sctk_thread_mutex_unlock (&sctk_attr_lock);

  sctk_thread_mutex_lock (&(keys->lock));
  if (keys->number >= keys->max_number)
    {
      int j;
      keys->max_number += 100;

      for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
	{
	  keys->value[i] =
	    sctk_realloc ((void *) (keys->value[i]),
			  keys->max_number *
			  sizeof (MPI_Caching_key_value_t));
	  for (j = keys->number; j < keys->max_number; j++)
	    {
	      keys->value[i][j].flag = 0;
	    }
	}
      keys->attrs_fn =
	sctk_realloc ((void *) (keys->attrs_fn),
		      keys->max_number * sizeof (MPI_Caching_key_t));
      for (i = keys->number; i < keys->max_number; i++)
	{
	  keys->attrs_fn[i].used = 0;
	}
    }

  *keyval = keys->number;
  keys->number++;

  keys->attrs_fn[*keyval].copy_fn = copy_fn;
  keys->attrs_fn[*keyval].delete_fn = delete_fn;
  keys->attrs_fn[*keyval].extra_state = extra_state;
  keys->attrs_fn[*keyval].used = 1;
  keys->attrs_fn[*keyval].fortran_key = 0;

  sctk_thread_mutex_unlock (&(keys->lock));
  *keyval += MPI_MAX_KEY_DEFINED;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Keyval_free (int *keyval)
{
  MPI_Caching_struct_t *keys;
  PMPC_Get_keys ((void *) &keys);
  if ((keys == NULL) || (*keyval < 0))
    {
      return MPI_ERR_INTERN;
    }

  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Attr_set_fortran (int keyval)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_SUCCESS;
  MPI_Caching_struct_t *keys;
  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  keyval -= MPI_MAX_KEY_DEFINED;

  PMPC_Get_keys ((void *) &keys);
  if ((keys == NULL) || (keyval < 0))
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  sctk_thread_mutex_lock (&(keys->lock));
  if (keys->attrs_fn[keyval].used == 0)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }

  keys->attrs_fn[keyval].fortran_key = 1;
  sctk_thread_mutex_unlock (&(keys->lock));

  return res;
}


static int
__INTERNAL__PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
  int res = MPI_SUCCESS;
  MPI_Caching_struct_t *keys;
  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  keyval -= MPI_MAX_KEY_DEFINED;

  PMPC_Get_keys ((void *) &keys);
  if ((keys == NULL) || (keyval < 0))
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  sctk_thread_mutex_lock (&(keys->lock));
  if (keys->attrs_fn[keyval].used == 0)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  if (keys->value[comm][keyval].flag == 1)
    {
      res = __INTERNAL__PMPI_Attr_delete (comm, keyval);
    }
  if (keys->attrs_fn[keyval].fortran_key == 0)
    {
      keys->value[comm][keyval].attr = attr_value;
    }
  else
    {
      int val;
      long long_val;
      val = (*((int *) attr_value));
      long_val = (long) val;
      keys->value[comm][keyval].attr = (void *) long_val;
    }
  keys->value[comm][keyval].flag = 1;
  sctk_thread_mutex_unlock (&(keys->lock));
  return res;
}

static int
__INTERNAL__PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value,
			   int *flag)
{
  MPI_Caching_struct_t *keys;
  void **attr;

  *flag = 0;
  attr = (void **) attr_value;
  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      *flag = 1;
      *attr = defines_attr_tab[keyval];
      return MPI_SUCCESS;
    }
  keyval -= MPI_MAX_KEY_DEFINED;

  PMPC_Get_keys ((void *) &keys);
  if ((keys == NULL) || (keyval < 0))
    {
      return MPI_ERR_INTERN;
    }
  sctk_thread_mutex_lock (&(keys->lock));
  if (keys->attrs_fn[keyval].used == 0)
    {
      sctk_thread_mutex_unlock (&(keys->lock));
      return MPI_ERR_INTERN;
    }

  *flag = keys->value[comm][keyval].flag;
  if (keys->attrs_fn[keyval].fortran_key == 0)
    {
      *attr = keys->value[comm][keyval].attr;
    }
  else
    {
      *attr = keys->value[comm][keyval].attr;
    }

  sctk_thread_mutex_unlock (&(keys->lock));
  sctk_nodebug ("Get %d = %ld", keyval + MPI_MAX_KEY_DEFINED,
		*((unsigned long *) attr), *((unsigned long *) attr_value));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
  MPI_Caching_struct_t *keys;
  int res;
  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      return MPI_ERR_INTERN;
    }
  keyval -= MPI_MAX_KEY_DEFINED;
  PMPC_Get_keys ((void *) &keys);
  if ((keys == NULL) || (keyval < 0))
    {
      return MPI_ERR_INTERN;
    }
  sctk_thread_mutex_lock (&(keys->lock));
  if (keys->attrs_fn[keyval].used == 0)
    {
      sctk_thread_mutex_unlock (&(keys->lock));
      return MPI_ERR_INTERN;
    }

  if (keys->value[comm][keyval].flag == 1)
    {
      if (keys->attrs_fn[keyval].delete_fn != NULL)
	{
	  if (keys->attrs_fn[keyval].fortran_key == 0)
	    {
	      res =
		keys->attrs_fn[keyval].delete_fn (comm,
						  keyval +
						  MPI_MAX_KEY_DEFINED,
						  keys->value[comm][keyval].
						  attr,
						  keys->attrs_fn[keyval].
						  extra_state);
	    }
	  else
	    {
	      int fort_key;
	      int val;
	      long long_val;
	      int *ext;
	      long_val = (long) (keys->value[comm][keyval].attr);
	      val = (int) long_val;
	      fort_key = keyval + MPI_MAX_KEY_DEFINED;
	      ext = (int *) (keys->attrs_fn[keyval].extra_state);

	      ((MPI_Delete_function_fortran *) keys->attrs_fn[keyval].
	       delete_fn) (&comm, &fort_key, &val, ext, &res);
	    }
	}
    }

  keys->value[comm][keyval].attr = NULL;
  keys->value[comm][keyval].flag = 0;
  sctk_thread_mutex_unlock (&(keys->lock));
  return res;
}

static int
SCTK__MPI_Attr_clean_communicator (MPI_Comm comm)
{
  MPI_Caching_struct_t *keys;
  int i;
  int res = MPI_SUCCESS;
  PMPC_Get_keys ((void *) &keys);
  if (keys == NULL)
    {
      return res;
    }
  sctk_thread_mutex_lock (&sctk_attr_lock);
  for (i = 0; i < keys->max_number; i++)
    {
      if (keys->value[comm][i].flag == 1)
	{
	  res = __INTERNAL__PMPI_Attr_delete (comm, i + MPI_MAX_KEY_DEFINED);

	  if (res != MPI_SUCCESS)
	    {
	      sctk_thread_mutex_unlock (&sctk_attr_lock);
	      return res;
	    }

	}
    }
  sctk_thread_mutex_unlock (&sctk_attr_lock);
  return res;
}

int
MPI_DUP_FN (MPI_Comm oldcom, int keyval, void *extra_state,
	    void *attr_val_in, void *attr_val_out, int *flag)
{
  void **attr;
  attr = (void **) attr_val_out;
  *attr = attr_val_in;
  *flag = 1;
  return MPI_SUCCESS;
}

static int
SCTK__MPI_Attr_communicator_dup (MPI_Comm old, MPI_Comm new)
{
  MPI_Caching_struct_t *keys;
  int res = MPI_SUCCESS;
  int i;
  PMPC_Get_keys ((void *) &keys);
  if (keys == NULL)
    {
      return res;
    }
  sctk_thread_mutex_lock (&sctk_attr_lock);
  for (i = 0; i < keys->max_number; i++)
    {
      if (keys->attrs_fn[i].copy_fn != NULL)
	{
	  if (keys->value[old][i].flag == 1)
	    {
	      void *arg;
	      int flag;
	      MPI_Copy_function *cpy;

	      cpy = keys->attrs_fn[i].copy_fn;

	      if (keys->attrs_fn[i].fortran_key == 0)
		{
		  res =
		    cpy (old, i + MPI_MAX_KEY_DEFINED,
			 keys->attrs_fn[i].extra_state,
			 keys->value[old][i].attr, (void *) (&arg), &flag);
		}
	      else
		{
		  int fort_key;
		  int val;
		  int *ext;
		  int val_out;
		  long long_val;
		  long_val = (long) (keys->value[old][i].attr);
		  val = (int) long_val;
		  fort_key = i + MPI_MAX_KEY_DEFINED;
		  ext = (int *) (keys->attrs_fn[i].extra_state);
		  sctk_nodebug ("%d val", val);
		  ((MPI_Copy_function_fortran *) cpy) (&old, &fort_key, ext,
						       &val, &val_out, &flag,
						       &res);
		  sctk_nodebug ("%d val_out", val_out);
		  arg = &val_out;
		}
	      sctk_nodebug ("Copy %d %ld->%ld flag %d",
			    i + MPI_MAX_KEY_DEFINED,
			    (unsigned long) keys->value[old][i].attr,
			    (unsigned long) arg, flag);
	      if (flag)
		{
		  __INTERNAL__PMPI_Attr_put (new, i + MPI_MAX_KEY_DEFINED,
					     arg);
		}
	      if (res != MPI_SUCCESS)
		{
		  sctk_thread_mutex_unlock (&sctk_attr_lock);
		  return res;
		}

	    }
	}
    }
  sctk_thread_mutex_unlock (&sctk_attr_lock);
  return res;
}

typedef struct
{
  int ndims;
  int *dims;
  int *periods;
  int reorder;
} mpi_topology_cart_t;

typedef struct
{
  int nnodes;
  int nedges;
  int nindex;

  int *edges;
  int *index;
  int reorder;
} mpi_topology_graph_t;

typedef union
{
  mpi_topology_cart_t cart;
  mpi_topology_graph_t graph;
} mpi_topology_info_t;

typedef struct
{
  int type[SCTK_MAX_COMMUNICATOR_NUMBER];
  mpi_topology_info_t data[SCTK_MAX_COMMUNICATOR_NUMBER];
  char names[SCTK_MAX_COMMUNICATOR_NUMBER][MPI_MAX_NAME_STRING + 1];
  sctk_spinlock_t lock;
} mpi_topology_t;

static void
__sctk_init_mpi_topo ()
{
  int i;
  mpi_topology_t *task_specific;
  task_specific = sctk_malloc (sizeof (mpi_topology_t));
  for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
    {
      task_specific->type[i] = MPI_UNDEFINED;
      sprintf (task_specific->names[i], "undefined");
    }
  sprintf (task_specific->names[0], "MPI_COMM_WORLD");
  PMPC_Set_comm_type (task_specific);
}

static int
SCTK__MPI_Comm_communicator_dup (MPI_Comm comm, MPI_Comm newcomm)
{
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] == MPI_CART)
    {
      task_specific->type[newcomm] = MPI_CART;
      task_specific->data[newcomm].cart.ndims =
	task_specific->data[comm].cart.ndims;
      task_specific->data[newcomm].cart.reorder =
	task_specific->data[comm].cart.reorder;
      task_specific->data[newcomm].cart.dims =
	sctk_malloc (task_specific->data[comm].cart.ndims * sizeof (int));
      memcpy (task_specific->data[newcomm].cart.dims,
	      task_specific->data[comm].cart.dims,
	      task_specific->data[comm].cart.ndims * sizeof (int));
      task_specific->data[newcomm].cart.periods =
	sctk_malloc (task_specific->data[comm].cart.ndims * sizeof (int));
      memcpy (task_specific->data[newcomm].cart.periods,
	      task_specific->data[comm].cart.periods,
	      task_specific->data[comm].cart.ndims * sizeof (int));
    }

  if (task_specific->type[comm] == MPI_GRAPH)
    {
      task_specific->type[newcomm] = MPI_GRAPH;

      task_specific->data[newcomm].graph.nnodes =
	task_specific->data[comm].graph.nnodes;
      task_specific->data[newcomm].graph.reorder =
	task_specific->data[comm].graph.reorder;
      task_specific->data[newcomm].graph.index =
	sctk_malloc (task_specific->data[comm].graph.nnodes * sizeof (int));
      memcpy (task_specific->data[newcomm].graph.index,
	      task_specific->data[comm].graph.index,
	      task_specific->data[comm].graph.nnodes * sizeof (int));
      task_specific->data[newcomm].graph.edges =
	sctk_malloc (task_specific->data[comm].graph.nnodes * sizeof (int));
      memcpy (task_specific->data[newcomm].graph.edges,
	      task_specific->data[comm].graph.edges,
	      task_specific->data[comm].graph.nnodes * sizeof (int));

      task_specific->data[newcomm].graph.nedges =
	task_specific->data[comm].graph.nedges;
      task_specific->data[newcomm].graph.nindex =
	task_specific->data[comm].graph.nindex;
    }
  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
SCTK__MPI_Comm_communicator_free (MPI_Comm comm)
{
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] == MPI_CART)
    {
      sctk_free (task_specific->data[comm].cart.dims);
      sctk_free (task_specific->data[comm].cart.periods);
    }
  if (task_specific->type[comm] == MPI_GRAPH)
    {
      sctk_free (task_specific->data[comm].graph.index);
      sctk_free (task_specific->data[comm].graph.edges);
    }
  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Comm_get_name (MPI_Comm comm, char *comm_name,
				int *resultlen)
{
  mpi_topology_t *task_specific;
  int len;

  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  len = strlen (task_specific->names[comm]);

  memcpy (comm_name, task_specific->names[comm], len + 1);

  *resultlen = len;

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}
static int
__INTERNAL__PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  mpi_topology_t *task_specific;
  int len;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  memset (task_specific->names[comm], '\0', MPI_MAX_NAME_STRING + 1);

  len = strlen (comm_name);
  if (len > MPI_MAX_NAME_STRING)
    {
      len = MPI_MAX_NAME_STRING;
    }
  memcpy (task_specific->names[comm], comm_name, len);

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  *topo_type = task_specific->type[comm];

  sctk_nodebug ("type %d", *topo_type);

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims,
			      int *periods, int reorder, MPI_Comm * comm_cart)
{
  mpi_topology_t *task_specific;
  int nb_tasks = 1;
  int size;
  int res = MPI_ERR_INTERN;
  int i;
  int rank;

  __INTERNAL__PMPI_Comm_rank (comm_old, &rank);

  for (i = 0; i < ndims; i++)
    {
      nb_tasks *= dims[i];
      sctk_nodebug ("dims[%d] = %d", i, dims[i]);
    }

  __INTERNAL__PMPI_Comm_size (comm_old, &size);

  sctk_nodebug ("%d <= %d", nb_tasks, size);
  if (nb_tasks > size)
    {
      MPI_ERROR_REPORT (comm_old, MPI_ERR_COMM, "too small group size");
    }

#warning "Very simple approach never reorder nor take care of hardware topology"

  if (nb_tasks == size)
    {
      res = __INTERNAL__PMPI_Comm_dup (comm_old, comm_cart);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  else
    {
      MPI_Group comm_group;
      MPI_Group new_group;
      int ranges[1][3];

      res = __INTERNAL__PMPI_Comm_group (comm_old, &comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      /*Retire ranks higher than nb_tasks */

      ranges[0][0] = nb_tasks;
      ranges[0][1] = size;
      ranges[0][2] = 1;

      res =
	__INTERNAL__PMPI_Group_range_excl (comm_group, 1, ranges, &new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      res = __INTERNAL__PMPI_Comm_create (comm_old, new_group, comm_cart);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }


  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  task_specific->type[*comm_cart] = MPI_CART;
  task_specific->data[*comm_cart].cart.ndims = ndims;
  task_specific->data[*comm_cart].cart.reorder = reorder;
  task_specific->data[*comm_cart].cart.dims =
    sctk_malloc (ndims * sizeof (int));
  memcpy (task_specific->data[*comm_cart].cart.dims, dims,
	  ndims * sizeof (int));
  task_specific->data[*comm_cart].cart.periods =
    sctk_malloc (ndims * sizeof (int));
  memcpy (task_specific->data[*comm_cart].cart.periods, periods,
	  ndims * sizeof (int));

  sctk_spinlock_unlock (&(task_specific->lock));
  return res;
}

static int
__INTERNAL__PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
  int i;
  int j;
  int prod = 1;
  int dim_needed = 0;
  int *new_dims;
  int init_nodes;

  init_nodes = nnodes;

  for (i = 0; i < ndims; i++)
    {
      if (dims[i] > 0)
	{
	  prod *= dims[i];
	}
      else
	{
	  dim_needed++;
	}
      if (dims[i] < 0)
	{
	  MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
	}
    }

  if (nnodes % prod != 0)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
    }

  if (dim_needed == 0)
    {
      return MPI_SUCCESS;
    }
  new_dims = malloc (dim_needed * sizeof (int));

  for (j = 0; j < dim_needed; j++)
    {
      new_dims[j] = 1;
    }
  nnodes = nnodes / prod;

  /*Compute decomposition */
#warning "Approximate dims decomposition"
  j = 0;
  for (i = 2; i <= nnodes;)
    {
      if (nnodes % i == 0)
	{
	  new_dims[j] *= i;
	  nnodes = nnodes / i;
	  sctk_nodebug ("Put %d in %d (new %d) remain %d", i, j, new_dims[j],
			nnodes);
	  j = (j + 1) % dim_needed;
	}
      else
	{
	  i++;
	}
    }

  /*Store result */
  j = 0;
  for (i = 0; i < ndims; i++)
    {
      if (dims[i] == 0)
	{
	  dims[i] = new_dims[j];
	  j++;
	}
    }
  free (new_dims);

  {
    int nb_tasks = 1;

    for (i = 0; i < ndims; i++)
      {
	nb_tasks *= dims[i];
      }
    assume (init_nodes == nb_tasks);
  }

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index,
			       int *edges, int reorder, MPI_Comm * comm_graph)
{
  int size;
  int res;
  mpi_topology_t *task_specific;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);

#warning "Very simple approach never reorder nor take care of hardware topology"
  if (nnodes == size)
    {
      res = __INTERNAL__PMPI_Comm_dup (comm_old, comm_graph);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }
  else
    {
      MPI_Group comm_group;
      MPI_Group new_group;
      int ranges[1][3];

      res = __INTERNAL__PMPI_Comm_group (comm_old, &comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      /*Retire ranks higher than nb_tasks */

      ranges[0][0] = nnodes;
      ranges[0][1] = size;
      ranges[0][2] = 1;

      res =
	__INTERNAL__PMPI_Group_range_excl (comm_group, 1, ranges, &new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}

      res = __INTERNAL__PMPI_Comm_create (comm_old, new_group, comm_graph);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&comm_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      res = __INTERNAL__PMPI_Group_free (&new_group);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
    }

  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  task_specific->type[*comm_graph] = MPI_GRAPH;


  task_specific->data[*comm_graph].graph.nnodes = nnodes;
  task_specific->data[*comm_graph].graph.reorder = reorder;
  task_specific->data[*comm_graph].graph.index =
    sctk_malloc (nnodes * sizeof (int));
  memcpy (task_specific->data[*comm_graph].graph.index, index,
	  nnodes * sizeof (int));
  task_specific->data[*comm_graph].graph.edges =
    sctk_malloc (index[nnodes - 1] * sizeof (int));
  memcpy (task_specific->data[*comm_graph].graph.edges, edges,
	  index[nnodes - 1] * sizeof (int));

  task_specific->data[*comm_graph].graph.nedges = index[nnodes - 1];
  task_specific->data[*comm_graph].graph.nindex = nnodes;
  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *nnodes = task_specific->data[comm].graph.nnodes;
  *nedges = task_specific->data[comm].graph.nedges;

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
			    int *index, int *edges)
{
  int i;
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  for (i = 0; (i < maxindex) && (i < task_specific->data[comm].graph.nindex);
       i++)
    {
      index[i] = task_specific->data[comm].graph.index[i];
    }
  for (i = 0; (i < maxedges) && (i < task_specific->data[comm].graph.nedges);
       i++)
    {
      edges[i] = task_specific->data[comm].graph.edges[i];
    }

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
  mpi_topology_t *task_specific;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_CART)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *ndims = task_specific->data[comm].cart.ndims;

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims,
			   int *periods, int *coords)
{
  mpi_topology_t *task_specific;
  int rank;
  int res;

  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));
  __INTERNAL__PMPI_Comm_rank (comm, &rank);

  if (task_specific->type[comm] != MPI_CART)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  if (maxdims != task_specific->data[comm].cart.ndims)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
    }

  memcpy (dims, task_specific->data[comm].cart.dims, maxdims * sizeof (int));
  memcpy (periods,
	  task_specific->data[comm].cart.periods, maxdims * sizeof (int));



  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  sctk_spinlock_unlock (&(task_specific->lock));
  return res;
}

static int
__INTERNAL__PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank, int locked)
{
  mpi_topology_t *task_specific;
  int loc_rank = 0;
  int dims_coef = 1;
  int i;
  PMPC_Get_comm_type ((void *) (&task_specific));
  if (!locked)
    sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_CART)
    {
      if (!locked)
	sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  for (i = 0; i < task_specific->data[comm].cart.ndims - 1; i++)
    {
      dims_coef *= task_specific->data[comm].cart.dims[i];
    }

  for (i = 0; i < task_specific->data[comm].cart.ndims; i++)
    {
      loc_rank += coords[i] * dims_coef;
      sctk_nodebug ("coords[%d] = %d", i, coords[i]);
      dims_coef = dims_coef / task_specific->data[comm].cart.dims[i];
    }

  *rank = loc_rank;
  sctk_nodebug ("rank %d", loc_rank);
  if (!locked)
    sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_coords (MPI_Comm comm, int init_rank, int maxdims,
			      int *coords, int locked)
{
  mpi_topology_t *task_specific;
  int i;
  int dims_coef = 1;
  int rank;

  rank = init_rank;

  PMPC_Get_comm_type ((void *) (&task_specific));
  if (!locked)
    sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_CART)
    {
      if (!locked)
	sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  if (maxdims != task_specific->data[comm].cart.ndims)
    {
      if (!locked)
	sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
    }

  dims_coef = 1;
  for (i = 0; i < maxdims - 1; i++)
    {
      dims_coef *= task_specific->data[comm].cart.dims[i];
    }

  sctk_nodebug ("dims_coef %d", dims_coef);

  assume (dims_coef > 0);
  for (i = 0; i < maxdims; i++)
    {
      coords[i] = rank / dims_coef;
      sctk_nodebug ("rank %d coords %d = %d = (%d / %d)", init_rank, i,
		    coords[i], rank, dims_coef);
      rank = rank % dims_coef;
      dims_coef /= task_specific->data[comm].cart.dims[i];
    }

  if (!locked)
    sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors_count (MPI_Comm comm, int rank,
					int *nneighbors)
{
  mpi_topology_t *task_specific;
  int start;
  int end;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  end = task_specific->data[comm].graph.index[rank];
  start = 0;
  if (rank)
    {
      start = task_specific->data[comm].graph.index[rank - 1];
    }

  *nneighbors = end - start;


  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
				  int *neighbors)
{
  mpi_topology_t *task_specific;
  int start;
  int end;
  int i;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  if (task_specific->type[comm] != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  end = task_specific->data[comm].graph.index[rank];
  start = 0;
  if (rank)
    {
      start = task_specific->data[comm].graph.index[rank - 1];
    }

  for (i = 0;
       i < maxneighbors && i + start < task_specific->data[comm].graph.nedges;
       i++)
    {
      neighbors[i] = task_specific->data[comm].graph.edges[i + start];
    }

  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_shift (MPI_Comm comm, int direction, int displ,
			     int *source, int *dest)
{
  mpi_topology_t *task_specific;
  int *coords;
  int res;
  int rank;
  int maxdims;
  int new_val;
  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));

  __INTERNAL__PMPI_Comm_rank (comm, &rank);

  if (task_specific->type[comm] != MPI_CART)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  if (direction >= task_specific->data[comm].cart.ndims)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid direction");
    }

  maxdims = task_specific->data[comm].cart.ndims;

  coords = sctk_malloc (maxdims * sizeof (int));

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  coords[direction] = coords[direction] + displ;

  sctk_nodebug ("New val + %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (task_specific->data[comm].cart.periods[direction])
	{
	  coords[direction] += task_specific->data[comm].cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   task_specific->data[comm].cart.dims[direction])
    {
      if (task_specific->data[comm].cart.periods[direction])
	{
	  coords[direction] -= task_specific->data[comm].cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else
    {
      res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
    }


  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  *dest = new_val;

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  coords[direction] = coords[direction] - displ;

  sctk_nodebug ("New val - %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (task_specific->data[comm].cart.periods[direction])
	{
	  coords[direction] += task_specific->data[comm].cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   task_specific->data[comm].cart.dims[direction])
    {
      if (task_specific->data[comm].cart.periods[direction])
	{
	  coords[direction] -= task_specific->data[comm].cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else
    {
      res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
    }


  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  *source = new_val;

  sctk_free (coords);

  sctk_nodebug ("dest %d src %d rank %d dir %d", *dest, *source, rank,
		direction);
  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_sub (MPI_Comm comm, int *remain_dims,
			   MPI_Comm * comm_new)
{
  mpi_topology_t *task_specific;
  int res;
  int nb_comm;
  int i;
  int *coords_in_new;
  int color;
  int rank;
  int size;
  int ndims = 0;
  int j;

  PMPC_Get_comm_type ((void *) (&task_specific));
  sctk_spinlock_lock (&(task_specific->lock));
  __INTERNAL__PMPI_Comm_rank (comm, &rank);

  if (task_specific->type[comm] != MPI_CART)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  coords_in_new =
    malloc (task_specific->data[comm].cart.ndims * sizeof (int));

  res =
    __INTERNAL__PMPI_Cart_coords (comm, rank,
				  task_specific->data[comm].cart.ndims,
				  coords_in_new, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  nb_comm = 1;
  for (i = 0; i < task_specific->data[comm].cart.ndims; i++)
    {
      sctk_nodebug ("Remain %d %d", i, remain_dims[i]);
      if (remain_dims[i] == 0)
	{
	  nb_comm *= task_specific->data[comm].cart.dims[i];
	  coords_in_new[i] = 0;
	}
      else
	{
	  ndims++;
	}
    }

  sctk_nodebug ("%d new comms", nb_comm);

  res = __INTERNAL__PMPI_Cart_rank (comm, coords_in_new, &color, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  sctk_nodebug ("%d color", color);
  res = __INTERNAL__PMPI_Comm_split (comm, color, rank, comm_new);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(task_specific->lock));
      return res;
    }

  sctk_free (coords_in_new);

  task_specific->type[*comm_new] = MPI_CART;
  task_specific->data[*comm_new].cart.ndims = ndims;
  task_specific->data[*comm_new].cart.reorder = 0;
  task_specific->data[*comm_new].cart.dims =
    sctk_malloc (ndims * sizeof (int));
  task_specific->data[*comm_new].cart.periods =
    sctk_malloc (ndims * sizeof (int));

  j = 0;
  for (i = 0; i < task_specific->data[comm].cart.ndims; i++)
    {
      if (remain_dims[i])
	{
	  task_specific->data[*comm_new].cart.dims[j] =
	    task_specific->data[comm].cart.dims[i];
	  task_specific->data[*comm_new].cart.periods[j] =
	    task_specific->data[comm].cart.periods[i];
	  j++;
	}
    }


  __INTERNAL__PMPI_Comm_size (*comm_new, &size);
  __INTERNAL__PMPI_Comm_rank (*comm_new, &rank);
  sctk_nodebug ("%d on %d new rank", rank, size);
  sctk_spinlock_unlock (&(task_specific->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_map (MPI_Comm comm, int ndims, int *dims,
			   int *periods, int *newrank)
{
  int res;
  int nnodes = 1;
  int i;

  for (i = 0; i < ndims; i++)
    {
      nnodes *= dims[i];
    }

#warning "Should be optimized"
  res = __INTERNAL__PMPI_Comm_rank (comm, newrank);

  if (*newrank > nnodes)
    {
      *newrank = MPI_UNDEFINED;
    }

  return res;
}

static int
__INTERNAL__PMPI_Graph_map (MPI_Comm comm, int nnodes, int *index,
			    int *edges, int *newrank)
{
  int res;

#warning "Should be optimized"
  res = __INTERNAL__PMPI_Comm_rank (comm, newrank);

  if (*newrank > nnodes)
    {
      *newrank = MPI_UNDEFINED;
    }

  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Get_processor_name (char *name, int *resultlen)
{
  return PMPC_Get_processor_name (name, resultlen);
}

static int
__INTERNAL__PMPI_Get_version (int *version, int *subversion)
{
  return PMPC_Get_version (version, subversion);
}

static int
__INTERNAL__PMPI_Errhandler_create (MPI_Handler_function * function,
				    MPI_Errhandler * errhandler)
{
  mpi_errors_handler_t *task_specific;
  int i;
  int found = 0;
  PMPC_Get_errors ((void **) &task_specific);

  sctk_thread_mutex_lock (&(task_specific->lock));

  for (i = 0; i < task_specific->user_func_nb; i++)
    {
      if (task_specific->user_func[i].status == 0)
	{
	  found = 1;
	  break;
	}
    }

  if (found == 0)
    {
      task_specific->user_func_nb++;
      task_specific->user_func =
	sctk_realloc (task_specific->user_func,
		      task_specific->user_func_nb *
		      sizeof (MPI_Handler_user_function_t));
    }


  task_specific->user_func[i].func = function;
  task_specific->user_func[i].status = 1;
  *errhandler = i;

  sctk_thread_mutex_unlock (&(task_specific->lock));

  MPI_ERROR_SUCESS ();
}

void
MPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file,
		   int line)
{
  char str[1024];
  int i;
  __INTERNAL__PMPI_Error_string (*error, str, &i);
  if (i != 0)
    sctk_error ("%s in file %s at line %d %s", str, file, line, msg);
  else
    sctk_error ("Unknown error");
  __INTERNAL__PMPI_Abort (*comm, *error);
}

void
MPI_Return_error (MPI_Comm * comm, int *error, ...)
{
}

static int
__INTERNAL__PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  mpi_errors_handler_t *task_specific;
  PMPC_Get_errors ((void **) &task_specific);

  sctk_thread_mutex_lock (&(task_specific->lock));
  if ((errhandler < 0) || (errhandler >= task_specific->user_func_nb))
    {
      sctk_thread_mutex_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }


  if (task_specific->user_func[errhandler].status == 0)
    {
      sctk_thread_mutex_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

  task_specific->func[comm] = task_specific->user_func[errhandler].func;
  task_specific->func_ident[comm] = errhandler;

  sctk_thread_mutex_unlock (&(task_specific->lock));

  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  mpi_errors_handler_t *task_specific;
  PMPC_Get_errors ((void **) &task_specific);

  sctk_thread_mutex_lock (&(task_specific->lock));
  *errhandler = task_specific->func_ident[comm];
/*   task_specific->user_func[*errhandler].status++;  */
  sctk_thread_mutex_unlock (&(task_specific->lock));

  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  mpi_errors_handler_t *task_specific;
  PMPC_Get_errors ((void **) &task_specific);

  sctk_thread_mutex_lock (&(task_specific->lock));

  if ((*errhandler < 0) || (*errhandler >= task_specific->user_func_nb))
    {
      sctk_thread_mutex_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

  if (task_specific->user_func[*errhandler].status == 0)
    {
      sctk_thread_mutex_unlock (&(task_specific->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

/*   task_specific->user_func[*errhandler].status--;  */

/*   if(task_specific->user_func[*errhandler].status <= 0){ */
/*     task_specific->user_func[*errhandler].status = 0; */
/*     task_specific->user_func[*errhandler].func = NULL; */
/*   } */

  sctk_thread_mutex_unlock (&(task_specific->lock));

  *errhandler = MPI_ERRHANDLER_NULL;
  MPI_ERROR_SUCESS ();
}

#define MPI_Error_string_convert(code,msg) \
  case code: sprintf(string,msg); break

static int
__INTERNAL__PMPI_Error_string (int errorcode, char *string, int *resultlen)
{
  size_t lngt;
  string[0] = '\0';
  switch (errorcode)
    {
      MPI_Error_string_convert (MPI_ERR_BUFFER, "Invalid buffer pointer");
      MPI_Error_string_convert (MPI_ERR_COUNT, "Invalid count argument");
      MPI_Error_string_convert (MPI_ERR_TYPE, "Invalid datatype argument");
      MPI_Error_string_convert (MPI_ERR_TAG, "Invalid tag argument");
      MPI_Error_string_convert (MPI_ERR_COMM, "Invalid communicator");
      MPI_Error_string_convert (MPI_ERR_RANK, "Invalid rank");
      MPI_Error_string_convert (MPI_ERR_ROOT, "Invalid root");
      MPI_Error_string_convert (MPI_ERR_TRUNCATE,
				"Message truncated on receive");
      MPI_Error_string_convert (MPI_ERR_GROUP, "Invalid group");
      MPI_Error_string_convert (MPI_ERR_OP, "Invalid operation");
      MPI_Error_string_convert (MPI_ERR_REQUEST,
				"Invalid mpi_request handle");
      MPI_Error_string_convert (MPI_ERR_TOPOLOGY, "Invalid topology");
      MPI_Error_string_convert (MPI_ERR_DIMS, "Invalid dimension argument");
      MPI_Error_string_convert (MPI_ERR_ARG, "Invalid argument");
      MPI_Error_string_convert (MPI_ERR_OTHER,
				"Other error; use Error_string");
      MPI_Error_string_convert (MPI_ERR_UNKNOWN, "Unknown error");
      MPI_Error_string_convert (MPI_ERR_INTERN, "Internal error code");
      MPI_Error_string_convert (MPI_ERR_IN_STATUS,
				"Look in status for error value");
      MPI_Error_string_convert (MPI_ERR_PENDING, "Pending request");
      MPI_Error_string_convert (MPI_NOT_IMPLEMENTED, "Not implemented");
    default:
      sctk_warning ("%d error code unknown", errorcode);
    }
  lngt = strlen (string);
  *resultlen = (int) lngt;
  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Error_class (int errorcode, int *errorclass)
{
  *errorclass = errorcode;
  MPI_ERROR_SUCESS ();
}

static double
__INTERNAL__PMPI_Wtime (void)
{
  return PMPC_Wtime ();
}

static double
__INTERNAL__PMPI_Wtick (void)
{
  return PMPC_Wtick ();
}

static int
__INTERNAL__PMPI_Init_thread (int *argc, char ***argv, int required,
			      int *provided)
{
  int res;
  int max_thread_type = MPI_THREAD_MULTIPLE;
  res = __INTERNAL__PMPI_Init (argc, argv);
  if (required < max_thread_type)
    {
      *provided = required;
    }
  else
    {
      *provided = max_thread_type;
    }
  return res;
}

static int
__INTERNAL__PMPI_Init (int *argc, char ***argv)
{
  int res;
  int rank;
  res = PMPC_Init (argc, argv);
  if (res != MPI_SUCCESS)
    {
      return res;
    }
  __sctk_init_mpc_request ();
  __sctk_init_mpi_buffer ();
  __sctk_init_mpi_errors ();
  __sctk_init_mpi_topo ();
  __sctk_init_mpi_op ();
  __sctk_init_mpc_group ();

  __INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &rank);
  __INTERNAL__PMPI_Barrier (MPI_COMM_WORLD);
  return res;
}

static int
__INTERNAL__PMPI_Finalize (void)
{
  return PMPC_Finalize ();
}

static int
__INTERNAL__PMPI_Initialized (int *flag)
{
  return PMPC_Initialized (flag);
}

static int
__INTERNAL__PMPI_Abort (MPI_Comm comm, int val)
{
  return PMPC_Abort (comm, val);
}

/*
  PMPI FUNCTIONS
*/

#define SCTK__MPI_Check_retrun_val(res,comm)do{if(res == MPI_SUCCESS){return res;} else {MPI_ERROR_REPORT(comm,res,"Generic error retrun");}}while(0)
/* #define SCTK__MPI_Check_retrun_val(res) return res */

static inline void
SCTK__MPI_INIT_STATUS (MPI_Status * status)
{
}

static inline void
SCTK__MPI_INIT_REQUEST (MPI_Request * request)
{
  *request = MPI_REQUEST_NULL;
}

int
PMPI_Send (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	   MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag,
	   MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_STATUS (status);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (source, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Recv (buf, count, datatype, source, tag, comm, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  mpi_check_type (datatype, MPI_COMM_WORLD);

  if (status == NULL)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_IN_STATUS, "");
    }

  if (count == NULL)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COUNT, "");
    }

  res = __INTERNAL__PMPI_Get_count (status, datatype, count);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Bsend (buf, count, datatype, dest, tag, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Ssend (buf, count, datatype, dest, tag, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Rsend (buf, count, datatype, dest, tag, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Buffer_attach (void *buffer, int size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_attach (buffer, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Buffer_detach (void *buffer, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_detach (buffer, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  res =
    __INTERNAL__PMPI_Isend (buf, count, datatype, dest, tag, comm, request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Ibsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Issend (buf, count, datatype, dest, tag, comm, request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Irsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irsend (buf, count, datatype, dest, tag, comm, request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
	    int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (source, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, tag, comm, request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Wait (request, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Test (request, flag, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Request_free (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Request_free (request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Waitany (int count, MPI_Request array_of_requests[], int *index,
	      MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Waitany (count, array_of_requests, index, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Testany (int count, MPI_Request array_of_requests[], int *index,
	      int *flag, MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testany (count, array_of_requests, index, flag, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Waitall (int count, MPI_Request array_of_requests[],
	      MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Waitall (count, array_of_requests, array_of_statuses);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Testall (int count, MPI_Request array_of_requests[], int *flag,
	      MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testall (count, array_of_requests, flag,
			      array_of_statuses);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Waitsome (int incount, MPI_Request array_of_requests[],
	       int *outcount, int array_of_indices[],
	       MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Waitsome (incount, array_of_requests, outcount,
			       array_of_indices, array_of_statuses);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Testsome (int incount, MPI_Request array_of_requests[], int *outcount,
	       int array_of_indices[], MPI_Status array_of_statuses[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testsome (incount, array_of_requests, outcount,
			       array_of_indices, array_of_statuses);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
	     MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Iprobe (source, tag, comm, flag, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Probe (source, tag, comm, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cancel (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Cancel (request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Test_cancelled (MPI_Status * status, int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Test_cancelled (status, flag);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Send_init (void *buf, int count, MPI_Datatype datatype, int dest,
		int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Send_init (buf, count, datatype, dest, tag, comm,
				request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Bsend_init (void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Bsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest,
		 int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ssend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Rsend_init (void *buf, int count, MPI_Datatype datatype, int dest,
		 int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Rsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source,
		int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    mpi_check_rank (source, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Recv_init (buf, count, datatype, source, tag, comm,
				request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Start (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Start (request);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Startall (int count, MPI_Request array_of_requests[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Startall (count, array_of_requests);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Sendrecv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       int dest, int sendtag,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       int source, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  mpi_check_type (dest, comm);
  mpi_check_type (source, comm);
  mpi_check_type (sendtype, comm);
  mpi_check_type (recvtype, comm);
  mpi_check_count (sendcount, comm);
  mpi_check_count (recvcount, comm);
  mpi_check_tag (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  if (sendcount != 0)
    {
      mpi_check_buf (sendbuf, comm);
    }
  if (recvcount != 0)
    {
      mpi_check_buf (recvbuf, comm);
    }
  res =
    __INTERNAL__PMPI_Sendrecv (sendbuf, sendcount, sendtype, dest, sendtag,
			       recvbuf, recvcount, recvtype, source, recvtag,
			       comm, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Sendrecv_replace (void *buf, int count, MPI_Datatype datatype,
		       int dest, int sendtag, int source, int recvtag,
		       MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  mpi_check_count (count, comm);
  mpi_check_tag (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  mpi_check_type (dest, comm);
  mpi_check_type (source, comm);
  if (count != 0)
    {
      mpi_check_buf (buf, comm);
    }
  res =
    __INTERNAL__PMPI_Sendrecv_replace (buf, count, datatype, dest, sendtag,
				       source, recvtag, comm, status);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_contiguous (int count,
		      MPI_Datatype old_type, MPI_Datatype * new_type_p)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_contiguous (count, old_type, new_type_p);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_vector (int count,
		  int blocklength,
		  int stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Type_vector (count, blocklength, stride, old_type,
				  newtype_p);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_hvector (int count,
		   int blocklen,
		   MPI_Aint stride,
		   MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Type_hvector (count, blocklen, stride, old_type,
				   newtype_p);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_indexed (int count,
		   int blocklens[],
		   int indices[],
		   MPI_Datatype old_type, MPI_Datatype * newtype)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Type_indexed (count, blocklens, indices, old_type,
				   newtype);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_hindexed (int count,
		    int blocklens[],
		    MPI_Aint indices[],
		    MPI_Datatype old_type, MPI_Datatype * newtype)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, old_type,
				    newtype);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_struct (int count,
		  int blocklens[],
		  MPI_Aint indices[],
		  MPI_Datatype old_types[], MPI_Datatype * newtype)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Type_struct (count, blocklens, indices, old_types,
				  newtype);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Address (void *location, MPI_Aint * address)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Address (location, address);
  SCTK__MPI_Check_retrun_val (res, comm);
}

  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
int
PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_extent (datatype, extent);
  SCTK__MPI_Check_retrun_val (res, comm);
}

  /* See the 1.1 version of the Standard.  The standard made an 
     unfortunate choice here, however, it is the standard.  The size returned 
     by MPI_Type_size is specified as an int, not an MPI_Aint */
int
PMPI_Type_size (MPI_Datatype datatype, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_size (datatype, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

  /* MPI_Type_count was withdrawn in MPI 1.1 */
int
PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_lb (datatype, displacement);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_ub (MPI_Datatype datatype, MPI_Aint * displacement)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_ub (datatype, displacement);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_commit (MPI_Datatype * datatype)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Type_commit (datatype);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Type_free (MPI_Datatype * datatype)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  mpi_check_type (*datatype, MPI_COMM_WORLD);
  res = __INTERNAL__PMPI_Type_free (datatype);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Get_elements (MPI_Status * status, MPI_Datatype datatype, int *elements)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_elements (status, datatype, elements);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Pack (void *inbuf,
	   int incount,
	   MPI_Datatype datatype,
	   void *outbuf, int outcount, int *position, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Pack (inbuf, incount, datatype, outbuf, outcount,
			   position, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Unpack (void *inbuf,
	     int insize,
	     int *position,
	     void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Unpack (inbuf, insize, position, outbuf, outcount,
			     datatype, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Pack_size (int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Pack_size (incount, datatype, comm, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Barrier (MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  sctk_nodebug ("Entering BARRIER %d", comm);
  res = __INTERNAL__PMPI_Barrier (comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  sctk_nodebug ("Entering BCAST %d", comm);
  res = __INTERNAL__PMPI_Bcast (buffer, count, datatype, root, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	     void *recvbuf, int recvcnt, MPI_Datatype recvtype,
	     int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Gather (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			     recvtype, root, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int *recvcnts, int *displs,
	      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Gatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
			      displs, recvtype, root, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
	      MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Scatter (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			      recvtype, root, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Scatterv (void *sendbuf, int *sendcnts, int *displs,
	       MPI_Datatype sendtype, void *recvbuf, int recvcnt,
	       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Scatterv (sendbuf, sendcnts, displs, sendtype, recvbuf,
			       recvcnt, recvtype, root, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Allgather (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		void *recvbuf, int recvcount, MPI_Datatype recvtype,
		MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Allgather (sendbuf, sendcount, sendtype, recvbuf,
				recvcount, recvtype, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Allgatherv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		 void *recvbuf, int *recvcounts, int *displs,
		 MPI_Datatype recvtype, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Allgatherv (sendbuf, sendcount, sendtype, recvbuf,
				 recvcounts, displs, recvtype, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Alltoall (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Alltoall (sendbuf, sendcount, sendtype, recvbuf,
			       recvcount, recvtype, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls,
		MPI_Datatype sendtype, void *recvbuf, int *recvcnts,
		int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Alltoallv (sendbuf, sendcnts, sdispls, sendtype, recvbuf,
				recvcnts, rdispls, recvtype, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	     MPI_Op op, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root,
			     comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Op_create (MPI_User_function * function, int commute, MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Op_create (function, commute, op);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Op_free (MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Op_free (op);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  mpi_check_buf (sendbuf, comm);
  mpi_check_buf (recvbuf, comm);
  mpi_check_count (count, comm);
  mpi_check_type (datatype, comm);
  sctk_nodebug ("Entering ALLREDUCE %d", comm);
  res =
    __INTERNAL__PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Reduce_scatter (sendbuf, recvbuf, recvcnts, datatype, op,
				     comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Scan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	   MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_size (MPI_Group group, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_size (group, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_rank (MPI_Group group, int *rank)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_rank (group, rank);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1,
			    MPI_Group group2, int *ranks2)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Group_translate_ranks (group1, n, ranks1, group2,
					    ranks2);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_compare (MPI_Group group1, MPI_Group group2, int *result)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_compare (group1, group2, result);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_group (MPI_Comm comm, MPI_Group * group)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Comm_group (comm, group);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_union (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_union (group1, group2, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_intersection (MPI_Group group1, MPI_Group group2,
			 MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_intersection (group1, group2, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_difference (MPI_Group group1, MPI_Group group2,
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_difference (group1, group2, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_incl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_incl (group, n, ranks, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_excl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_excl (group, n, ranks, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_range_incl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_range_incl (group, n, ranges, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_range_excl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_range_excl (group, n, ranges, newgroup);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Group_free (MPI_Group * group)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Group_free (group);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_size (MPI_Comm comm, int *size)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_size (comm, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_rank (MPI_Comm comm, int *rank)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_rank (comm, rank);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm1, comm1);
  mpi_check_comm (comm2, comm2);
  res = __INTERNAL__PMPI_Comm_compare (comm1, comm2, result);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_dup (MPI_Comm comm, MPI_Comm * newcomm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_dup (comm, newcomm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_create (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  if (group == MPI_GROUP_NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
    }
  res = __INTERNAL__PMPI_Comm_create (comm, group, newcomm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_split (MPI_Comm comm, int color, int key, MPI_Comm * newcomm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_split (comm, color, key, newcomm);
  sctk_nodebug ("SPLIT Com %d Color %d, key %d newcomm %d", comm, color, key,
		*newcomm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_free (MPI_Comm * comm)
{
  int res = MPI_ERR_INTERN;
  if (comm == NULL)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }
  mpi_check_comm (*comm, *comm);
  res = __INTERNAL__PMPI_Comm_free (comm);
  SCTK__MPI_Check_retrun_val (res, *comm);
}

int
PMPI_Comm_test_inter (MPI_Comm comm, int *flag)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_test_inter (comm, flag);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_remote_size (MPI_Comm comm, int *size)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_remote_size (comm, size);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_remote_group (MPI_Comm comm, MPI_Group * group)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_remote_group (comm, group);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Intercomm_create (MPI_Comm local_comm, int local_leader,
		       MPI_Comm peer_comm, int remote_leader, int tag,
		       MPI_Comm * newintercomm)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Intercomm_create (local_comm, local_leader, peer_comm,
				       remote_leader, tag, newintercomm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Intercomm_merge (MPI_Comm intercomm, int high, MPI_Comm * newintracomm)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Intercomm_merge (intercomm, high, newintracomm);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Keyval_create (MPI_Copy_function * copy_fn,
		    MPI_Delete_function * delete_fn,
		    int *keyval, void *extra_state)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Keyval_create (copy_fn, delete_fn, keyval, extra_state);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Keyval_free (int *keyval)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Keyval_free (keyval);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_put (comm, keyval, attr_value);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_get (comm, keyval, attr_value, flag);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_delete (comm, keyval);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Topo_test (comm, topo_type);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims, int *periods,
		  int reorder, MPI_Comm * comm_cart)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  res =
    __INTERNAL__PMPI_Cart_create (comm_old, ndims, dims, periods, reorder,
				  comm_cart);
  SCTK__MPI_Check_retrun_val (res, comm_old);
}

int
PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Dims_create (nnodes, ndims, dims);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		   int reorder, MPI_Comm * comm_graph)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  res =
    __INTERNAL__PMPI_Graph_create (comm_old, nnodes, index, edges, reorder,
				   comm_graph);
  SCTK__MPI_Check_retrun_val (res, comm_old);
}

int
PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graphdims_get (comm, nnodes, nedges);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
		int *index, int *edges)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_get (comm, maxindex, maxedges, index, edges);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cartdim_get (comm, ndims);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods,
	       int *coords)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_get (comm, maxdims, dims, periods, coords);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_rank (comm, coords, rank, 0);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_coords (MPI_Comm comm, int rank, int maxdims, int *coords)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 0);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Graph_neighbors_count (MPI_Comm comm, int rank, int *nneighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_neighbors_count (comm, rank, nneighbors);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
		      int *neighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res =
    __INTERNAL__PMPI_Graph_neighbors (comm, rank, maxneighbors, neighbors);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_shift (MPI_Comm comm, int direction, int displ, int *source,
		 int *dest)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_shift (comm, direction, displ, source, dest);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_sub (MPI_Comm comm, int *remain_dims, MPI_Comm * comm_new)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_sub (comm, remain_dims, comm_new);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Cart_map (MPI_Comm comm_old, int ndims, int *dims, int *periods,
	       int *newrank)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  res = __INTERNAL__PMPI_Cart_map (comm_old, ndims, dims, periods, newrank);
  SCTK__MPI_Check_retrun_val (res, comm_old);
}

int
PMPI_Graph_map (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		int *newrank)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  res = __INTERNAL__PMPI_Graph_map (comm_old, nnodes, index, edges, newrank);
  SCTK__MPI_Check_retrun_val (res, comm_old);
}

int
PMPI_Get_processor_name (char *name, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_processor_name (name, resultlen);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Get_version (int *version, int *subversion)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_version (version, subversion);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Errhandler_create (MPI_Handler_function * function,
			MPI_Errhandler * errhandler)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_create (function, errhandler);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_set (comm, errhandler);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_get (comm, errhandler);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_free (errhandler);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Error_string (int errorcode, char *string, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_string (errorcode, string, resultlen);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Error_class (int errorcode, int *errorclass)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_class (errorcode, errorclass);
  SCTK__MPI_Check_retrun_val (res, comm);
}

double
PMPI_Wtime (void)
{
  double res = 0;
  res = __INTERNAL__PMPI_Wtime ();
  return res;
}

double
PMPI_Wtick (void)
{
  double res = 0;
  res = __INTERNAL__PMPI_Wtick ();
  return res;
}

int
PMPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init_thread (argc, argv, required, provided);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Init (int *argc, char ***argv)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init (argc, argv);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Finalize (void)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Finalize ();
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Initialized (int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Initialized (flag);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Abort (MPI_Comm comm, int errorcode)
{
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Abort (comm, errorcode);
  SCTK__MPI_Check_retrun_val (res, comm);
}


  /* Note that we may need to define a @PCONTROL_LIST@ depending on whether 
     stdargs are supported */
int
PMPI_Pcontrol (const int level, ...)
{
  return MPI_SUCCESS;
}


int
PMPI_Comm_get_name (MPI_Comm comm, char *comm_name, int *resultlen)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_get_name (comm, comm_name, resultlen);
  SCTK__MPI_Check_retrun_val (res, comm);
}

int
PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_set_name (comm, comm_name);
  SCTK__MPI_Check_retrun_val (res, comm);
}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#warning "Default mpc_user_main__ has been removed because of TLS compilation..."
#if 0
int mpc_user_main__ (int argc, char **argv);
int
__sctk__mpc_user_main__ (int argc, char **argv)
{
#if defined (MPC_OpenMP)
  sctk_warning
    ("You have to include mpi.h, omp.h or mpc.h in the file containing main function");
#else
  sctk_warning
    ("You have to include mpi.h or mpc.h in the file containing main function");
#endif
  return 0;
}

#pragma weak mpc_user_main__ = __sctk__mpc_user_main__
#endif

#endif
