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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#include <sctk_iso_alloc.h>

#ifndef SCTK_ISO_ALLOC_LIB
#include <sctk_debug.h>
#include <sctk_spinlock.h>

#if defined(MPC_Threads)
#include <sctk_thread.h>
#endif

static volatile unsigned long isoalloc_ptr = 0;
static sctk_spinlock_t lock;

void *
sctk_iso_malloc (size_t size)
{
  void *tmp;
  assume (size > 0);
  if(size % 16 != 0){
    size += 16 - (size % 16);
  }
  assume(size % 16 == 0);

#if defined(MPC_Threads)
  assume (sctk_multithreading_initialised == 0);
#endif
  sctk_spinlock_lock (&lock);
  assume ((unsigned long) size + isoalloc_ptr < SCTK_MAX_ISO_ALLOC_SIZE);
  tmp = sctk_isoalloc_buffer + isoalloc_ptr;
  isoalloc_ptr += size;
  sctk_nodebug ("Iso allocate %p %lu, %lu %p", tmp, size,
		SCTK_MAX_ISO_ALLOC_SIZE - isoalloc_ptr, sctk_isoalloc_buffer);
  sctk_spinlock_unlock (&lock);
  return tmp;
}

void *
sctk_iso_init ()
{
  sctk_nodebug ("Iso buffer %p", sctk_isoalloc_buffer);
  return sctk_isoalloc_buffer;
}

void
sctk_iso_alloc_stat (char *buf)
{
  double used_mem;
  double free_mem;

  used_mem = (isoalloc_ptr) / 1024.0;
  free_mem =
    (SCTK_MAX_ISO_ALLOC_SIZE - (unsigned long) isoalloc_ptr) / 1024.0;

  sprintf (buf, "IsoAlloc Used %6.2fKo Free %6.2fKo", used_mem, free_mem);
}

#endif
