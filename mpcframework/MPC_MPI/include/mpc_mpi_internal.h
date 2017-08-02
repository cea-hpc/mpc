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
//#include "sctk_alloc.h"
#include "mpc_common.h"
#include "mpc_reduction.h"
#include "mpcthread.h"
#include "mpi_alloc_mem.h"
#include "nbc.h"
#include "sctk_alloc.h"
#include "sctk_collective_communications.h"
#include "sctk_communicator.h"
#include "sctk_communicator.h"
#include "sctk_debug.h"
#include "sctk_handle.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_runtime_config.h"
#include "sctk_spinlock.h"
#include "sctk_thread.h"
#include "sctk_topology.h"
#include <mpc.h>
#include <mpc_extern32.h>
#include <mpc_mpi.h>
#include <sctk_debug.h>
#include <sctk_ethread_internal.h>
#include <sctk_spinlock.h>
#include <string.h>
#include <uthash.h>
#include <utlist.h>
TODO("Optimize algorithme for derived types")

#define ENABLE_COLLECTIVES_ON_INTERCOMM  
//#define MPC_MPI_USE_REQUEST_CACHE
//#define MPC_MPI_USE_LOCAL_REQUESTS_QUEUE
int __INTERNAL__PMPI_Attr_set_fortran (int keyval);

char * sctk_char_fortran_to_c (char *buf, int size, char ** free_ptr);
void sctk_char_c_to_fortran (char *buf, int size);

#if defined(SCTK_USE_CHAR_MIXED)
#define SCTK_CHAR_END(size)
#define SCTK_CHAR_MIXED(size)  ,long int size
#else
#define SCTK_CHAR_END(size) ,long int size
#define SCTK_CHAR_MIXED(size)
#endif


struct sctk_list_elem {
  void* elem;
  struct sctk_list_elem *prev;
  struct sctk_list_elem *next;
};


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

	/****** NBC_HANDLES ******/
        int NBC_Pthread_nb; // number of elements in the list
                            // NBC_Pthread_handles
        struct sctk_list_elem *NBC_Pthread_handles;
        sctk_thread_mutex_t list_handles_lock;
        sctk_thread_t NBC_Pthread;

        /****** NBC_INITIALIZE ******/
        int nbc_initialized_per_task;
        sctk_thread_mutex_t nbc_initializer_lock;

}mpc_mpi_data_t;


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
	sctk_spinlock_t lock; /**< Lock protecting the data-structure */	

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

	/*Owner*/
	void* task_req_id;

	/*NBC_handle */
	int is_nbc;
	NBC_Handle nbc_handle;

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


MPI_request_struct_t * __sctk_internal_get_MPC_requests();
void __sctk_init_mpc_request();
MPI_internal_request_t *
__sctk_convert_mpc_request_internal_cache_get(MPI_Request *req,
                                              MPI_request_struct_t *requests);
void __sctk_convert_mpc_request_internal_cache_register(
    MPI_internal_request_t *req);
void sctk_delete_internal_request_clean(MPI_internal_request_t *tmp);
MPI_internal_request_t *
__sctk_new_mpc_request_internal_local_get(MPI_Request *req,
                                          MPI_request_struct_t *requests);
int sctk_delete_internal_request_local_put(MPI_internal_request_t *tmp,
                                           MPI_request_struct_t *requests);
void sctk_delete_internal_request(MPI_internal_request_t *tmp,
                                  MPI_request_struct_t *requests);
void sctk_check_auto_free_list(MPI_request_struct_t *requests);
void __sctk_init_mpc_request_internal(MPI_internal_request_t *tmp);
MPI_internal_request_t *
__sctk_new_mpc_request_internal(MPI_Request *req,
                                MPI_request_struct_t *requests);
MPC_Request *__sctk_new_mpc_request(MPI_Request *req,
                                    MPI_request_struct_t *requests);
MPI_internal_request_t *
__sctk_convert_mpc_request_internal(MPI_Request *req,
                                    MPI_request_struct_t *requests);
MPC_Request *__sctk_convert_mpc_request(MPI_Request *req,
                                        MPI_request_struct_t *requests);
void __sctk_add_in_mpc_request(MPI_Request *req, void *t,
                               MPI_request_struct_t *requests);
void __sctk_delete_mpc_request(MPI_Request *req,
                               MPI_request_struct_t *requests);

sctk_derived_datatype_t *sctk_get_derived_datatype(MPC_Datatype datatype);
int *sctk_group_raw_ranks(MPI_Group group);

typedef struct {
  MPC_Op op;
  int used;
  int commute;
} sctk_op_t;

sctk_op_t *sctk_convert_to_mpc_op(MPI_Op op);


