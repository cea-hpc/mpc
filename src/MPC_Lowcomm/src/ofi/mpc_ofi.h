/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:33 CEST 2023                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_OFI_H
#define MPC_OFI_H

#include <stdint.h>

#include "mpc_ofi_context.h"
#include <mpc_mempool.h>

typedef struct _mpc_lowcomm_ofi_rail_info_s
{
   struct _mpc_ofi_context_t ctx;
}_mpc_lowcomm_ofi_rail_info_t;

#define MPC_OFI_EP_MEMPOOL_MIN 10
#define MPC_OFI_EP_MEMPOOL_MAX 100

struct _mpc_ofi_domain_buffer_manager_t;
typedef struct _mpc_lowcomm_ofi_endpoint_info_s
{
   mpc_mempool_t bsend;
   mpc_mempool_t deffered;
}_mpc_lowcomm_ofi_endpoint_info_t;


typedef struct lcr_ofi_am_hdr {
	uint8_t am_id;
	size_t length;
   char * data[0];
} lcr_ofi_am_hdr_t;

struct _mpc_ofi_shared_pinning_context
{
	uint64_t ofi_remote_mr_key;
   void * addr;
   size_t size;
};

struct _mpc_ofi_pinning_context
{
	struct fid_mr *ofi;
   struct _mpc_ofi_shared_pinning_context shared;
};



#endif /* MPC_OFI_H */
