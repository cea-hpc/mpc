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

#ifndef __MPCOMPT_MACROS_H__
#define __MPCOMPT_MACROS_H__

/* Enable/Disable OMPT support */
#define OMPT_SUPPORT 0

#if OMPT_SUPPORT
#include "ompt.h"
#include <mpc_config.h>

#ifdef HAVE___BUILTIN_FRAME_ADDRESS
#define MPCOMPT_FRAME_ADDRESS_SUPPORT
#endif

#ifdef HAVE___BUILTIN_RETURN_ADDRESS
#define MPCOMPT_RETURN_ADDRESS_SUPPORT
#endif

#if defined ( MPCOMPT_FRAME_ADDRESS_SUPPORT ) || defined ( MPCOMPT_RETURN_ADDRESS_SUPPORT )
#define MPCOMPT_HAS_FRAME_SUPPORT 1
#else
#define MPCOMPT_HAS_FRAME_SUPPORT 0
#endif

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_MACROS_H__ */