static inline sctk_op_can_commute( sctk_op_t * op , MPI_Datatype type )
{
	if( op->commute == 0 )
	{
		return 0;
	}

	if( type == MPI_FLOAT )
	{
		return 0;
	}

	if( type == MPI_DOUBLE )
	{
		return 0;
	}

	return 1;
}



MPC_Op_f sctk_get_common_function(MPC_Datatype datatype, MPC_Op op);

/*
  SHARED INTERNAL FUNCTIONS
*/

static inline int sctk_mpi_type_is_shared_mem(MPI_Datatype type, int count) {
  if (SHM_COLL_BUFF_MAX_SIZE <= count) {
    return 0;
  }

  switch (type) {
  case MPI_INT:
  case MPI_FLOAT:
  case MPI_CHAR:
  case MPI_DOUBLE:
    return 1;

  default:
    return 0;
  }
}

static inline int sctk_mpi_op_is_shared_mem(MPI_Datatype op) {
  switch (op) {
  case MPI_SUM:
    return 1;

  default:
    return 0;
  }
}

static inline void sctk_mpi_shared_mem_buffer_fill(union shared_mem_buffer *b,
                                                   MPI_Datatype type, int count,
                                                   void *src) {
  int i;

  switch (type) {
  case MPI_INT:
    for (i = 0; i < count; i++)
      b->i[i] = ((int *)src)[i];
    break;
  case MPI_FLOAT:
    for (i = 0; i < count; i++)
      b->f[i] = ((float *)src)[i];
    break;
  case MPI_CHAR:
    for (i = 0; i < count; i++)
      b->c[i] = ((char *)src)[i];
    break;

  case MPI_DOUBLE:
    for (i = 0; i < count; i++)
      b->d[i] = ((double *)src)[i];
    break;

  default:
    sctk_fatal("Unsupported data-type");
  }
}

static inline void sctk_mpi_shared_mem_buffer_get(union shared_mem_buffer *b,
                                                  MPI_Datatype type, int count,
                                                  void *dest,
                                                  size_t source_size) {
  int i;
  MPI_Aint tsize;

  switch (type) {
  case MPI_INT:
    for (i = 0; i < count; i++)
      ((int *)dest)[i] = b->i[i];
    break;
  case MPI_FLOAT:
    for (i = 0; i < count; i++)
      ((float *)dest)[i] = b->f[i];
    break;
  case MPI_CHAR:
    for (i = 0; i < count; i++)
      ((char *)dest)[i] = b->c[i];
    break;

  case MPI_DOUBLE:
    for (i = 0; i < count; i++)
      ((double *)dest)[i] = b->d[i];
    break;

  default:
    /* This is the crazy case contig to derived
     * when being packed (non uniform types) */
    PMPI_Type_extent(type, &tsize);
    int cnt = 0;
    PMPI_Unpack(b->c, source_size, &cnt, dest, count, type, MPI_COMM_WORLD);
  }
}

#define MPI_ERROR_REPORT(comm, error,message) return SCTK__MPI_ERROR_REPORT__(comm, error,message,__FILE__, __LINE__)
int __MPC_Error_init();

void SCTK__MPI_INIT_REQUEST (MPI_Request * request);
int SCTK__MPI_ERROR_REPORT__ (MPC_Comm comm, int error, char *message, char *file, int line);

int __INTERNAL__PMPI_Type_extent (MPI_Datatype, MPI_Aint *);
int __INTERNAL__PMPI_Type_size (MPI_Datatype, int *);
int __INTERNAL__PMPI_Pack (void *, int, MPI_Datatype, void *, int,
                                  int *, MPI_Comm);
int __INTERNAL__PMPI_Unpack (void *, int, int *, void *, int,
                                    MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Pack_size (int, MPI_Datatype, MPI_Comm, int *);

int __INTERNAL__PMPI_Waitall (int, MPI_Request *, MPI_Status *);
int __INTERNAL__PMPI_Waitany (int, MPI_Request *, int *, MPI_Status *);

int __INTERNAL__PMPI_Irecv (void *, int, MPI_Datatype, int, int,
				   MPI_Comm, MPI_Request *);
int __INTERNAL__PMPI_Allgather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Allgatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Allreduce (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Alltoall (void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Alltoallv (void *, int *, int *, MPI_Datatype,void *, int *, int *, MPI_Datatype, MPI_Comm);
int __INTERNAL__PMPI_Alltoallw (void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm);
int __INTERNAL__PMPI_Bcast (void *, int, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Exscan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Gather (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Gatherv (void *, int, MPI_Datatype, void *, int *, int *, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Reduce (void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
int __INTERNAL__PMPI_Reduce_scatter (void *, void *, int *, MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Reduce_scatter_block (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Scan (void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
int __INTERNAL__PMPI_Scatter (void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);
int __INTERNAL__PMPI_Scatterv (void *, int *, int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

int NBC_Finalize(sctk_thread_t *NBC_thread);

