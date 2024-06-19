/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPITYPE_DATALOOP_H
#define MPITYPE_DATALOOP_H

#include <assert.h>
#include <stdlib.h>

#include <mpi.h>
#include "mpc_config.h"

/* This file maps all the various DLOOP_ macros to appropriate MPIT_ calls,
 * and it defines the types that we will use internally.
 */

/* Note: this is where you define the prefix that will be prepended on
 * all externally visible generic dataloop and segment functions.
 */
#define PREPEND_PREFIX(fn) MPIT_ ## fn

/* These following dataloop-specific types will be used throughout the DLOOP
 * instance:
 */
#define DLOOP_Offset     MPI_Aint
#define DLOOP_Count      MPI_Aint
#define DLOOP_Handle     MPI_Datatype
#define DLOOP_Type       MPI_Datatype
#define DLOOP_Buffer     void *
#define DLOOP_VECTOR     struct MPIT_iovec
#define DLOOP_VECTOR_LEN len
#define DLOOP_VECTOR_BUF base

struct MPIT_iovec {
    DLOOP_Offset base;
    DLOOP_Offset len;
};

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()
 * DLOOP_Handle_size()
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_loopdepth()
 * DLOOP_Handle_hasloop()
 *
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_get_loopdepth_macro(handle_,depth_,flag_) \
    MPIT_Datatype_get_loopdepth(handle_,&(depth_),flag_)

#define DLOOP_Handle_get_loopsize_macro(handle_,size_,flag_) \
    MPIT_Datatype_get_loopsize(handle_,&(size_),flag_)

#define DLOOP_Handle_set_loopptr_macro(handle_,lptr_,flag_) \
    MPIT_Datatype_set_loopptr(handle_,lptr_,flag_)

#define DLOOP_Handle_set_loopdepth_macro(handle_,depth_,flag_) \
    MPIT_Datatype_set_loopdepth(handle_,depth_,flag_)

#define DLOOP_Handle_set_loopsize_macro(handle_,size_,flag_) \
    MPIT_Datatype_set_loopsize(handle_,size_,flag_)

#define DLOOP_Handle_get_loopptr_macro(handle_,lptr_,flag_) \
    MPIT_Datatype_get_loopptr(handle_,&(lptr_),flag_)

#define DLOOP_Handle_get_size_macro(handle_,size_) \
    MPIT_Datatype_get_size(handle_,&(size_))

#define DLOOP_Handle_get_basic_type_macro(handle_,eltype_) \
    MPIT_Datatype_get_el_type(handle_, &(eltype_), 0)

#define DLOOP_Handle_get_extent_macro(handle_,extent_) \
    MPIT_Datatype_get_extent(handle_,&(extent_))

#define DLOOP_Handle_hasloop_macro(handle_)	\
    (MPIT_Datatype_is_nontrivial(handle_))

/* allocate, free, and copy functions must also be defined. */
#define DLOOP_Malloc malloc
#define DLOOP_Free   free
#define DLOOP_Memcpy memcpy

/* debugging output function */
#define DLOOP_dbg_printf printf

/* assert function */
#define DLOOP_Assert assert

#if 0
/* contents access functions -- use MPICH2 versions for now. */
#define MPIT_Type_access_contents MPID_Type_access_contents
#define MPIT_Type_release_contents MPID_Type_release_contents
#endif

/* Include dataloop_parts.h at the end to get the rest of the prototypes
 * and defines, in terms of the prefixes and types above.
 */
#include "./dataloop_parts.h"
#include "./dataloop_create.h"

/* accessor functions */
void MPIT_Datatype_get_size(MPI_Datatype type, DLOOP_Offset *size_p);
void MPIT_Datatype_get_extent(MPI_Datatype type, DLOOP_Offset *extent_p);
void MPIT_Datatype_get_block_info(MPI_Datatype type, DLOOP_Offset *true_lb,
				  DLOOP_Offset *count, int *n_contig);
int MPIT_Datatype_is_nontrivial(MPI_Datatype type);
void MPIT_Datatype_get_el_type(MPI_Datatype type, MPI_Datatype *eltype_p,
			       int flag);

/* accessor functions used only by dataloop code proper */
void MPIT_Datatype_get_loopptr(MPI_Datatype type, MPIT_Dataloop **ptr_p,
			       int flag);
void MPIT_Datatype_get_loopsize(MPI_Datatype type, int *size_p, int flag);
void MPIT_Datatype_get_loopdepth(MPI_Datatype type, int *depth_p, int flag);
void MPIT_Datatype_set_loopptr(MPI_Datatype type, MPIT_Dataloop *ptr, int flag);
void MPIT_Datatype_set_loopsize(MPI_Datatype type, int size, int flag);
void MPIT_Datatype_set_loopdepth(MPI_Datatype type, int depth, int flag);

/* accessor functions from elsewhere */
void MPIT_Type_access_contents(MPI_Datatype type,
			       int **ints_p,
			       MPI_Aint **aints_p,
			       MPI_Datatype **types_p);
void MPIT_Type_release_contents(MPI_Datatype type,
				int **ints_p,
				MPI_Aint **aints_p,
				MPI_Datatype **types_p);

/* These values are defined by DLOOP code.
 *
 * Note: DLOOP_DATALOOP_ALL_BYTES not currently used in MPICH2.
 */
#define MPID_DATALOOP_HETEROGENEOUS DLOOP_DATALOOP_HETEROGENEOUS
#define MPID_DATALOOP_HOMOGENEOUS   DLOOP_DATALOOP_HOMOGENEOUS

/* These names are used in MPICH2 for some reason I don't understand;
 * don't fight battles you don't need to win.
 */
#define NMPI_Type_contiguous   PMPI_Type_contiguous
#define NMPI_Type_extent       PMPI_Type_extent
#define NMPI_Type_free         PMPI_Type_free
#define NMPI_Type_get_envelope PMPI_Type_get_envelope
#define NMPI_Type_hvector      PMPI_Type_hvector
#define NMPI_Type_struct       PMPI_Type_struct
#define NMPI_Type_vector       PMPI_Type_vector

/* This is a set of temporary fixes. */
#define DLOOP_OFFSET_CAST_TO_VOID_PTR
#define DLOOP_VOID_PTR_CAST_TO_OFFSET (DLOOP_Offset)(uintptr_t)
#define DLOOP_PTR_DISP_CAST_TO_OFFSET (DLOOP_Offset)(intptr_t)

#define DLOOP_Ensure_Offset_fits_in_pointer(offset_) \
    DLOOP_Assert((offset_) == (DLOOP_Offset)(void *)(offset_))

#ifndef ATTRIBUTE
# if defined(HAVE_GCC_ATTRIBUTE)
#  define ATTRIBUTE(x_) __attribute__(x_)
# else
#  define ATTRIBUTE(x_)
# endif
#endif

/* Marks a variable decl as unused.  Examples:
 *   int UNUSED(foo) = 3;
 *   void bar(int UNUSED(baz)) {} */
#define UNUSED_(x_) x_##__UNUSED ATTRIBUTE((unused))

#endif
