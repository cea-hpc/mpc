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

#include "mpc_common_debug.h"
#include "nbc.h"
#include "sctk_alloc.h"
#include "mpc_lowcomm.h"

#include "errh.h"
#include <mpc_lowcomm_msg.h>
#include "mpc_common_spinlock.h"
#include "mpc_coll.h"

#include "mpc_topology.h"
#include <mpc.h>
#include <mpc_extern32.h>
#include <mpc_mpi.h>
#include <mpc_common_debug.h>

#include <mpc_common_spinlock.h>
#include <string.h>
#include <uthash.h>
#include <utlist.h>

#include "lcp.h"

TODO("Expose this header cleanly");
#include <shm_coll.h>


#define ENABLE_COLLECTIVES_ON_INTERCOMM
//#define MPC_MPI_USE_REQUEST_CACHE
//#define MPC_MPI_USE_LOCAL_REQUESTS_QUEUE

/********************
 * INIT AND RELEASE *
 ********************/

int mpc_mpi_initialize(void);
int mpc_mpi_release(void);

/*******************
 * ERROR REPORTING *
 *******************/

/*
 * TODO: RULE FROM THE MPI STANDARD
 * In order to fully support the MPI standard, we cannot fail if the returned value of
 * an MPI function is different than MPI_SUCCESS.
 * We could add an additional MPC mode in order to fail in the case of a wrong returned value.
 */
int _mpc_mpi_report_error(mpc_lowcomm_communicator_t comm, int error, char *message, char * function, char *file, int line);

#define MPI_ERROR_REPORT(comm, error, message)      return _mpc_mpi_report_error(comm, error, message, (char*)__FUNCTION__, (char*)__FILE__, __LINE__)
#define MPI_HANDLE_RETURN_VAL(res, comm)            do { if(res == MPI_SUCCESS){ return res; } else { MPI_ERROR_REPORT(comm, res, "Generic error return"); } } while(0)
#define MPI_HANDLE_ERROR(res, comm, desc_string)    do { if(res != MPI_SUCCESS){ MPI_ERROR_REPORT(comm, res, desc_string); } } while(0)



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

struct _mpc_mpi_buffer;

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
	struct _mpc_mpi_buffer *buffers;

	/****** OP ******/
	struct sctk_mpi_ops_s *ops;

	/****** LOCK ******/
	mpc_common_spinlock_t lock;

	/****** NBC_HANDLES ******/
        int NBC_Pthread_nb; // number of elements in the list
                            // NBC_Pthread_handles
        struct sctk_list_elem *NBC_Pthread_handles;
        mpc_thread_mutex_t list_handles_lock;
        mpc_thread_sem_t pending_req;
        mpc_thread_t NBC_Pthread;

        /****** NBC_INITIALIZE ******/
        int nbc_initialized_per_task;
        mpc_thread_mutex_t nbc_initializer_lock;

}mpc_mpi_data_t;


/************************************************************************/
/* Request Handling                                                    */
/************************************************************************/

typedef enum
{
	MPC_MPI_PERSISTENT_BCAST_INIT,
	MPC_MPI_PERSISTENT_GATHER_INIT,
	MPC_MPI_PERSISTENT_ALLGATHER_INIT,
	MPC_MPI_PERSISTENT_ALLTOALL_INIT,
	MPC_MPI_PERSISTENT_ALLTOALLV_INIT,
	MPC_MPI_PERSISTENT_ALLTOALLW_INIT,
	MPC_MPI_PERSISTENT_REDUCE_INIT,
	MPC_MPI_PERSISTENT_SCAN_INIT,
	MPC_MPI_PERSISTENT_EXSCAN_INIT,
	MPC_MPI_PERSISTENT_REDUCE_SCATTER_INIT,
	MPC_MPI_PERSISTENT_REDUCE_SCATTER_BLOCK_INIT,
	MPC_MPI_PERSISTENT_ALLREDUCE_INIT,
	MPC_MPI_PERSISTENT_SCATTER_INIT,
	MPC_MPI_PERSISTENT_SCATTERV_INIT,
	MPC_MPI_PERSISTENT_GATHERV_INIT,
	MPC_MPI_PERSISTENT_ALLGATHERV_INIT,
	MPC_MPI_PERSISTENT_SEND_INIT,
	MPC_MPI_PERSISTENT_BSEND_INIT,
	MPC_MPI_PERSISTENT_RSEND_INIT,
	MPC_MPI_PERSISTENT_SSEND_INIT,
	MPC_MPI_PERSISTENT_RECV_INIT,
	MPC_MPI_PERSISTENT_BARRIER_INIT
} MPI_Persistant_op_t;

