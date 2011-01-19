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

#ifndef __SCTK__ISO_ALLOC_H_
#define __SCTK__ISO_ALLOC_H_

#include "sctk_config.h"
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define SCTK_MAX_ISO_ALLOC_SIZE 400UL*1024UL*1024UL

  void *sctk_iso_malloc (size_t size);
  void *sctk_iso_init (void);
  void sctk_iso_alloc_stat (char *buf);

  extern char sctk_isoalloc_buffer[];

#ifdef SCTK_ISO_ALLOC_LIB
  char sctk_isoalloc_buffer[SCTK_MAX_ISO_ALLOC_SIZE];
#endif

#ifdef __cplusplus
}
#endif
#endif
