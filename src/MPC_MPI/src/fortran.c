#include <mpc_mpi_fortran.h>

#include <mpc_common_types.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_flags.h>
#include <mpc_common_debug.h>

#include <datatype.h>
#include <errh.h>

#include <uthash.h>

#include <mpc_lowcomm_communicator.h>
#include "session.h"

/*******************
* FORTRAN SUPPORT *
*******************/

typedef void *_fortran_handle_storage_t;

struct _mpc_fortran_handle
{
	MPI_Fint                  key;
	_fortran_handle_storage_t handle_value;
	UT_hash_handle            hh;
};

struct _mpc_handle_factory
{
	OPA_int_t                   current_handle_id;
	struct _mpc_fortran_handle *handles;
	mpc_common_spinlock_t       lock;
};

void _mpc_handle_factory_init(struct _mpc_handle_factory *fact)
{
	/* Start at 100 to avoid NULL handles */
	OPA_store_int(&fact->current_handle_id, 100);
	fact->handles = NULL;
	mpc_common_spinlock_init(&fact->lock, 0);
}

MPI_Fint _mpc_handle_factory_set(struct _mpc_handle_factory *fact, void *handle)
{
	struct _mpc_fortran_handle *h = malloc(sizeof(struct _mpc_fortran_handle) );

	h->key          = OPA_fetch_and_add_int(&fact->current_handle_id, 1);
	h->handle_value = handle;

	mpc_common_spinlock_lock(&fact->lock);
	HASH_ADD_INT(fact->handles, key, h);
	mpc_common_spinlock_unlock(&fact->lock);

	return h->key;
}

void * _mpc_handle_factory_get(struct _mpc_handle_factory *fact, MPI_Fint handle)
{
	struct _mpc_fortran_handle *h = NULL;

	mpc_common_spinlock_lock(&fact->lock);
	HASH_FIND_INT(fact->handles, &handle, h);
	mpc_common_spinlock_unlock(&fact->lock);

	if(h)
	{
		return h->handle_value;
	}
	else
	{
		return NULL;
	}
}

void _mpc_handle_factory_delete(struct _mpc_handle_factory *fact, MPI_Fint handle)
{
	struct _mpc_fortran_handle *h = NULL;

	mpc_common_spinlock_lock(&fact->lock);
	HASH_FIND_INT(fact->handles, &handle, h);
	if(h)
	{
		HASH_DEL(fact->handles, h);
		free(h);
	}
	mpc_common_spinlock_unlock(&fact->lock);
}

/**********************************
* FACTORIES FOR ALL HANDLE TYPES *
**********************************/

#if 0
static struct _mpc_handle_factory __comm_factory;
static struct _mpc_handle_factory __types_factory;
static struct _mpc_handle_factory __groups_factory;
#endif
static struct _mpc_handle_factory __requests_factory;
#if 0
static struct _mpc_handle_factory __wins_factory;
static struct _mpc_handle_factory __ops_factory;
static struct _mpc_handle_factory __infos_factory;
static struct _mpc_handle_factory __errhandlers_factory;
#endif

static void __initialize_handle_factories()
{
#if 0
	assume(sizeof(MPI_Comm) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__comm_factory);

	assume(sizeof(MPI_Datatype) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__types_factory);

	assume(sizeof(MPI_Group) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__groups_factory);

#endif
	assume(sizeof(MPI_Request) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__requests_factory);
#if 0

	assume(sizeof(MPI_Win) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__wins_factory);

	assume(sizeof(MPI_Op) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__ops_factory);

	assume(sizeof(MPI_Info) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__infos_factory);

	assume(sizeof(MPI_Errhandler) <= sizeof(_fortran_handle_storage_t) );
	_mpc_handle_factory_init(&__errhandlers_factory);
#endif
}

/********************
* SESSIONS HANDLES *
********************/

