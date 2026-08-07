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
#ifndef MPC_LOWCOMM_REQUEST_H
#define MPC_LOWCOMM_REQUEST_H


#include "mpc_lowcomm_msg.h"
#include "mpc_lowcomm_types.h"
#include <stddef.h>
struct mpi_request_info
{
	size_t size;
	void   (*request_init_func)(void *request);
};
int mpc_lowcomm_request_complete(mpc_lowcomm_request_t *request);

extern mpc_lowcomm_request_t __request_null;

int _mpc_lowcomm_request_send_complete(int status, void *request, size_t length);


void mpc_lowcomm_request_initialize(void *request);

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/


void mpc_lowcomm_request_pack(void *request, void *buffer);
void mpc_lowcomm_request_unpack(void *request, void *buffer);

#define SCTK_PACK_REALLOC_STEP 10
void mpc_lowcomm_request_add_pack(mpc_lowcomm_request_t *req, void *adr,
                                  const unsigned int nb_items,
                                  const size_t elem_size,
                                  unsigned long *begins,
                                  unsigned long *ends);

void mpc_lowcomm_request_add_pack_absolute(mpc_lowcomm_request_t *req,
                                           const void *adr, const unsigned int nb_items,
                                           const size_t elem_size,
                                           long *begins,
                                           long *ends);

int request_is_null_or_cancelled(mpc_lowcomm_request_t *request);

#endif
