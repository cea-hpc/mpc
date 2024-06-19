/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPITYPES_H
#define MPITYPES_H

#include "mpitypes_dataloop.h"
#include "dataloop_parts.h"

int MPIT_Type_init(MPI_Datatype type);
int MPIT_Type_is_initialized(MPI_Datatype type);

int MPIT_Type_memcpy(void *typeptr,
		     int count,
		     MPI_Datatype type,
		     void *streamptr,
		     int direction,
		     MPI_Offset start,
		     MPI_Offset end);

int MPIT_Type_flatten(void *typeptr,
		      int count,
		      MPI_Datatype type,
		      MPI_Aint start,
		      MPI_Aint end,
		      MPI_Aint *disps,
		      int *blocklens,
		      int *lengthp);

int MPIT_Type_blockct(int count,
		      MPI_Datatype type,
		      MPI_Aint start,
		      MPI_Aint end,
		      MPI_Aint *blocksp);

void MPIT_Type_debug(MPI_Datatype type);

typedef DLOOP_Offset MPIT_Offset;
#define MPIT_MEMCPY_TO_USERBUF DLOOP_M2M_TO_USERBUF
#define MPIT_MEMCPY_FROM_USERBUF DLOOP_M2M_FROM_USERBUF

#if 1
#define MPIT_dbg_printf(...)
#else
#define MPIT_dbg_printf printf
#endif

#endif