MPI_Session PMPI_Session_f2c(MPI_Fint session)
{
	return mpc_mpi_session_f2c(session);
}

MPI_Fint PMPI_Session_c2f(MPI_Session session)
{
	return mpc_mpi_session_c2f(session);
}

/*************************************
 *  MPI-2 : Fortran handle conversion *
 **************************************/

/********
* COMM *
********/

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm)
{
	return mpc_lowcomm_get_communicator_from_linear_id(comm);
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm)
{
	return mpc_lowcomm_communicator_linear_id(comm);
}

void mpc_fortran_comm_delete(__UNUSED__ MPI_Fint comm)
{
#if 0
	_mpc_handle_factory_delete(&__comm_factory, comm);
#endif
}

/*********
* TYPES *
*********/

#define MPC_DT_MAP_TO_F(datatype) \
	(MPI_Fint)(datatype->id + MPC_LOWCOMM_TYPE_COMMON_LIMIT)

#define MPC_DT_MAP_FROM_F(datatype) \
	_mpc_dt_on_slot_get(datatype - MPC_LOWCOMM_TYPE_COMMON_LIMIT)


MPI_Datatype PMPI_Type_f2c(__UNUSED__ MPI_Fint datatype)
{
#if 0
	_fortran_handle_storage_t pdatatype = NULL;
	pdatatype = _mpc_handle_factory_get(&__types_factory, datatype);

	MPI_Datatype ret = MPI_DATATYPE_NULL;
	memcpy(&ret, &pdatatype, sizeof(MPI_Datatype) );

	return ret;
#else
	if(datatype < MPC_LOWCOMM_TYPE_COMMON_LIMIT)
	{
		return (MPI_Datatype)( (long)datatype);
	}
	else if(datatype < SCTK_USER_DATA_TYPES_MAX + MPC_LOWCOMM_TYPE_COMMON_LIMIT)
	{
		return MPC_DT_MAP_FROM_F(datatype);
	}

	return (MPI_Datatype)( (long)datatype);
#endif
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype)
{
#if 0
	_fortran_handle_storage_t pdatatype = NULL;
	memcpy(&pdatatype, &datatype, sizeof(MPI_Datatype) );

	return _mpc_handle_factory_set(&__types_factory, pdatatype);
#else
	/* Exceptions */
	if(datatype == MPI_DATATYPE_NULL ||
	   datatype == MPI_LB ||
	   datatype == MPI_UB)
	{
		return (MPI_Fint)(long)datatype;
	}

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* Predefined value (stored in the id field) */
			datatype = _mpc_dt_get_datatype(datatype);
			return (MPI_Fint)datatype->id;

		case MPC_DATATYPES_USER:
			/* Map on the id for fortran index */
			return MPC_DT_MAP_TO_F(datatype);

		case MPC_DATATYPES_UNKNOWN:
			/* Invalid handle => invalid handle */
			return SCTK_USER_DATA_TYPES_MAX + MPC_LOWCOMM_TYPE_COMMON_LIMIT;
	}
	/* We should never reach this point */
	not_reachable();
#endif
}

void mpc_fortran_datatype_delete(__UNUSED__ MPI_Fint *datatype)
{
#if 0
	_mpc_handle_factory_delete(&__types_factory, datatype);
#else
	MPI_Datatype type = PMPI_Type_f2c(*datatype);
	PMPI_Type_free(&type);
	*datatype = (MPI_Fint)(long)MPI_DATATYPE_NULL;
#endif
}

/**********
* GROUPS *
**********/

