/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc_mpi.h"
#include "mpitypes.h"

/* MPIT_Type_xflatten_params
 *
 * inbuf - pointer to input buffer. Note that there is no "outbuf"; we
 *    use the pointer stored in the outsegp instead.
 * startoff, length - offset (relative to inbuf) and length of "current"
 *    contiguous region. Length initialized to -1. These are modified in
 *    both input and output leaf functions.
 * totalbytes, procbytes - count of the number of bytes to process (total)
 *    and that have been processed (proc). Totalbytes is only read by
 *    leaf functions; procbytes is only referenced in input leaf functions.
 * outsegp - pointer to segment structure for output processing.
 *
 * idisps, odisps, blklens - arrays of input and output offsets and
 *    corresponding length of regions
 */
typedef struct MPIT_Type_xflatten_params_s {
    int index, length, procbytes, totalbytes;
    void *inbuf;
    MPI_Aint startoff; /* stored relative to bufp */
    MPIT_Segment *outsegp;

    MPI_Aint idisps[1000], odisps[1000];
    int blklens[1000];
} MPIT_Type_xflatten_params;

static int MPIT_Leaf_contig_inxflatten(MPI_Aint    *blocksp,
				       MPI_Datatype el_type,
				       MPI_Aint     rel_off,
				       void        *bufp,
				       void        *v_paramp);

static int MPIT_Leaf_contig_outxflatten(MPI_Aint    *blocksp,
					MPI_Datatype el_type,
					MPI_Aint     rel_off,
					void        *bufp,
					void        *v_paramp);

int MPIT_Type_xpack(void *inbuf, int incount, MPI_Datatype intype,
		    void *outbuf, int outcount, MPI_Datatype outtype,
		    double *setuptime)
{
    int i, j, mpi_errno, insize, outsize;
    MPI_Aint last = -1;
    MPIT_Segment *insegp, *outsegp;
    MPI_Aint inextent, outextent, lb;
    double start;

    MPIT_Type_xflatten_params params;

    start = MPI_Wtime();

    MPI_Type_size(intype, &insize);
    MPI_Type_size(outtype, &outsize);

    /* Note: This version only supports equivalent total sizes and
     *       equivalent counts.
     */
    assert(incount * insize == outcount * outsize);
    assert(incount == outcount);

    insegp  = MPIT_Segment_alloc();
    outsegp = MPIT_Segment_alloc();

    params.inbuf      = inbuf;
    params.length     = -1;
    params.outsegp    = outsegp;
    params.procbytes  = 0;

    mpi_errno = MPIT_Segment_init(NULL, incount, intype, insegp, 0);
    mpi_errno = MPIT_Segment_init(outbuf, outcount, outtype, outsegp, 0);

    params.totalbytes = insize;
    mpi_errno = MPIT_Segment_init(NULL, 1, intype, insegp, 0);
    mpi_errno = MPIT_Segment_init(outbuf, 1, outtype, outsegp, 0);

    MPI_Type_get_extent(intype, &lb, &inextent);
    MPI_Type_get_extent(outtype, &lb, &outextent);

    /* Generate template for one instance of each type */
    MPIT_Segment_manipulate(insegp, 0, &last,
			    MPIT_Leaf_contig_inxflatten,
			    NULL, /* vector fn. */
			    NULL, /* blkidx fn. */
			    NULL, /* index fn. */
			    NULL, /* size fn. */
			    &params);

    /* Save "initialization" time */
    *setuptime = MPI_Wtime() - start;

    /* Copy using incount instances of our template */
    for (i=0; i < incount; i++) {
	for (j=0; j < params.index; j++) /* for each region in flattened rep. */
	{
	    int k;

	    char * src = inbuf  + params.idisps[j];
	    char *dest = outbuf + params.odisps[j];
	    const int sz = params.blklens[j];
	    
	    /* On some platforms memcpy() does a bad job with doubles. */
	    if (!(((MPI_Aint) src | (MPI_Aint) dest | sz) & 0x7)) {
	    /* if (0) { */
		for (k=0; k < sz / 8; k++) {
		    *(int64_t *) dest = *(int64_t *) src;
		    dest += 8;
		    src  += 8;
		}
	    }
	    else {
		memcpy(dest, src, sz);
	    }
	}
	inbuf  = (void *) (((MPI_Aint) inbuf)  + inextent);
	outbuf = (void *) (((MPI_Aint) outbuf) + outextent);
    }

    MPIT_Segment_free(insegp);
    MPIT_Segment_free(outsegp);

    return MPI_SUCCESS;
}

/* Leaf functions */
static int MPIT_Leaf_contig_inxflatten(MPI_Aint    *blocksp,
				       MPI_Datatype el_type,
				       MPI_Aint     rel_off,
				       void        *UNUSED(bufp),
				       void        *v_paramp)
{
    int el_size, sizebytes;
    MPIT_Type_xflatten_params *paramp = v_paramp;

    /* TODO: cut out this size if possible; maybe force ALL_BYTES? */
    MPI_Type_size(el_type, &el_size);
    sizebytes = *blocksp * el_size;
   
    if (paramp->length < 0) {
	paramp->startoff = rel_off;
	paramp->length   = sizebytes;
	paramp->index    = 0;
    }
    else if (rel_off == paramp->startoff + paramp->length + 1) {
	paramp->length += sizebytes;
    }
    else {
	int mylength;
	MPI_Aint last;

	mylength = paramp->length;
	last = paramp->procbytes + mylength;
	MPIT_Segment_manipulate(paramp->outsegp,
				paramp->procbytes,
				&last,
				MPIT_Leaf_contig_outxflatten,
				NULL, NULL, NULL, NULL, paramp);
	assert(last == paramp->procbytes + mylength);

	paramp->procbytes += mylength;
	paramp->startoff = rel_off;
	paramp->length = sizebytes;
    }

    if (paramp->procbytes + paramp->length == paramp->totalbytes) {	
	int mylength;
	MPI_Aint last;

	mylength = paramp->length;
	last = paramp->procbytes + mylength;
	MPIT_Segment_manipulate(paramp->outsegp,
				paramp->procbytes,
				&last,
				MPIT_Leaf_contig_outxflatten,
				NULL, NULL, NULL, NULL, paramp);
	assert(last == paramp->procbytes + mylength);
    }
    return 0;
}
					
static int MPIT_Leaf_contig_outxflatten(MPI_Aint    *blocksp,
					MPI_Datatype el_type,
					MPI_Aint     rel_off,
					void        *UNUSED(bufp),
					void        *v_paramp)
{
    int el_size, size, copysize;
    MPIT_Type_xflatten_params *paramp = v_paramp;

    MPI_Type_size(el_type, &el_size);
    size = *blocksp * el_size;

    copysize = (paramp->length < size) ? paramp->length : size;

    assert(paramp->index < 1000);
    paramp->idisps[paramp->index] = paramp->startoff;
    paramp->odisps[paramp->index] = rel_off;
    paramp->blklens[paramp->index] = copysize;

    paramp->index++;

    if (size >= paramp->length) {
	*blocksp = copysize / el_size;
	return 1;
    }
    else {
	paramp->startoff += copysize;
	paramp->length   -= copysize;
	return 0;
    }
}

/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