typedef struct prtd_map {
        uint32_t p_id_start;
        uint32_t p_id_end;
} prtd_map_t;

typedef struct {
	void        *buf;
	int          count;
	MPI_Datatype datatype;
	int          dest_source;
	int          tag;
	MPI_Comm     comm;

	mpc_lowcomm_request_t tag_req;
	mpc_lowcomm_request_t rkey_req;
	mpc_lowcomm_request_t fin_req;

        int       partitions;
        int       length;
        lcp_mem_h rkey_prtd;
        size_t    rkey_prtd_pkg_size; 
        lcp_mem_h rkey_cflags;
        size_t    rkey_cflags_pkg_size; 
        OPA_int_t counter;
        int       complete;
        int       fin;

        union {
                struct {
                        void *rkeys_buf;
                        size_t rkeys_size;
                        int *send_cflags;
                } send;

                struct {
                       int         send_partitions;
                       prtd_map_t *map;
                       int        *cflags_buf;
                       void       *rkeys_buf;
                       size_t      rkeys_buf_size;
                } recv;
        };
} mpi_partitioned_t;

typedef struct MPI_Persistant_s
{
	void *buf;
	int count;
	MPI_Datatype datatype;
	int dest_source;
	int tag;
	MPI_Comm comm;
	MPI_Persistant_op_t op;
	/* collective */
	const void *sendbuf;
	void *recvbuf;
	int root;
	int sendcount;
	int recvcount;
	const int *sendcounts;
	const int *recvcounts;
	const int *sdispls;
	const int *rdispls;
	MPI_Datatype sendtype;
	MPI_Datatype recvtype;
	const MPI_Datatype *sendtypes;
	const MPI_Datatype *recvtypes;
	MPI_Op op_coll;
	MPI_Info info;
} MPI_Persistant_t;