MPI_Group PMPI_Group_f2c(MPI_Fint group)
{
	if( (MPI_Group)( (long)group) == MPI_GROUP_EMPTY)
	{
		return mpc_lowcomm_group_empty();
	}

	if( (MPI_Group)( (long)group) == MPI_GROUP_NULL)
	{
		return MPI_GROUP_NULL;
	}

	return mpc_lowcomm_group_from_id(group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group)
{
	if(group == MPI_GROUP_NULL ||
	   group == MPI_GROUP_EMPTY)
	{
		return (MPI_Fint)( (long)group);
	}

	return mpc_lowcomm_group_linear_id(group);
}

void mpc_fortran_group_delete(__UNUSED__ MPI_Fint *group)
{
#if 0
	_mpc_handle_factory_delete(&__groups_factory, group);
#endif
	MPI_Group c_group = PMPI_Group_f2c(*group);
	PMPI_Group_free(&c_group);
	*group = PMPI_Group_c2f(MPI_GROUP_NULL);
}

/***********
* REQUEST *
***********/

MPI_Request PMPI_Request_f2c(MPI_Fint request)
{
#if 1 
	_fortran_handle_storage_t prequest = NULL;
	prequest = _mpc_handle_factory_get(&__requests_factory, request);

	MPI_Request ret = MPI_REQUEST_NULL;
	memcpy(&ret, &prequest, sizeof(MPI_Request) );

	return ret;
#else
	return request;
#endif
}

MPI_Fint PMPI_Request_c2f(MPI_Request request)
{
#if 1
	_fortran_handle_storage_t prequest = NULL;
	memcpy(&prequest, &request, sizeof(MPI_Request) );

	return _mpc_handle_factory_set(&__requests_factory, prequest);
#else
	return request;
#endif
}

void mpc_fortran_request_delete(__UNUSED__ MPI_Fint request)
{
#if 0
	_mpc_handle_factory_delete(&__requests_factory, request);
#endif
}

/*******
* WIN *
*******/

MPI_Win PMPI_Win_f2c(MPI_Fint win)
{
#if 0
	_fortran_handle_storage_t pwin = NULL;
	pwin = _mpc_handle_factory_get(&__wins_factory, win);

	MPI_Win ret = MPI_WIN_NULL;
	memcpy(&ret, &pwin, sizeof(MPI_Win) );

	return ret;
#else
	return win;
#endif
}

MPI_Fint PMPI_Win_c2f(MPI_Win win)
{
#if 0
	_fortran_handle_storage_t pwin = NULL;
	memcpy(&pwin, &win, sizeof(MPI_Win) );

	return _mpc_handle_factory_set(&__wins_factory, pwin);
#else
	return win;
#endif
}

void mpc_fortran_win_delete(__UNUSED__ MPI_Fint win)
{
#if 0
	_mpc_handle_factory_delete(&__wins_factory, win);
#endif
}

/******
* OP *
******/

MPI_Op PMPI_Op_f2c(MPI_Fint op)
{
#if 0
	_fortran_handle_storage_t pop = NULL;
	pop = _mpc_handle_factory_get(&__ops_factory, op);

	MPI_Op ret = MPI_OP_NULL;
	memcpy(&ret, &pop, sizeof(MPI_Op) );

	return ret;
#else
	return op;
#endif
}

MPI_Fint PMPI_Op_c2f(MPI_Op op)
{
#if 0
	_fortran_handle_storage_t pop = NULL;
	memcpy(&pop, &op, sizeof(MPI_Op) );

	return _mpc_handle_factory_set(&__ops_factory, pop);
#else
	return op;
#endif
}

void mpc_fortran_op_delete(__UNUSED__ MPI_Fint op)
{
#if 0
	_mpc_handle_factory_delete(&__ops_factory, op);
#endif
}

/********
* INFO *
********/

MPI_Info PMPI_Info_f2c(MPI_Fint info)
{
#if 0
	_fortran_handle_storage_t pinfo = NULL;
	pinfo = _mpc_handle_factory_get(&__infos_factory, info);

	MPI_Info ret = MPI_INFO_NULL;
	memcpy(&ret, &pinfo, sizeof(MPI_Info) );

	return ret;
#else
	return info;
#endif
}

MPI_Fint PMPI_Info_c2f(MPI_Info info)
{
#if 0
	_fortran_handle_storage_t pinfo = NULL;
	memcpy(&pinfo, &info, sizeof(MPI_Info) );

	return _mpc_handle_factory_set(&__infos_factory, pinfo);
#else
	return info;
#endif
}

void mpc_fortran_info_delete(__UNUSED__ MPI_Fint info)
{
#if 0
	_mpc_handle_factory_delete(&__infos_factory, info);
#endif
}

/**************
* ERRHANDLER *
**************/

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler)
{
#if 0
	_fortran_handle_storage_t perrhandler = NULL;
	perrhandler = _mpc_handle_factory_get(&__errhandlers_factory, errhandler);

	MPI_Errhandler ret = MPI_INFO_NULL;
	memcpy(&ret, &perrhandler, sizeof(MPI_Errhandler) );

	return ret;
#else
	return _mpc_mpi_errhandler_from_idx(errhandler);
#endif
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
#if 0
	_fortran_handle_storage_t perrhandler = NULL;
	memcpy(&perrhandler, &errhandler, sizeof(MPI_Errhandler) );

	return _mpc_handle_factory_set(&__errhandlers_factory, perrhandler);
#else
	return (MPI_Fint)_mpc_mpi_errhandler_to_idx(errhandler);
#endif
}

