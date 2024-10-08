/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpimem.h"

/*
 * This file contains two of the routines from the "safe" versions of the
 * various string routines in src/util/mem/safestr.c .  These are duplicated
 * here because the MPICH2 utility routines can no longer be used by other
 * applications.
 */

/*
 * MPIU_Strncpy - Copy at most n characters.  Stop once a null is reached.
 *
 * This is different from strncpy, which null pads so that exactly
 * n characters are copied.  The strncpy behavior is correct for many
 * applications because it guarantees that the string has no uninitialized
 * data.
 *
 * If n characters are copied without reaching a null, return an error.
 * Otherwise, return 0.
 *
 * Question: should we provide a way to request the length of the string,
 * since we know it?
 */
/*@ MPIU_Strncpy - Copy a string with a maximum length

    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
.   outstr - String to copy into

    Notes:
    This routine is the routine that you wish 'strncpy' was.  In copying
    'instr' to 'outstr', it stops when either the end of 'outstr' (the
    null character) is seen or the maximum length 'maxlen' is reached.
    Unlike 'strncpy', it does not add enough nulls to 'outstr' after
    copying 'instr' in order to move precisely 'maxlen' characters.
    Thus, this routine may be used anywhere 'strcpy' is used, without any
    performance cost related to large values of 'maxlen'.

    If there is insufficient space in the destination, the destination is
    still null-terminated, to avoid potential failures in routines that neglect
    to check the error code return from this routine.

  Module:
  Utility
  @*/
int MPIU_Strncpy( char *dest, const char *src, size_t n )
{
    char * __restrict__ d_ptr = dest;
    const char * __restrict__ s_ptr = src;
    register int i;

    if (n == 0) return 0;

    i = (int)n;
    while (*s_ptr && i-- > 0) {
	*d_ptr++ = *s_ptr++;
    }

    if (i > 0) {
	*d_ptr = 0;
	return 0;
    }
    else {
	/* Force a null at the end of the string (gives better safety
	   in case the user fails to check the error code) */
	dest[n-1] = 0;
	/* We may want to force an error message here, at least in the
	   debugging version */
	/*printf( "failure in copying %s with length %d\n", src, n ); */
	return 1;
    }
}

/* Append src to dest, but only allow dest to contain n characters (including
   any null, which is always added to the end of the line */
/*@ MPIU_Strnapp - Append to a string with a maximum length

    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
.   outstr - String to copy into

    Notes:
    This routine is similar to 'strncat' except that the 'maxlen' argument
    is the maximum total length of 'outstr', rather than the maximum
    number of characters to move from 'instr'.  Thus, this routine is
    easier to use when the declared size of 'instr' is known.

  Module:
  Utility
  @*/
int MPIU_Strnapp( char *dest, const char *src, size_t n )
{
    char * __restrict__ d_ptr = dest;
    const char *  __restrict__ s_ptr = src;
    register int i;

    /* Get to the end of dest */
    i = (int)n;
    while (i-- > 0 && *d_ptr) d_ptr++;
    if (i <= 0) return 1;

    /* Append.  d_ptr points at first null and i is remaining space. */
    while (*s_ptr && i-- > 0) {
	*d_ptr++ = *s_ptr++;
    }

    /* We allow i >= (not just >) here because the first while decrements
       i by one more than there are characters, leaving room for the null */
    if (i >= 0) {
	*d_ptr = 0;
	return 0;
    }
    else {
	/* Force the null at the end */
	*--d_ptr = 0;

	/* We may want to force an error message here, at least in the
	   debugging version */
	return 1;
    }
}
