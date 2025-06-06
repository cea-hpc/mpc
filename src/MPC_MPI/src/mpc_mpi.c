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
#include "mpc_mpi.h"
#include <dlfcn.h>
#include <stddef.h>
#include <string.h>

#include <mpc_mpi_internal.h>

#include <mpc_launch.h>

#include <mpc_common_datastructure.h>
#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_common_profiler.h>
#include <mpc_common_rank.h>


#include "mpc_keywords.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_topology.h"
#include "mpi_conf.h"

#include "mpc_mpi_halo.h"


#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_mpi_weak.h"
#endif

#if defined(MPC_Accelerators)
#include <mpc_thread_accelerator.h>
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_ptl_offcoll.h>
#endif

#ifdef MPC_Threads
#include <mpc_thread.h>
#endif

#include "mpc_reduction.h"
#include "mpitypes.h"

#include "mpc_launch_pmi.h"
#include "mpc_lowcomm_workshare.h"

#include "osc_mpi.h"

#include "mpit.h"

/*******************
* FORTRAN SUPPORT *
*******************/

static volatile int          did_resolve_fortran_binds = 0;
static mpc_common_spinlock_t did_resolve_fortran_binds_lock;

static inline void fortran_check_binds_resolve()
{
	if(did_resolve_fortran_binds)
	{
		return;
	}

	mpc_common_spinlock_lock_yield(&did_resolve_fortran_binds_lock);

	if(did_resolve_fortran_binds)
	{
		mpc_common_spinlock_unlock(&did_resolve_fortran_binds_lock);
		return;
	}

	void *handle = dlopen(NULL, RTLD_LAZY);

	void (*fortran_init)() =
		(void (*)() )dlsym(handle, "mpc_fortran_init_predefined_constants_");

	if(!fortran_init)
	{
		fortran_init =
			(void (*)() )dlsym(handle, "mpc_fortran_init_predefined_constants__");
	}

	if(!fortran_init)
	{
		fortran_init =
			(void (*)() )dlsym(handle, "mpc_fortran_init_predefined_constants___");
	}

	if(fortran_init)
	{
		(fortran_init)();
	}
	else
	{
		mpc_common_nodebug("No symbol");
	}

	void (*fortran08_init)() =
		(void (*)() )dlsym(handle, "mpc_fortran08_init_predefined_constants_");

	if(!fortran08_init)
	{
		fortran08_init =
			(void (*)() )dlsym(handle, "mpc_fortran08_init_predefined_constants__");
	}

	if(!fortran08_init)
	{
		fortran08_init =
			(void (*)() )dlsym(handle, "mpc_fortran08_init_predefined_constants___");
	}

	if(fortran08_init)
	{
		(fortran08_init)();
	}
	else
	{
		mpc_common_nodebug("No symbol08");
	}

	dlclose(handle);

	did_resolve_fortran_binds = 1;

	mpc_common_spinlock_unlock(&did_resolve_fortran_binds_lock);
}

/******************
* ERROR HANDLING *
******************/

int _mpc_mpi_report_error(mpc_lowcomm_communicator_t comm, int error, char *message, char *function, char *file, int line)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	MPI_Errhandler errh = (MPI_Errhandler)_mpc_mpi_handle_get_errhandler(
		(sctk_handle)comm, _MPC_MPI_HANDLE_COMM);

	if(errh != MPI_ERRHANDLER_NULL)
	{
		mpc_common_nodebug("ERRH is %d for %d", errh, comm);
		MPI_Handler_function *func = _mpc_mpi_errhandler_resolve(errh);
		int      error_id          = error;
		MPI_Comm comm_id           = __mpc_lowcomm_communicator_to_predefined(comm);

		mpc_common_nodebug("CALL %p (%d)", func, errh);
		if(func)
		{
			(func)(&comm_id, &error_id, message, function, file, line);
		}
		_mpc_mpi_errhandler_free(errh);
	}
	return error;
}

/*
 * INTERNAL FUNCTIONS
 */


static int __PMPI_Type_contiguous_inherits(unsigned long, MPI_Datatype,
                                           MPI_Datatype *, struct _mpc_dt_context *ctx);

/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_allgather_graph(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(const void *, int, MPI_Datatype, void *, const int [], const int [], MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(const void *, int, MPI_Datatype, void *, const int [], const int [], MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_alltoall_cart(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_alltoall_graph(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(const void *, const int [], const int [], MPI_Datatype, void *, const int [], const int [], MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(const void *, const int [], const int [], MPI_Datatype, void *, const int [], const int [], MPI_Datatype, MPI_Comm);
static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(const void *, const int [], const MPI_Aint [], const MPI_Datatype [], void *, const int [], const MPI_Aint [], const MPI_Datatype [], MPI_Comm comm);
static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(const void *, const int [], const MPI_Aint [], const MPI_Datatype [], void *, const int [], const MPI_Aint [], const MPI_Datatype [], MPI_Comm comm);


static inline int __PMPI_Cart_rank_locked(MPI_Comm, const int *, int *);
static int __PMPI_Cart_coords_locked(MPI_Comm, int, int, int *);

struct MPI_request_struct_s;
static int __Isend_test_req(const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm,
                            MPI_Request *request, int is_valid_request);

/* checkpoint */

typedef struct
{
	void *attr;
	int   flag;
} MPI_Caching_key_value_t;

typedef struct
{
	int  ndims;
	int *dims;
	int *periods;
	int  reorder;
} mpi_topology_cart_t;

typedef struct
{
	int  nnodes;
	int  nedges;
	int  nindex;

	int *edges;
	int *index;
	int  reorder;
} mpi_topology_graph_t;

typedef union
{
	mpi_topology_cart_t  cart;
	mpi_topology_graph_t graph;
} mpi_topology_info_t;

typedef struct
{
	int                   type;
	mpi_topology_info_t   data;
	char                  names[MPI_MAX_NAME_STRING + 1];
	mpc_common_spinlock_t lock;
} mpi_topology_per_comm_t;

#define MAX_TOPO_DEPTH    10

typedef struct mpc_mpi_per_communicator_s
{
	/****** Attributes ******/
	MPI_Caching_key_value_t *key_vals;
	int                      max_number;

	/****** Topologies ******/
	mpi_topology_per_comm_t  topo;

	/****** LOCK ******/
	mpc_common_spinlock_t    lock;
}mpc_mpi_per_communicator_t;

#define MPC_MPI_MAX_NUMBER_FUNC    3

struct MPI_request_struct_s;
struct MPI_group_struct_s;
typedef struct MPI_group_struct_s   MPI_group_struct_t;
struct _mpc_mpi_buffer;
typedef struct _mpc_mpi_buffer      MPI_buffer_struct_t;
struct sctk_mpi_ops_s;
typedef struct sctk_mpi_ops_s       sctk_mpi_ops_t;

MPI_internal_request_t *mpc_mpi_request_empty = MPI_REQUEST_NULL;

//FIXME: change name
void PMPI_internal_free_request(MPI_Request *request) {
        MPI_internal_request_t *req = *request;
        assert(*request);

        req->is_nbc = 0;
        mpc_lowcomm_request_free(_mpc_cl_get_lowcomm_request(req));

        *request = MPI_REQUEST_NULL;
}

static
void mpc_mpi_per_communicator_copy_func(mpc_mpi_per_communicator_t **to, mpc_mpi_per_communicator_t *from)
{
	int i = 0;

	mpc_common_spinlock_lock(&(from->lock) );
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s) );
	memcpy(*to, from, sizeof(mpc_mpi_per_communicator_t) );
	( (*to)->key_vals) = sctk_malloc(from->max_number * sizeof(MPI_Caching_key_value_t) );
	for(i = 0; i < from->max_number; i++)
	{
		( (*to)->key_vals[i].attr) = NULL;
		( (*to)->key_vals[i].flag) = 0;
	}

	/* Reset TOPO */
	mpc_common_spinlock_init(&(*to)->topo.lock, 0);
	(*to)->topo.type = MPI_UNDEFINED;

	mpc_common_spinlock_unlock(&(from->lock) );
	mpc_common_spinlock_unlock(&( (*to)->lock) );
}

static
void mpc_mpi_per_communicator_dup_copy_func(mpc_mpi_per_communicator_t **to, mpc_mpi_per_communicator_t *from)
{
	mpc_common_spinlock_lock(&(from->lock) );
	*to = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s) );
	memcpy(*to, from, sizeof(mpc_mpi_per_communicator_t) );

	/* Reset TOPO */
	mpc_common_spinlock_init(&(*to)->topo.lock, 0);
	(*to)->topo.type = MPI_UNDEFINED;

	mpc_common_spinlock_unlock(&(from->lock) );
	mpc_common_spinlock_unlock(&( (*to)->lock) );
}

static inline mpc_mpi_per_communicator_t * __get_per_comm_data(mpc_lowcomm_communicator_t comm)
{
	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific;
	mpc_per_communicator_t *tmp;


	task_specific = mpc_cl_per_mpi_process_ctx_get();
	tmp           = _mpc_cl_per_communicator_get(task_specific, comm);

	if(tmp == NULL)
	{
		return NULL;
	}

	return tmp->mpc_mpi_per_communicator;
}

static void __sctk_init_mpi_topo_per_comm(mpc_mpi_per_communicator_t *tmp)
{
	tmp->topo.type = MPI_UNDEFINED;
	sprintf(tmp->topo.names, "undefined");
}

int mpc_mpi_per_communicator_init(mpc_mpi_per_communicator_t *pc)
{
	__sctk_init_mpi_topo_per_comm(pc);
	pc->max_number = 0;
	mpc_common_spinlock_init(&pc->topo.lock, 0);

	return 0;
}

static inline mpc_mpi_data_t *__get_per_task_data()
{
	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific;

	task_specific = mpc_cl_per_mpi_process_ctx_get();
	return task_specific->mpc_mpi_data;
}

static inline void PMPC_Get_buffers(struct _mpc_mpi_buffer **buffers)
{
	*buffers = __get_per_task_data()->buffers;
}

static inline void PMPC_Set_buffers(struct _mpc_mpi_buffer *buffers)
{
	__get_per_task_data()->buffers = buffers;
}

static inline void PMPC_Get_op(struct sctk_mpi_ops_s **ops)
{
	*ops = __get_per_task_data()->ops;
}

static inline void PMPC_Set_op(struct sctk_mpi_ops_s *ops)
{
	__get_per_task_data()->ops = ops;
}

/** Fast yield logic */

static volatile int __do_yield = 0;

static inline void __force_yield()
{
	__do_yield |= 1;
}

/** Do we need to yield in this process for collectives (overloaded)
 */
static inline void sctk_init_yield_as_overloaded()
{
	if(mpc_topology_get_pu_count() < mpc_common_get_local_task_count() )
	{
		__do_yield = 1;
	}

	if(1 < mpc_common_get_process_count() )
	{
		// We need to progress messages
		__do_yield = 1;
	}
}

/* Error Handling */

static void __sctk_init_mpi_errors()
{
	_mpc_cl_error_init();
}

static int mpi_check_op_type_func_MPI_SUM(MPI_Datatype datatype)
{
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_MAX(MPI_Datatype datatype)
{
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_MIN(MPI_Datatype datatype)
{
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_PROD(MPI_Datatype datatype)
{
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_LAND(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_BAND(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();

	return 0;
}

static int mpi_check_op_type_func_MPI_LOR(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_BOR(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();

	return 0;
}

static int mpi_check_op_type_func_MPI_LXOR(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_BXOR(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();

	return 0;
}

static int mpi_check_op_type_func_MPI_MINLOC(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();
	mpi_check_op_type_func_integer();
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type_func_MPI_MAXLOC(MPI_Datatype datatype)
{
	mpi_check_op_type_func_float();
	mpi_check_op_type_func_integer();
	mpi_check_op_type_func_notavail(MPI_BYTE);

	return 0;
}

static int mpi_check_op_type(MPI_Op op, MPI_Datatype datatype)
{
#if 1
	if( (op <= MPI_MAXLOC) && (mpc_lowcomm_datatype_is_common(datatype) ) )
	{
		switch(op)
		{
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

#define mpi_check_op(op, type, comm)                             \
	if( (op == MPI_OP_NULL) || mpi_check_op_type(op, type) ) \
	MPI_ERROR_REPORT(comm, MPI_ERR_OP, "")

static int SCTK__MPI_Attr_clean_communicator(MPI_Comm comm);
static int SCTK__MPI_Attr_communicator_dup(MPI_Comm old, MPI_Comm new);


#if 0
MPI_request_struct_t *__sctk_internal_get_MPC_requests()
{
	MPI_request_struct_t *requests;

	PMPC_Get_requests( (void *)&requests);
	return requests;
}

/** \brief Initialize MPI interface request handling */
void __sctk_init_mpc_request()
{
	static mpc_thread_mutex_t sctk_request_lock = SCTK_THREAD_MUTEX_INITIALIZER;
	MPI_request_struct_t *    requests = NULL;

	/* Check wether requests are already initalized */
	PMPC_Get_requests( (void *)&requests);
	assume(requests == NULL);

	mpc_thread_mutex_lock(&sctk_request_lock);

	/* Allocate the request structure */
	requests = sctk_malloc(sizeof(MPI_request_struct_t) );

	/*Init request struct */
	mpc_common_spinlock_init(&requests->lock, 0);
	requests->tab            = NULL;
	requests->free_list      = NULL;
	requests->auto_free_list = NULL;
	requests->max_size       = 0;
	/* Create the associated buffered allocator */
	sctk_buffered_alloc_create(&(requests->buf), sizeof(MPI_internal_request_t) );

	/* Register the new array in the task specific data-structure */
	PMPC_Set_requests(requests);

	mpc_thread_mutex_unlock(&sctk_request_lock);
}

inline void sctk_delete_internal_request_clean(MPI_internal_request_t *tmp)
{
	/* Release the internal request */
	tmp->used = 0;
        if (tmp->saved_datatype != NULL) {
	        sctk_free(tmp->saved_datatype);
        }
	tmp->saved_datatype = NULL;
}

#define __sctk_new_mpc_request_internal_local_get(a, b)    NULL
#define sctk_delete_internal_request_local_put(a, b)       0

/** \brief Delete an internal request and put it in the free-list */
inline void sctk_delete_internal_request(MPI_internal_request_t *tmp, MPI_request_struct_t *requests)
{
	sctk_delete_internal_request_clean(tmp);

	/* Push in in the head of the free list */
	tmp->next           = requests->free_list;
	requests->free_list = tmp;
}

/** \brief Walk the auto-free list in search for a terminated call (only head)
 *
 *  \param requests a pointer to the request array structure
 * */
inline void sctk_check_auto_free_list(MPI_request_struct_t *requests)
{
	/* If there is an auto free list */
	if(requests->auto_free_list != NULL)
	{
		MPI_internal_request_t *tmp = NULL;
		int flag = 0;

		/* Take HEAD */
		tmp = (MPI_internal_request_t *)requests->auto_free_list;

		/* Test it */
		mpc_lowcomm_test(&(tmp->req), &flag, MPC_LOWCOMM_STATUS_NULL);

		/* If call has ended */
		if(flag != 0)
		{
			MPI_internal_request_t *tmp_new = NULL;

			/* Remove HEAD from the auto-free list */
			tmp_new = (MPI_internal_request_t *)tmp->next;
			requests->auto_free_list = tmp_new;

			/* Free the request and put it in the free list */
			sctk_delete_internal_request(tmp, requests);
		}
	}
}

/** \brief Create a new \ref MPI_internal_request_t
 *
 *  \param req Request to allocate (will be written with the ID of the request)
 * */
/* extern inline */
MPI_internal_request_t *__sctk_new_mpc_request_internal(MPI_Request *req,
                                                        MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp = NULL;

	tmp = __sctk_new_mpc_request_internal_local_get(req, requests);

	if(tmp == NULL)
	{
		/* Lock the request struct */
		mpc_common_spinlock_lock(&(requests->lock) );

		/* Try to free the HEAD of the auto free list */
		sctk_check_auto_free_list(requests);

		/* If the free list is empty */
		if(requests->free_list == NULL)
		{
			int old_size = 0;
			int i = 0;

			/* Previous size */
			old_size = requests->max_size;
			/* New incremented size */
			requests->max_size += 5;
			/* Allocate TAB */
			requests->tab = sctk_realloc(requests->tab, requests->max_size * sizeof(MPI_internal_request_t *) );

			assume(requests->tab != NULL);

			/* Ensure at least one loop iteration is performed,
			   otherwise tmp is NULL and dereferenced later. */
			assume(old_size < requests->max_size);

			/* Fill in the new array slots */
			for(i = old_size; i < requests->max_size; i++)
			{
				mpc_common_nodebug("%lu", i);
				/* Allocate the MPI_internal_request_t */
				tmp = sctk_buffered_malloc(&(requests->buf), sizeof(MPI_internal_request_t) );
				assume(tmp != NULL);

				/* Save it in the array */
				requests->tab[i] = tmp;

				/* Set not used */
				tmp->used = 0;
				mpc_common_spinlock_init(&tmp->lock, 0);
				tmp->task_req_id = requests;

				/* Put the newly allocated slot in the free list */
				tmp->next           = requests->free_list;
				requests->free_list = tmp;

				/* Register its offset in the tab array */
				tmp->rank           = i;
				tmp->saved_datatype = NULL;
			}
		}

		/* Now we should have a request in the free list */

		/* Take head from the free list */
		tmp = (MPI_internal_request_t *)requests->free_list;

		/* Remove from free list */
		requests->free_list = tmp->next;

		mpc_common_spinlock_unlock(&(requests->lock) );
	}

	/* Mark it as used */
	tmp->used = 1;
	/* Mark it as freable */
	tmp->freeable = 1;
	/* Mark it as active */
	tmp->is_active = 1;
	/* Disable auto-free */
	tmp->auto_free      = 0;
	tmp->saved_datatype = NULL;

	/* Mark it as not an nbc */
	tmp->is_nbc = 0;
	/* Mark it as not a persistant */
	tmp->is_persistent = 0;
	/* Mark it as not a intermediate nbc persistant */
	tmp->is_intermediate_nbc_persistent = 0;

	/* Set request to be the id in the tab array (rank) */
	*req = tmp->rank;

	return tmp;
}

/** \brief Create a new \ref mpc_lowcomm_request_t */
/* extern inline */
mpc_lowcomm_request_t *__sctk_new_mpc_request(MPI_Request *req, MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	/* Acquire a free MPI_Internal request */
	tmp = __sctk_new_mpc_request_internal(req, requests);

	/* Return its inner mpc_lowcomm_request_t */
	return &(tmp->req);
}

/** \brief Convert an \ref MPI_Request to an \ref MPI_internal_request_t
 *
 *      \param req Request to convert to an \ref MPI_internal_request_t
 *  \return a pointer to the \MPI_internal_request_t associated with req NULL if not found
 *  */
MPI_internal_request_t *
__sctk_convert_mpc_request_internal(MPI_Request *req,
                                    MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	/* Retrieve the interger ID of this request */
	int int_req = *req;

	/* If it is request NULL we have nothing to get */
	if(int_req == MPI_REQUEST_NULL)
	{
		return NULL;
	}

	/* Retrieve the request array */
	assume(requests != NULL);

	/* Check bounds */
	assume( ( (int_req) >= 0) && ( (int_req) < requests->max_size) );

	/* Lock it */
	mpc_common_spinlock_lock(&(requests->lock) );

	mpc_common_nodebug("Convert request %d", *req);

	/* Directly get the request in the tab */
	tmp = requests->tab[int_req];

	/* Unlock the request array */
	mpc_common_spinlock_unlock(&(requests->lock) );

	assume(tmp->task_req_id == requests);

	/* Is rank coherent with the offset */
	assume(tmp->rank == *req);

	/* Is this request in used */
	/*assume(tmp->used); // Seems to conflict while using progress thread */

	/* Return the MPI_internal_request_t * */
	return tmp;
}

/** \brief Convert an MPI_Request to an MPC_Request
 * \param req Request to convert to an \ref MPC_Request
 * \return a pointer to the mpc_lowcomm_request_t NULL if not found
 */
inline mpc_lowcomm_request_t *__sctk_convert_mpc_request(MPI_Request *req, MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	/* Resolve the MPI_internal_request_t */
	tmp = __sctk_convert_mpc_request_internal(req, requests);

	/* If there was no MPI_internal_request_t or it was unactive */
	if( (tmp == NULL) || (tmp->is_active == 0) )
	{
		/* Return the null request */
		return &MPC_REQUEST_NULL;
	}

	/* Return the mpc_lowcomm_request_t field */
	return &(tmp->req);
}

/** Brief save a data-type in a request
 *
 *  \param req Target request
 *  \param t Datatype to store
 *
 * */
inline void __sctk_add_in_mpc_request(MPI_Request *req, void *t, MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	tmp = __sctk_convert_mpc_request_internal(req, requests);
	tmp->saved_datatype = t;
}

/** Delete a request
 *  \param req Request to delete
 */
inline void __sctk_delete_mpc_request(MPI_Request *req,
                                      MPI_request_struct_t *requests)
{
	MPI_internal_request_t *tmp;

	/* Convert resquest to an integer */
	int int_req = *req;

	/* If it is request null there is nothing to do */
	if(int_req == MPI_REQUEST_NULL)
	{
		return;
	}

	assume(requests != NULL);
	/* Lock it */
	mpc_common_nodebug("Delete request %d", *req);

	/* Retrieve the request */
	tmp = __sctk_convert_mpc_request_internal(req, requests);

	//memset(&tmp->req, 0, sizeof(mpc_lowcomm_request_t) );

	/* Clear the request */
	mpc_common_spinlock_lock(&(tmp->lock) );

	/* if request is not active disable auto-free */
	if(tmp->is_active == 0)
	{
		tmp->auto_free = 0;
	}

	/* Deactivate the request */
	tmp->is_active = 0;

	/* Can the request be freed ? */
	if(tmp->freeable == 1)
	{
		/* Make sure the rank matches the TAB offset */
		assume(tmp->rank == *req);

		/* Auto free ? */
		if(tmp->auto_free == 0)
		{
			/* Call delete internal request to push it in the free list */
			if(sctk_delete_internal_request_local_put(tmp, requests) == 0)
			{
				mpc_common_spinlock_lock(&(requests->lock) );
				sctk_delete_internal_request(tmp, requests);
				mpc_common_spinlock_unlock(&(requests->lock) );
			}
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		}
		else
		{
			/* Remove it from the free list */
			mpc_common_spinlock_lock(&(requests->lock) );
			tmp->next = requests->auto_free_list;
			requests->auto_free_list = tmp;
			mpc_common_spinlock_unlock(&(requests->lock) );
			/* Set the source request to NULL */
			*req = MPI_REQUEST_NULL;
		}
	}
	mpc_common_spinlock_unlock(&(tmp->lock) );
}
#endif

/************************************************************************/
/* Halo storage Handling                                                */
/************************************************************************/

static volatile int   __sctk_halo_initialized      = 0;
mpc_common_spinlock_t __sctk_halo_initialized_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static struct sctk_mpi_halo_context __sctk_halo_context;

/** \brief Halo Context getter for MPI */
static inline struct sctk_mpi_halo_context *sctk_halo_context_get()
{
	return &__sctk_halo_context;
}

/** \brief Function called in \ref MPI_Init Initialized halo context once
 */
static inline void __sctk_init_mpc_halo()
{
	mpc_common_spinlock_lock(&__sctk_halo_initialized_lock);

	if(__sctk_halo_initialized)
	{
		mpc_common_spinlock_unlock(&__sctk_halo_initialized_lock);
		return;
	}

	sctk_mpi_halo_context_init(sctk_halo_context_get() );

	__sctk_halo_initialized = 1;

	mpc_common_spinlock_unlock(&__sctk_halo_initialized_lock);
}

/*
 * BUFFERS
 */

typedef struct _mpc_mpi_buffer
{
	void *                buffer;
	int                   size;
	mpc_common_spinlock_t lock;
} mpi_buffer_t;

static inline void __buffer_init()
{
	mpi_buffer_t *tmp;

	tmp         = sctk_malloc(sizeof(mpi_buffer_t) );
	tmp->size   = 0;
	tmp->buffer = NULL;
	mpc_common_spinlock_init(&tmp->lock, 0);
	PMPC_Set_buffers(tmp);
}

static inline mpi_buffer_overhead_t *__buffer_next_header(mpi_buffer_overhead_t *head, mpi_buffer_t *tmp)
{
	mpi_buffer_overhead_t *head_next;

	head_next =
		(mpi_buffer_overhead_t *)( (char *)head +
		                           (sizeof(mpi_buffer_overhead_t) + head->size) );
	if( (unsigned long)head_next >= (unsigned long)tmp->buffer + tmp->size)
	{
		head_next = NULL;
	}

	return head_next;
}

static inline mpi_buffer_overhead_t *__buffer_compact(int size, mpi_buffer_t *tmp)
{
	mpi_buffer_overhead_t *found = NULL;
	mpi_buffer_overhead_t *head;
	mpi_buffer_overhead_t *head_next;

	int flag;

	head = (mpi_buffer_overhead_t *)(tmp->buffer);
/*      fprintf(stderr,"Min %p max %p\n",tmp->buffer , ((char*)tmp->buffer) + tmp->size); */

	while(head != NULL)
	{
/*        fprintf(stderr,"%p %d\n",head,head->request); */
		if(head->request != MPI_REQUEST_NULL)
		{
			PMPI_Test(&(head->request), &flag, MPI_STATUS_IGNORE);
			assume( (head->request == MPI_REQUEST_NULL) || (flag == 0) );
		}
		if(head->request == MPI_REQUEST_NULL)
		{
			mpc_common_nodebug("1 : %p freed", head);
			head_next = __buffer_next_header(head, tmp);

			/*Compact from head */
			while(head_next != NULL)
			{
/*        fprintf(stderr,"NEXT %p %d\n",head_next,head_next->request); */
				if(head_next->request != MPI_REQUEST_NULL)
				{
					PMPI_Test(&(head_next->request), &flag, MPI_STATUS_IGNORE);
					assume( (head->request == MPI_REQUEST_NULL) || (flag == 0) );
				}
				if(head_next->request == MPI_REQUEST_NULL)
				{
					mpc_common_nodebug("2 : %p freed", head_next);
					head->size = head->size + head_next->size + sizeof(mpi_buffer_overhead_t);
					mpc_common_nodebug("MERGE Create new buffer of size %d (%d + %d)", head->size, head_next->size, head->size - head_next->size - sizeof(mpi_buffer_overhead_t) );
				}
				else
				{
					break;
				}

				head_next = __buffer_next_header(head, tmp);
			}
		}

		if( (head->request == MPI_REQUEST_NULL) && (head->size >= size) /* && (found == NULL) */)
		{
			if(found != NULL)
			{
				if(head->size < found->size)
				{
					found = head;
				}
			}
			else
			{
				found = head;
			}
		}

		head_next = __buffer_next_header(head, tmp);
		head      = head_next;
	}
	return found;
}

static int __Ibsend_test_req(const void *buf, int count, MPI_Datatype datatype,
                             int dest, int tag, MPI_Comm comm,
                             MPI_Request *request, int is_valid_request)
{
	mpi_buffer_t *         tmp;
	int                    size;
	int                    res;
	mpi_buffer_overhead_t *head;
	mpi_buffer_overhead_t *head_next;
	void *                 head_buf;
	mpi_buffer_overhead_t *found = NULL;

	//~ get the pack size

	res = PMPI_Pack_size(count, datatype, comm, &size);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(size % sizeof(mpi_buffer_overhead_t) )
	{
		size +=
			sizeof(mpi_buffer_overhead_t) -
			(size % sizeof(mpi_buffer_overhead_t) );
	}
	assume(size % sizeof(mpi_buffer_overhead_t) == 0);

	mpc_common_nodebug("MSG size %d", size);

	PMPC_Get_buffers(&tmp);
	mpc_common_spinlock_lock_yield(&(tmp->lock) );

	if(tmp->buffer == NULL)
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "No buffer available");
	}

	found = __buffer_compact(size, tmp);

	mpc_common_nodebug("found = %d", found);

	if(found)
	{
		int position = 0;
		head     = found;
		head_buf = (char *)head + sizeof(mpi_buffer_overhead_t);

		assume(head->request == MPI_REQUEST_NULL);

		if(head->size >= size + (int)sizeof(mpi_buffer_overhead_t) )
		{
			int old_size;
			old_size           = head->size;
			head->size         = size;
			head_next          = __buffer_next_header(head, tmp);
			head_next->size    = old_size - size - sizeof(mpi_buffer_overhead_t);
			head_next->request = MPI_REQUEST_NULL;
		}

		res = PMPI_Pack(buf, count, datatype, head_buf, head->size, &position, comm);

		if(res != MPI_SUCCESS)
		{
			mpc_common_spinlock_unlock(&(tmp->lock) );
			return res;
		}

		assume(position <= size);


		res = __Isend_test_req(head_buf, position, MPI_PACKED, dest, tag, comm, &(head->request), 0);

		/*       fprintf(stderr,"Add request %d %d\n",head->request,res); */

		if(res != MPI_SUCCESS)
		{
			mpc_common_spinlock_unlock(&(tmp->lock) );
			return res;
		}

		if(is_valid_request == 1)
		{
			_mpc_cl_get_lowcomm_request(*request)->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
		}
		else
		{
			//	*request = MPI_REQUEST_NULL;
                        *request = (MPI_internal_request_t *)mpc_lowcomm_request_alloc();
			_mpc_cl_get_lowcomm_request(request)->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
		}
	}
	else
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "No space left in buffer");
	}

	mpc_common_spinlock_unlock(&(tmp->lock) );
	return MPI_SUCCESS;
}

static int __Isend_test_req(const void *buf, int count, MPI_Datatype datatype,
                            int dest, int tag, MPI_Comm comm,
                            MPI_Request *request, int is_valid_request)
{
	if(!_mpc_dt_is_contig_mem(datatype) && (count != 0) )
	{
		int res;

		if(count > 1)
		{
			MPI_Datatype new_datatype;
			res = PMPI_Type_contiguous(count, datatype, &new_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res = PMPI_Type_commit(&new_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res =
				__Isend_test_req(buf, 1, new_datatype, dest, tag,
				                 comm, request, is_valid_request);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res = PMPI_Type_free(&new_datatype);
			return res;
		}
		else
		{
			int derived_ret = 0;
			_mpc_lowcomm_general_datatype_t derived_datatype;

			res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &derived_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			if(is_valid_request)
			{
				res = mpc_mpi_cl_open_pack(_mpc_cl_get_lowcomm_request(*request));
			}
			else
			{
                                *request = mpc_lowcomm_request_alloc();
				res = mpc_mpi_cl_open_pack(_mpc_cl_get_lowcomm_request(*request));
			}
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			{
				long *tmp;
				tmp = sctk_malloc(derived_datatype.opt_count * 2 * sizeof(long) );
                                (*request)->saved_datatype = tmp;

				memcpy(tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof(long) );
				memcpy(&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof(long) );

				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends   = &(tmp[derived_datatype.opt_count]);
			}

			res = mpc_mpi_cl_add_pack_absolute(buf, derived_datatype.opt_count,
                                                           derived_datatype.opt_begins, derived_datatype.opt_ends,
                                                           MPC_LOWCOMM_CHAR, _mpc_cl_get_lowcomm_request(*request));

			if(res != MPI_SUCCESS)
			{
				return res;
			}

			res = mpc_mpi_cl_isend_pack(dest, tag, comm, _mpc_cl_get_lowcomm_request(*request));
			return res;
		}
	}
	else
	{
		if(is_valid_request)
		{
			return _mpc_cl_isend(buf, count, datatype, dest,
                                             tag, comm, _mpc_cl_get_lowcomm_request(*request));
		}
		else
		{
                        *request = (MPI_internal_request_t *)mpc_lowcomm_request_alloc();
			return _mpc_cl_isend(buf, count, datatype, dest, tag, comm,
                                             _mpc_cl_get_lowcomm_request(*request));
		}
	}
}

static int __Irecv_test_req(void *buf, int count, MPI_Datatype datatype,
                            int source, int tag, MPI_Comm comm,
                            MPI_Request *request, int is_valid_request)
{
	if(!_mpc_dt_is_contig_mem(datatype) )
	{
		int res;

		if(count > 1)
		{
			MPI_Datatype new_datatype;
			res = PMPI_Type_contiguous(count, datatype, &new_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res = PMPI_Type_commit(&new_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res =
				__Irecv_test_req(buf, 1, new_datatype, source,
				                 tag, comm, request,
				                 is_valid_request);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			res = PMPI_Type_free(&new_datatype);
			return res;
		}
		else
		{
			int derived_ret = 0;
			_mpc_lowcomm_general_datatype_t derived_datatype;

			res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &derived_datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			if(!is_valid_request)
			{
                                *request = mpc_lowcomm_request_alloc();
			}

                        res = mpc_mpi_cl_open_pack(_mpc_cl_get_lowcomm_request(*request));

			if(res != MPI_SUCCESS)
			{
				return res;
			}

			{
				long *tmp;
				tmp = sctk_malloc(derived_datatype.opt_count * 2 * sizeof(long) );
                                (*request)->saved_datatype = tmp;

				memcpy(tmp, derived_datatype.opt_begins, derived_datatype.opt_count * sizeof(long) );
				memcpy(&(tmp[derived_datatype.opt_count]), derived_datatype.opt_ends, derived_datatype.opt_count * sizeof(long) );

				derived_datatype.opt_begins = tmp;
				derived_datatype.opt_ends   = &(tmp[derived_datatype.opt_count]);
			}

			res =
				mpc_mpi_cl_add_pack_absolute(buf, derived_datatype.opt_count, derived_datatype.opt_begins,
                                                             derived_datatype.opt_ends, MPC_LOWCOMM_CHAR,
                                                             _mpc_cl_get_lowcomm_request(*request));
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			res = mpc_mpi_cl_irecv_pack(source, tag, comm, _mpc_cl_get_lowcomm_request(*request));
			return res;
		}
	}
	else
	{
		if(!is_valid_request)
		{
                        *request = mpc_lowcomm_request_alloc();
		}

		return _mpc_cl_irecv(buf, count, datatype, source, tag, comm,
		             _mpc_cl_get_lowcomm_request(*request));

	}
}

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/

int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                        MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request)
{
        MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();

	return _mpc_cl_grequest_start(query_fn, free_fn, cancel_fn, extra_state,
                                      _mpc_cl_get_lowcomm_request(req));
}

int PMPI_Grequest_complete(MPI_Request request)
{
        MPI_internal_request_t *req = request;
	return _mpc_cl_grequest_complete(*(_mpc_cl_get_lowcomm_request(req)));
}

/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

int PMPIX_Grequest_start(MPI_Grequest_query_function *query_fn,
                         MPI_Grequest_free_function *free_fn,
                         MPI_Grequest_cancel_function *cancel_fn,
                         MPIX_Grequest_poll_fn *poll_fn,
                         void *extra_state,
                         MPI_Request *request)
{
        *request = mpc_lowcomm_request_alloc();

	return _mpc_cl_egrequest_start(query_fn, free_fn, cancel_fn, poll_fn, extra_state,
                                       _mpc_cl_get_lowcomm_request(*request));
}

/************************************************************************/
/* Extended Generalized Requests Class                                  */
/************************************************************************/

int PMPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
                                MPI_Grequest_free_function *free_fn,
                                MPI_Grequest_cancel_function *cancel_fn,
                                MPIX_Grequest_poll_fn *poll_fn,
                                MPIX_Grequest_wait_fn *wait_fn,
                                MPIX_Grequest_class *new_class)
{
	return _mpc_cl_grequest_class_create(query_fn, cancel_fn, free_fn, poll_fn, wait_fn, new_class);
}

int PMPIX_Grequest_class_allocate(MPIX_Grequest_class target_class, void *extra_state, MPI_Request *request)
{
        *request = mpc_lowcomm_request_alloc();

	return _mpc_cl_grequest_class_allocate(target_class, extra_state,
                                               _mpc_cl_get_lowcomm_request(*request));
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/** \brief This function creates a contiguous MPI_Datatype with possible CTX inheritance
 *
 *  Such datatype is obtained by directly appending  count copies of data_in type
 *  HERE CONTEXT CAN BE OVERRIDED (for example to be a contiguous vector)
 *
 *  \param count Number of elements of type data_in in the data_out type
 *  \param data_in The type to be replicated count times
 *  \param data_out The output type
 *  \param ctx THe context is is inherited from (in case another datatype is built-on top of it)
 *
 */
static inline int __PMPI_Type_contiguous_inherits(unsigned long count, MPI_Datatype data_in, MPI_Datatype *data_out, struct _mpc_dt_context *ref_dtctx)
{
/* Set its context */
	struct _mpc_dt_context *dtctx = NULL;

	struct _mpc_dt_context local_dtctx;

	_mpc_dt_context_clear(&local_dtctx);
	local_dtctx.combiner = MPI_COMBINER_CONTIGUOUS;
	local_dtctx.count    = count;
	local_dtctx.oldtype  = data_in;

	if(ref_dtctx == NULL)
	{
		dtctx = &local_dtctx;
	}
	else
	{
		dtctx = ref_dtctx;
	}

	/* If source datatype is a derived datatype we have to create a new derived datatype */
	if(!_mpc_dt_is_contig_mem(data_in) )
	{
		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		_mpc_cl_general_datatype_try_get_info(data_in, &derived_ret, &input_datatype);

		/* Compute the total datatype size including boundaries */
		unsigned long extent;
		PMPI_Type_extent(data_in, (MPI_Aint *)&extent);

		/* By definition the number of output types is count times the one of the input type */
		unsigned long count_out = input_datatype.count * count;

		/* Allocate datatype description */
		long *begins_out = sctk_malloc(count_out * sizeof(long) );
		long *ends_out   = sctk_malloc(count_out * sizeof(long) );
		mpc_lowcomm_datatype_t *datatypes = sctk_malloc(count_out * sizeof(mpc_lowcomm_datatype_t) );

		if(!begins_out || !ends_out || !datatypes)
		{
			MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_INTERN, "Failled to allocate type context");
		}

		/* Fill in datatype description for a contiguous type */
		unsigned long i;

		/* Input : ABC of length count_in
		 * Output : ABCABCABC... of length count_out
		 */

		long new_ub = input_datatype.ub;
		long new_lb = input_datatype.lb;
		long next_ub, next_lb, cur_ub, cur_lb;

		next_ub = input_datatype.ub;
		next_lb = input_datatype.lb;

		for(i = 0; i < count_out; i++)
		{
			cur_ub = next_ub;
			cur_lb = next_lb;

			begins_out[i] = input_datatype.begins[i % input_datatype.count]    /* Original begin offset in the input block */
			                + extent * (i / input_datatype.count);             /* New offset due to type replication */

			ends_out[i] = input_datatype.ends[i % input_datatype.count]        /* Original end offset in the input block */
			              + extent * (i / input_datatype.count);               /* New offset due to type replication */

			datatypes[i] = input_datatype.datatypes[i % input_datatype.count];

			if(i % input_datatype.count == input_datatype.count - 1)
			{
				next_ub = cur_ub + extent;
				next_lb = cur_ub;

				if(cur_ub > new_ub)
				{
					new_ub = cur_ub;
				}
				if(cur_lb < new_lb)
				{
					new_lb = cur_lb;
				}
			}


			mpc_common_nodebug("%d , %lu-%lu <- %lu-%lu", i, begins_out[i],
			                   ends_out[i], input_datatype.begins[i % input_datatype.count],
			                   input_datatype.ends[i % input_datatype.count]);
		}

		/* Handle the NULL count case */
		if(!count)
		{
			new_ub = 0;
			new_lb = 0;
		}

		/* Actually create the new datatype */
		_mpc_cl_general_datatype(data_out, begins_out, ends_out, datatypes, count_out, new_lb, input_datatype.is_lb, new_ub, input_datatype.is_ub, dtctx);

		/* Free temporary buffers */
		sctk_free(datatypes);
		sctk_free(begins_out);
		sctk_free(ends_out);
	}
	else
	{
		/* Here we handle contiguous or common datatypes which can be replicated directly */
		_mpc_cl_type_hcontiguous_ctx(data_out, count, &data_in, dtctx);
	}

	return MPI_SUCCESS;
}

/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 *      \param count Number of blocks
 *      \param blocklength length of every blocks
 *      \param indices Offset of each block in old_type size multiples
 *      \param old_type Input data-type
 *      \param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Here we just artificially create an array of blocklength to call PMPI_Type_indexed */
	int *blocklength_array = sctk_malloc(count * sizeof(int) );

	assume(blocklength_array != NULL);

	int i;

	for(i = 0; i < count; i++)
	{
		blocklength_array[i] = blocklength;
	}

	/* Call the orignal indexed function */
	int res = PMPI_Type_create_hindexed(count, blocklength_array, indices, old_type, newtype);

	/* Set its context to overide the one from hdindexed */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner    = MPI_COMBINER_HINDEXED_BLOCK;
	dtctx.count       = count;
	dtctx.blocklength = blocklength;
	dtctx.array_of_displacements_addr = indices;
	dtctx.oldtype = old_type;
	_mpc_cl_type_ctx_set(*newtype, &dtctx);

	/* Free the tmp buffer */
	sctk_free(blocklength_array);

	return res;
}

/** \brief This function creates a sequence of contiguous blocks each with their offset but with the same length
 *      \param count Number of blocks
 *      \param blocklength length of every blocks
 *      \param indices Offset of each block in old_type size multiples
 *      \param old_type Input data-type
 *      \param newtype_p New vector data-type
 */
static int __INTERNAL__PMPI_Type_create_indexed_block(int count, int blocklength, const int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	/* Convert the indices to bytes */
	MPI_Aint extent;

	/* Retrieve type extent */
	PMPI_Type_extent(old_type, &extent);


	/* Create a temporary offset array */
	MPI_Aint *byte_offsets = sctk_malloc(count * sizeof(MPI_Aint) );

	assume(byte_offsets != NULL);

	int i;

	/* Fill it with by converting type based indices to bytes */
	for(i = 0; i < count; i++)
	{
		byte_offsets[i] = indices[i] * extent;
	}

	/* Call the orignal indexed function */
	int res = __INTERNAL__PMPI_Type_create_hindexed_block(count, blocklength, byte_offsets, old_type, newtype);

	/* Set its context to overide the one from hdindexed block */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner               = MPI_COMBINER_INDEXED_BLOCK;
	dtctx.count                  = count;
	dtctx.blocklength            = blocklength;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype                = old_type;
	_mpc_cl_type_ctx_set(*newtype, &dtctx);

	/* Free the temporary byte offset */
	sctk_free(byte_offsets);

	return res;
}

/* #########################################################################
 * sctk_MPIOI_Type_block, sctk_MPIOI_Type_cyclic,  sctk_Type_create_darray
 * and sctk_Type_create_subarray are from the ROMIO implemntations and
 * are subject to ROMIO copyright:
 * Copyright (C) 1997 University of Chicago.
 *                             COPYRIGHT
 * #########################################################################
 */

/************************************************************************/
/* DARRAY IMPLEMENTATION                                                */
/************************************************************************/


static int sctk_MPIOI_Type_block(const int *array_of_gsizes, int dim, int ndims, int nprocs,
                                 int rank, int darg, int order, MPI_Aint orig_extent,
                                 MPI_Datatype type_old, MPI_Datatype *type_new,
                                 MPI_Aint *st_offset);

static int sctk_MPIOI_Type_cyclic(const int *array_of_gsizes, int dim, int ndims, int nprocs,
                                  int rank, int darg, int order, MPI_Aint orig_extent,
                                  MPI_Datatype type_old, MPI_Datatype *type_new,
                                  MPI_Aint *st_offset);


int sctk_Type_create_darray(int size, int rank, int ndims,
                            const int *array_of_gsizes, const int *array_of_distribs,
                            const int *array_of_dargs, const int *array_of_psizes,
                            int order, MPI_Datatype oldtype,
                            MPI_Datatype *newtype)
{
	MPI_Datatype type_old, type_new = MPI_DATATYPE_NULL, types[3];
	int          procs, tmp_rank, i, tmp_size, blklens[3], *coords;
	MPI_Aint *   st_offsets, orig_extent, disps[3];

	PMPI_Type_extent(oldtype, &orig_extent);

/* calculate position in Cartesian grid as MPI would (row-major
 * ordering) */
	coords   = (int *)sctk_malloc(ndims * sizeof(int) );
	procs    = size;
	tmp_rank = rank;
	for(i = 0; i < ndims; i++)
	{
		procs     = procs / array_of_psizes[i];
		coords[i] = tmp_rank / procs;
		tmp_rank  = tmp_rank % procs;
	}

	st_offsets = (MPI_Aint *)sctk_malloc(ndims * sizeof(MPI_Aint) );
	type_old   = oldtype;

	if(order == MPI_ORDER_FORTRAN)
	{
		/* dimension 0 changes fastest */
		for(i = 0; i < ndims; i++)
		{
			switch(array_of_distribs[i])
			{
				case MPI_DISTRIBUTE_BLOCK:
					sctk_MPIOI_Type_block(array_of_gsizes, i, ndims,
					                      array_of_psizes[i],
					                      coords[i], array_of_dargs[i],
					                      order, orig_extent,
					                      type_old, &type_new,
					                      st_offsets + i);
					break;

				case MPI_DISTRIBUTE_CYCLIC:
					sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims,
					                       array_of_psizes[i], coords[i],
					                       array_of_dargs[i], order,
					                       orig_extent, type_old,
					                       &type_new, st_offsets + i);
					break;

				case MPI_DISTRIBUTE_NONE:
					/* treat it as a block distribution on 1 process */
					sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, 1, 0,
					                      MPI_DISTRIBUTE_DFLT_DARG, order,
					                      orig_extent,
					                      type_old, &type_new,
					                      st_offsets + i);
					break;
			}
			if(i)
			{
				PMPI_Type_free(&type_old);
			}
			PMPI_Type_commit(&type_new);
			type_old = type_new;
		}

		/* add displacement and UB */
		disps[1] = st_offsets[0];
		tmp_size = 1;
		for(i = 1; i < ndims; i++)
		{
			tmp_size *= array_of_gsizes[i - 1];
			disps[1] += (MPI_Aint)tmp_size * st_offsets[i];
		}
		/* rest done below for both Fortran and C order */
	}

	else /* order == MPI_ORDER_C */
	{
		/* dimension ndims-1 changes fastest */
		for(i = ndims - 1; i >= 0; i--)
		{
			switch(array_of_distribs[i])
			{
				case MPI_DISTRIBUTE_BLOCK:
					sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
					                      coords[i], array_of_dargs[i], order,
					                      orig_extent, type_old, &type_new,
					                      st_offsets + i);
					break;

				case MPI_DISTRIBUTE_CYCLIC:
					sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims,
					                       array_of_psizes[i], coords[i],
					                       array_of_dargs[i], order,
					                       orig_extent, type_old, &type_new,
					                       st_offsets + i);
					break;

				case MPI_DISTRIBUTE_NONE:
					/* treat it as a block distribution on 1 process */
					sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
					                      coords[i], MPI_DISTRIBUTE_DFLT_DARG, order, orig_extent,
					                      type_old, &type_new, st_offsets + i);
					break;
			}
			if(i != ndims - 1)
			{
				PMPI_Type_free(&type_old);
			}
			PMPI_Type_commit(&type_new);
			type_old = type_new;
		}

		/* add displacement and UB */
		disps[1] = st_offsets[ndims - 1];
		tmp_size = 1;
		for(i = ndims - 2; i >= 0; i--)
		{
			tmp_size *= array_of_gsizes[i + 1];
			disps[1] += (MPI_Aint)tmp_size * st_offsets[i];
		}
	}

	disps[1] *= orig_extent;

	disps[2] = orig_extent;
	for(i = 0; i < ndims; i++)
	{
		disps[2] *= (MPI_Aint)array_of_gsizes[i];
	}

	disps[0]   = 0;
	blklens[0] = blklens[1] = blklens[2] = 1;
	types[0]   = MPI_LB;
	types[1]   = type_new;
	types[2]   = MPI_UB;

	PMPI_Type_struct(3, blklens, disps, types, newtype);

	PMPI_Type_free(&type_new);
	sctk_free(st_offsets);
	sctk_free(coords);

	return MPI_SUCCESS;
}

/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_block(const int *array_of_gsizes, int dim, int ndims, int nprocs,
                                 int rank, int darg, int order, MPI_Aint orig_extent,
                                 MPI_Datatype type_old, MPI_Datatype *type_new,
                                 MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
 *  rank = coordinate of this process in dimension dim */
	int      blksize, global_size, mysize, i, j;
	MPI_Aint stride;

	global_size = array_of_gsizes[dim];

	if(darg == MPI_DISTRIBUTE_DFLT_DARG)
	{
		blksize = (global_size + nprocs - 1) / nprocs;
	}
	else
	{
		blksize = darg;

		/* --BEGIN ERROR HANDLING-- */
		if(blksize <= 0)
		{
			return MPI_ERR_ARG;
		}

		if(blksize * nprocs < global_size)
		{
			return MPI_ERR_ARG;
		}
		/* --END ERROR HANDLING-- */
	}

	j      = global_size - blksize * rank;
	mysize = (blksize < j) ? blksize : j;
	if(mysize < 0)
	{
		mysize = 0;
	}

	stride = orig_extent;
	if(order == MPI_ORDER_FORTRAN)
	{
		if(dim == 0)
		{
			PMPI_Type_contiguous(mysize, type_old, type_new);
		}
		else
		{
			for(i = 0; i < dim; i++)
			{
				stride *= (MPI_Aint)array_of_gsizes[i];
			}
			PMPI_Type_hvector(mysize, 1, stride, type_old, type_new);
		}
	}
	else
	{
		if(dim == ndims - 1)
		{
			PMPI_Type_contiguous(mysize, type_old, type_new);
		}
		else
		{
			for(i = ndims - 1; i > dim; i--)
			{
				stride *= (MPI_Aint)array_of_gsizes[i];
			}
			PMPI_Type_hvector(mysize, 1, stride, type_old, type_new);
		}
	}

	*st_offset = (MPI_Aint)blksize * (MPI_Aint)rank;
	/* in terms of no. of elements of type oldtype in this dimension */
	if(mysize == 0)
	{
		*st_offset = 0;
	}

	return MPI_SUCCESS;
}

/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_cyclic(const int *array_of_gsizes, int dim, int ndims, int nprocs,
                                  int rank, int darg, int order, MPI_Aint orig_extent,
                                  MPI_Datatype type_old, MPI_Datatype *type_new,
                                  MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
 *  rank = coordinate of this process in dimension dim */
	int          blksize, i, blklens[3], st_index, end_index, local_size, rem, count;
	MPI_Aint     stride, disps[3];
	MPI_Datatype type_tmp, types[3];

	if(darg == MPI_DISTRIBUTE_DFLT_DARG)
	{
		blksize = 1;
	}
	else
	{
		blksize = darg;
	}

	/* --BEGIN ERROR HANDLING-- */
	if(blksize <= 0)
	{
		return MPI_ERR_ARG;
	}
	/* --END ERROR HANDLING-- */

	st_index  = rank * blksize;
	end_index = array_of_gsizes[dim] - 1;

	if(end_index < st_index)
	{
		local_size = 0;
	}
	else
	{
		local_size  = ( (end_index - st_index + 1) / (nprocs * blksize) ) * blksize;
		rem         = (end_index - st_index + 1) % (nprocs * blksize);
		local_size += (rem < blksize) ? rem : blksize;
	}

	count = local_size / blksize;
	rem   = local_size % blksize;

	stride = (MPI_Aint)nprocs * (MPI_Aint)blksize * orig_extent;
	if(order == MPI_ORDER_FORTRAN)
	{
		for(i = 0; i < dim; i++)
		{
			stride *= (MPI_Aint)array_of_gsizes[i];
		}
	}
	else
	{
		for(i = ndims - 1; i > dim; i--)
		{
			stride *= (MPI_Aint)array_of_gsizes[i];
		}
	}

	PMPI_Type_hvector(count, blksize, stride, type_old, type_new);

	if(rem)
	{
		/* if the last block is of size less than blksize, include
		 * it separately using MPI_Type_struct */

		types[0]   = *type_new;
		types[1]   = type_old;
		disps[0]   = 0;
		disps[1]   = (MPI_Aint)count * stride;
		blklens[0] = 1;
		blklens[1] = rem;

		PMPI_Type_struct(2, blklens, disps, types, &type_tmp);

		PMPI_Type_free(type_new);
		*type_new = type_tmp;
	}

	/* In the first iteration, we need to set the displacement in that
	 * dimension correctly. */
	if( ( (order == MPI_ORDER_FORTRAN) && (dim == 0) ) ||
	    ( (order == MPI_ORDER_C) && (dim == ndims - 1) ) )
	{
		types[0]   = MPI_LB;
		disps[0]   = 0;
		types[1]   = *type_new;
		disps[1]   = (MPI_Aint)rank * (MPI_Aint)blksize * orig_extent;
		types[2]   = MPI_UB;
		disps[2]   = orig_extent * (MPI_Aint)array_of_gsizes[dim];
		blklens[0] = blklens[1] = blklens[2] = 1;
		PMPI_Type_struct(3, blklens, disps, types, &type_tmp);
		PMPI_Type_free(type_new);
		*type_new = type_tmp;

		*st_offset = 0; /* set it to 0 because it is taken care of in
		                 * the struct above */
	}
	else
	{
		*st_offset = (MPI_Aint)rank * (MPI_Aint)blksize;

		/* st_offset is in terms of no. of elements of type oldtype in
		 * this dimension */
	}

	if(local_size == 0)
	{
		*st_offset = 0;
	}

	return MPI_SUCCESS;
}

/************************************************************************/
/* SUBARRAY IMPLEMENTATION                                              */
/************************************************************************/


int sctk_Type_create_subarray(int ndims,
                              const int *array_of_sizes,
                              const int *array_of_subsizes,
                              const int *array_of_starts,
                              int order,
                              MPI_Datatype oldtype,
                              MPI_Datatype *newtype)
{
	MPI_Aint     extent, disps[3], size;
	int          i, blklens[3], rc;
	MPI_Datatype tmp1, tmp2, types[3];

	rc = PMPI_Type_extent(oldtype, &extent);
	if (rc)
	{
		mpc_common_debug_fatal("%s: PMPI_Type_extent failed", __func__);
	}

	if(order == MPI_ORDER_FORTRAN)
	{
		/* dimension 0 changes fastest */
		if(ndims == 1)
		{
			rc = PMPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_contiguous failed", __func__);
			}

			rc = PMPI_Type_commit(&tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
			}
		}
		else
		{
			rc = PMPI_Type_vector(array_of_subsizes[1],
			                 array_of_subsizes[0],
			                 array_of_sizes[0], oldtype, &tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_vector failed", __func__);
			}

			rc = PMPI_Type_commit(&tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
			}

			size = (MPI_Aint)array_of_sizes[0] * extent;
			for(i = 2; i < ndims; i++)
			{
				size *= (MPI_Aint)array_of_sizes[i - 1];

				rc = PMPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_hvector failed", __func__);
				}

				rc = PMPI_Type_free(&tmp1);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_free failed", __func__);
				}

				tmp1 = tmp2;

				rc = PMPI_Type_commit(&tmp1);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
				}
			}
		}

		/* add displacement and UB */
		disps[1] = array_of_starts[0];
		size     = 1;
		for(i = 1; i < ndims; i++)
		{
			size     *= (MPI_Aint)array_of_sizes[i - 1];
			disps[1] += size * (MPI_Aint)array_of_starts[i];
		}
		/* rest done below for both Fortran and C order */
	}

	else /* order == MPI_ORDER_C */
	{
		/* dimension ndims-1 changes fastest */
		if(ndims == 1)
		{
			rc = PMPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_contiguous failed", __func__);
			}

			rc = PMPI_Type_commit(&tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
			}
		}
		else
		{
			int rc = PMPI_Type_vector(array_of_subsizes[ndims - 2],
			                 array_of_subsizes[ndims - 1],
			                 array_of_sizes[ndims - 1], oldtype, &tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_vector failed", __func__);
			}

			rc = PMPI_Type_commit(&tmp1);
			if (rc)
			{
				mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
			}

			size = (MPI_Aint)array_of_sizes[ndims - 1] * extent;
			for(i = ndims - 3; i >= 0; i--)
			{
				size *= (MPI_Aint)array_of_sizes[i + 1];
				rc = PMPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_hvector failed", __func__);
				}

				rc = PMPI_Type_free(&tmp1);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_free failed", __func__);
				}

				tmp1 = tmp2;

				rc = PMPI_Type_commit(&tmp1);
				if (rc)
				{
					mpc_common_debug_fatal("%s: PMPI_Type_commit failed", __func__);
				}
			}
		}

		/* add displacement and UB */
		disps[1] = array_of_starts[ndims - 1];
		size     = 1;
		for(i = ndims - 2; i >= 0; i--)
		{
			size     *= (MPI_Aint)array_of_sizes[i + 1];
			disps[1] += size * (MPI_Aint)array_of_starts[i];
		}
	}

	disps[1] *= extent;

	disps[2] = extent;
	for(i = 0; i < ndims; i++)
	{
		disps[2] *= (MPI_Aint)array_of_sizes[i];
	}

	disps[0]   = 0;
	blklens[0] = blklens[1] = blklens[2] = 1;
	types[0]   = MPI_LB;
	types[1]   = tmp1;
	types[2]   = MPI_UB;

	rc = PMPI_Type_struct(3, blklens, disps, types, newtype);
	if (rc)
	{
		mpc_common_debug_fatal("%s: PMPI_Type_struct failed", __func__);
	}

	rc = PMPI_Type_free(&tmp1);
	if (rc)
	{
		mpc_common_debug_fatal("%s: PMPI_Type_free failed", __func__);
	}

	return MPI_SUCCESS;
}

/* #########################################################################
*  END OF ROMIO CODE
* ########################################################################*/


static inline MPI_Datatype *__get_typemask(MPI_Datatype datatype, int *type_mask_count, mpc_lowcomm_datatype_t *static_type)
{
	*type_mask_count = 0;

	bool flag = false;

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			*type_mask_count = 1;
			*static_type     = datatype;
			return static_type;

			break;

		case MPC_DATATYPES_USER:
			_mpc_cl_type_is_allocated(datatype, &flag);
			if(flag)
			{
				if(mpc_mpi_cl_type_is_contiguous(datatype) )
				{
					*type_mask_count = 1;
					if(mpc_lowcomm_datatype_is_common(datatype->datatypes[0]) )
					{
						*static_type = datatype->datatypes[0];
						return static_type;
					}
					else
					{
						/* We have to continue the unpacking until finding a common type */
						return __get_typemask(datatype->datatypes[0], type_mask_count, static_type);
					}
				}

				*type_mask_count = datatype->count;
				return datatype->datatypes;
			}
			break;

		default:
			mpc_common_debug_fatal("Unreachable code in __get_typemask");
	}

	return NULL;
}

#define MPI_MAX_CONCURENT    128

/* Function pointer for user collectives */



/* Collectives */
int mpc_mpi_cl_egreq_progress_poll();

int __INTERNAL__PMPI_Barrier_intra_shm_sig(MPI_Comm comm)
{
	struct sctk_comm_coll *        coll        = mpc_communicator_shm_coll_get(comm);
	struct shared_mem_barrier_sig *barrier_ctx = &coll->shm_barrier_sig;

	// mpc_common_debug_error("BARRIER CTX : %p", barrier_ctx	);

	if(!coll)
	{
		return MPI_ERR_COMM;
	}

	int rank;

	_mpc_cl_comm_rank(comm, &rank);

	volatile int the_signal = 0;

	volatile int *toll = &barrier_ctx->tollgate[rank];

	int cnt = 0;

	if(__do_yield)
	{
		while(*toll != OPA_load_int(&barrier_ctx->fare) )
		{
			mpc_thread_yield();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			if( (cnt++ & 0xFF) == 0)
			{
				mpc_mpi_cl_egreq_progress_poll();
			}
		}
	}
	else
	{
		while(*toll != OPA_load_int(&barrier_ctx->fare) )
		{
			sctk_cpu_relax();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			if( (cnt++ & 0xFF) == 0)
			{
				mpc_mpi_cl_egreq_progress_poll();
			}
		}
	}

	/* I Own the cell */
	OPA_store_ptr(&barrier_ctx->sig_points[rank], (void *)&the_signal);

	/* Next time we expect the opposite */
	*toll = !*toll;

	if(OPA_fetch_and_decr_int(&barrier_ctx->counter) == 1)
	{
		/* The last task */
		int size = coll->comm_size;

		/* Reset counter */
		OPA_store_int(&barrier_ctx->counter, size);

		/* Free others */
		int i;
		for(i = 0; i < size; i++)
		{
			int *sig = OPA_load_ptr(&barrier_ctx->sig_points[i]);
			*sig = 1;
		}

		/* Reverse the Fare */
		int current_fare = OPA_load_int(&barrier_ctx->fare);
		OPA_store_int(&barrier_ctx->fare, !current_fare);
	}
	else
	{
		if(__do_yield)
		{
			while(the_signal == 0)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
				if( (cnt++ & 0xFF) == 0)
				{
					mpc_mpi_cl_egreq_progress_poll();
				}
			}
		}
		else
		{
			while(the_signal == 0)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
				if( (cnt++ & 0xFF) == 0)
				{
					mpc_mpi_cl_egreq_progress_poll();
				}
			}
		}
	}

	return MPI_SUCCESS;
}

int __MPC_init_node_comm_ctx(struct sctk_comm_coll *coll, MPI_Comm comm)
{
	int is_shared = 0;

	int   rank = 0;
	void *tmp_ctx;

	PMPI_Comm_size(comm, &coll->comm_size);
	PMPI_Comm_rank(comm, &rank);

	if(!rank)
	{
		tmp_ctx = mpc_lowcomm_allocmem_pool_alloc_check(sizeof(struct sctk_comm_coll),
		                                                &is_shared);

		if(is_shared == 0)
		{
			sctk_free(tmp_ctx);
			tmp_ctx = NULL;
		}

		if(!tmp_ctx)
		{
			tmp_ctx = (void *)0x1;
		}
		else
		{
			sctk_per_node_comm_context_init(tmp_ctx, comm, coll->comm_size);
		}

		assert(tmp_ctx != NULL);
		mpc_lowcomm_bcast(&tmp_ctx, sizeof(void *), 0, comm);
		mpc_lowcomm_barrier(comm);
		coll->node_ctx = tmp_ctx;
	}
	else
	{
		mpc_lowcomm_bcast(&tmp_ctx, sizeof(void *), 0, comm);
		mpc_lowcomm_barrier(comm);
		coll->node_ctx = tmp_ctx;
	}

	return MPI_SUCCESS;
}

static inline int __MPC_node_comm_coll_check(struct sctk_comm_coll *coll, MPI_Comm comm)
{
	if(coll->node_ctx == (void *)0x1)
	{
		/* A previous alloc failed*/
		return 0;
	}

	if(coll->node_ctx == NULL)
	{
		__MPC_init_node_comm_ctx(coll, comm);
	}

	return 1;
}

int __INTERNAL__PMPI_Barrier_intra(MPI_Comm comm)
{
#ifdef MPC_USE_PORTALS
	int res;
	int rank, size;
	_mpc_cl_comm_size(comm, &size);
	_mpc_cl_comm_rank(comm, &rank);

	if(ptl_offcoll_enabled() )
	{
		res = ptl_offcoll_barrier(mpc_lowcomm_communicator_id(comm), rank, size);
		return res;
	}
#endif

	return mpc_lowcomm_barrier(comm);
}

int __INTERNAL__PMPI_Barrier_intra_shared_node(MPI_Comm comm)
{
	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	if(__MPC_node_comm_coll_check(coll, comm) )
	{
		return mpc_lowcomm_barrier_shm_on_context(&coll->node_ctx->shm_barrier,
		                                          coll->comm_size);
	}
	else
	{
		return __INTERNAL__PMPI_Barrier_intra(comm);
	}
}

int __INTERNAL__PMPI_Barrier_inter(MPI_Comm comm)
{
	int root = 0, buf = 0, rank, size;
	int res = MPI_ERR_INTERN;

	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	/* Barrier on local intracomm */
	if(size > 1)
	{
		res = PMPI_Barrier(mpc_lowcomm_communicator_get_local(comm) );
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	/* Broadcasts between local and remote groups */
	if(mpc_lowcomm_communicator_in_master_group(comm) )
	{
		root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
		res  = PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
		root = 0;
		res  = PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		root = 0;
		res  = PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
		root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
		res  = PMPI_Bcast(&buf, 1, MPI_INT, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Bcast_inter(void *buffer, int count, MPI_Datatype datatype,
                                 int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	mpc_lowcomm_status_t status;
	int rank;

	if(root == MPI_PROC_NULL)
	{
		MPI_ERROR_SUCCESS();
	}
	else if(root == MPC_ROOT)
	{
		/* root send to remote group leader */
		res = PMPI_Send_internal(buffer, count, datatype, 0, MPC_BROADCAST_TAG,
		                         comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		res = _mpc_cl_comm_rank(comm, &rank);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		if(rank == 0)
		{
			/* local leader recv from remote group leader */
			res = PMPI_Recv_internal(buffer, count, datatype, root,
			                         MPC_BROADCAST_TAG, comm, &status);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}
		/* Intracomm broadcast */
		res = PMPI_Bcast(buffer, count, datatype, 0,
		                 mpc_lowcomm_communicator_get_local(comm) );
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	return res;
}

int __INTERNAL__PMPI_Bcast_intra_shm(void *buffer, int count,
                                     MPI_Datatype datatype, int root,
                                     MPI_Comm comm)
{
	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	int rank, res;

	PMPI_Comm_rank(comm, &rank);

	struct shared_mem_bcast *bcast_ctx = sctk_comm_coll_get_bcast(coll, rank);


	/* First pay the toll gate */
	if(__do_yield)
	{
		while(bcast_ctx->tollgate[rank] !=
		      OPA_load_int(&bcast_ctx->fare) )
		{
			mpc_thread_yield();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}
	else
	{
		while(bcast_ctx->tollgate[rank] !=
		      OPA_load_int(&bcast_ctx->fare) )
		{
			sctk_cpu_relax();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}

	/* Reverse state so that only a root done can unlock by
	 * also reversing the fare */
	bcast_ctx->tollgate[rank] = !bcast_ctx->tollgate[rank];

	void *   data_buff = buffer;
	MPI_Aint tsize     = 0;

	res = PMPI_Type_extent(datatype, &tsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	int is_shared_mem_buffer = sctk_mpi_type_is_shared_mem(datatype, count);
	int is_contig_type       = _mpc_dt_is_contig_mem(datatype);

	/* Now am I the root ? */
	if(root == rank)
	{
		if(__do_yield)
		{
			while(OPA_cas_int(&bcast_ctx->owner, -1, -2) != -1)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_cas_int(&bcast_ctx->owner, -1, -2) != -1)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		bcast_ctx->root_in_buff = 0;

		/* Does root need to pack ? */
		if(!is_contig_type)
		{
			/* We have a tmp bufer where to reduce */
			data_buff = sctk_malloc(count * tsize);

			assume(data_buff != NULL);

			/* If non-contig, we need to pack to the TMP buffer
			 * where the reduction will be operated */
			int cnt = 0;
			PMPI_Pack(buffer, count, datatype, data_buff, tsize * count, &cnt, comm);

			/* We had to allocate the segment save it for release by the last */
			OPA_store_ptr(&bcast_ctx->to_free, data_buff);

			/* Set pack as reference */
			bcast_ctx->target_buff = data_buff;
		}
		else
		{
			/* Set the ref buffer */
			bcast_ctx->target_buff = data_buff;

			/* Can we use the SHM buffer ? */

			if(is_shared_mem_buffer)
			{
				/* Set my value in the TMP buffer */
				sctk_mpi_shared_mem_buffer_fill(&bcast_ctx->buffer, datatype, count,
				                                data_buff);
				bcast_ctx->root_in_buff = 1;
			}
		}

		/* Save source type infos */
		bcast_ctx->stype_size = tsize;
		bcast_ctx->scount     = count;

		/* Now unleash the others */
		OPA_store_int(&bcast_ctx->owner, rank);
	}
	else
	{
		/* Wait for the root */
		if(__do_yield)
		{
			while(OPA_load_int(&bcast_ctx->owner) != root)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&bcast_ctx->owner) != root)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
	}

	/* If we are here the root has set its data */
	if(rank != root)
	{
		/* We are in the TMB buffers */
		if(bcast_ctx->root_in_buff)
		{
			sctk_mpi_shared_mem_buffer_get(&bcast_ctx->buffer, datatype, count,
			                               buffer,
			                               bcast_ctx->scount * bcast_ctx->stype_size);
		}
		else
		{
			void *src = bcast_ctx->target_buff;

			/* Datatype has to be unpacked */
			if(!is_contig_type)
			{
				/* If non-contig, we need to unpack to the final buffer */
				int cnt = 0;
				PMPI_Unpack(src, bcast_ctx->scount * bcast_ctx->stype_size, &cnt, buffer,
				            count, datatype, comm);
			}
			else
			{
				/* Yes ! this type is contiguous */
				memcpy(buffer, src, tsize * count);
			}
		}
	}

	/* Now leave the pending list and if I am the last I free */

	if(bcast_ctx->root_in_buff)
	{
		if(OPA_fetch_and_decr_int(&bcast_ctx->left_to_pop) == 1)
		{
			goto SHM_BCAST_RELEASE;
		}
	}
	else
	{
		/* Sorry rank 0 we have to make sure that the root stays here if we are
		 * not using the async buffers */

		OPA_decr_int(&bcast_ctx->left_to_pop);

		if(rank == root)
		{
			/* Wait for everybody */

			if(__do_yield)
			{
				while(OPA_load_int(&bcast_ctx->left_to_pop) != 0)
				{
					mpc_thread_yield();
					MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
				}
			}
			else
			{
				while(OPA_load_int(&bcast_ctx->left_to_pop) != 0)
				{
					sctk_cpu_relax();
					MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
				}
			}

			goto SHM_BCAST_RELEASE;
		}
	}

	return MPI_SUCCESS;

SHM_BCAST_RELEASE: {
		void *to_free = OPA_load_ptr(&bcast_ctx->to_free);

		if(to_free)
		{
			OPA_store_ptr(&bcast_ctx->to_free, 0);
			sctk_free(to_free);
		}

		/* Set the counter */
		OPA_store_int(&bcast_ctx->left_to_pop, coll->comm_size);

		OPA_store_int(&bcast_ctx->owner, -1);

		int current_fare = OPA_load_int(&bcast_ctx->fare);
		OPA_store_int(&bcast_ctx->fare, !current_fare);

		return MPI_SUCCESS;
	}
}

struct shared_node_coll_data
{
	void *buffer_addr;
	char  is_counter;
};


int __INTERNAL__PMPI_Bcast_intra(void *buffer, int count, MPI_Datatype datatype,
                                 int root, MPI_Comm comm);

int __INTERNAL__PMPI_Bcast_intra_shared_node_impl(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPI_Comm comm, struct sctk_comm_coll *coll)
{
	int rank;

	PMPI_Comm_rank(comm, &rank);

	int tsize;

	PMPI_Type_size(datatype, &tsize);

	size_t to_bcast_size = tsize * count;

	struct shared_node_coll_data cdata;

	cdata.is_counter = 0;

	if(rank == root)
	{
		if(!mpc_lowcomm_allocmem_is_in_pool(buffer) )
		{
			int is_shared = 0;
			cdata.buffer_addr = mpc_lowcomm_allocmem_pool_alloc_check(to_bcast_size + sizeof(OPA_int_t),
			                                                          &is_shared);

			if(!is_shared)
			{
				/* We failed! */
				sctk_free(cdata.buffer_addr);
				cdata.buffer_addr = NULL;
				mpc_common_debug_fatal("Failed to allocate in memory pool");
			}
			else
			{
				/* Fill the buffer */
				OPA_store_int( (OPA_int_t *)cdata.buffer_addr, coll->comm_size);
				/*
				 * NOLINTBEGIN(clang-analyzer-core.NonNullParamChecker):
				 * False positive "Null pointer passed to 2nd parameter expecting 'nonnull'".
				 * `buffer` is NULL iff `rank != root` in
				 * __INTERNAL__PMPI_Scatter_intra_shared_node_impl.
				 * The current section is only executed if `rank == root`,
				 * therefore buffer is non-NULL.
				 */
				memcpy(cdata.buffer_addr + sizeof(OPA_int_t), buffer, to_bcast_size);
				/* NOLINTEND(clang-analyzer-core.NonNullParamChecker) */
				cdata.is_counter = 1;
			}
		}
		else
		{
			cdata.buffer_addr = buffer;
		}
	}

	__INTERNAL__PMPI_Bcast_intra(&cdata.buffer_addr, sizeof(struct shared_node_coll_data), MPI_CHAR, root, comm);

	/*
	 * NOLINTBEGIN(clang-analyzer-core.UndefinedBinaryOperatorResult):
	 * `cdata.buffer_addr` is necessarily non-garbage:
	 * - If `rank == root`, initialization occurs prior to Bcast.
	 * - If `rank != root`, initialization occurs within Bcast.
	 */
	if(cdata.buffer_addr != NULL)
	{
		/* NOLINTEND(clang-analyzer-core.UndefinedBinaryOperatorResult) */
		if(rank != root)
		{
			if(cdata.is_counter)
			{
				memcpy(buffer, cdata.buffer_addr + sizeof(OPA_int_t), to_bcast_size);

				int token = OPA_fetch_and_decr_int( (OPA_int_t *)cdata.buffer_addr);
				if(token == 2)
				{
					mpc_lowcomm_allocmem_pool_free_size(cdata.buffer_addr, to_bcast_size + sizeof(OPA_int_t) );
				}
			}
			else
			{
				memcpy(buffer, cdata.buffer_addr, to_bcast_size);
			}
		}
	}
	else
	{
		/* Fallback to regular bcast */
		return __INTERNAL__PMPI_Bcast_intra(buffer, count, datatype, root, comm);
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Bcast_intra_shared_node(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPI_Comm comm)
{
	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	//TODO to expose as config vars
	if(__MPC_node_comm_coll_check(coll, comm) &&
	   ( (4 <= coll->comm_size) || (1024 < count) )  )
	{
		return __INTERNAL__PMPI_Bcast_intra_shared_node_impl(buffer, count, datatype, root, comm, coll);
	}
	else
	{
		return __INTERNAL__PMPI_Bcast_intra(buffer, count, datatype, root, comm);
	}
}

int __INTERNAL__PMPI_Bcast_intra(void *buffer, int count, MPI_Datatype datatype,
                                 int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN, size, rank;

	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
#ifdef MPC_USE_PORTALS
	if(ptl_offcoll_enabled() && _mpc_dt_get_kind(datatype) == MPC_DATATYPES_COMMON)
	{
		size_t tmp_size;
		_mpc_cl_type_size(datatype, &tmp_size);
		size_t length = ( (size_t)count) * ( (size_t)tmp_size);

		res = ptl_offcoll_bcast(mpc_lowcomm_communicator_id(comm), rank, size, buffer, length, root);
		return res;
	}
#endif

	return _mpc_mpi_collectives_bcast(buffer, count, datatype, root, comm);
}

int __INTERNAL__PMPI_Gather_intra(const void *sendbuf, int sendcnt,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  const int recvcnt, MPI_Datatype recvtype, int root,
                                  MPI_Comm comm)
{
	return _mpc_mpi_collectives_gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int __INTERNAL__PMPI_Gather_inter(void *sendbuf, int sendcnt,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcnt, MPI_Datatype recvtype, int root,
                                  MPI_Comm comm)
{
	char *   ptmp;
	int      i, res = MPI_ERR_INTERN, size;
	MPI_Aint incr, extent;

	res = PMPI_Comm_remote_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(MPI_PROC_NULL == root)
	{
		MPI_ERROR_SUCCESS();
	}
	else if(root != MPI_ROOT)
	{
		res = PMPI_Send_internal(sendbuf, sendcnt, sendtype, root,
		                         MPC_GATHER_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		res = PMPI_Type_extent(recvtype, &extent);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		incr = extent * recvcnt;
		for(i = 0, ptmp = (char *)recvbuf; i < size; ++i, ptmp += incr)
		{
			res = PMPI_Recv_internal(ptmp, recvcnt, recvtype, i, MPC_GATHER_TAG,
			                         comm, MPI_STATUS_IGNORE);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}
	}
	return res;
}

int __INTERNAL__PMPI_Gatherv_intra_shm(const void *sendbuf, int sendcnt,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       const int *recvcnts, const int *displs,
                                       MPI_Datatype recvtype, int root,
                                       MPI_Comm comm)
{
	struct sctk_comm_coll *    coll   = mpc_communicator_shm_coll_get(comm);
	struct shared_mem_gatherv *gv_ctx = &coll->shm_gatherv;

	volatile int rank, res;

	PMPI_Comm_rank(comm, (int *)&rank);

	/* First pay the toll gate */
	if(__do_yield)
	{
		while(gv_ctx->tollgate[rank] != OPA_load_int(&gv_ctx->fare) )
		{
			mpc_thread_yield();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}
	else
	{
		while(gv_ctx->tollgate[rank] != OPA_load_int(&gv_ctx->fare) )
		{
			sctk_cpu_relax();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}

	/* Reverse state so that only a root done can unlock by
	 * also reversing the fare */
	gv_ctx->tollgate[rank] = !gv_ctx->tollgate[rank];

	void *   data_buff = (void *)sendbuf;
	MPI_Aint stsize    = 0;

	res = PMPI_Type_extent(sendtype, &stsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	int did_allocate_send = 0;

	gv_ctx->send_type_size[rank] = stsize;

	/* Does root need to pack ? */
	if(!_mpc_dt_is_contig_mem(sendtype) )
	{
		/* We have a tmp bufer where to reduce */
		data_buff = sctk_malloc(sendcnt * stsize);

		assume(data_buff != NULL);

		/* If non-contig, we need to pack to the TMP buffer
		 * where the reduction will be operated */
		int cnt = 0;
		PMPI_Pack(sendbuf, sendcnt, sendtype, data_buff, stsize * sendcnt, &cnt,
		          comm);

		/* We had to allocate the segment save it for release by the last */
		OPA_store_ptr(&gv_ctx->src_buffs[rank], data_buff);
		did_allocate_send = 1;
	}

	gv_ctx->send_count[rank] = sendcnt;

	/* Now am I the root ? */
	if(root == rank)
	{
		if(__do_yield)
		{
			while(OPA_cas_int(&gv_ctx->owner, -1, -2) != -1)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_cas_int(&gv_ctx->owner, -1, -2) != -1)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		/* Set the ref buffer */
		gv_ctx->target_buff = recvbuf;
		gv_ctx->counts      = recvcnts;
		gv_ctx->disps       = displs;

		MPI_Aint rtsize = 0;
		res = PMPI_Type_extent(recvtype, &rtsize);
		gv_ctx->rtype_size = rtsize;

		if(!_mpc_dt_is_contig_mem(sendtype) )
		{
			gv_ctx->let_me_unpack = 1;
		}
		else
		{
			gv_ctx->let_me_unpack = 0;
		}

		/* Now unleash the others */
		OPA_store_int(&gv_ctx->owner, rank);
	}
	else
	{
		/* Wait for the root */
		if(__do_yield)
		{
			while(OPA_load_int(&gv_ctx->owner) != root)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&gv_ctx->owner) != root)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
	}

	/* if some other processes don't have contig mem we should also unpack */
	if(!_mpc_dt_is_contig_mem(sendtype) && rank != root)
	{
		gv_ctx->let_me_unpack = 1;
	}
	/* Where to write ? */
	if(sendbuf != MPI_IN_PLACE)
	{
		if(gv_ctx->let_me_unpack)
		{
			/* If we are here the root has a non
			 * contiguous data-type, we then
			 * have to save our buffer and
			 * then leave the root at work to fill the segments */

			/* Is it already packed on our side ? */
			if(!did_allocate_send)
			{
				/* We need to put it in buffer */
				data_buff = sctk_malloc(sendcnt * stsize);
				assume(data_buff != NULL);
				memcpy(data_buff, sendbuf, sendcnt * stsize);
				OPA_store_ptr(&gv_ctx->src_buffs[rank], data_buff);
			}

			/*else
			 * {
			 *       OPA_store_ptr( &gv_ctx->src_buffs[rank], data_buff );
			 *       was done when packing
			 * }*/
		}
		else
		{
			/* If we are here we can directly write
			 * in the target buffer as the type is contig
			 * we just have to look for the right disp */

			void * to     = NULL;
			size_t to_cpy = 0;

			if(!gv_ctx->disps)
			{
				/* Gather case */
				to_cpy = gv_ctx->counts[0];
				to     = gv_ctx->target_buff + (to_cpy * gv_ctx->rtype_size) * rank;
			}
			else
			{
				to_cpy = gv_ctx->counts[rank];
				to     = gv_ctx->target_buff + (gv_ctx->disps[rank] * gv_ctx->rtype_size);
			}

			memcpy(to, sendbuf, to_cpy * gv_ctx->rtype_size);
		}
	}

	OPA_decr_int(&gv_ctx->left_to_push);

	if(rank == root)
	{
		/* Wait for all the others */
		if(__do_yield)
		{
			while(OPA_load_int(&gv_ctx->left_to_push) != 0)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&gv_ctx->left_to_push) != 0)
			{
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
				sctk_cpu_relax();
			}
		}

		/* Was it required that the root
		 * unpacks the whole thing ? */
		if(gv_ctx->let_me_unpack)
		{
			int i;

			for(i = 0; i < coll->comm_size; i++)
			{
				int   cnt  = 0;
				void *to   = NULL;
				void *from = NULL;

				from = OPA_load_ptr(&gv_ctx->src_buffs[i]);

				if(!gv_ctx->disps)
				{
					/* Gather case */
					mpc_common_nodebug("UNPACK %d@%d in %d => %d\%d", gv_ctx->send_count[i],
					                   gv_ctx->send_type_size[i], i, gv_ctx->counts[0],
					                   gv_ctx->rtype_size);
					to = gv_ctx->target_buff +
					     (gv_ctx->counts[0] * gv_ctx->rtype_size) * i;
					PMPI_Unpack(from, gv_ctx->send_count[i] * gv_ctx->send_type_size[i],
					            &cnt, to, gv_ctx->counts[0], recvtype, comm);
				}
				else
				{
					/* Gatherv case */
					to = gv_ctx->target_buff + (gv_ctx->disps[i] * gv_ctx->rtype_size);
					PMPI_Unpack(from, gv_ctx->send_count[i] * gv_ctx->send_type_size[i],
					            &cnt, to, gv_ctx->counts[i], recvtype, comm);
				}
			}
		}

		/* On free */
		OPA_store_int(&gv_ctx->left_to_push, coll->comm_size);

		OPA_store_int(&gv_ctx->owner, -1);

		int current_fare = OPA_load_int(&gv_ctx->fare);
		OPA_store_int(&gv_ctx->fare, !current_fare);
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Gather_intra_shm(const void *sendbuf, int sendcnt,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const int recvcnt,
                                      MPI_Datatype recvtype, int root,
                                      MPI_Comm comm)
{
	return __INTERNAL__PMPI_Gatherv_intra_shm(sendbuf, sendcnt, sendtype, recvbuf,
	                                          &recvcnt, NULL, recvtype, root, comm);
}

int __INTERNAL__PMPI_Gatherv_intra(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int *recvcnts, int *displs,
                                   MPI_Datatype recvtype, int root,
                                   MPI_Comm comm)
{
	return _mpc_mpi_collectives_gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
}

int __INTERNAL__PMPI_Gatherv_inter(const void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   const int *recvcnts, const int *displs,
                                   MPI_Datatype recvtype, int root,
                                   MPI_Comm comm)
{
	int          i, size, res = MPI_ERR_INTERN;
	char *       ptmp;
	MPI_Aint     extent;
	MPI_Request *recvrequest;

	res = PMPI_Comm_remote_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	recvrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request) );

	if(root == MPI_PROC_NULL)
	{
		res = MPI_SUCCESS;
	}
	else if(root != MPI_ROOT)
	{
		res = PMPI_Send_internal(sendbuf, sendcnt, sendtype, root,
		                         MPC_GATHERV_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			sctk_free(recvrequest);
			return res;
		}
	}
	else
	{
		res = PMPI_Type_extent(recvtype, &extent);
		if(res != MPI_SUCCESS)
		{
			sctk_free(recvrequest);
			return res;
		}

		for(i = 0; i < size; ++i)
		{
			ptmp = ( (char *)recvbuf) + (extent * displs[i]);

			res = PMPI_Irecv_internal(ptmp, recvcnts[i], recvtype, i,
			                          MPC_GATHERV_TAG, comm, &recvrequest[i]);
			if(res != MPI_SUCCESS)
			{
				sctk_free(recvrequest);
				return res;
			}
		}

		res = PMPI_Waitall(size, recvrequest, MPI_STATUSES_IGNORE);
		if(res != MPI_SUCCESS)
		{
			sctk_free(recvrequest);
			return res;
		}
	}
	sctk_free(recvrequest);
	return res;
}

int __INTERNAL__PMPI_Scatter_intra(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcnt, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm)
{
	return _mpc_mpi_collectives_scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int __INTERNAL__PMPI_Scatter_inter(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcnt, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm)
{
	int          i, size, res = MPI_ERR_INTERN;
	char *       ptmp;
	MPI_Aint     incr;
	MPI_Request *sendrequest;

	res = PMPI_Comm_remote_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	sendrequest = (MPI_Request *)sctk_malloc(size * sizeof(MPI_Request) );

	if(root == MPI_PROC_NULL)
	{
		res = MPI_SUCCESS;
	}
	else if(root != MPI_ROOT)
	{
		res = PMPI_Recv_internal(recvbuf, recvcnt, recvtype, root,
		                         MPC_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}
	}
	else
	{
		res = PMPI_Type_extent(sendtype, &incr);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}

		incr *= sendcnt;
		for(i = 0, ptmp = (char *)sendbuf; i < size; ++i, ptmp += incr)
		{
			res = PMPI_Isend_internal(ptmp, sendcnt, sendtype, i, MPC_SCATTER_TAG,
			                          comm, &(sendrequest[i]) );
			if(res != MPI_SUCCESS)
			{
				sctk_free(sendrequest);
				return res;
			}
		}

		res = PMPI_Waitall(size, sendrequest, MPI_STATUSES_IGNORE);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}
	}
	sctk_free(sendrequest);
	return res;
}

int __INTERNAL__PMPI_Scatter_intra(void *sendbuf, int sendcnt,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcnt, MPI_Datatype recvtype, int root,
                                   MPI_Comm comm);

int __INTERNAL__PMPI_Scatter_intra_shared_node_impl(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                                                    int root, MPI_Comm comm, struct sctk_comm_coll *coll)
{
	/* WARNING we can only be here with a regular scatter
	 * sendtype == recvtype && recvcount == sendcount */
	int rank;

	PMPI_Comm_rank(comm, &rank);

	int tsize;

	PMPI_Type_size(sendtype, &tsize);

	size_t to_scatter_size = tsize * sendcnt * coll->comm_size;

	static __thread struct shared_node_coll_data *cdata = NULL;
	cdata             = mpc_lowcomm_allocmem_pool_alloc(sizeof(struct shared_node_coll_data) );
	cdata->is_counter = 0;

	if(!mpc_lowcomm_allocmem_is_in_pool(sendbuf) )
	{
		int is_shared = 0;
		cdata->buffer_addr = mpc_lowcomm_allocmem_pool_alloc_check(to_scatter_size + sizeof(OPA_int_t),
																   &is_shared);

		if(!is_shared)
		{
			/* We failed! */
			sctk_free(cdata->buffer_addr);
			cdata->buffer_addr = NULL;
			mpc_common_debug_fatal("Failed to allocate in memory pool");
		}
		else
		{
			/* Fill the buffer */
			OPA_store_int( (OPA_int_t *)cdata->buffer_addr, coll->comm_size);
			memcpy(cdata->buffer_addr + sizeof(OPA_int_t), sendbuf, to_scatter_size);
			cdata->is_counter = 1;
		}
	}
	else
	{
		cdata->buffer_addr = sendbuf;
	}

	__INTERNAL__PMPI_Bcast_intra_shared_node(&cdata->buffer_addr, sizeof(struct shared_node_coll_data), MPI_CHAR, root, comm);

	if(cdata->buffer_addr != NULL)
	{
		if(cdata->is_counter)
		{
			memcpy(recvbuf, cdata->buffer_addr + sizeof(OPA_int_t) + ((ptrdiff_t)rank * tsize * sendcnt), (long)recvcnt * tsize);
			int token = OPA_fetch_and_decr_int( (OPA_int_t *)cdata->buffer_addr);
			if(token == 1)
			{
				mpc_lowcomm_allocmem_pool_free_size(cdata->buffer_addr, to_scatter_size + sizeof(OPA_int_t) );
			}
		}
		else
		{
			memcpy(recvbuf, cdata->buffer_addr + ((ptrdiff_t)rank * tsize * sendcnt), (long)recvcnt * tsize);
		}
	}
	else
	{
		/* Fallback to regular bcast */
		return __INTERNAL__PMPI_Scatter_intra(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Scatter_intra_shared_node(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                                               void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                                               int root, MPI_Comm comm)
{
	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	//TODO to expose as config vars
	if(__MPC_node_comm_coll_check(coll, comm) &&
	   mpc_lowcomm_allocmem_is_in_pool(sendbuf) )
	{
		int bool_val = _mpc_dt_is_contig_mem(sendtype);
		bool_val &= (sendtype == recvtype);
		bool_val &= (sendcnt == recvcnt);

		int rez = 0;

		PMPI_Allreduce(&bool_val, &rez, 1, MPI_INT, MPI_BAND, comm);

		if(!rez)
		{
			return __INTERNAL__PMPI_Scatter_intra(sendbuf, sendcnt, sendtype,
			                                      recvbuf, recvcnt, recvtype,
			                                      root, comm);
		}


		/* We need to be able to amortize the agreement allreduce */
		return __INTERNAL__PMPI_Scatter_intra_shared_node_impl(sendbuf, sendcnt, sendtype,
		                                                       recvbuf, recvcnt, recvtype,
		                                                       root, comm, coll);
	}
	else
	{
		return __INTERNAL__PMPI_Scatter_intra(sendbuf, sendcnt, sendtype,
		                                      recvbuf, recvcnt, recvtype,
		                                      root, comm);
	}
}

int __INTERNAL__PMPI_Scatterv_intra_shm(void *sendbuf, int *sendcnts,
                                        int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcnt,
                                        MPI_Datatype recvtype, int root,
                                        MPI_Comm comm)
{
	struct sctk_comm_coll *     coll   = mpc_communicator_shm_coll_get(comm);
	struct shared_mem_scatterv *sv_ctx = &coll->shm_scatterv;

	mpc_common_nodebug("SCATTER SEND %d CNT %d RECV %d CNT %d", sendtype, sendcnts[0],
	                   recvtype, recvcnt);

	int rank, res;

	PMPI_Comm_rank(comm, &rank);

	/* First pay the toll gate */
	if(__do_yield)
	{
		while(sv_ctx->tollgate[rank] != OPA_load_int(&sv_ctx->fare) )
		{
			mpc_thread_yield();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}
	else
	{
		while(sv_ctx->tollgate[rank] != OPA_load_int(&sv_ctx->fare) )
		{
			sctk_cpu_relax();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}

	/* Reverse state so that only a root done can unlock by
	 * also reversing the fare */
	sv_ctx->tollgate[rank] = !sv_ctx->tollgate[rank];

	void *data_buff = sendbuf;

	MPI_Aint rtype_size = 0;

	if(rank != root)
	{
		res = PMPI_Type_extent(recvtype, &rtype_size);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	/* RDV with ROOT */

	/* Now am I the root ? */
	if(root == rank)
	{
		MPI_Aint stsize = 0;
		res = PMPI_Type_extent(sendtype, &stsize);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		if(__do_yield)
		{
			while(OPA_cas_int(&sv_ctx->owner, -1, -2) != -1)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_cas_int(&sv_ctx->owner, -1, -2) != -1)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		sv_ctx->send_type = sendtype;

		/* Does root need to pack ? */
		if(!_mpc_dt_is_contig_mem(sendtype) )
		{
			/* Sorry derived data-types involved
			 * lets pack it all */

			/* Are we in the Scatter config ? */
			if(!displs)
			{
				int cnt = 0;
				mpc_common_nodebug("PACK S t %d cnt %d extent %d", sendtype,
				                   sendcnts[0] * coll->comm_size, stsize);
				/* We are a Scatter */
				size_t buff_size = sendcnts[0] * stsize * coll->comm_size;
				data_buff = sctk_malloc(buff_size);
				assume(data_buff != NULL);
				PMPI_Pack(sendbuf, sendcnts[0] * coll->comm_size, sendtype, data_buff,
				          buff_size, &cnt, comm);
				/* Only store in 0 the big Pack */
				OPA_store_ptr(&sv_ctx->src_buffs[0], data_buff);
				sv_ctx->stype_size = cnt / (coll->comm_size * sendcnts[0]);
			}
			else
			{
				/* We are a Scatterv */
				int i;
				for(i = 0; i < coll->comm_size; ++i)
				{
					int   cnt  = 0;
					void *from = sendbuf + displs[i] * stsize;

					data_buff = sctk_malloc(sendcnts[i] * stsize);
					assume(data_buff != NULL);
					PMPI_Pack(from, sendcnts[i], sendtype, data_buff,
					          stsize * sendcnts[i], &cnt, comm);
					/* Only store in 0 the big Pack */
					OPA_store_ptr(&sv_ctx->src_buffs[i], data_buff);
					if(sendcnts[i])
					{
						sv_ctx->stype_size = cnt / sendcnts[i];
					}
				}
			}


			/* Notify leaves that it is
			 * going to be expensive */
			sv_ctx->was_packed = 1;
		}
		else
		{
			/* Yes ! We are contiguous we can start
			 * to perform a little */
			OPA_store_ptr(&sv_ctx->src_buffs[0], data_buff);
			/* Notify leaves that we are on fastpath */
			sv_ctx->was_packed = 0;
			sv_ctx->stype_size = stsize;
		}

		/* Set root infos */
		sv_ctx->disps  = displs;
		sv_ctx->counts = sendcnts;

		/* Now unleash the others */
		OPA_store_int(&sv_ctx->owner, rank);
	}
	else
	{
		/* Wait for the root */
		if(__do_yield)
		{
			while(OPA_load_int(&sv_ctx->owner) != root)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&sv_ctx->owner) != root)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
	}

	//if (!_mpc_dt_is_contig_mem(sendtype) && rank != root) {
	//sv_ctx->was_packed = 1;
	//}
	/* Where to write ? */
	if(recvbuf != MPI_IN_PLACE)
	{
		if(sv_ctx->was_packed)
		{
			void * from    = NULL;
			size_t to_cpy  = 0;
			int    do_free = 0;

			/* Root packed it all for us
			 * as it has a non-contig datatype */

			/* Is it already packed on our side ? */
			if(!displs)
			{
				mpc_common_nodebug("UNPACK S RT %d rcnt %d rext %d FROM %d ext %d", recvtype,
				                   recvcnt, rtype_size, sv_ctx->counts[0],
				                   sv_ctx->stype_size);
				/* We are a Scatter only data in [0] */
				void *data = OPA_load_ptr(&sv_ctx->src_buffs[0]);
				from   = data + rank * sv_ctx->counts[0] * sv_ctx->stype_size;
				to_cpy = sv_ctx->counts[0] * sv_ctx->stype_size;

				/* Will be freed by root */
			}
			else
			{
				/* We are a ScatterV data in the whole array */
				from    = OPA_load_ptr(&sv_ctx->src_buffs[rank]);
				to_cpy  = sv_ctx->counts[rank] * sv_ctx->stype_size;
				do_free = 1;
			}

			int cnt = 0;
			PMPI_Unpack(from, to_cpy, &cnt, recvbuf, recvcnt, recvtype, comm);

			if(do_free)
			{
				sctk_free(from);
			}
		}
		else
		{
			/* If we are here we can directly read
			 * in the target buffer as the type is contig
			 * we just have to look for the right disp */
			void * from   = NULL;
			size_t to_cpy = 0;

			if(!sv_ctx->disps)
			{
				/* Scatter case */
				void *data = OPA_load_ptr(&sv_ctx->src_buffs[0]);
				from   = data + sv_ctx->counts[0] * rank * sv_ctx->stype_size;
				to_cpy = sv_ctx->counts[0] * sv_ctx->stype_size;
			}
			else
			{
				/* ScatterV case */
				void *data = OPA_load_ptr(&sv_ctx->src_buffs[0]);
				from   = data + sv_ctx->disps[rank] * sv_ctx->stype_size;
				to_cpy = sv_ctx->counts[rank] * sv_ctx->stype_size;
			}

			if(!_mpc_dt_is_contig_mem(recvtype) )
			{
				/* Recvtype is non-contig */
				int cnt = 0;
				PMPI_Unpack(from, to_cpy, &cnt, recvbuf, recvcnt, recvtype, comm);
			}
			else
			{
				/* Recvtype is contiguous */
				memcpy(recvbuf, from, to_cpy);
			}
		}
	}

	OPA_decr_int(&sv_ctx->left_to_pop);

	if(rank == root)
	{
		/* Wait for all the others */
		if(__do_yield)
		{
			while(OPA_load_int(&sv_ctx->left_to_pop) != 0)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&sv_ctx->left_to_pop) != 0)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		/* Do we need to free data packed for Scatter case ? */
		if(!_mpc_dt_is_contig_mem(sendtype) )
		{
			if(!displs)
			{
				void *data = OPA_load_ptr(&sv_ctx->src_buffs[0]);
				sctk_free(data);
			}
		}

		/* On free */
		OPA_store_int(&sv_ctx->left_to_pop, coll->comm_size);

		OPA_store_int(&sv_ctx->owner, -1);

		int current_fare = OPA_load_int(&sv_ctx->fare);
		OPA_store_int(&sv_ctx->fare, !current_fare);
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Scatter_intra_shm(void *sendbuf, int sendcnt,
                                       MPI_Datatype sendtype,
                                       void *recvbuf, int recvcnt,
                                       MPI_Datatype recvtype, int root,
                                       MPI_Comm comm)
{
	return __INTERNAL__PMPI_Scatterv_intra_shm(sendbuf, &sendcnt, NULL, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int __INTERNAL__PMPI_Scatterv_intra(void *sendbuf, int *sendcnts, int *displs,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcnt, MPI_Datatype recvtype,
                                    int root, MPI_Comm comm)
{
	return _mpc_mpi_collectives_scatterv(sendbuf, sendcnts, displs, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int __INTERNAL__PMPI_Scatterv_inter(const void *sendbuf, const int *sendcnts, const int *displs,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcnt, MPI_Datatype recvtype,
                                    int root, MPI_Comm comm)
{
	int      i, rsize, res = MPI_ERR_INTERN;
	char *   ptmp;
	MPI_Aint extent;

	MPI_Request *sendrequest;

	res = PMPI_Comm_remote_size(comm, &rsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	sendrequest = (MPI_Request *)sctk_malloc(rsize * sizeof(MPI_Request) );

	if(root == MPI_PROC_NULL)
	{
		res = MPI_SUCCESS;
	}
	else if(MPI_ROOT != root)
	{
		res = PMPI_Recv_internal(recvbuf, recvcnt, recvtype, root,
		                         MPC_SCATTERV_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}
	}
	else
	{
		res = PMPI_Type_extent(sendtype, &extent);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}

		for(i = 0; i < rsize; ++i)
		{
			ptmp = ( (char *)sendbuf) + (extent * displs[i]);
			res  = PMPI_Isend_internal(ptmp, sendcnts[i], sendtype, i,
			                           MPC_SCATTERV_TAG, comm, &(sendrequest[i]) );
			if(res != MPI_SUCCESS)
			{
				sctk_free(sendrequest);
				return res;
			}
		}

		res = PMPI_Waitall(rsize, sendrequest, MPI_STATUSES_IGNORE);
		if(res != MPI_SUCCESS)
		{
			sctk_free(sendrequest);
			return res;
		}
	}
	sctk_free(sendrequest);
	return res;
}

int __INTERNAL__PMPI_Allgather_intra(void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     MPI_Comm comm)
{
	return _mpc_mpi_collectives_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int __INTERNAL__PMPI_Allgather_inter(void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     MPI_Comm comm)
{
	int      size, rank, remote_size, res = MPI_ERR_INTERN;
	int      root = 0;
	MPI_Aint slb, sextent;
	void *   tmp_buf = NULL;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &remote_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if( (rank == 0) && (sendcount != 0) )
	{
		res = PMPI_Type_lb(sendtype, &slb);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
		res = PMPI_Type_extent(sendtype, &sextent);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		tmp_buf = (void *)sctk_malloc(sextent * sendcount * size * sizeof(void *) );
		tmp_buf = (void *)( (char *)tmp_buf - slb);
	}

	if(sendcount != 0)
	{
		res = PMPI_Gather(sendbuf, sendcount, sendtype, tmp_buf,
		                  sendcount, sendtype, 0,
		                  mpc_lowcomm_communicator_get_local(comm) );
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	if(mpc_lowcomm_communicator_in_master_group(comm) )
	{
		if(sendcount != 0)
		{
			root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
			res  = PMPI_Bcast(tmp_buf, size * sendcount, sendtype, root,
			                  comm);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}

		if(recvcount != 0)
		{
			root = 0;
			res  = PMPI_Bcast(recvbuf, remote_size * recvcount, recvtype,
			                  root, comm);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}
	}
	else
	{
		if(recvcount != 0)
		{
			root = 0;
			res  = PMPI_Bcast(recvbuf, remote_size * recvcount, recvtype,
			                  root, comm);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}

		if(sendcount != 0)
		{
			root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
			res  = PMPI_Bcast(tmp_buf, size * sendcount, sendtype, root,
			                  comm);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}
	}
	return res;
}

int __INTERNAL__PMPI_Allgatherv_intra(void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int *recvcounts, int *displs,
                                      MPI_Datatype recvtype, MPI_Comm comm)
{
	return _mpc_mpi_collectives_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

int __INTERNAL__PMPI_Allgatherv_inter(const void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const int *recvcounts, const int *displs,
                                      MPI_Datatype recvtype, MPI_Comm comm)
{
	int      size, rsize, rank, res = MPI_ERR_INTERN;
	int      root = 0;
	MPI_Aint extent;
	mpc_lowcomm_communicator_t local_comm;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &rsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(mpc_lowcomm_communicator_in_master_group(comm) )
	{
		root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
		res  = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
		                    recvcounts, displs, recvtype, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		root = 0;
		res  = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
		                    recvcounts, displs, recvtype, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		root = 0;
		res  = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
		                    recvcounts, displs, recvtype, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
		res  = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
		                    recvcounts, displs, recvtype, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	rsize--;
	root       = 0;
	local_comm = mpc_lowcomm_communicator_get_local(comm);
	res        = PMPI_Type_extent(recvtype, &extent);
	for(; rsize >= 0; rsize--)
	{
		res = PMPI_Bcast( ( (char *)recvbuf) + (displs[rsize] * extent),
		                  recvcounts[rsize], recvtype, root, local_comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	return res;
}

int __INTERNAL__PMPI_Alltoall_intra(void *sendbuf, int sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype,
                                    MPI_Comm comm)
{
	return _mpc_mpi_collectives_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int __INTERNAL__PMPI_Alltoall_inter(void *sendbuf, int sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype,
                                    MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int local_size, remote_size, max_size, i;
	mpc_lowcomm_status_t status;
	MPI_Aint             sendtype_extent, recvtype_extent;
	int   src, dst, rank;
	char *sendaddr, *recvaddr;

	res = _mpc_cl_comm_size(comm, &local_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &remote_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Type_extent(sendtype, &sendtype_extent);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Type_extent(recvtype, &recvtype_extent);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	max_size = (local_size < remote_size) ? remote_size : local_size;
	for(i = 0; i < max_size; i++)
	{
		src = (rank - i + max_size) % max_size;
		dst = (rank + i) % max_size;

		if(src >= remote_size)
		{
			src      = MPI_PROC_NULL;
			recvaddr = NULL;
		}
		else
		{
			recvaddr = (char *)recvbuf + src * recvcount * recvtype_extent;
		}

		if(dst >= remote_size)
		{
			dst      = MPI_PROC_NULL;
			sendaddr = NULL;
		}
		else
		{
			sendaddr = (char *)sendbuf + dst * sendcount * sendtype_extent;
		}
		res = PMPI_Sendrecv_internal(
			sendaddr, sendcount, sendtype, dst, MPC_ALLTOALL_TAG, recvaddr,
			recvcount, recvtype, src, MPC_ALLTOALL_TAG, comm, &status);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	return res;
}

int __INTERNAL__PMPI_Alltoall_intra_shared_node(void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPI_Comm comm)
{
	/* We handle the simple case only */
	int bool_val = _mpc_dt_is_contig_mem(sendtype);

	bool_val &= (sendtype == recvtype);
	bool_val &= (sendcount == recvcount);

	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	if(!__MPC_node_comm_coll_check(coll, comm) )
	{
		return __INTERNAL__PMPI_Alltoall_intra(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}

	int rez = 0;

	PMPI_Allreduce(&bool_val, &rez, 1, MPI_INT, MPI_BAND, comm);

	if(!rez)
	{
		return __INTERNAL__PMPI_Alltoall_intra(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}
	else
	{
		int i;
		int tsize, rank;
		int ret;

		ret = PMPI_Type_size(sendtype, &tsize);
		if (ret != MPI_SUCCESS)
		{
			return ret;
		}

		ret = PMPI_Comm_rank(comm, &rank);
		if (ret != MPI_SUCCESS)
		{
			return ret;
		}

		for(i = 0; i < coll->comm_size; i++)
		{
			PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf + (tsize * recvcount) * i, recvcount, recvtype, i, comm);
		}
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Alltoallv_intra_shm(const void *sendbuf, const int *sendcnts,
                                         const int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, int *recvcnts,
                                         const int *rdispls, MPI_Datatype recvtype,
                                         MPI_Comm comm)
{
	struct sctk_comm_coll *coll   = mpc_communicator_shm_coll_get(comm);
	struct shared_mem_a2a *aa_ctx = &coll->shm_a2a;

	int rank;

	_mpc_cl_comm_rank(comm, &rank);

	struct sctk_shared_mem_a2a_infos info;
	int is_in_place = 0;

	if(sendbuf == MPI_IN_PLACE)
	{
		info.sendtype = recvtype;
		info.recvtype = recvtype;

		PMPI_Type_extent(recvtype, &info.stype_size);
		info.disps  = rdispls;
		info.counts = recvcnts;
		is_in_place = 1;
		size_t to_cpy = 0;
		if(sdispls)
		{
			int i;
			for(i = 0; i < coll->comm_size; i++)
			{
				to_cpy += info.stype_size * rdispls[i];
			}
			to_cpy += recvcnts[coll->comm_size - 1] * info.stype_size;
		}
		else
		{
			to_cpy = coll->comm_size * recvcnts[0] * info.stype_size;
		}
		info.source_buff = malloc(to_cpy);
		memcpy( (void *)info.source_buff, recvbuf, to_cpy);
	}
	else
	{
		info.sendtype = sendtype;
		info.recvtype = recvtype;

		PMPI_Type_extent(sendtype, &info.stype_size);
		info.source_buff = sendbuf;
		info.disps       = sdispls;
		info.counts      = sendcnts;
	}

	MPI_Aint rtsize;

	PMPI_Type_extent(recvtype, &rtsize);

	/* Pack if needed */
	if(!_mpc_dt_is_contig_mem(sendtype) )
	{
		int i;

		info.packed_buff = sctk_malloc(sizeof(void *) * coll->comm_size);

		assume(info.packed_buff != NULL);

		for(i = 0; i < coll->comm_size; i++)
		{
			size_t to_cpy = 0;
			void * from   = NULL;
			int    scnt   = 0;

			if(!sdispls)
			{
				/* Alltoall */
				from   = (void *)sendbuf + sendcnts[0] * i * info.stype_size;
				to_cpy = info.stype_size * sendcnts[0];
				scnt   = sendcnts[0];
			}
			else
			{
				/* Alltoallv */
				from   = (void *)sendbuf + sdispls[i] * info.stype_size;
				to_cpy = info.stype_size * sendcnts[i];
				scnt   = sendcnts[i];
			}

			/* Sendtype is non-contig */
			info.packed_buff[i] = sctk_malloc(to_cpy);
			assume(info.source_buff != NULL);

			int cnt = 0;
			PMPI_Pack(from, scnt, sendtype, info.packed_buff[i], to_cpy, &cnt, comm);
		}
	}
	else
	{
		info.packed_buff = NULL;
	}

	/* Register the infos in the array */
	aa_ctx->infos[rank] = &info;

	PMPI_Barrier(comm);

	int i, j;

	for(j = 0; j < coll->comm_size; j++)
	{
		for(i = 0; i < coll->comm_size; i++)
		{
			int err = mpc_lowcomm_check_type_compat(aa_ctx->infos[i]->sendtype, aa_ctx->infos[j]->recvtype);
			if(err != MPI_SUCCESS)
			{
				return err;
			}
		}
	}

	for(j = 0; j < coll->comm_size; j++)
	{
		/* Try to split readings */
		i = (rank + j) % coll->comm_size;

		/* No need to copy if we work in place */
		if(is_in_place && (rank == i) )
		{
			continue;
		}

		/* Get data from each rank */

		void * from   = NULL;
		void * to     = NULL;
		size_t to_cpy = 0;
		int    rcount = 0;

		/* Choose dest and size */
		if(sdispls && rdispls)
		{
			/* This is All2Allv */
			// to_cpy = aa_ctx->infos[i]->counts[i] * aa_ctx->infos[i]->stype_size;
			to_cpy = recvcnts[i] * rtsize;
			to     = recvbuf + (rdispls[i] * rtsize);
			rcount = recvcnts[i];
		}
		else
		{
			/* This is All2All */
			// to_cpy = aa_ctx->infos[i]->counts[0] * aa_ctx->infos[i]->stype_size;
			to_cpy = recvcnts[0] * rtsize;
			to     = recvbuf + (recvcnts[0] * rtsize) * i;
			rcount = recvcnts[0];
		}

		if(aa_ctx->infos[i]->packed_buff)
		{
			from = aa_ctx->infos[i]->packed_buff[rank];
		}
		else
		{
			if(sdispls && rdispls)
			{
				/* Alltoallv */
				from = (void *)aa_ctx->infos[i]->source_buff +
				       aa_ctx->infos[i]->disps[rank] * aa_ctx->infos[i]->stype_size;
			}
			else
			{
				/* Alltoall */
				from = (void *)aa_ctx->infos[i]->source_buff + to_cpy * rank;
			}
		}

		if(!_mpc_dt_is_contig_mem(recvtype) )
		{
			/* Recvtype is non-contig */
			int cnt = 0;
			PMPI_Unpack(from, to_cpy, &cnt, to, rcount, recvtype, comm);
		}
		else
		{
			/* We can memcpy */
			memcpy(to, from, to_cpy);
		}
	}

	PMPI_Barrier(comm);

	if(is_in_place)
	{
		sctk_free( (void *)info.source_buff);
	}

	if(!_mpc_dt_is_contig_mem(sendtype))
	{
		if (info.packed_buff != NULL)
		{
			for(i = 0; i < coll->comm_size; i++)
			{
				sctk_free(info.packed_buff[i]);
			}

			sctk_free(info.packed_buff);
		}

		info.source_buff = NULL;
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Alltoallv_intra(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype recvtype, MPI_Comm comm)
{
	return _mpc_mpi_collectives_alltoallv(sendbuf, sendcnts, sdispls, sendtype, recvbuf, recvcnts, rdispls, recvtype, comm);
}

int __INTERNAL__PMPI_Alltoallv_inter(const void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     const int *recvcnts, const int *rdispls,
                                     MPI_Datatype recvtype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int local_size, remote_size, max_size, i;
	mpc_lowcomm_status_t status;
	size_t sendtype_extent, recvtype_extent;
	int    src, dst, rank, sendcount, recvcount;
	char * sendaddr, *recvaddr;

	res = _mpc_cl_comm_size(comm, &local_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &remote_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Type_extent(sendtype, (MPI_Aint *)&sendtype_extent);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Type_extent(recvtype, (MPI_Aint *)&recvtype_extent);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	max_size = (local_size < remote_size) ? remote_size : local_size;
	for(i = 0; i < max_size; i++)
	{
		src = (rank - i + max_size) % max_size;
		dst = (rank + i) % max_size;
		if(src >= remote_size)
		{
			src       = MPI_PROC_NULL;
			recvaddr  = NULL;
			recvcount = 0;
		}
		else
		{
			recvaddr  = (char *)recvbuf + rdispls[src] * recvtype_extent;
			recvcount = recvcnts[src];
		}
		if(dst >= remote_size)
		{
			dst       = MPI_PROC_NULL;
			sendaddr  = NULL;
			sendcount = 0;
		}
		else
		{
			sendaddr  = (char *)sendbuf + sdispls[dst] * sendtype_extent;
			sendcount = sendcnts[dst];
		}

		res = PMPI_Sendrecv_internal(
			sendaddr, sendcount, sendtype, dst, MPC_ALLTOALLV_TAG, recvaddr,
			recvcount, recvtype, src, MPC_ALLTOALLV_TAG, comm, &status);
	}
	return res;
}

int __INTERNAL__PMPI_Alltoallw_intra(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype *sendtypes, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype *recvtypes, MPI_Comm comm)
{
	return _mpc_mpi_collectives_alltoallw(sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);
}

int __INTERNAL__PMPI_Alltoallw_inter(void *sendbuf, int *sendcnts, int *sdispls,
                                     MPI_Datatype *sendtypes, void *recvbuf,
                                     int *recvcnts, int *rdispls,
                                     MPI_Datatype *recvtypes, MPI_Comm comm)
{
	int          res = MPI_ERR_INTERN;
	int          local_size, remote_size, max_size, i;
	MPI_Status   status;
	MPI_Datatype sendtype, recvtype;
	int          src, dst, rank, sendcount, recvcount;
	char *       sendaddr, *recvaddr;

	res = _mpc_cl_comm_size(comm, &local_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &remote_size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	max_size = (local_size < remote_size) ? remote_size : local_size;
	for(i = 0; i < max_size; i++)
	{
		src = (rank - i + max_size) % max_size;
		dst = (rank + i) % max_size;
		if(src >= remote_size)
		{
			src       = MPI_PROC_NULL;
			recvaddr  = NULL;
			recvcount = 0;
			recvtype  = MPI_DATATYPE_NULL;
		}
		else
		{
			recvaddr  = (char *)recvbuf + rdispls[src];
			recvcount = recvcnts[src];
			recvtype  = recvtypes[src];
		}
		if(dst >= remote_size)
		{
			dst       = MPI_PROC_NULL;
			sendaddr  = NULL;
			sendcount = 0;
			sendtype  = MPI_DATATYPE_NULL;
		}
		else
		{
			sendaddr  = (char *)sendbuf + sdispls[dst];
			sendcount = sendcnts[dst];
			sendtype  = sendtypes[dst];
		}

		res = PMPI_Sendrecv_internal(
			sendaddr, sendcount, sendtype, dst, MPC_ALLTOALLW_TAG, recvaddr,
			recvcount, recvtype, src, MPC_ALLTOALLW_TAG, comm, &status);
	}
	return res;
}

/* Neighbor collectives */
static int __INTERNAL__PMPI_Neighbor_allgather_cart(
	const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	MPI_Aint                    extent;
	int                         rank;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;
	int                         rc = MPI_SUCCESS, dim, nreqs = 0;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	rc = PMPI_Type_extent(recvtype, &extent);
	MPI_HANDLE_ERROR(rc, comm, "Type extent failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc( (4 * (topo->data.cart.ndims) ) * sizeof(MPI_Request *) );

	for(dim = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", srank,
				rank);
			rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, srank, 2, comm,
			                         &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank,
				srank);
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, srank, 2, comm,
			                          &reqs[nreqs + 1]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			nreqs += 2;
		}

		recvbuf = (char *)recvbuf + extent * recvcount;

		if(drank != MPI_PROC_NULL)
		{
			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgather_cart: Recv from %d to %d", drank,
				rank);
			rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, drank, 2, comm,
			                         &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgather_cart: Rank %d send to %d", rank,
				drank);
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, drank, 2, comm,
			                          &reqs[nreqs + 1]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			nreqs += 2;
		}

		recvbuf = (char *)recvbuf + extent * recvcount;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgather_graph(
	const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int                         i  = 0;
	int                         rc = MPI_SUCCESS;
	int                         degree;
	int                         neighbor;
	int                         rank;
	const int *                 edges;
	MPI_Aint                    extent;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	rc = PMPI_Graph_neighbors_count(comm, rank, &degree);
	MPI_HANDLE_ERROR(rc, comm, "Graph neighbors count failed");

	edges = topo->data.graph.edges;
	if(rank > 0)
	{
		edges += topo->data.graph.index[rank - 1];
	}

	rc = PMPI_Type_extent(recvtype, &extent);
	MPI_HANDLE_ERROR(rc, comm, "Receive type extent failed");

	reqs = sctk_malloc( (2 * degree) * sizeof(MPI_Request *) );

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		mpc_common_nodebug(
			"__INTERNAL__PMPI_Neighbor_allgather_graph: Recv from %d to %d",
			edges[neighbor], rank);
		rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm,
		                         &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
		recvbuf = (char *)recvbuf + extent * recvcount;

		mpc_common_nodebug(
			"__INTERNAL__PMPI_Neighbor_allgather_graph: Rank %d send to %d", rank,
			edges[neighbor]);
		rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, edges[neighbor], 1,
		                          comm, &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}
	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_cart(
	const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	MPI_Aint                    extent;
	int                         rank;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;
	int                         rc = MPI_SUCCESS, dim, nreqs = 0, i;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	rc = PMPI_Type_extent(recvtype, &extent);
	MPI_HANDLE_ERROR(rc, comm, "Type extent failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc( (4 * (topo->data.cart.ndims) ) * sizeof(MPI_Request *) );

	for(dim = 0, i = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim, i += 2)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d",
				srank, rank);
			rc = PMPI_Irecv_internal( (char *)recvbuf + displs[i] * extent, recvcounts[i],
			                          recvtype, srank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank,
				srank);
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, srank, 2, comm,
			                          &reqs[nreqs + 1]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs += 2;
		}

		if(drank != MPI_PROC_NULL)
		{
			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgatherv_cart: Recv from %d to %d",
				drank, rank);
			rc =
				PMPI_Irecv_internal( (char *)recvbuf + displs[i + 1] * extent,
				                     recvcounts[i + 1], recvtype, drank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}

			mpc_common_nodebug(
				"__INTERNAL__PMPI_Neighbor_allgatherv_cart: Rank %d send to %d", rank,
				drank);
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, drank, 2, comm,
			                          &reqs[nreqs + 1]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs += 2;
		}
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_allgatherv_graph(
	const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int                         i  = 0;
	int                         rc = MPI_SUCCESS;
	int                         degree;
	int                         neighbor;
	int                         rank;
	const int *                 edges;
	MPI_Aint                    extent;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	rc = PMPI_Graph_neighbors_count(comm, rank, &degree);
	MPI_HANDLE_ERROR(rc, comm, "Graph neighbors count failed");

	edges = topo->data.graph.edges;
	if(rank > 0)
	{
		edges += topo->data.graph.index[rank - 1];
	}

	rc = PMPI_Type_extent(recvtype, &extent);
	MPI_HANDLE_ERROR(rc, comm, "Type extent failed");

	reqs = sctk_malloc( (2 * degree) * sizeof(MPI_Request *) );

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		mpc_common_nodebug(
			"__INTERNAL__PMPI_Neighbor_allgatherv_graph: Recv from %d to %d",
			edges[neighbor], rank);
		rc = PMPI_Irecv_internal( (char *)recvbuf + displs[neighbor] * extent,
		                          recvcounts[neighbor], recvtype, edges[neighbor], 2, comm,
		                          &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;

		mpc_common_nodebug(
			"__INTERNAL__PMPI_Neighbor_allgatherv_graph: Rank %d send to %d", rank,
			edges[neighbor]);
		rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, edges[neighbor], 2,
		                          comm, &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_cart(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype,
                                                   void *recvbuf, int recvcount,
                                                   MPI_Datatype recvtype,
                                                   MPI_Comm comm)
{
	MPI_Aint                    rdextent;
	MPI_Aint                    sdextent;
	int                         rank;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;
	int                         rc = MPI_SUCCESS, dim, nreqs = 0;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	rc = PMPI_Type_extent(recvtype, &rdextent);
	MPI_HANDLE_ERROR(rc, comm, "Receive type extent failed");

	rc = PMPI_Type_extent(sendtype, &sdextent);
	MPI_HANDLE_ERROR(rc, comm, "Send type extent failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc( (4 * (topo->data.cart.ndims) ) * sizeof(MPI_Request *) );

	for(dim = 0, nreqs = 0; dim < topo->data.cart.ndims; ++dim)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, srank, 2, comm,
			                         &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		recvbuf = (char *)recvbuf + rdextent * recvcount;

		if(drank != MPI_PROC_NULL)
		{
			rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, drank, 2, comm,
			                         &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		recvbuf = (char *)recvbuf + rdextent * recvcount;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	for(dim = 0; dim < topo->data.cart.ndims; ++dim)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, srank, 2, comm,
			                          &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		sendbuf = (char *)sendbuf + sdextent * sendcount;

		if(drank != MPI_PROC_NULL)
		{
			rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, drank, 2, comm,
			                          &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		sendbuf = (char *)sendbuf + sdextent * sendcount;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoall_graph(
	const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int                         i  = 0;
	int                         rc = MPI_SUCCESS;
	int                         degree;
	int                         neighbor;
	int                         rank;
	const int *                 edges;
	MPI_Aint                    rdextent;
	MPI_Aint                    sdextent;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");
	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	rc = PMPI_Graph_neighbors_count(comm, rank, &degree);
	MPI_HANDLE_ERROR(rc, comm, "Graph neighbors count failed");

	edges = topo->data.graph.edges;
	if(rank > 0)
	{
		edges += topo->data.graph.index[rank - 1];
	}

	rc = PMPI_Type_extent(recvtype, &rdextent);
	MPI_HANDLE_ERROR(rc, comm, "Receive type extent failed");

	rc = PMPI_Type_extent(sendtype, &sdextent);
	MPI_HANDLE_ERROR(rc, comm, "Send type extent failed");

	reqs = sctk_malloc( (2 * degree) * sizeof(MPI_Request *) );

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, edges[neighbor], 1, comm,
		                         &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
		recvbuf = (char *)recvbuf + rdextent * recvcount;
	}

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Isend_internal( (void *)sendbuf, sendcount, sendtype, edges[neighbor], 1,
		                          comm, &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
		sendbuf = (char *)sendbuf + sdextent * sendcount;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_cart(
	const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
	void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
	MPI_Comm comm)
{
	MPI_Aint                    rdextent;
	MPI_Aint                    sdextent;
	int                         rank;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;
	int                         rc = MPI_SUCCESS, dim, nreqs = 0, i;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	rc = PMPI_Type_extent(recvtype, &rdextent);
	MPI_HANDLE_ERROR(rc, comm, "Receive type extent failed");

	rc = PMPI_Type_extent(recvtype, &sdextent);
	MPI_HANDLE_ERROR(rc, comm, "Send type extent failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc( (4 * (topo->data.cart.ndims) ) * sizeof(MPI_Request *) );

	for(dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Irecv_internal( (char *)recvbuf + rdispls[i] * rdextent, recvcounts[i],
			                          recvtype, srank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		if(drank != MPI_PROC_NULL)
		{
			rc =
				PMPI_Irecv_internal( (char *)recvbuf + rdispls[i + 1] * rdextent,
				                     recvcounts[i + 1], recvtype, drank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	for(dim = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Isend_internal( (char *)sendbuf + sdispls[i] * sdextent, sendcounts[i],
			                          sendtype, srank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		if(drank != MPI_PROC_NULL)
		{
			rc =
				PMPI_Isend_internal( (char *)sendbuf + sdispls[i + 1] * sdextent,
				                     sendcounts[i + 1], sendtype, drank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallv_graph(
	const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype,
	void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype,
	MPI_Comm comm)
{
	int                         i  = 0;
	int                         rc = MPI_SUCCESS;
	int                         degree;
	int                         neighbor;
	int                         rank;
	const int *                 edges;
	MPI_Aint                    rdextent;
	MPI_Aint                    sdextent;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	rc = PMPI_Graph_neighbors_count(comm, rank, &degree);
	MPI_HANDLE_ERROR(rc, comm, "Graph neighbors count failed");

	edges = topo->data.graph.edges;
	if(rank > 0)
	{
		edges += topo->data.graph.index[rank - 1];
	}

	rc = PMPI_Type_extent(recvtype, &rdextent);
	MPI_HANDLE_ERROR(rc, comm, "Receive type extent failed");

	rc = PMPI_Type_extent(sendtype, &sdextent);
	MPI_HANDLE_ERROR(rc, comm, "Send type extent failed");

	reqs = sctk_malloc( (2 * degree) * sizeof(MPI_Request *) );

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Irecv_internal( (char *)recvbuf + rdispls[neighbor] * rdextent,
		                          recvcounts[neighbor], recvtype, edges[neighbor], 1, comm,
		                          &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Isend_internal( (char *)sendbuf + sdispls[neighbor] * sdextent,
		                          sendcounts[neighbor], sendtype, edges[neighbor], 1, comm,
		                          &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_cart(
	const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
	const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
	const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int                         rank;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;
	int                         rc = MPI_SUCCESS, dim, nreqs = 0, i;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");


	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	reqs = sctk_malloc( (4 * (topo->data.cart.ndims) ) * sizeof(MPI_Request *) );

	for(dim = 0, nreqs = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Irecv_internal( (char *)recvbuf + rdispls[i], recvcounts[i], recvtypes[i],
			                          srank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		if(drank != MPI_PROC_NULL)
		{
			rc = PMPI_Irecv_internal( (char *)recvbuf + rdispls[i + 1], recvcounts[i + 1],
			                          recvtypes[i + 1], drank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	for(dim = 0, i = 0; dim < topo->data.cart.ndims; ++dim, i += 2)
	{
		int srank = MPI_PROC_NULL, drank = MPI_PROC_NULL;

		if(topo->data.cart.dims[dim] > 1)
		{
			PMPI_Cart_shift(comm, dim, 1, &srank, &drank);
		}
		else if(1 == topo->data.cart.dims[dim] && topo->data.cart.periods[dim])
		{
			srank = drank = rank;
		}

		if(srank != MPI_PROC_NULL)
		{
			rc = PMPI_Isend_internal( (char *)sendbuf + sdispls[i], sendcounts[i], sendtypes[i],
			                          srank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}

		if(drank != MPI_PROC_NULL)
		{
			rc = PMPI_Isend_internal( (char *)sendbuf + sdispls[i + 1], sendcounts[i + 1],
			                          sendtypes[i + 1], drank, 2, comm, &reqs[nreqs]);
			if(rc != MPI_SUCCESS)
			{
				break;
			}
			nreqs++;
		}
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
}

static int __INTERNAL__PMPI_Neighbor_alltoallw_graph(
	const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
	const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
	const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int                         i  = 0;
	int                         rc = MPI_SUCCESS;
	int                         degree;
	int                         neighbor;
	int                         rank;
	const int *                 edges;
	MPI_Request *               reqs;
	mpi_topology_per_comm_t *   topo;
	mpc_mpi_per_communicator_t *tmp;

	rc = PMPI_Comm_rank(comm, &rank);
	MPI_HANDLE_ERROR(rc, comm, "Comm rank failed");

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	rc = PMPI_Graph_neighbors_count(comm, rank, &degree);
	MPI_HANDLE_ERROR(rc, comm, "Graph neighbors count failed");

	edges = topo->data.graph.edges;
	if(rank > 0)
	{
		edges += topo->data.graph.index[rank - 1];
	}

	reqs = sctk_malloc( (2 * degree) * sizeof(MPI_Request *) );

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Irecv_internal( (char *)recvbuf + rdispls[neighbor], recvcounts[neighbor],
		                          recvtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	for(neighbor = 0; neighbor < degree; ++neighbor)
	{
		rc = PMPI_Isend_internal( (char *)sendbuf + sdispls[neighbor], sendcounts[neighbor],
		                          sendtypes[neighbor], edges[neighbor], 1, comm, &reqs[i]);
		if(rc != MPI_SUCCESS)
		{
			break;
		}
		i++;
	}

	if(rc != MPI_SUCCESS)
	{
		return rc;
	}

	return PMPI_Waitall(degree * 2, reqs, MPI_STATUSES_IGNORE);
}

struct sctk_mpi_ops_s
{
	sctk_op_t *           ops;
	int                   size;
	mpc_common_spinlock_t lock;
};

static sctk_op_t defined_op[MAX_MPI_DEFINED_OP];

#define sctk_add_op(operation)                                     \
	defined_op[MPI_ ## operation].op      = MPC_ ## operation; \
	defined_op[MPI_ ## operation].used    = 1;                 \
	defined_op[MPI_ ## operation].commute = 1;

void __sctk_init_mpi_op()
{
	sctk_mpi_ops_t *          ops;
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	static volatile int       done = 0;
	int i;

	ops       = (sctk_mpi_ops_t *)sctk_malloc(sizeof(sctk_mpi_ops_t) );
	ops->size = 0;
	ops->ops  = NULL;
	mpc_common_spinlock_init(&ops->lock, 0);

	mpc_thread_mutex_lock(&lock);
	if(done == 0)
	{
		for(i = 0; i < MAX_MPI_DEFINED_OP; i++)
		{
			defined_op[i].used = 0;
		}

		sctk_add_op(SUM);
		sctk_add_op(MAX);
		sctk_add_op(MIN);
		sctk_add_op(PROD);
		sctk_add_op(LAND);
		sctk_add_op(BAND);
		sctk_add_op(LOR);
		sctk_add_op(BOR);
		sctk_add_op(LXOR);
		sctk_add_op(BXOR);
		sctk_add_op(MINLOC);
		sctk_add_op(MAXLOC);
		done = 1;
	}
	mpc_thread_mutex_unlock(&lock);
	PMPC_Set_op(ops);
}

sctk_op_t *sctk_convert_to_mpc_op(MPI_Op op)
{
	sctk_mpi_ops_t *ops;
	sctk_op_t *     tmp;

	if(op < MAX_MPI_DEFINED_OP)
	{
		assume(defined_op[op].used == 1);
		return &(defined_op[op]);
	}

	PMPC_Get_op(&ops);
	mpc_common_spinlock_lock(&(ops->lock) );

	op -= MAX_MPI_DEFINED_OP;
	assume(op < ops->size);
	assume(ops->ops[op].used == 1);

	tmp = &(ops->ops[op]);

	mpc_common_spinlock_unlock(&(ops->lock) );
	return tmp;
}

#define ADD_FUNC_HANDLER(func, t, op)   \
	if(datatype == t)               \
	op = (sctk_Op_f)func ## _ ## t; \
	else

#define ADD_FUNC_HANDLER_C_INTEGER(op, func)                           \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INT, op)                    \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_LONG, op)                   \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_SHORT, op)                  \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED_SHORT, op)         \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED_LONG, op)          \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_LONG_LONG_INT, op)          \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT, op) \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_LONG_LONG, op)              \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED_LONG_LONG, op)     \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_SIGNED_CHAR, op)            \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UNSIGNED_CHAR, op)          \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INT8_T, op)                 \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INT16_T, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INT32_T, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INT64_T, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UINT8_T, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UINT16_T, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UINT32_T, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_UINT64_T, op)


#define ADD_FUNC_HANDLER_FORTRAN_INTEGER(op, func)                     \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INTEGER, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INTEGER1, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INTEGER2, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INTEGER4, op)               \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_INTEGER8, op)


#define ADD_FUNC_HANDLER_FLOATING_POINT(op, func)                      \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_FLOAT, op)                  \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_DOUBLE, op)                 \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_REAL, op)                   \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_DOUBLE_PRECISION, op)       \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_LONG_DOUBLE, op)            \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_REAL4, op)                  \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_REAL8, op)                  \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_REAL16, op)


#define ADD_FUNC_HANDLER_LOGICAL(op, func)                             \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_LOGICAL, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_C_BOOL, op)                 \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_CXX_BOOL, op)


#define ADD_FUNC_HANDLER_COMPLEX(op, func)                             \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_COMPLEX, op)                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_C_COMPLEX, op)              \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_C_FLOAT_COMPLEX, op)        \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_C_DOUBLE_COMPLEX, op)       \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX, op)  \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_CXX_FLOAT_COMPLEX, op)      \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_CXX_DOUBLE_COMPLEX, op)     \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX, op)\
		ADD_FUNC_HANDLER(func, MPC_DOUBLE_COMPLEX, op)                 \
		ADD_FUNC_HANDLER(func, MPC_COMPLEX, op)                        \
		ADD_FUNC_HANDLER(func, MPC_COMPLEX8, op)                       \
		ADD_FUNC_HANDLER(func, MPC_COMPLEX16, op)                      \
		ADD_FUNC_HANDLER(func, MPC_COMPLEX32, op)


#define ADD_FUNC_HANDLER_BYTE(op, func)                                \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_BYTE, op)                   \


#define ADD_FUNC_HANDLER_MULTI_LANG(op, func)                          \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_AINT, op)                   \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_OFFSET, op)                 \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_COUNT, op)

#define ADD_FUNC_HANDLER_MIN_MAX_LOC(op, func)                         \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_AINT, op)                   \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_OFFSET, op)                 \
		ADD_FUNC_HANDLER(func, MPC_LOWCOMM_COUNT, op)


#define COMPAT_DATA_TYPE_START if (0) {}

#define COMPAT_DATA_TYPE_END                                           \
	 else {                                                            \
		mpc_common_debug_error("No such operation");                   \
		mpc_common_debug_abort();                                      \
	}

static inline void type_unsupported()
{
	mpc_common_debug_abort_log(MPC_COMMON_DEBUG_INFO,
			"Reduction operation not allowed for this type");
}

#define COMPAT_DATA_TYPE_MIN_MAX(op, func)                             \
	else if(op == func) {                                              \
		ADD_FUNC_HANDLER_C_INTEGER(op, func)                           \
		ADD_FUNC_HANDLER_FORTRAN_INTEGER(op, func)                     \
		ADD_FUNC_HANDLER_FLOATING_POINT(op, func)                      \
		ADD_FUNC_HANDLER_MULTI_LANG(op, func)                          \
		type_unsupported();                                            \
	}

#define COMPAT_DATA_TYPE_SUM_PROD(op, func)                            \
	else if(op == func) {                                              \
		ADD_FUNC_HANDLER_C_INTEGER(op, func)                           \
		ADD_FUNC_HANDLER_FORTRAN_INTEGER(op, func)                     \
		ADD_FUNC_HANDLER_FLOATING_POINT(op, func)                      \
		ADD_FUNC_HANDLER_COMPLEX(op, func)                             \
		ADD_FUNC_HANDLER_MULTI_LANG(op, func)                          \
		type_unsupported();                                            \
	}

#define COMPAT_DATA_TYPE_LOGICAL_OP(op, func)                          \
	else if(op == func) {                                              \
		ADD_FUNC_HANDLER_C_INTEGER(op, func)                           \
		ADD_FUNC_HANDLER_LOGICAL(op, func)                             \
		type_unsupported();                                            \
	}

#define COMPAT_DATA_TYPE_BINARY_OP(op, func)                           \
	else if(op == func) {                                              \
		ADD_FUNC_HANDLER_C_INTEGER(op, func)                           \
		ADD_FUNC_HANDLER_FORTRAN_INTEGER(op, func)                     \
		ADD_FUNC_HANDLER_BYTE(op, func)                                \
		ADD_FUNC_HANDLER_MULTI_LANG(op, func)                          \
		type_unsupported();                                            \
	}

#define COMPAT_DATA_TYPE_MIN_MAX_LOC(op, func)            \
	else if(op == func) {                                 \
		ADD_FUNC_HANDLER(func, MPC_FLOAT_INT, op)         \
		ADD_FUNC_HANDLER(func, MPC_LONG_INT, op)          \
		ADD_FUNC_HANDLER(func, MPC_DOUBLE_INT, op)        \
		ADD_FUNC_HANDLER(func, MPC_LONG_DOUBLE_INT, op)   \
		ADD_FUNC_HANDLER(func, MPC_SHORT_INT, op)         \
		ADD_FUNC_HANDLER(func, MPC_2INT, op)              \
		ADD_FUNC_HANDLER(func, MPC_2FLOAT, op)            \
		ADD_FUNC_HANDLER(func, MPC_2INTEGER, op)          \
		ADD_FUNC_HANDLER(func, MPC_2REAL, op)             \
		ADD_FUNC_HANDLER(func, MPC_2DOUBLE_PRECISION, op) \
		type_unsupported();                               \
	}

sctk_Op_f sctk_get_common_function(mpc_lowcomm_datatype_t datatype, sctk_Op op)
{
	sctk_Op_f func;

	func = op.func;

	/*Internals function */
	COMPAT_DATA_TYPE_START
	COMPAT_DATA_TYPE_MIN_MAX(func, MPC_MAX_func)
	COMPAT_DATA_TYPE_MIN_MAX(func, MPC_MIN_func)
	COMPAT_DATA_TYPE_SUM_PROD(func, MPC_SUM_func)
	COMPAT_DATA_TYPE_SUM_PROD(func, MPC_PROD_func)
	COMPAT_DATA_TYPE_BINARY_OP(func, MPC_BAND_func)
	COMPAT_DATA_TYPE_LOGICAL_OP(func, MPC_LAND_func)
	COMPAT_DATA_TYPE_BINARY_OP(func, MPC_BXOR_func)
	COMPAT_DATA_TYPE_LOGICAL_OP(func, MPC_LXOR_func)
	COMPAT_DATA_TYPE_BINARY_OP(func, MPC_BOR_func)
	COMPAT_DATA_TYPE_LOGICAL_OP(func, MPC_LOR_func)
	COMPAT_DATA_TYPE_MIN_MAX_LOC(func, MPC_MAXLOC_func)
	COMPAT_DATA_TYPE_MIN_MAX_LOC(func, MPC_MINLOC_func)
	COMPAT_DATA_TYPE_END

	return func;
}

#define MPI_SHM_OP_SUM(t)                        \
	for(i = 0; i < size; i++){               \
		for(j = 0; j < count; j++){      \
			res.t[j] += b[i].t[j]; } \
	}

#define MPI_SHM_OP_PROD(t)                       \
	for(i = 0; i < size; i++){               \
		for(j = 0; j < count; j++){      \
			res.t[j] *= b[i].t[j]; } \
	}

static inline void
sctk_mpi_shared_mem_buffer_collect(union shared_mem_buffer *b,
                                   MPI_Datatype type, int count, MPI_Op op,
                                   void *dest, int size)
{
	int i, j;
	union shared_mem_buffer res = { 0 };
	size_t tsize = 0;

	if(type == MPI_INT)
	{
		tsize = sizeof(int);
	}
	else if(type == MPI_FLOAT)
	{
		tsize = sizeof(float);
	}
	else if(type == MPI_CHAR)
	{
		tsize = sizeof(char);
	}
	else if(type == MPI_DOUBLE)
	{
		tsize = sizeof(double);
	}
	else
	{
		mpc_common_debug_fatal("Unsupported data-type");
	}

	switch(op)
	{
		case MPI_SUM:
			if(type == MPI_INT)
			{
				MPI_SHM_OP_SUM(i)
			}
			else if(type == MPI_FLOAT)
			{
				MPI_SHM_OP_SUM(f)
			}
			else if(type == MPI_CHAR)
			{
				MPI_SHM_OP_SUM(c)
			}
			else if(type == MPI_DOUBLE)
			{
				MPI_SHM_OP_SUM(d)
			}
			else
			{
				mpc_common_debug_fatal("Unsupported data-type");
			}
			break;

		case MPI_PROD:
			if(type == MPI_INT)
			{
				MPI_SHM_OP_PROD(i)
			}
			else if(type == MPI_FLOAT)
			{
				MPI_SHM_OP_PROD(f)
			}
			else if(type == MPI_CHAR)
			{
				MPI_SHM_OP_PROD(c)
			}
			else if(type == MPI_DOUBLE)
			{
				MPI_SHM_OP_PROD(d)
			}
			else
			{
				mpc_common_debug_fatal("Unsupported data-type");
			}
			break;
	}

	sctk_mpi_shared_mem_buffer_get(&res, type, count, dest, count * tsize);
}

#define MAX_FOR_RED_STATIC_BUFF    4096
#define MAX_FOR_RED_STATIC_REQ     24

int __INTERNAL__PMPI_Reduce_shm(void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPI_Comm comm)
{
	struct sctk_comm_coll *coll = mpc_communicator_shm_coll_get(comm);

	int rank;

	_mpc_cl_comm_rank(comm, &rank);

	struct shared_mem_reduce *reduce_ctx = sctk_comm_coll_get_red(coll, rank);
	int      res;
	MPI_Aint tsize = 0;

	res = PMPI_Type_extent(datatype, &tsize);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	int i, size;

	size = coll->comm_size;

	/* Handle in-place */
	if(sendbuf == MPI_IN_PLACE)
	{
		/* We will use the local recv data in root
		 * to reduce in tmp_buff */
		sendbuf = recvbuf;
	}

	/* First pay the toll gate */

	if(__do_yield)
	{
		while(reduce_ctx->tollgate[rank] != OPA_load_int(&reduce_ctx->fare) )
		{
			mpc_thread_yield();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}
	else
	{
		while(reduce_ctx->tollgate[rank] != OPA_load_int(&reduce_ctx->fare) )
		{
			sctk_cpu_relax();
			MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
		}
	}

	/* Reverse state so that only a root done can unlock by
	 * also reversing the fare */
	reduce_ctx->tollgate[rank] = !reduce_ctx->tollgate[rank];

	void *data_buff   = sendbuf;
	void *result_buff = recvbuf;
	int   allocated   = 0;

	//mpc_common_debug_error("OP %d T %d CONT %d FROM %p to %p (%p, %p)", op, datatype, count
	//, sendbuf, recvbuf, data_buff, result_buff );

	int will_be_in_shm_buff = sctk_mpi_type_is_shared_mem(datatype, count) &&
	                          (sctk_mpi_op_is_shared_mem(op) );
	int is_contig_type = _mpc_dt_is_contig_mem(datatype);

	/* Do we need to pack ? */
	if(!is_contig_type)
	{
		/* We have a tmp bufer where to reduce */
		data_buff = sctk_malloc(count * tsize);

		assume(data_buff != NULL);

		/* If non-contig, we need to pack to the TMP buffer
		 * where the reduction will be operated */
		int cnt = 0;
		PMPI_Pack(sendbuf, count, datatype, data_buff, tsize * count, &cnt, comm);
		/* We had to allocate the segment */
		allocated = 1;
	}
	else
	{
		if( (rank == root) && (sendbuf != MPI_IN_PLACE) && !will_be_in_shm_buff)
		{
			memcpy(result_buff, data_buff, count * tsize);
		}
	}

	/* Root RDV phase */

	if(root == rank)
	{
		if(__do_yield)
		{
			while(OPA_cas_int(&reduce_ctx->owner, -1, -2) != -1)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_cas_int(&reduce_ctx->owner, -1, -2) != -1)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		/* Set the local infos */

		/* Now put in the CTX where we would like to reduce */
		if(is_contig_type)
		{
			reduce_ctx->target_buff = result_buff;
		}
		else
		{
			reduce_ctx->target_buff = data_buff;
		}

		/* We use this to create a memory fence
		 * protecting target buffer address*/
		sctk_cpu_relax();

		/* Now unleash the others */
		OPA_store_int(&reduce_ctx->owner, rank);
	}
	else
	{
		if(__do_yield)
		{
			while(OPA_load_int(&reduce_ctx->owner) != root)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&reduce_ctx->owner) != root)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
	}

	/* Get variables */
	size_t     reduce_pipelined_tresh = 0;
	size_t     reduce_force_nocommute = 0;
	sctk_op_t *mpi_op = NULL;
	sctk_Op    mpc_op = { 0 };


	/* This is the TMP buffer case fastpath */
	if(will_be_in_shm_buff)
	{
		/* Set my value in the TMB buffer */
		sctk_mpi_shared_mem_buffer_fill(&reduce_ctx->buffer[rank], datatype, count,
		                                data_buff);

		goto SHM_REDUCE_DONE;
	}

	/* If we are here the buffer is probably too large */
	reduce_pipelined_tresh = _mpc_mpi_config()->coll_opts.reduce_pipelined_tresh;
	reduce_force_nocommute = _mpc_mpi_config()->coll_opts.force_nocommute;

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	if(!sctk_op_can_commute(mpi_op, datatype) || reduce_force_nocommute)
	{
		/* Wait for the GO in order */
		if(__do_yield)
		{
			while(OPA_load_int(&reduce_ctx->left_to_push) != (rank + 1) )
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&reduce_ctx->left_to_push) != (rank + 1) )
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		if(rank != root)
		{
			if(mpc_op.u_func != NULL)
			{
				mpc_op.u_func(data_buff, (void *)reduce_ctx->target_buff, &count, &datatype);
			}
			else
			{
				sctk_Op_f func;
				func = sctk_get_common_function(datatype, mpc_op);
				func(data_buff, (void *)reduce_ctx->target_buff, count, datatype);
			}
		}
	}
	else
	{
		if(rank != root)
		{
			if(reduce_pipelined_tresh < (size_t)(count * tsize) )
			{
				int per_lock = count / reduce_ctx->pipelined_blocks;
				int rest     = count % per_lock;

				size_t stripe_offset = tsize * per_lock;

				int rest_done = 0;

				for(i = 0; i < reduce_ctx->pipelined_blocks; i++)
				{
					int target_cell = (rank + i) % reduce_ctx->pipelined_blocks;

					if(rest != 0)
					{
						/* Now process the rest (only once) */
						if( (rank % reduce_ctx->pipelined_blocks) == i)
						{
							mpc_common_spinlock_lock_yield(&reduce_ctx->buff_lock[0]);

							if(rest_done == 0)
							{
								// mpc_common_debug_error("THe rest %d over %d divided by %d", rest, count,
								// SHM_COLL_BUFF_LOCKS);
								void *from = data_buff + reduce_ctx->pipelined_blocks * stripe_offset;
								void *to   = (void *)reduce_ctx->target_buff + reduce_ctx->pipelined_blocks * stripe_offset;


								if(mpc_op.u_func != NULL)
								{
									mpc_op.u_func(from, to, &rest, &datatype);
								}
								else
								{
									sctk_Op_f func;
									func = sctk_get_common_function(datatype, mpc_op);
									func(from, to, rest, datatype);
								}

								rest_done = 1;
							}

							mpc_common_spinlock_unlock(&reduce_ctx->buff_lock[0]);
						}
					}

					/* Now process the core */
					// mpc_common_debug_error("TARG %d R %d i %d/%d SEG %d STR %ld", target_cell,
					// rank, i, reduce_ctx->pipelined_blocks, per_lock, stripe_offset);

					void *from = data_buff + target_cell * stripe_offset;
					void *to   = (void *)reduce_ctx->target_buff + target_cell * stripe_offset;

					mpc_common_spinlock_lock_yield(&reduce_ctx->buff_lock[target_cell]);

					if(mpc_op.u_func != NULL)
					{
						mpc_op.u_func(from, to, &per_lock, &datatype);
					}
					else
					{
						sctk_Op_f func;
						func = sctk_get_common_function(datatype, mpc_op);
						func(from, to, per_lock, datatype);
					}

					mpc_common_spinlock_unlock(&reduce_ctx->buff_lock[target_cell]);
				}
			}
			else
			{
				mpc_common_spinlock_lock_yield(&reduce_ctx->buff_lock[0]);

				if(mpc_op.u_func != NULL)
				{
					mpc_op.u_func(data_buff, (void *)reduce_ctx->target_buff, &count, &datatype);
				}
				else
				{
					sctk_Op_f func;
					func = sctk_get_common_function(datatype, mpc_op);
					func(data_buff, (void *)reduce_ctx->target_buff, count, datatype);
				}

				mpc_common_spinlock_unlock(&reduce_ctx->buff_lock[0]);
			}
		}
	}

SHM_REDUCE_DONE:

	/* I'm done, notify */
	OPA_decr_int(&reduce_ctx->left_to_push);

	/* Do we need to unpack and/or free ? */

	if(rank == root)
	{
		if(__do_yield)
		{
			while(OPA_load_int(&reduce_ctx->left_to_push) != 0)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}
		else
		{
			while(OPA_load_int(&reduce_ctx->left_to_push) != 0)
			{
				sctk_cpu_relax();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		}

		if(will_be_in_shm_buff)
		{
			sctk_mpi_shared_mem_buffer_collect(reduce_ctx->buffer, datatype, count,
			                                   op, result_buff, size);
		}
		else if(!_mpc_dt_is_contig_mem(datatype) )
		{
			/* If non-contig, we need to unpack to the final buffer */
			int cnt = 0;
			PMPI_Unpack( (void *)reduce_ctx->target_buff, tsize * count, &cnt, recvbuf, count,
			             datatype, comm);

			/* We had to allocate the segment */
			sctk_free(data_buff);
		}
		/* Set the counter */
		OPA_store_int(&reduce_ctx->left_to_push, size);

		reduce_ctx->target_buff = NULL;

		/* Now flag slot as free */
		OPA_store_int(&reduce_ctx->owner, -1);

		/* And reverse the fare */
		int current_fare = OPA_load_int(&reduce_ctx->fare);
		OPA_store_int(&reduce_ctx->fare, !current_fare);
	}
	else
	{
		if(allocated)
		{
			sctk_free(data_buff);
		}
	}

	return MPI_SUCCESS;
}

int __INTERNAL__PMPI_Reduce_intra(void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root,
                                  MPI_Comm comm)
{
	int        size = 0;
	int        rank = 0;
	int        res = 0;


	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(size == 1)
	{
		if(sendbuf == MPI_IN_PLACE)
		{
			return MPI_SUCCESS;
		}

		int psize = 0;
		PMPI_Pack_size(count, datatype, comm, &psize);

		void *tmp = sctk_malloc(psize);

		assume(tmp != NULL);

		int pos = 0;
		PMPI_Pack(sendbuf, count, datatype, tmp, psize, &pos, comm);

		pos = 0;
		PMPI_Unpack(tmp, psize, &pos, recvbuf, count, datatype, comm);

		sctk_free(tmp);

		return MPI_SUCCESS;
	}

	return _mpc_mpi_collectives_reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int __INTERNAL__PMPI_Reduce_inter(void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root,
                                  MPI_Comm comm)
{
	int           res = MPI_ERR_INTERN;
	unsigned long size;

	mpc_lowcomm_status_t status;
	int   rank;
	void *tmp_buf;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(root == MPI_PROC_NULL)
	{
		MPI_ERROR_SUCCESS();
	}

	if(root == MPI_ROOT)
	{
		res = PMPI_Recv_internal(recvbuf, count, datatype, 0, MPC_REDUCE_TAG, comm, &status);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		int type_size;
		res = PMPI_Type_size(datatype, &type_size);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		size    = count * type_size;
		tmp_buf = (void *)sctk_malloc(size);
		res     = PMPI_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, mpc_lowcomm_communicator_get_local(comm) );
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		if(rank == 0)
		{
			res = PMPI_Send_internal(tmp_buf, count, datatype, root, MPC_REDUCE_TAG, comm);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}
	}
	return res;
}

int __INTERNAL__PMPI_Allreduce_intra_simple(void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;

	res = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0,
	                  comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Bcast(recvbuf, count, datatype, 0, comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	return res;
}

#define STATIC_ALLRED_BLOB    4096

int __INTERNAL__PMPI_Allreduce_intra_pipeline(void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;

	char  blob[STATIC_ALLRED_BLOB];
	char  blob1[STATIC_ALLRED_BLOB];
	void *tmp_buff  = blob;
	void *tmp_buff1 = blob1;
	int   to_free   = 0;

	sctk_op_t *mpi_op = sctk_convert_to_mpc_op(op);
	sctk_Op    mpc_op = mpi_op->op;


	MPI_Aint dsize;

	res = PMPI_Type_extent(datatype, &dsize);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	int per_block = 1024;

	if(STATIC_ALLRED_BLOB <= (dsize * per_block) )
	{
		tmp_buff  = sctk_malloc(dsize * per_block);
		tmp_buff1 = sctk_malloc(dsize * per_block);
		to_free   = 1;
	}


	int size;

	_mpc_cl_comm_size(comm, &size);
	int rank;

	_mpc_cl_comm_rank(comm, &rank);

	int left_to_send = count;
	int sent         = 0;


	if(sendbuf == MPI_IN_PLACE)
	{
		sendbuf = recvbuf;
	}

	void *sbuff = sendbuf;

	int left  = (rank - 1);
	int right = (rank + 1) % size;

	if( (left < 0)  )
	{
		left = (size - 1);
	}


	/* Create Blocks in the COUNT */
	while(left_to_send)
	{
		int to_send = per_block;

		if(left_to_send < to_send)
		{
			to_send = left_to_send;
		}

		int         i;
		MPI_Request rr;
		MPI_Request lr;

		for(i = 0; i < size; i++)
		{
			if(i == 0)
			{
				/* Is is now time to send local buffer to the right while accumulating it locally */
				PMPI_Isend_internal(sendbuf + dsize * sent, to_send, datatype, right, MPC_ALLREDUCE_TAG, comm, &rr);

				/* Copy local block in recv */
				if( (sbuff != MPI_IN_PLACE) )
				{
					memcpy(recvbuf + dsize * sent, sendbuf + dsize * sent, to_send * dsize);
				}
			}
			else
			{
				PMPI_Wait(&lr, MPI_STATUS_IGNORE);
				/* Our data is already moving around we now forward tmp_buff */

				memcpy(tmp_buff1, tmp_buff, dsize * to_send);

				if(i != (size - 1) )
				{
					PMPI_Isend_internal(tmp_buff1, to_send, datatype, right, MPC_ALLREDUCE_TAG, comm, &rr);
				}
			}

			if(i != (size - 1) )
			{
				PMPI_Irecv_internal(tmp_buff, to_send, datatype, left, MPC_ALLREDUCE_TAG, comm, &lr);
			}

			if(i != 0)
			{
				/* Data is ready now accumulate */
				if(mpc_op.u_func != NULL)
				{
					mpc_op.u_func(tmp_buff1, recvbuf + sent * dsize, &to_send, &datatype);
				}
				else
				{
					sctk_Op_f func;
					func = sctk_get_common_function(datatype, mpc_op);
					func(tmp_buff1, recvbuf + sent * dsize, to_send, datatype);
				}
			}

			if(i != (size - 1) )
			{
				PMPI_Wait(&rr, MPI_STATUS_IGNORE);
			}
		}



		left_to_send -= to_send;
		sent         += to_send;
	}



	if(to_free == 1)
	{
		sctk_free(tmp_buff);
		sctk_free(tmp_buff1);
	}

	return res;
}

int __INTERNAL__PMPI_Allreduce_intra(void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	return _mpc_mpi_collectives_allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

static int __copy_buffer(void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype)
{
	int  res          = MPI_ERR_INTERN;
	bool is_allocated = false;

	if(mpc_lowcomm_datatype_is_common(datatype) )
	{
		MPI_Aint dsize;
		res = PMPI_Type_extent(datatype, &dsize);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
		memcpy(recvbuf, sendbuf, count * dsize);
	}

	_mpc_cl_type_is_allocated(datatype, &is_allocated);
	if(_mpc_dt_is_user_defined(datatype) && (count != 0) && (is_allocated == 1) )
	{
		if(mpc_mpi_cl_type_is_contiguous(datatype) )
		{
			MPI_Aint dsize;
			res = PMPI_Type_extent(datatype, &dsize);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			memcpy(recvbuf, sendbuf, count * dsize);
		}

		MPI_Request request_send;
		MPI_Request request_recv;

		res = PMPI_Isend_internal(sendbuf, count, datatype,
		                          0, MPC_COPY_TAG, MPI_COMM_SELF, &request_send);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Irecv_internal(recvbuf, count, datatype,
		                          0, MPC_COPY_TAG, MPI_COMM_SELF, &request_recv);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Wait(&(request_recv), MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Wait(&(request_send), MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	return res;
}

int __INTERNAL__PMPI_Allreduce_intra_binary_tree(void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int        res = MPI_ERR_INTERN;
	sctk_Op    mpc_op;
	sctk_op_t *mpi_op;
	int        size;
	int        rank;

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(sctk_op_can_commute(mpi_op, datatype) || (size == 1) || (size % 2 != 0) )
	{
		res = __INTERNAL__PMPI_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		void *   tmp_buf;
		MPI_Aint dsize;
		int      allocated       = 0;
		int      is_MPI_IN_PLACE = 0;
		int      step            = 2;

		sctk_Op_f func;
		func = sctk_get_common_function(datatype, mpc_op);

		res = PMPI_Type_extent(datatype, &dsize);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		tmp_buf = sctk_malloc(count * dsize);
		if(sendbuf == MPI_IN_PLACE)
		{
			is_MPI_IN_PLACE = 1;
			sendbuf         = recvbuf;
		}
		allocated = 1;

		if(is_MPI_IN_PLACE == 0)
		{
			res = __copy_buffer(sendbuf, recvbuf, count, datatype);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
		}

		for(step = 1; step < size; step = step * 2)
		{
			if(rank % (2 * step) != 0)
			{
				MPI_Request request_send;

				//fprintf(stderr,"DOWN STEP %d %d Send to %d\n",step,rank,rank-step);

				res = PMPI_Isend_internal(recvbuf, count, datatype,
				                          rank - step, MPC_ALLREDUCE_TAG, comm, &request_send);
				if(res != MPI_SUCCESS)
				{
					return res;
				}

				res = PMPI_Wait(&(request_send), MPI_STATUS_IGNORE);
				if(res != MPI_SUCCESS)
				{
					return res;
				}

				step = step * 2;
				break;
			}
			else
			{
				if(rank + step < size)
				{
					//fprintf(stderr,"DOWN STEP %d %d Recv from %d\n",step,rank,rank+step);
					res = PMPI_Recv_internal(tmp_buf, count, datatype,
					                         rank + step, MPC_ALLREDUCE_TAG, comm, MPI_STATUS_IGNORE);
					if(res != MPI_SUCCESS)
					{
						return res;
					}


					if(mpc_op.u_func != NULL)
					{
						mpc_op.u_func(tmp_buf, recvbuf, &count, &datatype);
					}
					else
					{
						func(tmp_buf, recvbuf, count, datatype);
					}
				}
			}
		}

		step = step / 2;
		//fprintf(stderr,"DONE %d STEP %d\n",rank,step);

		for(; step > 0; step = step / 2)
		{
			if(rank % (2 * step) == 0)
			{
				if(rank + step < size)
				{
					MPI_Request request_send;
					//fprintf(stderr,"UP STEP %d %d Send to %d\n",step,rank,rank+step);
					res = PMPI_Isend_internal(recvbuf, count, datatype,
					                          rank + step, MPC_ALLREDUCE_TAG, comm, &request_send);
					if(res != MPI_SUCCESS)
					{
						return res;
					}

					res = PMPI_Wait(&(request_send), MPI_STATUS_IGNORE);
					if(res != MPI_SUCCESS)
					{
						return res;
					}
				}
			}
			else
			{
				//fprintf(stderr,"UP STEP %d %d Recv from %d\n",step,rank,rank-step);
				res = PMPI_Recv_internal(recvbuf, count, datatype,
				                         rank - step, MPC_ALLREDUCE_TAG, comm, MPI_STATUS_IGNORE);
			}
		}

		if(allocated == 1)
		{
			free(tmp_buf);
		}
	}
	//PMPI_Barrier(comm);
	return res;
}

int __INTERNAL__PMPI_Allreduce_inter(void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int root, rank, res = MPI_ERR_INTERN;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(mpc_lowcomm_communicator_in_master_group(comm) )
	{
		root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
		res  = PMPI_Reduce(sendbuf, recvbuf, count, datatype,
		                   op, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		root = 0;
		res  = PMPI_Reduce(sendbuf, recvbuf, count, datatype,
		                   op, root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}
	else
	{
		root = 0;
		res  = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op,
		                   root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
		res  = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op,
		                   root, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	res = PMPI_Bcast(recvbuf, count, datatype, 0,
	                 mpc_lowcomm_communicator_get_local(comm) );
	return res;
}

int __INTERNAL__PMPI_Reduce_scatter_intra(void *sendbuf, void *recvbuf, int *recvcnts,
                                          MPI_Datatype datatype, MPI_Op op,
                                          MPI_Comm comm)
{
	return _mpc_mpi_collectives_reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
}

int __INTERNAL__PMPI_Reduce_scatter_inter(void *sendbuf, void *recvbuf, int *recvcnts,
                                          MPI_Datatype datatype, MPI_Op op,
                                          MPI_Comm comm)
{
	int         res, i, rank, root = 0, rsize, lsize;
	int         totalcounts;
	MPI_Aint    extent;
	char *      tmpbuf = NULL, *tmpbuf2 = NULL;
	MPI_Request req;
	int *       disps = NULL;
	sctk_Op     mpc_op;
	sctk_op_t * mpi_op;
	sctk_Op_f   func;

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &lsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &rsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	for(totalcounts = 0, i = 0; i < lsize; i++)
	{
		totalcounts += recvcnts[i];
	}

	if(rank == root)
	{
		res = PMPI_Type_extent(datatype, &extent);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		disps = (int *)sctk_malloc(sizeof(int) * lsize);
		if(NULL == disps)
		{
			return MPI_ERR_INTERN;
		}
		disps[0] = 0;
		for(i = 0; i < (lsize - 1); ++i)
		{
			disps[i + 1] = disps[i] + recvcnts[i];
		}

		tmpbuf  = (char *)sctk_malloc(totalcounts * extent);
		tmpbuf2 = (char *)sctk_malloc(totalcounts * extent);
		if(NULL == tmpbuf || NULL == tmpbuf2)
		{
			return MPI_ERR_INTERN;
		}

		res = PMPI_Isend_internal(sendbuf, totalcounts, datatype, 0,
		                          MPC_REDUCE_SCATTER_TAG, comm, &req);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Recv_internal(tmpbuf2, totalcounts, datatype, 0,
		                         MPC_REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Wait(&req, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		for(i = 1; i < rsize; i++)
		{
			res = PMPI_Recv_internal(tmpbuf, totalcounts, datatype,
			                         i, MPC_REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE);
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			if(mpc_op.u_func != NULL)
			{
				mpc_op.u_func(tmpbuf, tmpbuf2, &totalcounts, &datatype);
			}
			else
			{
				func = sctk_get_common_function(datatype, mpc_op);
				func(tmpbuf, tmpbuf2, totalcounts, datatype);
			}
		}
	}
	else
	{
		res = PMPI_Send_internal(sendbuf, totalcounts, datatype, root,
		                         MPC_REDUCE_SCATTER_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	res = PMPI_Scatterv(tmpbuf2, recvcnts, disps, datatype,
	                    recvbuf, recvcnts[rank], datatype, 0, mpc_lowcomm_communicator_get_local(comm) );

	return res;
}

int __INTERNAL__PMPI_Reduce_scatter_block_intra(void *sendbuf, void *recvbuf, int recvcnt,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPI_Comm comm)
{
	return _mpc_mpi_collectives_reduce_scatter_block(sendbuf, recvbuf, recvcnt, datatype, op, comm);
}

int __INTERNAL__PMPI_Reduce_scatter_block_inter(void *sendbuf, void *recvbuf, int recvcnt,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPI_Comm comm)
{
	int         res = MPI_ERR_INTERN, i, rank, root = 0, rsize, lsize;
	int         totalcounts;
	MPI_Aint    extent;
	char *      tmpbuf = NULL, *tmpbuf2 = NULL;
	MPI_Request req;
	sctk_Op     mpc_op;
	sctk_op_t * mpi_op;
	sctk_Op_f   func;

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &lsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = PMPI_Comm_remote_size(comm, &rsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	totalcounts = lsize * recvcnt;

	if(rank == root)
	{
		res = PMPI_Type_extent(datatype, &extent);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		tmpbuf  = (char *)sctk_malloc(totalcounts * extent);
		tmpbuf2 = (char *)sctk_malloc(totalcounts * extent);
		if(NULL == tmpbuf || NULL == tmpbuf2)
		{
			return MPI_ERR_INTERN;
		}

		res = PMPI_Isend_internal(sendbuf, totalcounts, datatype, 0,
		                          MPC_REDUCE_SCATTER_BLOCK_TAG, comm, &req);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Recv_internal(tmpbuf2, totalcounts, datatype, 0,
		                         MPC_REDUCE_SCATTER_BLOCK_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		res = PMPI_Wait(&req, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		for(i = 1; i < rsize; i++)
		{
			res = PMPI_Recv_internal(tmpbuf, totalcounts, datatype, i,
			                         MPC_REDUCE_SCATTER_BLOCK_TAG, comm, MPI_STATUS_IGNORE);
			if(res != MPI_SUCCESS)
			{
				return res;
			}

			if(mpc_op.u_func != NULL)
			{
				mpc_op.u_func(tmpbuf, tmpbuf2, &totalcounts, &datatype);
			}
			else
			{
				func = sctk_get_common_function(datatype, mpc_op);
				func(tmpbuf, tmpbuf2, totalcounts, datatype);
			}
		}
	}
	else
	{
		res = PMPI_Send_internal(sendbuf, totalcounts, datatype, root,
		                         MPC_REDUCE_SCATTER_BLOCK_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	res = PMPI_Scatter(tmpbuf2, recvcnt, datatype, recvbuf, recvcnt,
	                   datatype, 0, mpc_lowcomm_communicator_get_local(comm) );

	return res;
}

int __INTERNAL__PMPI_Scan_intra(void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	sctk_Op     mpc_op;
	sctk_op_t * mpi_op;
	int         size, dsize;
	int         rank;
	MPI_Request request;
	char *      tmp = NULL;

	int res;

	if(sendbuf == MPI_IN_PLACE)
	{
		sendbuf = recvbuf;
	}

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Isend_internal(sendbuf, count, datatype, rank, MPC_SCAN_TAG, comm, &request);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Recv_internal(recvbuf, count, datatype, rank, MPC_SCAN_TAG, comm, MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS)
	{
		return res;
	}


	res = PMPI_Barrier(comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(rank != 0)
	{
		res = PMPI_Type_size(datatype, &dsize);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		tmp = sctk_malloc(dsize * count);
		res = PMPI_Recv_internal(tmp, count, datatype, rank - 1, MPC_SCAN_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		if(mpc_op.u_func != NULL)
		{
			mpc_op.u_func(tmp, recvbuf, &count, &datatype);
		}
		else
		{
			sctk_Op_f func;
			func = sctk_get_common_function(datatype, mpc_op);
			func(tmp, recvbuf, count, datatype);
		}
	}

	if(rank + 1 < size)
	{
		res = PMPI_Send_internal(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	sctk_free(tmp);
	res = PMPI_Wait(&(request), MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Barrier(comm);
	return res;
}

int __INTERNAL__PMPI_Exscan_intra(void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	sctk_Op     mpc_op;
	sctk_op_t * mpi_op;
	int         size;
	int         rank;
	MPI_Request request;
	MPI_Aint    dsize;
	void *      tmp;
	int         res = MPI_ERR_INTERN;

	if(sendbuf == MPI_IN_PLACE)
	{
		sendbuf = recvbuf;
	}

	mpi_op = sctk_convert_to_mpc_op(op);
	mpc_op = mpi_op->op;

	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Isend_internal(sendbuf, count, datatype, rank,
	                          MPC_EXSCAN_TAG, comm, &request);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Type_extent(datatype, &dsize);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	tmp = sctk_malloc(dsize * count);

	res = PMPI_Recv_internal(tmp, count, datatype, rank,
	                         MPC_EXSCAN_TAG, comm, MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Barrier(comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(rank != 0)
	{
		res = PMPI_Recv_internal(recvbuf, count, datatype, rank - 1,
		                         MPC_EXSCAN_TAG, comm, MPI_STATUS_IGNORE);
		if(res != MPI_SUCCESS)
		{
			return res;
		}

		if(mpc_op.u_func != NULL)
		{
			mpc_op.u_func(recvbuf, tmp, &count, &datatype);
		}
		else
		{
			sctk_Op_f func;
			func = sctk_get_common_function(datatype, mpc_op);
			func(recvbuf, tmp, count, datatype);
		}
	}

	if(rank + 1 < size)
	{
		res = PMPI_Send_internal(tmp, count, datatype, rank + 1,
		                         MPC_EXSCAN_TAG, comm);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	sctk_free(tmp);
	res = PMPI_Wait(&(request), MPI_STATUS_IGNORE);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	res = PMPI_Barrier(comm);
	return res;
}

static int SCTK__MPI_Comm_communicator_dup(MPI_Comm comm, MPI_Comm newcomm);
static int SCTK__MPI_Comm_communicator_free(MPI_Comm comm);



/************************************************************************/
/* Attribute Handling                                                   */
/************************************************************************/

static int   MPI_TAG_UB_VALUE = 512 * 1024 * 1024;
static char *MPI_HOST_VALUE[4096];
static int   MPI_IO_VALUE = 0;
static int   MPI_WTIME_IS_GLOBAL_VALUE = 0;
static int   MPI_APPNUM_VALUE;
static int   MPI_UNIVERSE_SIZE_VALUE;
static int   MPI_LASTUSEDCODE_VALUE;

typedef int (MPI_Copy_function_fortran) (MPI_Comm *, int *, int *, int *, int *, int *, int *);
typedef int (MPI_Delete_function_fortran) (MPI_Comm *, int *, int *, int *, int *);

static void *defines_attr_tab[MPI_MAX_KEY_DEFINED] =
{
	(void *)&MPI_TAG_UB_VALUE /*MPI_TAG_UB */,
	&MPI_HOST_VALUE,
	&MPI_IO_VALUE,
	&MPI_WTIME_IS_GLOBAL_VALUE,
	&MPI_APPNUM_VALUE,
	&MPI_UNIVERSE_SIZE_VALUE,
	&MPI_LASTUSEDCODE_VALUE
};


static int SCTK__MPI_Attr_clean_communicator(MPI_Comm comm)
{
	int                         res = MPI_SUCCESS;
	int                         i;
	mpc_mpi_data_t *            tmp;
	mpc_mpi_per_communicator_t *tmp_per_comm;


	tmp = __get_per_task_data();
	mpc_common_spinlock_lock(&(tmp->lock) );
	tmp_per_comm = __get_per_comm_data(comm);

	if(!tmp_per_comm)
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		return MPI_SUCCESS;
	}

	mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

	for(i = 0; i < tmp_per_comm->max_number; i++)
	{
		if(tmp_per_comm->key_vals[i].flag == 1)
		{
			mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
			mpc_common_spinlock_unlock(&(tmp->lock) );
			res = PMPI_Attr_delete(comm, i + MPI_MAX_KEY_DEFINED);
			mpc_common_spinlock_lock(&(tmp->lock) );
			mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

			if(res != MPI_SUCCESS)
			{
				mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
				mpc_common_spinlock_unlock(&(tmp->lock) );
				return res;
			}
		}
	}

	mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
	mpc_common_spinlock_unlock(&(tmp->lock) );
	return res;
}

static int SCTK__MPI_Attr_communicator_dup(MPI_Comm prev, MPI_Comm newcomm)
{
	int                         res = MPI_SUCCESS;
	mpc_mpi_data_t *            tmp;
	mpc_mpi_per_communicator_t *tmp_per_comm_old;
	mpc_mpi_per_communicator_t *tmp_per_comm_new;
	int                         i;

	tmp = __get_per_task_data();
	mpc_common_spinlock_lock(&(tmp->lock) );
	tmp_per_comm_old = __get_per_comm_data(prev);
	mpc_common_spinlock_lock(&(tmp_per_comm_old->lock) );

	tmp_per_comm_new             = __get_per_comm_data(newcomm);
	tmp_per_comm_new->key_vals   = sctk_malloc(tmp_per_comm_old->max_number * sizeof(MPI_Caching_key_value_t) );
	tmp_per_comm_new->max_number = tmp_per_comm_old->max_number;

	for(i = 0; i < tmp_per_comm_old->max_number; i++)
	{
		tmp_per_comm_new->key_vals[i].flag = 0;
		tmp_per_comm_new->key_vals[i].attr = NULL;
	}

	for(i = 0; i < tmp_per_comm_old->max_number; i++)
	{
		if(tmp->attrs_fn[i].copy_fn != NULL)
		{
			if(tmp_per_comm_old->key_vals[i].flag == 1)
			{
				void *arg = NULL;
				int   flag = 0;
				void *cpy = tmp->attrs_fn[i].copy_fn;

				if(tmp->attrs_fn[i].fortran_key == 0)
				{
					MPI_Copy_function * copy_fn = (MPI_Copy_function*)cpy;
					res =
						copy_fn(prev, i + MPI_MAX_KEY_DEFINED,
						    tmp->attrs_fn[i].extra_state,
						    tmp_per_comm_old->key_vals[i].attr, (void *)(&arg), &flag);
				}
				else
				{
					int  fort_key = i + MPI_MAX_KEY_DEFINED;
					int *ext = (int *)(tmp->attrs_fn[i].extra_state);;
					int  val_out = 0;
					long long_val = (long)(tmp_per_comm_old->key_vals[i].attr);
					int  val =(int)long_val;

					mpc_common_nodebug("%d val", val);
					( (MPI_Copy_function_fortran *)cpy)(&prev, &fort_key, ext, &val, &val_out, &flag, &res);
					mpc_common_nodebug("%d val_out", val_out);
					arg = &val_out;
				}
				mpc_common_nodebug("i = %d Copy %d %ld->%ld flag %d", i, i + MPI_MAX_KEY_DEFINED,
				                   (unsigned long)tmp_per_comm_old->key_vals[i].attr,
				                   (unsigned long)arg, flag);
				if(flag)
				{
					mpc_common_spinlock_unlock(&(tmp_per_comm_old->lock) );
					mpc_common_spinlock_unlock(&(tmp->lock) );
					mpc_common_nodebug("arg = %d", *( ( (int *)arg) ) );
					PMPI_Attr_put(newcomm, i + MPI_MAX_KEY_DEFINED, arg);
					mpc_common_spinlock_lock(&(tmp->lock) );
					mpc_common_spinlock_lock(&(tmp_per_comm_old->lock) );
				}
				if(res != MPI_SUCCESS)
				{
					mpc_common_spinlock_unlock(&(tmp_per_comm_old->lock) );
					mpc_common_spinlock_unlock(&(tmp->lock) );
					return res;
				}
			}
		}
	}
	mpc_common_spinlock_unlock(&(tmp_per_comm_old->lock) );
	mpc_common_spinlock_unlock(&(tmp->lock) );
	return res;
}

#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
static void __sctk_init_mpi_topo()
{
  int cpu_this = mpc_topology_get_global_current_cpu();

  /* send to root cpu id and node rank */
  int rank, size;
  _mpc_cl_comm_rank(MPI_COMM_WORLD, &rank);
  _mpc_cl_comm_size(MPI_COMM_WORLD, &size);

  int *tab_cpuid = ( int* )sctk_malloc(2 * size * sizeof(int));
  tab_cpuid[rank * 2] = cpu_this;
  tab_cpuid[rank * 2 + 1] = mpc_common_get_node_rank();
  mpc_lowcomm_allgather(MPI_IN_PLACE, tab_cpuid, 2*sizeof(int), MPC_COMM_WORLD);

  mpc_topology_init_distance_simulation_factors(tab_cpuid, size);

  sctk_free(tab_cpuid);
}
#endif

static int SCTK__MPI_Comm_communicator_dup(MPI_Comm comm, MPI_Comm newcomm)
{
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo_old;
	mpi_topology_per_comm_t *   topo_new;

	tmp      = __get_per_comm_data(comm);
	topo_old = &(tmp->topo);
	tmp      = __get_per_comm_data(newcomm);
	topo_new = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo_old->lock) );

	if(topo_old->type == MPI_CART)
	{
		topo_new->type              = MPI_CART;
		topo_new->data.cart.ndims   = topo_old->data.cart.ndims;
		topo_new->data.cart.reorder = topo_old->data.cart.reorder;
		topo_new->data.cart.dims    = sctk_malloc(topo_old->data.cart.ndims * sizeof(int) );
		memcpy(topo_new->data.cart.dims, topo_old->data.cart.dims, topo_old->data.cart.ndims * sizeof(int) );
		topo_new->data.cart.periods = sctk_malloc(topo_old->data.cart.ndims * sizeof(int) );
		memcpy(topo_new->data.cart.periods, topo_old->data.cart.periods, topo_old->data.cart.ndims * sizeof(int) );
	}

	if(topo_old->type == MPI_GRAPH)
	{
		topo_new->type = MPI_GRAPH;

		topo_new->data.graph.nnodes  = topo_old->data.graph.nnodes;
		topo_new->data.graph.reorder = topo_old->data.graph.reorder;
		topo_new->data.graph.index   = sctk_malloc(topo_old->data.graph.nindex * sizeof(int) );
		memcpy(topo_new->data.graph.index, topo_old->data.graph.index, topo_old->data.graph.nindex * sizeof(int) );
		topo_new->data.graph.edges = sctk_malloc(topo_old->data.graph.nedges * sizeof(int) );
		memcpy(topo_new->data.graph.edges, topo_old->data.graph.edges, topo_old->data.graph.nedges * sizeof(int) );

		topo_new->data.graph.nedges = topo_old->data.graph.nedges;
		topo_new->data.graph.nindex = topo_old->data.graph.nindex;
	}

	mpc_common_spinlock_unlock(&(topo_old->lock) );

	return MPI_SUCCESS;
}

static int SCTK__MPI_Comm_communicator_free(MPI_Comm comm)
{
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp = __get_per_comm_data(comm);

	if(!tmp)
	{
		return MPI_SUCCESS;
	}

	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type == MPI_CART)
	{
		sctk_free(topo->data.cart.dims);
		sctk_free(topo->data.cart.periods);
	}

	if(topo->type == MPI_GRAPH)
	{
		sctk_free(topo->data.graph.index);
		sctk_free(topo->data.graph.edges);
	}

	topo->type = MPI_UNDEFINED;

	mpc_common_spinlock_unlock(&(topo->lock) );

	TODO("There is a problem with this free this should be refactored to lowcomm BTW");
	//sctk_free(tmp->key_vals);
	sctk_free(tmp);
	return MPI_SUCCESS;
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
static int assignnodes(int ndim, int nfactor, int *pfacts, int *counts, int **pdims)
{
	int *bins;
	int  i, j;
	int  n;
	int  f;
	int *p;
	int *pmin;

	if(0 >= ndim)
	{
		return MPI_ERR_DIMS;
	}

	/* Allocate and initialize the bins */
	bins = (int *)sctk_malloc( (unsigned)ndim * sizeof(int) );
	if(NULL == bins)
	{
		return MPI_ERR_INTERN;
	}
	*pdims = bins;

	for(i = 0, p = bins; i < ndim; ++i, ++p)
	{
		*p = 1;
	}

	/* Loop assigning factors from the highest to the lowest */
	for(j = nfactor - 1; j >= 0; --j)
	{
		f = pfacts[j];
		for(n = counts[j]; n > 0; --n)
		{
			/* Assign a factor to the smallest bin */
			pmin = bins;
			for(i = 1, p = pmin + 1; i < ndim; ++i, ++p)
			{
				if(*p < *pmin)
				{
					pmin = p;
				}
			}
			*pmin *= f;
		}
	}

	/* Sort dimensions in decreasing order (O(n^2) for now) */
	for(i = 0, pmin = bins; i < ndim - 1; ++i, ++pmin)
	{
		for(j = i + 1, p = pmin + 1; j < ndim; ++j, ++p)
		{
			if(*p > *pmin)
			{
				n     = *p;
				*p    = *pmin;
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
static int getfactors(int num, int nprime, int *primes, int **pcounts)
{
	int *counts;
	int  i;
	int *p;
	int *c;

	if(0 >= nprime)
	{
		return MPI_ERR_INTERN;
	}

	/* Allocate the factor counts array */
	counts = (int *)sctk_malloc( (unsigned)nprime * sizeof(int) );
	if(NULL == counts)
	{
		return MPI_ERR_INTERN;
	}

	*pcounts = counts;

	/* Loop over the prime numbers */
	i = nprime - 1;
	p = primes + i;
	c = counts + i;

	for(; i >= 0; --i, --p, --c)
	{
		*c = 0;
		while( (num % *p) == 0)
		{
			++(*c);
			num /= *p;
		}
	}

	if(1 != num)
	{
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
static int getprimes(int num, int *pnprime, int **pprimes)
{
	int  i, j;
	int  n;
	int  size;
	int *primes;

	/* Allocate the array of primes */
	size   = (num / 2) + 1;
	primes = (int *)sctk_malloc( (unsigned)size * sizeof(int) );
	if(NULL == primes)
	{
		return MPI_ERR_INTERN;
	}
	*pprimes = primes;

	/* Find the prime numbers */
	i           = 0;
	primes[i++] = 2;

	for(n = 3; n <= num; n += 2)
	{
		for(j = 1; j < i; ++j)
		{
			if( (n % primes[j]) == 0)
			{
				break;
			}
		}

		if(j == i)
		{
			if(i >= size)
			{
				return MPI_ERR_DIMS;
			}
			primes[i++] = n;
		}
	}

	*pnprime = i;
	return MPI_SUCCESS;
}

static inline int __PMPI_Cart_rank_locked(MPI_Comm comm, const int *coords, int *rank)
{
	int loc_rank  = 0;
	int dims_coef = 1;
	int i;
	mpc_mpi_per_communicator_t *tmp  = __get_per_comm_data(comm);
	mpi_topology_per_comm_t *   topo = &(tmp->topo);

	if(topo->type != MPI_CART)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	for(i = topo->data.cart.ndims - 1; i >= 0; i--)
	{
		loc_rank  += coords[i] * dims_coef;
		dims_coef *= topo->data.cart.dims[i];
	}

	*rank = loc_rank;
	return MPI_SUCCESS;
}

static int __PMPI_Cart_coords_locked(MPI_Comm comm, int init_rank, int maxdims,
                                     int *coords)
{
	int i;
	int pi = 1;
	int qi = 1;
	int rank;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	rank = init_rank;


	if(topo->type != MPI_CART)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	if(maxdims != topo->data.cart.ndims)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
	}

	for(i = maxdims - 1; i >= 0; --i)
	{
		pi       *= topo->data.cart.dims[i];
		coords[i] = (rank % pi) / qi;
		qi        = pi;
	}

	return MPI_SUCCESS;
}

void
MPI_Default_error(__UNUSED__ MPI_Comm *comm, __UNUSED__ int *error, __UNUSED__ char *msg, __UNUSED__ char *file,
                  __UNUSED__ int line)
{
}

void
MPI_Abort_error(MPI_Comm *comm, int *error, char *msg, char *file,
                int line)
{
	char str[1024];
	int  i;

	PMPI_Error_string(*error, str, &i);
	if(i != 0)
	{
		mpc_common_debug_error("%s in file %s at line %d %s", str, file, line, msg);
	}
	else
	{
		mpc_common_debug_error("Unknown error");
	}
	/* The lib is not supposed to crash but deliver message */
	PMPI_Abort(*comm, *error);
}

void
MPI_Return_error(__UNUSED__ MPI_Comm *comm, __UNUSED__ int *error, ...)
{
}

/*********************************************
* INTERNAL PROFILER REDUCE WHEN IN MPI MODE *
*********************************************/

#ifdef MPC_Profiler
void mpc_mp_profiler_do_reduce()
{
	int rank = mpc_common_get_task_rank();

	struct sctk_profiler_array *reduce_array = sctk_profiler_get_reduce_array();

	if(!reduce_array && rank == 0)
	{
		reduce_array = sctk_profiler_array_new();
	}

	if(!sctk_internal_profiler_get_tls_array() )
	{
		mpc_common_debug_error("Profiler TLS is not initialized");
		return;
	}

	if(sctk_profiler_internal_enabled() )
	{
		mpc_common_debug_error("This section must be entered with a disabled profiler");
		abort();
	}


	PMPI_Reduce(sctk_internal_profiler_get_tls_array()->sctk_profile_hits,
	            reduce_array->sctk_profile_hits,
	            SCTK_PROFILE_KEY_COUNT,
	            MPI_LONG_LONG_INT,
	            MPI_SUM,
	            0,
	            MPI_COMM_WORLD);

	/* Reduce the run time */
	PMPI_Reduce(&sctk_internal_profiler_get_tls_array()->run_time,
	            &reduce_array->run_time,
	            1,
	            MPI_LONG_LONG_INT,
	            MPI_SUM,
	            0,
	            MPI_COMM_WORLD);

	PMPI_Reduce(sctk_internal_profiler_get_tls_array()->sctk_profile_value,
	            reduce_array->sctk_profile_value,
	            SCTK_PROFILE_KEY_COUNT,
	            MPI_LONG_LONG_INT,
	            MPI_SUM,
	            0,
	            MPI_COMM_WORLD);

	PMPI_Reduce(sctk_internal_profiler_get_tls_array()->sctk_profile_max,
	            reduce_array->sctk_profile_max,
	            SCTK_PROFILE_KEY_COUNT,
	            MPI_LONG_LONG_INT,
	            MPI_MAX,
	            0,
	            MPI_COMM_WORLD);

	PMPI_Reduce(sctk_internal_profiler_get_tls_array()->sctk_profile_min,
	            reduce_array->sctk_profile_min,
	            SCTK_PROFILE_KEY_COUNT,
	            MPI_LONG_LONG_INT,
	            MPI_MIN,
	            0,
	            MPI_COMM_WORLD);
}

#endif



inline void
SCTK__MPI_INIT_REQUEST(MPI_Request *request)
{
	*request = MPI_REQUEST_NULL;
}

/** \brief Swap an ALLOCATED segment in place using zero-copy if possible
 *  \param sendrecv_buf Adress of the pointer to the buffer used to send and receive data
 *  \param remote_rank Rank which is part of the exchange
 *  \param size Total size of the message in bytes
 *  \param comm Target communicator
 */
int PMPIX_Swap(void **sendrecv_buf, int remote_rank, MPI_Count size, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(remote_rank == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	mpi_check_comm(comm);

	/* First resolve the source and dest rank
	 * in the comm_world */
	int remote = mpc_lowcomm_communicator_world_rank_of( (const mpc_lowcomm_communicator_t)comm, remote_rank);

	/* Now check if we are on the same node for both communications */
	if(!mpc_lowcomm_is_remote_rank(remote) )
	{
		/* Perform the zero-copy message */

		void *tmp_ptr = NULL;

		/* Exchange pointers */
		res = PMPI_Sendrecv_internal(sendrecv_buf, sizeof(void *), MPI_BYTE, remote_rank, 58740,
		                             &tmp_ptr, sizeof(void *), MPI_BYTE, remote_rank, 58740,
		                             comm, MPI_STATUS_IGNORE);

		if(res != MPI_SUCCESS)
		{
			return res;
		}

		mpc_common_debug("SWAPPING ");

		/* Replace by the remote ptr */
		*sendrecv_buf = tmp_ptr;
	}
	else
	{
		/* Allocate a temporary buffer for the copy */
		void *tmp_buff = sctk_malloc(size);

		assume(tmp_buff != NULL);

		/* We have to do a standard sendrecv */
		res = PMPI_Sendrecv_internal(*sendrecv_buf, size, MPI_BYTE, remote_rank, 58740,
		                             tmp_buff, size, MPI_BYTE, remote_rank, 58740,
		                             comm, MPI_STATUS_IGNORE);

		mpc_common_debug("COPYING !");

		if(res != MPI_SUCCESS)
		{
			return res;
		}

		/* Copy from the source buffer to the target one */
		memcpy(*sendrecv_buf, tmp_buff, size);

		sctk_free(tmp_buff);
	}

	return MPI_SUCCESS;
}

/** \brief Swap between two ALLOCATED segment in place using zero-copy if possible
 *  \param send_buf Adress of the pointer to the buffer used to send data
 *  \param recvbuff Adress of the pointer to the buffer used to receive data
 *  \param remote_rank Rank which is part of the exchange
 *  \param size Total size of the message in bytes
 *  \param comm Target communicator
 */
int PMPIX_Exchange(void **send_buf, void **recvbuff, int remote_rank, MPI_Count size, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(remote_rank == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	mpi_check_comm(comm);

	/* First resolve the source and dest rank
	 * in the comm_world */
	int remote = mpc_lowcomm_communicator_world_rank_of( (const mpc_lowcomm_communicator_t)comm, remote_rank);

	/* Now check if we are on the same node for both communications */
	if(!mpc_lowcomm_is_remote_rank(remote) )
	{
		/* Perform the zero-copy message */

		/* Exchange pointers */
		res = PMPI_Sendrecv_internal(send_buf, sizeof(void *), MPI_BYTE, remote_rank, 58740,
		                             recvbuff, sizeof(void *), MPI_BYTE, remote_rank, 58740,
		                             comm, MPI_STATUS_IGNORE);

		if(res != MPI_SUCCESS)
		{
			return res;
		}

		mpc_common_debug("SWAPPING EX!");
	}
	else
	{
		/* Fallback to sendrecv */
		/* We have to do a standard sendrecv */
		res = PMPI_Sendrecv_internal(*send_buf, size, MPI_BYTE, remote_rank, 58740,
		                             *recvbuff, size, MPI_BYTE, remote_rank, 58740,
		                             comm, MPI_STATUS_IGNORE);

		mpc_common_debug("COPYING EX!");

		if(res != MPI_SUCCESS)
		{
			return res;
		}
	}

	return MPI_SUCCESS;
}

int PMPI_Send_internal(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPI_Comm comm)
{

	_mpc_mpi_mpit_event_type_t event_type = MPC_MPI_T_SEND;	
	int is_any = mpc_mpi_mpit_looking_for_event_to_trigger((int)event_type);
	if(is_any)
	{
		void * data;
		MPI_Aint *array_of_displacements;
		MPI_Datatype *array_of_datatypes;
		mpc_mpi_mpit_instance_get_ptr(&data, &array_of_datatypes, &array_of_displacements, (int)event_type);
		if(data != NULL && array_of_datatypes != NULL && array_of_displacements != NULL)
		{
			memcpy(data, &dest, sizeof(array_of_datatypes[0]));
			memcpy((void *)((char *)data + array_of_displacements[1]), &tag, sizeof(array_of_datatypes[1]));
		}

		mpc_mpi_mpit_trigger_event((int)event_type);
	}
	int res = MPI_ERR_INTERN;

	SCTK_PROFIL_START(MPI_Send);

	if(dest == MPC_PROC_NULL)
	{
		return MPI_SUCCESS;
	}

	mpi_check_comm(comm);
	int size;

	_mpc_cl_comm_size(comm, &size);
	mpi_check_rank_send(dest, size, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_count(count, comm);

	if(count != 0)
	{
		mpi_check_buf(buf, comm);
	}

        res = _mpc_cl_send(buf, count, datatype, dest, tag, comm);
        MPI_HANDLE_ERROR(res, comm, "Low level contiguous send failed");
        return res;

	if(count > 1)
	{
		mpc_common_nodebug("count > 1");
		MPI_Datatype new_datatype;
		res = PMPI_Type_contiguous(count, datatype, &new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Could not create contiguous datatype");

		res = PMPI_Type_commit(&new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Failed committing datatype");

		res = PMPI_Send_internal(buf, 1, new_datatype, dest, tag, comm);
		MPI_HANDLE_ERROR(res, comm, "Failed sending with contiguous datatype");

		res = PMPI_Type_free(&new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Failed freeing temp contiguous datatype");
	}

	mpc_lowcomm_request_t request;
	mpc_lowcomm_status_t  status;

	int derived_ret = 0;
	_mpc_lowcomm_general_datatype_t derived_datatype;

	res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &derived_datatype);
	MPI_HANDLE_ERROR(res, comm, "Could not retrieve datatype info");

	res = mpc_mpi_cl_open_pack(&request);
	MPI_HANDLE_ERROR(res, comm, "Failed opening MPI pack");

	res = mpc_mpi_cl_add_pack_absolute(buf, derived_datatype.count, derived_datatype.begins, derived_datatype.ends, MPC_LOWCOMM_CHAR, &request);
	MPI_HANDLE_ERROR(res, comm, "Failed adding to pack");

	res = mpc_mpi_cl_isend_pack(dest, tag, comm, &request);
	MPI_HANDLE_ERROR(res, comm, "Failed sending packed data");

	res = mpc_lowcomm_wait(&request, &status);
	MPI_HANDLE_ERROR(res, comm, "Failed waiting for requests");


	SCTK_PROFIL_END(MPI_Send);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)
{
	/* Internally MPC can use negative tags */
	mpi_check_tag_send(tag, comm);
	return PMPI_Send_internal(buf, count, datatype, dest, tag, comm);
}

int PMPI_Recv_internal(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, MPI_Status *status)
{

        mpc_mpi_mpit_looking_for_event_to_trigger(0);
        //__mpit.callbacks[1]->cb(__mpit.callbacks[1]->event_instance, __mpit.callbacks[1]->event_registration, __mpit.callbacks[1]->cb_safety, __mpit.callbacks[1]->user_data);
	_mpc_mpi_mpit_event_type_t event_type = MPC_MPI_T_RECV;	
	int is_any = mpc_mpi_mpit_looking_for_event_to_trigger((int)event_type);
	if(is_any)
	{
		void * data;
		MPI_Aint *array_of_displacements;
		MPI_Datatype *array_of_datatypes;
		mpc_mpi_mpit_instance_get_ptr(&data, &array_of_datatypes, &array_of_displacements, (int)event_type);
		if(data != NULL && array_of_datatypes != NULL && array_of_displacements != NULL)
		{
			memcpy(data, &source, sizeof(array_of_datatypes[0]));
			memcpy((void *)((char *)data + array_of_displacements[1]), &tag, sizeof(array_of_datatypes[1]));
		}

		mpc_mpi_mpit_trigger_event((int)event_type);
	}
	SCTK_PROFIL_START(MPI_Recv);

	int res = MPI_ERR_INTERN;

	if(source == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_Status empty_status;
		empty_status.MPC_SOURCE = MPI_PROC_NULL;
		empty_status.MPC_TAG    = MPI_ANY_TAG;
		empty_status.MPC_ERROR  = MPI_SUCCESS;
		empty_status.cancelled  = 0;
		empty_status.size       = 0;

		if(status != MPI_STATUS_IGNORE)
		{
			*status = empty_status;
		}

		return res;
	}



	mpi_check_comm(comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_count(count, comm);
	mpc_common_nodebug("tag %d", tag);
	int size;

	_mpc_cl_comm_size(comm, &size);
	mpi_check_rank(source, size, comm);

	if(count != 0)
	{
		mpi_check_buf(buf, comm);
	}



        res = _mpc_cl_recv(buf, count, datatype, source, tag, comm, status);
        MPI_HANDLE_ERROR(res, comm, "Failed low-level contiguous recv");
        return res;


	if(count > 1)
	{
		MPI_Datatype new_datatype;
		res = PMPI_Type_contiguous(count, datatype, &new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Failed creating contiguous datatype");

		res = PMPI_Type_commit(&new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Failed committing contiguous datatype");

		res = PMPI_Recv_internal(buf, 1, new_datatype, source, tag, comm, status);
		MPI_HANDLE_ERROR(res, comm, "Failed recv with contiguous datatype");

		res = PMPI_Type_free(&new_datatype);
		MPI_HANDLE_ERROR(res, comm, "Failed freeing contiguous datatype");

		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	mpc_lowcomm_request_t request;

	memset(&request, 0, sizeof(mpc_lowcomm_request_t) );

	int derived_ret = 0;
	_mpc_lowcomm_general_datatype_t derived_datatype;

	res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret,
	                                            &derived_datatype);
	MPI_HANDLE_ERROR(res, comm, "Failed retrieving datatype info");

	res = mpc_mpi_cl_open_pack(&request);
	MPI_HANDLE_ERROR(res, comm, "Failed opening pack");

	res = mpc_mpi_cl_add_pack_absolute(
		buf, derived_datatype.count,
		derived_datatype.begins, derived_datatype.ends,
		MPC_LOWCOMM_CHAR, &request);
	MPI_HANDLE_ERROR(res, comm, "Failed adding to pack");

	res = mpc_mpi_cl_irecv_pack(source, tag, comm, &request);
	MPI_HANDLE_ERROR(res, comm, "Failed during irecv pack");

	res = mpc_lowcomm_wait(&request, status);
	MPI_HANDLE_ERROR(res, comm, "Failed waiting for requests");

	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = status->MPI_ERROR;
		}
	}

	SCTK_PROFIL_END(MPI_Recv);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status)
{
	mpi_check_tag(tag, comm);
	return PMPI_Recv_internal(buf, count, datatype, source, tag, comm, status);
}

int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
	mpi_check_type(datatype, MPI_COMM_WORLD);
	mpi_check_type_created(datatype, MPI_COMM_WORLD);

	if(status == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_IN_STATUS, "");
	}

	if(count == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_COUNT, "");
	}

	return _mpc_cl_status_get_count(status, datatype, count);
}

int PMPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(dest == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_Request request;

	res = PMPI_Ibsend(buf, count, datatype, dest, tag, comm, &request);
	MPI_HANDLE_ERROR(res, comm, "Failed to proceed to ibsend");

	res = PMPI_Wait(&request, MPI_STATUS_IGNORE);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(dest == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	bool is_allocated = false;

	_mpc_cl_type_is_allocated(datatype, &is_allocated);
	if(is_allocated)
	{
		if(mpc_mpi_cl_type_is_contiguous(datatype) )
		{
			res = _mpc_cl_ssend(buf, count, datatype, dest, tag, comm);
		}
		else
		{
			res = PMPI_Send_internal(buf, count, datatype, dest, tag, comm);
		}
	}
	else
	{
		res = MPI_ERR_TYPE;
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(dest == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}
	res = PMPI_Send_internal(buf, count, datatype, dest, tag, comm);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

/****************************
* BUFFER ATTACH AND DETACH *
****************************/



int PMPI_Buffer_attach(void *buf, int count)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_buffer_t *         tmp;
	mpi_buffer_overhead_t *head;

	PMPC_Get_buffers(&tmp);

	mpc_common_spinlock_lock(&(tmp->lock) );
	if( (tmp->buffer != NULL) || (count < 0) )
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_HANDLE_RETURN_VAL(MPI_ERR_BUFFER, comm);
	}
	tmp->buffer = buf;
	tmp->size   = count;
	mpc_common_nodebug("Buffer size %d", count);
	head = (mpi_buffer_overhead_t *)(tmp->buffer);

	head->size    = tmp->size - sizeof(mpi_buffer_overhead_t);
	head->request = MPI_REQUEST_NULL;
	mpc_common_spinlock_unlock(&(tmp->lock) );

	return MPI_SUCCESS;
}

int PMPI_Buffer_detach(void *buffer, int *size)
{
	mpi_buffer_t *tmp;

	PMPC_Get_buffers(&tmp);
	mpc_common_spinlock_lock(&(tmp->lock) );

	/* Flush pending messages */
	{
		mpi_buffer_overhead_t *head;
		mpi_buffer_overhead_t *head_next;
		int flag;
		int pending;
		do
		{
			pending = 0;
			head    = (mpi_buffer_overhead_t *)(tmp->buffer);

			while(head != NULL)
			{
				if(head->request != MPI_REQUEST_NULL)
				{
					PMPI_Test(&(head->request), &flag,
					          MPI_STATUS_IGNORE);
				}
				if(head->request != MPI_REQUEST_NULL)
				{
					pending++;
				}
				head_next = __buffer_next_header(head, tmp);
				head      = head_next;
			}
			if(pending != 0)
			{
				mpc_thread_yield();
				MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
			}
		} while(pending != 0);
	}

	*( (void **)buffer) = tmp->buffer;
	*size       = tmp->size;
	tmp->size   = 0;
	tmp->buffer = NULL;
	mpc_common_spinlock_unlock(&(tmp->lock) );

	return MPI_SUCCESS;
}

MPI_internal_request_t *mpc_mpi_alloc_request()
{
        return mpc_lowcomm_request_alloc();
}

int PMPI_Isend_internal(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                        MPI_Comm comm, MPI_Request *request)
{
	int res = MPI_ERR_INTERN;

	SCTK__MPI_INIT_REQUEST(request);
	if(dest == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}


	mpi_check_comm(comm);
	int size;
	_mpc_cl_comm_size(comm, &size);
	mpi_check_rank_send(dest, size, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_count(count, comm);
	mpc_common_nodebug("tag %d", tag);

	if(count != 0)
	{
		mpi_check_buf(buf, comm);
	}

        *request = mpc_mpi_alloc_request();

        res = _mpc_cl_isend(buf, count, datatype, dest, tag, comm,
                            _mpc_cl_get_lowcomm_request(*request));

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request)
{
	mpi_check_tag_send(tag, comm);
	return PMPI_Isend_internal(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request)
{
	int res = MPI_ERR_INTERN;

	SCTK__MPI_INIT_REQUEST(request);
	if(dest == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	{
		mpi_check_comm(comm);
		int size = 0;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	return __Ibsend_test_req(buf, count, datatype, dest, tag,
	                         comm, request, 0);
}

int PMPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request)
{
	return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                MPI_Comm comm, MPI_Request *request)
{
	return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Irecv_internal(void *buf, int count, MPI_Datatype datatype, int source,
                        int tag, MPI_Comm comm, MPI_Request *request)
{
	int res = MPI_ERR_INTERN;

	SCTK__MPI_INIT_REQUEST(request);
	if(source == MPC_PROC_NULL)
	{
		res = MPI_SUCCESS;
		MPI_HANDLE_RETURN_VAL(res, comm);
	}

	{
		int size;
		mpi_check_comm(comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank(source, size, comm);


		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

        *request = mpc_mpi_alloc_request();

        res = _mpc_cl_irecv(buf, count, datatype, source, tag, comm,
                            _mpc_cl_get_lowcomm_request(*request));


	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request)
{
	mpi_check_tag(tag, comm);
	return PMPI_Irecv_internal(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Wait(MPI_Request *request, MPI_Status *status)
{
# if MPC_ENABLE_INTEROP_MPI_OMP
    if (mpc_thread_mpi_omp_wait(1, request, NULL, status, MPIX_PARTITION_NONE)) return 0;
# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

	mpc_common_nodebug("entering MPI_Wait request = %d", *request);
	MPI_Comm comm = MPI_COMM_WORLD;

	int res = MPI_ERR_INTERN;

	if(*request == MPI_REQUEST_NULL)
	{
		res = MPI_SUCCESS;
		if(status)
		{
			status->MPC_SOURCE = MPI_PROC_NULL;
			status->MPC_TAG    = MPI_ANY_TAG;
			status->MPC_ERROR  = MPI_SUCCESS;
			status->size       = 0;
		}
                MPI_HANDLE_RETURN_VAL(res, comm);
	}

	//MPI_request_struct_t *  requests;
	MPI_internal_request_t *tmp = *request;

	mpc_lowcomm_status_t *mpc_status = status;
	mpc_lowcomm_status_t  static_mpc_status;
	memset(&static_mpc_status, 0, sizeof(mpc_lowcomm_status_t) );

	if(status == MPI_STATUS_IGNORE)
	{
		mpc_status = &static_mpc_status;
	}

	mpc_common_nodebug("wait request %d", *request);


	if( (tmp != NULL) && (tmp->is_nbc == 1) )
	{
		res = NBC_Wait(&(tmp->nbc_handle), mpc_status);
	}
	else
	{
		mpc_lowcomm_request_t *mpcreq = _mpc_cl_get_lowcomm_request(*request);

		if(mpcreq->request_type == REQUEST_GENERALIZED)
		{
			res = _mpc_cl_waitall(1, mpcreq, mpc_status);
		}
		else
		{
			res = mpc_lowcomm_wait(mpcreq, mpc_status);
		}
	}

	if(tmp != NULL && !tmp->is_persistent)
	{
                PMPI_internal_free_request(&tmp);
	}


	if(mpc_status->MPI_ERROR != MPI_SUCCESS)
	{
		res = mpc_status->MPI_ERROR;
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	/* Initialize status error to success */
	if(status != MPI_STATUS_IGNORE)
	{
		status->MPC_ERROR = MPI_SUCCESS;
	}

	if(*request == MPI_REQUEST_NULL)
	{
		/* Status for NULL requests */
		if(status != MPI_STATUS_IGNORE)
		{
			status->MPC_SOURCE = MPI_PROC_NULL;
			status->MPC_TAG    = MPI_ANY_TAG;
			status->size       = 0;
		}

		res   = MPI_SUCCESS;
		*flag = 1;
		return res;
	}

	MPI_internal_request_t *tmp = *request;

	if( (tmp != NULL) && (tmp->is_nbc == 1) )
	{
		res = NBC_Test(&(tmp->nbc_handle), flag, status);
	}
	else
	{
		res = mpc_lowcomm_test(_mpc_cl_get_lowcomm_request(tmp), flag, status);
	}

	if(*flag)
	{
                PMPI_internal_free_request(&tmp);
	}
	else
	{
		mpc_thread_yield();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}

	if(status != MPI_STATUS_IGNORE)
	{
		if( (status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING) )
		{
			res = status->MPI_ERROR;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Request_free(MPI_Request *request)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_SUCCESS;

	if(NULL == request)
	{
		mpc_common_nodebug("request NULL");
		res = MPI_ERR_REQUEST;
	}
	else if(MPI_REQUEST_NULL == *request)
	{
		mpc_common_nodebug("request MPI_REQUEST_NULL");
		res = MPI_ERR_REQUEST;
	}
	else
	{
		res = MPI_SUCCESS;
                PMPI_internal_free_request(request);
                *request = MPI_REQUEST_NULL;
	}
	MPI_HANDLE_RETURN_VAL(res, comm);
}

static inline int __check_unified_handle_ctx(int count, MPI_Request array_of_requests[])
{

	/* Check for session Coherency */
	mpc_lowcomm_communicator_id_t seen_handle_ctx_id = MPC_LOWCOMM_COMM_NULL_ID;

	mpc_lowcomm_request_t *req;

	int i;

	for(i = 0; i < count; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			continue;
		}

		req = _mpc_cl_get_lowcomm_request(array_of_requests[i]);

		if(req == &MPC_REQUEST_NULL)
		{
			continue;
		}

		void *req_handle_ctx = mpc_lowcomm_communicator_get_context_pointer(req->comm);
		mpc_lowcomm_communicator_id_t req_handle_ctx_id = mpc_lowcomm_communicator_handle_ctx_id(req_handle_ctx);

		if(seen_handle_ctx_id != MPC_LOWCOMM_COMM_NULL_ID)
		{
			if(seen_handle_ctx_id != req_handle_ctx_id)
			{
				MPI_ERROR_REPORT(req->comm, MPI_ERR_REQUEST, "It is not legal to mix requests from multiple sessions in MPI Calls");
			}
		}
		else
		{
			seen_handle_ctx_id = req_handle_ctx_id;
		}
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Waitany(int count,
                 MPI_Request array_of_requests[],
                 int *index,
                 MPI_Status *status)
{
	mpc_common_nodebug("entering PMPI_Waitany");

# if MPC_ENABLE_INTEROP_MPI_OMP
    if (mpc_thread_mpi_omp_wait(count, array_of_requests, index, status, MPIX_PARTITION_NONE)) return 0;
# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_SUCCESS;

	res = __check_unified_handle_ctx(count, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	int flag = 0;

	while(!flag)
	{
		res = PMPI_Testany(count, array_of_requests, index, &flag, status);

		if(res != MPI_SUCCESS)
		{
			break;
		}
	}

	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = status->MPI_ERROR;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Testany(int count,
                 MPI_Request array_of_requests[],
                 int *index,
                 int *flag,
                 MPI_Status *status)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_SUCCESS;

	int i;

	*index = MPI_UNDEFINED;
	*flag  = 1;


	if(status != MPC_LOWCOMM_STATUS_NULL)
	{
		status->MPI_ERROR = MPC_LOWCOMM_SUCCESS;
	}

	if(!array_of_requests)
	{
		return MPI_SUCCESS;
	}


	res = __check_unified_handle_ctx(count, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	for(i = 0; i < count; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			continue;
		}

		{
			mpc_lowcomm_request_t *req;
			req = _mpc_cl_get_lowcomm_request(array_of_requests[i]);
			if(req == &MPC_REQUEST_NULL)
			{
				continue;
			}
			else
			{
				MPI_internal_request_t *reqtmp;
				reqtmp = array_of_requests[i];
				int flag_test = 0;
				if( (reqtmp != NULL) && (reqtmp->is_nbc == 1) )
				{
					res = NBC_Test(&(reqtmp->nbc_handle), &flag_test, status);
				}
				else
				{
					res = mpc_lowcomm_test(req, &flag_test, status);
					if(flag_test == 0)
					{
						*flag = 0;
					}
					else
					{
						*flag = 1;
					}
				}
			}
		}

		if(res != MPI_SUCCESS)
		{
			MPI_HANDLE_RETURN_VAL(res, comm);
		}

		if(*flag)
		{
                        PMPI_internal_free_request(&array_of_requests[i]);
			*index = i;
			break;
		}
	}

	if(status != MPI_STATUS_IGNORE)
	{
		if( (status->MPI_ERROR != MPI_SUCCESS) && (status->MPI_ERROR != MPI_ERR_PENDING) )
		{
			res = status->MPI_ERROR;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

/* Slightly different from PMPI_Testall to wait NBC requests
 * completion inside PMPI_Waitall */

static int __testall_for_NBC(int count, MPI_Request array_of_requests[], int *flag,
                             MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;


	int i;
	int done = 0;
	int loc_flag;

	*flag    = 0;
	if(array_of_statuses != MPC_LOWCOMM_STATUS_NULL)
	{
		for(i = 0; i < count; i++)
		{
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		}
	}

	for(i = 0; i < count; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			done++;
			loc_flag = 0;
			res      = MPI_SUCCESS;
		}
		else
		{
			MPI_internal_request_t *reqtmp = array_of_requests[i];
			mpc_lowcomm_request_t *req = _mpc_cl_get_lowcomm_request(reqtmp);

			if(req == &MPC_REQUEST_NULL)
			{
				done++;
				loc_flag = 0;
				res      = MPI_SUCCESS;
			}
			else
			{
				if( (reqtmp != NULL) && (reqtmp->is_nbc == 1) )
				{
					res = NBC_Test(
						&(reqtmp->nbc_handle), &loc_flag,
						(array_of_statuses == MPI_STATUSES_IGNORE)
						? MPI_STATUS_IGNORE
						: &(array_of_statuses[i]) );
					if(loc_flag)
					{
						array_of_requests[i] = MPI_REQUEST_NULL;
					}
				}
				else
				{
					res = mpc_lowcomm_test(
						req, &loc_flag,
						(array_of_statuses == MPC_LOWCOMM_STATUS_NULL)
						? MPC_LOWCOMM_STATUS_NULL
						: &(array_of_statuses[i]) );
				}
			}
		}
		if(loc_flag)
		{
			done++;
		}
		if(res != MPI_SUCCESS)
		{
			break;
		}
	}

	mpc_common_nodebug("done %d tot %d", done, count);
	*flag = (done == count);
	if(*flag == 0)
	{
		mpc_thread_yield();
	}


	MPI_HANDLE_RETURN_VAL(res, comm);
}

#define PMPI_WAIT_ALL_STATIC_TRSH    32

int PMPI_Waitall(int count, MPI_Request array_of_requests[],
                 MPI_Status array_of_statuses[])
{
	mpc_common_nodebug("entering PMPI_Waitall");

# if MPC_ENABLE_INTEROP_MPI_OMP
    if (mpc_thread_mpi_omp_wait(count, array_of_requests, NULL, array_of_statuses, MPIX_PARTITION_NONE)) return 0;
# endif /* MPC_ENABLE_INTEROP_MPI_OMP */

	MPI_Comm comm = MPI_COMM_WORLD;

	int i;

	/* Set the MPI_Status to MPI_SUCCESS */
	if(array_of_statuses != MPI_STATUSES_IGNORE)
	{
		for(i = 0; i < count; i++)
		{
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		}
	}

	int res = __check_unified_handle_ctx(count, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	/* Convert MPI requests to MPC ones */

	/* Prepare arrays for MPC requests */
	int *array_is_persistent; /*avoid retrieving request at freeing (costly) when it is a persistent*/
	int  static_is_persistent[PMPI_WAIT_ALL_STATIC_TRSH];

	mpc_lowcomm_request_t **mpc_array_of_requests;
	mpc_lowcomm_request_t * static_array_of_requests[PMPI_WAIT_ALL_STATIC_TRSH];

	MPI_Request *mpc_array_of_requests_nbc;
	MPI_Request  static_array_of_requests_nbc[PMPI_WAIT_ALL_STATIC_TRSH];

	if(count < PMPI_WAIT_ALL_STATIC_TRSH)
	{
		array_is_persistent = static_is_persistent;

		mpc_array_of_requests_nbc = static_array_of_requests_nbc;

		mpc_array_of_requests = static_array_of_requests;
	}
	else
	{
		array_is_persistent = sctk_malloc(sizeof(int) * count);
		assume(array_is_persistent != NULL);

		mpc_array_of_requests_nbc = sctk_malloc(sizeof(MPI_Request) * count);
		assume(mpc_array_of_requests_nbc != NULL);

		mpc_array_of_requests = sctk_malloc(sizeof(mpc_lowcomm_request_t *) * count);
		assume(mpc_array_of_requests != NULL);
	}

	MPI_internal_request_t *tmp;

	int has_nbc = 0;

	/* Fill the array with those requests */
	for(i = 0; i < count; i++)
	{
                tmp = array_of_requests[i];
		if(tmp != MPI_REQUEST_NULL && (tmp->is_persistent || tmp->is_intermediate_nbc_persistent) )
		{
			array_is_persistent[i] = 1;
		}
		else
		{
			array_is_persistent[i] = 0;
		}

		if( (tmp != MPI_REQUEST_NULL) && (tmp->is_nbc == 1) )
		{
			has_nbc = 1;
			mpc_array_of_requests[i]     = &MPC_REQUEST_NULL;
			mpc_array_of_requests_nbc[i] = array_of_requests[i];
		}
		else
		{
			mpc_array_of_requests_nbc[i] = MPI_REQUEST_NULL;

			/* Handle NULL requests */
			if(tmp == MPI_REQUEST_NULL)
			{
				mpc_array_of_requests[i] = &MPC_REQUEST_NULL;
			}
			else
			{
				mpc_array_of_requests[i] = _mpc_cl_get_lowcomm_request(tmp);
			}
		}
	}

	if(has_nbc)
	{
		int nbc_flag = 0;

		while(!nbc_flag)
		{
			__testall_for_NBC(count, mpc_array_of_requests_nbc,
			                  &nbc_flag, MPI_STATUSES_IGNORE);
		}
	}


	/* Call the MPC waitall implementation */
	int ret = _mpc_cl_waitallp(count, mpc_array_of_requests,
	                           (mpc_lowcomm_status_t *)array_of_statuses);

	/* Something bad happened? */
	MPI_HANDLE_ERROR(ret, comm, "Error in waitall");

	/* Delete the MPI requests */
	for(i = 0; i < count; i++)
	{
		if(!array_is_persistent[i] && array_of_requests[i] != MPI_REQUEST_NULL)
		{
                        PMPI_internal_free_request(&array_of_requests[i]);
		}
	}

	/* If needed free the mpc_array_of_requests */
	if(PMPI_WAIT_ALL_STATIC_TRSH <= count)
	{
		sctk_free(mpc_array_of_requests);
		sctk_free(mpc_array_of_requests_nbc);
	}

	if(array_of_statuses && (array_of_statuses != MPI_STATUSES_IGNORE) )
	{
		int i;
		for(i = 0; i < count; i++)
		{
			if(array_of_statuses[i].MPI_ERROR != MPI_SUCCESS)
			{
				ret = MPI_ERR_IN_STATUS;
			}
		}
	}

	MPI_HANDLE_RETURN_VAL(ret, comm);
}

int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = __check_unified_handle_ctx(count, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	int i;
	int done = 0;
	int loc_flag;

	*flag    = 0;
	if(array_of_statuses != MPC_LOWCOMM_STATUS_NULL)
	{
		for(i = 0; i < count; i++)
		{
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		}
	}

	for(i = 0; i < count; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			done++;
			loc_flag = 0;
			res      = MPI_SUCCESS;
		}
		else
		{
			MPI_internal_request_t *reqtmp;
                        reqtmp = array_of_requests[i];
			mpc_lowcomm_request_t *req;
                        req = _mpc_cl_get_lowcomm_request(reqtmp);

			if(req == &MPC_REQUEST_NULL)
			{
				done++;
				loc_flag = 0;
				res      = MPI_SUCCESS;
			}
			else
			{
				if( (reqtmp != NULL) && (reqtmp->is_nbc == 1) )
				{
					res = NBC_Test(
						&(reqtmp->nbc_handle), &loc_flag,
						(array_of_statuses == MPI_STATUSES_IGNORE)
						? MPI_STATUS_IGNORE
						: &(array_of_statuses[i]) );
				}
				else
				{
					res = mpc_lowcomm_test(
						req, &loc_flag,
						(array_of_statuses == MPC_LOWCOMM_STATUS_NULL)
						? MPC_LOWCOMM_STATUS_NULL
						: &(array_of_statuses[i]) );
				}
			}
		}
		if(loc_flag)
		{
			done++;
		}
		if(res != MPI_SUCCESS)
		{
			break;
		}
	}

	if(done == count)
	{
		for(i = 0; i < count; i++)
		{
                        PMPI_internal_free_request(&array_of_requests[i]);
		}
	}
	mpc_common_nodebug("done %d tot %d", done, count);
	*flag = (done == count);
	if(*flag == 0)
	{
		mpc_thread_yield();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}

	if(array_of_statuses != MPI_STATUSES_IGNORE)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			if( (array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING) )
			{
				res = MPI_ERR_IN_STATUS;
			}
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Waitsome(int incount, MPI_Request array_of_requests[],
                  int *outcount, int array_of_indices[],
                  MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = __check_unified_handle_ctx(incount, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	int i;
	int req_null_count = 0;


	*outcount = MPI_UNDEFINED;

	for(i = 0; i < incount; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			req_null_count++;
		}
		else
		{
			mpc_lowcomm_request_t *req;
                        req = _mpc_cl_get_lowcomm_request(array_of_requests[i]);
			if(req == &MPC_REQUEST_NULL)
			{
				req_null_count++;
			}
		}
	}

	if(req_null_count == incount)
	{
		return MPI_SUCCESS;
	}

	do
	{
		//      mpc_thread_yield ();
		res = PMPI_Testsome(incount, array_of_requests, outcount,
		                    array_of_indices, array_of_statuses);
	} while( (res == MPI_SUCCESS) &&
	         ( (*outcount == MPI_UNDEFINED) || (*outcount == 0) ) );

	if( (array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED) )
	{
		int i;
		for(i = 0; i < *outcount; i++)
		{
			if( (array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING) )
			{
				res = MPI_ERR_IN_STATUS;
			}
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
	MPI_Comm comm = MPI_COMM_WORLD;

	int res = MPI_SUCCESS;

	if( (array_of_requests == NULL) && (incount != 0) )
	{
		return MPI_ERR_REQUEST;
	}

	if( ( (outcount == NULL || array_of_indices == NULL) && incount > 0) || incount < 0)
	{
		return MPI_ERR_ARG;
	}

	res = __check_unified_handle_ctx(incount, array_of_requests);
	if(res != MPI_SUCCESS)
	{
		/* Note error handler is already called in __check_unified_handle_ctx */
		return res;
	}

	int i;
	int done           = 0;
	int no_active_done = 0;
	int loc_flag;



	for(i = 0; i < incount; i++)
	{
		if(array_of_requests[i] != MPI_REQUEST_NULL)
		{
			int tmp;
			mpc_lowcomm_request_t *req;
                        req = _mpc_cl_get_lowcomm_request(array_of_requests[i]);
			if(req == &MPC_REQUEST_NULL)
			{
				loc_flag = 0;
				tmp      = MPI_SUCCESS;
				no_active_done++;
			}
			else
			{
				MPI_internal_request_t *req_internal;
                                req_internal = array_of_requests[i];
				if( (req_internal != NULL) && (req_internal->is_nbc == 1) )
				{
					tmp = NBC_Test(&(req_internal->nbc_handle), &loc_flag, (array_of_statuses == MPI_STATUSES_IGNORE) ? MPC_LOWCOMM_STATUS_NULL : &(array_of_statuses[done]) );
				}
				else
				{
					tmp = mpc_lowcomm_test(req, &loc_flag, (array_of_statuses == MPI_STATUSES_IGNORE) ? MPC_LOWCOMM_STATUS_NULL : &(array_of_statuses[done]) );
				}
			}
			if(loc_flag)
			{
				array_of_indices[done] = i;
                                PMPI_internal_free_request(&array_of_requests[i]);
				done++;
			}
			MPI_HANDLE_ERROR(tmp, comm, "Error when testing request");
		}
		else
		{
			no_active_done++;
		}
	}

	*outcount = done;

	if(no_active_done == incount)
	{
		*outcount = MPI_UNDEFINED;
		mpc_thread_yield();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}
	else
	{
		mpc_thread_yield();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}

	if( (array_of_statuses != MPI_STATUSES_IGNORE) && (*outcount != MPI_UNDEFINED) )
	{
		int i;
		for(i = 0; i < *outcount; i++)
		{
			if( (array_of_statuses[i].MPI_ERROR != MPI_SUCCESS) && (array_of_statuses[i].MPI_ERROR != MPI_ERR_PENDING) )
			{
				res = MPI_ERR_IN_STATUS;
			}
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status)
{
	int size;
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	mpi_check_tag(tag, comm);
	_mpc_cl_comm_size(comm, &size);

	mpi_check_rank(source, size, comm);


	res = mpc_lowcomm_iprobe(source, tag, comm, flag, status);

	if(!(*flag) )
	{
		mpc_thread_yield();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}


	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	int size;
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	mpi_check_tag(tag, comm);
	_mpc_cl_comm_size(comm, &size);

	mpi_check_rank(source, size, comm);

	res = mpc_lowcomm_probe(source, tag, comm, status);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Cancel(MPI_Request *request)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	if(MPI_REQUEST_NULL == *request)
	{
		mpc_common_nodebug("request MPI_REQUEST_NULL");
		MPI_HANDLE_RETURN_VAL(MPI_ERR_REQUEST, comm);
	}

	MPI_internal_request_t *req = *request;

	if(req->is_active == 1)
	{
                res = mpc_lowcomm_request_cancel(_mpc_cl_get_lowcomm_request(req));
	}
	else
	{
		res = MPI_ERR_REQUEST;
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req;
        mpc_lowcomm_request_t *mpc_req;

        req = *request;
        mpc_req = _mpc_cl_get_lowcomm_request(req);
	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
                mpc_req->request_type = REQUEST_NULL;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = 0;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag         = MPI_ANY_TAG;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_SEND_INIT;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
                mpc_req->request_type = REQUEST_SEND;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = count;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag         = tag;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_SEND_INIT;
	}
	return MPI_SUCCESS;
}

int PMPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype,
                    int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req = *request;
        mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);

	mpc_common_nodebug("new request %d", *request);
	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_NULL;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = 0;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag         = MPI_ANY_TAG;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_BSEND_INIT;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_SEND;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = count;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag         = tag;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_BSEND_INIT;
	}
	return MPI_SUCCESS;
}

int PMPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req = *request;
        mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);

	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_NULL;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = 0;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag         = MPI_ANY_TAG;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_SSEND_INIT;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_SEND;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = count;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag         = tag;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_SSEND_INIT;
	}
	return MPI_SUCCESS;
}

int PMPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                    int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		mpi_check_comm(comm);
		int size;
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank_send(dest, size, comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag_send(tag, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req = *request;
        mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);

	if(dest == MPC_PROC_NULL)
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_NULL;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = 0;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = MPC_PROC_NULL;
		req->persistant.tag         = MPI_ANY_TAG;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_RSEND_INIT;
	}
	else
	{
		req->freeable         = 0;
		req->is_active        = 0;
		mpc_req->request_type = REQUEST_SEND;

		req->persistant.buf         = (void *)buf;
		req->persistant.count       = count;
		req->persistant.datatype    = datatype;
		req->persistant.dest_source = dest;
		req->persistant.tag         = tag;
		req->persistant.comm        = comm;
		req->persistant.op          = MPC_MPI_PERSISTENT_RSEND_INIT;
	}
	return MPI_SUCCESS;
}

int PMPI_Psend_init(const void *buf, int partitions, int count,
                    MPI_Datatype datatype, int dest, int tag,
                    MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
	MPI_internal_request_t *req;
	UNUSED(info);
        UNUSED(req);
        UNUSED(buf);
        UNUSED(partitions);
        UNUSED(count);
        UNUSED(datatype);
        UNUSED(dest);
        UNUSED(tag);
        UNUSED(comm);

        req = *request = mpc_lowcomm_request_alloc();
        not_implemented();

	//return mpi_psend_init(buf, partitions, count, datatype, dest, tag,
	//                      comm, req);

        return 0;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
	{
		int size;
		mpi_check_comm(comm);
		mpi_check_type(datatype, comm);
		mpi_check_type_commited(datatype, comm);
		mpi_check_count(count, comm);
		mpc_common_nodebug("tag %d", tag);
		mpi_check_tag(tag, comm);
		_mpc_cl_comm_size(comm, &size);
		mpi_check_rank(source, size, comm);

		if(count != 0)
		{
			mpi_check_buf(buf, comm);
		}
	}

	MPI_internal_request_t *req;
        mpc_lowcomm_request_t *mpc_req;

	//req                   = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests() );
        req = *request = mpc_lowcomm_request_alloc();
        mpc_req = _mpc_cl_get_lowcomm_request(req);
	req->freeable         = 0;
	req->is_active        = 0;
	mpc_req->request_type = REQUEST_RECV;

	req->persistant.buf         = buf;
	req->persistant.count       = count;
	req->persistant.datatype    = datatype;
	req->persistant.dest_source = source;
	req->persistant.tag         = tag;
	req->persistant.comm        = comm;
	req->persistant.op          = MPC_MPI_PERSISTENT_RECV_INIT;

	return MPI_SUCCESS;
}

int PMPI_Precv_init(void *buf, int partitions, int count,
                    MPI_Datatype datatype, int source, int tag,
                    MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
	MPI_internal_request_t *req;
	UNUSED(info);
        UNUSED(req);
        UNUSED(request);
	UNUSED(info);
        UNUSED(req);
        UNUSED(buf);
        UNUSED(partitions);
        UNUSED(count);
        UNUSED(datatype);
        UNUSED(source);
        UNUSED(tag);
        UNUSED(comm);

        //req = *request = mpc_lowcomm_request_alloc();
	//req = __sctk_new_mpc_request_internal(request,
	//                                      __sctk_internal_get_MPC_requests() );
        not_implemented();
	//mpi_precv_init(buf, partitions, count, datatype, source,
	//                      tag, comm, req);
        return 0;
}

int PMPI_Start(MPI_Request *request)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	if( (request == NULL) || (*request == MPI_REQUEST_NULL) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_REQUEST, "");
	}

	MPI_internal_request_t *req = *request;
        mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
	//MPI_request_struct_t *  requests;

	//requests = __sctk_internal_get_MPC_requests();
	//req      = __sctk_convert_mpc_request_internal(request, requests);
	if(req->is_active != 0)
	{
		return MPI_ERR_REQUEST;
	}
	if(!req->is_persistent)
	{ /* TODO only if req not persistant (has to be changed)*/
		req->is_active = 1;
	}

	if(mpc_req->request_type == REQUEST_NULL)
	{
		req->is_active = 0;
		return MPI_SUCCESS;
	}

	if(req->is_partitioned)
	{
                not_implemented();
		MPI_HANDLE_RETURN_VAL(res, comm);
	}


	NBC_Handle *handle = NULL;

	switch(req->persistant.op)
	{
		case MPC_MPI_PERSISTENT_SEND_INIT:
			res =
				__Isend_test_req(req->persistant.buf,
				                 req->persistant.count,
				                 req->persistant.datatype,
				                 req->persistant.dest_source,
				                 req->persistant.tag,
				                 req->persistant.comm, request, 1);
			break;

		case MPC_MPI_PERSISTENT_BSEND_INIT:
			res =
				__Ibsend_test_req(req->persistant.buf,
				                  req->persistant.count,
				                  req->persistant.datatype,
				                  req->persistant.dest_source,
				                  req->persistant.tag,
				                  req->persistant.comm, request, 1);
			break;

		case MPC_MPI_PERSISTENT_RSEND_INIT:
			res =
				__Isend_test_req(req->persistant.buf,
				                 req->persistant.count,
				                 req->persistant.datatype,
				                 req->persistant.dest_source,
				                 req->persistant.tag,
				                 req->persistant.comm, request, 1);
			break;

		case MPC_MPI_PERSISTENT_SSEND_INIT:
			res =
				__Isend_test_req(req->persistant.buf,
				                 req->persistant.count,
				                 req->persistant.datatype,
				                 req->persistant.dest_source,
				                 req->persistant.tag,
				                 req->persistant.comm, request, 1);
			break;

		case MPC_MPI_PERSISTENT_RECV_INIT:
			res =
				__Irecv_test_req(req->persistant.buf,
				                 req->persistant.count,
				                 req->persistant.datatype,
				                 req->persistant.dest_source,
				                 req->persistant.tag,
				                 req->persistant.comm, request, 1);
			break;

		case MPC_MPI_PERSISTENT_BCAST_INIT:
		case MPC_MPI_PERSISTENT_REDUCE_INIT:
		case MPC_MPI_PERSISTENT_ALLREDUCE_INIT:
		case MPC_MPI_PERSISTENT_SCATTER_INIT:
		case MPC_MPI_PERSISTENT_SCATTERV_INIT:
		case MPC_MPI_PERSISTENT_GATHER_INIT:
		case MPC_MPI_PERSISTENT_GATHERV_INIT:
		case MPC_MPI_PERSISTENT_REDUCE_SCATTER_BLOCK_INIT:
		case MPC_MPI_PERSISTENT_REDUCE_SCATTER_INIT:
		case MPC_MPI_PERSISTENT_ALLGATHER_INIT:
		case MPC_MPI_PERSISTENT_ALLGATHERV_INIT:
		case MPC_MPI_PERSISTENT_ALLTOALL_INIT:
		case MPC_MPI_PERSISTENT_ALLTOALLV_INIT:
		case MPC_MPI_PERSISTENT_ALLTOALLW_INIT:
		case MPC_MPI_PERSISTENT_SCAN_INIT:
		case MPC_MPI_PERSISTENT_EXSCAN_INIT:
		case MPC_MPI_PERSISTENT_BARRIER_INIT:
			handle             = &(req->nbc_handle);
			handle->row_offset = sizeof(int);
			res = NBC_Start(handle, (NBC_Schedule *)handle->schedule);
			break;

		default:
			not_reachable();
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Startall(int count, MPI_Request array_of_requests[])
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;
	int      i;

	for(i = 0; i < count; i++)
	{
		if(array_of_requests[i] == MPI_REQUEST_NULL)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_REQUEST, "");
		}
	}

	for(i = 0; i < count; i++)
	{
		res = PMPI_Start(&(array_of_requests[i]) );
		if(res != MPI_SUCCESS)
		{
			break;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Pready(int partition, MPI_Request request)
{
        UNUSED(partition);
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	if(request == MPI_REQUEST_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_REQUEST, "");
	}

	MPI_internal_request_t *req = request;
        UNUSED(req);

        not_implemented();
	//res = mpi_pready(partition, req);

	MPI_HANDLE_RETURN_VAL(res, comm);

}

int PMPI_Parrived(MPI_Request request, int partition, int *flags)
{
        UNUSED(partition);
        UNUSED(flags);
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	if(request == MPI_REQUEST_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_REQUEST, "");
	}

	MPI_internal_request_t *req = request;
        UNUSED(req);

        not_implemented();
	//res = mpi_parrived(partition, req, flags);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Sendrecv_internal(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           int dest, int sendtag,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                           int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
	int res = MPI_ERR_INTERN;
	int size;

	mpi_check_comm(comm);
	_mpc_cl_comm_size(comm, &size);

	mpi_check_type(sendtype, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(sendtype, comm);
	mpi_check_type_commited(recvtype, comm);
	mpi_check_count(sendcount, comm);
	mpi_check_count(recvcount, comm);
	mpi_check_rank_send(dest, size, comm);
	mpi_check_rank(source, size, comm);

	if(sendcount != 0)
	{
		mpi_check_buf(sendbuf, comm);
	}
	if(recvcount != 0)
	{
		mpi_check_buf(recvbuf, comm);
	}

	MPI_Request s_request;
	MPI_Request r_request;
	MPI_Status  s_status;

	mpc_common_nodebug("__INTERNAL__PMPI_Sendrecv TYPE %d %d", sendtype, recvtype);

	res = PMPI_Isend_internal(sendbuf, sendcount, sendtype, dest, sendtag, comm, &s_request);

	MPI_HANDLE_ERROR(res, comm, "Failed doing Isend");

	res = PMPI_Irecv_internal(recvbuf, recvcount, recvtype, source, recvtag, comm, &r_request);

	MPI_HANDLE_ERROR(res, comm, "Failed doing Irecv");

	res = PMPI_Wait(&s_request, &s_status);

	MPI_HANDLE_ERROR(res, comm, "Failed waiting request");

	res = PMPI_Wait(&r_request, status);

	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = MPI_ERR_IN_STATUS;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
	mpi_check_tag_send(sendtag, comm);
	mpi_check_tag(recvtag, comm);

	return PMPI_Sendrecv_internal(sendbuf, sendcount, sendtype, dest, sendtag,
	                              recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                          int dest, int sendtag, int source, int recvtag,
                          MPI_Comm comm, MPI_Status *status)
{
	int res = MPI_ERR_INTERN;
	int size;

	mpi_check_comm(comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_count(count, comm);
	mpi_check_tag_send(sendtag, comm);
	mpi_check_tag(recvtag, comm);
	_mpc_cl_comm_size(comm, &size);
	mpi_check_rank_send(dest, size, comm);
	mpi_check_rank(source, size, comm);
	if(count != 0)
	{
		mpi_check_buf(buf, comm);
	}

	int         type_size;
	char *      tmp;
	MPI_Request sendreq;
	MPI_Request recvreq;
	int         position = 0;

	mpc_common_nodebug("PMPI_Sendrecv_replace TYPE %d", datatype);

	res = PMPI_Pack_size(count, datatype, comm, &type_size);

	MPI_HANDLE_ERROR(res, comm, "Pack Size failed");

	tmp = sctk_malloc(type_size);


	if(tmp == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_INTERN, "Temporary Buffer alloc Failed");
	}

	res = PMPI_Pack(buf, count, datatype, tmp, type_size, &position, comm);
	if(res != MPI_SUCCESS)
	{
		sctk_free(tmp);
		MPI_ERROR_REPORT(comm, res, "Pack Failed");
	}

	mpc_common_nodebug("position %d, %d", position, type_size);
	mpc_common_nodebug("receive type = %d", datatype);
	res = PMPI_Irecv_internal(buf, count, datatype, source, recvtag, comm,
	                          &recvreq);
	if(res != MPI_SUCCESS)
	{
		sctk_free(tmp);
		MPI_ERROR_REPORT(comm, res, "Irecv Failed");
	}
	mpc_common_nodebug("Send type = %d", MPI_PACKED);
	res = PMPI_Isend_internal(tmp, position, MPI_PACKED, dest, sendtag, comm,
	                          &sendreq);
	if(res != MPI_SUCCESS)
	{
		sctk_free(tmp);
		MPI_ERROR_REPORT(comm, res, "Isend Failed");
	}
	res = PMPI_Wait(&sendreq, status);
	if(res != MPI_SUCCESS)
	{
		sctk_free(tmp);
		MPI_ERROR_REPORT(comm, res, "Wait Failed");
	}
	res = PMPI_Wait(&recvreq, status);
	if(res != MPI_SUCCESS)
	{
		sctk_free(tmp);
		MPI_ERROR_REPORT(comm, res, "Wait Failed");
	}

	sctk_free(tmp);

	if(status != MPI_STATUS_IGNORE)
	{
		if(status->MPI_ERROR != MPI_SUCCESS)
		{
			res = MPI_ERR_IN_STATUS;
		}
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = _mpc_cl_status_set_elements(status, datatype, count);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = _mpc_cl_status_set_elements_x(status, datatype, count);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Status_set_cancelled(MPI_Status *status, int cancelled)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = mpc_lowcomm_status_set_cancelled(status, cancelled);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
	mpc_lowcomm_request_t *mpc_request = _mpc_cl_get_lowcomm_request(request);

	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = _mpc_cl_request_get_status(*mpc_request, flag, status);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Test_cancelled(const MPI_Status *status, int *flag)
{
	mpc_lowcomm_status_get_cancelled(status, flag);
	return MPI_SUCCESS;
}

/************************************************************************/
/* MPI Datatype Interface                                               */
/************************************************************************/

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype *new_type_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	*new_type_p = MPC_DATATYPE_NULL;

	mpi_check_count(count, comm);
	mpi_check_type(old_type, comm);
	mpi_check_type_created(old_type, comm);

	/* Here we set the ctx to NULL in order to create a contiguous type (no overriding) */
	return __PMPI_Type_contiguous_inherits(count, old_type, new_type_p, NULL);
}

int PMPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype old_type, MPI_Datatype *newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type_created(old_type, comm);

	*newtype_p = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);

	if(blocklength < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
	}

	mpi_check_count(count, comm);

	MPI_Aint extent;

	/* Retrieve type extent */
	PMPI_Type_extent(old_type, &extent);

	/* Set its context to overide the one from hvector */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner    = MPI_COMBINER_VECTOR;
	dtctx.count       = count;
	dtctx.blocklength = blocklength;
	dtctx.stride      = stride;
	dtctx.oldtype     = old_type;


	if( (blocklength == stride) && (0 <= stride) )
	{
		int ret = __PMPI_Type_contiguous_inherits(count * blocklength, old_type, newtype_p, &dtctx);
		_mpc_cl_type_ctx_set(*newtype_p, &dtctx);
		return ret;
	}


	/* Compute the stride in bytes */
	unsigned long stride_t = stride * extent;

	/* Call the hvector function */
	res = PMPI_Type_hvector(count, blocklength, stride_t, old_type, newtype_p);

	if(res == MPI_SUCCESS)
	{
		_mpc_cl_type_ctx_set(*newtype_p, &dtctx);
	}

	return res;
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype *newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(old_type, MPI_COMM_WORLD);
	mpi_check_type_created(old_type, comm);
	mpi_check_count(count, comm);

	*newtype_p = MPC_DATATYPE_NULL;


	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
	}

	unsigned long extent;
	unsigned long stride_t;

	stride_t = (unsigned long)stride;

	/* Set its context */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner    = MPI_COMBINER_HVECTOR;
	dtctx.count       = count;
	dtctx.blocklength = blocklen;
	dtctx.stride_addr = stride;
	dtctx.oldtype     = old_type;

	/* Is it a valid data-type ? */
	if(mpc_dt_is_valid(old_type) )
	{
		/* Retrieve the inner structure */
		old_type = _mpc_dt_get_datatype(old_type);

		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		_mpc_cl_general_datatype_try_get_info(old_type, &derived_ret, &input_datatype);

		/* Compute the extent */
		PMPI_Type_extent(old_type, (MPI_Aint *)&extent);

		/*  Handle the contiguous case or Handle count == 0 */
		if( ( ( (blocklen * extent) == (size_t)stride) ) ||
		    (count == 0) )
		{
			int ret = __PMPI_Type_contiguous_inherits(count * blocklen, old_type, newtype_p, &dtctx);
			_mpc_cl_type_ctx_set(*newtype_p, &dtctx);
			return ret;
		}

		mpc_common_nodebug("extend %lu, count %d, blocklen %d, stide %lu", extent, count, blocklen, stride);

		/* The vector is created by repeating the input blocks
		 * count * blocklen times where the actual input type
		 * contains input_datatype.count entries */
		unsigned long count_out = input_datatype.count * count * blocklen;

		long *begins_out = sctk_malloc(count_out * sizeof(long) );
		long *ends_out   = sctk_malloc(count_out * sizeof(long) );
		mpc_lowcomm_datatype_t *datatypes = sctk_malloc(count_out * sizeof(mpc_lowcomm_datatype_t) );

		/* Here we flatten the vector */
		int           i;
		int           j;
		unsigned long k;


		long new_ub = input_datatype.ub;
		long new_lb = input_datatype.lb;
		long next_ub, next_lb, cur_ub, cur_lb;

		next_ub = input_datatype.ub;
		next_lb = input_datatype.lb;


		for(i = 0; i < count; i++)
		{
			/* For block */
			for(j = 0; j < blocklen; j++)
			{
				cur_ub = next_ub;
				cur_lb = next_lb;

				/* For each block in the block length */
				for(k = 0; k < input_datatype.count; k++)
				{
					/* For each input block */
					begins_out[(i * blocklen + j) * input_datatype.count + k] = input_datatype.begins[k] + (stride_t * i) + (j * extent);
					ends_out[(i * blocklen + j) * input_datatype.count + k]   = input_datatype.ends[k] + (stride_t * i) + (j * extent);
					datatypes[(i * blocklen + j) * input_datatype.count + k]  = input_datatype.datatypes[k];
				}
				next_ub = cur_ub + extent;
				next_lb = cur_lb + extent;

				if(cur_ub > new_ub)
				{
					new_ub = cur_ub;
				}
				if(cur_lb < new_lb)
				{
					new_lb = cur_lb;
				}
			}

			next_ub = input_datatype.ub + ( (stride * (i + 1) ) );
			next_lb = input_datatype.lb + ( (stride * (i + 1) ) );
		}

		/* Create the derived datatype */
		_mpc_cl_general_datatype(newtype_p, begins_out, ends_out, datatypes, count_out, new_lb, input_datatype.is_lb, new_ub, input_datatype.is_ub, &dtctx);

		/* Free temporary arrays */
		sctk_free(begins_out);
		sctk_free(ends_out);
		sctk_free(datatypes);

		return MPI_SUCCESS;
	}

	return MPI_ERR_TYPE;
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype *newtype_p)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(old_type, MPI_COMM_WORLD);
	mpi_check_type_created(old_type, comm);
	mpi_check_count(count, comm);

	*newtype_p = MPC_DATATYPE_NULL;


	if(blocklen < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
	}

	res = PMPI_Type_hvector(count, blocklen, stride, old_type, newtype_p);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_indexed(int count, const int blocklens[], const int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;
	int      i;

	mpi_check_count(count, comm);

	*newtype = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);

	if( (blocklens == NULL) || (indices == NULL) )
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
		}
	}

	mpi_check_type_created(old_type, MPI_COMM_SELF);

	MPI_Aint extent;

	/* Retrieve type extent */
	PMPI_Type_extent(old_type, &extent);

	/* Create a temporary offset array */
	MPI_Aint *byte_offsets = sctk_malloc(count * sizeof(MPI_Aint) );

	assume(byte_offsets != NULL);

	/* Fill it with by converting type based indices to bytes */
	for(i = 0; i < count; i++)
	{
		byte_offsets[i] = indices[i] * extent;
	}

	/* Call the byte offseted version of type_indexed */
	res = PMPI_Type_create_hindexed(count, blocklens, byte_offsets, old_type, newtype);

	/* Set its context to overide the one from hdindexed */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner               = MPI_COMBINER_INDEXED;
	dtctx.count                  = count;
	dtctx.array_of_blocklenght   = blocklens;
	dtctx.array_of_displacements = indices;
	dtctx.oldtype                = old_type;
	_mpc_cl_type_ctx_set(*newtype, &dtctx);

	/* Release the temporary byte offset array */
	sctk_free(byte_offsets);

	return res;
}

int PMPI_Type_hindexed(int count, const int blocklens[], const MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;
	int      i;

	mpi_check_count(count, comm);

	*newtype = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);


	if( (blocklens == NULL) || (indices == NULL) )
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
		}
	}

	mpi_check_type_created(old_type, comm);
	res = PMPI_Type_create_hindexed(count, blocklens, indices, old_type, newtype);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_create_indexed_block(int count, int blocklength, const int indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_count(count, comm);

	*newtype = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);


	if(blocklength < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
	}


	mpi_check_type_created(old_type, comm);
	res = __INTERNAL__PMPI_Type_create_indexed_block(count, blocklength, indices, old_type, newtype);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_count(count, comm);


	*newtype = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);

	if(blocklength < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
	}

	mpi_check_type_created(old_type, comm);
	res = __INTERNAL__PMPI_Type_create_hindexed_block(count, blocklength, indices, old_type, newtype);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_create_hindexed(int count, const int blocklens[], const MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	int i = 0;

	mpi_check_count(count, comm);

	*newtype = MPC_DATATYPE_NULL;

	mpi_check_type(old_type, MPI_COMM_WORLD);

	if( (blocklens == NULL) || (indices == NULL) )
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
		}
	}
	mpi_check_type_created(old_type, comm);

	/* Set its context */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner                    = MPI_COMBINER_HINDEXED;
	dtctx.count                       = count;
	dtctx.array_of_blocklenght        = blocklens;
	dtctx.array_of_displacements_addr = indices;
	dtctx.oldtype                     = old_type;

	/* Is it a valid data-type ? */
	if(mpc_dt_is_valid(old_type) )
	{
		int derived_ret = 0;
		/* Retrieve the inner structure */
		if(mpc_lowcomm_datatype_is_common(old_type) )
		{
			old_type = _mpc_dt_get_datatype(old_type);
		}

		_mpc_lowcomm_general_datatype_t input_datatype;

		/* Retrieve input datatype informations */
		_mpc_cl_general_datatype_try_get_info(old_type, &derived_ret, &input_datatype);

		/* Get datatype extent */
		unsigned long extent = 0;
		PMPI_Type_extent(old_type, (MPI_Aint *)&extent);

		/*  Handle the contiguous case or Handle count == 0 */
		if(count == 0)
		{
			int ret = __PMPI_Type_contiguous_inherits(0, old_type, newtype, &dtctx);
			_mpc_cl_type_ctx_set(*newtype, &dtctx);
			return ret;
		}


		/* Compute the total number of blocks */
		unsigned long count_out = 0;
		int           i;
		for(i = 0; i < count; i++)
		{
			count_out += input_datatype.count * blocklens[i];
		}

		/* Allocate context */
		long *begins_out = sctk_malloc(count_out * sizeof(long) );
		long *ends_out   = sctk_malloc(count_out * sizeof(long) );
		mpc_lowcomm_datatype_t *datatypes = sctk_malloc(count_out * sizeof(mpc_lowcomm_datatype_t) );

		mpc_common_nodebug("%lu extent", extent);

		int           j = 0;
		unsigned long k = 0;
		unsigned long step   = 0;
		long int      new_ub = input_datatype.ub;
		long int      new_lb = input_datatype.lb;


		for(i = 0; i < count; i++)
		{
			/* For each output entry */
			for(j = 0; j < blocklens[i]; j++)
			{
				/* For each entry in the block */
				for(k = 0; k < input_datatype.count; k++)
				{
					/* For each block in the input */
					begins_out[step] = input_datatype.begins[k] + indices[i] + j * extent;
					ends_out[step]   = input_datatype.ends[k] + indices[i] + j * extent;
					datatypes[step]  = input_datatype.datatypes[k];
					step++;
				}
			}


			/* Update UB */
			long int candidate_b = 0;

                        candidate_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);

                        if(new_ub < candidate_b)
                        {
                                new_ub = candidate_b;
                        }

                        /* Update LB */
                        candidate_b = input_datatype.lb + indices[i];

                        if(candidate_b < new_lb)
                        {
                                new_lb = candidate_b;
                        }
                }

		/* Create the derived datatype */
		_mpc_cl_general_datatype(newtype, begins_out, ends_out, datatypes, count_out, new_lb, input_datatype.is_lb, new_ub, input_datatype.is_ub, &dtctx);


		/* Free temporary arrays */
		sctk_free(begins_out);
		sctk_free(ends_out);
		sctk_free(datatypes);

		return MPI_SUCCESS;
	}
	return MPI_ERR_TYPE;
}

int PMPI_Type_struct(int count,
                     const int *blocklens,
                     const MPI_Aint *indices,
                     const MPI_Datatype *old_types,
                     MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;
	int      i = 0;

	mpi_check_count(count, comm);

	*newtype = MPC_DATATYPE_NULL;


	if( (old_types == NULL) || (indices == NULL) || (blocklens == NULL) )
	{
		return MPI_SUCCESS;
	}

	for(i = 0; i < count; i++)
	{
		mpi_check_type(old_types[i], MPI_COMM_WORLD);

		mpi_check_type_created(old_types[i], comm);

		if(blocklens[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Error negative block lengths provided");
		}
	}
	unsigned long glob_count_out   = 0;
	long          new_lb           = (long)(-1);
	int           new_is_lb        = 0;
	long          new_ub           = 0;
	long          common_type_size = 0;
	int           new_is_ub        = 0;
	unsigned long my_count_out     = 0;

	if(!count)
	{
		new_lb = 0;
	}

	// find malloc size
	for(i = 0; i < count; i++)
	{
		if( (old_types[i] != MPI_UB) && (old_types[i] != MPI_LB) )
		{
			int derived_ret = 0;
			_mpc_lowcomm_general_datatype_t input_datatype;

			int count_in = 0;

			if(!_mpc_dt_is_contig_mem(old_types[i]) )
			{
				_mpc_cl_general_datatype_try_get_info(old_types[i], &derived_ret, &input_datatype);
				count_in = input_datatype.count;
				mpc_common_nodebug("[%d]Derived length %d", old_types[i], count_in);
			}
			else
			{
				count_in = 1;
				mpc_common_nodebug("[%d]Contiguous length %d", old_types[i], count_in);
			}

			my_count_out += count_in * blocklens[i];
		}
	}

	mpc_common_nodebug("my_count_out = %d", my_count_out);
	long *begins_out = sctk_malloc(my_count_out * sizeof(long) );
	long *ends_out   = sctk_malloc(my_count_out * sizeof(long) );
	mpc_lowcomm_datatype_t *datatypes = sctk_malloc(my_count_out * sizeof(mpc_lowcomm_datatype_t) );

	for(i = 0; i < count; i++)
	{
		if( (old_types[i] != MPI_UB) && (old_types[i] != MPI_LB) )
		{
			int    j = 0;
			size_t tmp = 0;

			unsigned long extent = 0;
			unsigned long k = 0;
			unsigned long stride_t = 0;


			int derived_ret = 0;
			_mpc_lowcomm_general_datatype_t input_datatype;


			if(!_mpc_dt_is_contig_mem(old_types[i]) )
			{
				_mpc_cl_general_datatype_try_get_info(old_types[i], &derived_ret, &input_datatype);

				PMPI_Type_extent(old_types[i], (MPI_Aint *)&extent);
				mpc_common_nodebug("Extend %lu", extent);
				stride_t = (unsigned long)indices[i];

				for(j = 0; j < blocklens[i]; j++)
				{
					for(k = 0; k < input_datatype.count; k++)
					{
						begins_out[glob_count_out] = input_datatype.begins[k] + stride_t + (extent * j);
						ends_out[glob_count_out]   = input_datatype.ends[k] + stride_t + (extent * j);
						datatypes[glob_count_out]  = input_datatype.datatypes[k];

						mpc_common_nodebug(" begins_out[%lu] = %lu", glob_count_out, begins_out[glob_count_out]);
						mpc_common_nodebug(" ends_out[%lu] = %lu", glob_count_out, ends_out[glob_count_out]);

						glob_count_out++;
					}
				}

				mpc_common_nodebug("derived type %d new_lb %d new_ub %d before", i, new_lb, new_ub);

				if(input_datatype.is_ub)
				{
					long new_b = 0;
					new_b = input_datatype.ub + indices[i] + extent * (blocklens[i] - 1);
					mpc_common_nodebug("cur ub %d", new_b);
					if( new_b > new_ub || new_is_ub == 0)
					{
						new_ub = new_b;
					}
					new_is_ub = 1;
				}

				if(input_datatype.is_lb)
				{
					long new_b = 0;
					new_b = input_datatype.lb + indices[i];
					mpc_common_nodebug("cur lb %d", new_b);
					if( new_b < new_lb || new_is_lb == 0)
					{
						new_lb = new_b;
					}
					new_is_lb = 1;
				}

				mpc_common_nodebug("derived type %d new_lb %d new_ub %d after ", i, new_lb, new_ub);
			}
			else
			{
				long *begins_in = NULL;
				long *ends_in = NULL;
				long  begins_in_static = 0;
				long  ends_in_static = 0;

				begins_in    = &begins_in_static;
				ends_in      = &ends_in_static;
				begins_in[0] = 0;
				_mpc_cl_type_size(old_types[i], &(tmp) );
				ends_in[0] = begins_in[0] + tmp - 1;
				mpc_common_nodebug("Type size %lu", tmp);

				PMPI_Type_extent(old_types[i], (MPI_Aint *)&extent);

				stride_t = (unsigned long)indices[i];
				for(j = 0; j < blocklens[i]; j++)
				{
					begins_out[glob_count_out] = begins_in[0] + stride_t + (extent * j);
					ends_out[glob_count_out]   = ends_in[0] + stride_t + (extent * j);
					datatypes[glob_count_out]  = old_types[i];

					/* We use this variable to keep track of structure ending
					 * in order to do alignment when we only handle common types */
					if(common_type_size < ends_out[glob_count_out])
					{
						common_type_size = ends_out[glob_count_out];
					}

					glob_count_out++;
				}
				mpc_common_nodebug("simple type %d new_lb %d new_ub %d", i, new_lb, new_ub);
/*                              fprintf(stderr,"Type struct %ld-%ld\n",begins_out[i],ends_out[i]+1); */
			}
		}
		else
		{
			if(old_types[i] == MPI_UB)
			{
				new_is_ub = 1;
				new_ub    = indices[i];
			}
			else
			{
				new_is_lb = 1;
				new_lb    = indices[i];
			}
			mpc_common_nodebug("hard bound %d new_lb %d new_ub %d", i, new_lb, new_ub);
		}
		mpc_common_nodebug("%d new_lb %d new_ub %d", i, new_lb, new_ub);
	}
/*      fprintf(stderr,"End Type\n"); */


	/* Now check for padding if involved types are all common
	 * in order to make alignment decisions (p. 85 of the standard)
	 *
	 * struct
	 * {
	 *              int a;
	 *              char c;
	 * };
	 *
	 * I I I I C
	 *
	 * Should map to
	 *
	 * I I I I C - - -
	 *
	 * To fulfil alignment constraints
	 *
	 * */

	int did_pad_struct = 0;

	if(count)
	{
		int          types_are_all_common = 1;
		unsigned int max_pad_value        = 0;

		/* First check if we are playing with common datatypes
		 * Note that types with UB LB are ignored */
		for(i = 0; i < count; i++)
		{
			MPI_Aint cur_type_extent = 0;
			if(!mpc_lowcomm_datatype_is_common(old_types[i]) )
			{
				types_are_all_common = 0;
				break;
			}

			/*
			 * Check if all common types are well aligned. If not, skip the
			 * padding procedure
			 */
			PMPI_Type_extent(old_types[i], &cur_type_extent);
			if(indices[i] % cur_type_extent != 0)
			{
				types_are_all_common = 0;
				break;
			}

			if(max_pad_value < cur_type_extent)
			{
				max_pad_value = cur_type_extent;
			}

		}


		if(types_are_all_common)
		{
			common_type_size++;
			if( (max_pad_value > 0) && (common_type_size % max_pad_value != 0) )
			{
				unsigned int extent_mod = max_pad_value;


				long missing_to_allign = common_type_size % extent_mod;

				/* Set an UB to the type to fulfil alignment */
				new_is_ub = 1;
				new_ub    = common_type_size + extent_mod - missing_to_allign;

				if(new_ub != common_type_size)
				{
					did_pad_struct = 1;
				}
			}
		}
	}


	/* Padding DONE HERE */



	/* Set its context */
	struct _mpc_dt_context dtctx;
	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner                    = MPI_COMBINER_STRUCT;
	dtctx.count                       = count;
	dtctx.array_of_blocklenght        = blocklens;
	dtctx.array_of_displacements_addr = indices;
	dtctx.array_of_types              = old_types;

	res = _mpc_cl_general_datatype(newtype, begins_out, ends_out, datatypes, glob_count_out, new_lb, new_is_lb, new_ub, new_is_ub, &dtctx);
	assert(res == MPI_SUCCESS);


	/* Set the struct type as padded to return the UB in MPI_Type_size */

	if(did_pad_struct)
	{
		_mpc_cl_type_flag_padded(*newtype);
	}

	/*   mpc_common_nodebug("new_type %d",* newtype); */
	/*   mpc_common_nodebug("final new_lb %d,%d new_ub %d %d",new_lb,new_is_lb,new_ub,new_is_ub); */
	/*   { */
	/*     int i ;  */
	/*     for(i = 0; i < glob_count_out; i++){ */
	/*       mpc_common_nodebug("%d begins %lu ends %lu",i,begins_out[i],ends_out[i]); */
	/*     } */
	/*   } */

	sctk_free(begins_out);
	sctk_free(ends_out);

	return MPI_SUCCESS;
}

int PMPI_Type_create_struct(int count, const int blocklens[], const MPI_Aint indices[], const MPI_Datatype old_types[], MPI_Datatype *newtype)
{
	return PMPI_Type_struct(count, blocklens, indices, old_types, newtype);
}

/*********************
* ADDRESS OPERATORS *
*********************/


int PMPI_Address(const void *location, MPI_Aint *address)
{
	*address = (MPI_Aint)location;
	return MPI_SUCCESS;
}

int PMPI_Get_address(const void *location, MPI_Aint *address)
{
	return PMPI_Address(location, address);
}

/* We could add __attribute__((deprecated)) to routines like MPI_Type_extent */
int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	/* Shortcuts for most common datatypes */
	if(datatype == MPI_LONG)
	{
		*extent = sizeof(long);
	}
	else if(datatype == MPI_SHORT)
	{
		*extent = sizeof(short);
	}
	else if(datatype == MPI_CHAR ||
	        datatype == MPI_BYTE)
	{
		*extent = sizeof(char);
	}
	else if(datatype == MPI_INT)
	{
		*extent = sizeof(int);
	}
	else if(datatype == MPI_FLOAT)
	{
		*extent = sizeof(float);
	}
	else if(datatype == MPI_DOUBLE)
	{
		*extent = sizeof(double);
	}
	else
	{
		/* Special cases */
		MPI_Aint UB = 0;
		MPI_Aint LB = 0;

		PMPI_Type_lb(datatype, &LB);
		PMPI_Type_ub(datatype, &UB);

		*extent = (MPI_Aint)( (unsigned long)UB - (unsigned long)LB);

		mpc_common_nodebug("UB %d LB %d EXTENT %d", UB, LB, *extent);
	}

	return MPI_SUCCESS;
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	PMPI_Type_lb(datatype, lb);
	res = PMPI_Type_extent(datatype, extent);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent)
{
	return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent)
{
	return _mpc_cl_type_get_true_extent(datatype, true_lb, true_extent);
}

int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent)
{
	return _mpc_cl_type_get_true_extent(datatype, true_lb, true_extent);
}

int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner)
{
	return _mpc_cl_type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int PMPI_Type_get_contents(MPI_Datatype datatype,
                           int max_integers,
                           int max_addresses,
                           int max_datatypes,
                           int array_of_integers[],
                           MPI_Aint array_of_addresses[],
                           MPI_Datatype array_of_datatypes[])
{
	return _mpc_cl_type_get_contents(datatype, max_integers, max_addresses, max_datatypes, array_of_integers, array_of_addresses, array_of_datatypes);
}

/* See the 1.1 version of the Standard.  The standard made an
* unfortunate choice here, however, it is the standard.  The size returned
* by MPI_Type_size is specified as an int, not an MPI_Aint */
int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	MPI_Count tmp_size = 0;
	int       real_val = 0;

	res = PMPI_Type_size_x(datatype, &tmp_size);

	real_val = (int)tmp_size;
	*size    = real_val;

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	size_t    tmp_size = 0;
	MPI_Count real_val = 0;

	res = _mpc_cl_type_size(datatype, &tmp_size);

	real_val = tmp_size;
	*size    = real_val;

	return res;
}

/* MPI_Type_count was withdrawn in MPI 1.1 */
int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;


	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);
	unsigned long i = 0;

	int derived_ret = 0;
	_mpc_lowcomm_general_datatype_t input_datatype;

	_mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &input_datatype);

	if(derived_ret)
	{
		if( (input_datatype.is_lb == false) && input_datatype.count)
		{
			*displacement = (MPI_Aint)input_datatype.opt_begins[0];
			for(i = 1; i < input_datatype.opt_count; i++)
			{
				if( (long)*displacement > input_datatype.opt_begins[i])
				{
					*displacement = (MPI_Aint)input_datatype.opt_begins[i];
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

	mpc_common_nodebug("%lu lb", *displacement);
	return MPI_SUCCESS;
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement)
{
	MPI_Comm comm = MPI_COMM_WORLD;


	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	unsigned long i = 0;

	int derived_ret = 0;
	_mpc_lowcomm_general_datatype_t input_datatype;

	_mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &input_datatype);

	if(derived_ret)
	{
		if( (input_datatype.is_ub == false) && input_datatype.count)
		{
			*displacement = (MPI_Aint)input_datatype.opt_ends[0];

			for(i = 1; i < input_datatype.opt_count; i++)
			{
				if( (long)*displacement < input_datatype.opt_ends[i])
				{
					*displacement = (MPI_Aint)input_datatype.opt_ends[i];
				}
				mpc_common_nodebug("Current ub %lu, %lu", input_datatype.opt_ends[i], *displacement);
			}

			*displacement += (MPI_Aint)1;
		}
		else
		{
			*displacement = input_datatype.ub;
		}
	}
	else
	{
		size_t tmp = 0;
		_mpc_cl_type_size(datatype, &tmp);
		*displacement = (MPI_Aint)tmp;
	}

	mpc_common_nodebug("%lu ub", *displacement);
	return MPI_SUCCESS;
}

int PMPI_Type_create_resized(MPI_Datatype old_type, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;


	mpi_check_type(old_type, comm);
	mpi_check_type_created(old_type, comm);

	if(mpc_dt_is_valid(old_type) )
	{
		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t input_datatype;
		/* Retrieve input datatype informations */
		_mpc_cl_general_datatype_try_get_info(old_type, &derived_ret, &input_datatype);

		struct _mpc_dt_context dtctx;
		_mpc_dt_context_clear(&dtctx);
		dtctx.combiner = MPI_COMBINER_RESIZED;
		dtctx.lb       = lb;
		dtctx.extent   = extent;
		dtctx.oldtype  = old_type;

		/* Duplicate it with updated bounds in new_type */
		return _mpc_cl_general_datatype(new_type,
		                                input_datatype.begins,
		                                input_datatype.ends,
		                                input_datatype.datatypes,
		                                input_datatype.count,
		                                lb, 1,
		                                lb + extent, 1, &dtctx);
	}
	return MPI_ERR_TYPE;
}

int PMPI_Type_commit(MPI_Datatype *datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(*datatype, comm);

	mpi_check_type_created(*datatype, comm);

	return _mpc_cl_type_commit(datatype);
}

int PMPI_Type_free(MPI_Datatype *datatype)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(*datatype, comm);

	mpi_check_type_created(*datatype, comm);

	return _mpc_cl_type_free(datatype);
}

int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype)
{
	return _mpc_cl_type_dup(oldtype, newtype);
}

int PMPI_Type_get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *count)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(datatype, comm);

	int           res  = MPI_SUCCESS;
	long long int size = 0;
	int           data_size = 0;

	/* First check if the status is valid */
	if(MPI_STATUS_IGNORE == status || NULL == count)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Invalid argument");
	}

	/* Now check the data-type */
	mpi_check_type_created(datatype, MPI_COMM_WORLD);

	/* If the msg was empty */
	if(status->size == 0)
	{
		*count = 0;
		return res;
	}

	/* Get type size */
	res = PMPI_Type_size(datatype, &data_size);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	unsigned long i            = 0;

	*count = 0;

	/* If size is NULL we have no element */
	if(data_size == 0)
	{
		return res;
	}

	bool is_allocated             = false;


	datatype = _mpc_dt_get_datatype(datatype);

	/* If we are here our type has a size
	 * we can now count the number of elements*/

	/* This is the size we received */
	size = status->size;

	*count = 0;

	/* Test it is properly existing */
	_mpc_cl_type_is_allocated(datatype, &is_allocated);
	assert(is_allocated != 0);

	/* Try to rely on the datatype layout */
	/* layout = _mpc_dt_get_layout(datatype->context, &count); */

	/* Retrieve the number of block in the datatype */
	size_t el_count = datatype->count;

        /* Count the number of elements by substracting
         * individual blocks from total size until reaching 0 */
        for(i = 0; i < el_count; i++)
        {
                size -= datatype->ends[i] - datatype->begins[i] + 1;

                (*count)++;

                if(size <= 0)
                {
                        break;
                }
        }


	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Get_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count *elements)
{
	return PMPI_Type_get_elements_x(status, datatype, elements);
}

int PMPI_Type_get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	MPI_Count tmp_elements = 0;

	res = PMPI_Type_get_elements_x(status, datatype, &tmp_elements);

	*elements = tmp_elements;

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
	return PMPI_Type_get_elements(status, datatype, elements);
}

int PMPI_Type_create_darray(int size,
                            int rank,
                            int ndims,
                            const int array_of_gsizes[],
                            const int array_of_distribs[],
                            const int array_of_dargs[],
                            const int array_of_psizes[],
                            int order,
                            MPI_Datatype oldtype,
                            MPI_Datatype *newtype)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(oldtype, comm);
	mpi_check_type_created(oldtype, comm);

	int csize = 0;

	_mpc_cl_comm_size(comm, &csize);
	mpi_check_rank(rank, size, comm);


	*newtype = MPC_DATATYPE_NULL;

	res = sctk_Type_create_darray(size, rank, ndims, array_of_gsizes,
	                              array_of_distribs, array_of_dargs, array_of_psizes,
	                              order, oldtype, newtype);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	/* Fill its context */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner          = MPI_COMBINER_DARRAY;
	dtctx.size              = size;
	dtctx.ndims             = ndims;
	dtctx.rank              = rank;
	dtctx.array_of_gsizes   = array_of_gsizes;
	dtctx.array_of_distribs = array_of_distribs;
	dtctx.array_of_dargs    = array_of_dargs;
	dtctx.array_of_psizes   = array_of_psizes;
	dtctx.order             = order;
	dtctx.oldtype           = oldtype;
	_mpc_cl_type_ctx_set(*newtype, &dtctx);

	return MPI_SUCCESS;

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Type_create_subarray(int ndims,
                              const int array_of_sizes[],
                              const int array_of_subsizes[],
                              const int array_of_starts[],
                              int order,
                              MPI_Datatype oldtype,
                              MPI_Datatype *new_type)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	mpi_check_type(oldtype, comm);
	mpi_check_type_created(oldtype, comm);

	/* Create the subarray */
	res = sctk_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, new_type);

	if(res != MPI_SUCCESS)
	{
		return res;
	}

	/* Fill its context */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner          = MPI_COMBINER_SUBARRAY;
	dtctx.ndims             = ndims;
	dtctx.array_of_sizes    = array_of_sizes;
	dtctx.array_of_subsizes = array_of_subsizes;
	dtctx.array_of_starts   = array_of_starts;
	dtctx.order             = order;
	dtctx.oldtype           = oldtype;
	_mpc_cl_type_ctx_set(*new_type, &dtctx);

	return res;

	MPI_HANDLE_RETURN_VAL(res, comm);
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

int PMPI_Pack(const
              void *inbuf,
              int incount,
              MPI_Datatype datatype,
              void *outbuf, int outsize, int *position, MPI_Comm comm)
{
	mpi_check_comm(comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	if( (NULL == outbuf) || (NULL == position) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outbuf or position");
	}
	if(incount < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "wrong incount");
	}
	else if(outsize < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outsize");
	}
	else if(incount == 0)
	{
		return MPI_SUCCESS;
	}
	mpi_check_buf(inbuf, comm);

	unsigned long copied = 0;

        MPIT_Type_init(datatype);

        return MPIT_Type_memcpy((void *)inbuf, incount, datatype, outbuf,
                                MPIT_MEMCPY_FROM_USERBUF, *position, outsize);

	if(!_mpc_dt_is_contig_mem(datatype) )
	{
		unsigned long i = 0;
		int           j = 0;
		unsigned long extent = 0;

		PMPI_Type_extent(datatype, (MPI_Aint *)&extent);

		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t input_datatype;

		int res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &input_datatype);
		MPI_HANDLE_ERROR(res, comm, "Unknown datatype");

		for(j = 0; j < incount; j++)
		{
			for(i = 0; i < input_datatype.opt_count; i++)
			{
				unsigned long size = 0;
				size = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;

				if(*position + size > (size_t)outsize)
				{
					MPI_ERROR_REPORT(comm, MPI_ERR_TRUNCATE, "outbuf too small");
				}

				mpc_common_nodebug("Pack %lu->%lu, ==> %lu %lu", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);

				if(size != 0)
				{
					memcpy(&( ( (char *)outbuf)[*position]), &( ( (char *)inbuf)[input_datatype.opt_begins[i]]), size);
				}

				copied += size;
				mpc_common_nodebug("Pack %lu->%lu, ==> %lu %lu done", input_datatype.opt_begins[i] + extent * j, input_datatype.opt_ends[i] + extent * j, *position, size);

				*position = *position + size;
			}

			inbuf = (char *)inbuf + extent;
		}

		mpc_common_nodebug("%lu <= %lu", copied, outsize);
		assume(copied <= (unsigned int)outsize);
		MPI_ERROR_SUCCESS();
	}

	size_t size = 0;

	_mpc_cl_type_size(datatype, &size);
	mpc_common_nodebug("Pack %lu->%lu, ==> %lu %lu", 0, size * incount, *position, size * incount);

	if(incount * size > (size_t)outsize + *position)
	{
		size_t trunc_size = ( (outsize + *position) / size) * size;
		memcpy(&( ( (char *)outbuf)[*position]), inbuf, trunc_size);
		MPI_ERROR_REPORT(comm, MPI_ERR_TRUNCATE, "outbuf too small");
	}

	memcpy(&( ( (char *)outbuf)[*position]), inbuf, size * incount);
	mpc_common_nodebug("Pack %lu->%lu, ==> %lu %lu done", 0, size * incount, *position, size * incount);
	*position = *position + size * incount;
	copied   += size * incount;
	mpc_common_nodebug("%lu == %lu", copied, outsize);
	assume(copied <= (unsigned long)outsize);

	MPI_ERROR_SUCCESS();
}

int PMPI_Unpack(const void *inbuf,
                int insize,
                int *position,
                void *outbuf,
                int outcount,
                MPI_Datatype datatype,
                MPI_Comm comm)
{
	mpi_check_comm(comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);

	if( (NULL == inbuf) || (NULL == position) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "wrong outbuff or position");
	}

	if( (outcount < 0) || (insize < 0) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "wrong outcount insize");
	}

	if(outcount == 0)
	{
		MPI_ERROR_SUCCESS();
	}
	mpi_check_buf(outbuf, comm);

        MPIT_Type_init(datatype);

        return MPIT_Type_memcpy(outbuf, outcount, datatype, (void *)inbuf,
                                MPIT_MEMCPY_TO_USERBUF, *position, insize);

	unsigned int copied = 0;

	mpc_common_nodebug("MPI_Unpack insise = %d, position = %d, outcount = %d, datatype = %d, comm = %d", insize, *position, outcount, datatype, comm);
	if(!_mpc_dt_is_contig_mem(datatype) )
	{
		unsigned long i = 0;
		int           j = 0;
		unsigned long extent = 0;

		PMPI_Type_extent(datatype, (MPI_Aint *)&extent);

		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t output_datatype;

		int res = _mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &output_datatype);
		MPI_HANDLE_ERROR(res, comm, "Unknown datatype");

		size_t lb = 0;

		if(output_datatype.is_lb)
		{
			lb = output_datatype.lb;
		}

		for(j = 0; j < outcount; j++)
		{
			for(i = 0; i < output_datatype.opt_count; i++)
			{
				size_t size = 0;
				size = output_datatype.opt_ends[i] - output_datatype.opt_begins[i] + 1;

				if(*position + size > (size_t)insize)
				{
					MPI_ERROR_REPORT(comm, MPI_ERR_TRUNCATE, "inbuf too small");
				}

				mpc_common_nodebug("Unpack %lu %lu, ==> %lu->%lu", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.ends[i] + extent * j);
				if(size != 0)
				{
					memcpy(&( ( (char *)outbuf + lb)[output_datatype.opt_begins[i]]), &( ( (char *)inbuf)[*position]), size);
				}
				mpc_common_nodebug("Unpack %lu %lu, ==> %lu->%lu done", *position, size, output_datatype.opt_begins[i] + extent * j, output_datatype.opt_ends[i] + extent * j);

				*position = *position + size;

				copied += size;
				/* done */
				assert(copied <= (unsigned int)insize);
			}
			outbuf = (char *)outbuf + extent;
		}

		mpc_common_nodebug("%lu <= %lu", copied, insize);
		assert(copied <= (unsigned int)insize);
		MPI_ERROR_SUCCESS();
	}

	size_t size = 0;
	_mpc_cl_type_size(datatype, &size);
	mpc_common_nodebug("Unpack %lu %lu, ==> %lu->%lu", *position, size * outcount, 0, size * outcount);

	if(outcount * size > (size_t)insize + *position)
	{
		size_t trunc_size = ( (insize + *position) / size) * size;
		memcpy(outbuf, &( ( (char *)inbuf)[*position]), trunc_size);
		MPI_ERROR_REPORT(comm, MPI_ERR_TRUNCATE, "inbuf too small");
	}

	memcpy(outbuf, &( ( (char *)inbuf)[*position]), size * outcount);
	mpc_common_nodebug("Unpack %lu %lu, ==> %lu->%lu done", *position, size * outcount, 0, size * outcount);
	*position = *position + size * outcount;
	copied   += size * outcount;
	assert(copied <= (unsigned int)insize);
	MPI_ERROR_SUCCESS();

}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
	mpi_check_comm(comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);
	mpi_check_count(incount, comm);

	*size = 0;
	mpc_common_nodebug("incount size %d", incount);
	if(mpc_dt_is_valid(datatype) )
	{
		unsigned long i = 0;
		int           j = 0;

		int derived_ret = 0;
		_mpc_lowcomm_general_datatype_t input_datatype;

		_mpc_cl_general_datatype_try_get_info(datatype, &derived_ret, &input_datatype);

		mpc_common_nodebug("derived datatype : res %d, count_in %d, lb %d, is_lb %d, ub %d, is_ub %d", derived_ret, input_datatype.count, input_datatype.lb, input_datatype.is_lb, input_datatype.ub, input_datatype.is_ub);

		for(j = 0; j < incount; j++)
		{
			for(i = 0; i < input_datatype.opt_count; i++)
			{
				size_t sizet = 0;
				sizet = input_datatype.opt_ends[i] - input_datatype.opt_begins[i] + 1;
				*size = *size + sizet;
				mpc_common_nodebug("PACK derived part size %d, %lu-%lu", sizet,
				                   input_datatype.opt_begins[i], input_datatype.opt_ends[i]);
			}
		}

		mpc_common_nodebug("PACK derived final size %d", *size);
		return MPI_SUCCESS;
	}

	return MPI_ERR_TYPE;
}

/* DataType Keyval Management */

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                            MPI_Type_delete_attr_function *type_delete_attr_fn,
                            int *type_keyval, void *extra_state)
{
	return _mpc_cl_type_create_keyval(type_copy_attr_fn, type_delete_attr_fn,
	                                  type_keyval, extra_state);
}

int PMPI_Type_free_keyval(int *type_keyval)
{
	return _mpc_cl_type_free_keyval(type_keyval);
}

int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val)
{
	return _mpc_cl_type_set_attr(datatype, type_keyval, attribute_val);
}

int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag)
{
	*flag = 0;
	return _mpc_cl_type_get_attr(datatype, type_keyval, (void **)attribute_val, flag);
}

int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
{
	return _mpc_cl_type_delete_attr(datatype, type_keyval);
}

/************************************************************************/
/* MPI Pack external Support                                            */
/************************************************************************/

int PMPI_Pack_external_size(const char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	mpi_check_type(datatype, comm);
	mpi_check_type_created(datatype, comm);
	if(strcmp(datarep, "external32") != 0)
	{
		mpc_common_debug_warning("MPI_Pack_external_size: unsuported data-rep %s", datarep);
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	unsigned int i = 0;
	MPI_Aint     count = 0;
	MPI_Aint     extent = 0;

	datatype = _mpc_dt_get_datatype(datatype);

	/* Now generate the final size according to the internal type
	 * derived or contiguous one */
	*size = 0;

	/* For derived, we work block by block */
	for(i = 0; i < datatype->count; i++)
	{
		/* Get type extent */
		PMPI_Type_extent(datatype->datatypes[i], &extent);

		if(!extent)
		{
			continue;
		}

		/* Compute count */
		count = (datatype->ends[i] - datatype->begins[i] + 1) / extent;

		/* Add count times external size */
		*size += MPC_Extern32_common_type_size(datatype->datatypes[i]) * count;
	}

	*size = *size * incount;

	return MPI_SUCCESS;
}

int PMPI_Pack_external(const char *datarep, const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position)
{
	if(!strcmp(datarep, "external32") )
	{
		int      pack_size     = 0;
		MPI_Aint ext_pack_size = 0;
		PMPI_Pack_external_size(datarep, incount, datatype, &ext_pack_size);
		PMPI_Pack_size(incount, datatype, MPI_COMM_WORLD, &pack_size);

		int pos = 0;
		/* MPI_Pack takes an integer output size */
		int int_outsize = pack_size;

		if( (int)outsize < pack_size)
		{
			pack_size = outsize;
		}

		char *native_pack_buff = sctk_malloc(pack_size);
		memset(native_pack_buff, 0, pack_size);
		assume(native_pack_buff != NULL);

		/* Just pack */
		PMPI_Pack(inbuf, incount, datatype, native_pack_buff, int_outsize, &pos, MPI_COMM_WORLD);

		*position += ext_pack_size;

		/* We now have a contiguous vector gathering data-types
		 * Now apply the conversion first by extracting the datatype vector
		 * and then by converting */

		int                    type_vector_count = 0;
		MPI_Datatype *         type_vector       = NULL;
		mpc_lowcomm_datatype_t static_type;

		/* Now extract the type mask */
		type_vector = __get_typemask(datatype, &type_vector_count, &static_type);

		/* And now apply the encoding */
		MPC_Extern32_convert(type_vector,
		                     type_vector_count,
		                     native_pack_buff,
		                     pack_size,
		                     outbuf,
		                     ext_pack_size,
		                     1);

		sctk_free(native_pack_buff);
	}
	else
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "MPI_Pack_external: No such representation");
	}

	return MPI_SUCCESS;
}

int PMPI_Unpack_external(const char *datarep, const void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype)
{
	if(!strcmp(datarep, "external32") )
	{
		int      pack_size     = 0;
		MPI_Aint ext_pack_size = 0;
		PMPI_Pack_external_size(datarep, outcount, datatype, &ext_pack_size);
		PMPI_Pack_size(outcount, datatype, MPI_COMM_WORLD, &pack_size);

		char *native_pack_buff = sctk_malloc(pack_size);
		memset(native_pack_buff, 0, pack_size);
		assume(native_pack_buff != NULL);

		/* We start with a contiguous vector gathering data-types
		 * first extracting the datatype vector
		 * and then by converting the key is that the endianness conversion
		 * is a symmetrical one*/

		int                    type_vector_count = 0;
		MPI_Datatype *         type_vector       = NULL;
		mpc_lowcomm_datatype_t static_type = NULL;

		/* Now extract the type mask */
		type_vector = __get_typemask(datatype, &type_vector_count, &static_type);

		/* And now apply the encoding */
		MPC_Extern32_convert(type_vector,
		                     type_vector_count,
		                     native_pack_buff,
		                     pack_size,
		                     (char *)inbuf,
		                     ext_pack_size,
		                     0);

		/* Now we just have to unpack the converted content */
		int pos = 0;
		PMPI_Unpack(native_pack_buff, insize, &pos, outbuf, outcount, datatype, MPI_COMM_WORLD);

		*position = pos;

		sctk_free(native_pack_buff);
	}
	else
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "MPI_Pack_external: No such representation");
	}

	return MPI_SUCCESS;
}

int PMPI_Type_set_name(MPI_Datatype datatype, const char *name)
{
	return _mpc_cl_type_set_name(datatype, name);
}

int PMPI_Type_get_name(MPI_Datatype datatype, char *name, int *resultlen)
{
	return _mpc_cl_type_get_name(datatype, name, resultlen);
}

/* Neighbors collectives */
int PMPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank = 0;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	if(recvtype == MPI_DATATYPE_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "recvtype must be a valid type");
	}
	else if(recvcount < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "recvcount must be superior or equal to zero");
	}

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		if(sendcount == 0 && recvcount == 0)
		{
			return MPI_SUCCESS;
		}
	}
	else
	{
		if(recvcount == 0)
		{
			return MPI_SUCCESS;
		}
	}

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type == MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		mpc_common_nodebug("__INTERNAL__PMPI_Neighbor_allgather_cart");
		res = __INTERNAL__PMPI_Neighbor_allgather_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}
	else if(topo->type == MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		mpc_common_nodebug("__INTERNAL__PMPI_Neighbor_allgather_graph");
		res = __INTERNAL__PMPI_Neighbor_allgather_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}
	else
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}
	return res;
}

int PMPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	_mpc_cl_comm_size(comm, &size);

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	if(!(topo->type == MPI_CART || topo->type == MPI_GRAPH) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Topology must be MPI_CART or MPI_GRAPH");
	}
	else if(recvtype == MPI_DATATYPE_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "recvtype must be a valid type");
	}

	for(i = 0; i < size; ++i)
	{
		if(recvcounts[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "recvcounts must be superior or equal to zero");
		}
	}

	if(NULL == displs)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "displs must be valid");
	}

	if(!mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		for(i = 0; i < size; ++i)
		{
			if(recvcounts[i] != 0)
			{
				break;
			}
		}
		if(i >= size)
		{
			return MPI_SUCCESS;
		}
	}

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type == MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		mpc_common_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_cart");
		res = __INTERNAL__PMPI_Neighbor_allgatherv_cart(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
	}
	else if(topo->type == MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		mpc_common_nodebug("__INTERNAL__PMPI_Neighbor_allgatherv_graph");
		res = __INTERNAL__PMPI_Neighbor_allgatherv_graph(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
	}
	else
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}
	return res;
}

int PMPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	int rank;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	if(!(topo->type == MPI_CART || topo->type == MPI_GRAPH) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Topology must be MPI_CART or MPI_GRAPH");
	}
	else if(recvtype == MPI_DATATYPE_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "recvtype must be a valid type");
	}
	else if(recvcount < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "recvcount must be superior or equal to zero");
	}

	if(sendcount == 0 && recvcount == 0)
	{
		return MPI_SUCCESS;
	}

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type == MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoall_cart(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}
	else if(topo->type == MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoall_graph(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
	}
	else
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}
	return res;
}

int PMPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	if(!(topo->type == MPI_CART || topo->type == MPI_GRAPH) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Topology must be MPI_CART or MPI_GRAPH");
	}
	else if( (NULL == sendcounts) || (NULL == sdispls) || (NULL == recvcounts) || (NULL == rdispls) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Invalid arg");
	}

	_mpc_cl_comm_size(comm, &size);
	for(i = 0; i < size; i++)
	{
		if(recvcounts[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "recvcounts must be superior or equal to zero");
		}
		else if(MPI_DATATYPE_NULL == recvtype)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "recvtype must be a valid type");
		}
	}

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type == MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoallv_cart(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
	}
	else if(topo->type == MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoallv_graph(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
	}
	else
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}
	return res;
}

int PMPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
	int i, rank, size;
	int res = MPI_ERR_INTERN;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	if( (NULL == sendcounts) || (NULL == sdispls) || (NULL == sendtypes) ||
	    (NULL == recvcounts) || (NULL == rdispls) || (NULL == recvtypes) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Invalid arg");
	}

	_mpc_cl_comm_size(comm, &size);
	for(i = 0; i < size; i++)
	{
		if(recvcounts[i] < 0)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_COUNT, "recvcounts must be superior or equal to zero");
		}
		else if(MPI_DATATYPE_NULL == recvtypes[i] || recvtypes == NULL)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "recvtypes must be valid types");
		}
	}

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type == MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoallw_cart(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
	}
	else if(topo->type == MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		res = __INTERNAL__PMPI_Neighbor_alltoallw_graph(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
	}
	else
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}
	return res;
}

int PMPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
	sctk_Op *       mpc_op = NULL;
	sctk_mpi_ops_t *ops = NULL;
	int             i = 0;

	PMPC_Get_op(&ops);
	mpc_common_spinlock_lock(&(ops->lock) );

	for(i = 0; i < ops->size; i++)
	{
		if(ops->ops[i].used == 0)
		{
			mpc_op = &(ops->ops[i].op);
			break;
		}
	}

	if(mpc_op == NULL)
	{
		i          = ops->size;
		ops->size += 10;
		ops->ops   = sctk_realloc(ops->ops, ops->size * sizeof(sctk_op_t) );
		assume(ops->ops != NULL);
	}

	*op = i + MAX_MPI_DEFINED_OP;
	ops->ops[i].used = 1;
	mpc_op           = &(ops->ops[i].op);

	ops->ops[i].commute = commute;

	if(commute)
	{
		_mpc_cl_op_create(function, commute, mpc_op);
	}
	else
	{
		mpc_op->u_func = function;
	}

	mpc_common_spinlock_unlock(&(ops->lock) );
	return MPI_SUCCESS;
}

int PMPI_Op_free(MPI_Op *op)
{
	MPI_Comm comm = MPI_COMM_WORLD;


	if( (*op < 0) || (*op < MAX_MPI_DEFINED_OP) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_OP, "");
	}

	sctk_Op *       mpc_op = NULL;
	sctk_mpi_ops_t *ops;

	PMPC_Get_op(&ops);
	mpc_common_spinlock_lock(&(ops->lock) );

	assume(*op >= MAX_MPI_DEFINED_OP);

	*op -= MAX_MPI_DEFINED_OP;

	assume(*op < ops->size);
	assume(ops->ops[*op].used == 1);

	mpc_op = &(ops->ops[*op].op);

	_mpc_cl_op_free(mpc_op);
	*op = MPI_OP_NULL;
	mpc_common_spinlock_unlock(&(ops->lock) );
	return MPI_SUCCESS;
}

int PMPI_Op_commutative(__UNUSED__ MPI_Op op, __UNUSED__ int *commute)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
	mpc_common_nodebug("Enter Comm_size comm %d", comm);
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, size);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
	mpc_common_nodebug("Enter Comm_rank comm %d", comm);
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	res = _mpc_cl_comm_rank(comm, rank);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
	mpc_common_nodebug("Enter Comm_compare");

	if(comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL)
	{
		if(comm1 != comm2)
		{
			*result = MPI_UNEQUAL;
		}
		else
		{
			*result = MPI_IDENT;
		}
		return MPI_ERR_ARG;
	}

	mpi_check_comm(comm1);
	mpi_check_comm(comm2);

	mpc_lowcomm_handle_ctx_t comm1ctx = mpc_lowcomm_communicator_get_context_pointer(comm1);
	mpc_lowcomm_handle_ctx_t comm2ctx = mpc_lowcomm_communicator_get_context_pointer(comm2);

	if(!mpc_lowcomm_handle_ctx_equal(comm1ctx, comm2ctx) )
	{
		MPI_ERROR_REPORT(comm1, MPI_ERR_COMM, "Cannot compare communicators from different sessions.");
	}

	int result_group;

	*result = MPI_UNEQUAL;

	if(mpc_lowcomm_communicator_is_intercomm(comm1) != mpc_lowcomm_communicator_is_intercomm(comm2) )
	{
		*result = MPI_UNEQUAL;
		return MPI_SUCCESS;
	}

	if(comm1 == comm2)
	{
		*result = MPI_IDENT;
		return MPI_SUCCESS;
	}
	else
	{
		if(mpc_lowcomm_communicator_size(comm1) != mpc_lowcomm_communicator_size(comm2) )
		{
			*result = MPI_UNEQUAL;
			return MPI_SUCCESS;
		}

		if(comm1 == MPI_COMM_SELF || comm2 == MPI_COMM_SELF)
		{
			if(mpc_lowcomm_communicator_size(comm1) != mpc_lowcomm_communicator_size(comm2) )
			{
				*result = MPI_UNEQUAL;
			}
			else
			{
				*result = MPI_CONGRUENT;
			}
			return MPI_SUCCESS;
		}

		MPI_Group comm_group1;
		MPI_Group comm_group2;

		PMPI_Comm_group(comm1, &comm_group1);
		PMPI_Comm_group(comm2, &comm_group2);

		PMPI_Group_compare(comm_group1, comm_group2, &result_group);

		if(result_group == MPI_SIMILAR)
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

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
	mpc_common_nodebug("Enter Comm_dup");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	if(newcomm == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}

	MPI_Errhandler err;

	res = _mpc_cl_comm_dup(comm, newcomm);
	if(res != MPI_SUCCESS)
	{
		*newcomm = MPI_COMM_NULL;
		return res;
	}
	res = PMPI_Errhandler_get(comm, &err);
	if(res != MPI_SUCCESS)
	{
		*newcomm = MPI_COMM_NULL;
		return res;
	}
	res = PMPI_Errhandler_set(*newcomm, err);
	if(res != MPI_SUCCESS)
	{
		*newcomm = MPI_COMM_NULL;
		return res;
	}
	res = SCTK__MPI_Attr_communicator_dup(comm, *newcomm);
	if(res != MPI_SUCCESS)
	{
		*newcomm = MPI_COMM_NULL;
		return res;
	}
	res = SCTK__MPI_Comm_communicator_dup(comm, *newcomm);
	if(res != MPI_SUCCESS)
	{
		*newcomm = MPI_COMM_NULL;
	}
	_mpc_mpi_errhandler_free(err);
	return res;
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
	mpc_common_nodebug("Enter Comm_create");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	if(newcomm == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_GROUP, "");
	}
	res = _mpc_cl_comm_create(comm, group, newcomm);
	MPI_HANDLE_ERROR(res, comm, "Comm creation failed");

	if(*newcomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		res = PMPI_Errhandler_get(comm, &errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot retrieve errhandler");

		res = PMPI_Errhandler_set(*newcomm, errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot set errhandler");

		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader,
                          MPI_Comm peer_comm, int remote_leader, int tag,
                          MPI_Comm *newintercomm)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;
	int      size;
        int rank;

	mpi_check_comm(local_comm);
	mpi_check_comm(peer_comm);

	_mpc_cl_comm_size(local_comm, &size);
	_mpc_cl_comm_rank(local_comm, &rank);

	if(mpc_lowcomm_communicator_is_intercomm(local_comm) == 0)
	{
		mpi_check_rank_send(local_leader, size, comm);
	}

	_mpc_cl_comm_size(peer_comm, &size);

	if(mpc_lowcomm_communicator_is_intercomm(peer_comm) == 0 &&
           rank == local_leader)
	{
		mpi_check_rank_send(remote_leader, size, comm);
	}

	if(newintercomm == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}

	res = _mpc_cl_intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
	MPI_HANDLE_ERROR(res, comm, "Failed intercomm creation");

	if(*newintercomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		res = PMPI_Errhandler_get(local_comm, &errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot retrieve errhandler");

		res = PMPI_Errhandler_set(*newintercomm, errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot set errhandler");

		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
	mpc_common_nodebug("Enter Comm_split");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	if(newcomm == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
	res = _mpc_cl_comm_split(comm, color, key, newcomm);
	MPI_HANDLE_ERROR(res, comm, "Comm creation failed");

	if(*newcomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		res = PMPI_Errhandler_get(comm, &errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot retrieve errhandler");

		res = PMPI_Errhandler_set(*newcomm, errh);
		MPI_HANDLE_ERROR(res, comm, "Cannot set errhandler");

		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(res, comm);
}

static int __split_guided(MPI_Comm comm, int split_type __UNUSED__, int key __UNUSED__, MPI_Info info,
                          int *guided_shared_memory)
{
	int  buflen = 1024;
	char value[1024];
	int  flag;
	int  color = MPC_PROC_NULL;

	_mpc_cl_info_get(info, "mpi_hw_subdomain_type", buflen, value, &flag);

	if(!strcmp(value, "mpi_shared_memory") ) /* ensure consistency MPI standard 4.0 */
	{
		*guided_shared_memory = 1;
		return 0;
	}
	else
	{
		*guided_shared_memory = 0;
	}

	int size = mpc_lowcomm_communicator_size(comm);

	if(size == 1)
	{
		return MPI_PROC_NULL;
	}

	color = mpc_topology_guided_compute_color(value);

	if(color < 0)
	{
		return MPI_PROC_NULL;
	}

	if(mpc_common_get_node_count() > 1)/* create color with node id */
	{
		char str_node_idx[512];
		int  node = mpc_common_get_node_rank();
		snprintf(str_node_idx, 512, "%d%d%d%d%d%d", node, node, node, node, node, color);
		/* add node number enough time to be sure id not interfere between nodes */
		color = atoi(str_node_idx);
	}

	return color;
}

static int __split_unguided(MPI_Comm comm, int split_type __UNUSED__, int key __UNUSED__, __UNUSED__ MPI_Info info)
{
	int size  = mpc_lowcomm_communicator_size(comm);
	int color = MPC_PROC_NULL;

	if(size == 1)
	{
		return MPI_PROC_NULL;
	}
	int *tab_cpuid, *tab_color;
	int  cpu_this = mpc_topology_get_global_current_cpu();
	mpc_lowcomm_set_uid_t my_gid = mpc_lowcomm_monitor_get_gid();

	/* send to root cpu id and node rank */
	int root = 0;
	int rank;

	_mpc_cl_comm_rank(comm, &rank);
	if(rank == root)
	{
		tab_cpuid = ( int * )sctk_malloc(2 * size * sizeof(int) );
	}
	else
	{
		tab_cpuid = ( int * )sctk_malloc(2 * sizeof(int) );
	}
	tab_cpuid[0] = cpu_this;
	tab_cpuid[1] = mpc_common_get_node_rank();
	tab_color    = ( int * )sctk_malloc(size * sizeof(int) );
	mpc_lowcomm_gather(tab_cpuid, tab_cpuid, 2 * sizeof(int), root, comm);

	/* root computes color with infos gathered*/
	if(rank == root)
	{
		int k;
		int is_multinode = 0;

		/* if callers on several nodes, split with node number */
		for(k = 0; k < size; k++)
		{
			/* one rank is not on the same node / group */
			if(tab_cpuid[1] != tab_cpuid[2 * k + 1] || my_gid != mpc_lowcomm_peer_get_set(mpc_lowcomm_communicator_uid(comm, k)))
			{
				is_multinode = 1;
				break;
			}
		}
		if(is_multinode)
		{
			for(k = 0; k < size; k++)
			{
				tab_color[k] = tab_cpuid[2 * k + 1]; /* color is node rank */
			}
		}
		else
		{
			mpc_topology_unguided_compute_color(tab_color, tab_cpuid, size);
		}
	}

	/* root sends color computed */ //TODO scatter
	mpc_lowcomm_bcast( ( void * )tab_color, size * sizeof(int), root, comm);

	if(tab_color[rank] == -1)
	{
		return MPI_PROC_NULL;
	}
	color = tab_color[rank];
	sctk_free(tab_color);
	sctk_free(tab_cpuid);
	return color;
}

static inline int __split_at_level(MPI_Comm comm, int key, const char *level)
{
	/* Fill the INFO */
	MPI_Info split_val;

	PMPI_Info_create(&split_val);
	PMPI_Info_set(split_val, "mpi_hw_subdomain_type", level);

	int guided_shared_memory = 1;

	int ret = __split_guided(comm, MPI_COMM_TYPE_HW_SUBDOMAIN, key, split_val, &guided_shared_memory);

	assume(guided_shared_memory == 0);

	PMPI_Info_free(&split_val);

	return ret;
}

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, __UNUSED__ MPI_Info info,
                         MPI_Comm *newcomm)
{
	int color = 0;

	int guided_shared_memory = 0; /* ensure consistency MPI standard 4.0 */

	if(split_type == MPI_COMM_TYPE_HW_SUBDOMAIN)
	{
		color = __split_guided(comm, split_type, key, info,
		                       &guided_shared_memory);
	}

	if(guided_shared_memory)
	{
		split_type = MPI_COMM_TYPE_SHARED;
	}

	switch(split_type)
	{
		case MPI_COMM_TYPE_HW_UNGUIDED:
			color = __split_unguided(comm, split_type, key, info);
			break;
			break;

		case MPI_COMM_TYPE_APP:
			if(!mpc_launch_pmi_get_app_rank(&color) )
			{
				mpc_common_debug_warning("app rank not properly assigned");
			}
			break;

		case MPI_COMM_TYPE_SHARED:
			/* Try numa first */
			color = __split_at_level(comm, key, "NUMANode");

			/* If it failed use the node_rank */
			if(color == MPI_PROC_NULL)
			{
				color = mpc_common_get_node_rank();
			}
			break;

		case MPI_COMM_TYPE_NUMA:
			color = __split_at_level(comm, key, "NUMANode");
			break;

		case MPI_COMM_TYPE_SOCKET:
			color = __split_at_level(comm, key, "L3Cache");

			/* If it failed use lower cache */
			if(color == MPI_PROC_NULL)
			{
				color = __split_at_level(comm, key, "L2Cache");
			}
			break;

		case MPI_COMM_TYPE_UNIX_PROCESS:
			color = mpc_common_get_process_rank();
			break;

		case MPI_COMM_TYPE_NODE:
			color = mpc_common_get_node_rank();
			break;

		case MPI_COMM_TYPE_HW_SUBDOMAIN:
			/* Done in previous if */
			break;
	}

	return PMPI_Comm_split(comm, color, key, newcomm);
}

int PMPI_Comm_free(MPI_Comm *comm)
{
  mpc_common_nodebug("Enter %s", __func__);
	int res = MPI_ERR_INTERN;

	if(comm == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_COMM, "");
	}

	if( (*comm == MPI_COMM_WORLD) || (*comm == MPI_COMM_SELF) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_COMM, "");
	}

	mpi_check_comm(*comm);

	MPI_Errhandler errh = (MPI_Errhandler)_mpc_mpi_handle_get_errhandler(
		(sctk_handle) * comm, _MPC_MPI_HANDLE_COMM);

	res = PMPI_Errhandler_free(&errh);
	MPI_HANDLE_ERROR(res, MPI_COMM_WORLD, "Failed to free errhandler attached to comm");

	res = SCTK__MPI_Attr_clean_communicator(*comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

  mpc_common_nodebug("Reaching barrier on %p...", comm);
	PMPI_Barrier(*comm);
  mpc_common_nodebug("Barrier completed");

	res = SCTK__MPI_Comm_communicator_free(*comm);
	if(res != MPI_SUCCESS)
	{
		return res;
	}


	res = _mpc_cl_comm_free(comm);

  mpc_common_nodebug("Exit %s", __func__);

	MPI_HANDLE_RETURN_VAL(res, *comm);
}

int PMPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
	mpc_common_nodebug("Enter Comm_test_inter");

	mpi_check_comm(comm);
	*flag = mpc_lowcomm_communicator_is_intercomm(comm);
	return MPI_SUCCESS;
}

int PMPI_Comm_remote_size(MPI_Comm comm, int *size)
{
	mpc_common_nodebug("Enter Comm_remote_size comm %d", comm);
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);
	if(mpc_lowcomm_communicator_is_intercomm(comm) == 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}

	res = _mpc_cl_comm_remote_size(comm, size);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
	mpi_check_comm(intercomm);

	if(!mpc_lowcomm_communicator_is_intercomm(intercomm) )
	{
		MPI_ERROR_REPORT(intercomm, MPI_ERR_COMM, "MPI_Intercomm_merge expects an inter-communicator as input");
	}

	/************************
	* CHECK HIGH COHERENCY *
	************************/
	int      rank       = mpc_lowcomm_communicator_rank(intercomm);
	MPI_Comm local_comm = mpc_lowcomm_communicator_get_local(intercomm);


	/* Normalize to 1 */
	high = !!high;
	int test_high = high;

	PMPI_Allreduce(&high, &test_high, 1, MPI_INT, MPI_BAND, local_comm);

	int in_error = 0;

	if(test_high != high)
	{
		in_error = 1;
	}

	if(rank == 0)
	{
		int remote_in_error = 0;
		PMPI_Sendrecv(&in_error, 1, MPI_INT, 0, 123, &remote_in_error, 1, MPI_INT, 0, 123, intercomm, MPI_STATUS_IGNORE);
		in_error |= remote_in_error;
	}

	PMPI_Bcast(&in_error, 1, MPI_INT, 0, local_comm);

	if(in_error)
	{
		MPI_ERROR_REPORT(intercomm, MPI_ERR_ARG, "MPI_Intercomm_merge mismatching high values");
	}

	/********************
	* PROCEED TO MERGE *
	********************/

	int res = _mpc_cl_intercommcomm_merge(intercomm, high, newintracomm);

	MPI_HANDLE_ERROR(res, intercomm, "Failed intercomm merge");

	if(*newintracomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		res = PMPI_Errhandler_get(intercomm, &errh);
		MPI_HANDLE_ERROR(res, intercomm, "Cannot retrieve errhandler");

		res = PMPI_Errhandler_set(*newintracomm, errh);
		MPI_HANDLE_ERROR(res, intercomm, "Cannot set errhandler");

		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(res, intercomm);
}

/**************************
* KEYVALS AND ATTRIBUTES *
**************************/

int PMPI_Keyval_create(MPI_Copy_function *copy_fn,
                       MPI_Delete_function *delete_fn,
                       int *keyval, void *extra_state)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	int i;
	int fortran_key;

	if(*keyval == -1)
	{
		fortran_key = 1;
	}
	else
	{
		fortran_key = 0;
	}
	mpc_mpi_data_t *tmp = __get_per_task_data();

	mpc_common_nodebug("number = %d, max_number = %d", tmp->number, tmp->max_number);

	mpc_common_spinlock_lock(&(tmp->lock) );

	if(tmp->number >= tmp->max_number)
	{
		tmp->max_number += 100;
		tmp->attrs_fn    = sctk_realloc( (void *)(tmp->attrs_fn), tmp->max_number * sizeof(MPI_Caching_key_t) );
		for(i = tmp->number; i < tmp->max_number; i++)
		{
			tmp->attrs_fn[i].used = 0;
		}
	}

	*keyval = tmp->number;
	tmp->number++;

	tmp->attrs_fn[*keyval].copy_fn     = copy_fn;
	tmp->attrs_fn[*keyval].delete_fn   = delete_fn;
	tmp->attrs_fn[*keyval].extra_state = extra_state;
	tmp->attrs_fn[*keyval].used        = 1;
	tmp->attrs_fn[*keyval].fortran_key = fortran_key;

	mpc_common_spinlock_unlock(&(tmp->lock) );

	*keyval += MPI_MAX_KEY_DEFINED;
	res      = MPI_SUCCESS;

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Keyval_free(int *keyval)
{
	MPI_Comm comm = MPI_COMM_WORLD;

	if(*keyval == MPI_KEYVAL_INVALID)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	TODO("Optimize to free memory")
	* keyval = MPI_KEYVAL_INVALID;
	return MPI_SUCCESS;
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attr_value)
{
	int res = MPI_SUCCESS;

	mpi_check_comm(comm);

	mpc_mpi_data_t *            tmp;
	mpc_mpi_per_communicator_t *tmp_per_comm;
	int i;

	tmp     = __get_per_task_data();
	keyval -= MPI_MAX_KEY_DEFINED;

	if(keyval < 0)
	{
		mpc_common_nodebug("wrong keyval");
		MPI_ERROR_REPORT(comm, MPI_ERR_KEYVAL, "");
	}

	mpc_common_spinlock_lock(&(tmp->lock) );

	if(tmp->attrs_fn[keyval].used == 0)
	{
		mpc_common_nodebug("not used");
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_KEYVAL, "");
	}

	tmp_per_comm = __get_per_comm_data(comm);

	if (tmp_per_comm == NULL)
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_INTERN, "__get_per_comm_data returned NULL pointer");
	}

	assert(tmp_per_comm->key_vals);

	mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

	if(tmp_per_comm->max_number <= keyval)
	{
		if(tmp_per_comm->key_vals == NULL)
		{
			tmp_per_comm->key_vals = sctk_malloc( (keyval + 1) * sizeof(MPI_Caching_key_value_t) );
		}
		else
		{
			tmp_per_comm->key_vals = sctk_realloc(tmp_per_comm->key_vals, (keyval + 1) * sizeof(MPI_Caching_key_value_t) );
		}

		for(i = tmp_per_comm->max_number; i <= keyval; i++)
		{
			tmp_per_comm->key_vals[i].flag = 0;
			tmp_per_comm->key_vals[i].attr = NULL;
		}

		tmp_per_comm->max_number = keyval + 1;
	}

	if(tmp_per_comm->key_vals[keyval].flag == 1)
	{
		mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
		mpc_common_spinlock_unlock(&(tmp->lock) );
		res = PMPI_Attr_delete(comm, keyval + MPI_MAX_KEY_DEFINED);
		mpc_common_spinlock_lock(&(tmp->lock) );
		mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

		if(res != MPI_SUCCESS)
		{
			mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
			mpc_common_spinlock_unlock(&(tmp->lock) );
			MPI_ERROR_REPORT(comm, res, "");
		}
	}

	if(tmp->attrs_fn[keyval].fortran_key == 0)
	{
		mpc_common_nodebug("put %d for keyval %d", *( (int *)attr_value), keyval);
		tmp_per_comm->key_vals[keyval].attr = (void *)attr_value;
	}
	else
	{
		long ltmp = 0;
		ltmp = ltmp + *(long *)attr_value;
		tmp_per_comm->key_vals[keyval].attr = (void *)ltmp;
	}

	tmp_per_comm->key_vals[keyval].flag = 1;

	mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
	mpc_common_spinlock_unlock(&(tmp->lock) );

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
	int res = MPI_SUCCESS;

	mpi_check_comm(comm);

	mpc_mpi_data_t *            tmp;
	mpc_mpi_per_communicator_t *tmp_per_comm;
	void **attr;

	*flag = 0;
	attr  = (void **)attr_value;

	if( (keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED) )
	{
		*flag = 1;
		*attr = defines_attr_tab[keyval];
		return MPI_SUCCESS;
	}

	keyval -= MPI_MAX_KEY_DEFINED;

	/* wrong keyval */
	if(keyval < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_KEYVAL, "");
	}

	/* get TLS var for checking if keyval exist */
	tmp = __get_per_task_data();
	mpc_common_spinlock_lock(&(tmp->lock) );

	/* it doesn-t exist */
	if(tmp->attrs_fn[keyval].used == 0)
	{
		*flag = 0;
		mpc_common_spinlock_unlock(&(tmp->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_INTERN, "");
	}

	/* get TLS var to check attributes for keyval */
	tmp_per_comm = __get_per_comm_data(comm);
	mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

	/* it doesn't have any */
	if(tmp_per_comm->key_vals == NULL)
	{
		*flag = 0;
		*attr = NULL;
	}
	else if(keyval >= tmp_per_comm->max_number)
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
		if(tmp->attrs_fn[keyval].fortran_key == 0)
		{
			*attr = tmp_per_comm->key_vals[keyval].attr;
		}
		else
		{
			long ltmp;
			ltmp  = (long)tmp_per_comm->key_vals[keyval].attr;
			*attr = (void *)ltmp;
		}
	}

	mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
	mpc_common_spinlock_unlock(&(tmp->lock) );

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval)
{
	mpi_check_comm(comm);

	int                         res = MPI_SUCCESS;
	mpc_mpi_data_t *            tmp;
	mpc_mpi_per_communicator_t *tmp_per_comm;

	if( (keyval >= 0) && (keyval < MPI_MAX_KEY_DEFINED) )
	{
		return MPI_ERR_KEYVAL;
	}

	keyval -= MPI_MAX_KEY_DEFINED;

	tmp = __get_per_task_data();
	mpc_common_spinlock_lock(&(tmp->lock) );

	if( (tmp == NULL) || (keyval < 0) )
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		return MPI_ERR_ARG;
	}
	if(tmp->attrs_fn[keyval].used == 0)
	{
		mpc_common_spinlock_unlock(&(tmp->lock) );
		return MPI_ERR_ARG;
	}

	tmp_per_comm = __get_per_comm_data(comm);
	mpc_common_spinlock_lock(&(tmp_per_comm->lock) );

	if(tmp_per_comm->key_vals[keyval].flag == 1)
	{
		if(tmp->attrs_fn[keyval].delete_fn != NULL)
		{
			if(tmp->attrs_fn[keyval].fortran_key == 0)
			{
				mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
				mpc_common_spinlock_unlock(&(tmp->lock) );
				if(mpc_common_get_flags()->is_fortran)
				{
#ifdef MPC_FORTRAN
					void (*fort_delete)(int *pcomm, int *keyval, void *attr, void *state, int *ierr) = NULL;

					fort_delete = (void (*)(int *, int *, void *, void *, int *) ) tmp->attrs_fn[keyval].delete_fn;

					int fcomm   = MPI_Comm_c2f(comm);
					int fkeyval = keyval + MPI_MAX_KEY_DEFINED;
					fort_delete(&fcomm, &fkeyval,
					            tmp_per_comm->key_vals[keyval].attr,
					            tmp->attrs_fn[keyval].extra_state,
					            &res);
#else
					not_reachable();
#endif
				}
				else
				{
					MPI_Delete_function * delfn = (MPI_Delete_function*)tmp->attrs_fn[keyval].delete_fn;
					res = delfn(comm, keyval + MPI_MAX_KEY_DEFINED,
					                                      tmp_per_comm->key_vals[keyval].attr,
					                                      tmp->attrs_fn[keyval].extra_state);
				}

				mpc_common_spinlock_lock(&(tmp_per_comm->lock) );
				mpc_common_spinlock_lock(&(tmp->lock) );
			}
			else
			{
				int  fort_key;
				int  val;
				long long_val;
				int *ext;
				long_val = (long)(tmp_per_comm->key_vals[keyval].attr);
				val      = (int)long_val;
				fort_key = keyval + MPI_MAX_KEY_DEFINED;
				ext      = (int *)(tmp->attrs_fn[keyval].extra_state);

				mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
				mpc_common_spinlock_unlock(&(tmp->lock) );
				( (MPI_Delete_function_fortran *)tmp->attrs_fn[keyval].delete_fn)(&comm, &fort_key, &val, ext, &res);
				mpc_common_spinlock_lock(&(tmp_per_comm->lock) );
				mpc_common_spinlock_lock(&(tmp->lock) );
			}
		}
	}

	if(res == MPI_SUCCESS)
	{
		tmp_per_comm->key_vals[keyval].attr = NULL;
		tmp_per_comm->key_vals[keyval].flag = 0;
	}

	mpc_common_spinlock_unlock(&(tmp_per_comm->lock) );
	mpc_common_spinlock_unlock(&(tmp->lock) );

	MPI_HANDLE_RETURN_VAL(res, comm);
}

/******************************
 *  MPI Comm Keyval Management *
 *******************************/

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn,
                            int *comm_keyval, void *extra_state)
{
	return PMPI_Keyval_create(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

int PMPI_Comm_free_keyval(int *comm_keyval)
{
	return PMPI_Keyval_free(comm_keyval);
}

int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
	return PMPI_Attr_put(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
	return PMPI_Attr_get(comm, comm_keyval, attribute_val, flag);
}

int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
	return PMPI_Attr_delete(comm, comm_keyval);
}

/**************
* TOPOLOGIES *
**************/

int PMPI_Topo_test(MPI_Comm comm, int *topo_type)
{
	mpi_check_comm(comm);

	mpc_mpi_per_communicator_t *tmp = __get_per_comm_data(comm);

	assume(tmp != NULL);
	mpi_topology_per_comm_t *topo = &(tmp->topo);

	assume(topo != NULL);

	mpc_common_spinlock_lock(&(topo->lock) );

	if( (topo->type == MPI_CART) || (topo->type == MPI_GRAPH) )
	{
		*topo_type = topo->type;
	}
	else
	{
		*topo_type = MPI_UNDEFINED;
	}


	mpc_common_spinlock_unlock(&(topo->lock) );

	return MPI_SUCCESS;
}

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                     int reorder, MPI_Comm *comm_cart)
{
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm_old);
	{
		int i;
		int size;
		int sum = 1;

		if(comm_old == MPI_COMM_NULL || ndims < 0)
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_COMM, "");
		}

		_mpc_cl_comm_size(comm_old, &size);

		if(ndims >= 1 &&
		   (periods == NULL || comm_cart == NULL) )
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
		}

		for(i = 0; i < ndims; i++)
		{
			if(dims[i] < 0)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "");
			}
			if(dims[i] > size)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
			}
			sum *= dims[i];
		}
		if(sum > size)
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
		}
	}

	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	int nb_tasks = 1;
	int size;
	int i;
	int rank;

	_mpc_cl_comm_rank(comm_old, &rank);

	for(i = 0; i < ndims; i++)
	{
		if(dims[i] <= 0)
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "One of the dimensions is equal or less than zero");
		}
		nb_tasks *= dims[i];
		mpc_common_nodebug("dims[%d] = %d", i, dims[i]);
	}

	_mpc_cl_comm_size(comm_old, &size);

	mpc_common_nodebug("%d <= %d", nb_tasks, size);
	if(nb_tasks > size)
	{
		MPI_ERROR_REPORT(comm_old, MPI_ERR_COMM, "too small group size");
	}

	INFO("Very simple approach never reorder nor take care of hardware topology")

	if(nb_tasks == size)
	{
		res = PMPI_Comm_dup(comm_old, comm_cart);
		MPI_HANDLE_ERROR(res, comm_old, "Failled duplicating communicators");
	}
	else
	{
		MPI_Group comm_group;
		MPI_Group new_group;
		int       ranges[1][3];

		res = PMPI_Comm_group(comm_old, &comm_group);
		MPI_HANDLE_ERROR(res, comm_old, "Failed retrieving comm group");

		/*Retire ranks higher than nb_tasks */

		ranges[0][0] = nb_tasks;
		ranges[0][1] = size - 1;
		ranges[0][2] = 1;

		res = PMPI_Group_range_excl(comm_group, 1, ranges, &new_group);
		MPI_HANDLE_ERROR(res, comm_old, "Failed during PMPI_Group_range_excl");

		res = PMPI_Comm_create(comm_old, new_group, comm_cart);
		MPI_HANDLE_ERROR(res, comm_old, "Failed during PMPI_Comm_create");

		res = PMPI_Group_free(&comm_group);
		MPI_HANDLE_ERROR(res, comm_old, "Failed during PMPI_Group_free");

		res = PMPI_Group_free(&new_group);
		MPI_HANDLE_ERROR(res, comm_old, "Failed during PMPI_Group_free");
	}

	if(*comm_cart != MPC_COMM_NULL)
	{
		tmp  = __get_per_comm_data(*comm_cart);
		topo = &(tmp->topo);
		mpc_common_spinlock_lock(&(topo->lock) );
		topo->type              = MPI_CART;
		topo->data.cart.ndims   = ndims;
		topo->data.cart.reorder = reorder;
		topo->data.cart.dims    = sctk_malloc(ndims * sizeof(int) );
		memcpy(topo->data.cart.dims, dims, ndims * sizeof(int) );
		topo->data.cart.periods = sctk_malloc(ndims * sizeof(int) );
		memcpy(topo->data.cart.periods, periods, ndims * sizeof(int) );
		mpc_common_spinlock_unlock(&(topo->lock) );
	}

	MPI_HANDLE_RETURN_VAL(res, comm_old);
}

int PMPI_Dims_create(int nnodes, int ndims, int *dims)
{
	mpc_common_nodebug("Enter PMPI_Dims_create");
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	if(nnodes <= 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	int  i;
	int  freeprocs;
	int  freedims;
	int  nprimes;
	int *primes;
	int *factors;
	int *procs;
	int *p;

	if(ndims < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
	}

	/* Get # of free-to-be-assigned processes and # of free dimensions */
	freeprocs = nnodes;
	freedims  = 0;
	for(i = 0, p = dims; i < ndims; ++i, ++p)
	{
		if(*p == 0)
		{
			++freedims;
		}
		else if( (*p < 0) || ( (nnodes % *p) != 0) )
		{
			MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
		}
		else
		{
			freeprocs /= *p;
		}
	}

	if(freedims == 0)
	{
		if(freeprocs == 1)
		{
			return MPI_SUCCESS;
		}
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
	}

	if(freeprocs < 1)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_DIMS, "Invalid dims");
	}
	else if(freeprocs == 1)
	{
		for(i = 0; i < ndims; ++i, ++dims)
		{
			if(*dims == 0)
			{
				*dims = 1;
			}
		}
		return MPI_SUCCESS;
	}

	/* Compute the relevant prime numbers for factoring */
	res = getprimes(freeprocs, &nprimes, &primes);
	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "getprimes failed");

	/* Factor the number of free processes */
	res = getfactors(freeprocs, nprimes, primes, &factors);
	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "getfactors failed");

	/* Assign free processes to free dimensions */
	res = assignnodes(freedims, nprimes, primes, factors, &procs);
	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "assignnodes failed");

	/* Return assignment results */
	p = procs;
	for(i = 0; i < ndims; ++i, ++dims)
	{
		if(*dims == 0)
		{
			*dims = *p++;
		}
	}

	sctk_free( (char *)primes);
	sctk_free( (char *)factors);
	sctk_free( (char *)procs);

	return MPI_SUCCESS;
}

int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, const int index[], const int edges[],
                      int reorder, MPI_Comm *comm_graph)
{
	mpc_common_nodebug("Enter PMPI_Graph_create");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm_old);
	{
		int i;
		int size;
		int nb_edge = 0;

		_mpc_cl_comm_size(comm_old, &size);

		if( (nnodes < 0) || (nnodes > size) )
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
		}

		if(nnodes == 0)
		{
			*comm_graph = MPI_COMM_NULL;
			return MPI_SUCCESS;
		}


		nb_edge = index[nnodes - 1];
		for(i = 0; i < nb_edge; i++)
		{
			if(edges[i] < 0)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
			}
			if(edges[i] >= size)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
			}
		}
	}

	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	int size;

	_mpc_cl_comm_size(comm_old, &size);

	INFO("Very simple approach never reorder nor take care of hardware topology")
	if(nnodes == size)
	{
		res = PMPI_Comm_dup(comm_old, comm_graph);
		MPI_HANDLE_ERROR(res, comm_old, "MPI_Comm_dup failed");
	}
	else
	{
		MPI_Group comm_group;
		MPI_Group new_group;
		int       ranges[1][3];

		res = PMPI_Comm_group(comm_old, &comm_group);
		MPI_HANDLE_ERROR(res, comm_old, "PMPI_Comm_group failed");

		/*Retire ranks higher than nb_tasks */

		ranges[0][0] = nnodes;
		ranges[0][1] = size - 1;
		ranges[0][2] = 1;

		res = PMPI_Group_range_excl(comm_group, 1, ranges, &new_group);
		MPI_HANDLE_ERROR(res, comm_old, "MPI_Group_range_excl failed");

		res = PMPI_Comm_create(comm_old, new_group, comm_graph);
		MPI_HANDLE_ERROR(res, comm_old, "PMPI_Comm_create failed");

		res = PMPI_Group_free(&comm_group);
		MPI_HANDLE_ERROR(res, comm_old, "PMPI_Group_free failed");

		res = PMPI_Group_free(&new_group);
		MPI_HANDLE_ERROR(res, comm_old, "PMPI_Group_free failed");
	}

	if(*comm_graph)
	{
		tmp  = __get_per_comm_data(*comm_graph);
		topo = &(tmp->topo);
		mpc_common_spinlock_lock(&(topo->lock) );
		topo->type = MPI_GRAPH;
		topo->data.graph.nnodes  = nnodes;
		topo->data.graph.reorder = reorder;
		topo->data.graph.index   = sctk_malloc(nnodes * sizeof(int) );
		memcpy(topo->data.graph.index, index, nnodes * sizeof(int) );
		topo->data.graph.edges = sctk_malloc(index[nnodes - 1] * sizeof(int) );
		memcpy(topo->data.graph.edges, edges, index[nnodes - 1] * sizeof(int) );
		topo->data.graph.nedges = index[nnodes - 1];
		topo->data.graph.nindex = nnodes;
		mpc_common_spinlock_unlock(&(topo->lock) );
	}

	return MPI_SUCCESS;
}

int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
	mpc_common_nodebug("Enter PMPI_Graphdims_get");

	mpi_check_comm(comm);

	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type != MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	*nnodes = topo->data.graph.nnodes;
	*nedges = topo->data.graph.nedges;
	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges,
                   int *index, int *edges)
{
	mpc_common_nodebug("Enter PMPI_Graph_get");

	mpi_check_comm(comm);
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	int i;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type != MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	for(i = 0; (i < maxindex) && (i < topo->data.graph.nindex);
	    i++)
	{
		index[i] = topo->data.graph.index[i];
	}
	for(i = 0; (i < maxedges) && (i < topo->data.graph.nedges);
	    i++)
	{
		edges[i] = topo->data.graph.edges[i];
	}

	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
	mpc_common_nodebug("Enter PMPI_Cartdim_get");

	mpi_check_comm(comm);

	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type != MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	*ndims = topo->data.cart.ndims;

	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods,
                  int *coords)
{
	mpc_common_nodebug("Enter PMPI_Cart_get");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);  {
		int size;
		_mpc_cl_comm_size(comm, &size);
	}

	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	int rank;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);

	mpc_common_spinlock_lock(&(topo->lock) );
	if(topo->type != MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	if(maxdims != topo->data.cart.ndims)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid max_dims");
	}

	memcpy(dims, topo->data.cart.dims, maxdims * sizeof(int) );
	memcpy(periods, topo->data.cart.periods, maxdims * sizeof(int) );

	res = __PMPI_Cart_coords_locked(comm, rank, maxdims, coords);
	mpc_common_spinlock_unlock(&(topo->lock) );
	return res;
}

int PMPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank)
{
	mpc_common_nodebug("Enter PMPI_Cart_rank");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);

	mpc_mpi_per_communicator_t *tmp  = __get_per_comm_data(comm);
	mpi_topology_per_comm_t *   topo = &(tmp->topo);

	mpc_common_spinlock_lock(&topo->lock);

	res = __PMPI_Cart_rank_locked(comm, coords, rank);

	mpc_common_spinlock_unlock(&topo->lock);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords)
{
	mpc_common_nodebug("Enter PMPI_Cart_coords");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);

	mpc_mpi_per_communicator_t *tmp  = __get_per_comm_data(comm);
	mpi_topology_per_comm_t *   topo = &(tmp->topo);

	mpc_common_spinlock_lock(&topo->lock);

	res = __PMPI_Cart_coords_locked(comm, rank, maxdims, coords);

	mpc_common_spinlock_unlock(&topo->lock);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

void PMPIX_Get_hwsubdomain_types(char *value)
{
	/* if new level added, change enum and arrays in mpc_topology.h accordingly */
	int buflen = 1024;
	int i;

	for(i = 0; i < MPC_LOWCOMM_HW_TYPE_COUNT; ++i)
	{
		if(i == 0)
		{
			snprintf(value, buflen, "%s ", mpc_topology_split_hardware_type_name[i]);
		}
		else
		{
			strcat(value, mpc_topology_split_hardware_type_name[i]);
			if(i != MPC_LOWCOMM_HW_TYPE_COUNT - 1)
			{
				strcat(value, " ");
			}
		}
	}
}

int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
	mpi_check_comm(comm);
	int start;
	int end;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type != MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	end   = topo->data.graph.index[rank];
	start = 0;
	if(rank)
	{
		start = topo->data.graph.index[rank - 1];
	}

	*nneighbors = end - start;


	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors,
                         int *neighbors)
{
	mpi_check_comm(comm);
	int start;
	int i;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	if(topo->type != MPI_GRAPH)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	start = 0;
	if(rank)
	{
		start = topo->data.graph.index[rank - 1];
	}

	for(i = 0; i < maxneighbors && i + start < topo->data.graph.nedges; i++)
	{
		neighbors[i] = topo->data.graph.edges[i + start];
	}

	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int *source,
                    int *dest)
{
	mpi_check_comm(comm);
	if(direction < 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_DIMS, "");
	}

	if(source == NULL || dest == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	int *coords;
	int  res;
	int  rank;
	int  maxdims;
	int  new_val = MPI_PROC_NULL;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );

	_mpc_cl_comm_rank(comm, &rank);


	if(topo->type != MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	if(direction >= topo->data.cart.ndims)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid direction");
	}

	maxdims = topo->data.cart.ndims;

	coords = sctk_malloc(maxdims * sizeof(int) );

	res = __PMPI_Cart_coords_locked(comm, rank, maxdims, coords);
	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		return res;
	}

	coords[direction] = coords[direction] + displ;

	mpc_common_nodebug("New val + %d", coords[direction]);
	if(coords[direction] < 0)
	{
		if(topo->data.cart.periods[direction])
		{
			coords[direction] += topo->data.cart.dims[direction];
			res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
		}
		else
		{
			new_val = MPI_PROC_NULL;
		}
	}
	else if(coords[direction] >=
	        topo->data.cart.dims[direction])
	{
		if(topo->data.cart.periods[direction])
		{
			coords[direction] -= topo->data.cart.dims[direction];
			res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
		}
		else
		{
			new_val = MPI_PROC_NULL;
		}
	}
	else
	{
		res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
	}


	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		return res;
	}

	*dest = new_val;

	res = __PMPI_Cart_coords_locked(comm, rank, maxdims, coords);
	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		return res;
	}

	coords[direction] = coords[direction] - displ;

	mpc_common_nodebug("New val - %d", coords[direction]);
	if(coords[direction] < 0)
	{
		if(topo->data.cart.periods[direction])
		{
			coords[direction] += topo->data.cart.dims[direction];
			res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
		}
		else
		{
			new_val = MPI_PROC_NULL;
		}
	}
	else if(coords[direction] >=
	        topo->data.cart.dims[direction])
	{
		if(topo->data.cart.periods[direction])
		{
			coords[direction] -= topo->data.cart.dims[direction];
			res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
		}
		else
		{
			new_val = MPI_PROC_NULL;
		}
	}
	else
	{
		res = __PMPI_Cart_rank_locked(comm, coords, &new_val);
	}


	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		return res;
	}

	*source = new_val;

	sctk_free(coords);

	mpc_common_nodebug("dest %d src %d rank %d dir %d", *dest, *source, rank,
	                   direction);
	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *comm_new)
{
	mpc_common_nodebug("Enter PMPI_Cart_sub");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm);

	int  i;
	int *coords_in_new;
	int  color = 0;
	int  rank;
	int  size;
	int  ndims = 0;
	int  j;
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	mpi_topology_per_comm_t *   topo_new;

	tmp  = __get_per_comm_data(comm);
	topo = &(tmp->topo);
	_mpc_cl_comm_rank(comm, &rank);
	mpc_common_spinlock_lock(&(topo->lock) );

	if(remain_dims == NULL)
	{
		res = PMPI_Comm_dup(MPI_COMM_SELF, comm_new);
		MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "PMPI_Comm_dup failed");

		tmp      = __get_per_comm_data(*comm_new);
		topo_new = &(tmp->topo);

		topo_new->type            = MPI_CART;
		topo_new->data.cart.ndims = ndims;
		mpc_common_spinlock_init(&topo_new->lock, 0);
		mpc_common_spinlock_unlock(&(topo->lock) );
		return MPI_SUCCESS;
	}

	if(topo->type != MPI_CART)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		MPI_ERROR_REPORT(comm, MPI_ERR_TOPOLOGY, "Invalid type");
	}

	coords_in_new = malloc(topo->data.cart.ndims * sizeof(int) );

	res = __PMPI_Cart_coords_locked(comm, rank, topo->data.cart.ndims, coords_in_new);
	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		sctk_free(coords_in_new);
		return res;
	}


	int has_a_dim_left = 0;

	for(i = 0; i < topo->data.cart.ndims; i++)
	{
		if(remain_dims[i])
		{
			has_a_dim_left = 1;
			break;
		}
	}

	if(!has_a_dim_left)
	{
		if(rank == 0)
		{
			res = PMPI_Comm_dup(MPI_COMM_SELF, comm_new);
			if ( res != MPI_SUCCESS )
			{
				sctk_free(coords_in_new);
				MPI_ERROR_REPORT(MPI_COMM_SELF, res, "PMPI_Comm_dup failed");
			}
		}
		else
		{
			*comm_new = MPI_COMM_NULL;
		}

		mpc_common_spinlock_unlock(&(topo->lock) );
		sctk_free(coords_in_new);
		return MPI_SUCCESS;
	}


	for(i = 0; i < topo->data.cart.ndims; i++)
	{
		mpc_common_nodebug("Remain %d %d", i, remain_dims[i]);
		if(remain_dims[i] != 0)
		{
			ndims++;
			coords_in_new[i] = 0;
		}
	}

	res = __PMPI_Cart_rank_locked(comm, coords_in_new, &color);
	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		sctk_free(coords_in_new);
		return res;
	}

	mpc_common_nodebug("%d color", color);
	res = PMPI_Comm_split(comm, color, rank, comm_new);
	if(res != MPI_SUCCESS)
	{
		mpc_common_spinlock_unlock(&(topo->lock) );
		sctk_free(coords_in_new);
		return res;
	}

	sctk_free(coords_in_new);

	tmp      = __get_per_comm_data(*comm_new);
	topo_new = &(tmp->topo);

	topo_new->type              = MPI_CART;
	topo_new->data.cart.ndims   = ndims;
	topo_new->data.cart.reorder = 0;
	mpc_common_spinlock_init(&topo_new->lock, 0);
	topo_new->data.cart.dims    = sctk_malloc(ndims * sizeof(int) );
	topo_new->data.cart.periods = sctk_malloc(ndims * sizeof(int) );

	j = 0;
	for(i = 0; i < topo->data.cart.ndims; i++)
	{
		if(remain_dims[i])
		{
			topo_new->data.cart.dims[j]    = topo->data.cart.dims[i];
			topo_new->data.cart.periods[j] = topo->data.cart.periods[i];
			j++;
		}
	}

	_mpc_cl_comm_size(*comm_new, &size);
	_mpc_cl_comm_rank(*comm_new, &rank);
	mpc_common_nodebug("%d on %d new rank", rank, size);
	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

int PMPI_Cart_map(MPI_Comm comm_old, int ndims, const int dims[], __UNUSED__ const int periods[], int *newrank)
{
	mpc_common_nodebug("Enter PMPI_Cart_map");
	int res = MPI_ERR_INTERN;

	mpi_check_comm(comm_old);
	{
		int i;
		int size;
		int sum = 1;
		_mpc_cl_comm_size(comm_old, &size);
		if(ndims < 0)
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "");
		}
		for(i = 0; i < ndims; i++)
		{
			if(dims[i] <= 0)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "");
			}
			if(dims[i] >= size)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "");
			}
			sum *= dims[i];
		}
		if(sum > size)
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_DIMS, "");
		}
	}

	int nnodes = 1;
	int i;

	for(i = 0; i < ndims; i++)
	{
		nnodes *= dims[i];
	}

	TODO("Should be optimized why period is unused ?")
	res = _mpc_cl_comm_rank(comm_old, newrank);

	if(*newrank >= nnodes)
	{
		*newrank = MPI_UNDEFINED;
	}

	return res;
}

int PMPI_Graph_map(MPI_Comm comm_old, int nnodes, __UNUSED__ const int index[], const int edges[], int *newrank)
{
	mpc_common_nodebug("Enter PMPI_Graph_map");

	mpi_check_comm(comm_old);
	if(nnodes <= 0)
	{
		MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
	}
	{
		int i;
		int size;
		_mpc_cl_comm_size(comm_old, &size);
		if( (nnodes < 0) || (nnodes > size) )
		{
			MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
		}
		for(i = 0; i < nnodes; i++)
		{
			if(edges[i] < 0)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
			}
			if(edges[i] >= size)
			{
				MPI_ERROR_REPORT(comm_old, MPI_ERR_ARG, "");
			}
		}
	}

	TODO("Should be optimized, Index and Edges are unused")
	_mpc_cl_comm_rank(comm_old, newrank);

	if(nnodes <= *newrank)
	{
		*newrank = MPI_UNDEFINED;
	}

	return MPI_SUCCESS;
}

/*********************
* VARIOUS ACCESSORS *
*********************/

int PMPI_Get_processor_name(char *name, int *resultlen)
{
	MPI_Comm comm = MPI_COMM_WORLD;
	int      res  = MPI_ERR_INTERN;

	res = _mpc_cl_get_processor_name(name, resultlen);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Get_version(int *version, int *subversion)
{
	*version    = MPI_VERSION;
	*subversion = MPI_SUBVERSION;
	return MPI_SUCCESS;
}

/***************
* ERRHANDLERS *
***************/

int PMPI_Errhandler_create(MPI_Handler_function *function,
                           MPI_Errhandler *errhandler)
{
	_mpc_mpi_errhandler_register(function, errhandler);
	MPI_ERROR_SUCCESS();
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
	mpi_check_comm(comm);
	mpi_check_errhandler(errhandler, comm);

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	_mpc_mpi_handle_set_errhandler( (sctk_handle)comm, _MPC_MPI_HANDLE_COMM,
	                                (_mpc_mpi_errhandler_t)errhandler);
	MPI_ERROR_SUCCESS();
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler)
{
	mpi_check_comm(comm);

	comm        = __mpc_lowcomm_communicator_from_predefined(comm);
	*errhandler = (MPI_Errhandler)_mpc_mpi_handle_get_errhandler( (sctk_handle)comm,
	                                                              _MPC_MPI_HANDLE_COMM);

	MPI_ERROR_SUCCESS();
}

int PMPI_Errhandler_free(MPI_Errhandler *errhandler)
{
	if(!errhandler)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Null to pointer to an errhandler");
	}

        if (*errhandler == 0) {
                MPI_ERROR_SUCCESS();
        }
	mpi_check_errhandler(*errhandler, MPI_COMM_WORLD);

	int ierr = _mpc_mpi_errhandler_free(*errhandler);

	MPI_HANDLE_ERROR(ierr, MPI_COMM_WORLD, "Invalid errhandler");
	*errhandler = MPI_ERRHANDLER_NULL;
	MPI_ERROR_SUCCESS();
}

/* Error handling */
int PMPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler)
{
	_mpc_mpi_errhandler_register(file_errhandler_fn, errhandler);
	MPI_ERROR_SUCCESS();
}

int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
	mpi_check_errhandler(errhandler, MPI_COMM_WORLD);
	_mpc_mpi_handle_set_errhandler( (sctk_handle)file, _MPC_MPI_HANDLE_FILE,
	                                (_mpc_mpi_errhandler_t)errhandler);
	MPI_ERROR_SUCCESS();
	return MPI_SUCCESS;
}

int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
	*errhandler = (MPI_Errhandler)_mpc_mpi_handle_get_errhandler( (sctk_handle)file,
	                                                              _MPC_MPI_HANDLE_FILE);
	return MPI_SUCCESS;
}

int PMPI_File_call_errhandler(MPI_File file, int errorcode)
{
	MPI_Errhandler errh = SCTK_ERRHANDLER_NULL;

	PMPI_File_get_errhandler(file, &errh);

	if(errh != SCTK_ERRHANDLER_NULL)
	{
		_mpc_mpi_generic_errhandler_func_t hlndr = _mpc_mpi_errhandler_resolve(errh);

		if(hlndr)
		{
			(hlndr)(file, &errorcode);
		}
	}
	_mpc_mpi_errhandler_free(errh);

	return MPI_SUCCESS;
}

#define MPI_Error_string_convert(code, msg)   \
		case code:                    \
			(void)sprintf(string, msg); \
			break

int PMPI_Error_string(int errorcode, char *string, int *resultlen)
{
	size_t lngt = 0;

	string[0] = '\0';
	switch(errorcode)
	{
	MPI_Error_string_convert(MPI_ERR_BUFFER, "Invalid buffer pointer");
	MPI_Error_string_convert(MPI_ERR_COUNT, "Invalid count argument");
	MPI_Error_string_convert(MPI_ERR_TYPE, "Invalid datatype argument");
	MPI_Error_string_convert(MPI_ERR_TAG, "Invalid tag argument");
	MPI_Error_string_convert(MPI_ERR_COMM, "Invalid communicator");
	MPI_Error_string_convert(MPI_ERR_RANK, "Invalid rank");
	MPI_Error_string_convert(MPI_ERR_REQUEST, "Invalid mpi_request handle");
	MPI_Error_string_convert(MPI_ERR_ROOT, "Invalid root");
	MPI_Error_string_convert(MPI_ERR_GROUP, "Invalid group ");
	MPI_Error_string_convert(MPI_ERR_OP, "Invalid operation");
	MPI_Error_string_convert(MPI_ERR_TOPOLOGY, "Invalid topology");
	MPI_Error_string_convert(MPI_ERR_DIMS, "Invalid dimension argument");
	MPI_Error_string_convert(MPI_ERR_ARG, "Invalid argument of some other kind");
	MPI_Error_string_convert(MPI_ERR_UNKNOWN, "Unknown error");
	MPI_Error_string_convert(MPI_ERR_TRUNCATE, "Message truncated on receive");
	MPI_Error_string_convert(MPI_ERR_OTHER, "Other error; use Error_string");
	MPI_Error_string_convert(MPI_ERR_INTERN, "Internal MPI (implementation) error");
	MPI_Error_string_convert(MPI_ERR_IN_STATUS, "Error code in status");
	MPI_Error_string_convert(MPI_ERR_PENDING, "Pending request");
	MPI_Error_string_convert(MPI_ERR_KEYVAL, "Invalid keyval has been passed");
	MPI_Error_string_convert(MPI_ERR_NO_MEM, "MPI_ALLOC_MEM failed because memory is exhausted");
	MPI_Error_string_convert(MPI_ERR_BASE, "Invalid base passed to MPI_FREE_MEM");
	MPI_Error_string_convert(MPI_ERR_INFO_KEY, "Key longer than MPI_MAX_INFO_KEY");
	MPI_Error_string_convert(MPI_ERR_INFO_VALUE, "Value longer than MPI_MAX_INFO_VAL");
	MPI_Error_string_convert(MPI_ERR_INFO_NOKEY, "Invalid key passed to MPI_INFO_DELETE");
	MPI_Error_string_convert(MPI_ERR_SPAWN, "Error in spawning processes");
	MPI_Error_string_convert(MPI_ERR_PORT, "Invalid port name passed to MPI_COMM_CONNECT");
	MPI_Error_string_convert(MPI_ERR_SERVICE, "Invalid service name passed to MPI_UNPUBLISH_NAME");
	MPI_Error_string_convert(MPI_ERR_NAME, "Invalid service name passed to MPI_LOOKUP_NAME");
	MPI_Error_string_convert(MPI_ERR_WIN, "Invalid win argument");
	MPI_Error_string_convert(MPI_ERR_SIZE, "Invalid size argument");
	MPI_Error_string_convert(MPI_ERR_DISP, "Invalid disp argument");
	MPI_Error_string_convert(MPI_ERR_INFO, "Invalid info argument");
	MPI_Error_string_convert(MPI_ERR_LOCKTYPE, "Invalid locktype argument");
	MPI_Error_string_convert(MPI_ERR_ASSERT, "Invalid assert argument");
	MPI_Error_string_convert(MPI_ERR_RMA_CONFLICT, "Conflicting accesses to window");
	MPI_Error_string_convert(MPI_ERR_RMA_SYNC, "Wrong synchronization of RMA calls");
	MPI_Error_string_convert(MPI_ERR_FILE, "Invalid file handle");
	MPI_Error_string_convert(MPI_ERR_NOT_SAME, "Collective argument not identical on all processes, \
				or collective routines called in a different order by	different processes");
	MPI_Error_string_convert(MPI_ERR_AMODE, "Error related to the amode passed to MPI_FILE_OPEN");
	MPI_Error_string_convert(MPI_ERR_UNSUPPORTED_DATAREP, "Unsupported datarep passed to MPI_FILE_SET_VIEW");
	MPI_Error_string_convert(MPI_ERR_UNSUPPORTED_OPERATION, "Unsupported operation, \
				such as seeking on a file which	supports sequential access only");
	MPI_Error_string_convert(MPI_ERR_NO_SUCH_FILE, "File does not exist");
	MPI_Error_string_convert(MPI_ERR_FILE_EXISTS, "File exists");
	MPI_Error_string_convert(MPI_ERR_BAD_FILE, "Invalid file name (eg, path name too long");
	MPI_Error_string_convert(MPI_ERR_ACCESS, "Permission denied");
	MPI_Error_string_convert(MPI_ERR_NO_SPACE, "Not enough space");
	MPI_Error_string_convert(MPI_ERR_QUOTA, "Quota exceeded");
	MPI_Error_string_convert(MPI_ERR_READ_ONLY, "Read-only file or file system");
	MPI_Error_string_convert(MPI_ERR_FILE_IN_USE, "File operation could not be completed, \
				as the file is currently open by some process");
	MPI_Error_string_convert(MPI_ERR_DUP_DATAREP, "Conversion functions could not be registered because \
				a data representation identifier that was already defined	was passed to MPI_REGISTER_DATAREP");
	MPI_Error_string_convert(MPI_ERR_CONVERSION, "An error occurred in a user supplied data conversion function");
	MPI_Error_string_convert(MPI_ERR_IO, "Other I/O error");
	MPI_Error_string_convert(MPI_ERR_LASTCODE, "Last error code");

		default:
			mpc_common_debug_warning("%d error code unknown : %s", errorcode,
			                         string ? string : "");
	}

	lngt       = strlen(string);
	*resultlen = (int)lngt;
	MPI_ERROR_SUCCESS();
}

int PMPI_Error_class(int errorcode, int *errorclass)
{
	*errorclass = errorcode;
	MPI_ERROR_SUCCESS();
}

/**********
* TIMERS *
**********/

double PMPI_Wtime(void)
{
	return _mpc_cl_wtime();
}

double PMPI_Wtick(void)
{
	return _mpc_cl_wtick();
}

/*********************
* INIT AND FINALIZE *
*********************/

int PMPI_Query_thread(int *provided)
{
	TODO("USE sessions");
	*provided = MPI_THREAD_MULTIPLE;
	MPI_ERROR_SUCCESS();
}

static mpc_common_spinlock_t __init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

int mpc_mpi_initialize(void)
{
	mpc_common_spinlock_lock(&__init_lock);

	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);

	if(0 < task_specific->init_done)
	{
		task_specific->init_done++;
		mpc_common_spinlock_unlock(&__init_lock);
		MPI_ERROR_SUCCESS();
	}

	task_specific->init_done = 1;

	mpc_common_spinlock_unlock(&__init_lock);

	/* Proceed to INIT */

	sctk_init_yield_as_overloaded();

	task_specific->mpc_mpi_data = sctk_malloc(sizeof(struct mpc_mpi_data_s) );
	memset(task_specific->mpc_mpi_data, 0, sizeof(struct mpc_mpi_data_s) );
	mpc_common_spinlock_init(&task_specific->mpc_mpi_data->lock, 0);
	task_specific->mpc_mpi_data->requests = NULL;
	task_specific->mpc_mpi_data->buffers  = NULL;
	task_specific->mpc_mpi_data->ops      = NULL;

	task_specific->thread_level = MPC_THREAD_MULTIPLE;

        //TODO: move LCP task specific data to task_specific structure.
	//__sctk_init_mpc_request();
	__buffer_init();
	__sctk_init_mpi_errors();
#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
	__sctk_init_mpi_topo();
#endif
	__sctk_init_mpi_op();
	fortran_check_binds_resolve();
	__sctk_init_mpc_halo();

	MPI_APPNUM_VALUE       = 0;
	MPI_LASTUSEDCODE_VALUE = MPI_ERR_LASTCODE;
	_mpc_cl_comm_size(MPI_COMM_WORLD, &MPI_UNIVERSE_SIZE_VALUE);

	if(_mpc_mpi_config()->nbc.progress_thread)
	{
		task_specific->mpc_mpi_data->NBC_Pthread_handles = NULL;
		mpc_thread_mutex_init(&(task_specific->mpc_mpi_data->list_handles_lock),
		                      NULL);
		task_specific->mpc_mpi_data->nbc_initialized_per_task = 0;
		mpc_thread_mutex_init(
			&(task_specific->mpc_mpi_data->nbc_initializer_lock), NULL);
		mpc_thread_sem_init(&(task_specific->mpc_mpi_data->pending_req), 0, 0);
	}

	mpc_common_spinlock_lock(&(task_specific->per_communicator_lock) );

	mpc_per_communicator_t *per_communicator;
	mpc_per_communicator_t *per_communicator_tmp;


	HASH_ITER(hh, task_specific->per_communicator, per_communicator, per_communicator_tmp)
	{
		per_communicator->mpc_mpi_per_communicator = sctk_malloc(sizeof(struct mpc_mpi_per_communicator_s) );

		memset(per_communicator->mpc_mpi_per_communicator, 0, sizeof(struct mpc_mpi_per_communicator_s) );
		per_communicator->mpc_mpi_per_communicator_copy     = mpc_mpi_per_communicator_copy_func;
		per_communicator->mpc_mpi_per_communicator_copy_dup = mpc_mpi_per_communicator_dup_copy_func;

		mpc_common_spinlock_init(&per_communicator->mpc_mpi_per_communicator->lock, 0);

		mpc_mpi_per_communicator_t *tmp = per_communicator->mpc_mpi_per_communicator;

		__sctk_init_mpi_topo_per_comm(tmp);
		tmp->max_number = 0;
		mpc_common_spinlock_init(&tmp->topo.lock, 0);
	}

	mpc_common_spinlock_unlock(&(task_specific->per_communicator_lock) );

	_mpc_mpi_coll_allow_topological_comm();

	MPI_ERROR_SUCCESS();
}

int mpc_mpi_release(void)
{
	mpc_common_spinlock_lock(&__init_lock);

	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific = mpc_cl_per_mpi_process_ctx_get();

	if(task_specific->init_done < 0)
	{
		mpc_common_spinlock_unlock(&__init_lock);
		/* Already freed */
		return MPI_ERR_OTHER;
	}

	task_specific->init_done--;

	if(0 < task_specific->init_done)
	{
		mpc_common_spinlock_unlock(&__init_lock);
		/* Still running */
		return MPI_SUCCESS;
	}

	task_specific->init_done = -1;

	mpc_common_spinlock_unlock(&__init_lock);


	if(task_specific->mpc_mpi_data->nbc_initialized_per_task == 1)
	{
		task_specific->mpc_mpi_data->nbc_initialized_per_task = -1;
		mpc_thread_yield();
		NBC_Finalize(&(task_specific->mpc_mpi_data->NBC_Pthread) );
	}

	/* Clear attributes on COMM_SELF */
	SCTK__MPI_Attr_clean_communicator(MPI_COMM_SELF);

	fflush(stderr);
	fflush(stdout);


	MPI_ERROR_SUCCESS();
}

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
	int res = MPI_ERR_INTERN;

	int max_thread_type = MPI_THREAD_MULTIPLE;

	res = PMPI_Init(argc, argv);

	if(required < max_thread_type)
	{
		*provided = required;
	}
	else
	{
		*provided = max_thread_type;
	}

	MPI_HANDLE_RETURN_VAL(res, MPI_COMM_WORLD);
}

int PMPI_Init(int *argc __UNUSED__, char ***argv __UNUSED__)
{
	int res = MPI_ERR_INTERN;

	int flag;
	int flag_finalized;

	PMPI_Initialized(&flag);
	PMPI_Finalized(&flag_finalized);

	if( (flag != 0) || (flag_finalized != 0) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Init issue");
	}
	else
	{
		/* Set state to Initialized */
		mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();
		assume(task_specific != NULL);
		task_specific->mpi_init_state = 1;
		res = mpc_mpi_initialize();
	}

	MPI_HANDLE_RETURN_VAL(res, MPI_COMM_WORLD);
}

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
extern lcp_manager_h lcp_mngr_loc;
#endif
int PMPI_Finalize(void)
{
	int res = MPI_ERR_INTERN;

	PMPI_Barrier(MPI_COMM_WORLD);

        osc_module_fini();
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        //NOTE: Because Portals uses control flow, it may "lie" to the upper
        //      layer by saying that the message was sent while it actually
        //      needs a token before sending the message. To progress the
        //      communication, it is such needed to call LCP progression
        //      function.
        //      This is a known probleme, see ompi_mpi_finalize from OpenMPI, to
        //      overcome this, they use asynchronous PMIx call when possible.
        //      Otherwise, they do not solve the problem.
        int i = 0;
        while (i < 15) {
                lcp_progress(lcp_mngr_loc);
                usleep(100);
                i++;
        }
#endif

#ifdef MPC_Profiler
	mpc_common_init_callback_register("MPC Profile reduce",
	                                  "Reduce profile data before rendering them",
	                                  mpc_mp_profiler_do_reduce, 0);
	sctk_internal_profiler_render();
#endif

	int flag;

	PMPI_Finalized(&flag);

	if(flag != 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_OTHER, "MPI_Finalize issue");
	}

	res = mpc_mpi_release();

	/* Set state to Finalized */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();
	assume(task_specific != NULL);
	task_specific->mpi_init_state = 2;

	MPI_HANDLE_RETURN_VAL(res, MPI_COMM_WORLD);
}

int PMPI_Finalized(int *flag)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);
	*flag = (task_specific->mpi_init_state == 2);
	MPI_ERROR_SUCCESS();
}

int _mpc_mpi_init_counter(int *counter)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);
	*counter = task_specific->init_done;
	MPI_ERROR_SUCCESS();
}

int PMPI_Initialized(int *flag)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);
	*flag = (task_specific->mpi_init_state == 1);
	MPI_ERROR_SUCCESS();
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
	mpi_check_comm(comm);

	return _mpc_cl_abort(comm, errorcode);
}

int PMPI_Is_thread_main(int *flag)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific;

	task_specific = mpc_cl_per_mpi_process_ctx_get();
	*flag         = task_specific->init_done;
	return MPI_SUCCESS;
}

/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
 * stdargs are supported */
int PMPI_Pcontrol(__UNUSED__ const int level, ...)
{
	return MPI_SUCCESS;
}

/*************************************
 *  MPI-3 : Matched Probe             *
 **************************************/

/** This lock protects message handles attribution */
static mpc_common_spinlock_t __message_handle_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/** This is where message handle ids are generated */
static int __message_handle_id = 1;

/** This is a value telling if message handles have
 *  been initialized (HT in particular) */
static volatile int __message_handle_initialized = 0;

/** This is the HT where handle conversion is done */
struct mpc_common_hashtable __message_handle_ht;

/** This is the message handle data-structure */
struct MPC_Message
{
	int         message_id;
	void *      buff;
	size_t      size;
	MPI_Comm    comm;
	MPI_Status  status;
	MPI_Request request;
};

/** This is how you init the handle table once */
static void MPC_Message_handle_init_once()
{
	if(__message_handle_initialized)
	{
		return;
	}

	mpc_common_spinlock_lock_yield(&__message_handle_lock);
	if(__message_handle_initialized == 0)
	{
		__message_handle_initialized = 1;
		mpc_common_hashtable_init(&__message_handle_ht, 64);
		sctk_m_probe_matching_init();
	}

	mpc_common_spinlock_unlock(&__message_handle_lock);
}

#if 0
/** This is how you release the handle table once */
static void MPC_Message_handle_release_once()
{
	mpc_common_spinlock_lock(&__message_handle_lock);

	if(__message_handle_initialized == 1)
	{
		__message_handle_initialized = 0;
		mpc_common_hashtable_release(&__message_handle_ht);
	}

	mpc_common_spinlock_unlock(&__message_handle_lock);
}

#endif

/** This is how you create a new mesage */
struct MPC_Message *MPC_Message_new()
{
	MPC_Message_handle_init_once();

	struct MPC_Message *new = sctk_malloc(sizeof(struct MPC_Message) );

	assume(new != NULL);

	mpc_common_spinlock_lock(&__message_handle_lock);

	new->message_id = __message_handle_id++;
	new->request    = MPI_REQUEST_NULL;
	new->buff       = NULL;

	mpc_common_spinlock_unlock(&__message_handle_lock);

	mpc_common_hashtable_set(&__message_handle_ht, new->message_id, (void *)new);

	return new;
}

/** This is how you free a received message */
void MPC_Message_free(struct MPC_Message *m)
{
	MPC_Message_handle_init_once();
	sctk_free(m->buff);
	memset(m, 0, sizeof(struct MPC_Message) );
	sctk_free(m);
}

/** This is how you resolve and delete a message */
struct MPC_Message *MPC_Message_retrieve(MPI_Message message)
{
	struct MPC_Message *ret =
		(struct MPC_Message *)mpc_common_hashtable_get(&__message_handle_ht, message);

	if(ret)
	{
		mpc_common_hashtable_delete(&__message_handle_ht, message);
	}

	return ret;
}

/* probe and cancel */
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message,
                MPI_Status *status)
{
	MPI_Status st;

	if(status == MPI_STATUS_IGNORE)
	{
		status = &st;
	}

	if(source == MPI_PROC_NULL)
	{
		*message           = MPI_MESSAGE_NO_PROC;
		status->MPI_SOURCE = MPI_PROC_NULL;
		status->MPI_TAG    = MPI_ANY_TAG;
		return MPI_SUCCESS;
	}

	struct MPC_Message *m = MPC_Message_new();

	/* Do a probe & pick */

	/* Here we set the probing value */
	sctk_m_probe_matching_set(m->message_id);

	int ret = PMPI_Probe(source, tag, comm, status);

	if(ret != MPI_SUCCESS)
	{
		return ret;
	}

	/* Allocate Memory */
	int count = 0;

	PMPI_Get_count(status, MPI_CHAR, &count);
	m->buff = sctk_malloc(count);
	assume(m->buff);
	m->size = count;

	m->comm = comm;

	/* Post Irecv */
	PMPI_Irecv_internal(m->buff, count, MPI_CHAR, status->MPI_SOURCE, tag, comm,
	                    &m->request);

	/* Return the message ID */
	*( (int *)message) = m->message_id;

	sctk_m_probe_matching_reset();

	return MPI_SUCCESS;
}

int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag,
                 MPI_Message *message, MPI_Status *status)
{
	MPI_Status st;

	if(status == MPI_STATUS_IGNORE)
	{
		status = &st;
	}

	if(source == MPI_PROC_NULL)
	{
		*flag              = 1;
		*message           = MPI_MESSAGE_NO_PROC;
		status->MPI_SOURCE = MPI_PROC_NULL;
		status->MPI_TAG    = MPI_ANY_TAG;
		return MPI_SUCCESS;
	}

	struct MPC_Message *m = MPC_Message_new();

	sctk_m_probe_matching_set(m->message_id);
	/* Do a probe & pick */
	int ret = PMPI_Iprobe(source, tag, comm, flag, status);

	if(ret != MPI_SUCCESS)
	{
		return ret;
	}

	if(*flag)
	{
		/* Allocate Memory */
		int count = 0;
		PMPI_Get_count(status, MPI_CHAR, &count);
		m->buff = sctk_malloc(count);
		assume(m->buff);
		m->size = count;

		m->comm = comm;

		/* Post Irecv */
		PMPI_Irecv_internal(m->buff, count, MPI_CHAR, status->MPI_SOURCE, tag, comm,
		                    &m->request);

		/* Return the message ID */
		*( (int *)message) = m->message_id;
	}
	else
	{
		MPC_Message_free(m);
		*message = MPI_MESSAGE_NULL;
	}

	sctk_m_probe_matching_reset();

	return MPI_SUCCESS;
}

int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype,
               MPI_Message *message, MPI_Status *status)
{
	MPI_Status st;

	if(status == MPI_STATUS_IGNORE)
	{
		status = &st;
	}

	if(*message == MPI_MESSAGE_NO_PROC)
	{
		*message           = MPI_MESSAGE_NULL;
		status->MPI_SOURCE = MPI_PROC_NULL;
		status->MPI_TAG    = MPI_ANY_TAG;
		return MPI_SUCCESS;
	}

	struct MPC_Message *m = MPC_Message_retrieve(*message);

	if(!m)
	{
		return MPI_ERR_ARG;
	}

	/* This message is about to be consumed */
	*message = MPI_MESSAGE_NULL;

	mpc_common_nodebug(">>WAITINT FOR MESSAGE");
	PMPI_Wait(&m->request, MPI_STATUS_IGNORE);

	int pos = 0;

	mpc_common_nodebug("<< DONE WAITINT FOR MESSAGE");

	mpc_common_nodebug("UNPACK to %p", buf);
	PMPI_Unpack(m->buff, m->size, &pos, buf, count, datatype, m->comm);

	memcpy(status, &m->status, sizeof(MPI_Status) );

	MPC_Message_free(m);

	return MPI_SUCCESS;
}

struct MPC_Message_poll
{
	struct MPC_Message *m;
	MPI_Request *       req;
	void *              buff;
	int                 count;
	MPI_Datatype        datatype;
};

int MPI_Grequest_imrecv_query(__UNUSED__ void *extra_state, __UNUSED__ MPI_Status *status)
{
	return 0;
}

int MPI_Grequest_imrecv_poll(void *extra_state, MPI_Status *status)
{
	struct MPC_Message_poll *p = (struct MPC_Message_poll *)extra_state;

	assert(p != NULL);

	int flag = 0;

	PMPI_Test(&p->m->request, &flag, MPI_STATUS_IGNORE);

	if(flag == 1)
	{
		/* Message here then unpack */
		int pos = 0;
		PMPI_Unpack(p->m->buff, p->m->size, &pos, p->buff, p->count, p->datatype,
		            p->m->comm);

		memcpy(status, &p->m->status, sizeof(MPI_Status) );

		PMPI_Grequest_complete(*p->req);
	}

	return MPI_SUCCESS;
}

int MPI_Grequest_imrecv_cancel(__UNUSED__ void *extra_state, __UNUSED__ int complete)
{
	return MPI_SUCCESS;
}

int MPI_Grequest_imrecv_free(void *extra_state)
{
	struct MPC_Message_poll *p = (struct MPC_Message_poll *)extra_state;

	MPC_Message_free(p->m);
	sctk_free(p);

	return MPI_SUCCESS;
}

int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype,
                MPI_Message *message, MPI_Request *request)
{
	if(*message == MPI_MESSAGE_NO_PROC)
	{
		*message = MPI_MESSAGE_NULL;
		return MPI_SUCCESS;
	}

	struct MPC_Message *m = MPC_Message_retrieve(*message);

	if(!m)
	{
		return MPI_ERR_ARG;
	}

	/* This message is about to be consumed */
	*message = MPI_MESSAGE_NULL;

	/* Prepare a polling structure for generalized requests */
	struct MPC_Message_poll *poll = sctk_malloc(sizeof(struct MPC_Message_poll) );

	assume(poll != NULL);

	poll->req      = request;
	poll->m        = m;
	poll->buff     = buf;
	poll->count    = count;
	poll->datatype = datatype;

	/* Start a generalized request to do the work */
	PMPIX_Grequest_start(MPI_Grequest_imrecv_query, MPI_Grequest_imrecv_free,
	                     MPI_Grequest_imrecv_cancel, MPI_Grequest_imrecv_poll,
	                     (void *)poll, request);

	return MPI_SUCCESS;
}

/*************************************
 *  Communicator Naming                *
 **************************************/

int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
	mpi_check_comm(comm);
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	int len;

	tmp  = __get_per_comm_data(comm);
	if (tmp == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_INTERN, "__get_per_comm_data returned a NULL pointer");
	}

	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );
	len = strlen(topo->names);

	if(strcmp(topo->names, "undefined") == 0)
	{
		if(comm == MPI_COMM_WORLD)
		{
			sprintf(comm_name, "MPI_COMM_WORLD");
			len        = strlen(comm_name);
			*resultlen = len;
		}
		else if(comm == MPI_COMM_SELF)
		{
			sprintf(comm_name, "MPI_COMM_SELF");
			len        = strlen(comm_name);
			*resultlen = len;
		}
		else
		{
			memcpy(comm_name, topo->names, len + 1);
			*resultlen = len;
		}
		mpc_common_spinlock_unlock(&(topo->lock) );
	}
	else
	{
		memcpy(comm_name, topo->names, len + 1);
		*resultlen = len;
		mpc_common_spinlock_unlock(&(topo->lock) );
	}

	return MPI_SUCCESS;
}

int PMPI_Comm_set_name(MPI_Comm comm, const char *comm_name)
{
	mpi_check_comm(comm);
	mpc_mpi_per_communicator_t *tmp;
	mpi_topology_per_comm_t *   topo;
	int len;

	tmp  = __get_per_comm_data(comm);
	if (tmp == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_INTERN, "__get_per_comm_data returned a NULL pointer");
	}

	topo = &(tmp->topo);

	mpc_common_spinlock_lock(&(topo->lock) );
	memset(topo->names, '\0', MPI_MAX_NAME_STRING + 1);

	len = strlen(comm_name);
	if(len > MPI_MAX_NAME_STRING)
	{
		len = MPI_MAX_NAME_STRING;
	}
	memcpy(topo->names, comm_name, len);

	mpc_common_spinlock_unlock(&(topo->lock) );
	return MPI_SUCCESS;
}

/*************************************
 *  NULL Delete handlers              *
 **************************************/

int MPI_NULL_DELETE_FN(__UNUSED__ MPI_Comm datatype, __UNUSED__ int type_keyval, __UNUSED__ void *attribute_val_out, __UNUSED__ void *extra_state)
{
	mpc_common_nodebug("MPC_Mpi_null_delete_fn");
	return MPI_SUCCESS;
}

int MPI_NULL_COPY_FN(__UNUSED__ MPI_Comm oldcomm, __UNUSED__ int keyval, __UNUSED__ void *extra_state, __UNUSED__ void *attribute_val_in, __UNUSED__ void *attribute_val_out, __UNUSED__ int *flag)
{
	mpc_common_nodebug("MPC_Mpi_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPI_DUP_FN(__UNUSED__ MPI_Comm comm, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, __UNUSED__ void *attribute_val_in, __UNUSED__ void *attribute_val_out, int *flag)
{
	mpc_common_nodebug("MPC_Mpi_dup_fn");
	*flag = 1;
	*(void **)attribute_val_out = attribute_val_in;
	return MPI_SUCCESS;
}

/* type */
int MPI_TYPE_NULL_DELETE_FN(__UNUSED__ MPI_Datatype datatype, __UNUSED__ int type_keyval, __UNUSED__ void *attribute_val_out, __UNUSED__ void *extra_state)
{
	mpc_common_nodebug("MPC_Mpi_type_null_delete_fn");
	return MPI_SUCCESS;
}

int MPI_TYPE_NULL_COPY_FN(__UNUSED__ MPI_Datatype oldtype, __UNUSED__ int type_keyval, __UNUSED__ void *extra_state, __UNUSED__ void *attribute_val_in, __UNUSED__ void *attribute_val_out, __UNUSED__ int *flag)
{
	mpc_common_nodebug("MPC_Mpi_type_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPI_TYPE_DUP_FN(__UNUSED__ MPI_Datatype comm, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, void *attribute_val_in, void *attribute_val_out, __UNUSED__ int *flag)
{
	mpc_common_nodebug("MPC_Mpi_type_dup_fn");
	*flag = 1;
	*(void **)attribute_val_out = attribute_val_in;
	return MPI_SUCCESS;
}

/* comm */
int MPI_COMM_NULL_DELETE_FN(__UNUSED__ MPI_Comm datatype, __UNUSED__ int type_keyval, __UNUSED__ void *attribute_val_out, __UNUSED__ void *extra_state)
{
	mpc_common_nodebug("MPC_Mpi_comm_null_delete_fn");
	return MPI_SUCCESS;
}

int MPI_COMM_NULL_COPY_FN(__UNUSED__ MPI_Comm comm, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, __UNUSED__ void *attribute_val_in, __UNUSED__ void *attribute_val_out, __UNUSED__ int *flag)
{
	mpc_common_nodebug("MPC_Mpi_comm_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPI_COMM_DUP_FN(__UNUSED__ MPI_Comm comm, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag)
{
	mpc_common_nodebug("MPC_Mpi_comm_dup_fn");
	*flag = 1;
	*(void **)attribute_val_out = attribute_val_in;
	return MPI_SUCCESS;
}

/* win */
int MPI_WIN_NULL_DELETE_FN(__UNUSED__ MPI_Win win, __UNUSED__ int type_keyval, __UNUSED__ void *attribute_val_out, __UNUSED__ void *extra_state)
{
	mpc_common_nodebug("MPC_Mpi_win_null_delete_fn");
	return MPI_SUCCESS;
}

int MPI_WIN_NULL_COPY_FN(__UNUSED__ MPI_Win win, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, __UNUSED__ void *attribute_val_in, __UNUSED__ void *attribute_val_out, int *flag)
{
	mpc_common_nodebug("MPC_Mpi_win_null_copy_fn");
	*flag = 0;
	return MPI_SUCCESS;
}

int MPI_WIN_DUP_FN(__UNUSED__ MPI_Win win, __UNUSED__ int comm_keyval, __UNUSED__ void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag)
{
	mpc_common_nodebug("MPC_Mpi_win_dup_fn");
	*flag = 1;
	*(void **)attribute_val_out = attribute_val_in;
	return MPI_SUCCESS;
}

/***********************************
 *  Dummy One-Sided Communicationst *
 ************************************/

int PMPI_Free_mem(void *ptr)
{
	mpc_lowcomm_allocmem_pool_free(ptr);

	return MPI_SUCCESS;
}

int PMPI_Alloc_mem(MPI_Aint size, __UNUSED__ MPI_Info info, void *baseptr)
{
	*( (void **)baseptr) = mpc_lowcomm_allocmem_pool_alloc(size);

	if(*( (void **)baseptr) == NULL)
	{
		return MPI_ERR_INTERN;
	}



	return MPI_SUCCESS;
}

/***********************
*  MPI Info Management *
***********************/


int PMPI_Info_set(MPI_Info info, const char *key, const char *value)
{
	return _mpc_cl_info_set( (MPC_Info)info, key, value);
}

int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag)
{
	return _mpc_cl_info_get( (MPC_Info)info, key, valuelen, value, flag);
}

int PMPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag)
{
	int valuelen = 0;
	int ret      = PMPI_Info_get_valuelen(info, key, &valuelen, (int *)&flag);

	if(ret != MPI_SUCCESS)
	{
		return ret;
	}

	if(!flag)
	{
		return MPI_SUCCESS;
	}

	char *tmp_value = sctk_malloc(sizeof(char) * valuelen);

	ret = PMPI_Info_get(info, key, valuelen, tmp_value, (int *)&flag);

	if(ret != MPI_SUCCESS)
	{
		return ret;
	}

	if(!flag)
	{
		return MPI_SUCCESS;
	}

	snprintf(value, *buflen, "%s", tmp_value);
	sctk_free(tmp_value);

	*buflen = valuelen;

	return MPI_SUCCESS;
}

int PMPI_Info_free(MPI_Info *info)
{
	return _mpc_cl_info_free( (MPC_Info *)info);
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
	return _mpc_cl_info_dup( (MPC_Info)info, (MPC_Info * )newinfo);
}

int PMPI_Info_delete(MPI_Info info, const char *key)
{
	return _mpc_cl_info_delete( (MPC_Info)info, key);
}

int PMPI_Info_create(MPI_Info *info)
{
	return _mpc_cl_info_create( (MPC_Info *)info);
}

int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
	return _mpc_cl_info_get_nkeys( (MPC_Info)info, nkeys);
}

int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
	return _mpc_cl_info_get_count( (MPC_Info)info, n, key);
}

int PMPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag)
{
	return _mpc_cl_info_get_valuelen( (MPC_Info)info, key, valuelen, flag);
}

/*********************************
*  MPI_Halo Extra halo interface *
*********************************/
int PMPIX_Halo_cell_init(MPI_Halo *halo, char *label, MPI_Datatype type, int count)
{
	int new_id = 0;
	struct sctk_mpi_halo_s *new_cell = sctk_mpi_halo_context_new(sctk_halo_context_get(), &new_id);

	*halo = new_id;

	sctk_mpi_halo_init(new_cell, label, type, count);

	return MPI_SUCCESS;
}

int PMPIX_Halo_cell_release(MPI_Halo *halo)
{
	struct sctk_mpi_halo_s *cell = sctk_mpi_halo_context_get(sctk_halo_context_get(), *halo);

	if(!cell)
	{
		return MPI_ERR_ARG;
	}

	/* Remove cell from array */
	sctk_mpi_halo_context_set(sctk_halo_context_get(), *halo, NULL);

	sctk_mpi_halo_release(cell);

	*halo = MPI_HALO_NULL;

	return MPI_SUCCESS;
}

int PMPIX_Halo_cell_set(MPI_Halo halo, void *ptr)
{
	struct sctk_mpi_halo_s *cell = sctk_mpi_halo_context_get(sctk_halo_context_get(), halo);

	if(!cell)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_set(cell, ptr) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_cell_get(MPI_Halo halo, void **ptr)
{
	struct sctk_mpi_halo_s *cell = sctk_mpi_halo_context_get(sctk_halo_context_get(), halo);

	if(!cell)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_get(cell, ptr) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

/* Halo Exchange */

int PMPIX_Halo_exchange_init(MPI_Halo_exchange *ex)
{
	int new_id = 0;
	struct sctk_mpi_halo_exchange_s *new_ex = sctk_mpi_halo_context_exchange_new(sctk_halo_context_get(), &new_id);

	*ex = new_id;

	sctk_mpi_halo_exchange_init(new_ex);

	return MPI_SUCCESS;
}

int PMPIX_Halo_exchange_release(MPI_Halo_exchange *ex)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), *ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	sctk_mpi_halo_context_exchange_set(sctk_halo_context_get(), *ex, NULL);

	if(sctk_mpi_halo_exchange_release(exentry) )
	{
		return MPI_ERR_INTERN;
	}

	*ex = MPI_HALO_NULL;

	return MPI_SUCCESS;
}

int PMPIX_Halo_exchange_commit(MPI_Halo_exchange ex)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_exchange_commit(exentry) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_exchange(MPI_Halo_exchange ex)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_exchange_blocking(exentry) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_iexchange(MPI_Halo_exchange ex)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_iexchange(exentry) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_iexchange_wait(MPI_Halo_exchange ex)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_exchange_wait(exentry) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_cell_bind_local(MPI_Halo_exchange ex, MPI_Halo halo)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	struct sctk_mpi_halo_s *cell = sctk_mpi_halo_context_get(sctk_halo_context_get(), halo);

	if(!cell)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_bind_local(exentry, cell) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

int PMPIX_Halo_cell_bind_remote(MPI_Halo_exchange ex, MPI_Halo halo, int remote, char *remote_label)
{
	struct sctk_mpi_halo_exchange_s *exentry = sctk_mpi_halo_context_exchange_get(sctk_halo_context_get(), ex);

	if(!exentry)
	{
		return MPI_ERR_ARG;
	}

	struct sctk_mpi_halo_s *cell = sctk_mpi_halo_context_get(sctk_halo_context_get(), halo);

	if(!cell)
	{
		return MPI_ERR_ARG;
	}

	if(sctk_mpi_halo_bind_remote(exentry, cell, remote, remote_label) )
	{
		return MPI_ERR_INTERN;
	}

	return MPI_SUCCESS;
}

/* Error Handling */

int PMPI_Add_error_class(__UNUSED__ int *errorclass)
{
	TODO("IMPLEMENT ERROR CLASSES");
	return MPI_SUCCESS;
}

int PMPI_Add_error_code(__UNUSED__ int errorclass, __UNUSED__ int *errorcode)
{
	//not_implemented();
	return MPI_SUCCESS;
}

int PMPI_Add_error_string(__UNUSED__ int errorcode, __UNUSED__ const char *string)
{
	//not_implemented();
	return MPI_SUCCESS;
}

int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *function,
                                MPI_Errhandler *errhandler)
{
	return _mpc_cl_errhandler_create( (MPC_Handler_function *)function, errhandler);
}

int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	return _mpc_cl_errhandler_get(comm, errhandler);
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	return _mpc_cl_errhandler_set(comm, errhandler);
}

int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	_mpc_mpi_errhandler_t errh =
		_mpc_mpi_handle_get_errhandler( (sctk_handle)comm, _MPC_MPI_HANDLE_COMM);
	_mpc_mpi_generic_errhandler_func_t errf = _mpc_mpi_errhandler_resolve(errh);

	if(errf)
	{
		(errf)( (void *)&comm, &errorcode);
	}
	_mpc_mpi_errhandler_free(errh);

	return MPI_SUCCESS;
}

int PMPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
	_mpc_mpi_errhandler_t errh =
		_mpc_mpi_handle_get_errhandler( (sctk_handle)win, _MPC_MPI_HANDLE_WIN);
	_mpc_mpi_generic_errhandler_func_t errf = _mpc_mpi_errhandler_resolve(errh);

	if(errf)
	{
		(errf)( (void *)&win, &errorcode);
	}
	_mpc_mpi_errhandler_free(errh);

	return MPI_ERR_INTERN;
}

/*************************
*  MPI One-SIDED         *
*************************/

/* MPI Info storage in WIN                                              */

int PMPI_Win_set_info(MPI_Win win, MPI_Info info)
{
	/* Retrieve the MPI Desc */
	if(!win)
	{
		return MPI_ERR_ARG;
	}

	win->info = info;

	return MPI_SUCCESS;
}

int PMPI_Win_get_info(MPI_Win win, MPI_Info *info)
{
	/* Retrieve the MPI Desc */
	if(!win)
	{
		return MPI_ERR_ARG;
	}

	PMPI_Info_dup(win->info, info);

	return MPI_SUCCESS;
}

/* MPI Window Naming                                                    */

int PMPI_Win_set_name(MPI_Win win, const char *name)
{
	/* Retrieve the MPI Desc */
	if(!win)
	{
		return MPI_ERR_ARG;
	}

	if(MPI_MAX_OBJECT_NAME <= strlen(name) )
	{
		return MPI_ERR_ARG;
	}

	strcpy(win->win_name, name);

	return MPI_SUCCESS;
}

int PMPI_Win_get_name(MPI_Win win, char *name, int *len)
{
	/* Retrieve the MPI Desc */
	sprintf(name, "%s", win->win_name);
	*len = strlen(win->win_name);

	return MPI_SUCCESS;
}

/* MPI Window Creation/Release                                          */

int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                    MPI_Comm comm, MPI_Win *win)
{

        return mpc_win_create(&base, size, disp_unit, info, comm, win);
}

int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                      MPI_Comm comm, void *baseptr, MPI_Win *win)
{
        void *base;
        int rc = mpc_win_allocate(&base, size, disp_unit, info, comm, win);

        if (rc != MPI_SUCCESS) {
                return rc;
        }

        *((void **)baseptr) = base;

        return rc;
}

int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                             MPI_Comm comm, void *base, MPI_Win *win)
{
        UNUSED(size);
        UNUSED(disp_unit);
        UNUSED(info);
        UNUSED(comm);
        UNUSED(base);
        UNUSED(win);
        not_implemented();
	return 0;
}

int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
        return mpc_win_create_dynamic(info, comm, win);
}

int PMPI_Win_attach(MPI_Win, void *, MPI_Aint);
int PMPI_Win_detach(MPI_Win, const void *);

int PMPI_Win_free(MPI_Win *win)
{
        return mpc_win_free(*win);
}

/* RDMA Operations */

int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_get(origin_addr, origin_count, origin_datatype,
                           target_rank, target_disp, target_count,
                           target_datatype, win);
}

int PMPI_Put(const void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank,
             MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_put(origin_addr, origin_count, origin_datatype,
                           target_rank, target_disp, target_count,
                           target_datatype, win);
}

int PMPI_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
        if (target_rank == MPI_PROC_NULL) {
                *request = mpc_mpi_request_empty;
                return MPI_SUCCESS;
        }

        return mpc_osc_rput(origin_addr, origin_count, origin_datatype,
                            target_rank, target_disp, target_count,
                            target_datatype, win, request);
}

int PMPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
              int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
        if (target_rank == MPI_PROC_NULL) {
                *request = mpc_mpi_request_empty;
                return MPI_SUCCESS;
        }

        return mpc_osc_rget(origin_addr, origin_count, origin_datatype,
                            target_rank, target_disp, target_count,
                            target_datatype, win, request);
}

/* Synchronization calls Epochs */

int PMPI_Win_fence(int assert, MPI_Win win)
{
        return mpc_osc_fence(assert, win);
}

/* Active target sync */

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
        return mpc_osc_start(group, assert, win);
}

int PMPI_Win_complete(MPI_Win win)
{
        return mpc_osc_complete(win);
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
        return mpc_osc_post(group, assert, win);
}

int PMPI_Win_wait(MPI_Win win)
{
        return mpc_osc_wait(win);
}

int PMPI_Win_test(MPI_Win win, int *flag)
{
        return mpc_osc_test(win, flag);
}

/* Passive Target Sync */

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
        if (rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_lock(lock_type, rank, assert, win);
}

int PMPI_Win_unlock(int rank, MPI_Win win)
{
        if (rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_unlock(rank, win);
}

int PMPI_Win_lock_all(int assert, MPI_Win win)
{
        return mpc_osc_lock_all(assert, win);
}

int PMPI_Win_unlock_all(MPI_Win win)
{
        return mpc_osc_unlock_all(win);
}

/* Error Handling */

int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                               MPI_Errhandler *errhandler)
{
	return _mpc_cl_errhandler_create( (MPC_Handler_function *)win_errhandler_fn,
	                                  errhandler);
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
	_mpc_mpi_handle_set_errhandler( (sctk_handle)win, _MPC_MPI_HANDLE_WIN,
	                                (_mpc_mpi_errhandler_t)errhandler);
	return MPI_SUCCESS;
}

int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
	*errhandler = (MPC_Errhandler)_mpc_mpi_handle_get_errhandler( (sctk_handle)win,
	                                                              _MPC_MPI_HANDLE_WIN);
	return MPI_SUCCESS;
}

/* Window Flush Calls */

int PMPI_Win_sync(MPI_Win win)
{
        return mpc_osc_sync(win);
}

int PMPI_Win_flush(int rank, MPI_Win win)
{
        if (rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_flush(rank, win);
}

int PMPI_Win_flush_local(int rank, MPI_Win win)
{
        if (rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_flush_local(rank, win);
}

int PMPI_Win_flush_all(MPI_Win win)
{
        return mpc_osc_flush_all(win);
}

int PMPI_Win_flush_local_all(MPI_Win win)
{
        return mpc_osc_flush_local_all(win);
}

int PMPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
	return MPI_Comm_group(win->comm, group);
}

int PMPI_Accumulate(const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank,
                    MPI_Aint target_disp, int target_count,
                    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_accumulate(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, op, win);
}

int PMPI_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request *request)
{
        if (target_rank == MPI_PROC_NULL) {
                *request = mpc_mpi_request_empty;
                return MPI_SUCCESS;
        }

        return mpc_osc_raccumulate(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count,
                                   target_datatype, op, win, request);
}

int PMPI_Get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr,
                        int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_get_accumulate(origin_addr, origin_count,
                                      origin_datatype, result_addr,
                                      result_count, result_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, op, win);
}

int PMPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr,
                         int result_count, MPI_Datatype result_datatype,
                         int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win, MPI_Request *request)
{
        if (target_rank == MPI_PROC_NULL) {
                *request = mpc_mpi_request_empty;
                return MPI_SUCCESS;
        }

        return mpc_osc_rget_accumulate(origin_addr, origin_count,
                                       origin_datatype, result_addr,
                                       result_count, result_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, op, win, request);
}

int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Op op, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_fetch_and_op(origin_addr, result_addr, datatype,
                                    target_rank, target_disp, op, win);
}

int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win)
{
        if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

        return mpc_osc_compare_and_swap(origin_addr, compare_addr, result_addr,
                                        datatype, target_rank, target_disp,
                                        win);
}

/* ATTR handling */

int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
        return mpc_osc_win_set_attr(win, win_keyval, attribute_val);
}

int PMPI_Win_get_attr(MPI_Win win, int keyval, void *attr_val,
                      int *flag)
{
        return mpc_osc_win_get_attr(win, keyval, attr_val, flag);
}

int PMPI_Win_free_keyval(int *win_keyval)
{
        return mpc_osc_win_free_keyval(win_keyval);
}

int PMPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
        return mpc_osc_win_delete_attr(win, win_keyval);
}

int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                           MPI_Win_delete_attr_function *win_delete_attr_fn,
                           int *win_keyval, void *extra_state)
{
        return mpc_osc_win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

/* Win shared */

int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit,
                          void *baseptr)
{
        UNUSED(win);
        UNUSED(rank);
        UNUSED(size);
        UNUSED(disp_unit);
        UNUSED(baseptr);
        not_implemented();
	return 0;
}

int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size)
{
	return mpc_osc_win_attach(win, base, size);
}

int PMPI_Win_detach(MPI_Win win, const void *base)
{
	return mpc_osc_win_detach(win, base);
}

/* Checkpointing */
int PMPIX_Checkpoint(MPIX_Checkpoint_state *state)
{
	return _mpc_cl_checkpoint(state);
}

/* Aint OPs */

MPI_Aint PMPI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
	return base + disp;
}

MPI_Aint PMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
	return addr1 - addr2;
}

int PMPI_Get_library_version(char *version, __UNUSED__ int *resultlen)
{
	snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING - 1,
	         "MPC version %d.%d.%d%s %s",
	         MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH,
	         MPC_VERSION_PRE,
	         sctk_alloc_mode() );
	*resultlen = strlen(version);
	return MPI_SUCCESS;
}

/*******************
* NAME RESOLUTION *
*******************/

int PMPI_Lookup_name(const char *service_name,
                     __UNUSED__ MPI_Info info,
                     char *port_name)
{
	int ret = mpc_lowcomm_lookup_name(service_name,
	                                  port_name,
	                                  MPI_MAX_PORT_NAME);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		ret = MPI_ERR_NAME;
	}
	else
	{
		ret = MPI_SUCCESS;
	}

	MPI_HANDLE_RETURN_VAL(ret, MPI_COMM_SELF);
}

int PMPI_Publish_name(const char *service_name, MPI_Info info __UNUSED__, const char *port_name)
{
	int ret = mpc_lowcomm_publish_name(service_name, port_name);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		ret = MPI_ERR_INTERN;
	}
	else
	{
		ret = MPI_SUCCESS;
	}

	MPI_HANDLE_RETURN_VAL(ret, MPI_COMM_SELF);
}

int PMPI_Unpublish_name(const char *service_name,
                        __UNUSED__ MPI_Info info,
                        __UNUSED__ const char *port_name)
{
	int ret = mpc_lowcomm_unpublish_name(service_name, port_name);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		ret = MPI_ERR_INTERN;
	}
	else
	{
		ret = MPI_SUCCESS;
	}

	MPI_HANDLE_RETURN_VAL(ret, MPI_COMM_SELF);
}

/*****************************
* COMMUNICATOR INTERCONNECT *
*****************************/

int PMPI_Open_port(__UNUSED__ MPI_Info info, char *port_name)
{
	int ret = mpc_lowcomm_open_port(port_name, MPI_MAX_PORT_NAME);

	if(ret != 0)
	{
		ret = MPI_ERR_INTERN;
	}
	else
	{
		ret = MPI_SUCCESS;
	}

	MPI_HANDLE_RETURN_VAL(ret, MPI_COMM_SELF);
}

/* Process Creation and Management */
int PMPI_Close_port(const char *port_name)
{
	int ret = mpc_lowcomm_close_port(port_name);

	if(ret != 0)
	{
		ret = MPI_ERR_INTERN;
	}
	else
	{
		ret = MPI_SUCCESS;
	}

	MPI_HANDLE_RETURN_VAL(ret, MPI_COMM_SELF);
}

int PMPI_Comm_accept(const char *port_name,
                     __UNUSED__ MPI_Info info,
                     int root,
                     MPI_Comm comm,
                     MPI_Comm *newcomm)
{
  mpc_common_nodebug("Enter Comm_accept");

	mpi_check_comm(comm);

	if(mpc_lowcomm_communicator_is_intercomm(comm))
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "Comm_accept was provided an intercommunicator input comm");
	}

	if(newcomm == NULL)
	{
		// Returning MPI_ERR_ARG rather than MPI_ERR_COMM since newcomm is not an input param:
		// a newcomm == NULL is not a communicator error, just a wrong argument
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Comm_accept was provided a NULL newcomm");
	}

	if(port_name == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Comm_accept was provided a NULL port_name");
	}

	if(newcomm == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Comm_accept was provided a NULL newcomm");
	}

  int ret = mpc_lowcomm_communicator_accept(port_name,
	                                          root,
	                                          comm,
	                                          newcomm);

	ret = ret != 0 ? MPI_ERR_INTERN : MPI_SUCCESS;

  MPI_HANDLE_ERROR(ret, comm, "Comm accept failed");

  if(*newcomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		ret = PMPI_Errhandler_get(comm, &errh);
		MPI_HANDLE_ERROR(ret, comm, "Cannot retrieve errhandler");

		ret = PMPI_Errhandler_set(*newcomm, errh);
		MPI_HANDLE_ERROR(ret, comm, "Cannot set errhandler");
		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(ret, comm);
}

int PMPI_Comm_connect(const char *port_name,
                      __UNUSED__ MPI_Info info,
                      int root,
                      MPI_Comm comm,
                      MPI_Comm *newcomm)
{
  mpc_common_nodebug("Enter Comm_connect");

	mpi_check_comm(comm);

	if(mpc_lowcomm_communicator_is_intercomm(comm))
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "Comm_connect was provided an intercommunicator input comm");
	}

	if(newcomm == NULL)
	{
		// Returning MPI_ERR_ARG rather than MPI_ERR_COMM since newcomm is not an input param:
		// a newcomm == NULL is not a communicator error, just a wrong argument
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Comm_connect was provided a NULL newcomm");
	}

	if(port_name == NULL)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Comm_connect was provided a NULL port_name");
	}

  int ret = mpc_lowcomm_communicator_connect(port_name,
      root,
      comm,
      newcomm);
	ret = ret != 0 ? MPI_ERR_INTERN : MPI_SUCCESS;

  MPI_HANDLE_ERROR(ret, comm, "Comm connect failed");

  if(*newcomm != MPI_COMM_NULL)
	{
		MPI_Errhandler errh;
		ret = PMPI_Errhandler_get(comm, &errh);
		MPI_HANDLE_ERROR(ret, comm, "Cannot retrieve errhandler");

		ret = PMPI_Errhandler_set(*newcomm, errh);
		MPI_HANDLE_ERROR(ret, comm, "Cannot set errhandler");

		PMPI_Errhandler_free(&errh);
	}

	MPI_HANDLE_RETURN_VAL(ret, comm);
}

/************************************************************************/
/*  NOT IMPLEMENTED                                                     */
/************************************************************************/

int PMPI_Register_datarep(__UNUSED__ const char *datarep, __UNUSED__ MPI_Datarep_conversion_function *read_conversion_fn, __UNUSED__ MPI_Datarep_conversion_function *write_conversion_fn, __UNUSED__ MPI_Datarep_extent_function *dtype_file_extent_fn, __UNUSED__ void *extra_state)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* Communicator Management */
int PMPI_Comm_idup(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Comm *newcomm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_idup_with_info(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Comm *newcomm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_dup_with_info(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Comm *newcomm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_set_info(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_get_info(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info *info_used)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_disconnect(__UNUSED__ MPI_Comm *comm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_get_parent(__UNUSED__ MPI_Comm *parent)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_join(__UNUSED__ int fd, __UNUSED__ MPI_Comm *intercomm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_spawn(__UNUSED__ const char *command, __UNUSED__ char *argv[], __UNUSED__ int maxprocs, __UNUSED__ MPI_Info info, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Comm *intercomm, __UNUSED__ int array_of_errcodes[])
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Comm_spawn_multiple(__UNUSED__ int count, __UNUSED__ char *array_of_commands[], __UNUSED__ char **array_of_argv[], __UNUSED__ const int array_of_maxprocs[], const __UNUSED__ MPI_Info array_of_info[], __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Comm *intercomm, __UNUSED__ int array_of_errcodes[])
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* Dist graph operations */
int PMPI_Dist_graph_neighbors_count(__UNUSED__ MPI_Comm comm, __UNUSED__ int *indegree, __UNUSED__ int *outdegree, __UNUSED__ int *weighted)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Dist_graph_neighbors(__UNUSED__ MPI_Comm comm, __UNUSED__ int maxindegree, __UNUSED__ int sources[], __UNUSED__ int sourceweights[], __UNUSED__ int maxoutdegree, __UNUSED__ int destinations[], __UNUSED__ int destweights[])
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Dist_graph_create(__UNUSED__ MPI_Comm comm_old, __UNUSED__ int n, const __UNUSED__ int sources[], const __UNUSED__ int degrees[], const __UNUSED__ int destinations[], const __UNUSED__ int weights[], __UNUSED__ MPI_Info info, __UNUSED__ int reorder, __UNUSED__ MPI_Comm *comm_dist_graph)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Dist_graph_create_adjacent(__UNUSED__ MPI_Comm comm_old, __UNUSED__ int indegree, const __UNUSED__ int sources[], const __UNUSED__ int sourceweights[], __UNUSED__ int outdegree, const __UNUSED__ int destinations[], const __UNUSED__ int destweights[], __UNUSED__ MPI_Info info, __UNUSED__ int reorder, __UNUSED__ MPI_Comm *comm_dist_graph)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* collectives */
int PMPI_Reduce_local(__UNUSED__ const void *inbuf, __UNUSED__ void *inoutbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* FORTRAN TYPE */
int PMPI_Type_create_f90_complex(__UNUSED__ int precision, __UNUSED__ int range, __UNUSED__ MPI_Datatype *newtype)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Type_create_f90_integer(__UNUSED__ int range, __UNUSED__ MPI_Datatype *newtype)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Type_create_f90_real(__UNUSED__ int precision, __UNUSED__ int range, __UNUSED__ MPI_Datatype *newtype)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* MPIX methods */
int PMPIX_Comm_failure_ack(__UNUSED__ MPI_Comm comm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPIX_Comm_failure_get_acked(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Group *failedgrp)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPIX_Comm_agree(__UNUSED__ MPI_Comm comm, __UNUSED__ int *flag)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPIX_Comm_revoke(__UNUSED__ MPI_Comm comm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPIX_Comm_shrink(__UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Comm *newcomm)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/* I neighbor */

int PMPI_Neighbor_allgatherv_init(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const int displs[], __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Ineighbor_alltoallw(__UNUSED__ const void *sendbuf, __UNUSED__ const int sendcounts[], __UNUSED__ const MPI_Aint sdispls[], __UNUSED__ const __UNUSED__ MPI_Datatype sendtypes[], __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const MPI_Aint rdispls[], __UNUSED__ const __UNUSED__ MPI_Datatype recvtypes[], __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Ineighbor_alltoallv(__UNUSED__ const void *sendbuf, __UNUSED__ const int sendcounts[], __UNUSED__ const int sdispls[], __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const int rdispls[], __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Ineighbor_alltoall(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Ineighbor_allgatherv(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const int displs[], __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Ineighbor_allgather(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Neighbor_alltoallv_init(__UNUSED__ const void *sendbuf, __UNUSED__ const int sendcounts[], __UNUSED__ const int sdispls[], __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const int rdispls[], __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Neighbor_alltoallw_init(__UNUSED__ const void *sendbuf, __UNUSED__ const int sendcounts[], __UNUSED__ const MPI_Aint sdispls[], __UNUSED__ const __UNUSED__ MPI_Datatype sendtypes[], __UNUSED__ void *recvbuf, __UNUSED__ const int recvcounts[], __UNUSED__ const MPI_Aint rdispls[], __UNUSED__ const __UNUSED__ MPI_Datatype recvtypes[], __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Neighbor_alltoall_init(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

int PMPI_Neighbor_allgather_init(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPI_Info info, __UNUSED__ MPI_Request *request)
{
	not_implemented(); return MPI_ERR_INTERN;
}

/************************************************************************/
/* END NOT IMPLEMENTED                                                     */
/************************************************************************/

/******************
* INIT CALLBACKS *
******************/


void mpc_mpi_init() __attribute__( (constructor) );

void mpc_mpi_init()
{
	MPC_INIT_CALL_ONLY_ONCE

	/* Used to notify migration avoiding no yield barriers in this case */
	mpc_common_init_callback_register("MPC_MPI Force Yield",
	                                  "Force Yield as condition was met",
	                                  __force_yield, 0);
}

/***************
* COLLECTIVES *
***************/


int PMPI_Barrier(MPI_Comm comm)
{
  mpc_common_nodebug("Enter %s", __func__);
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Barrier);

	/* Error checking */
	mpi_check_comm(comm);




	int (*barrier_ptr)(MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(barrier_ptr, comm, barrier);
	assume(barrier_ptr != NULL);

	res = (barrier_ptr)(comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Barrier);

  mpc_common_nodebug("Exit %s", __func__);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Bcast);

	/* Error checking */
	mpi_check_comm(comm);

	if(MPI_IN_PLACE == buffer)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	_mpc_cl_comm_size(comm, &size);
	mpi_check_root(root, size, comm);
	mpi_check_rank_send(root, size, comm);
	mpi_check_buf(buffer, comm);
	mpi_check_count(count, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);




	/* Bcast is not synchronous, the collective can end directly if nothing
	 * has to be sent/received
	 */
	if(count <= 0)
	{
		return MPI_SUCCESS;
	}

	int (*bcast_ptr)(void *, int, MPI_Datatype, int, MPI_Comm comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(bcast_ptr, comm, bcast);
	assume(bcast_ptr != NULL);

	res = (bcast_ptr)(buffer, count, datatype, root, comm);

	/* Profiling */
	SCTK_PROFIL_END(MPI_Bcast);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */

	SCTK_PROFIL_START(MPI_Gather);

	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if( (sendcnt == 0 || recvcnt == 0) && !mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		return MPI_SUCCESS;
	}

	if(sendbuf == recvbuf && rank == root)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}

	/* Error checking */
	mpi_check_root(root, size, comm);
	if(rank != root)
	{
		mpi_check_buf(sendbuf, comm);
		mpi_check_count(sendcnt, comm);
		mpi_check_type(sendtype, comm);
		mpi_check_type_commited(sendtype, comm);
	}
	else
	{
		if(MPI_IN_PLACE != sendbuf)
		{
			mpi_check_buf(sendbuf, comm);
			mpi_check_count(sendcnt, comm);
			mpi_check_type(sendtype, comm);
			mpi_check_type_commited(sendtype, comm);
		}
		mpi_check_buf(recvbuf, comm);
		mpi_check_count(recvcnt, comm);
		mpi_check_type_commited(recvtype, comm);
	}

	if( (0 == sendcnt && MPI_ROOT != root &&
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



	int (*gather_ptr)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(gather_ptr, comm, gather);
	assume(gather_ptr != NULL);

	res = (gather_ptr)(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Gather);

	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcnts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Gatherv);

	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	/* Error checking */
	mpi_check_root(root, size, comm);
	mpi_check_buf(sendbuf, comm);
	mpi_check_count(sendcnt, comm);
	mpi_check_type(sendtype, comm);
	mpi_check_type_commited(sendtype, comm);
	mpi_check_buf(recvbuf, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(recvtype, comm);

	if(sendbuf == recvbuf && rank == root)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if( (rank != root && MPI_IN_PLACE == sendbuf) ||
	    (rank == root && MPI_IN_PLACE == recvbuf) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}


	int (*gatherv_ptr)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, int, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(gatherv_ptr, comm, gatherv);
	assume(gatherv_ptr != NULL);

	res = (gatherv_ptr)(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);



	/* Profiling */
	SCTK_PROFIL_END(MPI_Gatherv);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                 MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Scatter);

	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}

	/* Error checking */
	mpi_check_root(root, size, comm);
	if(rank == root)
	{
		mpi_check_buf(sendbuf, comm);
		mpi_check_count(sendcnt, comm);
		mpi_check_type(sendtype, comm);
		mpi_check_type_commited(sendtype, comm);
		if(recvbuf == sendbuf)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
		}
	}
	else
	{
		mpi_check_buf(recvbuf, comm);
		mpi_check_count(recvcnt, comm);
		mpi_check_type(recvtype, comm);
		mpi_check_type_commited(recvtype, comm);
	}

	if( (rank != root && MPI_IN_PLACE == recvbuf) ||
	    (rank == root && MPI_IN_PLACE == sendbuf) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if( (0 == recvcnt && MPI_ROOT != root &&
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


	int (*scatter_ptr)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(scatter_ptr, comm, scatter);
	assume(scatter_ptr != NULL);

	res = (scatter_ptr)(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Scatter);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Scatterv(const void *sendbuf, const int *sendcnts, const int *displs,
                  MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	int      res = MPI_ERR_INTERN;
	int      size, rsize, rank, i;
	MPI_Aint extent;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Scatterv);

	res = PMPI_Type_extent(sendtype, &extent);
	MPI_HANDLE_ERROR(res, comm, "Error getting extent");

	/* Error checking */
	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	MPI_HANDLE_ERROR(res, comm, "Error getting size");


	res = _mpc_cl_comm_rank(comm, &rank);

	MPI_HANDLE_ERROR(res, comm, "Error getting rank");

	if(sendbuf == recvbuf && rank == root)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		res = PMPI_Comm_remote_size(comm, &rsize);
		MPI_HANDLE_ERROR(res, comm, "Error getting remote size");

		mpi_check_root(root, rsize, comm);
		if(root == MPI_ROOT)
		{
			mpi_check_type(sendtype, comm);
			mpi_check_type_commited(sendtype, comm);
			for(i = 0; i < size; i++)
			{
				mpi_check_buf( (char *)(sendbuf) + displs[i] * extent, comm);
				mpi_check_count(sendcnts[i], comm);
			}
		}
		else if(root != MPI_PROC_NULL)
		{
			mpi_check_buf(recvbuf, comm);
			mpi_check_count(recvcnt, comm);
			mpi_check_type(recvtype, comm);
			mpi_check_type_commited(recvtype, comm);
		}
	}
	else
	{
		mpi_check_root(root, size, comm);
		if(rank == root)
		{
			mpi_check_type(sendtype, comm);
			mpi_check_type_commited(sendtype, comm);
			for(i = 0; i < size; i++)
			{
				mpi_check_buf( (char *)(sendbuf) + displs[i] * extent, comm);
				mpi_check_count(sendcnts[i], comm);
			}
			if(recvbuf != MPI_IN_PLACE)
			{
				mpi_check_buf(recvbuf, comm);
				mpi_check_count(recvcnt, comm);
				mpi_check_type(recvtype, comm);
				mpi_check_type_commited(recvtype, comm);
			}
		}
		else
		{
			mpi_check_buf(recvbuf, comm);
			mpi_check_count(recvcnt, comm);
			mpi_check_type(recvtype, comm);
			mpi_check_type_commited(recvtype, comm);
		}
	}

	if( (rank != root && MPI_IN_PLACE == recvbuf) ||
	    (rank == root && MPI_IN_PLACE == sendbuf) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}




	int (*scatterv_ptr)(const void *, const int *, const int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(scatterv_ptr, comm, scatterv);
	assume(scatterv_ptr != NULL);

	res = (scatterv_ptr)(sendbuf, sendcnts, displs, sendtype, recvbuf, recvcnt, recvtype, root, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Scatterv);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	/* Profiling */
	SCTK_PROFIL_START(MPI_Allgather);

	/* Error checking */
	mpi_check_comm(comm);

	if(sendbuf != MPI_IN_PLACE)
	{
		mpi_check_buf(sendbuf, comm);
		mpi_check_count(sendcount, comm);
		mpi_check_type(sendtype, comm);
		mpi_check_type_commited(sendtype, comm);
	}

	if(sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}
	mpi_check_buf(recvbuf, comm);
	mpi_check_count(recvcount, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(recvtype, comm);

        if (sendtype == MPI_DATATYPE_NULL) {
                sendtype = recvtype;
        } else if (recvtype == MPI_DATATYPE_NULL) {
                recvtype = sendtype;
        }

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}

	int is_intercomm = mpc_lowcomm_communicator_is_intercomm(comm);

	if(is_intercomm)
	{
		if(0 == sendcount && 0 == recvcount)
		{
			return MPI_SUCCESS;
		}
#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
		else
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
		}
#endif
	}
	else
	{
		if( (MPI_IN_PLACE != sendbuf && 0 == sendcount) || (0 == recvcount) )
		{
			return MPI_SUCCESS;
		}
	}

	if(MPI_IN_PLACE == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}


	int (*allgather_ptr)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(allgather_ptr, comm, allgather);
	assume(allgather_ptr != NULL);

	res = (allgather_ptr)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Allgather);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcount, const int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif

	/* Profiling */
	SCTK_PROFIL_START(MPI_Allgatherv);

	mpi_check_comm(comm);

	if(sendbuf != MPI_IN_PLACE)
	{
		mpi_check_buf(sendbuf, comm);
		mpi_check_count(sendcount, comm);
		mpi_check_type(sendtype, comm);
		mpi_check_type_commited(sendtype, comm);
	}

	if(sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "Sendbuf is equal to Recv Buff");
	}

	mpi_check_buf(recvbuf, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(recvtype, comm);

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}

	if(MPI_IN_PLACE == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "RecvBuff is MPI_IN_PLACE");
	}


	int (*allgatherv_ptr)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(allgatherv_ptr, comm, allgatherv);
	assume(allgatherv_ptr != NULL);

	res = (allgatherv_ptr)(sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Allgatherv);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Alltoall);

	/* Error checking */
	if(sendcount == 0 && recvcount == 0)
	{
		return MPI_SUCCESS;
	}
	mpi_check_comm(comm);
	if(sendbuf != MPI_IN_PLACE)
	{
		mpi_check_buf(sendbuf, comm);
		mpi_check_count(sendcount, comm);
		mpi_check_type(sendtype, comm);
		mpi_check_type_commited(sendtype, comm);
	}
	mpi_check_buf(recvbuf, comm);
	mpi_check_count(recvcount, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(recvtype, comm);

	if(MPI_IN_PLACE == sendbuf)
	{
		sendcount = recvcount;
		sendtype  = recvtype;
	}

	if(recvbuf == MPI_IN_PLACE || sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}



	int (*alltoall_ptr)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(alltoall_ptr, comm, alltoall);
	assume(alltoall_ptr != NULL);

	res = (alltoall_ptr)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Alltoall);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
                   const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size;
	int i;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Alltoallv);

	/* Error checking */
	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	if(MPI_IN_PLACE == sendbuf)
	{
		sendcnts = recvcnts;
		sdispls  = rdispls;
		sendtype = recvtype;
	}

	for(i = 0; i < size; i++)
	{
		mpi_check_count(sendcnts[i], comm);
		mpi_check_count(recvcnts[i], comm);
	}
	mpi_check_buf(sendbuf, comm);
	mpi_check_type(sendtype, comm);
	mpi_check_type_commited(sendtype, comm);
	mpi_check_buf(recvbuf, comm);
	mpi_check_type(recvtype, comm);
	mpi_check_type_commited(recvtype, comm);

	if(MPI_IN_PLACE == recvbuf || recvbuf == sendbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(sendtype != recvtype)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_TYPE, "Mismatched datatypes");
	}


	int (*alltoallv_ptr)(const void *, const int *, const int *, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(alltoallv_ptr, comm, alltoallv);
	assume(alltoallv_ptr != NULL);
	res = (alltoallv_ptr)(sendbuf, sendcnts, sdispls, sendtype, recvbuf, recvcnts, rdispls, recvtype, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Alltoallv);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Alltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype *sendtypes,
                   void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	//int size, rsize, rank;
	int      size, rank;
	int      i;
	MPI_Aint sextent, rextent;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Alltoallw);

	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	//res = PMPI_Comm_remote_size(comm, &rsize);
	//if(res != MPI_SUCCESS)
	//{
	//	return res;
	//}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	if(sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}
	/* Error checking */
	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	for(i = 0; i < size; i++)
	{
		if(sendbuf != MPI_IN_PLACE)
		{
			mpi_check_type(sendtypes[i], comm);
			mpi_check_type_commited(sendtypes[i], comm);
			res = PMPI_Type_extent(sendtypes[i], &sextent);
			if(res != MPI_SUCCESS)
			{
				return res;
			}
			mpi_check_buf( (char *)(sendbuf) + sdispls[i] * sextent, comm);
			mpi_check_count(sendcnts[i], comm);
		}
		mpi_check_type(recvtypes[i], comm);
		mpi_check_type_commited(sendtypes[i], comm);
		res = PMPI_Type_extent(recvtypes[i], &rextent);
		if(res != MPI_SUCCESS)
		{
			return res;
		}
		mpi_check_buf( (char *)(recvbuf) + rdispls[i] * rextent, comm);
		mpi_check_count(recvcnts[i], comm);
	}

	if(MPI_IN_PLACE == sendbuf)
	{
		sendcnts  = recvcnts;
		sdispls   = rdispls;
		sendtypes = recvtypes;
	}

	if( (NULL == sendcnts) || (NULL == sdispls) || (NULL == sendtypes) ||
	    (NULL == recvcnts) || (NULL == rdispls) || (NULL == recvtypes) ||
	    recvbuf == MPI_IN_PLACE)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}



	int (*alltoallw_ptr)(const void *, const int *, const int *, const MPI_Datatype *, void *, const int *, const int *, const MPI_Datatype *, MPI_Comm) = NULL;

	MPC_MPI_CONFIG_ROUTE_COLL(alltoallw_ptr, comm, alltoallw);
	assume(alltoallw_ptr != NULL);
	res = (alltoallw_ptr)(sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Alltoallw);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;
	int size, rank;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Reduce);

	/* Error checking */
	mpi_check_comm(comm);
	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}
	res = _mpc_cl_comm_rank(comm, &rank);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	mpi_check_root(root, size, comm);
	mpi_check_count(count, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		if(root == MPI_ROOT)
		{
			mpi_check_buf(recvbuf, comm);
		}
		else if(root != MPI_PROC_NULL)
		{
			mpi_check_buf(sendbuf, comm);
		}
	}
	else
	{
		if(rank == root)
		{
			if(sendbuf == MPI_IN_PLACE)
			{
				mpi_check_buf(recvbuf, comm);
			}
			else
			{
				mpi_check_buf(sendbuf, comm);
			}
			mpi_check_buf(recvbuf, comm);
		}
		else
		{
			mpi_check_buf(sendbuf, comm);
		}
	}

	if(rank == root &&
	   ( (MPI_IN_PLACE == recvbuf) || (sendbuf == recvbuf) ) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	if(0 == count)
	{
		return MPI_SUCCESS;
	}


	int (*reduce_ptr)(const void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(reduce_ptr, comm, reduce);
	assume(reduce_ptr != NULL);
	res = (reduce_ptr)(sendbuf, recvbuf, count, datatype, op, root, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Reduce);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Allreduce);

	/* Error checking */
	mpi_check_comm(comm);
	mpi_check_buf(sendbuf, comm);
	mpi_check_buf(recvbuf, comm);
	mpi_check_count(count, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);

	if(MPI_IN_PLACE == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_BUFFER, "");
	}
	if(0 == count)
	{
		return MPI_SUCCESS;
	}

	if(recvbuf != MPI_BOTTOM)
	{
		if(recvbuf == sendbuf)
		{
			MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
		}
	}


	int (*allreduce_ptr)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(allreduce_ptr, comm, allreduce);
	assume(allreduce_ptr != NULL);
	res = (allreduce_ptr)(sendbuf, recvbuf, count, datatype, op, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Allreduce);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcnts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int i;
	int size;
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Reduce_scatter);

	/* Error checking */
	mpi_check_comm(comm);
	if(MPI_IN_PLACE == recvbuf || sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}
	mpi_check_buf(sendbuf, comm);
	mpi_check_buf(recvbuf, comm);

	res = _mpc_cl_comm_size(comm, &size);
	if(res != MPI_SUCCESS)
	{
		return res;
	}

	for(i = 0; i < size; i++)
	{
		mpi_check_count(recvcnts[i], comm);
	}
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);


	int (*reduce_scatter_ptr)(const void *, void *, const int *, MPI_Datatype, MPI_Op, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(reduce_scatter_ptr, comm, reduce_scatter);
	assume(reduce_scatter_ptr != NULL);
	res = (reduce_scatter_ptr)(sendbuf, recvbuf, recvcnts, datatype, op, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Reduce_scatter);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcnt,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int size;
	int res = MPI_ERR_INTERN;

#ifndef ENABLE_COLLECTIVES_ON_INTERCOMM
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
#endif
	/* Profiling */
	SCTK_PROFIL_START(MPI_Reduce_scatter_block);

	/* Error checking */
	mpi_check_comm(comm);
	mpi_check_buf(sendbuf, comm);
	mpi_check_buf(recvbuf, comm);

	_mpc_cl_comm_size(comm, &size);

	if(MPI_IN_PLACE == recvbuf || sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}
	mpi_check_count(recvcnt, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);


	int (*reduce_scatter_block_ptr)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(reduce_scatter_block_ptr, comm, reduce_scatter_block);
	assume(reduce_scatter_block_ptr != NULL);
	res = (reduce_scatter_block_ptr)(sendbuf, recvbuf, recvcnt, datatype, op, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Reduce_scatter_block);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	/* Error checking */
	mpi_check_comm(comm);
	mpi_check_buf(sendbuf, comm);
	mpi_check_buf(recvbuf, comm);
	mpi_check_count(count, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);

	/* Invalid operation for intercommunicators */
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
	/* Profiling */
	SCTK_PROFIL_START(MPI_Scan);

	if(sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}



	int (*scan_ptr)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(scan_ptr, comm, scan);
	assume(scan_ptr != NULL);
	res = (scan_ptr)(sendbuf, recvbuf, count, datatype, op, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Scan);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm)
{
	int res = MPI_ERR_INTERN;

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}
	/* Profiling */
	SCTK_PROFIL_START(MPI_Exscan);

	if(sendbuf == recvbuf)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_ARG, "");
	}

	/* Error checking */
	mpi_check_comm(comm);
	mpi_check_buf(sendbuf, comm);
	mpi_check_buf(recvbuf, comm);
	mpi_check_count(count, comm);
	mpi_check_type(datatype, comm);
	mpi_check_type_commited(datatype, comm);
	mpi_check_op(op, datatype, comm);



	int (*exscan_ptr)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm) = NULL;
	MPC_MPI_CONFIG_ROUTE_COLL(exscan_ptr, comm, exscan);
	assume(exscan_ptr != NULL);
	res = (exscan_ptr)(sendbuf, recvbuf, count, datatype, op, comm);


	/* Profiling */
	SCTK_PROFIL_END(MPI_Exscan);
	MPI_HANDLE_RETURN_VAL(res, comm);
}

void mpc_init_mpi_request(void *request) {
        MPI_internal_request_t *req = (MPI_internal_request_t *)request;
        UNUSED(req);
        //FIXME: initialize field that need it.

        req->is_nbc = 0;
        return;
}

void __pass_mpi_request_info() {
        _mpc_cl_pass_mpi_request_info(sizeof(MPI_internal_request_t), mpc_init_mpi_request);
}

//FIXME: This is a non satisfactory hack in order to pass information from the
//       MPI module all the way to the Lowcomm module.
//       The goal is unify the request management in MPC.
void mpc_mpi_registration() __attribute__( (constructor) );

void mpc_mpi_registration()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Base Runtime Init Done", "MPI Request init", __pass_mpi_request_info, 15);

}
