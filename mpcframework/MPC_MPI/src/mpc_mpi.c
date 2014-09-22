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
/* #                                                                      # */
/* ######################################################################## */
#include <mpc.h>
#include <mpc_mpi.h>
#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_thread.h"
#include "sctk_communicator.h"
#include "sctk_collective_communications.h"
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
#include <uthash.h>
#include "mpi_array_types.h"

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_mpi_weak.h"
#endif

TODO("Optimize algorithme for derived types")

/* #define ENABLE_COLLECTIVES_ON_INTERCOMM  */

static int __INTERNAL__PMPI_Attr_set_fortran (int keyval);

static char *
sctk_char_fortran_to_c (char *buf, long int size)
{
  char *tmp;
  long int i;
  tmp = sctk_malloc (size + 1);
TODO("check memory liberation")

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
static int __INTERNAL__PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type);
static int __INTERNAL__PMPI_Type_commit (MPI_Datatype *);
static int __INTERNAL__PMPI_Type_free (MPI_Datatype *);
static int __INTERNAL__PMPI_Get_elements_x (MPI_Status *, MPI_Datatype, MPI_Count *);
int __INTERNAL__PMPI_Type_create_darray (int size,
					 int rank,
					 int ndims,
					 int array_of_gsizes[],
					 int array_of_distribs[],
					 int array_of_dargs[],
					 int array_of_psizes[],
					 int order,
					 MPI_Datatype oldtype,
					 MPI_Datatype *newtype);
int __INTERNAL__PMPI_Type_create_subarray (int ndims,
					   int array_of_sizes[],
					   int array_of_subsizes[],
					   int array_of_starts[],
					   int order,
					   MPI_Datatype oldtype,
					   MPI_Datatype * new_type);
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
				       
/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgather_graph(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(void *, int , MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(void *, int , MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoall_cart(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoall_graph(void *, int , MPI_Datatype , void *, int , MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(void *, int [], int [], MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(void *, int [], int [], MPI_Datatype , void *, int [], int [], MPI_Datatype , MPI_Comm );
static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(void *, int [], MPI_Aint [], MPI_Datatype [], void *, int [], MPI_Aint [], MPI_Datatype [], MPI_Comm comm);
static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(void *, int [], MPI_Aint [], MPI_Datatype [], void *, int [], MPI_Aint [], MPI_Datatype [], MPI_Comm comm);
  
static int __INTERNAL__PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op,
				    int, MPI_Comm);
static int __INTERNAL__PMPI_Op_create (MPI_User_function *, int, MPI_Op *);
static int __INTERNAL__PMPI_Op_free (MPI_Op *);
static int __INTERNAL__PMPI_Allreduce (void *, void *, int, MPI_Datatype,
				       MPI_Op, MPI_Comm);
static int __INTERNAL__PMPI_Reduce_scatter (void *, void *, int *,
					    MPI_Datatype, MPI_Op, MPI_Comm);
static int __INTERNAL__PMPI_Reduce_scatter_block (void *, void *, int,
					    MPI_Datatype, MPI_Op, MPI_Comm);
static int __INTERNAL__PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op,
				  MPI_Comm);
static int __INTERNAL__PMPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op,
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
static int __INTERNAL__PMPI_Comm_create_from_intercomm (MPI_Comm, MPI_Group, MPI_Comm *);
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
static int __INTERNAL__PMPI_Finalized (int*);
static int __INTERNAL__PMPI_Initialized (int *);
static int __INTERNAL__PMPI_Abort (MPI_Comm, int);
static int __INTERNAL__PMPI_Isend_test_req (void *buf, int count,
    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request);

  /* MPI-2 functions */
static int __INTERNAL__PMPI_Comm_get_name (MPI_Comm, char *, int *);
static int __INTERNAL__PMPI_Comm_set_name (MPI_Comm, char *);
static int __INTERNAL__PMPI_Init_thread (int *, char ***, int, int *);
static int __INTERNAL__PMPI_Query_thread (int *);


typedef struct
{
  void *attr;
  int flag;
} MPI_Caching_key_value_t;

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
  int type;
  mpi_topology_info_t data;
  char names[MPI_MAX_NAME_STRING + 1];
  sctk_spinlock_t lock;
} mpi_topology_per_comm_t;

#define MPC_MPI_MAX_NUMBER_FUNC 3

typedef struct mpc_mpi_per_communicator_s{
  /****** ERRORS ******/
  MPI_Handler_function *func;
  int func_ident;

  /****** Attributes ******/
  MPI_Caching_key_value_t* key_vals;
  int max_number;

  /****** Topologies ******/
  mpi_topology_per_comm_t topo;

  /****** LOCK ******/
  sctk_spinlock_t lock;
}mpc_mpi_per_communicator_t;

#define MPC_MPI_MAX_NUMBER_FUNC 3

typedef struct
{
  MPI_Copy_function *copy_fn;
  MPI_Delete_function *delete_fn;
  void *extra_state;
  int used;
  int fortran_key;
} MPI_Caching_key_t;

typedef struct
{
  MPI_Handler_function *func;
  int status;
} MPI_Handler_user_function_t;

struct MPI_request_struct_s;
struct MPI_group_struct_s;
typedef struct MPI_group_struct_s MPI_group_struct_t;
struct MPI_buffer_struct_s;
typedef struct MPI_buffer_struct_s MPI_buffer_struct_t;
struct sctk_mpi_ops_s;
typedef struct sctk_mpi_ops_s sctk_mpi_ops_t;

typedef struct mpc_mpi_data_s{
  /****** ERRORS ******/
  MPI_Handler_user_function_t* user_func;
  int user_func_nb;

  /****** Attributes ******/
  MPI_Caching_key_t *attrs_fn;
  int number;
  int max_number;

  /****** REQUESTS ******/
  struct MPI_request_struct_s *requests;

  /****** GROUPS ******/
  struct MPI_group_struct_s *groups;

  /****** BUFFERS ******/
  struct MPI_buffer_struct_s *buffers;

  /****** OP ******/
  struct sctk_mpi_ops_s *ops;

  /****** LOCK ******/
  sctk_spinlock_t lock;
}mpc_mpi_data_t;

static
void mpc_mpi_per_communicator_copy_func(mpc_mpi_per_communicator_t** to, mpc_mpi_per_communicator_t* from)
{
	int i = 0;
	sctk_spinlock_lock (&(from->lock));
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
	memcpy(*to,from,sizeof(mpc_mpi_per_communicator_t));
	((*to)->key_vals) = sctk_malloc(from->max_number*sizeof(MPI_Caching_key_value_t));
	for(i = 0 ; i < from->max_number ; i++)
	{
		((*to)->key_vals[i].attr) = NULL;
		((*to)->key_vals[i].flag) = 0;
	}
	sctk_spinlock_unlock (&(from->lock));
	sctk_spinlock_unlock (&((*to)->lock));
}

static
void mpc_mpi_per_communicator_dup_copy_func(mpc_mpi_per_communicator_t** to, mpc_mpi_per_communicator_t* from)
{
	sctk_spinlock_lock (&(from->lock));
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
	memcpy(*to,from,sizeof(mpc_mpi_per_communicator_t));
	sctk_spinlock_unlock (&(from->lock));
	sctk_spinlock_unlock (&((*to)->lock));
}

static inline
mpc_mpi_per_communicator_t* mpc_mpc_get_per_comm_data(sctk_communicator_t comm){
  struct sctk_task_specific_s * task_specific;
  mpc_per_communicator_t* tmp;

  task_specific = __MPC_get_task_specific ();
  tmp = sctk_thread_getspecific_mpc_per_comm(task_specific,comm);
  assume(tmp != NULL);
  return tmp->mpc_mpi_per_communicator;
}

static inline mpc_mpi_data_t * mpc_mpc_get_per_task_data()
{
	struct sctk_task_specific_s * task_specific;
	task_specific = __MPC_get_task_specific ();
	return task_specific->mpc_mpi_data;
}

static inline void PMPC_Get_requests(struct MPI_request_struct_s **requests){
  *requests = mpc_mpc_get_per_task_data()->requests;
}

static inline void PMPC_Set_requests(struct MPI_request_struct_s *requests){
  mpc_mpc_get_per_task_data()->requests = requests;
}

static inline void PMPC_Get_groups(struct MPI_group_struct_s **groups){
  *groups = mpc_mpc_get_per_task_data()->groups;
}

static inline void PMPC_Set_groups(struct MPI_group_struct_s *groups){
  mpc_mpc_get_per_task_data()->groups = groups;
}

static inline void PMPC_Get_buffers(struct MPI_buffer_struct_s **buffers){
  *buffers = mpc_mpc_get_per_task_data()->buffers;
}

static inline void PMPC_Set_buffers(struct MPI_buffer_struct_s *buffers){
  mpc_mpc_get_per_task_data()->buffers = buffers;
}

static inline void PMPC_Get_op(struct sctk_mpi_ops_s **ops){
  *ops = mpc_mpc_get_per_task_data()->ops;
}

static inline void PMPC_Set_op(struct sctk_mpi_ops_s *ops){
  mpc_mpc_get_per_task_data()->ops = ops;
}

/*
  ERRORS HANDLING
*/
static inline int
SCTK__MPI_ERROR_REPORT__ (MPC_Comm comm, int error, char *message, char *file,
			  int line)
{
  MPI_Comm comm_id;
  int error_id;
  MPI_Handler_function *func;
  mpc_mpi_per_communicator_t* tmp;

  tmp = mpc_mpc_get_per_comm_data(comm);

  if(tmp == NULL){
    comm = MPC_COMM_WORLD;
    tmp = mpc_mpc_get_per_comm_data(comm);
  }

  comm_id = comm;

  error_id = error;
  func = tmp->func;
  func (&comm_id, &error_id, message, file, line);
  return error;
}

static void
__sctk_init_mpi_errors_per_comm (mpc_mpi_per_communicator_t* tmp)
{
  tmp->func = (MPI_Handler_function *)MPI_Default_error;
  tmp->func_ident = MPI_ERRORS_ARE_FATAL;
}

static void
__sctk_init_mpi_errors ()
{
  mpc_mpi_data_t* data;

  data = mpc_mpc_get_per_task_data();

  data->user_func = sctk_malloc(MPC_MPI_MAX_NUMBER_FUNC*sizeof(MPI_Handler_user_function_t));
  data->user_func_nb = MPC_MPI_MAX_NUMBER_FUNC;

  data->user_func[MPI_ERRHANDLER_NULL].func =
    (MPI_Handler_function *) MPI_Default_error;
  data->user_func[MPI_ERRHANDLER_NULL].status = 1;
  data->user_func[MPI_ERRORS_RETURN].func =
    (MPI_Handler_function *) MPI_Return_error;
  data->user_func[MPI_ERRORS_RETURN].status = 1;
  data->user_func[MPI_ERRORS_ARE_FATAL].func =
    (MPI_Handler_function *) MPI_Abort_error;
  data->user_func[MPI_ERRORS_ARE_FATAL].status = 1;
}

#define MPI_ERROR_REPORT(comm, error,message) return SCTK__MPI_ERROR_REPORT__(comm, error,message,__FILE__, __LINE__)

#define MPI_ERROR_SUCESS() return MPI_SUCCESS

#define mpi_check_type(datatype,comm)		\
  if ( ((datatype >= SCTK_USER_DATA_TYPES_MAX) && (sctk_datatype_is_derived(datatype) != 1)) || (datatype == MPI_DATATYPE_NULL) ) \
    MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "Bad datatype provided");

#define mpi_check_type_create(datatype,comm)		\
  if ((datatype >= SCTK_USER_DATA_TYPES_MAX) && (sctk_datatype_is_derived(datatype) != 1) && ((datatype != MPI_UB) && (datatype != MPI_LB))) \
    MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "");

static int is_finalized = 0;
static int is_initialized = 0;

TODO("to optimize")
#define mpi_check_comm(com,comm)			\
  if((is_finalized != 0) || (is_initialized != 1)) MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_OTHER,""); \
  else if (com == MPI_COMM_NULL)				\
    MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_COMM,"Error in communicator");		\
  else if(mpc_mpc_get_per_comm_data(com) == NULL)	\
    MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"Error in communicator")

#define mpi_check_status(status,comm)		\
  if(status == MPI_STATUS_IGNORE)	\
    MPI_ERROR_REPORT(comm,MPI_ERR_IN_STATUS,"Error status is MPI_STATUS_IGNORE")

#define mpi_check_buf(buf,comm)					\
  if(buf == NULL)					\
    MPI_ERROR_REPORT(comm,MPI_ERR_BUFFER,"A NULL message buffer was provided"); \
  if( buf == MPI_BOTTOM ) \
      buf = NULL; /* Here we remove 1 from buff to restore pointer artihmetic */

#define mpi_check_count(count,comm)				\
  if(count < 0)							\
    MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"Error count cannot be negative")

#define mpi_check_rank(task,max_rank,comm)		\
  if((((task < 0) || (task >= max_rank)) && (sctk_is_inter_comm (comm) == 0)) && (task != MPI_ANY_SOURCE) && (task != MPI_PROC_NULL)) \
    MPI_ERROR_REPORT(comm,MPI_ERR_RANK,"Error bad rank provided")

#define mpi_check_rank_send(task,max_rank,comm)		\
  if((((task < 0) || (task >= max_rank)) && (sctk_is_inter_comm (comm) == 0)) && (task != MPI_PROC_NULL)) \
    MPI_ERROR_REPORT(comm,MPI_ERR_RANK,"Error bad rank provided")

#define mpi_check_root(task,max_rank,comm)		\
  if(((task < 0) || (task >= max_rank)) && (task != MPI_PROC_NULL))		\
    MPI_ERROR_REPORT(comm,MPI_ERR_ROOT,"Error bad root rank provided")

#define mpi_check_tag(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)) && (tag != MPI_ANY_TAG))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"Error bad tag provided")

#define mpi_check_tag_send(tag,comm)				\
  if(((tag < 0) || (tag > MPI_TAG_UB_VALUE)))	\
    MPI_ERROR_REPORT(comm,MPI_ERR_TAG,"Error bad tag provided")

#define mpi_check_op_type_func(t) case t: return mpi_check_op_type_func_##t(datatype)
#define mpi_check_op_type_func_notavail(t) case t: return 1 

#if 0
static int mpi_check_op_type_func_MPI_(MPI_Datatype datatype){
        switch(datatype){
		default:return 0;
        }
        return 0;
}
#endif
#define mpi_check_op_type_func_integer()\
	mpi_check_op_type_func_notavail(MPC_INT);\
        mpi_check_op_type_func_notavail(MPC_LONG)

#define mpi_check_op_type_func_float()\
        mpi_check_op_type_func_notavail(MPC_FLOAT);\
	mpi_check_op_type_func_notavail(MPC_DOUBLE)


static int mpi_check_op_type_func_MPI_SUM(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default: return 0;
        }
        return 0;
}

static int mpi_check_op_type_func_MPI_MAX(MPI_Datatype datatype){
	switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
	}
	return 0;
}
static int mpi_check_op_type_func_MPI_MIN(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_PROD(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_notavail(MPI_BYTE);	
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LAND(MPI_Datatype datatype){
        switch(datatype){
		mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BAND(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_LXOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_BXOR(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_MINLOC(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_integer();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}
static int mpi_check_op_type_func_MPI_MAXLOC(MPI_Datatype datatype){
        switch(datatype){
                mpi_check_op_type_func_float();
		mpi_check_op_type_func_integer();
		mpi_check_op_type_func_notavail(MPI_BYTE);
                default:return 0;
        }
        return 0;
}

static int mpi_check_op_type(MPI_Op op, MPI_Datatype datatype){
#if 1
	if((op <= MPI_MAXLOC) && ( sctk_datatype_is_common( datatype) )){
		switch(op){
			mpi_check_op_type_func(MPI_SUM);
			mpi_check_op_type_func(MPI_MAX);
			mpi_check_op_type_func(MPI_MIN);
			mpi_check_op_type_func(MPI_PROD);
			mpi_check_op_type_func(MPI_LAND);
			mpi_check_op_type_func(MPI_BAND);
			mpi_check_op_type_func(MPI_LOR);
			mpi_check_op_type_func(MPI_BOR);
			mpi_check_op_type_func(MPI_LXOR);
			mpi_check_op_type_func(MPI_BXOR);
			mpi_check_op_type_func(MPI_MINLOC);
			mpi_check_op_type_func(MPI_MAXLOC);		
		}
		return 0;
	}
#endif
	return 0;
}

#define mpi_check_op(op,type,comm)				\
  if((op == MPI_OP_NULL) || mpi_check_op_type(op,type))							\
    MPI_ERROR_REPORT(comm,MPI_ERR_OP,"")

static int SCTK__MPI_Attr_clean_communicator (MPI_Comm comm);
static int SCTK__MPI_Attr_communicator_dup (MPI_Comm old, MPI_Comm new);

/* const MPI_Comm MPI_COMM_SELF = MPC_COMM_SELF; */

/************************************************************************/
/* Request Handling                                                    */
/************************************************************************/

typedef enum
{ 
	Send_init,
	Bsend_init,
	Rsend_init,
	Ssend_init,
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
	MPC_Request req;	/**< Request to be stored */
	int used; 	/**< Is the request slot in use */
	volatile struct MPI_internal_request_s *next;
	int rank; 	/**< Offset in the tab array from  struct \ref MPI_request_struct_s */

	/* Persitant */
	MPI_Persistant_t persistant;
	int freeable;
	int is_active;

	/*Datatypes */
	void *saved_datatype;

	/*Req_free*/
	int auto_free;
} MPI_internal_request_t;

/** \brief MPI_Request managment structure 
 * 
 * 	In order to simplify the fortran interface it is preferable
 *  to store requests as integers. However we need to be able
 *  retrieve data associated with it (the \ref MPC_Request)
 *  by preserving a mapping between the interger id (MPI_Request)
 *  and the actual request (MPC_Request). Moreover,
 *  this structure tries to recycle requests in order to avoid
 *  reallocating them each time.
 * 
 *  To do so \ref MPI_internal_request_s are in a chained list,
 *  allowing their storage in both free_list and auto_free_list
 * 
 * */
 typedef struct MPI_request_struct_s
{
	sctk_spinlock_t lock; /**< Lock protecting the data-structure */
	/* Number of resquests */
	int max_size; /**< Current size of the tab array (starts at 0 and is increased 10 by 10) */
	MPI_internal_request_t **tab; /**< This array stores the \ref MPI_internal_request_t which are the containers for MPC_Requests */
	volatile MPI_internal_request_t *free_list; /**< In this array requests are ready to be used, when created requests go to this array */
	volatile MPI_internal_request_t *auto_free_list; /**< This list contains request which are automatically freed */
	sctk_alloc_buffer_t buf; /**< This is a buffer allocator used to allocate requests */
}MPI_request_struct_t;

/** \brief Initialize MPI interface request handling */
static inline void __sctk_init_mpc_request ()
{
	static sctk_thread_mutex_t sctk_request_lock =	SCTK_THREAD_MUTEX_INITIALIZER;
	MPI_request_struct_t *requests;
	
	/* Check wether requests are already initalized */
	PMPC_Get_requests ((void *) &requests);
	assume (requests == NULL);

	sctk_thread_mutex_lock (&sctk_request_lock);
	
	/* Allocate the request structure */
	requests = sctk_malloc (sizeof (MPI_request_struct_t));

	/*Init request struct */
	requests->lock = 0;
	requests->tab = NULL;
	requests->free_list = NULL;
	requests->auto_free_list = NULL;
	requests->max_size = 0;
	/* Create the associated buffered allocator */
	sctk_buffered_alloc_create (&(requests->buf), sizeof (MPI_internal_request_t));

	/* Register the new array in the task specific data-structure */
	PMPC_Set_requests (requests);
	
	sctk_thread_mutex_unlock (&sctk_request_lock);
}

/** \brief Delete an internal request and put it in the free-list */
static inline void sctk_delete_internal_request(MPI_internal_request_t *tmp, MPI_request_struct_t *requests )
{
	/* Release the internal request */
	tmp->used = 0;
	sctk_free (tmp->saved_datatype);
	tmp->saved_datatype = NULL;
	
	/* Push in in the head of the free list */
	tmp->next = requests->free_list;
	requests->free_list = tmp;
}

/** \brief Walk the auto-free list in search for a terminated call (only head) 
 * 
 *  \param requests a pointer to the request array structure 
 * */
static inline void sctk_check_auto_free_list(MPI_request_struct_t *requests)
{
	/* If there is an auto free list */
	if(requests->auto_free_list != NULL)
	{
		MPI_internal_request_t *tmp;
		int flag;
		int res;

		/* Take HEAD */
		tmp = (MPI_internal_request_t *)requests->auto_free_list;
		
		/* Test it */
		res = PMPC_Test (&(tmp->req), &flag, MPC_STATUS_IGNORE);
		
		/* If call has ended */
		if (flag != 0)
		{
			MPI_internal_request_t *tmp_new;

			/* Remove HEAD from the auto-free list */
			tmp_new = (MPI_internal_request_t *)tmp->next;
			requests->auto_free_list = tmp_new;
			
			/* Free the request and put it in the free list */
			sctk_delete_internal_request(tmp,requests);
		}
	}
}

/** \brief Initialize an \ref MPI_internal_request_t */
static inline void __sctk_init_mpc_request_internal(MPI_internal_request_t *tmp){
	memset (&(tmp->req), 0, sizeof (MPC_Request));
}

/** \brief Create a new \ref MPI_internal_request_t 
 * 
 *  \param req Request to allocate (will be written with the ID of the request)
 * */
static inline MPI_internal_request_t * __sctk_new_mpc_request_internal (MPI_Request * req)
{
	MPI_request_struct_t *requests;
	MPI_internal_request_t *tmp;

	/* Retrieve the request strunt from the env */
	PMPC_Get_requests ((void *) &requests);

	/* Lock the request struct */
	sctk_spinlock_lock (&(requests->lock));
	
	/* Try to free the HEAD of the auto free list */
	sctk_check_auto_free_list(requests);
	
	/* If the free list is empty */
	if (requests->free_list == NULL)
	{
		int old_size;
		int i;
		
		/* Previous size */
		old_size = requests->max_size;
		/* New incremented size */
		requests->max_size += 10;
		/* Allocate TAB */
		requests->tab = sctk_realloc (requests->tab,  requests->max_size * sizeof (MPI_internal_request_t *));
		
		assume (requests->tab != NULL);
		
		/* Fill in the new array slots */
		for (i = old_size; i < requests->max_size; i++)
		{
			MPI_internal_request_t *tmp;
			sctk_nodebug ("%lu", i);
			/* Allocate the MPI_internal_request_t */
			tmp = sctk_buffered_malloc (&(requests->buf),  sizeof (MPI_internal_request_t));
			assume (tmp != NULL);
			
			/* Save it in the array */
			requests->tab[i] = tmp;
			
			/* Set not used */
			tmp->used = 0;
			
			/* Put the newly allocated slot in the free list */
			tmp->next = requests->free_list;
			requests->free_list = tmp;
			
			/* Register its offset in the tab array */
			tmp->rank = i;
			tmp->saved_datatype = NULL;
		}
	}
	
	/* Now we should have a request in the free list */
	
	/* Take head from the free list */
	tmp = (MPI_internal_request_t *) requests->free_list;
	
	/* Mark it as used */
	tmp->used = 1;
	/* Mark it as freable */
	tmp->freeable = 1;
	/* Mark it as active */
	tmp->is_active = 1;
	/* Disable auto-free */
	tmp->auto_free = 0;
	tmp->saved_datatype = NULL;
	
	/* Remove from free list */
	requests->free_list = tmp->next;
	
	sctk_spinlock_unlock (&(requests->lock));
	
	/* Set request to be the id in the tab array (rank) */
	*req = tmp->rank;

	/* Intialize request content */
	__sctk_init_mpc_request_internal(tmp);

	return tmp;
}

/** \brief Create a new \ref MPC_Request */
static inline MPC_Request * __sctk_new_mpc_request (MPI_Request * req)
{
	MPI_internal_request_t *tmp;
	/* Acquire a free MPI_Iternal request */
	tmp = __sctk_new_mpc_request_internal (req);
	/* Return its inner MPC_Request */
	return &(tmp->req);
}

/** \brief Convert an \ref MPI_Request to an \ref MPI_internal_request_t
 * 
 * 	\param req Request to convert to an \ref MPI_internal_request_t
 *  \return a pointer to the \MPI_internal_request_t associated with req NULL if not found
 *  */
static inline MPI_internal_request_t * __sctk_convert_mpc_request_internal (MPI_Request * req)
{
	MPI_internal_request_t *tmp;
	MPI_request_struct_t *requests;
	
	/* Retrieve the interger ID of this request */
	int	int_req = *req;

	/* If it is request NULL we have nothing to get */
	if (int_req == MPI_REQUEST_NULL)
	{
		return NULL;
	}

	/* Retrieve the request array */
	PMPC_Get_requests ((void *) &requests);
	assume (requests != NULL);
	
	/* Lock it */
	sctk_spinlock_lock (&(requests->lock));
	
	sctk_nodebug ("Convert request %d", *req);
	/* Check bounds */
	assume (((int_req) >= 0) && ((int_req) < requests->max_size));
	
	/* Directly get the request in the tab */
	tmp = requests->tab[int_req];
	/* Is rank coherent with the offset */
	assume (tmp->rank == *req);
	/* Is this request in used */
	assume (tmp->used);
	
	/* Unlock the request array */
	sctk_spinlock_unlock (&(requests->lock));

	/* Return the MPI_internal_request_t * */
	return tmp;
}

/** \brief Convert an MPI_Request to an MPC_Request
 * \param req Request to convert to an \ref MPC_Request
 * \return a pointer to the MPC_Request NULL if not found
 */
static inline MPC_Request * __sctk_convert_mpc_request (MPI_Request * req)
{
	MPI_internal_request_t *tmp;

	/* Resolve the MPI_internal_request_t */
	tmp = __sctk_convert_mpc_request_internal (req);
	
	/* If there was no MPI_internal_request_t or it was unactive */
	if ((tmp == NULL) || (tmp->is_active == 0))
	{
		/* Return the null request */
		return &MPC_REQUEST_NULL;
	}
	
	/* Return the MPC_Request field */
	return &(tmp->req);
}

/** Brief save a data-type in a request 
 * 
 *  \param req Target request
 *  \param t Datatype to store
 * 
 * */
static inline void __sctk_add_in_mpc_request (MPI_Request * req, void *t)
{
	MPI_internal_request_t *tmp;
	tmp = __sctk_convert_mpc_request_internal (req);
	tmp->saved_datatype = t;
}

/** Delete a request
 *  \param req Request to delete
 */
static inline void __sctk_delete_mpc_request (MPI_Request * req)
{
	MPI_request_struct_t *requests;
	MPI_internal_request_t *tmp;
	
	/* Convert resquest to an integer */
	int int_req = *req;
	
	/* If it is request null there is nothing to do */
	if (int_req == MPI_REQUEST_NULL)
	{
		return;
	}

	/* Retrieve the request array */
	PMPC_Get_requests ((void *) &requests);
	assume (requests != NULL);
	/* Lock it */
	sctk_nodebug ("Delete request %d", *req);
	sctk_spinlock_lock (&(requests->lock));
	
	/* Check request id bounds */
	assume (((*req) >= 0) && ((*req) < requests->max_size));
	
	/* Retrieve the request */
	tmp = requests->tab[*req];

	/* if request is not active disable auto-free */
	if(tmp->is_active == 0)
	{
		tmp->auto_free = 0;
	}

	/* Deactivate the request */
	tmp->is_active = 0;

	/* Can the request be freed ? */
	if (tmp->freeable == 1)
	{
		/* Make sure the rank matches the TAB offset */
		assume (tmp->rank == *req);
		
		/* Auto free ? */
		if(tmp->auto_free == 0)
		{
			/* NO */
			/* Call delete internal request to push it in the free list */
			sctk_delete_internal_request(tmp,requests);
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		} else {
			/* Remove it from the free list */
			tmp->next = requests->auto_free_list;
			requests->auto_free_list = tmp;
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		}
	}
	
	/* Unlock the request array */
	sctk_spinlock_unlock (&(requests->lock));
}





static int
__INTERNAL__PMPI_Send (void *buf, int count, MPI_Datatype datatype, int dest,
		       int tag, MPI_Comm comm)
{
	sctk_nodebug ("SEND buf %p type %d", buf, datatype);
	if (sctk_datatype_is_derived (datatype) && (count != 0))
	{
		int res;

		sctk_nodebug ("Send derived type");

		if (count > 1)
		{
			sctk_nodebug("count > 1");
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
			MPC_Request request;
			MPC_Status status;

			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);

			if (res != MPI_SUCCESS)
			{
			return res;
			}
			res = PMPC_Open_pack (&request);
			if (res != MPI_SUCCESS)
			{
			return res;
			}

			res = PMPC_Add_pack_absolute (buf, derived_datatype.count, derived_datatype.begins, derived_datatype.ends, MPC_CHAR, &request);
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
		
		if(count == 0)
		{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
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
	if (sctk_datatype_is_derived (datatype))
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
			MPC_Request request;

			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);
			if (res != MPI_SUCCESS)
			{
				return res;
			}
			res = PMPC_Open_pack (&request);
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			res = PMPC_Add_pack_absolute (buf, derived_datatype.count, derived_datatype.begins, derived_datatype.ends, MPC_CHAR,	&request);
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


static int __INTERNAL__PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
{
	return  PMPC_Get_count(status, datatype, count);
}

static int
__INTERNAL__PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  MPI_Request request;
  int res;
  res = __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, &request);
  if(res != MPI_SUCCESS){
    return res;
  }
  res = __INTERNAL__PMPI_Wait(&request, MPI_STATUS_IGNORE);
  return res;
}


static int
__INTERNAL__PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}

      return PMPC_Ssend (buf, count, datatype, dest, tag, comm);
    }
}


static int
__INTERNAL__PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest,
			int tag, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
    }
  else
    {
      if (buf == NULL && count != 0)
	{
	  MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "");
	}
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}
      return PMPC_Rsend (buf, count, datatype, dest, tag, comm);
    }
}

