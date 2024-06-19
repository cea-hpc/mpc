/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "./dataloop.h"

/* This is a temporary definition to enable the restrict keyword that is part
   of C.  Sophisticated compilers can use this to improve code generation,
   particularly for loops that copy data */
#define restrict __restrict

#ifdef HAVE_ANY_INT64_T_ALIGNMENT
#define MPIR_ALIGN8_TEST(p1,p2)
#else
#define MPIR_ALIGN8_TEST(p1,p2) && ((((MPI_Aint)p1 | (MPI_Aint)p2) & 0x7) == 0)
#endif

#ifdef HAVE_ANY_INT32_T_ALIGNMENT
#define MPIR_ALIGN4_TEST(p1,p2)
#else
#define MPIR_ALIGN4_TEST(p1,p2) && ((((MPI_Aint)p1 | (MPI_Aint)p2) & 0x3) == 0)
#endif

#define MPIDI_COPY_FROM_VEC(src,dest,stride,type,nelms,count)	\
{								\
    type const * l_src = (type const *)src;                     \
    type * l_dest = (type *)dest;	                        \
    int i, j;							\
    const int l_stride = stride;				\
    if (nelms == 1) {						\
        for (i=count;i!=0;i--) {				\
            *l_dest++ = *l_src;					\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    else {							\
        for (i=count; i!=0; i--) {				\
            for (j=0; j<nelms; j++) {				\
                *l_dest++ = l_src[j];				\
	    }							\
            l_src = (type const *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    dest = (char *) l_dest;					\
    src  = (char *) l_src;                                      \
}

#define MPIDI_COPY_TO_VEC(src,dest,stride,type,nelms,count)	\
{								\
    type const * l_src = (type const *)src;                     \
    type * l_dest = (type *)dest;	                        \
    int i, j;							\
    const int l_stride = stride;				\
    if (nelms == 1) {						\
        for (i=count;i!=0;i--) {				\
            *l_dest = *l_src++;					\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else {							\
        for (i=count; i!=0; i--) {				\
            for (j=0; j<nelms; j++) {				\
                l_dest[j] = *l_src++;				\
	    }							\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    dest = (char *) l_dest;					\
    src  = (char *) l_src;                                      \
}

/* m2m_params defined in dataloop_parts.h */

int PREPEND_PREFIX(Segment_contig_m2m)(DLOOP_Offset *blocks_p,
				       DLOOP_Type    el_type,
				       DLOOP_Offset  rel_off,
				       DLOOP_Buffer  UNUSED(unused),
				       void         *v_paramp)
{
    DLOOP_Offset el_size; /* DLOOP_Count? */
    DLOOP_Offset size;
    struct PREPEND_PREFIX(m2m_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;

#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[contig unpack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.unpack.unpack_buffer,
	       el_size,
	       (int) *blocks_p);
#endif
#if 0
    if (size > 200000 && ((size & 0x3) == 0)
	MPIR_ALIGN8_TEST(paramp->streambuf,(paramp->userbuf+rel_off))) {
	if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	    double const *src = (double const *)paramp->streambuf;
	    double *dest = (double *)(paramp->userbuf+rel_off);
	    int count = size >> 3;
	    while (count--) *dest++=*src++;
	}
	else {
	    double const *src = (double const *)(paramp->userbuf+rel_off);
	    double *dest = (double *)paramp->streambuf;
	    int count = size >> 3;
	    while (count--) *dest++=*src++;
	}
    }
#endif
    if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	memcpy((char *) paramp->userbuf + rel_off, paramp->streambuf, size);
    }
    else {
	memcpy(paramp->streambuf, (char *) paramp->userbuf + rel_off, size);
    }

    paramp->streambuf += size;
    return 0;
}

/* Segment_vector_m2m
 *
 * Note: this combines both packing and unpacking functionality.
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
int PREPEND_PREFIX(Segment_vector_m2m)(DLOOP_Offset *blocks_p,
				       DLOOP_Count   UNUSED(unused),
				       DLOOP_Count   blksz,
				       DLOOP_Offset  stride,
				       DLOOP_Type    el_type,
				       DLOOP_Offset  rel_off, /* into buffer */
				       DLOOP_Buffer  UNUSED(unused2),
				       void         *v_paramp)
{
    DLOOP_Count i, blocks_left, whole_count;
    DLOOP_Offset el_size;
    struct PREPEND_PREFIX(m2m_params) *paramp = v_paramp;
    char *cbufp;

    cbufp = paramp->userbuf + rel_off;
    DLOOP_Handle_get_size_macro(el_type, el_size);

    whole_count = (blksz > 0) ? (*blocks_p / blksz) : 0;
    blocks_left = (blksz > 0) ? (*blocks_p % blksz) : 0;

    if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	if (el_size == 8 
	    MPIR_ALIGN8_TEST(paramp->streambuf,cbufp))
	{
	    MPIDI_COPY_TO_VEC(paramp->streambuf, cbufp, stride,
			      int64_t, blksz, whole_count);
	    MPIDI_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
			      int64_t, blocks_left, 1);
	}
	else if (el_size == 4
		 MPIR_ALIGN4_TEST(paramp->streambuf,cbufp))
	{
	    MPIDI_COPY_TO_VEC((paramp->streambuf), cbufp, stride,
			      int32_t, blksz, whole_count);
	    MPIDI_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
			      int32_t, blocks_left, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_TO_VEC(paramp->streambuf, cbufp, stride,
			      int16_t, blksz, whole_count);
	    MPIDI_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
			      int16_t, blocks_left, 1);
	}
	else {
	    for (i=0; i < whole_count; i++) {
		memcpy(cbufp, paramp->streambuf, blksz * el_size);
		paramp->streambuf += blksz * el_size;
		cbufp += stride;
	    }
	    if (blocks_left) {
		memcpy(cbufp, paramp->streambuf, blocks_left * el_size);
		paramp->streambuf += blocks_left * el_size;
	    }
	}
    }
    else /* M2M_FROM_USERBUF */ {
	if (el_size == 8 
	    MPIR_ALIGN8_TEST(cbufp,paramp->streambuf))
	{
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
				int64_t, blksz, whole_count);
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
				int64_t, blocks_left, 1);
	}
	else if (el_size == 4
		 MPIR_ALIGN4_TEST(cbufp,paramp->streambuf))
	{
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
				int32_t, blksz, whole_count);
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
				int32_t, blocks_left, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
				int16_t, blksz, whole_count);
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
				int16_t, blocks_left, 1);
	}
	else {
	    for (i=0; i < whole_count; i++) {
		memcpy(paramp->streambuf, cbufp, blksz * el_size);
		paramp->streambuf += blksz * el_size;
		cbufp += stride;
	    }
	    if (blocks_left) {
		memcpy(paramp->streambuf, cbufp, blocks_left * el_size);
		paramp->streambuf += blocks_left * el_size;
	    }
	}
    }

    return 0;
}