void mpc_fortran_errhandler_delete(__UNUSED__ MPI_Fint errhandler)
{
#if 0
	_mpc_handle_factory_delete(&__errhandlers_factory, errhandler);
#endif
}

/***************************
* COMMON INIT AND RELEASE *
***************************/

void mpc_fortran_interface_registration() __attribute__( (constructor) );

void mpc_fortran_interface_registration()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Base Runtime Init Done", "Init Fortran Handles Factories", __initialize_handle_factories, 128);
}

/*
 * F08 Bindings
 */
#if 0
MPI_Status * mpc_f08_status_ignore()
{
	return (MPI_Status *)MPI_STATUS_IGNORE;
}

MPI_Status * mpc_f08_statuses_ignore()
{
	return (MPI_Status *)MPI_STATUSES_IGNORE;
}

void * mpc_f08_in_place()
{
	return MPI_IN_PLACE;
}

MPI_Comm mpc_f08_world()
{
	return MPI_COMM_WORLD;
}

MPI_Comm mpc_f08_self()
{
	return MPI_COMM_SELF;
}

MPI_Comm mpc_f08_null()
{
	return MPI_COMM_NULL;
}

#endif

/*****************
* WEAK BINDINGS *
*****************/

#pragma weak MPI_Comm_f2c = PMPI_Comm_f2c
#pragma weak MPI_Comm_c2f = PMPI_Comm_c2f
#pragma weak MPI_Type_f2c = PMPI_Type_f2c
#pragma weak MPI_Type_c2f = PMPI_Type_c2f
#pragma weak MPI_Group_f2c = PMPI_Group_f2c
#pragma weak MPI_Group_c2f = PMPI_Group_c2f
#pragma weak MPI_Request_f2c = PMPI_Request_f2c
#pragma weak MPI_Request_c2f = PMPI_Request_c2f
#pragma weak MPI_Win_f2c = PMPI_Win_f2c
#pragma weak MPI_Win_c2f = PMPI_Win_c2f
#pragma weak MPI_Op_f2c = PMPI_Op_f2c
#pragma weak MPI_Op_c2f = PMPI_Op_c2f
#pragma weak MPI_Info_f2c = PMPI_Info_f2c
#pragma weak MPI_Info_c2f = PMPI_Info_c2f
#pragma weak MPI_Errhandler_f2c = PMPI_Errhandler_f2c
#pragma weak MPI_Errhandler_c2f = PMPI_Errhandler_c2f
#pragma weak MPI_Session_f2c = PMPI_Session_f2c
#pragma weak MPI_Session_c2f = PMPI_Session_c2f
