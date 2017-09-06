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
/* #   - ADAM Julien <adamj@paratools.com>                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_FT_IFACE_H
#define __SCTK_FT_IFACE_H

#include "sctk_ft_types.h"

/* See src/ft.c for Documentation */

int sctk_ft_init();

int sctk_ft_enabled();

void sctk_ft_checkpoint_init();

void sctk_ft_checkpoint_prepare();

void sctk_ft_checkpoint();

sctk_ft_state_t sctk_ft_checkpoint_wait();

void sctk_ft_checkpoint_finalize();

int sctk_ft_finalize();

const char* sctk_ft_str_status(sctk_ft_state_t);
int sctk_ft_disable();
int sctk_ft_enable();

void sctk_ft_no_suspend_start();
void sctk_ft_no_suspend_end();

#endif /* ifndef __FT_IFACE_H */
