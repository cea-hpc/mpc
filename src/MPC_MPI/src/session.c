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
#include "session.h"

#include "mpc_mpi_internal.h"
#include <mpc_common_debug.h>
#include <mpc_lowcomm_group.h>


#include <sctk_alloc.h>

/****************
* WEAK SYMBOLS *
****************/

#pragma weak MPI_Session_create_errhandler = PMPI_Session_create_errhandler
#pragma weak MPI_Session_set_errhandler = PMPI_Session_set_errhandler
#pragma weak MPI_Session_get_errhandler = PMPI_Session_get_errhandler
#pragma weak MPI_Session_call_errhandler = PMPI_Session_call_errhandler
#pragma weak MPI_Session_init = PMPI_Session_init
#pragma weak MPI_Session_finalize = PMPI_Session_finalize
#pragma weak MPI_Session_get_info = PMPI_Session_get_info
#pragma weak MPI_Session_get_num_psets = PMPI_Session_get_num_psets
#pragma weak MPI_Session_get_nth_pset = PMPI_Session_get_nth_pset
#pragma weak MPI_Session_get_pset_info = PMPI_Session_get_pset_info
#pragma weak MPI_Group_from_session_pset = PMPI_Group_from_session_pset

/******************
* ERROR HANDLING *
******************/

int PMPI_Session_create_errhandler(MPI_Session_errhandler_function *session_errhandler_fn,
                                   MPI_Errhandler *errhandler)
{
	*errhandler = MPI_ERRHANDLER_NULL;

	if(session_errhandler_fn == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Cannot pass null errhandler");
	}

	_mpc_mpi_errhandler_register( (_mpc_mpi_generic_errhandler_func_t)session_errhandler_fn, errhandler);

	assume(*errhandler != MPI_ERRHANDLER_NULL);

	MPI_ERROR_SUCESS();
}

int PMPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler)
{
	if(session == MPI_SESSION_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Cannot set errhandler to null session");
	}

	session->errh = errhandler;

	MPI_ERROR_SUCESS();
}

int PMPI_Session_get_errhandler(MPI_Session session, MPI_Errhandler *errhandler)
{
	if(session == MPI_SESSION_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Cannot set errhandler to null session");
	}

	*errhandler = session->errh;

	MPI_ERROR_SUCESS();
}

int PMPI_Session_call_errhandler(MPI_Session session, int error_code)
{
	if(session == MPI_SESSION_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Cannot set errhandler to null session");
	}

	if(session->errh == MPI_ERRHANDLER_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Cannot call NULL errhandler");
	}

	_mpc_mpi_generic_errhandler_func_t errh = _mpc_mpi_errhandler_resolve(session->errh);

	if(errh == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Failed to resolve errhandler");
	}

	( (MPI_Session_errhandler_function)errh)(&session, &error_code);

	MPI_ERROR_SUCESS();
}

/*********************
* INIT AND FINALIZE *
*********************/

struct mpc_mpi_session_s *__session_new(void)
{
	struct mpc_mpi_session_s *ret = sctk_malloc(sizeof(struct mpc_mpi_session_s) );

	memset(ret, 0, sizeof(struct mpc_mpi_session_s) );
	return ret;
}

void __session_free(struct mpc_mpi_session_s *session)
{
	sctk_free(session);
}

int PMPI_Session_init(MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session)
{
	int res = MPI_SUCCESS;

	*session = __session_new();

	if(info == MPI_INFO_NULL)
	{
		/* Store empty info */
		res = MPI_Info_create(&(*session)->infos);
	}
	else
	{
		res = PMPI_Info_dup(info, &(*session)->infos);
	}

	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "Could not create session info object");

	/* Now as MPC is always thread multiple set the thread_support level accordingly */
	PMPI_Info_set( (*session)->infos, "mpi_thread_support_level", "MPI_THREAD_MULTIPLE");

	if(res != MPI_SUCCESS)
	{
		/* Call errhandler */
		PMPI_Session_call_errhandler(*session, res);
		return res;
	}

	return res;
}

int PMPI_Session_finalize(MPI_Session *session)
{
	int res = MPI_SUCCESS;

	res = PMPI_Info_free(&(*session)->infos);

	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "Could not free session info object");

	__session_free(*session);

	*session = MPI_SESSION_NULL;

	MPI_ERROR_SUCESS();
}

int PMPI_Session_get_info(MPI_Session session, MPI_Info *info_used)
{
	return PMPI_Info_dup(session->infos, info_used);
}

int PMPI_Session_get_num_psets(MPI_Session session, MPI_Info info, int *npset_names)
{
	*npset_names = mpc_lowcomm_group_pset_count();
	MPI_ERROR_SUCESS();
}

int PMPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n, int *pset_len, char *pset_name)
{
	mpc_lowcomm_process_set_t *pset = mpc_lowcomm_group_pset_get_nth(n);

	if(!pset)
	{
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Could not retrieve this pset at rank n");
	}

	if(*pset_len)
	{
		snprintf(pset_name, *pset_len, pset->name);
	}

	*pset_len = strlen(pset->name);

	mpc_lowcomm_group_pset_free(pset);

	MPI_ERROR_SUCESS();
}

int PMPI_Session_get_pset_info(MPI_Session session, const char *pset_name, MPI_Info *info)
{
	mpc_lowcomm_process_set_t *pset = mpc_lowcomm_group_pset_get_by_name(pset_name);

	if(!pset)
	{
		mpc_common_debug_error("No such PSET %s", pset_name);
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Could not retrieve this pset");
	}

	int res = PMPI_Info_create(info);

	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "Could not create pset info object");

	char smpi_size[64];
	char sname[128];

	snprintf(smpi_size, 64, "%d", mpc_lowcomm_group_size(pset->group));
	snprintf(sname, 128, pset->name);

	PMPI_Info_set(*info, "mpi_size", smpi_size);
	PMPI_Info_set(*info, "name", sname);

	mpc_lowcomm_group_pset_free(pset);

	MPI_ERROR_SUCESS();
}

int PMPI_Group_from_session_pset(MPI_Session session, const char *pset_name, MPI_Group *newgroup)
{
	mpc_lowcomm_process_set_t *pset = mpc_lowcomm_group_pset_get_by_name(pset_name);

	if(!pset)
	{
		mpc_common_debug_error("No such PSET %s", pset_name);
		MPI_ERROR_REPORT(MPI_COMM_SELF, MPI_ERR_ARG, "Could not retrieve this pset");
	}

    *newgroup = mpc_lowcomm_group_dup(pset->group);

	mpc_lowcomm_group_pset_free(pset);

	MPI_ERROR_SUCESS();
}
