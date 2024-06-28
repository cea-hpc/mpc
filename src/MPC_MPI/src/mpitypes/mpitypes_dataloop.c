/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file implements functions necessary to store dataloop representations
 * as an attribute on the MPI datatype. It also implements functionality to
 * store a size, extent, and count of contig blocks in the same way. The
 * size and extent are stored as DLOOP_Offset-sized values, so that overflow
 * isn't a factor (as it would be on 32-bit platform with 32-bit MPI_Aints).
 *
 * Additionally this code puts an attribute on MPI_COMM_SELF with a delete
 * callback to clean up keyvals and any allocated memory (although currently
 * memory cleanup relies on the delete functions on the types being called
 * as well).
 *
 * Most of these functions are used internally only. Exceptions:
 * - MPIT_Type_init(MPI_Datatype type)
 * - MPIT_Type_is_initialized(MPI_Datatype type)
 */

#include <stdio.h>
#include <stdlib.h>

#include "mpitypes.h"
#include "datatype.h"
#include "mpitypes_dataloop.h"
#include "typesize_support.h"

typedef struct MPIT_Datatype_s {
    int             valid, refct;
    int             dloop_size, dloop_depth; /* size, depth of dloop struct */
    DLOOP_Offset    size, extent; /* size and extent of type */
    DLOOP_Offset    true_lb, true_extent;
    DLOOP_Dataloop *dloop;
    DLOOP_Count     contig_blks;
} MPIT_Datatype;

/* valid flags */
#define MPIT_DATATYPE_VALID_DLOOP_PTR    1
#define MPIT_DATATYPE_VALID_DLOOP_SIZE   2
#define MPIT_DATATYPE_VALID_DLOOP_DEPTH  4
#define MPIT_DATATYPE_VALID_TYPESZEXT    8
#define MPIT_DATATYPE_VALID_CONTIG_BLKS 16

#define MPIT_DATATYPE_VALID_DATALOOP (MPIT_DATATYPE_VALID_DLOOP_PTR | \
                                      MPIT_DATATYPE_VALID_DLOOP_SIZE | \
                                      MPIT_DATATYPE_VALID_DLOOP_DEPTH)

/* MPI attr keyvals used internally */
static MPIT_Datatype *MPIT_Datatype_allocate();
static void MPIT_Datatype_set_szext(MPI_Datatype type, MPIT_Datatype *dtp);

/* MPIT_Type_init
 *
 * Must be called before dataloop is accessed.
 *
 * Note: It is valid to call more than once on the same type; we simply
 *       don't do anything on subsequent calls.
 *
 * Returns MPI_SUCCESS.
 */
int MPIT_Type_init(MPI_Datatype type)
{
    MPIT_Datatype *dtp;

    /* trivial types don't get dataloops */
    if (!(MPIT_Datatype_is_nontrivial(type))) return MPI_SUCCESS;

    dtp = _mpc_dt_get_mpitypes_handle(type); 

    if (!dtp) {
	/* allocate structure and create dataloop representation */
	dtp = MPIT_Datatype_allocate();
    }
    if (!(dtp->valid & MPIT_DATATYPE_VALID_DLOOP_PTR)) {
	MPIT_Dataloop_create(type,
			     &dtp->dloop,
			     &dtp->dloop_size,
			     &dtp->dloop_depth,
			     0);

	dtp->valid |= MPIT_DATATYPE_VALID_DLOOP_PTR |
	    MPIT_DATATYPE_VALID_DLOOP_SIZE | MPIT_DATATYPE_VALID_DLOOP_DEPTH;
    }

    _mpc_dt_set_mpitypes_handle(type, (void *)dtp);

    return MPI_SUCCESS;
}

/* MPIT_Type_is_initialized
 *
 * Note that "trivial" types are always considered initialized. 
 *
 * Returns 0 if type has not been initialized for MPITypes, non-zero
 * otherwise.
 */
int MPIT_Type_is_initialized(MPI_Datatype type)
{
    MPIT_Datatype *dtp;

    /* trivial types are always considered "initialized" */
    if (!MPIT_Datatype_is_nontrivial(type)) return 1;

    dtp = _mpc_dt_get_mpitypes_handle(type);
    if (!dtp) return 0;
    else return 1;
}

void MPIT_Datatype_get_size(MPI_Datatype type, DLOOP_Offset *size_p)
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!dtp) {
	dtp = MPIT_Datatype_allocate();
    }

    if (!(dtp->valid & MPIT_DATATYPE_VALID_TYPESZEXT)) {
	MPIT_Datatype_set_szext(type, dtp);
    }

    *size_p = dtp->size;
    return;
}

void MPIT_Datatype_get_extent(MPI_Datatype type, DLOOP_Offset *extent_p)
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);
    
    if (!dtp) {
	dtp = MPIT_Datatype_allocate();
    }

    if (!(dtp->valid & MPIT_DATATYPE_VALID_TYPESZEXT)) {
	MPIT_Datatype_set_szext(type, dtp);
    }

    *extent_p = dtp->extent;
    return;
}

