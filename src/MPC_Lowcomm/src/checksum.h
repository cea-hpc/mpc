/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK__CHECKSUM_H_
#define __SCTK__CHECKSUM_H_

//#define SCTK_USE_CHECKSUM

#include "comm.h"

unsigned long _mpc_lowcomm_checksum_message(mpc_lowcomm_ptp_message_t *send,
                                            mpc_lowcomm_ptp_message_t *recv);
unsigned long _mpc_lowcomm_checksum_buffer(char *body, struct mpc_lowcomm_ptp_message_s *msg);
unsigned long _mpc_lowcomm_checksum_verify(mpc_lowcomm_ptp_message_t *send, mpc_lowcomm_ptp_message_t *recv);

void _mpc_lowcomm_checksum_register(mpc_lowcomm_ptp_message_t *msg);
void _mpc_lowcomm_checksum_unregister(mpc_lowcomm_ptp_message_t *msg);

#endif
void _mpc_lowcomm_checksum_init();
