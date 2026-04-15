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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#include "lowcomm_one_sided_internal.h"
#include "communicator.h"

mpc_lowcomm_communicator_t mpc_lowcomm_osc_win_get_comm(const mpc_lowcomm_window_t *const win)
{
	_mpc_lowcomm_communicator_acquire(win->comm);
	return win->comm;
}

mpc_lowcomm_group_t *mpc_lowcomm_osc_win_get_group(const mpc_lowcomm_window_t *const win)
{
	return mpc_lowcomm_group_dup(win->comm->group);
}

mpc_lowcomm_osc_win_attr_t mpc_lowcomm_osc_win_get_attr(const mpc_lowcomm_window_t *const win)
{
	return (mpc_lowcomm_osc_win_attr_t){ .base_data = win->base_data };
}
