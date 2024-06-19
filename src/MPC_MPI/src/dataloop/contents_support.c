/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Note: This does not exist in MPICH2.
 */

#include <stdlib.h>
#include <limits.h>

#include "./dataloop.h"

void PREPEND_PREFIX(Type_access_contents)(MPI_Datatype type,
					  int **ints_p,
					  MPI_Aint **aints_p,
					  MPI_Datatype **types_p)
{
    int mpi_errno, nr_ints, nr_aints, nr_types, combiner;

    mpi_errno = PMPI_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types,
				       &combiner);
    assert(mpi_errno == MPI_SUCCESS);
    assert(combiner != MPI_COMBINER_NAMED);

    *ints_p  = (int *) malloc(nr_ints * sizeof(int));
    *aints_p = (MPI_Aint *) malloc(nr_aints * sizeof(MPI_Aint));
    *types_p = (MPI_Datatype *) malloc(nr_types * sizeof(MPI_Datatype));

    mpi_errno = PMPI_Type_get_contents(type, nr_ints, nr_aints, nr_types,
				       *ints_p, *aints_p, *types_p);

    return;
}

void PREPEND_PREFIX(Type_release_contents)(MPI_Datatype type,
					   int **ints_p,
					   MPI_Aint **aints_p,
					   MPI_Datatype **types_p)
{
    int unused1, unused2, combiner, nr_types, i, mpi_errno;

    /* use get_envelope to grab the # of types to free */
    mpi_errno = PMPI_Type_get_envelope(type, &unused1, &unused2, &nr_types,
				       &combiner);
    assert(mpi_errno == MPI_SUCCESS);
    assert(combiner != MPI_COMBINER_NAMED);

    free(*ints_p);
    free(*aints_p);
    *ints_p  = (int *) NULL;
    *aints_p = (MPI_Aint *) NULL;

    /* derived types returned with get_contents must be freed */
    for (i=0; i < nr_types; i++) {
	int combiner, unused4;

	mpi_errno = PMPI_Type_get_envelope((*types_p)[i], &unused1, &unused2,
					   &unused4, &combiner);
	assert(mpi_errno == MPI_SUCCESS);

	if (combiner != MPI_COMBINER_NAMED) MPI_Type_free(&((*types_p)[i]));
    }

    free(*types_p);
    *types_p = (MPI_Datatype *) NULL;

    return;
}