/*
  BUFFERS
*/
typedef struct MPI_buffer_struct_s
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
/* 	fprintf(stderr,"Min %p max %p\n",tmp->buffer , ((char*)tmp->buffer) + tmp->size); */

	while (head != NULL)
	{
/* 	  fprintf(stderr,"%p %d\n",head,head->request); */
		if (head->request != MPI_REQUEST_NULL)
		{
			__INTERNAL__PMPI_Test (&(head->request), &flag, MPI_STATUS_IGNORE);
			assume((head->request == MPI_REQUEST_NULL) || (flag == 0));
		}
		if (head->request == MPI_REQUEST_NULL)
		{
			sctk_nodebug ("1 : %p freed", head);
			head_next = SCTK__buffer_next_header (head, tmp);

			/*Compact from head */
			while (head_next != NULL)
			{
/* 	  fprintf(stderr,"NEXT %p %d\n",head_next,head_next->request); */
				if (head_next->request != MPI_REQUEST_NULL)
				{
					__INTERNAL__PMPI_Test (&(head_next->request), &flag, MPI_STATUS_IGNORE);
					assume((head->request == MPI_REQUEST_NULL) || (flag == 0));
				}
				if (head_next->request == MPI_REQUEST_NULL)
				{
					sctk_nodebug ("2 : %p freed", head_next);
					head->size = head->size + head_next->size + sizeof (mpi_buffer_overhead_t);
					sctk_nodebug("MERGE Create new buffer of size %d (%d + %d)", head->size, head_next->size, head->size - head_next->size - sizeof (mpi_buffer_overhead_t));
				}
				else
				{
					break;
				}

				head_next = SCTK__buffer_next_header (head, tmp);
			}
		}

		if ((head->request == MPI_REQUEST_NULL) && (head->size >= size) /* && (found == NULL) */)
		{
		  if(found != NULL){
		    if(head->size < found->size){
		      found = head;
		    }
		  } else {
		    found = head;
		  }
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
  PMPC_Get_buffers (&tmp);

  sctk_spinlock_lock (&(tmp->lock));
  if((tmp->buffer != NULL) || (count < 0)){
    sctk_spinlock_unlock (&(tmp->lock));
    return MPI_ERR_BUFFER;
  }
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
  PMPC_Get_buffers (&tmp);
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
	if (pending != 0) sctk_thread_yield ();
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
__INTERNAL__PMPI_Ibsend_test_req (void *buf, int count, MPI_Datatype datatype,
				  int dest, int tag, MPI_Comm comm,
				  MPI_Request * request, int is_valid_request)
{
  mpi_buffer_t *tmp;
  int size;
  int res;
  MPI_request_struct_t *requests;
  mpi_buffer_overhead_t *head;
  mpi_buffer_overhead_t *head_next;
  void *head_buf;
  mpi_buffer_overhead_t *found = NULL;
  //~ get the pack size
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

  PMPC_Get_buffers (&tmp);
  sctk_spinlock_lock (&(tmp->lock));

  if (tmp->buffer == NULL)
    {
      sctk_spinlock_unlock (&(tmp->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_BUFFER, "No buffer available");
    }

  found = SCTK__MPI_Compact_buffer (size, tmp);
  sctk_nodebug("found = %d", found);
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

      res = __INTERNAL__PMPI_Isend_test_req (head_buf, position, MPI_PACKED, dest, tag, comm, &(head->request), 0);

/*       fprintf(stderr,"Add request %d %d\n",head->request,res); */

      if (res != MPI_SUCCESS)
	{
	  sctk_spinlock_unlock (&(tmp->lock));
	  return res;
	}

      if(is_valid_request == 1){
        MPI_internal_request_t* tmp_request;

        tmp_request = (MPI_internal_request_t *)__sctk_convert_mpc_request (request);
        tmp_request->req.completion_flag = SCTK_MESSAGE_DONE;

      } else {
//	*request = MPI_REQUEST_NULL;
	MPI_internal_request_t* tmp_request;
	__sctk_new_mpc_request (request);
	tmp_request = (MPI_internal_request_t *)__sctk_convert_mpc_request (request);
	tmp_request->req.completion_flag = SCTK_MESSAGE_DONE;
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
	if (sctk_datatype_is_derived (datatype) && (count != 0))
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
			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);
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
				tmp = sctk_malloc (derived_datatype.opt_count * 2 * sizeof (mpc_pack_absolute_indexes_t));
				__sctk_add_in_mpc_request (request, tmp);
				
				memcpy (tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				memcpy (&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				
				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends = &(tmp[derived_datatype.opt_count]);
			}

			res = PMPC_Add_pack_absolute (buf, derived_datatype.opt_count, derived_datatype.opt_begins, derived_datatype.opt_ends, MPC_CHAR,	__sctk_convert_mpc_request (request));
		
			if (res != MPI_SUCCESS)
			{
				return res;
			}

			res = PMPC_Isend_pack (dest, tag, comm, __sctk_convert_mpc_request (request));
			return res;
		}
	}
	else
	{
		if(count == 0)
		{
			/* code to avoid derived datatype */
			datatype = MPC_CHAR;
		}
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
  if (sctk_datatype_is_derived (datatype) && (count != 0))
    {
      return __INTERNAL__PMPI_Isend_test_req (buf, count, datatype, dest, tag,
					      comm, request,
					      is_valid_request);
    }
  else
    {
	if(count == 0)
	{
		/* code to avoid derived datatype */
		datatype = MPC_CHAR;
	}
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
  if (sctk_datatype_is_derived (datatype))
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

static int __INTERNAL__PMPI_Irsend (void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request)
{
  return __INTERNAL__PMPI_Irsend_test_req (buf, count, datatype, dest, tag, comm, request, 0);
}


static int __INTERNAL__PMPI_Irecv_test_req (void *buf, int count, MPI_Datatype datatype,
				 int source, int tag, MPI_Comm comm,
				 MPI_Request * request, int is_valid_request)
{
	if (sctk_datatype_is_derived (datatype))
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
			int derived_ret = 0;
			sctk_derived_datatype_t derived_datatype;

			res = MPC_Is_derived_datatype (datatype, &derived_ret, &derived_datatype);
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
				tmp = sctk_malloc (derived_datatype.opt_count * 2 * sizeof (mpc_pack_absolute_indexes_t));
				__sctk_add_in_mpc_request (request, tmp);

				memcpy (tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));
				memcpy (&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof (mpc_pack_absolute_indexes_t));

				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends = &(tmp[derived_datatype.opt_count]);
			}

			res =
			PMPC_Add_pack_absolute (buf, derived_datatype.opt_count, derived_datatype.opt_begins, derived_datatype.opt_ends, MPC_CHAR, __sctk_convert_mpc_request (request));
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

static int __INTERNAL__PMPI_Irecv (void *buf, int count, MPI_Datatype datatype,
			int source, int tag, MPI_Comm comm,
			MPI_Request * request)
{
  return __INTERNAL__PMPI_Irecv_test_req (buf, count, datatype, source, tag,
					  comm, request, 0);
}

static int __INTERNAL__PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
	int res;
	sctk_nodebug("wait request %d", *request);
	res = PMPC_Wait (__sctk_convert_mpc_request (request), status);

	__sctk_delete_mpc_request (request);
	return res;
}

static int __INTERNAL__PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
	int res;
	MPI_internal_request_t *tmp;
	tmp = __sctk_convert_mpc_request_internal(request);
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

static int __INTERNAL__PMPI_Request_free (MPI_Request * request)
{
	int res = MPI_SUCCESS;
	MPI_internal_request_t *tmp;
	tmp = __sctk_convert_mpc_request_internal (request);

	if (tmp)
    {
		tmp->freeable = 1;
		tmp->auto_free = 1;
		__sctk_delete_mpc_request (request);
		*request = MPI_REQUEST_NULL;
    }
	return res;
}

static int __INTERNAL__PMPI_Waitany (int count, MPI_Request * array_of_requests, int *index, MPI_Status * status)
{
	int flag = 0;

	while (!flag)
    {
		__INTERNAL__PMPI_Testany (count, array_of_requests, index, &flag, status);
      /* if (!flag) sctk_thread_yield (); */
    }
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Testany (int count, MPI_Request * array_of_requests, int *index, int *flag, MPI_Status * status)
{
  int i;
  *index = MPI_UNDEFINED;
  *flag = 1;
  int tmp;

  for (i = 0; i < count; i++)
    {
      if (array_of_requests[i] == MPI_REQUEST_NULL)
	{
	  continue;
	}

      {
	MPC_Request *req;
	req = __sctk_convert_mpc_request (&(array_of_requests[i]));
	if (req == &MPC_REQUEST_NULL)
	  {
	    continue;
	  }
	else
	  {
	    int flag_test = 0;
	    tmp = PMPC_Test (req, &flag_test, status);
	    if(flag_test == 0)
	      *flag = 0;
	    else
	      *flag = 1;
	  }
      }

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
  sctk_thread_yield();
  return MPI_SUCCESS;
}

#define PMPI_WAIT_ALL_STATIC_TRSH 32

static int __INTERNAL__PMPI_Waitall (int count, MPI_Request * array_of_requests, MPI_Status * array_of_statuses)
{
	int flag = 0;
	int i;

	/* Set the MPI_Status to MPI_SUCCESS */
	if(array_of_statuses != MPC_STATUSES_IGNORE){
	  for (i = 0; i < count; i++)
	    {
	      array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
	    }
	}
	
	/* Convert MPI resquests to MPC ones */
	
	/* Prepare an array for MPC requests */
	MPC_Request ** mpc_array_of_requests;
	MPC_Request * static_array_of_requests[PMPI_WAIT_ALL_STATIC_TRSH];
	
	if( count < PMPI_WAIT_ALL_STATIC_TRSH )
	{
		mpc_array_of_requests = static_array_of_requests;
	}
	else
	{
		mpc_array_of_requests = sctk_malloc(sizeof(MPC_Request *) * count);
		assume( mpc_array_of_requests != NULL );
	}
	
	/* Fill the array with those requests */
	for( i = 0 ; i < count ; i++ )
	{
		/* Handle NULL requests */
		if( array_of_requests[i] == MPI_REQUEST_NULL )
		{
			mpc_array_of_requests[i] = &MPC_REQUEST_NULL;
		}
		else
		{
			mpc_array_of_requests[i] = __sctk_convert_mpc_request (&(array_of_requests[i]));
		}

	}
	
	/* Call the MPC waitall implementation */
	int ret = __MPC_Waitallp (count,  mpc_array_of_requests, (MPC_Status *)array_of_statuses);
	
	/* Something bad hapenned ? */
	if( ret != MPI_SUCCESS )
		return ret;
	 
	/* Delete the MPI requests */
	for( i = 0 ; i < count ; i++ )
		__sctk_delete_mpc_request (&(array_of_requests[i]));
	
	/* If needed free the mpc_array_of_requests */
	if( PMPI_WAIT_ALL_STATIC_TRSH <= count )
	{
		sctk_free(mpc_array_of_requests);
	}
	
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Testall (int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
	int i;
	int done = 0;
	int loc_flag;
	*flag = 0;
	if(array_of_statuses != MPC_STATUSES_IGNORE){
	  for (i = 0; i < count; i++)
	    {
	      array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
	    }
	}

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
				tmp = PMPC_Test (req, &loc_flag, (array_of_statuses == MPC_STATUSES_IGNORE) ? MPC_STATUS_IGNORE:&(array_of_statuses[i]));
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
 	if(*flag == 0){
	  sctk_thread_yield();
	}
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Waitsome (int incount, MPI_Request * array_of_requests, int *outcount, int *array_of_indices, MPI_Status * array_of_statuses)
{
  int i;
  int req_null_count = 0;

  *outcount = MPI_UNDEFINED;

  for(i = 0; i < incount; i++){
    if(array_of_requests[i] == MPI_REQUEST_NULL){
      req_null_count++;
    } else {
      MPC_Request *req;
      req = __sctk_convert_mpc_request (&(array_of_requests[i]));
      if (req == &MPC_REQUEST_NULL)
	{
	  req_null_count++;
	}
    }
  }

  if(req_null_count == incount){
    return MPI_SUCCESS;
  }

  do
    {
      //      sctk_thread_yield ();
      __INTERNAL__PMPI_Testsome (incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    }while (*outcount == MPI_UNDEFINED);

  return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Testsome (int incount, MPI_Request * array_of_requests, int *outcount, int *array_of_indices, MPI_Status * array_of_statuses)
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
				tmp = PMPC_Test (req, &loc_flag,(array_of_statuses == MPI_STATUSES_IGNORE) ? MPC_STATUS_IGNORE:&(array_of_statuses[done]));
			}
			if (loc_flag)
			{
				array_of_indices[done] = i;
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
		  sctk_thread_yield();
	} else {
		sctk_thread_yield();
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
			 MPI_Status * status)
{
  int res;
  res = PMPC_Iprobe (source, tag, comm, flag, status);
  if (!(*flag)) sctk_thread_yield ();
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

static int __INTERNAL__PMPI_Test_cancelled (MPI_Status * status, int *flag)
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
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Send_init;

	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Send_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Bsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	int rank;
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request);
	sctk_nodebug("new request %d", *request);
	if(dest == MPC_PROC_NULL)
	{
	  	req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Bsend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Bsend_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Ssend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request);
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Ssend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Ssend_init;
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Rsend_init (void *buf, int count, MPI_Datatype datatype,
			     int dest, int tag, MPI_Comm comm,
			     MPI_Request * request)
{
	MPI_internal_request_t *req;
	req = __sctk_new_mpc_request_internal (request);
	if(dest == MPC_PROC_NULL)
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_NULL;

		req->persistant.buf = buf;
		req->persistant.count = 0;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag = MPI_ANY_TAG;
		req->persistant.comm = comm;
		req->persistant.op = Rsend_init;
	}
	else
	{
		req->freeable = 0;
		req->is_active = 0;
		req->req.request_type = REQUEST_SEND;

		req->persistant.buf = buf;
		req->persistant.count = count;
		req->persistant.datatype = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag = tag;
		req->persistant.comm = comm;
		req->persistant.op = Rsend_init;
	}
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
  req->req.request_type = REQUEST_RECV;

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
  if(req->is_active != 0)
	return MPI_ERR_REQUEST;
  req->is_active = 1;

  if(req->req.request_type == REQUEST_NULL)
  {
    *request = MPI_REQUEST_NULL;
    return MPI_SUCCESS;
  }

/*   __sctk_init_mpc_request_internal(req); */

  switch (req->persistant.op)
    {
    case Send_init:
/*       fprintf(stderr,"SEND to %d tag %d req %d\n",req->persistant.dest_source,req->persistant.tag,*request); */
      res =
	__INTERNAL__PMPI_Isend_test_req (req->persistant.buf,
					 req->persistant.count,
					 req->persistant.datatype,
					 req->persistant.dest_source,
					 req->persistant.tag,
					 req->persistant.comm, request, 1);
      break;
    case Bsend_init:
/*       fprintf(stderr,"SEND to %d tag %d req %d\n",req->persistant.dest_source,req->persistant.tag,*request); */
      res =
	__INTERNAL__PMPI_Ibsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Rsend_init:
/*       fprintf(stderr,"SEND to %d tag %d req %d\n",req->persistant.dest_source,req->persistant.tag,*request); */
      res =
	__INTERNAL__PMPI_Irsend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Ssend_init:
/*       fprintf(stderr,"SEND to %d tag %d req %d\n",req->persistant.dest_source,req->persistant.tag,*request); */
      res =
	__INTERNAL__PMPI_Issend_test_req (req->persistant.buf,
					  req->persistant.count,
					  req->persistant.datatype,
					  req->persistant.dest_source,
					  req->persistant.tag,
					  req->persistant.comm, request, 1);
      break;
    case Recv_init:
/*       fprintf(stderr,"RECV from %d tag %d req %d\n",req->persistant.dest_source,req->persistant.tag,*request); */
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
  sctk_nodebug ("__INTERNAL__PMPI_Sendrecv TYPE %d %d done", sendtype, recvtype);
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
  sctk_nodebug("receive type = %d", datatype);
  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, recvtag, comm,
			    &recvreq);
  if (res != MPI_SUCCESS)
    {
      sctk_free (tmp);
      return res;
    }
  sctk_nodebug("Send type = %d", MPI_PACKED);
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

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/

int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function * free_fn,
					  MPI_Grequest_cancel_function * cancel_fn, void *extra_state, MPI_Request * request)
{
	MPC_Request *new_request = __sctk_new_mpc_request (request);
	
	return PMPC_Grequest_start( query_fn, free_fn, cancel_fn, extra_state, new_request );
}


int PMPI_Grequest_complete(  MPI_Request request )
{
	MPC_Request *mpc_req = __sctk_convert_mpc_request (&request);
	return PMPC_Grequest_complete( *mpc_req );
}

/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

  int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
                          MPI_Grequest_free_function * free_fn,
           		  MPI_Grequest_cancel_function * cancel_fn, 
           		  MPIX_Grequest_poll_fn * poll_fn, 
           		  void *extra_state, 
           		  MPI_Request * request)
  {
	  MPC_Request *new_request = __sctk_new_mpc_request (request);
	  return PMPCX_Grequest_start(query_fn, free_fn, cancel_fn, poll_fn, extra_state, new_request);
  }

/************************************************************************/
/* Extended Generalized Requests Class                                  */
/************************************************************************/

int PMPIX_GRequest_class_create( MPI_Grequest_query_function * query_fn,
				 MPI_Grequest_cancel_function * cancel_fn,
				 MPI_Grequest_free_function * free_fn,
				 MPIX_Grequest_poll_fn * poll_fn,
				 MPIX_Grequest_wait_fn * wait_fn,
				 MPIX_Request_class * new_class )
{
	return PMPCX_GRequest_class_create(query_fn, cancel_fn, free_fn, poll_fn, wait_fn, new_class );
}
  
int PMPIX_Grequest_class_allocate( MPIX_Request_class target_class, void *extra_state, MPI_Request *request )
{
	MPC_Request *new_request = __sctk_new_mpc_request (request);
	return PMPCX_Grequest_class_allocate( target_class, extra_state, new_request );
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/** \brief This function creates a contiguous MPI_Datatype
 *  
 *  Such datatype is obtained by directly appending  count copies of data_in type
 *  
 *  If the source datatype is A[ ]B and count is 3 we obtain A[ ]BA[ ]BA[ ]B
 *  
 *  \param count Number of elements of type data_in in the data_out type
 *  \param data_in The type to be replicated count times
 *  \param data_out The output type
 * 
 */
static int __INTERNAL__PMPI_Type_contiguous (int count, MPI_Datatype data_in,  MPI_Datatype * data_out)
{
	/* If source datatype is a derived datatype we have to create a new derived datatype */
	if (sctk_datatype_is_derived (data_in))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (data_in, &derived_ret, &input_datatype);

		/* Compute the total datatype size including boundaries */
		unsigned long extent;
		__INTERNAL__PMPI_Type_extent (data_in, (MPI_Aint *) & extent);

		/* By definition the number of output types is count times the one of the input type */
		unsigned long count_out = input_datatype.count * count;
		
		/* Allocate datatype description */
		mpc_pack_absolute_indexes_t *begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t *ends_out =	sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t * datatypes = 	sctk_malloc (count_out * sizeof (sctk_datatype_t));

		if( !begins_out || !ends_out || !datatypes )
		{
			MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_INTERN, "Failled to allocate type context");
		}

		/* Fill in datatype description for a contiguous type */
		unsigned long i;

		/* Input : ABC of length count_in
		 * Output : ABCABCABC... of length count_out
		 */

		for (i = 0; i < count_out; i++)
		{
	
			begins_out[i] = input_datatype.begins[i % input_datatype.count]    /* Original begin offset in the input block */
							+ extent * (i / input_datatype.count); /* New offset due to type replication */
							
			ends_out[i] = input_datatype.ends[i % input_datatype.count]		   /* Original end offset in the input block */
							+ extent * (i / input_datatype.count); /* New offset due to type replication */
			
			datatypes[i] = input_datatype.datatypes[i % input_datatype.count];
			
			sctk_nodebug ("%d , %lu-%lu <- %lu-%lu", i, begins_out[i],
					ends_out[i], input_datatype.begins[i % input_datatype.count],
					input_datatype.ends[i % input_datatype.count]);
		}

		/* Update new type upperbound */
		int new_ub = input_datatype.ub + extent * (count - 1);

		/* Actually create the new datatype */
		PMPC_Derived_datatype (data_out, begins_out, ends_out, datatypes, count_out, input_datatype.lb,	input_datatype.is_lb, new_ub, input_datatype.is_ub);

		/* Set its context */
		struct Datatype_External_context dtctx;
		sctk_datatype_external_context_clear( &dtctx );
		dtctx.combiner = MPI_COMBINER_CONTIGUOUS;
		dtctx.count = count;
		dtctx.oldtype = data_in;
		MPC_Datatype_set_context( *data_out, &dtctx);
		
		/* Free temporary buffers */
		sctk_free (datatypes);
		sctk_free (begins_out);
		sctk_free (ends_out);
		

		return MPI_SUCCESS;
	}
	else
	{
		/* Here we handle contiguous or common datatypes which can be replicated directly */
		return PMPC_Type_hcontiguous(data_out, count, &data_in);
	}
}

/** \brief This function creates a vector datatype
 * 	\param count Number of items in the vector
 * 	\param blocklen Lenght of a block in terms of old_type
 * 	\param stride Size of gaps in terms of old_type
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_vector (int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	
	/* Compute the stride in bytes */
	unsigned long stride_t = stride * extent ;
	
	/* Call the hvector function */
	int res =  __INTERNAL__PMPI_Type_hvector( count, blocklen,  stride_t, old_type,  newtype_p);
	
	/* Set its context to overide the one from hvector */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_VECTOR;
	dtctx.count = count;
	dtctx.blocklength = blocklen;
	dtctx.stride = stride;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype_p, &dtctx);
	
	return res;
}

/** \brief This function creates a vector datatype with a stride in bytes
 * 	\param count Number of items in the vector
 * 	\param blocklen Lenght of a block in terms of old_type
 * 	\param stride Size of gaps in bytes
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_hvector (int count,
				 	 int blocklen,
					 MPI_Aint stride,
					 MPI_Datatype old_type,
					 MPI_Datatype * newtype_p)
{
	unsigned long extent;
	unsigned long stride_t;
	
	stride_t = (unsigned long) stride;
	
	/* Is it a derived data-type ? */
	if (sctk_datatype_is_derived (old_type))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		/* Compute the extent */
		__INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

		sctk_nodebug ("extend %lu, count %d, blocklen %d, stide %lu", extent, count, blocklen, stride);

		/* The vector is created by repeating the input blocks
		 * count * blocklen times where the actual input type
		 * contains input_datatype.count entries */
		unsigned long count_out = input_datatype.count * count * blocklen;
		
		mpc_pack_absolute_indexes_t *begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t *ends_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t *datatypes = sctk_malloc (count_out * sizeof (sctk_datatype_t));
		
		/* Here we flatten the vector */
		int i;
		int j;
		unsigned long k;
		for (i = 0; i < count; i++)
		{
			/* For block */
			for (j = 0; j < blocklen; j++)
			{
				/* For each block in the block length */
				for (k = 0; k < input_datatype.count; k++)
				{
					/* For each input block */
					begins_out[(i * blocklen + j) * input_datatype.count + k] =	input_datatype.begins[k] + (stride_t * i) + (j * extent);
					ends_out[(i * blocklen + j) * input_datatype.count + k] = input_datatype.ends[k] + (stride_t * i) + (j * extent);
					datatypes[(i * blocklen + j) * input_datatype.count + k] = input_datatype.datatypes[k];
				}
			}
		}

		/* Compute the new upper bound */
		int new_ub = input_datatype.ub + extent * stride * (count - 1) + extent * (blocklen - 1);

		/* Create the derived datatype */
		PMPC_Derived_datatype (newtype_p, begins_out, ends_out, datatypes, count_out, input_datatype.lb, input_datatype.is_lb, new_ub, input_datatype.is_ub);

		/* Set its context */
		struct Datatype_External_context dtctx;
		sctk_datatype_external_context_clear( &dtctx );
		dtctx.combiner = MPI_COMBINER_HVECTOR;
		dtctx.count = count;
		dtctx.blocklength = blocklen;
		dtctx.stride_addr = stride;
		dtctx.oldtype = old_type;
		MPC_Datatype_set_context( *newtype_p, &dtctx);
		
		/* Free temporary arrays */
		sctk_free (begins_out);
		sctk_free (ends_out);
		sctk_free (datatypes);

		return MPI_SUCCESS;
	}
	else
	{
		/* Here vector only handles derived data-types*/
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous type to a derived data-type */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call vector again with the temporary
		 * derived types */
		res = __INTERNAL__PMPI_Type_hvector (count, blocklen, (MPI_Aint) stride_t, data_out, newtype_p);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}

/** \brief This function creates a sequence of contiguous blocks each with their offset and length
 * 	\param count Number of blocks
 * 	\param blocklen length of individual blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_indexed (int count, int blocklens[], int indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	
	/* Create a temporary offset array */
	MPI_Aint * byte_offsets = sctk_malloc (count * sizeof (MPI_Aint));
	assume( byte_offsets != NULL );
	
	int i;
	/* Fill it with by converting type based indices to bytes */
	for( i = 0 ; i < count ; i++ )
		byte_offsets[i] = indices[i] * extent;
	
	/* Call the byte offseted version of type_indexed */
	int res =  __INTERNAL__PMPI_Type_hindexed (count, blocklens, byte_offsets, old_type, newtype);
	
	/* Set its context to overide the one from hdindexed */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_INDEXED;
	dtctx.count = count;
	dtctx.array_of_blocklenght = blocklens;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);

	/* Release the temporary byte offset array */
	sctk_free( byte_offsets );
	
	return res;
}

/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 * 	\param count Number of blocks
 * 	\param blocklength length of every blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Here we just artificially create an array of blocklength to call __INTERNAL__PMPI_Type_indexed */
	int * blocklength_array = sctk_malloc( count * sizeof( int ) );
	assume( blocklength_array != NULL );
	
	int i;
	for( i = 0 ; i < count ; i++ )
		blocklength_array[i] = blocklength;
	
	/* Call the orignal indexed function */
	int res = __INTERNAL__PMPI_Type_hindexed(count, blocklength_array, indices, old_type,  newtype);
	
	/* Set its context to overide the one from hdindexed */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_HINDEXED_BLOCK;
	dtctx.count = count;
	dtctx.blocklength = blocklength;
	dtctx.array_of_displacements_addr = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	/* Free the tmp buffer */
	sctk_free( blocklength_array );
	
	return res;
}