/* MPID_Segment_blkidx_m2m
 */
int PREPEND_PREFIX(Segment_blkidx_m2m)(DLOOP_Offset *blocks_p,
				       DLOOP_Count   UNUSED(count),
				       DLOOP_Count   blocklen,
				       DLOOP_Offset *offsetarray,
				       DLOOP_Type    el_type,
				       DLOOP_Offset  rel_off,
				       DLOOP_Buffer  UNUSED(unused),
				       void         *v_paramp)
{
#define OLDCODE 0
#if OLDCODE
    DLOOP_Count curblock = 0;
#endif
    DLOOP_Offset el_size;
    DLOOP_Offset blocks_left = *blocks_p;
    char *cbufp;
    struct PREPEND_PREFIX(m2m_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);

#if OLDCODE
    while (blocks_left) {
	char *src, *dest;

	DLOOP_Assert(curblock < count);

	cbufp = paramp->userbuf + rel_off + offsetarray[curblock];

	if (blocklen > blocks_left) blocklen = blocks_left;

	if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	    src  = paramp->streambuf;
	    dest = cbufp;
	}
	else {
	    src  = cbufp;
	    dest = paramp->streambuf;
	}

	/* note: macro modifies dest buffer ptr, so we must reset */
	if (el_size == 8
	    MPIR_ALIGN8_TEST(src, dest))
	{
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int64_t, blocklen, 1);
	}
	else if (el_size == 4
		 MPIR_ALIGN4_TEST(src,dest))
	{
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int32_t, blocklen, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int16_t, blocklen, 1);
	}
	else {
	    memcpy(dest, src, blocklen * el_size);
	}

	paramp->streambuf += blocklen * el_size;
	blocks_left -= blocklen;
	curblock++;
    }
