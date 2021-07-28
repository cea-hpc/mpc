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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMPT_DEVICE_H__
#define __MPCOMPT_DEVICE_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT

void
_mpc_omp_ompt_callback_device_initialize( int device_num,
                                      const char *type,
                                      ompt_device_t *device,
                                      ompt_function_lookup_t lookup,
                                      const char *documentation );

void
_mpc_omp_ompt_callback_device_finalize( int device_num );

void
_mpc_omp_ompt_callback_device_load( int device_num,
                                const char *filename,
                                int64_t offset_in_file,
                                void *vma_in_file,
                                size_t bytes,
                                void *host_addr,
                                void *device_addr,
                                uint64_t module_id );

void
_mpc_omp_ompt_callback_device_unload( int device_num,
                                  uint64_t module_id );

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_DEVICE_H__ */