/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 * 	\param count Number of blocks
 * 	\param blocklength length of every blocks
 * 	\param indices Offset of each block in old_type size multiples
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_indexed_block(int count, int blocklength, int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Convert the indices to bytes */
	MPI_Aint extent;
	/* Retrieve type extent */
	__INTERNAL__PMPI_Type_extent(old_type, &extent);
	
	sctk_error("NEWTYPE BEFFORE %d" , *newtype );
	
	/* Create a temporary offset array */
	MPI_Aint * byte_offsets = sctk_malloc (count * sizeof (MPI_Aint));
	assume( byte_offsets != NULL );
	
	int i;
	/* Fill it with by converting type based indices to bytes */
	for( i = 0 ; i < count ; i++ )
		byte_offsets[i] = indices[i] * extent;
	
	sctk_error("NEWTYPE BEFFORE %d" , *newtype );
	
	/* Call the orignal indexed function */
	int res = __INTERNAL__PMPI_Type_create_hindexed_block (count, blocklength, byte_offsets, old_type,  newtype);
	
	sctk_error("NEWTYPE AFTER %d" , *newtype );
		
	/* Set its context to overide the one from hdindexed block */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_INDEXED_BLOCK;
	dtctx.count = count;
	dtctx.blocklength = blocklength;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype = old_type;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	/* Free the temporary byte offset */
	sctk_free( byte_offsets );
	
	return res;
}



/** \brief This function creates a sequence of contiguous blocks each with their offset and length
 * 	\param count Number of blocks
 * 	\param blocklen length of individual blocks
 * 	\param indices Offset of each block in bytes
 * 	\param old_type Input data-type
 * 	\param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_hindexed (int count,
					  int blocklens[],
					  MPI_Aint indices[],
					  MPI_Datatype old_type, MPI_Datatype * newtype)
{
	/* Is it a derived data-type ? */
	if (sctk_datatype_is_derived (old_type))
	{
		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		/* Get datatype extent */
		unsigned long extent;
		__INTERNAL__PMPI_Type_extent (old_type, (MPI_Aint *) & extent);

		/* Compute the total number of blocks */
		unsigned long count_out = 0;
		int i;
		for (i = 0; i < count; i++)
		{
			count_out += input_datatype.count * blocklens[i];
		}

		/* Allocate context */
		mpc_pack_absolute_indexes_t * begins_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		mpc_pack_absolute_indexes_t * ends_out = sctk_malloc (count_out * sizeof (mpc_pack_absolute_indexes_t));
		sctk_datatype_t *datatypes = sctk_malloc (count_out * sizeof (sctk_datatype_t));

		sctk_nodebug ("%lu extent", extent);

		int j;
		unsigned long k;
		unsigned long step = 0;
		long int new_ub  = input_datatype.ub ;
		long int new_lb = input_datatype.lb;
	
		for (i = 0; i < count; i++)
		{
			/* For each output entry */
			for (j = 0; j < blocklens[i]; j++)
			{
				/* For each entry in the block */
				for (k = 0; k < input_datatype.count; k++)
				{
					/* For each block in the input */
					begins_out[step] = input_datatype.begins[k] + indices[i] + j * extent;
					ends_out[step] = input_datatype.ends[k] + indices[i] + j * extent;
					datatypes[step] = input_datatype.datatypes[k];
					step++;
				}
			}
			
			
			/* Update UB */
			long int candidate_b = 0;
			
			if (input_datatype.is_ub)
			{
				candidate_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);
				
				if ( input_datatype.ub < candidate_b)
				{
					new_ub = candidate_b;
				}
			}
			
			/* Update LB */
			if (input_datatype.is_lb)
			{
				candidate_b = input_datatype.lb + indices[i];
				
				if ( candidate_b < input_datatype.lb)
				{
					new_lb = candidate_b;
				}
			}
		}

		/* Create the derived datatype */
		PMPC_Derived_datatype (newtype, begins_out, ends_out, datatypes, count_out, new_lb,	input_datatype.is_lb, new_ub, input_datatype.is_ub);

		/* Set its context */
		struct Datatype_External_context dtctx;
		sctk_datatype_external_context_clear( &dtctx );
		dtctx.combiner = MPI_COMBINER_HINDEXED;
		dtctx.count = count;
		dtctx.array_of_blocklenght = blocklens;
		dtctx.array_of_displacements_addr = indices;
		dtctx.oldtype = old_type;
		MPC_Datatype_set_context( *newtype, &dtctx);
		
		/* Free temporary arrays */
		sctk_free (begins_out);
		sctk_free (ends_out);
		sctk_free (datatypes);
		
		return MPI_SUCCESS;
	}
	else
	{
		/* Here type hindexed only handles derived data-types */
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous/common type to a derived one */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call the hindexed function again */
		res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, data_out, newtype);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}

static int __INTERNAL__PMPI_Type_struct(int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	int i;
	int res = MPI_SUCCESS;

	unsigned long count_out = 0;
	unsigned long glob_count_out = 0;
	mpc_pack_absolute_indexes_t new_lb = (mpc_pack_absolute_indexes_t) (-1);
	int new_is_lb = 0;
	mpc_pack_absolute_indexes_t new_ub = 0;
	int new_is_ub = 0;

	unsigned long my_count_out = 0;
	

	// find malloc size
	for (i = 0; i < count; i++)
	{
		if ((old_types[i] != MPI_UB) && (old_types[i] != MPI_LB))
		{
			int derived_ret = 0;
			sctk_derived_datatype_t input_datatype;
			
			int count_in = 0;

			if (sctk_datatype_is_derived(old_types[i]))
			{
				MPC_Is_derived_datatype(old_types[i], &derived_ret, &input_datatype);
				count_in = input_datatype.count;
				sctk_nodebug("[%d]Derived length %d", old_types[i], count_in);
			}
			else
			{
				count_in = 1;
				sctk_nodebug("[%d]Contiguous length %d",old_types[i], count_in);
			}

			my_count_out += count_in * blocklens[i];
		}
	}
	
	sctk_nodebug("my_count_out = %d", my_count_out);
	mpc_pack_absolute_indexes_t * begins_out = sctk_malloc(my_count_out * sizeof(mpc_pack_absolute_indexes_t));
	mpc_pack_absolute_indexes_t * ends_out = sctk_malloc(my_count_out * sizeof(mpc_pack_absolute_indexes_t));
	sctk_datatype_t *datatypes = sctk_malloc (my_count_out * sizeof (sctk_datatype_t));

	for (i = 0; i < count; i++)
	{
		if ((old_types[i] != MPI_UB) && (old_types[i] != MPI_LB))
		{
			int res, j, is_lb, is_ub;
			size_t tmp;
			
			unsigned long count_in, extent, k, stride_t;
			unsigned long local_count_out = 0;
			unsigned long prev_count_out = 0;

			int derived_ret = 0;
			sctk_derived_datatype_t input_datatype;


			if (sctk_datatype_is_derived(old_types[i]))
			{
				MPC_Is_derived_datatype(old_types[i], &derived_ret, &input_datatype);
				
				__INTERNAL__PMPI_Type_extent(old_types[i], (MPI_Aint *) &extent);
				sctk_nodebug("Extend %lu", extent);
				stride_t = (unsigned long) indices[i];
				
				for (j = 0; j < blocklens[i]; j++)
				{
					for (k = 0; k < input_datatype.count; k++)
					{
						begins_out[glob_count_out] = input_datatype.begins[k] + stride_t + (extent * j);
						ends_out[glob_count_out] = input_datatype.ends[k] + stride_t + (extent * j);
						datatypes[glob_count_out] = input_datatype.datatypes[k];
						
						sctk_nodebug(" begins_out[%lu] = %lu", glob_count_out, begins_out[glob_count_out]);
						sctk_nodebug(" ends_out[%lu] = %lu", glob_count_out, ends_out[glob_count_out]);

						glob_count_out++;
					}
				}
				
				sctk_nodebug("derived type %d new_lb %d new_ub %d before", i, new_lb, new_ub);
				
				if (input_datatype.is_ub)
				{
					mpc_pack_absolute_indexes_t new_b;
					new_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);
					sctk_nodebug("cur ub %d", new_b);
					if ((long int) new_b > (long int) new_ub || new_is_ub == 0)
					{
						new_ub = new_b;
					}
					new_is_ub = 1;
				}
				
				if (input_datatype.is_lb)
				{
					mpc_pack_absolute_indexes_t new_b;
					new_b = input_datatype.lb + indices[i];
					sctk_nodebug("cur lb %d", new_b);
					if ((long int) new_b < (long int) new_lb || new_is_lb == 0)
					{
						new_lb = new_b;
					}
					new_is_lb = 1;
				}
				
				sctk_nodebug("derived type %d new_lb %d new_ub %d after ", i, new_lb, new_ub);
			}
			else
			{
				mpc_pack_absolute_indexes_t *begins_in, *ends_in;
				mpc_pack_absolute_indexes_t begins_in_static, ends_in_static;
				
				begins_in = &begins_in_static;
				ends_in = &ends_in_static;
				begins_in[0] = 0;
				PMPC_Type_size(old_types[i], &(tmp));
				ends_in[0] = begins_in[0] + tmp - 1;
				count_in = 1;
				sctk_nodebug("Type size %lu", tmp);

				__INTERNAL__PMPI_Type_extent(old_types[i], (MPI_Aint *) &extent);

				stride_t = (unsigned long) indices[i];
				for (j = 0; j < blocklens[i]; j++)
				{
						begins_out[glob_count_out] = begins_in[0] + stride_t + (extent * j);
						ends_out[glob_count_out] = ends_in[0] + stride_t + (extent * j);
						datatypes[glob_count_out] = old_types[i];

						sctk_debug(" begins_out[%lu] = %lu", glob_count_out, begins_out[glob_count_out]);
						sctk_debug(" ends_out[%lu] = %lu", glob_count_out, ends_out[glob_count_out]);

						glob_count_out++;
				}
				sctk_nodebug("simple type %d new_lb %d new_ub %d", i, new_lb, new_ub);
/* 				fprintf(stderr,"Type struct %ld-%ld\n",begins_out[i],ends_out[i]+1); */
			}

			prev_count_out = count_out;
			local_count_out = count_in * blocklens[i];
			count_out += local_count_out;
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
			sctk_nodebug("hard bound %d new_lb %d new_ub %d", i, new_lb, new_ub);
		}
		sctk_nodebug("%d new_lb %d new_ub %d", i, new_lb, new_ub);
	}
/* 	fprintf(stderr,"End Type\n"); */

	res = PMPC_Derived_datatype(newtype, begins_out, ends_out, datatypes, glob_count_out, new_lb, new_is_lb, new_ub, new_is_ub);
	assert(res == MPI_SUCCESS);

	/* Set its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_STRUCT;
	dtctx.count = count;
	dtctx.array_of_blocklenght = blocklens;
	dtctx.array_of_displacements_addr = indices;
	dtctx.array_of_types = old_types;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	/*   sctk_nodebug("new_type %d",* newtype); */
	/*   sctk_nodebug("final new_lb %d,%d new_ub %d %d",new_lb,new_is_lb,new_ub,new_is_ub); */
	/*   { */
	/*     int i ;  */
	/*     for(i = 0; i < glob_count_out; i++){ */
	/*       sctk_nodebug("%d begins %lu ends %lu",i,begins_out[i],ends_out[i]); */
	/*     } */
	/*   } */

	sctk_free(begins_out);
	sctk_free(ends_out);
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type)
{
	if( sctk_datatype_is_derived(old_type) )
	{
		int derived_ret;
		sctk_derived_datatype_t input_datatype;
		/* Retrieve input datatype informations */
		MPC_Is_derived_datatype (old_type, &derived_ret, &input_datatype);
		
		/* Duplicate it with updated bounds in new_type */
		PMPC_Derived_datatype ( new_type,
					input_datatype.begins,
					input_datatype.ends,
					input_datatype.datatypes,
					input_datatype.count,
					lb, 1,
					lb + extent, 1);
		
		struct Datatype_External_context dtctx;
		sctk_datatype_external_context_clear( &dtctx );
		dtctx.combiner = MPI_COMBINER_RESIZED;
		dtctx.lb = lb;
		dtctx.extent = extent;
		dtctx.oldtype = old_type;
		MPC_Datatype_set_context( *new_type, &dtctx);
		
		return MPI_SUCCESS;
	}
	else
	{
		/* Here tPMPI_Type_create_resized only handles derived data-types */
		int res;
		MPI_Datatype data_out;
		
		/* Convert the contiguous/common type to a derived one */
		PMPC_Type_convert_to_derived( old_type, &data_out );
		
		/* Call the hindexed function again */
		res = __INTERNAL__PMPI_Type_create_resized (data_out, lb, extent, new_type);
		
		/* Free the temporary type */
		__INTERNAL__PMPI_Type_free (&data_out);

		return res;
	}
}


int __INTERNAL__PMPI_Type_create_darray (int size,
					 int rank,
					 int ndims,
					 int array_of_gsizes[],
					 int array_of_distribs[],
					 int array_of_dargs[],
					 int array_of_psizes[],
					 int order,
					 MPI_Datatype oldtype,
					 MPI_Datatype *newtype)
{
	int res = sctk_Type_create_darray( size, rank, ndims,  array_of_gsizes,
					   array_of_distribs,  array_of_dargs, array_of_psizes,
					   order, oldtype,   newtype );
	
	if( res != MPI_SUCCESS )
		return res;
	
	/* Fill its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_DARRAY;
	dtctx.size = size;
	dtctx.rank = rank;
	dtctx.array_of_gsizes = array_of_gsizes;
	dtctx.array_of_distribs = array_of_distribs;
	dtctx.array_of_dargs = array_of_dargs;
	dtctx.array_of_psizes = array_of_psizes;
	dtctx.order = order;
	dtctx.oldtype = oldtype;
	MPC_Datatype_set_context( *newtype, &dtctx);
	
	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Type_create_subarray (int ndims,
					   int array_of_sizes[],
					   int array_of_subsizes[],
					   int array_of_starts[],
					   int order,
					   MPI_Datatype oldtype,
					   MPI_Datatype * new_type)
{
	/* Create the subarray */
	int res = sctk_Type_create_subarray( ndims,  array_of_sizes,  array_of_subsizes, array_of_starts,  order, oldtype,  new_type);

	if( res != MPI_SUCCESS )
		return res;
	
	/* Fill its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPI_COMBINER_SUBARRAY;
	dtctx.ndims = ndims;
	dtctx.array_of_sizes = array_of_sizes;
	dtctx.array_of_subsizes = array_of_subsizes;
	dtctx.array_of_starts = array_of_starts;
	dtctx.order = order;
	dtctx.oldtype = oldtype;
	MPC_Datatype_set_context( *new_type, &dtctx);
	
	return res;
}



static int __INTERNAL__PMPI_Address (void *location, MPI_Aint * address)
{
	*address = (MPI_Aint) location;
	return MPI_SUCCESS;
}

static int __INTERNAL__PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
	MPI_Aint UB;
	MPI_Aint LB;
	
	/* Special cases */
	if (datatype == MPI_DATATYPE_NULL)
	{
		/* Here we return 0 for data-type null
		 * in order to pass the struct-zero-count test */
		return 0;
	}

	__INTERNAL__PMPI_Type_lb (datatype, &LB);
	__INTERNAL__PMPI_Type_ub (datatype, &UB);

	*extent = (MPI_Aint) ((unsigned long) UB - (unsigned long) LB);
	return MPI_SUCCESS;
}

/* See the 1.1 version of the Standard.  The standard made an
   unfortunate choice here, however, it is the standard.  The size returned
   by MPI_Type_size is specified as an int, not an MPI_Aint */
static int __INTERNAL__PMPI_Type_size (MPI_Datatype datatype, int *size)
{
	size_t tmp_size;
	int real_val;
	int res;
	
	res = PMPC_Type_size (datatype, &tmp_size);
	
	real_val = tmp_size;
	*size = real_val;
	return res;
}

static int __INTERNAL__PMPI_Type_size_x (MPI_Datatype datatype, MPI_Count *size)
{
	size_t tmp_size;
	MPI_Count real_val;
	int res;

	res = PMPC_Type_size (datatype, &tmp_size);
	
	real_val = tmp_size;
	*size = real_val;
	return res;
}





/* MPI_Type_count was withdrawn in MPI 1.1 */
static int __INTERNAL__PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
	unsigned long i;

	int derived_ret = 0;
	sctk_derived_datatype_t input_datatype;

	MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
	
	if (derived_ret)
	{
		if (input_datatype.is_lb == 0)
		{
			*displacement = (MPI_Aint) input_datatype.opt_begins[0];
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				if ((mpc_pack_absolute_indexes_t) * displacement > input_datatype.opt_begins[i])
				{
					*displacement = (MPI_Aint) input_datatype.opt_begins[i];
				}
			}
		}
		else
		{
			*displacement = input_datatype.lb;
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
	unsigned long i;
	int e;

	int derived_ret = 0;
	sctk_derived_datatype_t input_datatype;

	MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
	
	if (derived_ret)
	{
		if (input_datatype.is_ub == 0)
		{
			*displacement = (MPI_Aint) input_datatype.opt_ends[0];
			
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				if ((mpc_pack_absolute_indexes_t) * displacement < input_datatype.opt_ends[i])
				{
					*displacement = (MPI_Aint) input_datatype.opt_ends[i];
				}
				sctk_nodebug ("Current ub %lu, %lu", input_datatype.opt_ends[i], *displacement);
			}
			
			e = 1;
			*displacement = (MPI_Aint) ((unsigned long) (*displacement) + e);
		}
		else
		{
			*displacement = input_datatype.ub;
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

static int __INTERNAL__PMPI_Type_commit (MPI_Datatype * datatype)
{
  return PMPC_Type_commit(datatype);
}

static int __INTERNAL__PMPI_Type_free (MPI_Datatype * datatype)
{
  return PMPC_Type_free (datatype);
}

static int __INTERNAL__PMPI_Get_elements_x (MPI_Status * status, MPI_Datatype datatype, MPI_Count *elements)
{
	int res = MPI_SUCCESS;
	long long int size;
	int data_size;

	/* First check if the status is valid */
	if (NULL == status || MPI_STATUSES_IGNORE == status ||  MPI_STATUS_IGNORE == status || NULL == elements)
	{
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid argument");
	}
	
	/* Now check the data-type */
	else if (MPI_DATATYPE_NULL == datatype)
	{
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_TYPE, "Invalid type");
	}

	/* Get type size */
	res = __INTERNAL__PMPI_Type_size (datatype, &data_size);
	
	if (res != MPI_SUCCESS) {
		return res;
	}

	
	MPI_Datatype data_in;
	int data_in_size;
	size_t count;
	sctk_task_specific_t *task_specific;
	unsigned long i;

	*elements = 0;
	
	/* If size is NULL we have no element */
	if (data_size == 0)
	{
		return res;
	}
	
	/* If we are here our type has a size
	 * we can now count the number of elements*/
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_CONTIGUOUS:
			/* A contiguous datatype is made of several
			 * elements of a single type next to each other
			 * we have to find out how many elements of this
			 * type are here */
			task_specific = __MPC_get_task_specific ();
			
			/* First retrieve the contiguous type descriptor */
			sctk_datatype_lock( task_specific );
			sctk_contiguous_datatype_t *contiguous_user_types = sctk_task_specific_get_contiguous_datatype( task_specific, datatype );
			/* Number of entries in the contiguous type */
			count = contiguous_user_types->count;
			/* Input type */
			data_in = contiguous_user_types->datatype;
			sctk_datatype_unlock( task_specific );
			
			/* Retrieve the size of the input type */
			res = __INTERNAL__PMPI_Type_size (data_in, &data_in_size);

			/* This is the size we received */
			size = status->size;

			*elements = size/data_in_size;
			
			if (res != MPI_SUCCESS) {
				return res;
			}
		break;
		
		case MPC_DATATYPES_DERIVED:
			task_specific = __MPC_get_task_specific ();
			
			/* This is the size we received */
			size = status->size;

			*elements = 0;

			/* Retrieve the derived datatype */
			sctk_datatype_lock( task_specific );
			sctk_assert ( MPC_TYPE_MAP_TO_DERIVED(datatype) < SCTK_USER_DATA_TYPES_MAX);	
			sctk_derived_datatype_t *target_type = sctk_task_specific_get_derived_datatype(  task_specific, datatype );
			sctk_assert ( target_type != NULL);
			sctk_datatype_unlock( task_specific );
			
			/* Try to rely on the datype layout */
			struct Datatype_layout *layout = sctk_datatype_layout( &target_type->context, &count );
			
			int done = 0;
			
			if( layout )
			{
				if( ! count )
				{
					sctk_fatal("We found an empty layout");
				}
				
				while( !done )
				{
				
					sctk_nodebug("count : %d  size : %d done : %d", count, size, done);
					for(i = 0; i < count; i++)
					{
						sctk_nodebug("BLOCK SIZE  : %d", layout[i].size );
						
						size -= layout[i].size;
						
						(*elements)++;

						if(size <= 0)
						{	
							done = 1;
							break;
						}
					}
				
				}
				
				sctk_free(layout);
			}
			else
			{
				/* Retrieve the number of block in the datatype */
				count = target_type->count;

				while( !done )
				{
					
					/* Count the number of elements by substracting
					* individual blocks from total size until reaching 0 */
					for(i = 0; i < count; i++)
					{
						size -= target_type->ends[i] - target_type->begins[i] + 1;
						
						(*elements)++;

						if(size <= 0)
						{
							done = 1;
							break;
						}
					}
				}
			}
		break;
		
		case MPC_DATATYPES_COMMON:
			/* This is the size we received */
			size = status->size;
			
			sctk_nodebug ("Normal type : count %d, data_type %d (size %d)", size, datatype, data_size);
			
			/* Check if we have an exact number of object */
			if (size % data_size == 0)
			{
				/* If so elements can be directly computed */
				size = size / data_size;
				*elements = size;
			}
			else
			{
				/* Here we can say nothing as the size is not
				 * a multiple of the data-type size */ 
				*elements = MPI_UNDEFINED;
			}
			break;
		
		default:
			not_reachable();
	}


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
	if (sctk_datatype_is_derived (datatype))
	{
		unsigned long i;
		int j;
		unsigned long extent;

		__INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;
		
		MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);

		for (j = 0; j < incount; j++)
		{
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				unsigned long size;
				size = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;
				
				sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);
				
				if (size != 0)
				{
					memcpy (&(((char *) outbuf)[*position]), &(((char *) inbuf)[input_datatype.opt_begins[i]]), size);
				}
				
				copied += size;
				sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);
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

		if(inbuf == NULL){
			return MPI_ERR_BUFFER;
		}

		if(outbuf == NULL){
			return MPI_ERR_BUFFER;
		}

		PMPC_Type_size (datatype, &size);
		sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu", 0, size * incount, *position, size * incount);
		memcpy (&(((char *) outbuf)[*position]), inbuf, size * incount);
		sctk_nodebug ("Pack %lu->%lu, ==> %lu %lu done", 0, size * incount, *position, size * incount);
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
	sctk_nodebug("MPI_Unpack insise = %d, position = %d, outcount = %d, datatype = %d, comm = %d", insize, *position, outcount, datatype, comm);
	if (sctk_datatype_is_derived (datatype))
	{
		unsigned long count_out;
		unsigned long i;
		int j;
		unsigned long extent;

		__INTERNAL__PMPI_Type_extent (datatype, (MPI_Aint *) & extent);

		int derived_ret = 0;
		sctk_derived_datatype_t output_datatype;

		MPC_Is_derived_datatype (datatype, &derived_ret, &output_datatype);
		for (j = 0; j < outcount; j++)
		{
			for (i = 0; i < output_datatype.opt_count; i++)
			{
				size_t size;
				size = output_datatype.opt_ends[i] - output_datatype.opt_begins[i] + 1;
				copied += size;
				sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.ends[i] + extent * j);
				if (size != 0)
				{
					memcpy (&(((char *) outbuf)[output_datatype.opt_begins[i]]), &(((char *) inbuf)[*position]), size);
				}
				sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.opt_ends[i] + extent * j);
				*position = *position + size;
				/* just to display the data size */
				int tmp;
				PMPI_Type_size(datatype, &tmp);
				sctk_nodebug ("copied %lu <= insize %lu | outcount %d, data_size %d", copied, insize, outcount, tmp);
				/* done */
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
		sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu", *position, size * outcount, 0, size * outcount);
		memcpy (outbuf, &(((char *) inbuf)[*position]), size * outcount);
		sctk_nodebug ("Unpack %lu %lu, ==> %lu->%lu done", *position, size * outcount, 0, size * outcount);
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
	if (sctk_datatype_is_derived (datatype))
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

		int derived_ret = 0;
		sctk_derived_datatype_t input_datatype;
		
		MPC_Is_derived_datatype (datatype, &derived_ret, &input_datatype);
		
		sctk_nodebug("derived datatype : res %d, count_in %d, lb %d, is_lb %d, ub %d, is_ub %d", derived_ret, input_datatype.count, input_datatype.lb, input_datatype.is_lb, input_datatype.ub, input_datatype.is_ub);
		
		for (j = 0; j < incount; j++)
		{
			for (i = 0; i < input_datatype.opt_count; i++)
			{
				size_t sizet;
				sizet = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;
				*size = *size + sizet;
				sctk_nodebug ("PACK derived part size %d, %lu-%lu", sizet,
				input_datatype.opt_begins[i], input_datatype.opt_ends[i]);
			}
		}
		
		sctk_nodebug ("PACK derived final size %d", *size);
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

  if (sctk_datatype_is_derived (datatype) && !sctk_datatype_is_struct_datatype(datatype))
    {
      int rank;
      int size;
      int offset = 0;
      void *tmp_buf;

      __INTERNAL__PMPI_Pack_size (count, datatype, comm, &size);
      tmp_buf = sctk_malloc (size);
      assume (tmp_buf != NULL);

      __INTERNAL__PMPI_Comm_rank (comm, &rank);
      if (root == rank)
	{
	  __INTERNAL__PMPI_Pack (buffer, count, datatype, tmp_buf, size, &offset, comm);
	  res = PMPC_Bcast (tmp_buf, size, MPI_PACKED, root, comm);

	}
      else
	{
	  res = PMPC_Bcast (tmp_buf, size, MPI_PACKED, root, comm);
	  __INTERNAL__PMPI_Unpack (tmp_buf, size, &offset, buffer, count, datatype, comm);
	}

      sctk_free (tmp_buf);
    }
  else
    {
      res = PMPC_Bcast (buffer, count, datatype, root, comm);
    }
  sctk_nodebug ("BCAST with root %d DONE res %d", root, res);
  return res;
}

#define MPI_MAX_CONCURENT 100
static int
__INTERNAL__PMPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
			 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
			 int root, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
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
	      __INTERNAL__PMPI_Waitall(j+1, recvrequest, MPI_STATUSES_IGNORE);
/*
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(recvrequest[j]),
					 MPI_STATUS_IGNORE);
		}
*/
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
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
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
              __INTERNAL__PMPI_Waitall(j+1,recvrequest, MPI_STATUSES_IGNORE);

/*	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(recvrequest[j]),
					 MPI_STATUS_IGNORE);
		}*/
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
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
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
              __INTERNAL__PMPI_Waitall(j+1,sendrequest, MPI_STATUSES_IGNORE);
/*
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(sendrequest[j]),
					 MPI_STATUS_IGNORE);
		}
*/
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
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
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
              __INTERNAL__PMPI_Waitall(j+1,sendrequest, MPI_STATUSES_IGNORE);
