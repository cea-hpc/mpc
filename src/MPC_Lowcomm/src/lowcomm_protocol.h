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
/* #   - REYNIER Florian    florian.reynier@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LOWCOMM_PROTOCOL_H
#define MPC_LOWCOMM_PROTOCOL_H

#include <stddef.h>
#ifdef __cplusplus
	extern "C" {
#endif

// #include "mpc_lowcomm.h"

void mpc_lowcomm_protocol_init_driver();
void mpc_lowcomm_protocol_finalize_driver();
void mpc_lowcomm_init_protocol_per_task();
int mpc_comm_progress(void);
void mpc_lowcomm_release_protocol_per_task();

#ifdef __cplusplus
	}
#endif
#endif