typedef struct MPI_internal_request_s
{
	mpc_common_spinlock_t lock; /**< Lock protecting the data-structure */

	mpc_lowcomm_request_t req;	/**< Request to be stored */
	int used; 	/**< Is the request slot in use */
	volatile struct MPI_internal_request_s *next;
	int rank; 	/**< Offset in the tab array from  struct \ref MPI_request_struct_s */

	/* Persitant */
	MPI_Persistant_t persistant;
        mpi_partitioned_t partitioned;
	int freeable;
	int is_active;
	int is_persistent;
	int is_partitioned;
	int is_intermediate_nbc_persistent;

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
	mpc_common_spinlock_t lock; /**< Lock protecting the data-structure */
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
mpc_lowcomm_request_t *__sctk_new_mpc_request(MPI_Request *req,
                                    MPI_request_struct_t *requests);
MPI_internal_request_t *
__sctk_convert_mpc_request_internal(MPI_Request *req,
                                    MPI_request_struct_t *requests);
mpc_lowcomm_request_t *__sctk_convert_mpc_request(MPI_Request *req,
                                        MPI_request_struct_t *requests);
void __sctk_add_in_mpc_request(MPI_Request *req, void *t,
                               MPI_request_struct_t *requests);
void __sctk_delete_mpc_request(MPI_Request *req,
                               MPI_request_struct_t *requests);

_mpc_dt_derived_t *_mpc_cl_per_mpi_process_ctx_derived_datatype_get(mpc_lowcomm_datatype_t datatype);

typedef struct {
  sctk_Op op;
  int used;
  int commute;
} sctk_op_t;

sctk_op_t *sctk_convert_to_mpc_op(MPI_Op op);


static inline int sctk_op_can_commute( sctk_op_t * op , MPI_Datatype type )
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



sctk_Op_f sctk_get_common_function(mpc_lowcomm_datatype_t datatype, sctk_Op op);

/*
  SHARED INTERNAL FUNCTIONS
*/

static inline int sctk_mpi_type_is_shared_mem(MPI_Datatype type, int count) {
  if (MPC_LOWCOMM_COMM_SHM_COLL_BUFF_MAX_SIZE <= count) {
    return 0;
  }

  if(type == MPI_INT   ||
     type == MPI_FLOAT ||
     type == MPI_CHAR  ||
     type == MPI_DOUBLE) {
      return 1;
  }
  
  return 0;
}

static inline int sctk_mpi_op_is_shared_mem(MPI_Op op) {
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

  if(type == MPI_INT) {
    for (i = 0; i < count; i++)
      b->i[i] = ((int *)src)[i];
    return;
  }
  if(type == MPI_FLOAT) {
    for (i = 0; i < count; i++)
      b->f[i] = ((float *)src)[i];
    return;
  }
  if(type == MPI_CHAR) {
    for (i = 0; i < count; i++)
      b->c[i] = ((char *)src)[i];
    return;
  }
  if(type == MPI_DOUBLE) {
    for (i = 0; i < count; i++)
      b->d[i] = ((double *)src)[i];
    return;
  }
  
  mpc_common_debug_fatal("Unsupported data-type");
}

static inline void sctk_mpi_shared_mem_buffer_get(union shared_mem_buffer *b,
                                                  MPI_Datatype type, int count,
                                                  void *dest,
                                                  size_t source_size) {
  int i;
  MPI_Aint tsize;

  if(type == MPI_INT) {
    for (i = 0; i < count; i++)
      ((int *)dest)[i] = b->i[i];
    return;
  }
  if(type == MPI_FLOAT) {
    for (i = 0; i < count; i++)
      ((float *)dest)[i] = b->f[i];
    return;
  }
  if(type == MPI_CHAR) {
    for (i = 0; i < count; i++)
      ((char *)dest)[i] = b->c[i];
    return;
  }
  if(type == MPI_DOUBLE) {
    for (i = 0; i < count; i++)
      ((double *)dest)[i] = b->d[i];
    return;
  }

  /* This is the crazy case contig to derived
   * when being packed (non uniform types) */
  PMPI_Type_extent(type, &tsize);
  int cnt = 0;
  PMPI_Unpack(b->c, source_size, &cnt, dest, count, type, MPI_COMM_WORLD);

}


int _mpc_cl_error_init();

void SCTK__MPI_INIT_REQUEST (MPI_Request * request);

int PMPI_Type_extent (MPI_Datatype, MPI_Aint *);
int PMPI_Type_size (MPI_Datatype, int *);

int NBC_Finalize(mpc_thread_t *NBC_thread);

/****************************
 * INTERNAL COMM OPERATIONS *
 ****************************/

/* All these functions do not check for negative
   tags to allow internal use */

int PMPI_Send_internal(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm);

int PMPI_Recv_internal(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status);

int PMPI_Sendrecv_internal(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int source, int recvtag, MPI_Comm comm, MPI_Status *status);

int PMPI_Isend_internal(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                        MPI_Comm comm, MPI_Request *request);

int PMPI_Irecv_internal(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request);

/* error handling */

#define MPI_ERROR_SUCCESS()    return MPI_SUCCESS

#define mpi_check_status_error(status) do{\
	if( (status)->MPI_ERROR != MPI_SUCCESS )\
	{\
		return (status)->MPI_ERROR; \
	}\
	}while(0)

#define mpi_check_status_array_error(size, statusses) \
	do{\
		int ___i;\
		for(___i = 0 ; ___i < size ; ___i++)\
		{\
			mpi_check_status_error(&statusses[___i]);\
		}\
	}while(0)

#define mpi_check_type(datatype, comm)     \
	if(datatype == MPI_DATATYPE_NULL){ \
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Bad datatype provided"); }

#define mpi_check_type_create(datatype, comm)                                                                                                                                              \
	if( (datatype >= SCTK_USER_DATA_TYPES_MAX) && (_mpc_dt_is_derived(datatype) != 1) && (_mpc_dt_is_contiguous(datatype) != 1) && ( (datatype != MPI_UB) && (datatype != MPI_LB) ) ){ \
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, ""); }

int _mpc_mpi_init_counter(int *counter);

