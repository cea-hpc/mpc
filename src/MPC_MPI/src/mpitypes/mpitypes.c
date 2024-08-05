/* -*- Mode: C; c-basic-offset:4 ; -*- */

#include "mpc_mpi.h"
#include "mpitypes.h"

/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* MPIT_Type_memcpy - Copies data between a region described by an MPI
   ( buf, count, type ) tuple and a contiguous data buffer.

   Direction can be either MPIT_MEMCPY_FROM_USERBUF or
   MPIT_MEMCPY_TO_USERBUF, where "userbuf" refers to the buffer described
   by the tuple.

   Start and end refer to starting and ending byte locations in the
   stream. Specifically, end refers to the byte offset just past the last
   to be processed (e.g. to process bytes 0..5, start = 0 and end = 6).

   Returns:
   - MPI_ERR_TYPE if datatype was not initialized prior.
   - MPI_ERR_OTHER if there is an internal error during processing.
   - MPI_SUCCESS on successful copy.
*/
int MPIT_Type_memcpy(void *typeptr,
		     int count,
		     MPI_Datatype type,
		     void *streamptr,
		     int direction,
		     MPI_Offset start,
		     MPI_Offset end)
{
    int mpi_errno;
    MPIT_Segment *segp;
    MPIT_Offset last;
    struct MPIT_m2m_params params;

    if (!MPIT_Type_is_initialized(type)) return MPI_ERR_TYPE;

    segp = MPIT_Segment_alloc();
    mpi_errno = MPIT_Segment_init(typeptr, count, type, segp, 0);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;

    params.userbuf   = typeptr;
    params.streambuf = streamptr;
    params.direction = direction;
    last = end;

    MPIT_Segment_manipulate(segp, start, &last,
			    MPIT_Segment_contig_m2m,
			    MPIT_Segment_vector_m2m,
			    MPIT_Segment_blkidx_m2m,
			    MPIT_Segment_index_m2m,
			    NULL, /* size function */
			    &params);

    MPIT_Segment_free(segp);

    return (last == end) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

/* MPIT_Type_flatten - Generates an offset-length description of a region
   described by an MPI ( buf, count, type ) tuple.

   Start and end refer to starting and ending byte locations in the
   "stream", if the tuple were to be used to serialize a data structure.
   Specifically, end refers to the byte offset just past the last to be
   processed (e.g. to process bytes 0..5, start = 0 and end = 6).

   Returns:
   - MPI_ERR_TYPE if datatype was not initialized prior.
   - MPI_ERR_OTHER if there is an internal error during processing.
   - MPI_SUCCESS on successful copy.

   Notes:
   - integer pointed to by lengthp is an in/out parameter, describing the
     length of the disps and blocklens arrays on input, and the number of
     elements consumed on output.
*/
int MPIT_Type_flatten(void *typeptr,
		      int count,
		      MPI_Datatype type,
		      MPI_Aint start,
		      MPI_Aint end,
		      MPI_Aint *disps,
		      int *blocklens,
		      int *lengthp)
{
    int mpi_errno;
    MPIT_Segment *segp;
    MPIT_Offset last;

    if (!MPIT_Type_is_initialized(type)) return MPI_ERR_TYPE;

    segp = MPIT_Segment_alloc();
    mpi_errno = MPIT_Segment_init(typeptr, count, type, segp, 0);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;

    /* In this case, we can simply call MPIT_Segment_mpi_flatten() */
    last = end;
    MPIT_Segment_mpi_flatten(segp, start, &last, blocklens, disps, lengthp);

    MPIT_Segment_free(segp);

    return (last == end) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

/* MPIT_Type_blockct - Generates a count of contiguous regions of a portion
   of a buffer described by an MPI ( buf, count, type ) tuple.

   Start and end refer to starting and ending byte locations in the
   "stream", if the tuple were to be used to serialize a data structure.
   Specifically, end refers to the byte offset just past the last to be
   processed (e.g. to process bytes 0..5, start = 0 and end = 6).
*/
int MPIT_Type_blockct(int count,
		      MPI_Datatype type,
		      MPI_Aint start,
		      MPI_Aint end,
		      MPI_Aint *blocksp)
{
    int mpi_errno;
    MPIT_Segment *segp;
    MPIT_Offset last;

    if (!MPIT_Type_is_initialized(type)) return MPI_ERR_TYPE;

    segp = MPIT_Segment_alloc();
    mpi_errno = MPIT_Segment_init(NULL, count, type, segp, 0);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;

    /* In this case, we can simply call MPIT_Segment_count_contig_blocks() */
    last = end;
    MPIT_Segment_count_contig_blocks(segp, start, &last, blocksp);

    MPIT_Segment_free(segp);

    return (last == end) ? MPI_SUCCESS : MPI_ERR_OTHER;
}

/* TODO: move into separate file, clean up other functions. */
void MPIT_Type_debug(MPI_Datatype type)
{
    MPIT_Dataloop *dlp;

    MPIT_Datatype_get_loopptr(type, &dlp, 0);
    assert(dlp);

    MPIT_Dataloop_print(dlp, 0);
}
