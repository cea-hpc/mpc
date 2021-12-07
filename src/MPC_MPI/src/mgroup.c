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
#include <mpc_common_debug.h>

#include "mpc_mpi.h"
#include "mpc_mpi_internal.h"

#include <mpc_config.h>

#include <mpc_common_helper.h>

#ifdef MPC_Threads
#include <mpc_thread.h>
#endif

/******************
* WEAK FUNCTIONS *
******************/

#pragma weak MPI_Group_size = PMPI_Group_size
#pragma weak MPI_Group_rank = PMPI_Group_rank
#pragma weak MPI_Group_translate_ranks = PMPI_Group_translate_ranks
#pragma weak MPI_Group_compare = PMPI_Group_compare
#pragma weak MPI_Group_union = PMPI_Group_union
#pragma weak MPI_Group_intersection = PMPI_Group_intersection
#pragma weak MPI_Group_difference = PMPI_Group_difference
#pragma weak MPI_Group_incl = PMPI_Group_incl
#pragma weak MPI_Group_excl = PMPI_Group_excl
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#pragma weak MPI_Group_range_excl = PMPI_Group_range_excl
#pragma weak MPI_Group_free = PMPI_Group_free
#pragma weak MPI_Comm_group = PMPI_Comm_group
#pragma weak MPI_Comm_remote_group = PMPI_Comm_remote_group
#pragma weak MPI_Comm_create_group = PMPI_Comm_create_group
#pragma weak MPI_Comm_create_from_group = PMPI_Comm_create_from_group

/*****************************************
* GROUP-BASED COMMUNICATOR CONSTRUCTORS *
*****************************************/

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "PMPI_Comm_create_group not possible on intercoms");
	}

	*newcomm = MPC_COMM_NULL;

	if(mpc_lowcomm_communicator_create_group(comm, group, tag, newcomm) != MPC_LOWCOMM_SUCCESS)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_OTHER, "PMPI_Comm_create_group failed");
	}

	_mpc_cl_attach_per_comm(comm, *newcomm);

	MPI_ERROR_SUCCESS();
}

int PMPI_Comm_create_from_group(MPI_Group group, const char *stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newcomm)
{
	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	if(group == MPI_GROUP_EMPTY)
	{
		*newcomm = MPI_COMM_NULL;
		return MPI_SUCCESS;
	}

	uint64_t hash  = mpc_common_hash_string(stringtag);
	int      ihash = (int)hash & 0xFFFFFF;

	int res = PMPI_Comm_create_group(MPI_COMM_WORLD, group, ihash, newcomm);
	MPI_HANDLE_ERROR(res, MPI_COMM_SELF, "Failed creating the comm group");

	if( (errhandler != MPI_ERRHANDLER_NULL) && (*newcomm != MPI_COMM_NULL) )
	{
		res = PMPI_Comm_set_errhandler(*newcomm, errhandler);
		MPI_HANDLE_ERROR(res, *newcomm, "Failed attaching the comm errhandler");
	}

	/* NOTE: this is the only moment multiple contexts from
	   various places can join on a given communicator this is where
	   we "unify" these contextes to share their ID and parameters */
	mpc_lowcomm_handle_ctx_t gctx = mpc_lowcomm_group_get_context_pointer(group);

    /* First Make sure all handles have the same value */
	if(*newcomm != MPI_COMM_NULL)
	{
		mpc_lowcomm_communicator_id_t roots_id = MPC_LOWCOMM_COMM_NULL_ID;

		if(mpc_lowcomm_communicator_rank(*newcomm) == 0)
		{
			roots_id = mpc_lowcomm_communicator_handle_ctx_id(gctx);
		}

		mpc_lowcomm_bcast(&roots_id, sizeof(mpc_lowcomm_communicator_id_t), 0, *newcomm);

		if( roots_id != mpc_lowcomm_communicator_handle_ctx_id(gctx) )
		{
			MPI_ERROR_REPORT(*newcomm, MPI_ERR_ARG, "Handles from different sessions called PMPI_Comm_create_from_group");
		}

		if(gctx)
		{
			res = mpc_lowcomm_communicator_handle_ctx_unify(*newcomm, gctx);
			if( res != MPC_LOWCOMM_SUCCESS)
			{
				MPI_ERROR_REPORT(*newcomm, MPI_ERR_COMM, "Incoherent parameters on Session info");
			}
		}

	}


	MPI_ERROR_SUCCESS();
}

