/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file is here only to keep includes in one place inside the
 * dataloop subdirectory. This file will be different for each use
 * of the dataloop code, but others should be identical.
 */
#ifndef DATALOOP_H
#define DATALOOP_H

/* This is specific to MPICH2 */
#include "mpitypes_dataloop.h"

/* a printf decimal format specifier for DLOOP_Offset */
#define DLOOP_OFFSET_FMT_DEC_SPEC "%ld"

/* a printf hexadecimal format specifier for DLOOP_Offset */
#define DLOOP_OFFSET_FMT_HEX_SPEC "%lx"

#endif