#else
    {
    char *dest;
    char const *src;
    cbufp = paramp->userbuf + rel_off;
    DLOOP_Offset const *offsetp = offsetarray;

    /* Separate the loops into the 2 directions */
    if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	int srcsize = el_size * blocklen;
	src = paramp->streambuf;
	/* Optimize for blocklen == 1 */
	if (blocklen == 1) {
	    /* Optimize for doubles and aligned */
	    if (el_size == 8 MPIR_ALIGN8_TEST(src,cbufp)) {
		int64_t const *restrict l_src = (int64_t *)src;
		while (blocks_left--) {
		    int64_t *restrict l_dest = 
			(int64_t *)(cbufp + *offsetp++);
		    *l_dest = *l_src++;
		}
		src = (char const *)l_src;
	    }
	    else {
		while (blocks_left--) {
		    dest = cbufp + *offsetp++;
		    if (el_size == 8 MPIR_ALIGN8_TEST(src,dest)) {
			int64_t const *l_src = (int64_t *)src;
			int64_t *l_dest = (int64_t *)dest;
			*l_dest = *l_src;
			src += 8;
		    }
		    else if (el_size == 4 MPIR_ALIGN4_TEST(src,dest)) {
			int32_t const *l_src = (int32_t *)src;
			int32_t *l_dest = (int32_t *)dest;
			*l_dest = *l_src;
			src += 4;
		    }
		    else if (el_size == 2) {
			int16_t const *l_src = (int16_t *)src;
			int16_t *l_dest = (int16_t *)dest;
			*l_dest = *l_src;
			src += 2;
		    }
		    else {
			memcpy(dest, src, el_size);
			src += el_size;
		    }
		}
	    }
	}
	else {
	    while (blocks_left) {
		
		if (blocklen > blocks_left) {
		    blocklen = blocks_left;
		    srcsize = el_size * blocklen;
		}
	    
		dest = cbufp + *offsetp++;
		
		/* note: macro modifies dest buffer ptr, so we must reset */
		if (el_size == 8
		    MPIR_ALIGN8_TEST(src, dest))
		    {
			MPIDI_COPY_FROM_VEC(src, dest, 0, int64_t, blocklen, 1);
		    }
		else if (el_size == 4
			 MPIR_ALIGN4_TEST(src,dest))
		    {
			MPIDI_COPY_FROM_VEC(src, dest, 0, int32_t, blocklen, 1);
		    }
		else if (el_size == 2) {
		    MPIDI_COPY_FROM_VEC(src, dest, 0, int16_t, blocklen, 1);
		}
		else {
		    memcpy(dest, src, srcsize);
		    src += srcsize;
		}
		
		blocks_left -= blocklen;
	    }
	}
	paramp->streambuf = (char *)src;
    }
    else {
	int destsize = el_size * blocklen;
	dest = paramp->streambuf;
	/* Optimize for blocklen == 1 */
	if (blocklen == 1) {
	    /* Optimize for doubles and aligned */
	    if (el_size == 8 MPIR_ALIGN8_TEST(cbufp,dest)) {
		int64_t * restrict l_dest = (int64_t *)dest;
		while (blocks_left--) {
		    register int64_t const * restrict l_src     = 
			(int64_t *)(cbufp + *offsetp++);
		    *l_dest++ = *l_src;
		}
		dest = (char *)l_dest;
	    }
	    else {
		while (blocks_left--) {
		    
		    src  = cbufp + *offsetp++;
		    
		    if (el_size == 8 MPIR_ALIGN8_TEST(src, dest)) {
			int64_t *l_dest = (int64_t *)dest;
			int64_t const *l_src = (int64_t *)src;
			*l_dest = *l_src;
			dest += 8;
		    }
		    else if (el_size == 4 MPIR_ALIGN4_TEST(src,dest)) {
			int32_t *l_dest = (int32_t *)dest;
			int32_t const *l_src = (int32_t *)src;
			*l_dest = *l_src;
			dest += 4;
		    }
		    else if (el_size == 2) {
			int16_t *l_dest = (int16_t *)dest;
			int16_t const *l_src = (int16_t *)src;
			*l_dest = *l_src;
			dest += 2;
		    }
		    else {
			memcpy(dest, src, el_size);
			dest += el_size;
		    }
		}
	    }
	}
	else {
	    while (blocks_left) {
		
		if (blocklen > blocks_left) {
		    blocklen = blocks_left;
		    destsize = el_size * blocklen;
		}
		
		src  = cbufp + *offsetp++;
		
		/* note: macro modifies dest buffer ptr, so we must reset */
		if (el_size == 8
		    MPIR_ALIGN8_TEST(src, dest))
		    {
			MPIDI_COPY_FROM_VEC(src, dest, 0, int64_t, blocklen, 1);
		    }
		else if (el_size == 4
			 MPIR_ALIGN4_TEST(src,dest))
		    {
			MPIDI_COPY_FROM_VEC(src, dest, 0, int32_t, blocklen, 1);
		    }
		else if (el_size == 2) {
		    MPIDI_COPY_FROM_VEC(src, dest, 0, int16_t, blocklen, 1);
		}
		else {
		    memcpy(dest, src, blocklen * el_size);
		    dest += destsize;
		}
		
		blocks_left -= blocklen;
	    }
	}
	paramp->streambuf = dest;
    }
    
    }
