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
#ifndef __MPCOMP_OMPT_GENERAL_H__
#define __MPCOMP_OMPT_GENERAL_H__

/* For definition of OMPT_SUPPORT macro */
#include "mpcomp_types_def.h"

#if OMPT_SUPPORT

#include "ompt.h"

int mpcomp_ompt_is_enabled(void);
ompt_callback_t mpcomp_ompt_get_callback(int);
void mpcomp_ompt_pre_init(void);
void mpcomp_ompt_post_init(void);

#endif /* OMPT_SUPPORT */

#endif /* __MPCOMP_OMPT_GENERAL_H__ */
