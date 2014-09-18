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
#include "mpi_array_types.h"
#include "mpc_common.h"

/* Content of this file is adapted from ROMIO:
 * Copyright (C) 1997 University of Chicago. 
                               COPYRIGHT

The following is a notice of limited availability of the code and
disclaimer, which must be included in the prologue of the code and in
all source listings of the code.

Copyright (C) 1997 University of Chicago

Permission is hereby granted to use, reproduce, prepare derivative
works, and to redistribute to others. 

The University of Chicago makes no representations as to the suitability, 
operability, accuracy, or correctness of this software for any purpose. 
It is provided "as is" without express or implied warranty. 

This software was authored by:
Rajeev Thakur: (630) 252-1682; thakur@mcs.anl.gov
Mathematics and Computer Science Division
Argonne National Laboratory, Argonne IL 60439, USA


                           GOVERNMENT LICENSE

Portions of this material resulted from work developed under a U.S.
Government Contract and are subject to the following license: the
Government is granted for itself and others acting on its behalf a
paid-up, nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform publicly
and display publicly. 

                              DISCLAIMER

This computer code material was prepared, in part, as an account of
work sponsored by an agency of the United States Government.  Neither
the United States Government, nor the University of Chicago, nor any
of their employees, makes any warranty express or implied, or assumes
any legal liability or responsibility for the accuracy, completeness,
or usefulness of any information, apparatus, product, or process
disclosed, or represents that its use would not infringe privately
owned rights.  

 */

/************************************************************************/
/* DARRAY IMPLEMENTATION                                                */
/************************************************************************/


static int sctk_MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims, int nprocs,
		     int rank, int darg, int order, MPI_Aint orig_extent,
		     MPI_Datatype type_old, MPI_Datatype *type_new,
		     MPI_Aint *st_offset);

static int sctk_MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset);