#define mpi_check_comm(comm)                                                        \
	if( comm == MPI_COMM_WORLD ){                              \
		int ____is_init = 0; \
		int ____is_fini = 0; \
		int ___init_counter = 0; \
		_mpc_mpi_init_counter(&___init_counter); \
		PMPI_Initialized(&____is_init);     \
		PMPI_Finalized(&____is_fini);        \
		if(!( ____is_init || (0 < ___init_counter) ) || ____is_fini) { \
			MPI_ERROR_REPORT(MPC_COMM_WORLD, MPI_ERR_OTHER, "The runtime is not initialized");                     \
		}\
	} \
	else if((comm == MPI_COMM_NULL) || (!mpc_lowcomm_communicator_exists(comm)) ) { \
		MPI_ERROR_REPORT(MPC_COMM_WORLD, MPI_ERR_COMM, "Error in communicator"); \
	}

#define mpi_check_status(status, comm)  \
	if(status == MPI_STATUS_IGNORE) \
		MPI_ERROR_REPORT(comm, MPI_ERR_IN_STATUS, "Error status is MPI_STATUS_IGNORE")

#define mpi_check_buf(buf, comm)                   \
	if( (buf == NULL) && (buf != MPI_BOTTOM) ) \
		MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "")


#define mpi_check_count(count, comm) \
	if(count < 0)                \
		MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "Error count cannot be negative")

#define mpi_check_rank(task, max_rank, comm) \
	do{ \
		if( !mpc_lowcomm_communicator_is_intercomm(comm) ) {\
			if( (task != MPI_ANY_SOURCE) && (task != MPI_PROC_NULL) ) \
			{\
				if( ( (task < 0) || (task >= max_rank) )) \
				{ \
					MPI_ERROR_REPORT(comm, MPI_ERR_RANK, "Error bad rank provided"); \
				}\
			}\
		} \
	}while(0)

#define mpi_check_rank_send(task, max_rank, comm) \
	do{ \
		if( !mpc_lowcomm_communicator_is_intercomm(comm) ) {\
			if(task != MPI_PROC_NULL)\
			{\
				if( (task < 0) || (task >= max_rank) ) \
				{ \
					MPI_ERROR_REPORT(comm, MPI_ERR_RANK, "Error bad rank provided"); \
				}\
			}\
		} \
	}while(0)

#define mpi_check_root(task, max_rank, comm)                                                        \
	if( ( (task < 0) || (task >= max_rank) ) && (task != MPI_PROC_NULL) && (task != MPI_ROOT) ) \
		MPI_ERROR_REPORT(comm, MPI_ERR_ROOT, "Error bad root rank provided")

#define mpi_check_tag(tag, comm)                                   \
	do { \
		if( tag != MPI_ANY_TAG ) \
		{ \
			if( (tag < 0) || (tag > MPI_TAG_UB_VALUE) ) \
			{ \
				MPI_ERROR_REPORT(comm, MPI_ERR_TAG, "Error bad tag provided"); \
			}\
		}\
	}\
	while(0)

#define mpi_check_tag_send(tag, comm)  \
	if( (tag < 0) || (tag > MPI_TAG_UB_VALUE) ) \
		MPI_ERROR_REPORT(comm, MPI_ERR_TAG, "Error bad tag provided")

#define mpi_check_op_type_func(t)             case t: return mpi_check_op_type_func_ ## t(datatype)
#define mpi_check_op_type_func_notavail(t)    if(datatype == t) return 1

#if 0
static int mpi_check_op_type_func_MPI_(MPI_Datatype datatype)
{
	switch(datatype)
	{
		default: return 0;
	}
	return 0;
}

#endif
#define mpi_check_op_type_func_integer()          \
	mpi_check_op_type_func_notavail(MPC_LOWCOMM_INT); \
	mpi_check_op_type_func_notavail(MPC_LOWCOMM_LONG)

#define mpi_check_op_type_func_float()               \
	mpi_check_op_type_func_notavail(MPC_LOWCOMM_FLOAT);  \
	mpi_check_op_type_func_notavail(MPC_LOWCOMM_DOUBLE); \
	mpi_check_op_type_func_notavail(MPC_LOWCOMM_LONG_DOUBLE)