/* MPIT_Datatype_get_block_info
 *
 * Parameters:
 * type     - MPI datatype
 * true_lb  - true_lb for type (offset to start of data)
 * count    - count of # of contiguous regions in the type
 * n_contig - flag, indicating if N of these types would also form a 
 *            single contiguous block
 */
void MPIT_Datatype_get_block_info(MPI_Datatype type,
				  DLOOP_Offset *true_lb_p,
				  DLOOP_Offset *count_p,
				  int *n_contig_p)
{
    int mpi_errno;
    int nr_ints, nr_aints, nr_types, combiner;

    mpi_errno = PMPI_Type_get_envelope(type, &nr_ints, &nr_aints,
				       &nr_types, &combiner);
    DLOOP_Assert(mpi_errno == MPI_SUCCESS);

    if (combiner == MPI_COMBINER_NAMED &&
	(type != MPI_FLOAT_INT &&
	 type != MPI_DOUBLE_INT &&
	 type != MPI_LONG_INT &&
	 type != MPI_SHORT_INT &&
	 type != MPI_LONG_DOUBLE_INT))
    {
	*true_lb_p  = 0;
	*count_p    = 1;
	*n_contig_p = 1;
    }
    else {
	MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);
	MPIT_Segment  *segp;
	DLOOP_Offset     bytes;

	if (!dtp) {
	    /* need to allocate structure and create dataloop representation */
	    dtp = MPIT_Datatype_allocate();
	}
	if (!(dtp->valid & MPIT_DATATYPE_VALID_DLOOP_PTR)) {
	    MPIT_Dataloop_create(type,
				 &dtp->dloop,
				 &dtp->dloop_size,
				 &dtp->dloop_depth, 0);
	    DLOOP_Assert(dtp->dloop != NULL);
	    dtp->valid |= MPIT_DATATYPE_VALID_DLOOP_PTR |
		MPIT_DATATYPE_VALID_DLOOP_SIZE |
		MPIT_DATATYPE_VALID_DLOOP_DEPTH;

	}
	    
	DLOOP_Assert((dtp->valid & MPIT_DATATYPE_VALID_DLOOP_PTR) &&
		     (dtp->valid & MPIT_DATATYPE_VALID_DLOOP_SIZE) &&
		     (dtp->valid & MPIT_DATATYPE_VALID_DLOOP_DEPTH));

	segp = MPIT_Segment_alloc();
	DLOOP_Assert(segp != NULL);

	MPIT_Segment_init(NULL, 1, type, segp, 0);
	bytes = SEGMENT_IGNORE_LAST;

	MPIT_Segment_count_contig_blocks(segp, 0, &bytes, &dtp->contig_blks);
	MPIT_Segment_free(segp);
	dtp->valid |= MPIT_DATATYPE_VALID_CONTIG_BLKS;

	if (!(dtp->valid & MPIT_DATATYPE_VALID_TYPESZEXT)) {
	    MPIT_Datatype_set_szext(type, dtp);
	}
	*true_lb_p  = dtp->true_lb;
	*count_p    = dtp->contig_blks;
	*n_contig_p = (dtp->contig_blks == 1 &&
		       dtp->true_extent == dtp->extent) ? 1 : 0;
    }

    return;
}

void MPIT_Datatype_get_el_type(MPI_Datatype type,
			       MPI_Datatype *eltype_p,
			       int flag)
{
    int mpi_errno;
    int nr_ints, nr_aints, nr_types, combiner;

    mpi_errno = PMPI_Type_get_envelope(type, &nr_ints, &nr_aints,
				       &nr_types, &combiner);
    DLOOP_Assert(mpi_errno == MPI_SUCCESS);

    if (combiner == MPI_COMBINER_NAMED) {
	if (type == MPI_FLOAT_INT ||
	    type == MPI_DOUBLE_INT ||
	    type == MPI_LONG_INT ||
	    type == MPI_SHORT_INT ||
	    type == MPI_LONG_DOUBLE_INT)
	{
	    *eltype_p = MPI_DATATYPE_NULL;
	}
	else if (type == MPI_2INT) {
	    *eltype_p = MPI_INT;
	}
	else {
	    /* all the other named types are their own element type */
	    *eltype_p = type;
	}
    }
    else {
	MPIT_Dataloop *dlp;
	MPIT_Datatype_get_loopptr(type, &dlp, flag);

	*eltype_p = dlp->el_type;
    }
    return;
}

/* dataloop-related functions used by dataloop code */
void MPIT_Datatype_get_loopptr(MPI_Datatype type,
			       MPIT_Dataloop **ptr_p,
			       int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    /* in the init dataloop path, we create the datatype attribute if it does
     * not exist.  why do we assume the attribute is already set?  We hit this
     * assertion when processing subarray types: after converting the subarray,
     * the dataloop_create call operates on a type (tmptype) w/o any attributes
     * -- RobL?
     */

    /* When building a dataloop representation of a type, we will hit sub-types
     * for which the MPI_Datatype structure has not been allocated. When we do,
     * we just return an NULL pointer.
     */
    if (!dtp || !(dtp->valid & MPIT_DATATYPE_VALID_DLOOP_PTR))
	*ptr_p = NULL;
    else 
	*ptr_p = dtp->dloop;

    return;
}

