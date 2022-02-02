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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LOWCOMM_PSET_H
#define MPC_LOWCOMM_PSET_H

#include "mpc_lowcomm_group.h"
#include <mpc_common_spinlock.h>
#include <mpc_common_types.h>


int _mpc_lowcomm_init_psets(void);

int _mpc_lowcomm_release_psets(void);

int _mpc_lowcomm_pset_push(char *name, mpc_lowcomm_group_t * group, int is_comm_self);

int _mpc_lowcomm_pset_register(void);

#endif /* MPC_LOWCOMM_PSET_H */