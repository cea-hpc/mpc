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
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_spinlock.h"
#include <string.h>
#include "sctk_dbg_mpc.h"

#ifdef MPC_Message_Passing
static sctk_spinlock_t sctk_message_passing_dbg_lock = 0;
static volatile int sctk_message_passing_debug_enable = 0;

void
sctk_message_passing_enable_debug (void)
{
  sctk_message_passing_debug_enable = 1;
  sctk_debug_printf ("Enable MPC message_passing debug support\n");
}

void
sctk_message_passing_disable_debug (void)
{
  sctk_debug_printf ("Disable MPC message_passing debug support\n");
  sctk_message_passing_debug_enable = 0;
}
#endif