void MPIT_Datatype_get_loopsize(MPI_Datatype type, int *size_p, int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!(dtp->valid & MPIT_DATATYPE_VALID_DLOOP_SIZE))
	*size_p = -1;
    else
	*size_p = dtp->dloop_size;

    return;
}

void MPIT_Datatype_get_loopdepth(MPI_Datatype type, int *depth_p, int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!(dtp->valid & MPIT_DATATYPE_VALID_DLOOP_DEPTH))
	*depth_p = -1;
    else
	*depth_p = dtp->dloop_depth;

    return;
}

void MPIT_Datatype_set_loopptr(MPI_Datatype type, MPIT_Dataloop *ptr, int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!dtp) {
	dtp = MPIT_Datatype_allocate();
    }

    dtp->dloop  = ptr;
    dtp->valid |= MPIT_DATATYPE_VALID_DLOOP_PTR;
    return;
}

void MPIT_Datatype_set_loopsize(MPI_Datatype type, int size, int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!dtp) {
	dtp = MPIT_Datatype_allocate();
    }

    dtp->dloop_size  = size;
    dtp->valid      |= MPIT_DATATYPE_VALID_DLOOP_SIZE;
    return;
}

void MPIT_Datatype_set_loopdepth(MPI_Datatype type, int depth, int UNUSED_(flag))
{
    MPIT_Datatype *dtp = _mpc_dt_get_mpitypes_handle(type);

    if (!dtp) {
	dtp = MPIT_Datatype_allocate();
    }

    dtp->dloop_depth  = depth;
    dtp->valid       |= MPIT_DATATYPE_VALID_DLOOP_DEPTH;
    return;
}

int MPIT_Datatype_is_nontrivial(MPI_Datatype type)
{
    int nr_ints, nr_aints, nr_types, combiner;
    int float_int = MPI_FLOAT_INT;
    int double_int = MPI_DOUBLE_INT;
    int long_int = MPI_LONG_INT;
    int short_int = MPI_SHORT_INT;
    int long_double_int = MPI_LONG_DOUBLE_INT;
    int combiner_name = MPI_COMBINER_NAMED;
    
    PMPI_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);
    if (combiner != MPI_COMBINER_NAMED ||
	type == MPI_FLOAT_INT ||
	type == MPI_DOUBLE_INT ||
	type == MPI_LONG_INT ||
	type == MPI_SHORT_INT ||
	type == MPI_LONG_DOUBLE_INT) {
            printf("1");
            return 1;
    }
    else {
            printf("0");
            return 0;
    }
}

static MPIT_Datatype *MPIT_Datatype_allocate()
{
    MPIT_Datatype *dtp;

    dtp = (MPIT_Datatype *) malloc(sizeof(MPIT_Datatype));
    DLOOP_Assert(dtp != NULL);
    dtp->valid       = 0;
    dtp->refct       = 1;
    dtp->dloop       = NULL;
    dtp->dloop_size  = -1;
    dtp->dloop_depth = -1;

    return dtp;
}

/* MPIT_Datatype_set_szext()
 *
 * Calculates size, extent, true_lb, and true_extent of type, fills in values
 * in MPIT_Datatype structure, and sets valid flag.
 *
 * Note: This code currently checks for compatible variable sizes at
 *       runtime, while this check could instead be performed at configure
 *       time to save a few instructions. This seems like micro-optimization,
 *       so I skipped it for now. -- RobR, 03/22/2007
 */
static void MPIT_Datatype_set_szext(MPI_Datatype type, MPIT_Datatype *dtp)
{
    int mpi_errno;

    if (sizeof(int) >= sizeof(DLOOP_Offset) &&
	sizeof(MPI_Aint) >= sizeof(DLOOP_Offset))
    {
	int size;
	MPI_Aint lb, extent, true_lb, true_extent;
	
	mpi_errno = PMPI_Type_size(type, &size);
	DLOOP_Assert(mpi_errno == MPI_SUCCESS);
	
	mpi_errno = PMPI_Type_get_extent(type, &lb, &extent);
	DLOOP_Assert(mpi_errno == MPI_SUCCESS);
	
	mpi_errno = PMPI_Type_get_true_extent(type, &true_lb, &true_extent); 

	dtp->size        = (DLOOP_Offset) size;
	dtp->extent      = (DLOOP_Offset) extent;
	dtp->true_lb     = (DLOOP_Offset) true_lb;
	dtp->true_extent = (DLOOP_Offset) true_extent;
    }
    else {
	MPIT_Type_footprint tfp;
	
	MPIT_Type_calc_footprint(type, &tfp);
	dtp->size        = tfp.size;
	dtp->extent      = tfp.extent;
	dtp->true_lb     = tfp.true_lb;
	dtp->true_extent = tfp.true_ub - tfp.true_lb;
    }

    dtp->valid |= MPIT_DATATYPE_VALID_TYPESZEXT;
    return;
}

/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