/*
	      for (; j >= 0; j--)
		{
		  __INTERNAL__PMPI_Wait (&(sendrequest[j]),
					 MPI_STATUS_IGNORE);
		}
*/
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
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
    {
      int root = 0;
      int size;
      __INTERNAL__PMPI_Comm_size (comm, &size);

      __INTERNAL__PMPI_Gather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
      __INTERNAL__PMPI_Bcast (recvbuf, size * recvcount, recvtype, root, comm);
      return MPI_SUCCESS;
    }
  else
    {
      return PMPC_Allgather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm); 
    }
}
static int
__INTERNAL__PMPI_Allgatherv (void *sendbuf, int sendcount,
			     MPI_Datatype sendtype, void *recvbuf,
			     int *recvcounts, int *displs,
			     MPI_Datatype recvtype, MPI_Comm comm)
{
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
    {
      int size;
      int root = 0;
      MPI_Aint dsize;

      __INTERNAL__PMPI_Comm_size (comm, &size);
      __INTERNAL__PMPI_Gatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
	    
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
      return PMPC_Allgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    }
}
static int
__INTERNAL__PMPI_Alltoall (void *sendbuf, int sendcount,
			   MPI_Datatype sendtype, void *recvbuf,
			   int recvcount, MPI_Datatype recvtype,
			   MPI_Comm comm)
{
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
    {
TODO("Should be optimized like PMPC_Alltoall")
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
  if (sctk_datatype_is_derived (sendtype) || sctk_datatype_is_derived (recvtype))
    {
TODO("Should be optimized like PMPC_Alltoallv")
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

static int
__INTERNAL__PMPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
				   void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
	return PMPC_Alltoallw (sendbuf, sendcnts, sdispls, sendtypes, recvbuf,
			    recvcnts, rdispls, recvtypes, comm);
}

/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    MPI_Aint extent;
    int rank;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	int rc = MPI_SUCCESS, dim, nreqs=0;
	PMPI_Comm_rank(comm, &rank);
	
	PMPI_Type_extent(recvtype, &extent);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc((4*(topo->data.cart.ndims))*sizeof(MPI_Request *));
	
	for (dim = 0, nreqs=0; dim < topo->data.cart.ndims ; ++dim) 
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;
		
        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }
	
        if (srank != MPI_PROC_NULL)
        {
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", srank, rank);
            rc = PMPI_Irecv(recvbuf, recvcount, recvtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) break;
			
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank, srank);
            rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, srank, 2, comm, &reqs[nreqs+1]);
            if (rc != MPI_SUCCESS) break;
	
            nreqs += 2;
        }
	
        recvbuf = (char *) recvbuf + extent * recvcount;

        if (drank != MPI_PROC_NULL)
        {
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", drank, rank);
            rc = PMPI_Irecv(recvbuf, recvcount, recvtype, drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) break;
			
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank, drank);
            rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, drank, 2, comm, &reqs[nreqs+1]);
            if (rc != MPI_SUCCESS) break;
	
            nreqs += 2;
        }
	
        recvbuf = (char *) recvbuf + extent * recvcount;
	}
	
	if (rc != MPI_SUCCESS) {
        return rc;
    }
	
    return PMPI_Waitall (nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgather_graph(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int i = 0;
	int rc = MPI_SUCCESS;
	int degree;
	int neighbor;
	int rank; 
	const int *edges;
	MPI_Aint extent;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	
	PMPI_Comm_rank(comm, &rank);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	PMPI_Graph_neighbors_count(comm, rank, &degree);
	
	edges = topo->data.graph.edges;
    if (rank > 0) {
        edges += topo->data.graph.index[rank - 1];
    }
    PMPI_Type_extent(recvtype, &extent);
    reqs = sctk_malloc((2*degree)*sizeof(MPI_Request *));
    
    for (neighbor = 0; neighbor < degree ; ++neighbor) 
    {
		sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_graph: Recv from %d to %d", edges[neighbor], rank);
		rc = PMPI_Irecv(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) 
			break;
		i++;
        recvbuf = (char *) recvbuf + extent * recvcount;
        
        sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_graph: Rank %d send to %d", rank, edges[neighbor]);
        rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, edges[neighbor], 1, comm, &reqs[i]);                          
        if (rc != MPI_SUCCESS) 
			break;
		i++;
	}
	if (rc != MPI_SUCCESS)
		return rc;
	
	return PMPI_Waitall(degree*2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    MPI_Aint extent;
    int rank;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	int rc = MPI_SUCCESS, dim, nreqs=0, i;
	PMPI_Comm_rank(comm, &rank);
	
	PMPI_Type_extent(recvtype, &extent);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc((4*(topo->data.cart.ndims))*sizeof(MPI_Request *));

    for (dim = 0, i = 0, nreqs = 0 ; dim < topo->data.cart.ndims ; ++dim, i += 2) 
    {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL) 
        {
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d", srank, rank);
            rc = PMPI_Irecv((char *) recvbuf + displs[i] * extent, recvcounts[i], recvtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) break;
			
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank, srank);
            rc = PMPI_Isend((void *) sendbuf, sendcount, sendtype, srank, 2, comm, &reqs[nreqs+1]);
            if (rc != MPI_SUCCESS) break;
            nreqs += 2;
        }

        if (drank != MPI_PROC_NULL) 
        {
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d", drank, rank);
            rc = PMPI_Irecv((char *) recvbuf + displs[i+1] * extent, recvcounts[i+1], recvtype, drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) break;
			
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank, drank);
            rc = PMPI_Isend((void *) sendbuf, sendcount, sendtype, drank, 2, comm, &reqs[nreqs+1]);
            if (rc != MPI_SUCCESS) break;
            nreqs += 2;
        }
    }

    if (rc != MPI_SUCCESS) {
        return rc;
    }

    return PMPI_Waitall (nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i = 0;
	int rc = MPI_SUCCESS;
	int degree;
	int neighbor;
	int rank; 
	const int *edges;
	MPI_Aint extent;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	
	PMPI_Comm_rank(comm, &rank);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	PMPI_Graph_neighbors_count(comm, rank, &degree);
	
    edges = topo->data.graph.edges;
    if (rank > 0) {
        edges += topo->data.graph.index[rank - 1];
    }

    PMPI_Type_extent(recvtype, &extent);
    reqs = sctk_malloc((2*degree)*sizeof(MPI_Request *));

    for (neighbor = 0; neighbor < degree ; ++neighbor) 
    {
		sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_graph: Recv from %d to %d", edges[neighbor], rank);
        rc = PMPI_Irecv((char *) recvbuf + displs[neighbor] * extent, recvcounts[neighbor], recvtype, edges[neighbor], 2, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) 
			break;
		i++;
		
		sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_graph: Rank %d send to %d", rank, edges[neighbor]);
        rc = PMPI_Isend((void *) sendbuf, sendcount, sendtype, edges[neighbor], 2, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) 
			break;
		i++;
    }

    if (rc != MPI_SUCCESS)
		return rc;

    return PMPI_Waitall(degree*2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_cart(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	MPI_Aint rdextent;
	MPI_Aint sdextent;
    int rank;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	int rc = MPI_SUCCESS, dim, nreqs=0, i;
	PMPI_Comm_rank(comm, &rank);
	
	PMPI_Type_extent(recvtype, &rdextent);
	PMPI_Type_extent(recvtype, &sdextent);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc((4*(topo->data.cart.ndims))*sizeof(MPI_Request *));

    for (dim = 0, nreqs = 0; dim < topo->data.cart.ndims ; ++dim) 
    {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;
        
        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Irecv(recvbuf, recvcount, recvtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        recvbuf = (char *) recvbuf + rdextent * recvcount;

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Irecv(recvbuf, recvcount, recvtype, drank, 2, comm, &reqs[nreqs]);                        
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        recvbuf = (char *) recvbuf + rdextent * recvcount;
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    for (dim = 0 ; dim < topo->data.cart.ndims ; ++dim) {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        sendbuf = (char *) sendbuf + sdextent * sendcount;

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        sendbuf = (char *) sendbuf + sdextent * sendcount;
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    return PMPI_Waitall (nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_graph(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int i = 0;
	int rc = MPI_SUCCESS;
	int degree;
	int neighbor;
	int rank; 
	const int *edges;
	MPI_Aint rdextent;
	MPI_Aint sdextent;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	
	PMPI_Comm_rank(comm, &rank);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	PMPI_Graph_neighbors_count(comm, rank, &degree);
	
	edges = topo->data.graph.edges;
    if (rank > 0) {
        edges += topo->data.graph.index[rank - 1];
    }
    
    PMPI_Type_extent(recvtype, &rdextent);
    PMPI_Type_extent(sendtype, &sdextent);
    reqs = sctk_malloc((2*degree)*sizeof(MPI_Request *));

    for (neighbor = 0; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Irecv(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) break;
        i++;
        recvbuf = (char *) recvbuf + rdextent * recvcount;
    }

    for (neighbor = 0 ; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Isend((void *)sendbuf, sendcount, sendtype, edges[neighbor], 1, comm, &reqs[i]);             
        if (rc != MPI_SUCCESS) break;
        i++;
        sendbuf = (char *) sendbuf + sdextent * sendcount;
    }

    if (rc != MPI_SUCCESS) {
        return rc;
    }

    return PMPI_Waitall(degree*2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
	MPI_Aint rdextent;
	MPI_Aint sdextent;
    int rank;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	int rc = MPI_SUCCESS, dim, nreqs=0, i;
	PMPI_Comm_rank(comm, &rank);
	
	PMPI_Type_extent(recvtype, &rdextent);
	PMPI_Type_extent(recvtype, &sdextent);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc((4*(topo->data.cart.ndims))*sizeof(MPI_Request *));

    for (dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims ; ++dim, i +=2) 
    {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;
        
        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Irecv((char *) recvbuf + rdispls[i] * rdextent, recvcounts[i], recvtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Irecv((char *) recvbuf + rdispls[i+1] * rdextent, recvcounts[i+1], recvtype, drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    for (dim = 0, i = 0; dim < topo->data.cart.ndims ; ++dim, i += 2) {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Isend((char *) sendbuf + sdispls[i] * sdextent, sendcounts[i], sendtype, srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Isend((char *) sendbuf + sdispls[i+1] * sdextent, sendcounts[i+1], sendtype, drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    return PMPI_Waitall (nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i = 0;
	int rc = MPI_SUCCESS;
	int degree;
	int neighbor;
	int rank; 
	const int *edges;
	MPI_Aint rdextent;
	MPI_Aint sdextent;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	
	PMPI_Comm_rank(comm, &rank);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	PMPI_Graph_neighbors_count(comm, rank, &degree);
	
	edges = topo->data.graph.edges;
    if (rank > 0) {
        edges += topo->data.graph.index[rank - 1];
    }
    
    PMPI_Type_extent(recvtype, &rdextent);
    PMPI_Type_extent(sendtype, &sdextent);
    reqs = sctk_malloc((2*degree)*sizeof(MPI_Request *));

    for (neighbor = 0; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Irecv((char *) recvbuf + rdispls[neighbor] * rdextent, recvcounts[neighbor], recvtype, edges[neighbor], 1, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) break;
        i++;
    }
    
    if (rc != MPI_SUCCESS) {
        return rc;
    }

    for (neighbor = 0 ; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Isend((char *) sendbuf + sdispls[neighbor] * sdextent, sendcounts[neighbor], sendtype, edges[neighbor], 1, comm, &reqs[i]);             
        if (rc != MPI_SUCCESS) break;
        i++;
    }

    if (rc != MPI_SUCCESS) {
        return rc;
    }

    return PMPI_Waitall(degree*2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int rank;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	int rc = MPI_SUCCESS, dim, nreqs=0, i;
	PMPI_Comm_rank(comm, &rank);
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc((4*(topo->data.cart.ndims))*sizeof(MPI_Request *));

    for (dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims ; ++dim, i +=2) 
    {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;
        
        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Irecv((char *) recvbuf + rdispls[i], recvcounts[i], recvtypes[i], srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Irecv((char *) recvbuf + rdispls[i+1], recvcounts[i+1], recvtypes[i+1], drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    for (dim = 0, i = 0; dim < topo->data.cart.ndims ; ++dim, i += 2) {
        int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

        if (topo->data.cart.dims[dim] > 1) {
            PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
        } else if (1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim]) {
            srank = drank = rank;
        }

        if (srank != MPI_PROC_NULL)  
        {
            rc = PMPI_Isend((char *) sendbuf + sdispls[i], sendcounts[i], sendtypes[i], srank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }

        if (drank != MPI_PROC_NULL) 
        {
            rc = PMPI_Isend((char *) sendbuf + sdispls[i+1], sendcounts[i+1], sendtypes[i+1], drank, 2, comm, &reqs[nreqs]);
            if (rc != MPI_SUCCESS) 
				break;
            nreqs++;
        }
    }

    if (rc != MPI_SUCCESS) 
	{
        return rc;
    }

    return PMPI_Waitall (nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int i = 0;
	int rc = MPI_SUCCESS;
	int degree;
	int neighbor;
	int rank; 
	const int *edges;
	MPI_Request * reqs;
	mpi_topology_per_comm_t* topo;
	mpc_mpi_per_communicator_t* tmp;
	
	PMPI_Comm_rank(comm, &rank);
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	PMPI_Graph_neighbors_count(comm, rank, &degree);
	
	edges = topo->data.graph.edges;
    if (rank > 0) {
        edges += topo->data.graph.index[rank - 1];
    }
    
    reqs = sctk_malloc((2*degree)*sizeof(MPI_Request *));

    for (neighbor = 0; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Irecv((char *) recvbuf + rdispls[neighbor], recvcounts[neighbor], recvtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);
        if (rc != MPI_SUCCESS) break;
        i++;
    }
    
    if (rc != MPI_SUCCESS) {
        return rc;
    }

    for (neighbor = 0 ; neighbor < degree ; ++neighbor) 
    {
        rc = PMPI_Isend((char *) sendbuf + sdispls[neighbor], sendcounts[neighbor], sendtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);             
        if (rc != MPI_SUCCESS) break;
        i++;
    }

    if (rc != MPI_SUCCESS) {
        return rc;
    }

    return PMPI_Waitall(degree*2, reqs, MPI_STATUSES_IGNORE);
}

typedef struct
{
  MPC_Op op;
  int used;
  int commute;
} sctk_op_t;

 struct sctk_mpi_ops_s
{
  sctk_op_t *ops;
  int size;
  sctk_spinlock_t lock;
} ;

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
#ifdef MPC_PosixAllocator
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
  PMPC_Get_op ( &ops);
  sctk_spinlock_lock (&(ops->lock));

  op -= MAX_MPI_DEFINED_OP;
  assume (op < ops->size);
  assume (ops->ops[op].used == 1);

  tmp = &(ops->ops[op]);

  sctk_spinlock_unlock (&(ops->lock));
  return tmp;
}

static inline int
__INTERNAL__PMPI_Reduce_derived_no_commute (void *sendbuf, void *recvbuf, int count,
					    MPI_Datatype datatype, MPI_Op op, int root,
					    MPI_Comm comm,MPC_Op mpc_op,sctk_op_t *mpi_op, int size, int rank)
{
	int res;
	void* tmp_buf;

	tmp_buf = recvbuf;
	
	if(rank != root)
	{
		MPI_Aint dsize;
		__INTERNAL__PMPI_Type_extent (datatype, &dsize);
		tmp_buf = malloc(count*dsize);
	}

	if(rank == size-1)
	{
		res = __INTERNAL__PMPI_Send (sendbuf, count, datatype, (rank + size - 1) % size, -3, comm);
		if (res != MPI_SUCCESS)
		{
			return res;
		}
	} else {
		res = __INTERNAL__PMPI_Recv (tmp_buf, count, datatype, (rank + 1) % size, -3, comm,
		MPI_STATUS_IGNORE);
		
		if (res != MPI_SUCCESS)
		{
			return res;
		}
	}

	if(rank != size-1)
	{
		if (mpc_op.u_func != NULL)
		{
			mpc_op.u_func (sendbuf, tmp_buf, &count, &datatype);
		}
		else
		{
			not_reachable ();
		}
	}

	if((rank == 0) && (root != 0))
	{
		res = __INTERNAL__PMPI_Send (tmp_buf, count, datatype, root, -3, comm);
		if (res != MPI_SUCCESS)
		{
			return res;
		}
	} else {
		if((rank != size-1) && ((rank != 0)))
		{
		res = __INTERNAL__PMPI_Send (tmp_buf, count, datatype, (rank + size - 1) % size, -3, comm);
		
			if (res != MPI_SUCCESS)
			{
				return res;
			}
		}
	}
	
	if(rank != root){
		free(tmp_buf);
	}
	
	return res;
}

static inline int
__INTERNAL__PMPI_Reduce_derived_commute (void *sendbuf, void *recvbuf, int count,
					    MPI_Datatype datatype, MPI_Op op, int root,
					    MPI_Comm comm,MPC_Op mpc_op,sctk_op_t *mpi_op, int size, int rank){
  /*To optimize*/
  return __INTERNAL__PMPI_Reduce_derived_no_commute(sendbuf,recvbuf,count,datatype,op,root,comm,mpc_op,mpi_op,size,rank);
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

	assume(recvbuf != NULL);

	/* fprintf(stderr,"Reduce %d %d\n",sctk_datatype_is_derived (datatype),mpi_op->commute == 0); */
	if ((sctk_datatype_is_derived (datatype) && !sctk_datatype_is_struct_datatype(datatype)) || (mpi_op->commute == 0))
	{
		int size;
		int rank;
		int res;
		__INTERNAL__PMPI_Comm_rank (comm, &rank);
		__INTERNAL__PMPI_Comm_size (comm, &size);

		if(size == 1){
		MPI_Request request_send;
		MPI_Request request_recv;

		if (sendbuf == MPI_IN_PLACE)
			request_send = MPI_REQUEST_NULL;
		else
			res = __INTERNAL__PMPI_Isend (sendbuf, count, datatype, 0, -3, comm, &request_send);

		if (res != MPI_SUCCESS)
		{
			return res;
		}
		res = __INTERNAL__PMPI_Irecv (recvbuf, count, datatype, 0, -3, comm, &request_recv);
		
		if (res != MPI_SUCCESS)
		{
			return res;
		}

		res = __INTERNAL__PMPI_Wait (&(request_recv), MPI_STATUS_IGNORE);
		
		if (res != MPI_SUCCESS)
		{
			return res;
		}
		
		res = __INTERNAL__PMPI_Wait (&(request_send), MPI_STATUS_IGNORE);
		
		if (res != MPI_SUCCESS)
		{
			return res;
		}
		} 
		else 
		{
			if(mpi_op->commute == 0)
			{
				__INTERNAL__PMPI_Reduce_derived_no_commute(sendbuf,recvbuf,count,datatype,op,root,comm,mpc_op,mpi_op,size,rank);
				if((rank == root) && (root != 0))
				{
					res = __INTERNAL__PMPI_Recv (recvbuf, count, datatype,0, -3, comm, MPI_STATUS_IGNORE);
					
					if (res != MPI_SUCCESS)
					{
						return res;
					}
				}
			}  
			else 
			{
				__INTERNAL__PMPI_Reduce_derived_commute(sendbuf,recvbuf,count,datatype,op,root,comm,mpc_op,mpi_op,size,rank);
				
				if((rank == root) && (root != 0))
				{
					res = __INTERNAL__PMPI_Recv (recvbuf, count, datatype,0, -3, comm, MPI_STATUS_IGNORE);
					
					if (res != MPI_SUCCESS)
					{
						return res;
					}
				}
			}

		}

		res = __INTERNAL__PMPI_Barrier (comm);
		return res;
	}
	else
	{
		return PMPC_Reduce (sendbuf, recvbuf, count, datatype, mpc_op, root, comm);
	}
}
static int
__INTERNAL__PMPI_Op_create (MPI_User_function * function, int commute,
			    MPI_Op * op)
{
  MPC_Op *mpc_op = NULL;
  sctk_mpi_ops_t *ops;
  int i;

  PMPC_Get_op ( &ops);
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

  PMPC_Get_op ( &ops);
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

static size_t MPI_Allreduce_derived_type_floor = 1024;
static int
__INTERNAL__PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
			    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
   MPC_Op mpc_op;
  sctk_op_t *mpi_op;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;
  if (sctk_datatype_is_derived (datatype) || (mpi_op->commute == 0))
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

 /* __INTERNAL__PMPI_Barrier (comm); */
 /* fprintf(stderr,"%d size, %d disze %d datatype %d op\n",size,dsize,datatype,op); */
 /* __INTERNAL__PMPI_Barrier (comm); */

  for (i = 0; i < size; i++)
    {
      if (sendbuf == MPI_IN_PLACE)
	  {
		sendbuf = recvbuf;
		res = __INTERNAL__PMPI_Reduce (sendbuf, recvbuf, recvcnts[i], datatype, op, i, comm);
	  }
	  else
	  {
	  	res = __INTERNAL__PMPI_Reduce (((char *) sendbuf) + (acc * dsize), recvbuf, recvcnts[i], datatype, op, i, comm);
	  }
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
      acc += recvcnts[i];
    }

  res = __INTERNAL__PMPI_Barrier (comm);

  return res;
}

static int
__INTERNAL__PMPI_Reduce_scatter_block (void *sendbuf, void *recvbuf, int recvcnt,
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
	__INTERNAL__PMPI_Reduce (((char *) sendbuf) + (recvcnt * i * dsize), recvbuf,
				 recvcnt, datatype, op, i, comm);
      if (res != MPI_SUCCESS)
	{
	  return res;
	}
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
  MPI_Aint dsize;
  void *tmp;
  int res;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;

  __INTERNAL__PMPI_Comm_rank (comm, &rank);
  __INTERNAL__PMPI_Comm_size (comm, &size);


  __INTERNAL__PMPI_Isend (sendbuf, count, datatype, rank, -3, comm, &request);

  res =  __INTERNAL__PMPI_Type_extent (datatype, &dsize);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  tmp = sctk_malloc (dsize*count);

  res =
    __INTERNAL__PMPI_Recv (recvbuf, count, datatype, rank, -3, comm,
			   MPI_STATUS_IGNORE);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  res = __INTERNAL__PMPI_Barrier (comm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  if (rank != 0)
    {
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

static int
__INTERNAL__PMPI_Exscan (void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  MPC_Op mpc_op;
  sctk_op_t *mpi_op;
  int size;
  int rank;
  MPI_Request request;
  MPI_Aint dsize;
  void *tmp;
  int res;

  mpi_op = sctk_convert_to_mpc_op (op);
  mpc_op = mpi_op->op;

  __INTERNAL__PMPI_Comm_rank (comm, &rank);
  __INTERNAL__PMPI_Comm_size (comm, &size);


  __INTERNAL__PMPI_Isend (sendbuf, count, datatype, rank, -3, comm, &request);

  res =  __INTERNAL__PMPI_Type_extent (datatype, &dsize);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  tmp = sctk_malloc (dsize*count);

  res =
    __INTERNAL__PMPI_Recv (tmp, count, datatype, rank, -3, comm,
			   MPI_STATUS_IGNORE);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  res = __INTERNAL__PMPI_Barrier (comm);
  if (res != MPI_SUCCESS)
    {
      return res;
    }

  if (rank != 0)
    {
      res =
	__INTERNAL__PMPI_Recv (recvbuf, count, datatype, rank - 1, -3, comm,
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
	  MPC_Op_f func;
	  func = sctk_get_common_function (datatype, mpc_op);
	  func (recvbuf, tmp, count, datatype);
	}
    }

  if (rank + 1 < size)
    {
      res =
	__INTERNAL__PMPI_Send (tmp, count, datatype, rank + 1, -3, comm);
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

struct MPI_group_struct_s
{
  sctk_spinlock_t lock;
  long max_size;
  MPI_internal_group_t **tab;
  volatile MPI_internal_group_t *free_list;
  sctk_alloc_buffer_t buf;
} ;


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
  if (tmp == NULL)
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

static inline int
__sctk_is_part_of_group(MPI_Group group)
{
	int rank, i;
	MPC_Group temp_group = __sctk_convert_mpc_group (group);
	__INTERNAL__PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
	for(i = 0 ; i <  temp_group->task_nb ; i++)
	{
		if(temp_group->task_list_in_global_ranks[i] == rank)
		{
			sctk_nodebug("rank = %d, temp_group->task_list_in_global_ranks[%d] == %d", rank, i, temp_group->task_list_in_global_ranks[i]);
			return 1;
		}
	}
	return 0;
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
      if (group->task_list_in_global_ranks[i] == grank)
	{
	  *rank = i;
	  return MPI_SUCCESS;
	}
    }
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_translate_ranks (MPI_Group mpi_group1, int n, int *ranks1, MPI_Group mpi_group2, int *ranks2)
{
	int i,j;
	MPC_Group group1;
	MPC_Group group2;

	if ( (MPI_GROUP_NULL == mpi_group1) || (MPI_GROUP_NULL == mpi_group2) )
	{
		sctk_nodebug("Wrong group 1 : group1 %d, group2 %d", mpi_group1, mpi_group2);
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	}
	if (n > 0 && ((NULL == ranks1) || (NULL == ranks2 )))
	{
		sctk_nodebug("Wrong group n > 0 && ((NULL == ranks1) || (NULL == ranks2 ))");
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	}

	if(n == 0)
		return MPI_SUCCESS;

	if ( MPI_GROUP_EMPTY == mpi_group1 || MPI_GROUP_EMPTY == mpi_group2 )
	{
		for (i = 0; i < n ; i++)
			ranks2[i] = MPI_UNDEFINED;

		return MPI_SUCCESS;
    }

	group1 = __sctk_convert_mpc_group (mpi_group1);
	group2 = __sctk_convert_mpc_group (mpi_group2);

	sctk_nodebug("n = %d", n);
	for(i = 0 ; i < n ; i++)
		sctk_nodebug("ranks1[%d] = %d", i, ranks1[i]);

        for(i = 0 ; i < n ; i++){
                if(ranks1[i] < 0) return MPI_ERR_RANK;
		if(ranks1[i] >= group1->task_nb) return MPI_ERR_RANK;
        }

	for(i = 0 ; i < group1->task_nb ; i++)
		sctk_nodebug("group1 : task_list_in_global_ranks[%d] = %d", i, group1->task_list_in_global_ranks[i]);

	for(i = 0 ; i < group2->task_nb ; i++)
		sctk_nodebug("group2 : task_list_in_global_ranks[%d] = %d", i, group2->task_list_in_global_ranks[i]);

	for (j = 0; j < n; j++)
    {
		int i;
		int grank;
		grank = group1->task_list_in_global_ranks[ranks1[j]];
		ranks2[j] = MPI_UNDEFINED;
		for (i = 0; i < group2->task_nb; i++)
		{
			if (group2->task_list_in_global_ranks[i] == grank)
			{
				ranks2[j] = i;
			}
		}
		sctk_nodebug ("%d is %d", ranks1[j], ranks2[j]);
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
		if (group1->task_list_in_global_ranks[i] != group2->task_list_in_global_ranks[i])
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
			if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
			{
				found = 1;
				break;
			}
		}
		if(found == 0)
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

  for(i = 0 ; i < group1->task_nb ; i++)
	sctk_nodebug("group1 : task_list_in_global_ranks[%d] = %d", i, group1->task_list_in_global_ranks[i]);

  for(i = 0 ; i < group2->task_nb ; i++)
	sctk_nodebug("group2 : task_list_in_global_ranks[%d] = %d", i, group2->task_list_in_global_ranks[i]);

  size = group1->task_nb + group2->task_nb;
  newgroup = __sctk_new_mpc_group (mpi_newgroup);

  newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
  (newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  memcpy ((newgroup)->task_list_in_global_ranks, group1->task_list_in_global_ranks,
	  group1->task_nb * sizeof (int));
  memcpy (&((newgroup)->task_list_in_global_ranks[group1->task_nb]), group2->task_list_in_global_ranks,
	  group2->task_nb * sizeof (int));

  for (i = 0; i < group1->task_nb; i++)
    {
	  for (j = 0; j < group2->task_nb; j++)
		{
		  if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
			{
			  (&((newgroup)->task_list_in_global_ranks[group1->task_nb]))[j] = -1;
			  size--;
			}
		}
    }

  for (i = 0; i < (newgroup)->task_nb; i++)
    {
      if ((newgroup)->task_list_in_global_ranks[i] == -1)
		{
		  for (j = i; j < (newgroup)->task_nb - 1; j++)
			{
			  (newgroup)->task_list_in_global_ranks[j] = (newgroup)->task_list_in_global_ranks[j + 1];
			}
		}
    }

  for(i = 0 ; i < size ; i++)
	sctk_nodebug("task_list_in_global_ranks[%d] = %d", i, (newgroup)->task_list_in_global_ranks[i]);
  (newgroup)->task_nb = size;

  if(size == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;

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
  (newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
  (newgroup)->task_nb = size;
  __sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;

  size = 0;
  for (i = 0; i < group1->task_nb; i++)
    {
      int j;
      for (j = 0; j < group2->task_nb; j++)
	{
	  if (group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
	    {
	      (newgroup)->task_list_in_global_ranks[size] = group1->task_list_in_global_ranks[i];
	      size++;
	      break;
	    }
	}
    }
  (newgroup)->task_nb = size;
  if(size == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_difference (MPI_Group mpi_group1, MPI_Group mpi_group2,
				   MPI_Group * mpi_newgroup)
{
  int result;
  MPC_Group group1;
  MPC_Group group2;
  MPI_internal_group_t *newgroup;

  group1 = __sctk_convert_mpc_group (mpi_group1);
  group2 = __sctk_convert_mpc_group (mpi_group2);
  newgroup = __sctk_new_mpc_group_internal (mpi_newgroup);
  result = PMPC_Group_difference (group1, group2, &(newgroup->group));
  if(result != MPI_SUCCESS)
	return result;
  if(newgroup->group->task_nb == 0)
	*mpi_newgroup = MPI_GROUP_EMPTY;
  return result;

}

static int
__INTERNAL__PMPI_Group_incl (MPI_Group mpi_group, int n, int *ranks,
			     MPI_Group * mpi_newgroup)
{
  int res;
  MPC_Group group;
  int i;
  int j; 
  MPI_internal_group_t *newgroup;

  if(n < 0)
	return MPI_ERR_ARG;

  if(n == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}


  group = __sctk_convert_mpc_group (mpi_group);


  for(i = 0; i < n ; i++){
    if((ranks[i] < 0) || (ranks[i] >= group->task_nb))
      MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
  }
  for(i = 0; i < n ; i++){
    for(j = 0; j < n ; j++){
	if((j != i) && (ranks[i] == ranks[j]))
	 MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
    }
  }

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
	int k;
	int is_out;
	int size;
	MPC_Group group;
	MPC_Group newgroup;

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"");
	else if ((NULL == ranks && n > 0) || (n < 0))
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"");

	group = __sctk_convert_mpc_group (mpi_group);
	newgroup = __sctk_new_mpc_group (mpi_newgroup);
	size = group->task_nb - n;

	if(size == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}
	if(n >=  group->task_nb )MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"");

  for(i = 0; i < n ; i++){
    for(j = 0; j < n ; j++){
        if((j != i) && (ranks[i] == ranks[j]))
         MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
    }
  }


	newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
	(newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
	(newgroup)->task_nb = size;
	__sctk_convert_mpc_group_internal (*mpi_newgroup)->group = newgroup;
	for(i=0; i<size; i++)
		(newgroup)->task_list_in_global_ranks[i] = -1;

	k=0;
	for(i=0 ; i<group->task_nb; i++)
	{
		is_out = 0;
		for(j=0; j<n; j++)
		{
			if((ranks[j] < 0) || (ranks[j] >= group->task_nb))
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_RANK,"Unvalid ranks");
			if(group->task_list_in_global_ranks[i] == ranks[j])
			{
				is_out = 1;
				break;
			}
		}
		if(!is_out)
		{
			(newgroup)->task_list_in_global_ranks[k] = group->task_list_in_global_ranks[i];
			k++;
		}
	}

	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Group_range_excl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
	TODO("Cette partie a été reprise sur OpenMPI");
	int err, i,index;
    int group_size;
    int * elements_int_list;

    int j,k;
    int *ranks_included=NULL;
    int *ranks_excluded=NULL;
    int first_rank,last_rank,stride;
    int count,result;
	sctk_nodebug("MPI_Group_range_excl");
	MPC_Group group;

/* Error checking */
	group = __sctk_convert_mpc_group (mpi_group);

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"group must not be MPI_GROUP_NULL");

	__INTERNAL__PMPI_Group_size (mpi_group, &group_size);
	elements_int_list = (int *) malloc(sizeof(int) * (group_size+1));

	if (NULL == elements_int_list)
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_INTERN,"");

	for (i = 0; i <= group_size; i++)
		elements_int_list[i] = -1;

	for ( i=0; i < n; i++)
	{
		first_rank = ranges[i][0];
		last_rank  = ranges[i][1];
		stride     = ranges[i][2];
		/*fprintf(stderr,"first_rank %d last_rank %d stride %d group_size %d\n",first_rank,last_rank,stride,group_size);*/
		if ((first_rank < 0) || (first_rank >= group_size) || (last_rank < 0) || (last_rank >= group_size) || (stride == 0))
		{
			MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
		}

		if ((first_rank < last_rank))
		{
			if (stride < 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index <= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else if (first_rank > last_rank)
		{
			if (stride > 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index >= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else
		{
			index = first_rank;
			if (elements_int_list[index] != -1)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}
			elements_int_list[index] = i;
		}
	}
	free ( elements_int_list);

/* computing the ranks to be included */
    count = 0;
    /* determine the number of excluded processes for the range-excl-method */
    for(j=0 ; j<n ; j++)
    {
        first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
            index = first_rank;
			while (index <= last_rank)
			{
				count ++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				count ++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
            index = first_rank;
			count ++;
		}
    }
    if (0 != count)
    {
        ranks_excluded = (int *)malloc( group_size*(sizeof(int)));
    }

    /* determine the list of excluded processes for the range-excl method */
    k = 0;
    i = 0;
    for(j=0 ; j<n ; j++)
    {
		first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
			index = first_rank;
			while (index <= last_rank)
			{
				ranks_excluded[i] = index;
				i++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				ranks_excluded[i] = index;
				i++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
			index = first_rank;
			ranks_excluded[i] = index;
			i++;
		}
    }

    if (0 != (group->task_nb - count))
	{
		ranks_included = (int *)malloc( group_size*(sizeof(int)));
	}
	for (j=0 ; j<group->task_nb ; j++)
	{
		for(index=0 ; index<i ; index++)
		{
			if(ranks_excluded[index] == j) break;
		}
		if (index == i)
		{
			ranks_included[k] = j;
			k++;
		}
	}
	//~ sctk_debug("---------");
	//~ for(i=0; i<count; i++)
		//~ sctk_debug("ranks_excluded[%d] = %d", i, ranks_excluded[i]);
	//~ if (NULL != ranks_excluded)
	//~ {
		//~ free(ranks_excluded);
	//~ }

/* including the ranks in the new group */
    result = __INTERNAL__PMPI_Group_incl(mpi_group, k, ranks_included, mpi_newgroup);

    if (NULL != ranks_included)
    {
        free(ranks_included);
    }
    if (NULL != ranks_excluded)
    {
        free(ranks_excluded);
    }

	return result;
}

static int
__INTERNAL__PMPI_Group_range_incl (MPI_Group mpi_group, int n,
				   int ranges[][3], MPI_Group * mpi_newgroup)
{
	TODO("Cette partie a été reprise sur OpenMPI");
	int err, i,index;
    int group_size;
    int * elements_int_list;

    int j,k;
    int *ranks_included=NULL;
    int first_rank,last_rank,stride;
    int count,result;

	MPC_Group group;
	MPC_Group newgroup;

/* Error checking */
	if(n == 0)
	{
		(*mpi_newgroup) = MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}

	if ( (MPI_GROUP_NULL == mpi_group) || (NULL == mpi_newgroup) )
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_GROUP,"group must not be MPI_GROUP_NULL");

	__INTERNAL__PMPI_Group_size (mpi_group, &group_size);
	elements_int_list = (int *) malloc(sizeof(int) * (group_size+1));

	if (NULL == elements_int_list)
		MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_INTERN,"");

	for (i = 0; i < group_size; i++)
		elements_int_list[i] = -1;

	for ( i=0; i < n; i++)
	{
		first_rank = ranges[i][0];
		last_rank  = ranges[i][1];
		stride     = ranges[i][2];

		if ((first_rank < 0) || (first_rank >= group_size) || (last_rank < 0) || (last_rank >= group_size) || (stride == 0))
		{
			MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
		}

		if ((first_rank < last_rank))
		{
			if (stride < 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index <= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else if (first_rank > last_rank)
		{
			if (stride > 0)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}

			for (index = first_rank; index >= last_rank; index += stride)
			{
				if (elements_int_list[index] != -1)
				{
					MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
				}
				elements_int_list[index] = i;
			}
		}
		else
		{
			index = first_rank;
			if (elements_int_list[index] != -1)
			{
				MPI_ERROR_REPORT(MPC_COMM_WORLD,MPI_ERR_ARG,"Wrong ranges");
			}
			elements_int_list[index] = i;
		}
	}
	free ( elements_int_list);

/* computing the ranks to be included */
    count = 0;
    /* determine the number of included processes for the range-incl-method */
    k = 0;
    for(j=0 ; j<n ; j++)
    {

        first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
            index = first_rank;
			while (index <= last_rank)
			{
				count ++;
				k++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				count ++;
				k++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
            index = first_rank;
			count ++;
			k++;
		}
    }
    if (0 != count)
    {
        ranks_included = (int *)malloc( (count)*(sizeof(int)));
    }

    /* determine the list of included processes for the range-incl method */
    k = 0;
    for(j=0 ; j<n ; j++)
    {
		first_rank = ranges[j][0];
		last_rank = ranges[j][1];
		stride = ranges[j][2];

		if (first_rank < last_rank)
		{
			/* positive stride */
			index = first_rank;
			while (index <= last_rank)
			{
				ranks_included[k] = index;
				k++;
				index += stride;
			}
		}
		else if (first_rank > last_rank)
		{
			/* negative stride */
			index = first_rank;
			while (index >= last_rank)
			{
				ranks_included[k] = index;
				k++;
				index += stride;
			}
		}
		else
		{
			/* first_rank == last_rank */
			index = first_rank;
			ranks_included[k] = index;
			k++;
		}
    }
/* including the ranks in the new group */
    result = __INTERNAL__PMPI_Group_incl(mpi_group, k, ranks_included, mpi_newgroup);

    if (NULL != ranks_included)
    {
        free(ranks_included);
    }
	return result;
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
	int result_group;
	*result = MPI_UNEQUAL;

	if(sctk_is_inter_comm (comm1) != sctk_is_inter_comm (comm2))
	{
		*result = MPI_UNEQUAL;
		return MPI_SUCCESS;
	}

	if (comm1 == comm2)
    {
		*result = MPI_IDENT;
		return MPI_SUCCESS;
    }
	else
    {
		if(sctk_get_nb_task_total(comm1) != sctk_get_nb_task_total(comm2))
		{
			*result = MPI_UNEQUAL;
			return MPI_SUCCESS;
		}

		if(comm1 == MPI_COMM_SELF || comm2 == MPI_COMM_SELF)
		{
			if(sctk_get_nb_task_total(comm1) != sctk_get_nb_task_total(comm2))
				*result = MPI_UNEQUAL;
			else
				*result = MPI_CONGRUENT;
			return MPI_SUCCESS;
		}


		MPI_Group comm_group1;
		MPI_Group comm_group2;

		__INTERNAL__PMPI_Comm_group (comm1, &comm_group1);
		__INTERNAL__PMPI_Comm_group (comm2, &comm_group2);

		__INTERNAL__PMPI_Group_compare (comm_group1, comm_group2, &result_group);

		if (result_group == MPI_SIMILAR)
		{
			*result = MPI_SIMILAR;
			return MPI_SUCCESS;
		}
		else if(result_group == MPI_IDENT)
		{
			*result = MPI_CONGRUENT;
			return MPI_SUCCESS;
		}
		else
		{
			*result = MPI_UNEQUAL;
			return MPI_SUCCESS;
		}
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
__INTERNAL__PMPI_Comm_create_from_intercomm (MPI_Comm comm, MPI_Group group,
			      MPI_Comm * newcomm)
{
  return PMPC_Comm_create_from_intercomm (comm, __sctk_convert_mpc_group (group), newcomm);
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
  return PMPC_Intercomm_create (local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

static int
__INTERNAL__PMPI_Intercomm_merge (MPI_Comm intercomm, int high,
				  MPI_Comm * newintracomm)
{
	int rank, grank, remote_high, remote_leader, local_leader;
	sctk_communicator_t peer_comm;
	MPI_Group local_group;
	MPI_Group remote_group;
	MPI_Group new_group;
	MPC_Status status;

	__INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &rank);
	__INTERNAL__PMPI_Comm_rank (intercomm, &grank);
	__INTERNAL__PMPI_Comm_group (intercomm, &local_group);
	__INTERNAL__PMPI_Comm_remote_group (intercomm, &remote_group);

	remote_leader = sctk_get_remote_leader(intercomm);
	local_leader = sctk_get_local_leader(intercomm);
	peer_comm = sctk_get_peer_comm(intercomm);
	if(grank == local_leader)
	{
		sctk_nodebug("grank = %d, rank %d : local_leader %d send to remote_leader %d", grank, rank, local_leader, remote_leader);
		__INTERNAL__PMPI_Sendrecv(&high, 1, MPC_INT, remote_leader, 629, &remote_high, 1, MPC_INT, remote_leader, 629, peer_comm, &status);
	}

	sctk_nodebug("rank %d, grank %d : boroadcast comm %d", rank, grank, intercomm);
	sctk_broadcast(&remote_high,sizeof(int),0,sctk_get_local_comm_id(intercomm));
	//~ PMPI_Bcast(&remote_high, 1, MPC_INT, 0, sctk_get_local_comm_id(intercomm));
	sctk_nodebug("rank %d : merge intercomm %d, high = %d, remote_high = %d", rank, intercomm, high, remote_high);
	/* TODO : Finir le merge avec le placement des groupes en fonction des paramètres "high" */
	if(sctk_is_in_local_group(intercomm))
	{
		if(high && !remote_high)
		{
			sctk_nodebug("high && !remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else if(!high && remote_high)
		{
			sctk_nodebug("!high && remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else if(high && remote_high)
		{
			sctk_nodebug("high && remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else
		{
			sctk_nodebug("!high && !remote_high && in local_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
	}
	else
	{
		if(high && !remote_high)
		{
			sctk_nodebug("high && !remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else if(!high && remote_high)
		{
			sctk_nodebug("!high && remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (local_group, remote_group, &new_group);
		}
		else if(high && remote_high)
		{
			sctk_nodebug("high && remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
		else
		{
			sctk_nodebug("!high && !remote_high && in remote_group");
			__INTERNAL__PMPI_Group_union (remote_group, local_group, &new_group);
		}
	}

	return __INTERNAL__PMPI_Comm_create_from_intercomm (intercomm, new_group, newintracomm);
}

/*
  Attributes
*/

static int MPI_TAG_UB_VALUE = 512*1024*1024;
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

static int
__INTERNAL__PMPI_Keyval_create (MPI_Copy_function * copy_fn,
				MPI_Delete_function * delete_fn,
				int *keyval, void *extra_state)
{
	int i;
	mpc_mpi_data_t* tmp;

	tmp = mpc_mpc_get_per_task_data();
	sctk_nodebug("number = %d, max_number = %d", tmp->number, tmp->max_number);
	sctk_spinlock_lock(&(tmp->lock));
	if (tmp->number >= tmp->max_number)
    {
		tmp->max_number += 100;
		tmp->attrs_fn = sctk_realloc ((void *) (tmp->attrs_fn), tmp->max_number * sizeof (MPI_Caching_key_t));
		for (i = tmp->number; i < tmp->max_number; i++)
		{
			tmp->attrs_fn[i].used = 0;
		}
    }

	*keyval = tmp->number;
	tmp->number++;

	tmp->attrs_fn[*keyval].copy_fn = copy_fn;
	tmp->attrs_fn[*keyval].delete_fn = delete_fn;
	tmp->attrs_fn[*keyval].extra_state = extra_state;
	tmp->attrs_fn[*keyval].used = 1;
	tmp->attrs_fn[*keyval].fortran_key = 0;

	sctk_spinlock_unlock(&(tmp->lock));
	*keyval += MPI_MAX_KEY_DEFINED;
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Keyval_free (int *keyval)
{
  TODO("Optimize to free memory")
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Attr_set_fortran (int keyval)
{
  int res = MPI_SUCCESS;
  MPI_Comm comm = MPI_COMM_WORLD;
  mpc_mpi_data_t* tmp;

  tmp = mpc_mpc_get_per_task_data();

  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  keyval -= MPI_MAX_KEY_DEFINED;

  if (keyval < 0)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }
  sctk_spinlock_lock(&(tmp->lock));
  if (tmp->attrs_fn[keyval].used == 0)
    {
	  sctk_spinlock_unlock(&(tmp->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }

  tmp->attrs_fn[keyval].fortran_key = 1;
  sctk_spinlock_unlock(&(tmp->lock));
  return res;
}

static int
__INTERNAL__PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
	int res = MPI_SUCCESS;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;
	int i;

	tmp = mpc_mpc_get_per_task_data();
	keyval -= MPI_MAX_KEY_DEFINED;

	if (keyval < 0)
    {
		sctk_nodebug("wrong keyval");
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
    }

	sctk_spinlock_lock(&(tmp->lock));
	if (tmp->attrs_fn[keyval].used == 0)
    {
		sctk_nodebug("not used");
		sctk_spinlock_unlock(&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
    }

	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	if((tmp_per_comm->key_vals != NULL) && (tmp_per_comm->key_vals[keyval].flag == 1))
	{
		sctk_spinlock_unlock(&(tmp_per_comm->lock));
		sctk_spinlock_unlock(&(tmp->lock));
		res = __INTERNAL__PMPI_Attr_delete (comm, keyval + MPI_MAX_KEY_DEFINED);
		sctk_spinlock_lock(&(tmp->lock));
		sctk_spinlock_lock(&(tmp_per_comm->lock));
		if(res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock(&(tmp_per_comm->lock));
			sctk_spinlock_unlock(&(tmp->lock));
			return res;
		}
    }

	if(tmp_per_comm->max_number <= keyval)
	{
		if(tmp_per_comm->key_vals == NULL)
			tmp_per_comm->key_vals = sctk_malloc((keyval+1)*sizeof(MPI_Caching_key_value_t));
		else
			tmp_per_comm->key_vals = sctk_realloc(tmp_per_comm->key_vals,(keyval+1)*sizeof(MPI_Caching_key_value_t));

		for(i = tmp_per_comm->max_number; i <= keyval; i++)
		{
			tmp_per_comm->key_vals[i].flag = 0;
			tmp_per_comm->key_vals[i].attr = NULL;
		}

		tmp_per_comm->max_number = keyval+1;
	}

    sctk_nodebug("put %d for keyval %d", *((int *)attr_value), keyval);
    tmp_per_comm->key_vals[keyval].attr = (void *) attr_value;
	tmp_per_comm->key_vals[keyval].flag = 1;

	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int
__INTERNAL__PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value,
			   int *flag)
{
	int res = MPI_SUCCESS;
	mpc_mpi_data_t* tmp;
	mpc_mpi_per_communicator_t* tmp_per_comm;
	void **attr;
	int i;

	*flag = 0;
	attr = (void **) attr_value;

	if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
		*flag = 1;
		*attr = defines_attr_tab[keyval];
		return MPI_SUCCESS;
    }

	keyval -= MPI_MAX_KEY_DEFINED;

	/* wrong keyval */
	if (keyval < 0)
    {
		MPI_ERROR_REPORT (comm, MPI_ERR_KEYVAL, "");
    }

	/* get TLS var for checking if keyval exist */
	tmp = mpc_mpc_get_per_task_data();
	sctk_spinlock_lock(&(tmp->lock));

	/* it doesn-t exist */
	if (tmp->attrs_fn[keyval].used == 0)
    {
		*flag = 0;
		sctk_spinlock_unlock(&(tmp->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_INTERN, "");
    }

	/* get TLS var to check attributes for keyval */
	tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
	sctk_spinlock_lock(&(tmp_per_comm->lock));

	/* it doesn't have any */
	if(tmp->number > tmp_per_comm->max_number)
	{
		*flag = 0;
		*attr = NULL;
	}
	else if(tmp_per_comm->key_vals == NULL )
	{
		*flag = 0;
		*attr = NULL;
	}
    else if(tmp_per_comm->key_vals[keyval].flag == 0)
	{
		*flag = 0;
		*attr = NULL;
	}
	else /* we found one */
	{
		*flag = 1;
        *attr = tmp_per_comm->key_vals[keyval].attr;
    }

	sctk_spinlock_unlock(&(tmp_per_comm->lock));
	sctk_spinlock_unlock(&(tmp->lock));
	return res;
}

static int
__INTERNAL__PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
  int res = MPI_SUCCESS;
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm;
  if ((keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED))
    {
      return MPI_ERR_KEYVAL;
    }
  keyval -= MPI_MAX_KEY_DEFINED;

  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));

  if ((tmp == NULL) || (keyval < 0))
    {
      sctk_spinlock_unlock(&(tmp->lock));
      return MPI_ERR_ARG;
    }
  if ((tmp->attrs_fn[keyval].used == 0) )
    {
      sctk_spinlock_unlock(&(tmp->lock));
      return MPI_ERR_ARG;
    }

  tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
  sctk_spinlock_lock(&(tmp_per_comm->lock));

  if (tmp_per_comm->key_vals[keyval].flag == 1)
    {
      if (tmp->attrs_fn[keyval].delete_fn != NULL)
	{
	  if (tmp->attrs_fn[keyval].fortran_key == 0)
	    {
	      res =
		tmp->attrs_fn[keyval].delete_fn (comm,
						  keyval +
						  MPI_MAX_KEY_DEFINED,
						  tmp_per_comm->key_vals[keyval].
						  attr,
						  tmp->attrs_fn[keyval].
						  extra_state);
	    }
	  else
	    {
          int fort_key = keyval + MPI_MAX_KEY_DEFINED;
          int *ext = (int *) (tmp->attrs_fn[keyval].extra_state);
	      ((MPI_Delete_function_fortran *) tmp->attrs_fn[keyval].
           delete_fn) (&comm, &fort_key, (int*)tmp_per_comm->key_vals[keyval].attr, ext, &res);
	    }
	}
    }

	if(res == MPI_SUCCESS)
	{
		tmp_per_comm->key_vals[keyval].attr = NULL;
		tmp_per_comm->key_vals[keyval].flag = 0;
	}
  sctk_spinlock_unlock(&(tmp_per_comm->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  return res;
}

static int
SCTK__MPI_Attr_clean_communicator (MPI_Comm comm)
{
  int res = MPI_SUCCESS;
  int i;
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm;


  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));
  tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
  sctk_spinlock_lock(&(tmp_per_comm->lock));

  for (i = 0; i < tmp_per_comm->max_number; i++)
    {
      if (tmp_per_comm->key_vals[i].flag == 1)
	{
	  sctk_spinlock_unlock(&(tmp_per_comm->lock));
	  sctk_spinlock_unlock(&(tmp->lock));
	  res = __INTERNAL__PMPI_Attr_delete (comm, i + MPI_MAX_KEY_DEFINED);
	  sctk_spinlock_lock(&(tmp->lock));
	  sctk_spinlock_lock(&(tmp_per_comm->lock));

	  if (res != MPI_SUCCESS)
	    {
	      sctk_spinlock_unlock(&(tmp_per_comm->lock));
	      sctk_spinlock_unlock(&(tmp->lock));
	      return res;
	    }

	}
    }
  sctk_spinlock_unlock(&(tmp_per_comm->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  return res;
}

static int
SCTK__MPI_Attr_communicator_dup (MPI_Comm old, MPI_Comm new)
{
  int res = MPI_SUCCESS;
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm_old;
  mpc_mpi_per_communicator_t* tmp_per_comm_new;
  int i;

  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));
  tmp_per_comm_old = mpc_mpc_get_per_comm_data(old);
  sctk_spinlock_lock(&(tmp_per_comm_old->lock));

  tmp_per_comm_new = mpc_mpc_get_per_comm_data(new);
  tmp_per_comm_new->key_vals = sctk_malloc(tmp_per_comm_old->max_number*sizeof(MPI_Caching_key_value_t));
  tmp_per_comm_new->max_number = tmp_per_comm_old->max_number;

  for(i = 0; i < tmp_per_comm_old->max_number; i++){
    tmp_per_comm_new->key_vals[i].flag = 0;
    tmp_per_comm_new->key_vals[i].attr = NULL;
  }

  for (i = 0; i < tmp_per_comm_old->max_number; i++)
    {
      if (tmp->attrs_fn[i].copy_fn != NULL)
	{
	  if (tmp_per_comm_old->key_vals[i].flag == 1)
	    {
	      void *arg;
	      int flag = 0;
	      MPI_Copy_function *cpy;

	      cpy = tmp->attrs_fn[i].copy_fn;

	      if (tmp->attrs_fn[i].fortran_key == 0)
		{
		  res =
		    cpy (old, i + MPI_MAX_KEY_DEFINED,
			 tmp->attrs_fn[i].extra_state,
			 tmp_per_comm_old->key_vals[i].attr, (void *) (&arg), &flag);
		}
	      else
		{
          int fort_key = i + MPI_MAX_KEY_DEFINED;
          int *ext = (int *) (tmp->attrs_fn[i].extra_state);
          int val = tmp_per_comm_old->key_vals[i].attr;
          ((MPI_Copy_function_fortran *) cpy) (&old, &fort_key, ext, &val, &arg, &flag, &res);
		}
	      sctk_nodebug ("i = %d Copy %d %ld->%ld flag %d", i, i + MPI_MAX_KEY_DEFINED,
			    (unsigned long) tmp_per_comm_old->key_vals[i].attr,
			    (unsigned long) arg, flag);
	      if (flag)
		{
			sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
			sctk_spinlock_unlock(&(tmp->lock));
			sctk_nodebug("arg = %d", *(((int *)arg)));
			__INTERNAL__PMPI_Attr_put (new, i + MPI_MAX_KEY_DEFINED, arg);
			sctk_spinlock_lock(&(tmp->lock));
			sctk_spinlock_lock(&(tmp_per_comm_old->lock));
		}
	      if (res != MPI_SUCCESS)
		{
		  sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
		  sctk_spinlock_unlock(&(tmp->lock));
		  return res;
		}

	    }
	}
    }
  sctk_spinlock_unlock(&(tmp_per_comm_old->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  return res;
}


static void __sctk_init_mpi_topo_per_comm (mpc_mpi_per_communicator_t* tmp){
  tmp->topo.type = MPI_UNDEFINED;
  sprintf (tmp->topo.names, "undefined");
}

static void
__sctk_init_mpi_topo ()
{
}

static int
SCTK__MPI_Comm_communicator_dup (MPI_Comm comm, MPI_Comm newcomm)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo_old;
  mpi_topology_per_comm_t* topo_new;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo_old = &(tmp->topo);
  tmp = mpc_mpc_get_per_comm_data(newcomm);
  topo_new = &(tmp->topo);

  sctk_spinlock_lock (&(topo_old->lock));

  if (topo_old->type == MPI_CART)
    {
      topo_new->type = MPI_CART;
      topo_new->data.cart.ndims =
	topo_old->data.cart.ndims;
      topo_new->data.cart.reorder =
	topo_old->data.cart.reorder;
      topo_new->data.cart.dims =
	sctk_malloc (topo_old->data.cart.ndims * sizeof (int));
      memcpy (topo_new->data.cart.dims,
	      topo_old->data.cart.dims,
	      topo_old->data.cart.ndims * sizeof (int));
      topo_new->data.cart.periods =
	sctk_malloc (topo_old->data.cart.ndims * sizeof (int));
      memcpy (topo_new->data.cart.periods,
	      topo_old->data.cart.periods,
	      topo_old->data.cart.ndims * sizeof (int));
    }

  if (topo_old->type == MPI_GRAPH)
    {
      topo_new->type = MPI_GRAPH;

      topo_new->data.graph.nnodes =
	topo_old->data.graph.nnodes;
      topo_new->data.graph.reorder =
	topo_old->data.graph.reorder;
      topo_new->data.graph.index =
	sctk_malloc (topo_old->data.graph.nnodes * sizeof (int));
      memcpy (topo_new->data.graph.index,
	      topo_old->data.graph.index,
	      topo_old->data.graph.nnodes * sizeof (int));
      topo_new->data.graph.edges =
	sctk_malloc (topo_old->data.graph.nnodes * sizeof (int));
      memcpy (topo_new->data.graph.edges,
	      topo_old->data.graph.edges,
	      topo_old->data.graph.nnodes * sizeof (int));

      topo_new->data.graph.nedges =
	topo_old->data.graph.nedges;
      topo_new->data.graph.nindex =
	topo_old->data.graph.nindex;
    }

  sctk_spinlock_unlock (&(topo_old->lock));

  return MPI_SUCCESS;
}

static int
SCTK__MPI_Comm_communicator_free (MPI_Comm comm)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type == MPI_CART)
    {
      sctk_free (topo->data.cart.dims);
      sctk_free (topo->data.cart.periods);
    }
  if (topo->type == MPI_GRAPH)
    {
      sctk_free (topo->data.graph.index);
      sctk_free (topo->data.graph.edges);
    }

  sctk_spinlock_unlock (&(topo->lock));

  sctk_free(tmp->key_vals);
  sctk_free(tmp);
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Comm_get_name (MPI_Comm comm, char *comm_name,
				int *resultlen)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int len;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));
  len = strlen (topo->names);

  memcpy (comm_name, topo->names, len + 1);

  *resultlen = len;
  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}
static int
__INTERNAL__PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int len;
  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));
  memset (topo->names, '\0', MPI_MAX_NAME_STRING + 1);

  len = strlen (comm_name);
  if (len > MPI_MAX_NAME_STRING)
    {
      len = MPI_MAX_NAME_STRING;
    }
  memcpy (topo->names, comm_name, len);

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	sctk_spinlock_lock (&(topo->lock));

	if((topo->type == MPI_CART) || (topo->type == MPI_GRAPH))
		*topo_type = topo->type;
	else
		*topo_type = MPI_UNDEFINED;


	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims,
			      int *periods, int reorder, MPI_Comm * comm_cart)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int res = MPI_ERR_INTERN;
  int nb_tasks = 1;
  int size;
  int i;
  int rank;

  __INTERNAL__PMPI_Comm_rank (comm_old, &rank);

  for (i = 0; i < ndims; i++)
    {
	  if(dims[i] <= 0)
		MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "One of the dimensions is equal or less than zero");
      nb_tasks *= dims[i];
      sctk_nodebug ("dims[%d] = %d", i, dims[i]);
    }

  __INTERNAL__PMPI_Comm_size (comm_old, &size);

  sctk_nodebug ("%d <= %d", nb_tasks, size);
  if (nb_tasks > size)
    {
      MPI_ERROR_REPORT (comm_old, MPI_ERR_COMM, "too small group size");
    }

INFO("Very simple approach never reorder nor take care of hardware topology")

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
      ranges[0][1] = size-1;
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

	if(*comm_cart != MPC_COMM_NULL)
	{
		tmp = mpc_mpc_get_per_comm_data(*comm_cart);
		topo = &(tmp->topo);
		sctk_spinlock_lock (&(topo->lock));
			topo->type = MPI_CART;
			topo->data.cart.ndims = ndims;
			topo->data.cart.reorder = reorder;
			topo->data.cart.dims = sctk_malloc (ndims * sizeof (int));
			memcpy (topo->data.cart.dims, dims, ndims * sizeof (int));
			topo->data.cart.periods = sctk_malloc (ndims * sizeof (int));
			memcpy (topo->data.cart.periods, periods, ndims * sizeof (int));
		sctk_spinlock_unlock (&(topo->lock));
	}
	return res;
}

/*
 *  assignnodes
 *
 *  Function:   - assign processes to dimensions
 *          - get "best-balanced" grid
 *          - greedy bin-packing algorithm used
 *          - sort dimensions in decreasing order
 *          - dimensions array dynamically allocated
 *  Accepts:    - # of dimensions
 *          - # of prime factors
 *          - array of prime factors
 *          - array of factor counts
 *          - ptr to array of dimensions (returned value)
 *  Returns:    - 0 or ERROR
 */
static int
assignnodes(int ndim, int nfactor, int *pfacts, int *counts, int **pdims)
{
    int *bins;
    int i, j;
    int n;
    int f;
    int *p;
    int *pmin;

    if (0 >= ndim) {
       return MPI_ERR_DIMS;
    }

    /* Allocate and initialize the bins */
    bins = (int *) sctk_malloc((unsigned) ndim * sizeof(int));
    if (NULL == bins) {
       return MPI_ERR_INTERN;
    }
    *pdims = bins;

    for (i = 0, p = bins; i < ndim; ++i, ++p) {
        *p = 1;
     }

    /* Loop assigning factors from the highest to the lowest */
    for (j = nfactor - 1; j >= 0; --j) {
       f = pfacts[j];
       for (n = counts[j]; n > 0; --n) {
            /* Assign a factor to the smallest bin */
            pmin = bins;
            for (i = 1, p = pmin + 1; i < ndim; ++i, ++p) {
                if (*p < *pmin) {
                    pmin = p;
                }
            }
            *pmin *= f;
        }
     }

     /* Sort dimensions in decreasing order (O(n^2) for now) */
     for (i = 0, pmin = bins; i < ndim - 1; ++i, ++pmin) {
         for (j = i + 1, p = pmin + 1; j < ndim; ++j, ++p) {
             if (*p > *pmin) {
                n = *p;
                *p = *pmin;
                *pmin = n;
             }
         }
     }

     return MPI_SUCCESS;
}

/*
 *  getfactors
 *
 *  Function:   - factorize a number
 *  Accepts:    - number
 *          - # of primes
 *          - array of primes
 *          - ptr to array of counts (returned value)
 *  Returns:    - 0 or ERROR
 */
static int
getfactors(int num, int nprime, int *primes, int **pcounts)
{
    int *counts;
    int i;
    int *p;
    int *c;

    if (0 >= nprime) {
        return MPI_ERR_INTERN;
    }

    /* Allocate the factor counts array */
    counts = (int *) sctk_malloc((unsigned) nprime * sizeof(int));
    if (NULL == counts) {
       return MPI_ERR_INTERN;
    }

    *pcounts = counts;

    /* Loop over the prime numbers */
    i = nprime - 1;
    p = primes + i;
    c = counts + i;

    for (; i >= 0; --i, --p, --c) {
        *c = 0;
        while ((num % *p) == 0) {
            ++(*c);
            num /= *p;
        }
    }

    if (1 != num) {
        return MPI_ERR_INTERN;
    }

    return MPI_SUCCESS;
}

/*
 *  getprimes
 *
 *  Function:   - get primes smaller than number
 *          - array of primes dynamically allocated
 *  Accepts:    - number
 *          - ptr # of primes (returned value)
 *          - ptr array of primes (returned values)
 *  Returns:    - 0 or ERROR
 */
static int
getprimes(int num, int *pnprime, int **pprimes) {

   int i, j;
   int n;
   int size;
   int *primes;

   /* Allocate the array of primes */
   size = (num / 2) + 1;
   primes = (int *) sctk_malloc((unsigned) size * sizeof(int));
   if (NULL == primes) {
       return MPI_ERR_INTERN;
   }
   *pprimes = primes;

   /* Find the prime numbers */
   i = 0;
   primes[i++] = 2;

   for (n = 3; n <= num; n += 2) {
      for (j = 1; j < i; ++j) {
         if ((n % primes[j]) == 0) {
             break;
          }
      }

      if (j == i) {
        if (i >= size) {
           return MPI_ERR_DIMS;
         }
         primes[i++] = n;
      }
   }

   *pnprime = i;
   return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
    int i;
    int freeprocs;
    int freedims;
    int nprimes;
    int *primes;
    int *factors;
    int *procs;
    int *p;
    int err;

	if (NULL == dims)
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid arg dims");

	if (1 > ndims)
		MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");

    /* Get # of free-to-be-assigned processes and # of free dimensions */
    freeprocs = nnodes;
    freedims = 0;
    for (i = 0, p = dims; i < ndims; ++i,++p)
    {
        if (*p == 0)
            ++freedims;
        else if ((*p < 0) || ((nnodes % *p) != 0))
            MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
        else
            freeprocs /= *p;
    }

    if (freedims == 0)
    {
		if (freeprocs == 1)
			return MPI_SUCCESS;
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
    }

    if (freeprocs < 1)
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
    else if (freeprocs == 1)
    {
        for (i = 0; i < ndims; ++i, ++dims)
        {
            if (*dims == 0)
               *dims = 1;
        }
        return MPI_SUCCESS;
    }

    /* Compute the relevant prime numbers for factoring */
    if (MPI_SUCCESS != (err = getprimes(freeprocs, &nprimes, &primes)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error getprimes");

    /* Factor the number of free processes */
    if (MPI_SUCCESS != (err = getfactors(freeprocs, nprimes, primes, &factors)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error getfactors");

    /* Assign free processes to free dimensions */
    if (MPI_SUCCESS != (err = assignnodes(freedims, nprimes, primes, factors, &procs)))
		MPI_ERROR_REPORT(MPI_COMM_WORLD, err, "Error assignnodes");

    /* Return assignment results */
    p = procs;
    for (i = 0; i < ndims; ++i, ++dims)
    {
        if (*dims == 0)
			*dims = *p++;
    }

    free((char *) primes);
    free((char *) factors);
    free((char *) procs);

	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index,
			       int *edges, int reorder, MPI_Comm * comm_graph)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int res;
  int size;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);

INFO("Very simple approach never reorder nor take care of hardware topology")
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
      ranges[0][1] = size-1;
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

	if(*comm_graph >= 0)
	{
		tmp = mpc_mpc_get_per_comm_data(*comm_graph);
		topo = &(tmp->topo);
		sctk_spinlock_lock (&(topo->lock));
			topo->type = MPI_GRAPH;
			topo->data.graph.nnodes = nnodes;
			topo->data.graph.reorder = reorder;
			topo->data.graph.index = sctk_malloc (nnodes * sizeof (int));
			memcpy (topo->data.graph.index, index, nnodes * sizeof (int));
			topo->data.graph.edges = sctk_malloc (index[nnodes - 1] * sizeof (int));
			memcpy (topo->data.graph.edges, edges, index[nnodes - 1] * sizeof (int));
			topo->data.graph.nedges = index[nnodes - 1];
			topo->data.graph.nindex = nnodes;
		sctk_spinlock_unlock (&(topo->lock));
	}
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *nnodes = topo->data.graph.nnodes;
  *nedges = topo->data.graph.nedges;
  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
			    int *index, int *edges)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;
  int i;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_GRAPH)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  for (i = 0; (i < maxindex) && (i < topo->data.graph.nindex);
       i++)
    {
      index[i] = topo->data.graph.index[i];
    }
  for (i = 0; (i < maxedges) && (i < topo->data.graph.nedges);
       i++)
    {
      edges[i] = topo->data.graph.edges[i];
    }

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  if (topo->type != MPI_CART)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  *ndims = topo->data.cart.ndims;

  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords)
{
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	int res;
	int rank;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);

	sctk_spinlock_lock (&(topo->lock));
		if (topo->type != MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}

		if (maxdims != topo->data.cart.ndims)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
		}

		memcpy (dims, topo->data.cart.dims, maxdims * sizeof (int));
		memcpy (periods, topo->data.cart.periods, maxdims * sizeof (int));

		res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
	sctk_spinlock_unlock (&(topo->lock));
	return res;
}

static int
__INTERNAL__PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank, int locked)
{
	int loc_rank = 0;
	int dims_coef = 1;
	int i;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	if (!locked)
		sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_CART)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	for (i = topo->data.cart.ndims - 1 ; i >= 0 ; i--)
	{
		loc_rank += coords[i] * dims_coef ;
		dims_coef *= topo->data.cart.dims[i] ;
	}

	*rank = loc_rank;
	sctk_nodebug ("rank %d", loc_rank);
	if (!locked)
		sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_coords (MPI_Comm comm, int init_rank, int maxdims,
			      int *coords, int locked)
{
	int i;
	int pi = 1;
	int qi = 1;
	int rank;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	rank = init_rank;

	if (!locked)
		sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_CART)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	if (maxdims != topo->data.cart.ndims)
	{
		if (!locked)
			sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
	}

	for (i = maxdims-1; i >= 0; --i)
	{
		pi *= topo->data.cart.dims[i];
		coords[i] = ( rank % pi ) / qi ;
		qi = pi ;
	}

	if (!locked)
		sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors_count (MPI_Comm comm, int rank,
					int *nneighbors)
{
	int start;
	int end;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_GRAPH)
    {
		sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

	end = topo->data.graph.index[rank];
	start = 0;
	if (rank)
    {
		start = topo->data.graph.index[rank - 1];
    }

	*nneighbors = end - start;


	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
				  int *neighbors)
{
	int start;
	int i;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);

	sctk_spinlock_lock (&(topo->lock));

	if (topo->type != MPI_GRAPH)
    {
		sctk_spinlock_unlock (&(topo->lock));
		MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

	start = 0;
	if (rank)
    {
		start = topo->data.graph.index[rank - 1];
    }

	for (i = 0; i < maxneighbors && i + start < topo->data.graph.nedges; i++)
    {
		neighbors[i] = topo->data.graph.edges[i + start];
    }

	sctk_spinlock_unlock (&(topo->lock));
	return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_shift (MPI_Comm comm, int direction, int displ,
			     int *source, int *dest)
{
  int *coords;
  int res;
  int rank;
  int maxdims;
  int new_val;
  mpc_mpi_per_communicator_t* tmp;
  mpi_topology_per_comm_t* topo;

  tmp = mpc_mpc_get_per_comm_data(comm);
  topo = &(tmp->topo);

  sctk_spinlock_lock (&(topo->lock));

  __INTERNAL__PMPI_Comm_rank (comm, &rank);

  if (topo->type != MPI_CART)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
    }

  if (direction >= topo->data.cart.ndims)
    {
      sctk_spinlock_unlock (&(topo->lock));
      MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid direction");
    }

  maxdims = topo->data.cart.ndims;

  coords = sctk_malloc (maxdims * sizeof (int));

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  coords[direction] = coords[direction] + displ;

  sctk_nodebug ("New val + %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] += topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   topo->data.cart.dims[direction])
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] -= topo->data.cart.dims[direction];
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
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  *dest = new_val;

  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 1);
  if (res != MPI_SUCCESS)
    {
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  coords[direction] = coords[direction] - displ;

  sctk_nodebug ("New val - %d", coords[direction]);
  if (coords[direction] < 0)
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] += topo->data.cart.dims[direction];
	  res = __INTERNAL__PMPI_Cart_rank (comm, coords, &new_val, 1);
	}
      else
	{
	  new_val = MPI_PROC_NULL;
	}
    }
  else if (coords[direction] >=
	   topo->data.cart.dims[direction])
    {
      if (topo->data.cart.periods[direction])
	{
	  coords[direction] -= topo->data.cart.dims[direction];
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
      sctk_spinlock_unlock (&(topo->lock));
      return res;
    }

  *source = new_val;

  sctk_free (coords);

  sctk_nodebug ("dest %d src %d rank %d dir %d", *dest, *source, rank,
		direction);
  sctk_spinlock_unlock (&(topo->lock));
  return MPI_SUCCESS;
}

static int
__INTERNAL__PMPI_Cart_sub (MPI_Comm comm, int *remain_dims,
			   MPI_Comm * comm_new)
{
	int res;
	int nb_comm;
	int i;
	int *coords_in_new;
	int color;
	int rank;
	int size;
	int ndims = 0;
	int j;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	mpi_topology_per_comm_t* topo_new;

	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	sctk_spinlock_lock (&(topo->lock));


		if (topo->type != MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}

		coords_in_new = malloc (topo->data.cart.ndims * sizeof (int));

		res = __INTERNAL__PMPI_Cart_coords (comm, rank, topo->data.cart.ndims, coords_in_new, 1);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}

		nb_comm = 1;
		for (i = 0; i < topo->data.cart.ndims; i++)
		{
			sctk_nodebug ("Remain %d %d", i, remain_dims[i]);
			if (remain_dims[i] == 0)
			{
				nb_comm *= topo->data.cart.dims[i];
			}
			else
			{
				ndims++;
				coords_in_new[i] = 0;
			}
		}

		sctk_nodebug ("%d new comms", nb_comm);

		res = __INTERNAL__PMPI_Cart_rank (comm, coords_in_new, &color, 1);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}

		sctk_nodebug ("%d color", color);
		res = __INTERNAL__PMPI_Comm_split (comm, color, rank, comm_new);
		if (res != MPI_SUCCESS)
		{
			sctk_spinlock_unlock (&(topo->lock));
			return res;
		}

		sctk_free (coords_in_new);

		tmp = mpc_mpc_get_per_comm_data(*comm_new);
		topo_new = &(tmp->topo);

		topo_new->type = MPI_CART;
		topo_new->data.cart.ndims = ndims;
		topo_new->data.cart.reorder = 0;
		topo_new->lock = SCTK_SPINLOCK_INITIALIZER;
		topo_new->data.cart.dims = sctk_malloc (ndims * sizeof (int));
		topo_new->data.cart.periods = sctk_malloc (ndims * sizeof (int));

		j = 0;
		for (i = 0; i < topo->data.cart.ndims; i++)
		{
			if (remain_dims[i])
			{
				topo_new->data.cart.dims[j] = topo->data.cart.dims[i];
				topo_new->data.cart.periods[j] = topo->data.cart.periods[i];
				j++;
			}
		}

		__INTERNAL__PMPI_Comm_size (*comm_new, &size);
		__INTERNAL__PMPI_Comm_rank (*comm_new, &rank);
		sctk_nodebug ("%d on %d new rank", rank, size);
	sctk_spinlock_unlock (&(topo->lock));
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

TODO("Should be optimized")
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
TODO("Should be optimized")
  __INTERNAL__PMPI_Comm_rank (comm, newrank);

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
  mpc_mpi_data_t* tmp;
  int i;
  int found = 0;

  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));

  for (i = 0; i < tmp->user_func_nb; i++)
    {
      if (tmp->user_func[i].status == 0)
	{
	  found = 1;
	  break;
	}
    }

  if (found == 0)
    {
      tmp->user_func_nb++;
      tmp->user_func =
	sctk_realloc (tmp->user_func,
		      tmp->user_func_nb *
		      sizeof (MPI_Handler_user_function_t));
    }


  tmp->user_func[i].func = function;
  tmp->user_func[i].status = 1;
  *errhandler = i;

  sctk_spinlock_unlock(&(tmp->lock));
  MPI_ERROR_SUCESS ();
}

void
MPI_Default_error (MPI_Comm * comm, int *error, char *msg, char *file,
		   int line)
{
/*   char str[1024]; */
/*   int i; */
/*   __INTERNAL__PMPI_Error_string (*error, str, &i); */
/*   if (i != 0) */
/*     sctk_error ("%s in file %s at line %d %s", str, file, line, msg); */
/*   else */
/*     sctk_error ("Unknown error"); */
  /* The lib is not supposed to crash but deliver message */
/*   __INTERNAL__PMPI_Abort (*comm, *error); */
}

void
MPI_Abort_error (MPI_Comm * comm, int *error, char *msg, char *file,
		   int line)
{
  char str[1024];
  int i;
  __INTERNAL__PMPI_Error_string (*error, str, &i);
  if (i != 0)
    sctk_error ("%s in file %s at line %d %s", str, file, line, msg);
  else
    sctk_error ("Unknown error");
  /* The lib is not supposed to crash but deliver message */
  __INTERNAL__PMPI_Abort (*comm, *error);
}

void
MPI_Return_error (MPI_Comm * comm, int *error, ...)
{
}


static int
__INTERNAL__PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm;

  if(MPI_ERRHANDLER_NULL == errhandler){
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid errhandler");
  }

  tmp = mpc_mpc_get_per_task_data();

  sctk_spinlock_lock(&(tmp->lock));
  if ((errhandler < 0) || (errhandler >= tmp->user_func_nb))
    {
      sctk_spinlock_unlock(&(tmp->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }


  if (tmp->user_func[errhandler].status == 0)
    {
      sctk_spinlock_unlock(&(tmp->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

  tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
  sctk_spinlock_lock(&(tmp_per_comm->lock));

  tmp_per_comm->func = tmp->user_func[errhandler].func;
  tmp_per_comm->func_ident = errhandler;

  sctk_spinlock_unlock(&(tmp_per_comm->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  mpc_mpi_data_t* tmp;
  mpc_mpi_per_communicator_t* tmp_per_comm;

  tmp = mpc_mpc_get_per_task_data();

  sctk_spinlock_lock(&(tmp->lock));
  tmp_per_comm = mpc_mpc_get_per_comm_data(comm);
  sctk_spinlock_lock(&(tmp_per_comm->lock));

  *errhandler = tmp_per_comm->func_ident;

  sctk_spinlock_unlock(&(tmp_per_comm->lock));
  sctk_spinlock_unlock(&(tmp->lock));
  MPI_ERROR_SUCESS ();
}

static int
__INTERNAL__PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  mpc_mpi_data_t* tmp;

  if(MPI_ERRHANDLER_NULL == *errhandler){
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid errhandler");
  }

  tmp = mpc_mpc_get_per_task_data();
  sctk_spinlock_lock(&(tmp->lock));

  if ((*errhandler < 0) || (*errhandler >= tmp->user_func_nb))
    {
      sctk_spinlock_unlock(&(tmp->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

  if (tmp->user_func[*errhandler].status == 0)
    {
      sctk_spinlock_unlock(&(tmp->lock));
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "Invalid errhandler");
    }

  tmp->user_func[*errhandler].status--;

  if(tmp->user_func[*errhandler].status <= 0){
    tmp->user_func[*errhandler].status = 0;
    tmp->user_func[*errhandler].func = NULL;
  }


  *errhandler = MPI_ERRHANDLER_NULL;
  sctk_spinlock_unlock(&(tmp->lock));
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
			MPI_Error_string_convert (MPI_ERR_BUFFER , "Invalid buffer pointer");
			MPI_Error_string_convert (MPI_ERR_COUNT , "Invalid count argument");
			MPI_Error_string_convert (MPI_ERR_TYPE , "Invalid datatype argument");
			MPI_Error_string_convert (MPI_ERR_TAG , "Invalid tag argument");
			MPI_Error_string_convert (MPI_ERR_COMM , "Invalid communicator");
			MPI_Error_string_convert (MPI_ERR_RANK , "Invalid rank");
      MPI_Error_string_convert (MPI_ERR_REQUEST, "Invalid mpi_request handle");
			MPI_Error_string_convert (MPI_ERR_ROOT , "Invalid root");
			MPI_Error_string_convert (MPI_ERR_GROUP , "Invalid group ");
			MPI_Error_string_convert (MPI_ERR_OP , "Invalid operation");
			MPI_Error_string_convert (MPI_ERR_TOPOLOGY , "Invalid topology");
			MPI_Error_string_convert (MPI_ERR_DIMS , "Invalid dimension argument");
			MPI_Error_string_convert (MPI_ERR_ARG , "Invalid argument of some other kind");
			MPI_Error_string_convert (MPI_ERR_UNKNOWN , "Unknown error");
			MPI_Error_string_convert (MPI_ERR_TRUNCATE , "Message truncated on receive");
      MPI_Error_string_convert (MPI_ERR_OTHER, "Other error; use Error_string");
			MPI_Error_string_convert (MPI_ERR_INTERN , "Internal MPI (implementation) error");
			MPI_Error_string_convert (MPI_ERR_IN_STATUS , "Error code in status");
			MPI_Error_string_convert (MPI_ERR_PENDING , "Pending request");
			MPI_Error_string_convert (MPI_ERR_KEYVAL , "Invalid keyval has been passed");
			MPI_Error_string_convert (MPI_ERR_NO_MEM , "MPI_ALLOC_MEM failed because memory is exhausted");
			MPI_Error_string_convert (MPI_ERR_BASE , "Invalid base passed to MPI_FREE_MEM");
			MPI_Error_string_convert (MPI_ERR_INFO_KEY , "Key longer than MPI_MAX_INFO_KEY");
			MPI_Error_string_convert (MPI_ERR_INFO_VALUE , "Value longer than MPI_MAX_INFO_VAL");
			MPI_Error_string_convert (MPI_ERR_INFO_NOKEY , "Invalid key passed to MPI_INFO_DELETE");
			MPI_Error_string_convert (MPI_ERR_SPAWN , "Error in spawning processes");
			MPI_Error_string_convert (MPI_ERR_PORT , "Ivalid port name passed to MPI_COMM_CONNECT");
			MPI_Error_string_convert (MPI_ERR_SERVICE , "Invalid service name passed to MPI_UNPUBLISH_NAME");
			MPI_Error_string_convert (MPI_ERR_NAME , "Invalid service name passed to MPI_LOOKUP_NAME");
			MPI_Error_string_convert (MPI_ERR_WIN , "Invalid win argument");
			MPI_Error_string_convert (MPI_ERR_SIZE , "Invalid size argument");
			MPI_Error_string_convert (MPI_ERR_DISP , "Invalid disp argument");
			MPI_Error_string_convert (MPI_ERR_INFO , "Invalid info argument");
			MPI_Error_string_convert (MPI_ERR_LOCKTYPE , "Invalid locktype argument");
			MPI_Error_string_convert (MPI_ERR_ASSERT , "Invalid assert argument");
			MPI_Error_string_convert (MPI_ERR_RMA_CONFLICT , "Conflicting accesses to window");
			MPI_Error_string_convert (MPI_ERR_RMA_SYNC , "Wrong synchronization of RMA calls");
			MPI_Error_string_convert (MPI_ERR_FILE , "Invalid file handle");
			MPI_Error_string_convert (MPI_ERR_NOT_SAME , "Collective argument not identical on all processes, \
				or collective routines called in a different order by	different processes");
			MPI_Error_string_convert (MPI_ERR_AMODE , "Error related to the amode passed to MPI_FILE_OPEN");
			MPI_Error_string_convert (MPI_ERR_UNSUPPORTED_DATAREP , "Unsupported datarep passed to MPI_FILE_SET_VIEW");
			MPI_Error_string_convert (MPI_ERR_UNSUPPORTED_OPERATION , "Unsupported operation, \
				such as seeking on a file which	supports sequential access only");
			MPI_Error_string_convert (MPI_ERR_NO_SUCH_FILE , "File does not exist");
			MPI_Error_string_convert (MPI_ERR_FILE_EXISTS , "File exists");
			MPI_Error_string_convert (MPI_ERR_BAD_FILE , "Invalid file name (eg, path name too long");
			MPI_Error_string_convert (MPI_ERR_ACCESS , "Permission denied");
			MPI_Error_string_convert (MPI_ERR_NO_SPACE , "Not enough space");
			MPI_Error_string_convert (MPI_ERR_QUOTA , "Quota exceeded");
			MPI_Error_string_convert (MPI_ERR_READ_ONLY , "Read-only file or file system");
			MPI_Error_string_convert (MPI_ERR_FILE_IN_USE , "File operation could not be completed, \
				as the file is currently open by some process");
			MPI_Error_string_convert (MPI_ERR_DUP_DATAREP , "Conversion functions could not be registered because \
				a data representation identifier that was already defined	was passed to MPI_REGISTER_DATAREP");
			MPI_Error_string_convert (MPI_ERR_CONVERSION , "An error occurred in a user supplied data conversion function");
			MPI_Error_string_convert (MPI_ERR_IO , "Other I/O error");
			MPI_Error_string_convert (MPI_ERR_LASTCODE , "Last error code");
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
__INTERNAL__PMPI_Query_thread (int *provided)
{
  return PMPC_Query_thread(provided);
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
  int flag; 
  int flag_finalized; 
  __INTERNAL__PMPI_Initialized(&flag);
  __INTERNAL__PMPI_Finalized(&flag_finalized);
  if((flag != 0) || (flag_finalized != 0)){
    MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Init issue");
  } else {
    int rank;
    sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
    struct sctk_task_specific_s * task_specific;
    mpc_per_communicator_t*per_communicator;
    mpc_per_communicator_t*per_communicator_tmp;
    res = PMPC_Init (argc, argv);
    if (res != MPI_SUCCESS)
      {
	return res;
      }
    is_initialized = 1;

    task_specific = __MPC_get_task_specific ();
    task_specific->mpc_mpi_data = malloc(sizeof(struct mpc_mpi_data_s));
    memset(task_specific->mpc_mpi_data,0,sizeof(struct mpc_mpi_data_s));
    task_specific->mpc_mpi_data->lock = lock;
    task_specific->mpc_mpi_data->requests = NULL;
    task_specific->mpc_mpi_data->groups = NULL;
    task_specific->mpc_mpi_data->buffers = NULL;
    task_specific->mpc_mpi_data->ops = NULL;

    __sctk_init_mpc_request ();
    __sctk_init_mpi_buffer ();
    __sctk_init_mpi_errors ();
    __sctk_init_mpi_topo ();
    __sctk_init_mpi_op ();
    __sctk_init_mpc_group ();

    sctk_spinlock_lock(&(task_specific->per_communicator_lock));
    HASH_ITER(hh,task_specific->per_communicator,per_communicator,per_communicator_tmp)
      {
	mpc_mpi_per_communicator_t* tmp;
	per_communicator->mpc_mpi_per_communicator = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s));
	memset(per_communicator->mpc_mpi_per_communicator,0,sizeof(struct mpc_mpi_per_communicator_s));
	per_communicator->mpc_mpi_per_communicator_copy = mpc_mpi_per_communicator_copy_func;
	per_communicator->mpc_mpi_per_communicator_copy_dup = mpc_mpi_per_communicator_dup_copy_func;
	per_communicator->mpc_mpi_per_communicator->lock = lock;

	tmp = per_communicator->mpc_mpi_per_communicator;

	__sctk_init_mpi_errors_per_comm (tmp);
	__sctk_init_mpi_topo_per_comm (tmp);
	tmp->max_number = 0;
	tmp->topo.lock = lock;
      }
    sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

    __INTERNAL__PMPI_Comm_rank (MPI_COMM_WORLD, &rank);
    __INTERNAL__PMPI_Barrier (MPI_COMM_WORLD);
  }
  return res;
}

static int
__INTERNAL__PMPI_Finalize (void)
{
  int res; 
  if(is_finalized != 0){
    MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Finalize issue");
  }
  res = PMPC_Finalize ();
  is_finalized = 1;
  return res;
}

static int
__INTERNAL__PMPI_Finalized (int* flag)
{
  *flag = is_finalized;
  return MPI_SUCCESS;
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

/*
 * TODO: RULE FROM THE MPI STANDARD
 * In order to fully support the MPI standard, we cannot fail if the returned value of
 * an MPI function is different than MPI_SUCCESS.
 * We could add an additional MPC mode in order to fail in the case of a wrong returned value.
 */
#define SCTK_MPI_CHECK_RETURN_VAL(res,comm)do{if(res == MPI_SUCCESS){return res;} else {MPI_ERROR_REPORT(comm,res,"Generic error retrun");}}while(0)
//~ #define SCTK_MPI_CHECK_RETURN_VAL(res, comm) return res

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
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Send (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag,
	   MPI_Comm comm, MPI_Status * status)
{
	sctk_nodebug("MPI_Recv count %d, datatype %d, source %d, tag %d, comm %d", count, datatype, source, tag, comm);
	int res = MPI_ERR_INTERN;
	if(source == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_Status empty_status;
		empty_status.MPC_SOURCE = MPI_PROC_NULL;
		empty_status.MPC_TAG = MPI_ANY_TAG;
		empty_status.MPC_ERROR = MPI_SUCCESS;
		empty_status.cancelled = 0;
		empty_status.size = 0;
		*status = empty_status;
		SCTK_MPI_CHECK_RETURN_VAL (res, comm);
	}
	SCTK__MPI_INIT_STATUS (status);
	{
		int size;
		mpi_check_comm (comm, comm);
		mpi_check_type (datatype, comm);
		mpi_check_count (count, comm);
		sctk_nodebug ("tag %d", tag);
		mpi_check_tag (tag, comm);
		__INTERNAL__PMPI_Comm_size (comm, &size);
		if(sctk_is_inter_comm(comm) == 0)
		{
			mpi_check_rank (source, size, comm);
		}
		//~ if(sctk_is_inter_comm(comm))
		//~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
		//~ int remote_size;
		//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
		//~ mpi_check_rank (source, remote_size, comm);
		//~ }
		//~ else
		//~ {
		//~ mpi_check_rank (source, size, comm);
		//~ }
		//~ }
		//~ else
		//~ mpi_check_rank (source, size, comm);
		if (count != 0)
		{
			mpi_check_buf (buf, comm);
		}
	}
	
	res = __INTERNAL__PMPI_Recv (buf, count, datatype, source, tag, comm, status);

	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Get_count (MPI_Status * status, MPI_Datatype datatype, int *count)
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
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Bsend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Ssend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }
  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res = __INTERNAL__PMPI_Rsend (buf, count, datatype, dest, tag, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Buffer_attach (void *buffer, int size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_attach (buffer, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Buffer_detach (void *buffer, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Buffer_detach (buffer, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	    MPI_Comm comm, MPI_Request * request)
{
	sctk_nodebug("Isend");
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  res =
    __INTERNAL__PMPI_Isend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Ibsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	  res = MPI_SUCCESS;
	  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ibsend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	  res = MPI_SUCCESS;
	  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Issend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Irsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(dest == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag_send (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irsend (buf, count, datatype, dest, tag, comm, request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
	    int tag, MPI_Comm comm, MPI_Request * request)
{
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  if(source == MPC_PROC_NULL)
  {
	res = MPI_SUCCESS;
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
  }

  {
    int size;
    mpi_check_comm (comm, comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);
    sctk_nodebug ("tag %d", tag);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (source, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (source, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (source, size, comm);

    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Irecv (buf, count, datatype, source, tag, comm, request);

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Wait (MPI_Request * request, MPI_Status * status)
{
	sctk_nodebug("entering MPI_Wait request = %d", *request);
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;

	if(*request == MPI_REQUEST_NULL)
	{
		res = MPI_SUCCESS;
		status->MPC_SOURCE = MPI_PROC_NULL;
		status->MPC_TAG = MPI_ANY_TAG;
		status->MPC_ERROR = MPI_SUCCESS;
		status->size = 0;
		SCTK_MPI_CHECK_RETURN_VAL (res, comm);
	}

	res = __INTERNAL__PMPI_Wait (request, status);
	
	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Test (MPI_Request * request, int *flag, MPI_Status * status)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	if(*request == MPI_REQUEST_NULL)
	{
		res = MPI_SUCCESS;
		status->MPC_SOURCE = MPI_PROC_NULL;
		status->MPC_TAG = MPI_ANY_TAG;
		status->MPC_ERROR = MPI_SUCCESS;
		status->size = 0;
		*flag = 1;
		return res;
	}
	
	res = __INTERNAL__PMPI_Test (request, flag, status);
	
	if(status != MPI_STATUS_IGNORE)
	{
		if((status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING))
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Request_free (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_SUCCESS;
  if (NULL == request) {
    sctk_nodebug("request NULL");
    res = MPI_ERR_REQUEST;
  } else if(MPI_REQUEST_NULL == *request) {
	sctk_nodebug("request MPI_REQUEST_NULL");
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);  
  } else
	res = __INTERNAL__PMPI_Request_free (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Waitany (int count, MPI_Request array_of_requests[], int *index,
	      MPI_Status * status)
{
	sctk_nodebug("entering PMPI_Waitany");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Waitany (count, array_of_requests, index, status);
  if(status != MPI_STATUS_IGNORE){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = status->MPI_ERROR;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Testany (int count, MPI_Request array_of_requests[], int *index,
	      int *flag, MPI_Status * status)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res =
    __INTERNAL__PMPI_Testany (count, array_of_requests, index, flag, status);
  if(status != MPI_STATUS_IGNORE){
    if((status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING)){
      res = status->MPI_ERROR;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Waitall (int count, MPI_Request array_of_requests[],
	      MPI_Status array_of_statuses[])
{
	sctk_nodebug("entering PMPI_Waitall");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  res =
    __INTERNAL__PMPI_Waitall (count, array_of_requests, array_of_statuses);

  if(array_of_statuses != MPI_STATUSES_IGNORE){
    int i;
    for(i = 0; i < count; i++){
      if(array_of_statuses[i].MPI_ERROR != MPI_SUCCESS){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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

  if(array_of_statuses != MPI_STATUSES_IGNORE){
    int i;
    for(i =0; i < count; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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

  if((array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED)){
    int i;
    for(i =0; i < *outcount; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Testsome (int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int index;
	int res = MPI_ERR_INTERN;
	if ((array_of_requests == NULL) && (incount != 0))
	{
		res = MPI_ERR_REQUEST;
	}
/* 	else */
/* 	{ */
/* 		for (index = 0; index < incount; ++index) */
/* 		{ */
/* 			if (array_of_requests[index] == -1) */
/* 			{ */
/* 				res = MPI_ERR_REQUEST; */
/* 				break; */
/* 			} */
/* 		} */
/* 	} */
	if (((outcount == NULL || array_of_indices == NULL) && incount > 0) || incount < 0)
	{
		return MPI_ERR_ARG;
	}
	res = __INTERNAL__PMPI_Testsome (incount, array_of_requests, outcount, array_of_indices, array_of_statuses);

	if((array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED)){
    int i;
    for(i =0; i < *outcount; i++){
      if((array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING)){
	res = MPI_ERR_IN_STATUS;
      }
    }
  }

	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag,
	     MPI_Status * status)
{
    int size;
  int res = MPI_ERR_INTERN;
	mpi_check_comm(comm, comm);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
  res = __INTERNAL__PMPI_Iprobe (source, tag, comm, flag, status);
/*   if((status != MPI_STATUS_IGNORE) && (*flag != 0)){ */
/*     if(status->MPI_ERROR != MPI_SUCCESS){ */
/*       res = MPI_ERR_IN_STATUS; */
/*     } */
/*   }  */

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int size;
  int res = MPI_ERR_INTERN;
	mpi_check_comm(comm, comm);
    mpi_check_tag (tag, comm);
    __INTERNAL__PMPI_Comm_size (comm, &size);
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
  res = __INTERNAL__PMPI_Probe (source, tag, comm, status);
/*   if(status != MPI_STATUS_IGNORE){ */
/*     if(status->MPI_ERROR != MPI_SUCCESS){ */
/*       res = MPI_ERR_IN_STATUS; */
/*     } */
/*   }  */

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cancel (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(MPI_REQUEST_NULL == *request) {
        sctk_nodebug("request MPI_REQUEST_NULL");
        SCTK_MPI_CHECK_RETURN_VAL (MPI_ERR_REQUEST, comm);
  }
  res = __INTERNAL__PMPI_Cancel (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Send_init (buf, count, datatype, dest, tag, comm,
				request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Bsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Ssend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank_send (dest, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (dest, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (dest, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (dest, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Rsend_init (buf, count, datatype, dest, tag, comm,
				 request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
    if(sctk_is_inter_comm(comm) == 0){
      mpi_check_rank (source, size, comm);
    }
    //~ if(sctk_is_inter_comm(comm))
    //~ {
		//~ if(sctk_is_in_local_group(comm))
		//~ {
			//~ int remote_size;
			//~ __INTERNAL__PMPI_Comm_remote_size(comm, &remote_size);
			//~ mpi_check_rank (source, remote_size, comm);
		//~ }
		//~ else
		//~ {
			//~ mpi_check_rank (source, size, comm);
		//~ }
	//~ }
	//~ else
		//~ mpi_check_rank (source, size, comm);
    if (count != 0)
      {
	mpi_check_buf (buf, comm);
      }
  }
  res =
    __INTERNAL__PMPI_Recv_init (buf, count, datatype, source, tag, comm,
				request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Start (MPI_Request * request)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if ((request == NULL) || (*request == MPI_REQUEST_NULL)) {
    MPI_ERROR_REPORT(comm,MPI_ERR_REQUEST,"");
  }
  res = __INTERNAL__PMPI_Start (request);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Startall (int count, MPI_Request array_of_requests[])
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int i; 
  for(i = 0 ; i < count ; i++){
  if (array_of_requests[i] == MPI_REQUEST_NULL) {
	MPI_ERROR_REPORT(comm,MPI_ERR_REQUEST,"");    
  }
  }
  res = __INTERNAL__PMPI_Startall (count, array_of_requests);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Sendrecv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       int dest, int sendtag,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       int source, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  int size;

  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);

  mpi_check_type (sendtype, comm);
  mpi_check_type (recvtype, comm);
  mpi_check_count (sendcount, comm);
  mpi_check_count (recvcount, comm);
  mpi_check_tag_send (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  mpi_check_rank_send(dest,size,comm);
  mpi_check_rank(source,size,comm);

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
  if(status != MPI_STATUS_IGNORE){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = MPI_ERR_IN_STATUS;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Sendrecv_replace (void *buf, int count, MPI_Datatype datatype,
		       int dest, int sendtag, int source, int recvtag,
		       MPI_Comm comm, MPI_Status * status)
{
  int res = MPI_ERR_INTERN;
  int size;
  mpi_check_comm (comm, comm);
  mpi_check_type (datatype, comm);
  mpi_check_count (count, comm);
  mpi_check_tag_send (sendtag, comm);
  mpi_check_tag (recvtag, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  mpi_check_rank_send(dest,size,comm);
  mpi_check_rank(source,size,comm);
  if (count != 0)
    {
      mpi_check_buf (buf, comm);
    }
  res =
    __INTERNAL__PMPI_Sendrecv_replace (buf, count, datatype, dest, sendtag,
				       source, recvtag, comm, status);
  if(status != MPI_STATUS_IGNORE){
    if(status->MPI_ERROR != MPI_SUCCESS){
      res = MPI_ERR_IN_STATUS;
    }
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count )
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_elements(status, datatype, count );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_elements_x(status, datatype, count );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Status_set_cancelled (MPI_Status *status, int cancelled)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Status_set_cancelled (status, cancelled);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Request_get_status (MPI_Request request, int *flag, MPI_Status *status)
{
	MPC_Request *mpc_request =  __sctk_convert_mpc_request(&request);
	
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = PMPC_Request_get_status(*mpc_request, flag, status);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Test_cancelled (MPI_Status * status, int *flag)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Test_cancelled (status, flag);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/************************************************************************/
/* MPI Datatype Interface                                               */
/************************************************************************/

int PMPI_Type_contiguous (int count, MPI_Datatype old_type, MPI_Datatype * new_type_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*new_type_p = MPC_DATATYPE_NULL;
	
	res = __INTERNAL__PMPI_Type_contiguous (count, old_type, new_type_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_vector (int count, int blocklength, int stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;
	
	if(blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	mpi_check_count(count,comm);
	
	res = __INTERNAL__PMPI_Type_vector (count, blocklength, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_hvector (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;

	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	res = __INTERNAL__PMPI_Type_hvector (count, blocklen, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_hvector (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype * newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	mpi_check_count(count,comm);
	
	*newtype_p = MPC_DATATYPE_NULL;
	
	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}
	
	res = __INTERNAL__PMPI_Type_hvector (count, blocklen, stride, old_type, newtype_p);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_indexed (int count, int blocklens[], int indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}
	
	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}

	mpi_check_type_create(old_type,comm);
	res =  __INTERNAL__PMPI_Type_indexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_hindexed (int count, int blocklens[],  MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}
	
	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}
	
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int PMPI_Type_create_indexed_block(int count, int blocklength, int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;
	
	if(indices == NULL)
	{
		return MPI_SUCCESS;
	}
	
	if( blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}

	
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_indexed_block(count, blocklength, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);	
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	
	*newtype = MPC_DATATYPE_NULL;
	
	if(indices == NULL)
	{
		return MPI_SUCCESS;
	}
	
	if( blocklength < 0)
	{
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
	}

	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_hindexed_block(count, blocklength, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);	
}

int PMPI_Type_create_hindexed (int count, int blocklens[], MPI_Aint indices[],  MPI_Datatype old_type, MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);

	*newtype = MPC_DATATYPE_NULL;
	
	if((blocklens == NULL) || (indices == NULL))
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_hindexed (count, blocklens, indices, old_type, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_struct (int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);
	
	*newtype = MPC_DATATYPE_NULL;

	if((old_types == NULL) || (indices == NULL) || (blocklens == NULL))
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		mpi_check_type_create(old_types[i],comm); 
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Error negative block lengths provided");
		}
	}

	res = __INTERNAL__PMPI_Type_struct (count, blocklens, indices, old_types, newtype);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_struct (int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype * newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	int i;
	mpi_check_count(count,comm);

	*newtype = MPC_DATATYPE_NULL;
	
	if((old_types == NULL) || (indices == NULL) || (blocklens == NULL))
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		mpi_check_type_create(old_types[i],comm); 
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"Bad datatype provided to PMPI_Type_create_struct");
		} 
	}

	res = __INTERNAL__PMPI_Type_struct (count, blocklens, indices, old_types, newtype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Address (void *location, MPI_Aint * address)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Address (location, address);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Get_address( void *location, MPI_Aint *address)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	res = __INTERNAL__PMPI_Address (location, address);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

  /* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
int PMPI_Type_extent (MPI_Datatype datatype, MPI_Aint * extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_extent (datatype, extent);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	
	__INTERNAL__PMPI_Type_lb (datatype, lb);
	res = __INTERNAL__PMPI_Type_extent (datatype, extent);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent)
{
	return PMPI_Type_get_extent( datatype, lb, extent );
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent)
{
	return PMPC_Type_get_true_extent( datatype, true_lb, true_extent );
}

int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent)
{
	return PMPC_Type_get_true_extent( datatype, true_lb, true_extent );
}

int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner)
{
	return PMPC_Type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int PMPI_Type_get_contents( MPI_Datatype datatype, 
		       	    int max_integers,
			    int max_addresses,
			    int max_datatypes,
			    int array_of_integers[],
			    MPI_Aint array_of_addresses[],
			    MPI_Datatype array_of_datatypes[])
{
	return PMPC_Type_get_contents(datatype, max_integers,max_addresses,max_datatypes,array_of_integers,array_of_addresses,array_of_datatypes);
}

  /* See the 1.1 version of the Standard.  The standard made an
     unfortunate choice here, however, it is the standard.  The size returned
     by MPI_Type_size is specified as an int, not an MPI_Aint */
int PMPI_Type_size (MPI_Datatype datatype, int *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_size (datatype, size);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_size_x(datatype, size);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

  /* MPI_Type_count was withdrawn in MPI 1.1 */
int PMPI_Type_lb (MPI_Datatype datatype, MPI_Aint * displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_lb (datatype, displacement);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_ub (MPI_Datatype datatype, MPI_Aint * displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(datatype,comm);
	res = __INTERNAL__PMPI_Type_ub (datatype, displacement);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(old_type,comm);
	res = __INTERNAL__PMPI_Type_create_resized( old_type, lb, extent, new_type );
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_commit (MPI_Datatype * datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type_create(*datatype,comm);
	res = __INTERNAL__PMPI_Type_commit (datatype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_free (MPI_Datatype * datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type (*datatype, MPI_COMM_WORLD);

	if( sctk_datatype_is_common(*datatype) )
	{
		MPI_ERROR_REPORT (comm, MPI_ERR_TYPE, "");
	}
	
	res = __INTERNAL__PMPI_Type_free (datatype);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_dup( MPI_Datatype old_type, MPI_Datatype *newtype )
{
	return PMPC_Type_dup(old_type, newtype);
}

int PMPI_Get_elements (MPI_Status * status, MPI_Datatype datatype, int *elements)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);
	
	MPI_Count tmp_elements = 0;
	
	res = __INTERNAL__PMPI_Get_elements_x (status, datatype, &tmp_elements);
	
	*elements = tmp_elements;
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Get_elements_x (MPI_Status * status, MPI_Datatype datatype, MPI_Count *elements)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(datatype,comm);

	res = __INTERNAL__PMPI_Get_elements_x (status, datatype, elements);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_darray (int size,
			     int rank,
			     int ndims,
			     int array_of_gsizes[],
			     int array_of_distribs[],
			     int array_of_dargs[],
			     int array_of_psizes[],
			     int order,
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(oldtype,comm);
	
	int csize;
	__INTERNAL__PMPI_Comm_size (comm, &csize);
	mpi_check_rank(rank, size, comm);
	
	
	res = __INTERNAL__PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
						  array_of_dargs, array_of_psizes, order, oldtype, newtype);
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_create_subarray (int ndims,
			       int array_of_sizes[],
			       int array_of_subsizes[],
			       int array_of_starts[],
			       int order,
			       MPI_Datatype oldtype,
			       MPI_Datatype * new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;
	mpi_check_type(oldtype,comm);
	
	res = __INTERNAL__PMPI_Type_create_subarray ( ndims, array_of_sizes, array_of_subsizes,
						      array_of_starts, order, oldtype, new_type);
	
	
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *rtype)
{
	/* This structure directly comes from the standard */
	switch(typeclass) 
	{
		case MPI_TYPECLASS_REAL: 
			switch(size)
			{
				case 4: *rtype = MPI_REAL4; return MPI_SUCCESS;
				case 8: *rtype = MPI_REAL8; return MPI_SUCCESS;
				case 16: *rtype = MPI_REAL16; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_REAL)");
			}
		case MPI_TYPECLASS_INTEGER: 
			switch(size)
			{
				case 1: *rtype = MPI_INTEGER1; return MPI_SUCCESS;
				case 2: *rtype = MPI_INTEGER2; return MPI_SUCCESS;
				case 4: *rtype = MPI_INTEGER4; return MPI_SUCCESS;
				case 8: *rtype = MPI_INTEGER8; return MPI_SUCCESS;
				case 16:  *rtype = MPI_INTEGER16; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_INTEGER)");
			}
		case MPI_TYPECLASS_COMPLEX: 
			switch(size)
			{
				case 8: *rtype = MPI_COMPLEX8; return MPI_SUCCESS;
				case 16: *rtype = MPI_COMPLEX16; return MPI_SUCCESS;
				case 32: *rtype = MPI_COMPLEX32; return MPI_SUCCESS;
				default: MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad size provided to MPI_Type_match_size (MPI_TYPECLASS_COMPLEX)");
			}
		default:
			MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Bad Type class provided to MPI_Type_match_size");
	}

	return MPI_SUCCESS;
}

int
PMPI_Pack (void *inbuf,
	   int incount,
	   MPI_Datatype datatype,
	   void *outbuf, int outcount, int *position, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	mpi_check_comm(comm,comm);
	mpi_check_type(datatype,comm);
	if ((NULL == outbuf) || (NULL == position) || (inbuf == NULL)) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	} else if (incount < 0) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"");
	} else if (outcount < 0) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	} else if(incount == 0) {
		return MPI_SUCCESS;
	}
	
	if(datatype < SCTK_USER_DATA_TYPES_MAX){
	  if(outcount < incount){
	    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	  }
	}
	res = __INTERNAL__PMPI_Pack (inbuf, incount, datatype, outbuf, outcount, position, comm);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Unpack (void *inbuf,
	     int insize,
	     int *position,
	     void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	mpi_check_comm(comm,comm);
	mpi_check_type(datatype,comm);
	if ((NULL == outbuf) || (NULL == inbuf) || (NULL == position)) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	} else if ((outcount < 0) || (insize < 0)) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"");
	} else if (datatype < 0) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"");
	}
	res = __INTERNAL__PMPI_Unpack (inbuf, insize, position, outbuf, outcount, datatype, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Pack_size (int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm(comm,comm);
  mpi_check_type(datatype,comm);
  mpi_check_count(incount,comm);
  res = __INTERNAL__PMPI_Pack_size (incount, datatype, comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Type_set_name( MPI_Datatype datatype, char *name )
{
	return PMPC_Type_set_name(datatype, name);
}

int PMPI_Type_get_name( MPI_Datatype datatype, char *name, int * resultlen )
{
	return PMPC_Type_get_name(datatype, name, resultlen);
}


int
PMPI_Barrier (MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
  if(sctk_is_inter_comm (comm)){
    MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
  }
#endif
  sctk_nodebug ("Entering BARRIER %d", comm);
  res = __INTERNAL__PMPI_Barrier (comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root,
	    MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size;
  
        mpi_check_comm(comm, comm);

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
        if(sctk_is_inter_comm (comm)){
          MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
        }
#endif
        
	if (MPI_IN_PLACE == buffer) {
          MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
      }
      
  __INTERNAL__PMPI_Comm_size (comm, &size);
	if((root < 0) || (root >= size)){
		MPI_ERROR_REPORT(comm,MPI_ERR_ROOT,"");
        }
	mpi_check_rank_send(root,size,comm);
	mpi_check_buf (buffer, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);

  sctk_nodebug ("Entering BCAST %d with count %d", comm, count);
  res = __INTERNAL__PMPI_Bcast (buffer, count, datatype, root, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	     void *recvbuf, int recvcnt, MPI_Datatype recvtype,
	     int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size, rank;
  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  __INTERNAL__PMPI_Comm_rank (comm, &rank);
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
	mpi_check_tag (root, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if ((0 == sendcnt && MPI_ROOT != root && 
			(rank != root ||
				(rank == root && MPI_IN_PLACE != sendbuf)
			)
		) ||
		(rank == root && MPI_IN_PLACE == sendbuf && 0 == recvcnt) || 
			(0 == recvcnt && 
				(MPI_ROOT == root || MPI_PROC_NULL == root)
			)
		) 
	{
		return MPI_SUCCESS;
	}

  res =
    __INTERNAL__PMPI_Gather (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			     recvtype, root, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int *recvcnts, int *displs,
	      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size, rank;
  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  __INTERNAL__PMPI_Comm_rank (comm, &rank);
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
//	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
	mpi_check_tag (root, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if ((rank != root && MPI_IN_PLACE == sendbuf) ||
        (rank == root && MPI_IN_PLACE == recvbuf)) 
    {
        MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
    }
 res =
    __INTERNAL__PMPI_Gatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
			      displs, recvtype, root, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	      void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
	      MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size, rank;
  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  __INTERNAL__PMPI_Comm_rank (comm, &rank);
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
	mpi_check_tag (root, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if ((rank != root && MPI_IN_PLACE == recvbuf) ||
        (rank == root && MPI_IN_PLACE == sendbuf)) 
    {
        MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
    }
    
	if ((0 == recvcnt && MPI_ROOT != root &&
			(rank != root ||
				(rank == root && MPI_IN_PLACE != recvbuf)
			)
		) ||
		(rank == root && MPI_IN_PLACE == recvbuf && 0 == sendcnt) ||
			(0 == sendcnt && 
				(MPI_ROOT == root || MPI_PROC_NULL == root)
			)
		) 
	{
        return MPI_SUCCESS;
    }
	
 res =
    __INTERNAL__PMPI_Scatter (sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
			      recvtype, root, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Scatterv (void *sendbuf, int *sendcnts, int *displs,
	       MPI_Datatype sendtype, void *recvbuf, int recvcnt,
	       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size, rank;
  mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
   __INTERNAL__PMPI_Comm_rank (comm, &rank);
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
//	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
	mpi_check_tag (root, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if ((rank != root && MPI_IN_PLACE == recvbuf) ||
        (rank == root && MPI_IN_PLACE == sendbuf)) 
    {
        MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
    }
  res =
    __INTERNAL__PMPI_Scatterv (sendbuf, sendcnts, displs, sendtype, recvbuf,
			       recvcnt, recvtype, root, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Allgather (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		void *recvbuf, int recvcount, MPI_Datatype recvtype,
		MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcount, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcount, comm);
	mpi_check_type (recvtype, comm);
	if(sctk_is_inter_comm (comm)){
	  if ( 0 == sendcount && 0 == recvcount ) 
	  {
		return MPI_SUCCESS;
	  }
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	  else
		MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
#endif
	}
	else
	{
		if ((MPI_IN_PLACE != sendbuf && 0 == sendcount) || (0 == recvcount)) {
            return MPI_SUCCESS;
		}
	}
	
	if (MPI_IN_PLACE == recvbuf) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
	
  res =
    __INTERNAL__PMPI_Allgather (sendbuf, sendcount, sendtype, recvbuf,
				recvcount, recvtype, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Allgatherv (void *sendbuf, int sendcount, MPI_Datatype sendtype,
		 void *recvbuf, int *recvcounts, int *displs,
		 MPI_Datatype recvtype, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcount, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
//	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if (MPI_IN_PLACE == recvbuf) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
  res =
    __INTERNAL__PMPI_Allgatherv (sendbuf, sendcount, sendtype, recvbuf,
				 recvcounts, displs, recvtype, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Alltoall (void *sendbuf, int sendcount, MPI_Datatype sendtype,
	       void *recvbuf, int recvcount, MPI_Datatype recvtype,
	       MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  if (sendcount == 0 && recvcount == 0) {
        return MPI_SUCCESS;
    }
	mpi_check_comm (comm, comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_count (sendcount, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (recvcount, comm);
	mpi_check_type (recvtype, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if (MPI_IN_PLACE == sendbuf) {
		sendcount = recvcount;
        sendtype = recvtype;
    }
	if (MPI_IN_PLACE == recvbuf) {
            MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
        }
  res =
    __INTERNAL__PMPI_Alltoall (sendbuf, sendcount, sendtype, recvbuf,
			       recvcount, recvtype, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls,
		MPI_Datatype sendtype, void *recvbuf, int *recvcnts,
		int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size;
  int i; 
   mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  for(i = 0; i < size; i++){
	mpi_check_count (sendcnts[i], comm);
	mpi_check_count (recvcnts[i], comm);
  }
	mpi_check_buf (sendbuf, comm);
//	mpi_check_count (sendcnt, comm);
	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
//	mpi_check_count (recvcnt, comm);
	mpi_check_type (recvtype, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if (MPI_IN_PLACE == sendbuf) {
            sendcnts = recvcnts;
            sdispls = rdispls;
            sendtype = recvtype;
        }
    if (MPI_IN_PLACE == recvbuf)
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  res =
    __INTERNAL__PMPI_Alltoallv (sendbuf, sendcnts, sdispls, sendtype, recvbuf,
				recvcnts, rdispls, recvtype, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int PMPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
				   void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
  int size;
  int i; 
   mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
 for(i = 0; i < size; i++){
        mpi_check_count (sendcnts[i], comm);
        mpi_check_count (recvcnts[i], comm);
	mpi_check_type (sendtypes[i], comm);
	mpi_check_type (recvtypes[i], comm);
  }

	mpi_check_buf (sendbuf, comm);
//	mpi_check_count (sendcnt, comm);
//	mpi_check_type (sendtype, comm);
	mpi_check_buf (recvbuf, comm);
//	mpi_check_count (recvcnt, comm);
//	mpi_check_type (recvtype, comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if (MPI_IN_PLACE == sendbuf) {
            sendcnts = recvcnts;
            sdispls    = rdispls;
            sendtypes  = recvtypes;
        }
    if (MPI_IN_PLACE == recvbuf) {
		MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
	}
	res = __INTERNAL__PMPI_Alltoallw (sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

/* Neighbors collectives */
int PMPI_Neighbor_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	} else if (recvcount < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcount must be superior or equal to zero");
	} 
    
    if(sctk_is_inter_comm (comm)){
		if (sendcount == 0 && recvcount == 0)
			return MPI_SUCCESS;
	} else {
		if (recvcount == 0)
			return MPI_SUCCESS;
	}
	
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart");
			res = __INTERNAL__PMPI_Neighbor_allgather_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgather_graph");
			res = __INTERNAL__PMPI_Neighbor_allgather_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	__INTERNAL__PMPI_Comm_size (comm, &size);
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	}
	
	for (i = 0; i < size; ++i) {
	  if (recvcounts[i] < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
	  }
	}

	if (NULL == displs) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_BUFFER,"displs must be valid");
	}
       
    if ( !sctk_is_inter_comm (comm) ) {
        for (i = 0; i < size; ++i) {
            if (recvcounts[i] != 0) {
                break;
            }
        }
        if (i >= size) {
            return MPI_SUCCESS;
        }
    }
    
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart");
			res = __INTERNAL__PMPI_Neighbor_allgatherv_cart(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			sctk_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_graph");
			res = __INTERNAL__PMPI_Neighbor_allgatherv_graph(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if (recvtype == MPI_DATATYPE_NULL) {	
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
	} else if (recvcount < 0) {
		MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcount must be superior or equal to zero");
	} 
	
	if (sendcount == 0 && recvcount == 0) {
        return MPI_SUCCESS;
    }
    
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoall_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoall_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoallv(void *sendbuf, int sendcounts[], int sdispls[], MPI_Datatype sendtype, void *recvbuf, int recvcounts[], int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if (!(topo->type == MPI_CART || topo->type == MPI_GRAPH)) {
		MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"Topology must be MPI_CART or MPI_GRAPH");
	} else if ((NULL == sendcounts) || (NULL == sdispls) || (NULL == recvcounts) || (NULL == rdispls)) {
		MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "Invalid arg");
	}
    
    __INTERNAL__PMPI_Comm_rank (comm, &size);
    for (i = 0; i < size; i++) 
    {
		if (recvcounts[i] < 0) {
			MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
		} else if (MPI_DATATYPE_NULL == recvtype) {
			MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtype must be a valid type");
		}
	}
           
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallv_cart(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallv_graph(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}

int PMPI_Neighbor_alltoallw(void *sendbuf, int sendcounts[], MPI_Aint sdispls[], MPI_Datatype sendtypes[], void *recvbuf, int recvcounts[], MPI_Aint rdispls[], MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t* tmp;
	mpi_topology_per_comm_t* topo;
	
	tmp = mpc_mpc_get_per_comm_data(comm);
	topo = &(tmp->topo);
	__INTERNAL__PMPI_Comm_rank (comm, &rank);
	
	if ((NULL == sendcounts) || (NULL == sdispls) || (NULL == sendtypes) || 
	    (NULL == recvcounts) || (NULL == rdispls) || (NULL == recvtypes)) {
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "Invalid arg");
    }
    
    __INTERNAL__PMPI_Comm_rank (comm, &size);
    for (i = 0; i < size; i++) 
    {
		if (recvcounts[i] < 0) {
			MPI_ERROR_REPORT(comm,MPI_ERR_COUNT,"recvcounts must be superior or equal to zero");
		} else if (MPI_DATATYPE_NULL == recvtypes[i] || recvtypes == NULL) {
			MPI_ERROR_REPORT(comm,MPI_ERR_TYPE,"recvtypes must be valid types");
		}
	}
	
	sctk_spinlock_lock (&(topo->lock));
		if (topo->type == MPI_CART)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallw_cart(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
		}
		else if (topo->type == MPI_GRAPH)
		{
			sctk_spinlock_unlock (&(topo->lock));
			res = __INTERNAL__PMPI_Neighbor_alltoallw_graph(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
		}
		else
		{
			sctk_spinlock_unlock (&(topo->lock));
			MPI_ERROR_REPORT (comm, MPI_ERR_TOPOLOGY, "Invalid type");
		}
	return res;
}


int
PMPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	     MPI_Op op, int root, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  int size, rank;
   mpi_check_comm (comm, comm);
  __INTERNAL__PMPI_Comm_size (comm, &size);
  __INTERNAL__PMPI_Comm_rank (comm, &rank);
	mpi_check_root(root,size,comm);
	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
	mpi_check_count (count, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(sctk_is_inter_comm (comm)){
	  MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
	}
#endif
	if ((rank != root && MPI_IN_PLACE == sendbuf) ||
        (rank == root && ((MPI_IN_PLACE == recvbuf) || (sendbuf == recvbuf)))) 
    {
        MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
    }
	if (0 == count) {
        return MPI_SUCCESS;
    }
  res =
    __INTERNAL__PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root,
			     comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Op_create (MPI_User_function * function, int commute, MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Op_create (function, commute, op);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Op_free (MPI_Op * op)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((*op < 0) || (*op < MAX_MPI_DEFINED_OP)){
    MPI_ERROR_REPORT(comm,MPI_ERR_OP,"");
  }
  res = __INTERNAL__PMPI_Op_free (op);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Allreduce (void *sendbuf, void *recvbuf, int count,
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
  if(sctk_is_inter_comm (comm)){
    MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
  }
#endif

  mpi_check_buf (sendbuf, comm);
  mpi_check_buf (recvbuf, comm);
  mpi_check_count (count, comm);
  mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
  sctk_nodebug ("Entering ALLREDUCE %d", comm);
  if( MPI_IN_PLACE == recvbuf ) 
    {
	    MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "");
    }
    if (0 == count) {
        return MPI_SUCCESS;
    }
  res =
    __INTERNAL__PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int i;
  int size; 
  int res = MPI_ERR_INTERN;
        mpi_check_comm (comm, comm);
  if (MPI_IN_PLACE == recvbuf) {
	  MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
  if(sctk_is_inter_comm (comm)){
    MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
  }
#endif

	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
  
  __INTERNAL__PMPI_Comm_size (comm, &size);

  for(i = 0; i < size; i++){
	mpi_check_count (recvcnts[i], comm);
  }
	mpi_check_type (datatype, comm);
	mpi_check_op (op,datatype, comm);
  res =
    __INTERNAL__PMPI_Reduce_scatter (sendbuf, recvbuf, recvcnts, datatype, op,
				     comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Reduce_scatter_block (void *sendbuf, void *recvbuf, int recvcnt,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int i;
  int size; 
  int res = MPI_ERR_INTERN;
  int * recvcnts;
        mpi_check_comm (comm, comm);

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
        if(sctk_is_inter_comm (comm)){
          MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
        }
#endif

	mpi_check_buf (sendbuf, comm);
	mpi_check_buf (recvbuf, comm);
  
  __INTERNAL__PMPI_Comm_size (comm, &size);

	mpi_check_count (recvcnts, comm);
	mpi_check_type (datatype, comm);
	mpi_check_op (op,datatype, comm);

  res =
    __INTERNAL__PMPI_Reduce_scatter_block (sendbuf, recvbuf, recvcnt, datatype, op,
				     comm);

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Scan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	   MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
        if(sctk_is_inter_comm (comm)){
          MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
        }
#endif

  mpi_check_buf (sendbuf, comm);
  mpi_check_buf (recvbuf, comm);
  mpi_check_count (count, comm);
  mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
  res = __INTERNAL__PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Exscan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	   MPI_Op op, MPI_Comm comm)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
        if(sctk_is_inter_comm (comm)){
          MPI_ERROR_REPORT(comm,MPI_ERR_COMM,"");
        }
#endif

  mpi_check_buf (sendbuf, comm);
  mpi_check_buf (recvbuf, comm);
  mpi_check_count (count, comm);
  mpi_check_type (datatype, comm);
	mpi_check_op (op, datatype,comm);
  res = __INTERNAL__PMPI_Exscan (sendbuf, recvbuf, count, datatype, op, comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_size (MPI_Group group, int *size)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_size (group, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_rank (MPI_Group group, int *rank)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_rank (group, rank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1,
			    MPI_Group group2, int *ranks2)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_translate_ranks (group1, n, ranks1, group2, ranks2);
  sctk_nodebug("RES = %d", res);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_compare (MPI_Group group1, MPI_Group group2, int *result)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_compare (group1, group2, result);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_group (MPI_Comm comm, MPI_Group * group)
{
	sctk_nodebug("Enter Comm_group");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm); 
  res = __INTERNAL__PMPI_Comm_group (comm, group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_union (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_union (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_intersection (MPI_Group group1, MPI_Group group2,
			 MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_intersection (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_difference (MPI_Group group1, MPI_Group group2,
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if((group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL)){
	MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }
  res = __INTERNAL__PMPI_Group_difference (group1, group2, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_incl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int i; 
  int size; 
  __INTERNAL__PMPI_Comm_size (comm, &size);
  for(i = 0; i < n ; i++){
    if((ranks[i] < 0) || (ranks[i] >= size)) MPI_ERROR_REPORT (comm, MPI_ERR_RANK, "");
  }
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_incl (group, n, ranks, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_excl (MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_excl (group, n, ranks, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_range_incl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_range_incl (group, n, ranges, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_range_excl (MPI_Group group, int n, int ranges[][3],
		       MPI_Group * newgroup)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }
  if(n < 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Group_range_excl (group, n, ranges, newgroup);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Group_free (MPI_Group * group)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  if(*group == MPI_GROUP_NULL){
        MPI_ERROR_REPORT (comm, MPI_ERR_GROUP, "");
  }

  res = __INTERNAL__PMPI_Group_free (group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_size (MPI_Comm comm, int *size)
{
	sctk_nodebug("Enter Comm_size comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_size (comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_rank (MPI_Comm comm, int *rank)
{
	sctk_nodebug("Enter Comm_rank comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_rank (comm, rank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result)
{
	sctk_nodebug("Enter Comm_compare");
	MPI_Comm comm = MPI_COMM_WORLD;
	int res = MPI_ERR_INTERN;

	if(comm1 < 0 || comm2 < 0)
	{
		if(comm1 != comm2)
			*result = MPI_UNEQUAL;
		else
			*result = MPI_IDENT;
		return MPI_ERR_ARG;
	}

	mpi_check_comm (comm1, comm1);
	mpi_check_comm (comm2, comm2);
	res = __INTERNAL__PMPI_Comm_compare (comm1, comm2, result);
	SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_dup (MPI_Comm comm, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_dup");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_dup (comm, newcomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_create (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_create");
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
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Intercomm_create (MPI_Comm local_comm, int local_leader,
		       MPI_Comm peer_comm, int remote_leader, int tag,
		       MPI_Comm * newintercomm)
{
	sctk_nodebug("Enter Intercomm_create");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  int size;

  mpi_check_comm(local_comm,local_comm);
  mpi_check_comm(peer_comm,peer_comm);
  
    __INTERNAL__PMPI_Comm_size (local_comm, &size);
    if(sctk_is_inter_comm(local_comm) == 0){
      mpi_check_rank_send (local_leader, size, comm);
    }
    __INTERNAL__PMPI_Comm_size (peer_comm, &size);
    if(sctk_is_inter_comm(peer_comm) == 0){
      mpi_check_rank_send (remote_leader, size, comm);
    }

  if (newintercomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Intercomm_create (local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_split (MPI_Comm comm, int color, int key, MPI_Comm * newcomm)
{
	sctk_nodebug("Enter Comm_split");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if (newcomm == NULL)
    {
      MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
    }
  res = __INTERNAL__PMPI_Comm_split (comm, color, key, newcomm);
  sctk_nodebug ("SPLIT Com %d Color %d, key %d newcomm %d", comm, color, key, *newcomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_free (MPI_Comm * comm)
{
	sctk_nodebug("Enter Comm_free");
  int res = MPI_ERR_INTERN;
  if (comm == NULL)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }
  if (*comm == MPI_COMM_WORLD)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }

  if (*comm == MPI_COMM_SELF)
    {
      MPI_ERROR_REPORT (MPI_COMM_WORLD, MPI_ERR_COMM, "");
    }

  mpi_check_comm (*comm, *comm);
  res = __INTERNAL__PMPI_Comm_free (comm);
  SCTK_MPI_CHECK_RETURN_VAL (res, *comm);
}

int
PMPI_Comm_test_inter (MPI_Comm comm, int *flag)
{
	sctk_nodebug("Enter Comm_test_inter");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_test_inter (comm, flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_remote_size (MPI_Comm comm, int *size)
{
	sctk_nodebug("Enter Comm_remote_size comm %d", comm);
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(sctk_is_inter_comm (comm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }
  res = __INTERNAL__PMPI_Comm_remote_size (comm, size);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_remote_group (MPI_Comm comm, MPI_Group * group)
{
	sctk_nodebug("Enter Comm_remote_group");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(sctk_is_inter_comm (comm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }

  res = __INTERNAL__PMPI_Comm_remote_group (comm, group);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Intercomm_merge (MPI_Comm intercomm, int high, MPI_Comm * newintracomm)
{
	sctk_nodebug("Enter Intercomm_merge");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
   mpi_check_comm (intercomm,comm);

  if(sctk_is_inter_comm (intercomm) == 0){
    MPI_ERROR_REPORT (comm, MPI_ERR_COMM, "");
  }

  res = __INTERNAL__PMPI_Intercomm_merge (intercomm, high, newintracomm);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
	TODO("PMPI_Comm_set_errhandler is dummy");
	return MPI_SUCCESS;
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
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Keyval_free (int *keyval)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  if(*keyval == MPI_KEYVAL_INVALID){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Keyval_free (keyval);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_put (MPI_Comm comm, int keyval, void *attr_value)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_put (comm, keyval, attr_value);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_get (MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_get (comm, keyval, attr_value, flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Attr_delete (MPI_Comm comm, int keyval)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Attr_delete (comm, keyval);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Topo_test (MPI_Comm comm, int *topo_type)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Topo_test (comm, topo_type);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims, int *periods,
		  int reorder, MPI_Comm * comm_cart)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int sum = 1;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  
  if(ndims < 0)
  {
    MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }
  else if (ndims >= 1 && 
	  (dims == NULL || periods == NULL || comm_cart == NULL))
  {
	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }

  for(i = 0; i < ndims; i++){
          if(dims[i] < 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
          if(dims[i] > size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
        sum *= dims[i];
  }
  if(sum > size){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }

  }

  res =
    __INTERNAL__PMPI_Cart_create (comm_old, ndims, dims, periods, reorder,
				  comm_cart);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Dims_create (int nnodes, int ndims, int *dims)
{
	sctk_nodebug("Enter PMPI_Dims_create");
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;

  if(nnodes <= 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Dims_create (nnodes, ndims, dims);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_create (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		   int reorder, MPI_Comm * comm_graph)
{
	sctk_nodebug("Enter PMPI_Graph_create");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int nb_edge = 0;
  int first_edge = 0;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  if((nnodes < 0) || (nnodes > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }
  nb_edge = index[nnodes -1];
  for(i = 0; i < nb_edge; i++){
	  if(edges[i] < 0){
        	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
	  }
          if(edges[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
  }
  for (i = 0; i < nnodes; i++){
    int j; 
    for(j = first_edge; j < index[i]; j++){
          if(edges[j] == i){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
    }
    first_edge = index[i];
  }
  }
  res =
    __INTERNAL__PMPI_Graph_create (comm_old, nnodes, index, edges, reorder,
				   comm_graph);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Graphdims_get (MPI_Comm comm, int *nnodes, int *nedges)
{
	sctk_nodebug("Enter PMPI_Graphdims_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graphdims_get (comm, nnodes, nedges);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_get (MPI_Comm comm, int maxindex, int maxedges,
		int *index, int *edges)
{
	sctk_nodebug("Enter PMPI_Graph_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_get (comm, maxindex, maxedges, index, edges);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cartdim_get (MPI_Comm comm, int *ndims)
{
	sctk_nodebug("Enter PMPI_Cartdim_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cartdim_get (comm, ndims);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods,
	       int *coords)
{
	sctk_nodebug("Enter PMPI_Cart_get");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);  {
  int i;
  int size;
  __INTERNAL__PMPI_Comm_size (comm, &size);
  if((maxdims <= 0) || (maxdims > size)){
        MPI_ERROR_REPORT (comm, MPI_ERR_DIMS, "");
  }
  }

  res = __INTERNAL__PMPI_Cart_get (comm, maxdims, dims, periods, coords);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_rank (MPI_Comm comm, int *coords, int *rank)
{
	sctk_nodebug("Enter PMPI_Cart_rank");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_rank (comm, coords, rank, 0);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_coords (MPI_Comm comm, int rank, int maxdims, int *coords)
{
	sctk_nodebug("Enter PMPI_Cart_coords");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_coords (comm, rank, maxdims, coords, 0);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_neighbors_count (MPI_Comm comm, int rank, int *nneighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Graph_neighbors_count (comm, rank, nneighbors);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Graph_neighbors (MPI_Comm comm, int rank, int maxneighbors,
		      int *neighbors)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res =
    __INTERNAL__PMPI_Graph_neighbors (comm, rank, maxneighbors, neighbors);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_shift (MPI_Comm comm, int direction, int displ, int *source,
		 int *dest)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  if(displ <= 0){
        MPI_ERROR_REPORT (comm, MPI_ERR_ARG, "");
  }

  res = __INTERNAL__PMPI_Cart_shift (comm, direction, displ, source, dest);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_sub (MPI_Comm comm, int *remain_dims, MPI_Comm * comm_new)
{
	sctk_nodebug("Enter PMPI_Cart_sub");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Cart_sub (comm, remain_dims, comm_new);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Cart_map (MPI_Comm comm_old, int ndims, int *dims, int *periods,
	       int *newrank)
{
	sctk_nodebug("Enter PMPI_Cart_map");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  {
  int i;
  int size;
  int sum = 1;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  if((ndims <= 0) || (ndims > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }
  for(i = 0; i < ndims; i++){
          if(dims[i] <= 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
          if(dims[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
          }
	sum *= dims[i];
  }
  if(sum > size){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_DIMS, "");
  }

  }

  res = __INTERNAL__PMPI_Cart_map (comm_old, ndims, dims, periods, newrank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Graph_map (MPI_Comm comm_old, int nnodes, int *index, int *edges,
		int *newrank)
{
	sctk_nodebug("Enter PMPI_Graph_map");
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm_old, comm_old);
  if(nnodes <= 0){
	MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }  
{
  int i;
  int size;
  __INTERNAL__PMPI_Comm_size (comm_old, &size);
  if((nnodes < 0) || (nnodes > size)){
        MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
  }
  for(i = 0; i < nnodes; i++){
          if(edges[i] < 0){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
          if(edges[i] >= size){
                MPI_ERROR_REPORT (comm_old, MPI_ERR_ARG, "");
          }
  }
  }

  res = __INTERNAL__PMPI_Graph_map (comm_old, nnodes, index, edges, newrank);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm_old);
}

int
PMPI_Get_processor_name (char *name, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_processor_name (name, resultlen);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Get_version (int *version, int *subversion)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Get_version (version, subversion);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_create (MPI_Handler_function * function,
			MPI_Errhandler * errhandler)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_create (function, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_set (comm, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler * errhandler)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Errhandler_get (comm, errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Errhandler_free (MPI_Errhandler * errhandler)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Errhandler_free (errhandler);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Error_string (int errorcode, char *string, int *resultlen)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_string (errorcode, string, resultlen);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Error_class (int errorcode, int *errorclass)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Error_class (errorcode, errorclass);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
PMPI_Query_thread (int *provided)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Query_thread (provided);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}


int
PMPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init_thread (argc, argv, required, provided);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Init (int *argc, char ***argv)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Init (argc, argv);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Finalize (void)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Finalize ();
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Finalized (int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Finalized (flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Initialized (int *flag)
{
  MPI_Comm comm = MPI_COMM_WORLD;
  int res = MPI_ERR_INTERN;
  res = __INTERNAL__PMPI_Initialized (flag);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Abort (MPI_Comm comm, int errorcode)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);

  res = __INTERNAL__PMPI_Abort (comm, errorcode);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int
PMPI_Comm_set_name (MPI_Comm comm, char *comm_name)
{
  int res = MPI_ERR_INTERN;
  mpi_check_comm (comm, comm);
  res = __INTERNAL__PMPI_Comm_set_name (comm, comm_name);
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

int MPC_Mpi_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* type */
int MPC_Mpi_type_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_type_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_type_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_type_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_type_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_type_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* comm */
int MPC_Mpi_comm_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_comm_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_comm_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_comm_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_comm_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_comm_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* win */
int MPC_Mpi_win_null_delete_fn( MPI_Datatype datatype, int type_keyval, void* attribute_val_out, void* extra_state )
{
	sctk_nodebug("MPC_Mpi_win_null_delete_fn");
	return MPI_SUCCESS;
}

int MPC_Mpi_win_null_copy_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_win_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPC_Mpi_win_dup_fn( MPI_Comm comm, int comm_keyval, void* extra_state, void* attribute_val_in, void* attribute_val_out, int* flag )
{
	sctk_nodebug("MPC_Mpi_win_dup_fn");
   *flag = 1;
   *(void**)attribute_val_out = attribute_val_in;
   return MPI_SUCCESS;
}

/* MPI-2 - Language interoperability - Transfer of Handles*/
MPI_Comm PMPI_Comm_f2c(MPI_Fint comm)
{
	return (MPI_Comm)comm;
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm)
{
	return (MPI_Fint)comm;
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype)
{
	return (MPI_Datatype)datatype;
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype)
{
	return (MPI_Fint)datatype;
}

MPI_Group PMPI_Group_f2c(MPI_Fint group)
{
	return (MPI_Group)group;
}

MPI_Fint PMPI_Group_c2f(MPI_Group group)
{
	return (MPI_Fint)group;
}

MPI_Request PMPI_Request_f2c(MPI_Fint request)
{
	return (MPI_Request)request;
}

MPI_Fint PMPI_Request_c2f(MPI_Request request)
{
	return (MPI_Fint)request;
}

MPI_File PMPI_File_f2c(MPI_Fint file)
{
	return (MPI_File)file;
}

MPI_Fint PMPI_File_c2f(MPI_File file)
{
	return (MPI_Fint)file;
}

MPI_Win PMPI_Win_f2c(MPI_Fint win)
{
	return (MPI_Win)win;
}

MPI_Fint PMPI_Win_c2f(MPI_Win win)
{
	return (MPI_Fint)win;
}

MPI_Op PMPI_Op_f2c(MPI_Fint op)
{
	return (MPI_Op)op;
}

MPI_Fint PMPI_Op_c2f(MPI_Op op)
{
	return (MPI_Fint)op;
}

MPI_Info PMPI_Info_f2c(MPI_Fint info)
{
	return (MPI_Info)info;
}

MPI_Fint PMPI_Info_c2f(MPI_Info info)
{
	return (MPI_Fint)info;
}

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler)
{
	return (MPI_Errhandler)errhandler;
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
	return (MPI_Fint)errhandler;
}

/*********************** 
*  MPI Info Management *
***********************/


int PMPI_Info_set( MPI_Info info, const char *key, const char *value )
{
	return PMPC_Info_set( (MPC_Info)info, key, value );
}

int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag)
{
	return PMPC_Info_get( (MPC_Info)info, key, valuelen, value, flag );
}

int PMPI_Info_free( MPI_Info *info )
{
	return PMPC_Info_free( (MPC_Info *)info );
}

int PMPI_Info_dup( MPI_Info info, MPI_Info *newinfo )
{
	return PMPC_Info_dup( (MPC_Info) info , (MPC_Info * ) newinfo );
}

int PMPI_Info_delete( MPI_Info info, const char *key )
{
	return PMPC_Info_delete( (MPC_Info) info , key );
}

int PMPI_Info_create( MPI_Info *info )
{
	return PMPC_Info_create( (MPC_Info *) info );
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys )
{
	return PMPC_Info_get_nkeys( (MPC_Info) info, nkeys );
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key )
{
	return PMPC_Info_get_nthkey( (MPC_Info) info, n , key );
}

int PMPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, int *flag)
{
	return PMPC_Info_get_valuelen( (MPC_Info) info , key , valuelen , flag );
}


//~ not implemented

/*
int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype){return MPI_SUCCESS;}
int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen){return MPI_SUCCESS;}
int PMPI_Type_set_name(MPI_Datatype datatype, const char *type_name){return MPI_SUCCESS;}

int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val){return MPI_SUCCESS;}
int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag){return MPI_SUCCESS;}
int PMPI_Win_free_keyval(int *win_keyval){return MPI_SUCCESS;}
int PMPI_Win_delete_attr(MPI_Win win, int win_keyval){return MPI_SUCCESS;}
int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
						   MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state){return MPI_SUCCESS;}
int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){return MPI_SUCCESS;}
int PMPI_Win_free(MPI_Win * win){return MPI_SUCCESS;}

int PMPI_Alloc_mem (MPI_Aint size, MPI_Info info, void *baseptr){return MPI_SUCCESS;}
int PMPI_Free_mem (void *base){return MPI_SUCCESS;}

int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){return MPI_SUCCESS;}


int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler){return MPI_SUCCESS;}
int PMPI_Finalized( int *flag ){return MPI_SUCCESS;}

int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val){return MPI_SUCCESS;}
int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag){return MPI_SUCCESS;}
int PMPI_Comm_free_keyval(int *comm_keyval){return MPI_SUCCESS;}
int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval){return MPI_SUCCESS;}
int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
						   MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state){return MPI_SUCCESS;}

int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val){return MPI_SUCCESS;}
int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag){return MPI_SUCCESS;}
int PMPI_Type_free_keyval(int *type_keyval){return MPI_SUCCESS;}
int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval){return MPI_SUCCESS;}
int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
						   MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state){return MPI_SUCCESS;}

int PMPI_Type_create_indexed_block(int count, int blocklength, int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype){return MPI_SUCCESS;}
int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner){return MPI_SUCCESS;}
int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
						  int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]){return MPI_SUCCESS;}

int PMPI_Type_create_darray(int size, int rank, int ndims, int array_of_gsizes[], int array_of_distribs[], int array_of_dargs[],
                           int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype){return MPI_SUCCESS;}
int PMPI_Get_address( void *location, MPI_Aint *address ){return MPI_SUCCESS;}
int PMPI_Type_create_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[],
						   MPI_Datatype array_of_types[], MPI_Datatype *newtype){return MPI_SUCCESS;}

int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count){return MPI_SUCCESS;}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size){return MPI_SUCCESS;}
int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent){return MPI_SUCCESS;}
int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent){return MPI_SUCCESS;}
int PMPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count){return MPI_SUCCESS;}
int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count){return MPI_SUCCESS;}

int PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[],
								   MPI_Datatype oldtype, MPI_Datatype * newtype){return MPI_SUCCESS;}
int PMPI_Pack_external_size(char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size){return MPI_SUCCESS;}
int PMPI_Pack_external(char *datarep, void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position){return MPI_SUCCESS;}
int PMPI_Unpack_external(char *datarep, void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype){return MPI_SUCCESS;}

int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *type){return MPI_SUCCESS;}
int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){return MPI_SUCCESS;}
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm){return MPI_SUCCESS;}
int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm){return MPI_SUCCESS;}
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info){return MPI_SUCCESS;}
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info * info_used){return MPI_SUCCESS;}
int PMPI_Add_error_class(int *errorclass){return MPI_SUCCESS;}
int PMPI_Add_error_code(int errorclass, int *errorcode){return MPI_SUCCESS;}
int PMPI_Add_error_string(int errorcode, char *string){return MPI_SUCCESS;}
int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode){return MPI_SUCCESS;}
int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *function, MPI_Errhandler *errhandler){return MPI_SUCCESS;}
int PMPI_Is_thread_main(int *flag){return MPI_SUCCESS;}
int PMPI_Query_thread( int *provided ){return MPI_SUCCESS;}
int PMPI_Get_library_version(char *version, int *resultlen){return MPI_SUCCESS;}
int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status){return MPI_SUCCESS;}
int PMPI_Status_set_cancelled(MPI_Status *status, int flag){return MPI_SUCCESS;}
int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
						MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request ){return MPI_SUCCESS;}

int PMPI_Grequest_complete(MPI_Request request){return MPI_SUCCESS;}
*/














//~ end
#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
INFO("Default mpc_user_main__ has been removed because of TLS compilation...")
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
