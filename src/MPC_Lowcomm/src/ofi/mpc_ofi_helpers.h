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
#ifndef MPC_OFI_HELPERS_H
#define MPC_OFI_HELPERS_H

#include <stdint.h>
#include <stdio.h>

#include <mpc_common_debug.h>

#include <rdma/fabric.h>
#include <rdma/fi_errno.h>

/***********
* DEFINES *
***********/

#define MPC_OFI_ADDRESS_LEN    512

/*****************
* ALLOC ALIGNED *
*****************/

typedef struct _mpc_ofi_aligned_mem_s
{
	void *orig;
	void *ret;
}_mpc_ofi_aligned_mem_t;

_mpc_ofi_aligned_mem_t _mpc_ofi_alloc_aligned(size_t size);
void _mpc_ofi_free_aligned(_mpc_ofi_aligned_mem_t *mem);

/*************
* PRINTINGS *
*************/

int _mpc_ofi_decode_cq_flags(uint64_t flags);
int _mpc_ofi_decode_mr_mode(uint64_t flags);
const char *_mpc_ofi_decode_endpoint_type(enum fi_ep_type);
enum fi_ep_type _mpc_ofi_encode_endpoint_type(const char *type);

/************
* RETCODES *
************/

#define MPC_OFI_DUMP_ERROR(a)           do{                                                                                             \
		int __ret = (a);                                                                                                        \
		if(__ret < 0){                                                                                                          \
			mpc_common_errorpoint_fmt("[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
		}                                                                                                                       \
}while(0)

#define MPC_OFI_CHECK_RET(a)            do{                                                                                             \
		int __ret = (a);                                                                                                        \
		if(__ret < 0){                                                                                                          \
			mpc_common_errorpoint_fmt("[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
			return __ret;                                                                                                   \
		}                                                                                                                       \
}while(0)

#define MPC_OFI_CHECK_RET_OR_NULL(a)    do{                                                                                             \
		int __ret = (a);                                                                                                        \
		if(__ret < 0){                                                                                                          \
			mpc_common_errorpoint_fmt("[MPC_OFI]@%s:%d: %s\n%s(%d)\n", __FILE__, __LINE__, #a, fi_strerror(-__ret), __ret); \
			return NULL;                                                                                                    \
		}                                                                                                                       \
}while(0)

/*********
* HINTS *
*********/

struct fi_info *_mpc_ofi_get_requested_hints(const char *provider, const char *endpoint_type);

#endif /* MPC_OFI_HELPERS_H */
