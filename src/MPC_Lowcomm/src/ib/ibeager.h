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


#ifndef __SCTK__IB_EAGER_H_
#define __SCTK__IB_EAGER_H_

#include <infiniband/verbs.h>
#include "ib.h"
#include "ibufs.h"
#include "qp.h"

typedef struct _mpc_lowcomm_ib_eager_header_s
{
	size_t payload_size;
} _mpc_lowcomm_ib_eager_header_t;


/* This the ibuf layout of an eager message */
typedef struct _mpc_lowcomm_ib_eager_s
{
	_mpc_lowcomm_ib_ibuf_header_t  ibuf_header;
	_mpc_lowcomm_ib_eager_header_t eager_header;
	mpc_lowcomm_ptp_message_body_t msg;
	char                           payload[0];
} _mpc_lowcomm_ib_eager_t;

#define IBUF_GET_EAGER_SIZE    sizeof(_mpc_lowcomm_ib_eager_t)

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/
_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_eager_prepare_msg(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *route_data, mpc_lowcomm_ptp_message_t *msg, size_t size, char is_control_message);
void _mpc_lowcomm_ib_eager_free_msg_no_recopy(void *arg);
void _mpc_lowcomm_ib_eager_recv_msg_no_recopy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp);
void _mpc_lowcomm_ib_eager_init(struct _mpc_lowcomm_ib_rail_info_s *rail_ib);
void _mpc_lowcomm_ib_eager_finalize(struct _mpc_lowcomm_ib_rail_info_s *rail_ib);
int _mpc_lowcomm_ib_eager_poll_recv(struct sctk_rail_info_s *rail, _mpc_lowcomm_ib_ibuf_t *ibuf);

#endif