/************************
* GROUP IMPLEMENTATION *
************************/

//#define DEBUG_GROUP_RANGES

int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *mpi_group)
{
	mpc_common_nodebug("Enter Comm_remote_group");
	int res = MPI_ERR_INTERN;
	mpi_check_comm(comm);
	if(mpc_lowcomm_communicator_is_intercomm(comm) == 0)
	{
		MPI_ERROR_REPORT(comm, MPI_ERR_COMM, "");
	}

	mpc_lowcomm_communicator_t remote = mpc_lowcomm_communicator_get_remote(comm);

	*mpi_group = mpc_lowcomm_communicator_group(remote);

	MPI_ERROR_SUCCESS();
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group *mpi_group)
{
	//mpc_lowcomm_communicator_print(comm, 1);

	mpc_common_nodebug("Enter Comm_group");
	mpi_check_comm(comm);

	*mpi_group = mpc_lowcomm_communicator_group(comm);

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_free(MPI_Group *group)
{
	if(*group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	if(*group == MPI_GROUP_EMPTY)
	{
		return MPI_SUCCESS;
	}

	int ret = mpc_lowcomm_group_free(group);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not free group");
	}

	*group = MPI_GROUP_NULL;

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
	if( (group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	mpc_lowcomm_group_eq_e res = mpc_lowcomm_group_compare(group1, group2);

	*result = res;

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[])
{
	if(n < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Negative n to PMPI_Group_translate_ranks");
	}

	if( (group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	int i;

	for(i = 0; i < n; i++)
	{
		if( (ranks1[i] < 0) || (mpc_lowcomm_group_size(group1) <= ranks1[i]) )
		{
			MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_RANK, "Invalid rank passed to PMPI_Group_translate_ranks");
		}
	}

	int ret = mpc_lowcomm_group_translate_ranks(group1, n, ranks1, group2, ranks2);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not tranlate ranks");
	}

	MPI_ERROR_SUCCESS();
}

static inline int __check_ranks_arrays(MPI_Group group, int n, const int ranks[])
{
	int size;

	PMPI_Group_size(group, &size);

	int i, j;

	for(i = 0; i < n; i++)
	{
		/* In group */
		if( (ranks[i] < 0) || (size <= ranks[i]) )
		{
			mpc_common_debug_error("rank[%d] == %d", i, ranks[i]);
			return MPI_ERR_RANK;
		}

		for(j = 0; j < n; j++)
		{
			if(i == j)
			{
				continue;
			}

			/* Not duplicate */
			if(ranks[i] == ranks[j])
			{
				mpc_common_debug_error("rank[%d] == ranks[%d] == %d", i, j, ranks[i]);
				return MPI_ERR_RANK;
			}
		}
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup)
{
	if(n < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Negative n to PMPI_Group_excl");
	}

	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Invalid group in PMPI_Group_excl");
	}

	MPI_HANDLE_ERROR(__check_ranks_arrays(group, n, ranks), MPI_COMM_WORLD, "Invalid Ranks");

#ifdef DEBUG_GROUP_RANGES
	int i;

	for(i = 0; i < n; i++)
	{
		mpc_common_debug_warning("RANKS[%d] = %d", i, ranks[i]);
	}
#endif

	*newgroup = mpc_lowcomm_group_excl(group, n, ranks);

	if(*newgroup == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not exclude ranks");
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup)
{
	if(n < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Negative n to PMPI_Group_incl");
	}

	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Invalid group in PMPI_Group_incl");
	}

	MPI_HANDLE_ERROR(__check_ranks_arrays(group, n, ranks), MPI_COMM_WORLD, "Invalid Ranks");

#ifdef DEBUG_GROUP_RANGES
	int i;

	for(i = 0; i < n; i++)
	{
		mpc_common_debug_warning("RANKS[%d] = %d", i, ranks[i]);
	}
#endif

	*newgroup = mpc_lowcomm_group_incl(group, n, ranks);

	if(*newgroup == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not include ranks");
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	*rank = mpc_lowcomm_group_rank(group);

	if(*rank == MPC_PROC_NULL)
	{
		*rank = MPI_UNDEFINED;
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_size(MPI_Group group, int *size)
{
	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	*size = mpc_lowcomm_group_size(group);
	MPI_ERROR_SUCCESS();
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
	if( (group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	*newgroup = mpc_lowcomm_group_difference(group1, group2);

	if(*newgroup == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not substract ranks");
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
	if( (group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	*newgroup = mpc_lowcomm_group_instersection(group1, group2);

	if(*newgroup == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not instersect ranks");
	}

	MPI_ERROR_SUCCESS();
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
	if( (group1 == MPI_GROUP_NULL) || (group2 == MPI_GROUP_NULL) )
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

	*newgroup = mpc_lowcomm_group_union(group1, group2);

	if(*newgroup == NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "Could not instersect ranks");
	}

	MPI_ERROR_SUCCESS();
}

int __check_ranges(MPI_Group group, int n, int ranges[][3])
{
	int i;

	int size;

	PMPI_Group_size(group, &size);


	for(i = 0; i < n; i++)
	{
		int j;

		if(ranges[i][2] == 0)
		{
			/* Null stride */
			return MPI_ERR_ARG;
		}


		if(ranges[i][2] < 0)
		{
			/* Neg Stride */
			if(ranges[i][0] < ranges[i][1])
			{
				return MPI_ERR_ARG;
			}
		}
		else
		{
			/* Pos Stride */
			if(ranges[i][0] > ranges[i][1])
			{
				return MPI_ERR_ARG;
			}
		}

		for(j = 0; j < 2; j++)
		{
			if( (ranges[i][j] < 0) || (size <= ranges[i][j]) )
			{
				return MPI_ERR_ARG;
			}
		}
	}

	MPI_ERROR_SUCCESS();
}

int *__linearize_ranges(int n, int ranges[][3], int *new_size)
{
	unsigned int max_array_size = 0;

	int i;

	for(i = 0; i < n; i++)
	{
		if(ranges[i][2] < 0)
		{
			max_array_size += ranges[i][0] - ranges[i][1] + 1;
		}
		else
		{
			max_array_size += ranges[i][1] - ranges[i][0] + 1;
		}
	}

	int *ret = sctk_malloc(max_array_size * sizeof(int) );
	assume(ret != NULL);

	int cnt = 0;

	for(i = 0; i < n; i++)
	{
		int j;

		if(ranges[i][2] < 0)
		{
			for(j = ranges[i][0]; j >= ranges[i][1]; j = j + ranges[i][2])
			{
				ret[cnt] = j;
				cnt++;
				assume(cnt <= max_array_size);
			}
		}
		else
		{
			for(j = ranges[i][0]; j <= ranges[i][1]; j = j + ranges[i][2])
			{
				ret[cnt] = j;
				cnt++;
				assume(cnt <= max_array_size);
			}
		}
	}

	*new_size = cnt;

	return ret;
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
	if(n < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Negative n to PMPI_Group_range_excl");
	}

	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

#ifdef DEBUG_GROUP_RANGES
	int i;

	for(i = 0; i < n; i++)
	{
		mpc_common_debug_warning("RANGES[%d] = [%d , %d , %d]", i, ranges[i][0], ranges[i][1], ranges[i][2]);
	}
#endif

	MPI_HANDLE_ERROR(__check_ranges(group, n, ranges), MPI_COMM_WORLD, "Invalid ranges");

	int new_size = 0;

	int *ranks = __linearize_ranges(n, ranges, &new_size);

	int res = PMPI_Group_excl(group, new_size, ranks, newgroup);

	sctk_free(ranks);

	MPI_HANDLE_RETURN_VAL(res, MPI_COMM_WORLD);
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
	if(n < 0)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_ARG, "Negative n to PMPI_Group_range_incl");
	}

	if(group == MPI_GROUP_NULL)
	{
		MPI_ERROR_REPORT(MPI_COMM_WORLD, MPI_ERR_GROUP, "");
	}

#ifdef DEBUG_GROUP_RANGES
	int i;

	for(i = 0; i < n; i++)
	{
		mpc_common_debug_warning("RANGES[%d] = [%d , %d , %d]", i, ranges[i][0], ranges[i][1], ranges[i][2]);
	}
#endif

	MPI_HANDLE_ERROR(__check_ranges(group, n, ranges), MPI_COMM_WORLD, "Invalid ranges");

	int new_size = 0;

	int *ranks = __linearize_ranges(n, ranges, &new_size);

	int res = PMPI_Group_incl(group, new_size, ranks, newgroup);

	sctk_free(ranks);

	MPI_HANDLE_RETURN_VAL(res, MPI_COMM_WORLD);
}