int sctk_Type_create_darray(int size, int rank, int ndims, 
			    int *array_of_gsizes, int *array_of_distribs, 
			    int *array_of_dargs, int *array_of_psizes, 
			    int order, MPI_Datatype oldtype, 
			    MPI_Datatype *newtype) 
{
    MPI_Datatype type_old, type_new=MPI_DATATYPE_NULL, types[3];
    int procs, tmp_rank, i, tmp_size, blklens[3], *coords;
    MPI_Aint *st_offsets, orig_extent, disps[3];

    MPI_Type_extent(oldtype, &orig_extent);

/* calculate position in Cartesian grid as MPI would (row-major
   ordering) */
    coords = (int *) sctk_malloc(ndims*sizeof(int));
    procs = size;
    tmp_rank = rank;
    for (i=0; i<ndims; i++) {
	procs = procs/array_of_psizes[i];
	coords[i] = tmp_rank/procs;
	tmp_rank = tmp_rank % procs;
    }

    st_offsets = (MPI_Aint *) sctk_malloc(ndims*sizeof(MPI_Aint));
    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
      /* dimension 0 changes fastest */
	for (i=0; i<ndims; i++) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims,
				 array_of_psizes[i],
				 coords[i], array_of_dargs[i],
				 order, orig_extent, 
				 type_old, &type_new,
				 st_offsets+i); 
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims, 
				  array_of_psizes[i], coords[i],
				  array_of_dargs[i], order,
				  orig_extent, type_old,
				  &type_new, st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, 1, 0, 
				 MPI_DISTRIBUTE_DFLT_DARG, order,
				 orig_extent, 
				 type_old, &type_new,
				 st_offsets+i); 
		break;
	    }
	    if (i) MPI_Type_free(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[0];
	tmp_size = 1;
	for (i=1; i<ndims; i++) {
	    tmp_size *= array_of_gsizes[i-1];
	    disps[1] += (MPI_Aint)tmp_size*st_offsets[i];
	}
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
        /* dimension ndims-1 changes fastest */
	for (i=ndims-1; i>=0; i--) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
				 coords[i], array_of_dargs[i], order,
				 orig_extent, type_old, &type_new,
				 st_offsets+i); 
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		sctk_MPIOI_Type_cyclic(array_of_gsizes, i, ndims, 
				  array_of_psizes[i], coords[i],
				  array_of_dargs[i], order, 
				  orig_extent, type_old, &type_new,
				  st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		sctk_MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
		      coords[i], MPI_DISTRIBUTE_DFLT_DARG, order, orig_extent, 
                           type_old, &type_new, st_offsets+i); 
		break;
	    }
	    if (i != ndims-1) MPI_Type_free(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[ndims-1];
	tmp_size = 1;
	for (i=ndims-2; i>=0; i--) {
	    tmp_size *= array_of_gsizes[i+1];
	    disps[1] += (MPI_Aint)tmp_size*st_offsets[i];
	}
    }

    disps[1] *= orig_extent;

    disps[2] = orig_extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)array_of_gsizes[i];
	
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = type_new;
    types[2] = MPI_UB;
    
    MPI_Type_struct(3, blklens, disps, types, newtype);

    MPI_Type_free(&type_new);
    sctk_free(st_offsets);
    sctk_free(coords);
    
    return MPI_SUCCESS;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims, int nprocs,
		     int rank, int darg, int order, MPI_Aint orig_extent,
		     MPI_Datatype type_old, MPI_Datatype *type_new,
		     MPI_Aint *st_offset) 
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int blksize, global_size, mysize, i, j;
    MPI_Aint stride;
    
    global_size = array_of_gsizes[dim];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
	blksize = (global_size + nprocs - 1)/nprocs;
    else {
	blksize = darg;

	/* --BEGIN ERROR HANDLING-- */
	if (blksize <= 0) {
	    return MPI_ERR_ARG;
	}

	if (blksize * nprocs < global_size) {
	    return MPI_ERR_ARG;
	}
	/* --END ERROR HANDLING-- */
    }

    j = global_size - blksize*rank;
    mysize = (blksize < j)?blksize:j;
    if (mysize < 0) mysize = 0;

    stride = orig_extent;
    if (order == MPI_ORDER_FORTRAN) {
	if (dim == 0) 
	    MPI_Type_contiguous(mysize, type_old, type_new);
	else {
	    for (i=0; i<dim; i++) stride *= (MPI_Aint)array_of_gsizes[i];
	    MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
	}
    }
    else {
	if (dim == ndims-1) 
	    MPI_Type_contiguous(mysize, type_old, type_new);
	else {
	    for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)array_of_gsizes[i];
	    MPI_Type_hvector(mysize, 1, stride, type_old, type_new);
	}

    }

    *st_offset = (MPI_Aint)blksize * (MPI_Aint)rank;
     /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0) *st_offset = 0;

    return MPI_SUCCESS;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int sctk_MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset) 
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int blksize, i, blklens[3], st_index, end_index, local_size, rem, count;
    MPI_Aint stride, disps[3];
    MPI_Datatype type_tmp, types[3];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG) blksize = 1;
    else blksize = darg;

    /* --BEGIN ERROR HANDLING-- */
    if (blksize <= 0) {
	return MPI_ERR_ARG;
    }
    /* --END ERROR HANDLING-- */
    
    st_index = rank*blksize;
    end_index = array_of_gsizes[dim] - 1;

    if (end_index < st_index) local_size = 0;
    else {
	local_size = ((end_index - st_index + 1)/(nprocs*blksize))*blksize;
	rem = (end_index - st_index + 1) % (nprocs*blksize);
	local_size +=  (rem < blksize)?rem:blksize;
    }

    count = local_size/blksize;
    rem = local_size % blksize;
    
    stride = (MPI_Aint)nprocs*(MPI_Aint)blksize*orig_extent;
    if (order == MPI_ORDER_FORTRAN)
	for (i=0; i<dim; i++) stride *= (MPI_Aint)array_of_gsizes[i];
    else for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)array_of_gsizes[i];

    MPI_Type_hvector(count, blksize, stride, type_old, type_new);

    if (rem) {
	/* if the last block is of size less than blksize, include
	   it separately using MPI_Type_struct */

	types[0] = *type_new;
	types[1] = type_old;
	disps[0] = 0;
	disps[1] = (MPI_Aint)count*stride;
	blklens[0] = 1;
	blklens[1] = rem;

	MPI_Type_struct(2, blklens, disps, types, &type_tmp);

	MPI_Type_free(type_new);
	*type_new = type_tmp;
    }

    /* In the first iteration, we need to set the displacement in that
       dimension correctly. */ 
    if ( ((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
         ((order == MPI_ORDER_C) && (dim == ndims-1)) ) {
        types[0] = MPI_LB;
        disps[0] = 0;
        types[1] = *type_new;
        disps[1] = (MPI_Aint)rank * (MPI_Aint)blksize * orig_extent;
        types[2] = MPI_UB;
        disps[2] = orig_extent * (MPI_Aint)array_of_gsizes[dim];
        blklens[0] = blklens[1] = blklens[2] = 1;
        MPI_Type_struct(3, blklens, disps, types, &type_tmp);
        MPI_Type_free(type_new);
        *type_new = type_tmp;

        *st_offset = 0;  /* set it to 0 because it is taken care of in
                            the struct above */
    }
    else {
        *st_offset = (MPI_Aint)rank * (MPI_Aint)blksize; 
        /* st_offset is in terms of no. of elements of type oldtype in
         * this dimension */ 
    }

    if (local_size == 0) *st_offset = 0;

    return MPI_SUCCESS;
}


/************************************************************************/
/* SUBARRAY IMPLEMENTATION                                              */
/************************************************************************/


int sctk_Type_create_subarray(int ndims,
			      int *array_of_sizes, 
			      int *array_of_subsizes,
			      int *array_of_starts,
			      int order,
			      MPI_Datatype oldtype, 
			      MPI_Datatype *newtype)
{
    MPI_Aint extent, disps[3], size;
    int i, blklens[3];
    MPI_Datatype tmp1, tmp2, types[3];

    MPI_Type_extent(oldtype, &extent);

    if (order == MPI_ORDER_FORTRAN) {
	/* dimension 0 changes fastest */
	if (ndims == 1) {
	    MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
	}
	else {
	    MPI_Type_vector(array_of_subsizes[1],
			    array_of_subsizes[0],
			    array_of_sizes[0], oldtype, &tmp1);
	    
	    size = (MPI_Aint)array_of_sizes[0]*extent;
	    for (i=2; i<ndims; i++) {
		size *= (MPI_Aint)array_of_sizes[i-1];
		MPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = array_of_starts[0];
	size = 1;
	for (i=1; i<ndims; i++) {
	    size *= (MPI_Aint)array_of_sizes[i-1];
	    disps[1] += size*(MPI_Aint)array_of_starts[i];
	}  
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
	/* dimension ndims-1 changes fastest */
	if (ndims == 1) {
	    MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
	}
	else {
	    MPI_Type_vector(array_of_subsizes[ndims-2],
			    array_of_subsizes[ndims-1],
			    array_of_sizes[ndims-1], oldtype, &tmp1);
	    
	    size = (MPI_Aint)array_of_sizes[ndims-1]*extent;
	    for (i=ndims-3; i>=0; i--) {
		size *= (MPI_Aint)array_of_sizes[i+1];
		MPI_Type_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = array_of_starts[ndims-1];
	size = 1;
	for (i=ndims-2; i>=0; i--) {
	    size *= (MPI_Aint)array_of_sizes[i+1];
	    disps[1] += size*(MPI_Aint)array_of_starts[i];
	}
    }
    
    disps[1] *= extent;
    
    disps[2] = extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)array_of_sizes[i];
    
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = tmp1;
    types[2] = MPI_UB;
    
    MPI_Type_struct(3, blklens, disps, types, newtype);

    MPI_Type_free(&tmp1);

    return MPI_SUCCESS;
}