#endif
    return 0;
}

/* MPID_Segment_index_m2m
 */
int PREPEND_PREFIX(Segment_index_m2m)(DLOOP_Offset *blocks_p,
				      DLOOP_Count   count,
				      DLOOP_Count  *blockarray,
				      DLOOP_Offset *offsetarray,
				      DLOOP_Type    el_type,
				      DLOOP_Offset  rel_off,
				      DLOOP_Buffer  UNUSED(unused),
				      void         *v_paramp)
{
    int curblock = 0;
    DLOOP_Offset el_size;
    DLOOP_Offset cur_block_sz, blocks_left = *blocks_p;
    char *cbufp;
    struct PREPEND_PREFIX(m2m_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);

    while (blocks_left) {
	char *src, *dest;

	DLOOP_Assert(curblock < count);
	cur_block_sz = blockarray[curblock];

	cbufp = paramp->userbuf + rel_off + offsetarray[curblock];

	if (cur_block_sz > blocks_left) cur_block_sz = blocks_left;

	if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
	    src  = paramp->streambuf;
	    dest = cbufp;
	}
	else {
	    src  = cbufp;
	    dest = paramp->streambuf;
	}

	/* note: macro modifies dest buffer ptr, so we must reset */
	if (el_size == 8
	    MPIR_ALIGN8_TEST(src, dest))
	{
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int64_t, cur_block_sz, 1);
	}
	else if (el_size == 4
		 MPIR_ALIGN4_TEST(src,dest))
	{
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int32_t, cur_block_sz, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_FROM_VEC(src, dest, 0, int16_t, cur_block_sz, 1);
	}
	else {
	    memcpy(dest, src, cur_block_sz * el_size);
	}

	paramp->streambuf += cur_block_sz * el_size;
	blocks_left -= cur_block_sz;
	curblock++;
    }

    return 0;
}

void PREPEND_PREFIX(Segment_pack)(DLOOP_Segment *segp,
				  DLOOP_Offset   first,
				  DLOOP_Offset  *lastp,
				  void          *streambuf)
{
    struct PREPEND_PREFIX(m2m_params) params;

    /* experimenting with discarding buf value in the segment, keeping in
     * per-use structure instead. would require moving the parameters around a
     * bit. same applies to Segment_unpack below.
     */
    params.userbuf   = segp->ptr;
    params.streambuf = streambuf;
    params.direction = DLOOP_M2M_FROM_USERBUF;

    PREPEND_PREFIX(Segment_manipulate)(segp, first, lastp,
				       PREPEND_PREFIX(Segment_contig_m2m),
				       PREPEND_PREFIX(Segment_vector_m2m),
				       PREPEND_PREFIX(Segment_blkidx_m2m),
				       PREPEND_PREFIX(Segment_index_m2m),
				       NULL, /* size fn */
				       &params);
    return;
}

void PREPEND_PREFIX(Segment_unpack)(DLOOP_Segment *segp,
				    DLOOP_Offset   first,
				    DLOOP_Offset  *lastp,
				    void *streambuf)
{
    struct PREPEND_PREFIX(m2m_params) params;

    params.userbuf   = segp->ptr;
    params.streambuf = streambuf;
    params.direction = DLOOP_M2M_TO_USERBUF;

    PREPEND_PREFIX(Segment_manipulate)(segp, first, lastp,
				       PREPEND_PREFIX(Segment_contig_m2m),
				       PREPEND_PREFIX(Segment_vector_m2m),
				       PREPEND_PREFIX(Segment_blkidx_m2m),
				       PREPEND_PREFIX(Segment_index_m2m),
				       NULL, /* size fn */
				       &params);
    return;
}

/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
