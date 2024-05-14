/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCR_DEF_H
#define LCR_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

//FIXME: to be changed
#define LCR_AM_ID_MAX 64
#define LCR_COMPONENT_NAME_MAX 16
#define LCR_DEVICE_NAME_MAX 64
#define LCR_DEVICE_MAX 64

typedef struct _mpc_lowcomm_endpoint_s _mpc_lowcomm_endpoint_t;
typedef struct sctk_rail_info_s        sctk_rail_info_t;
typedef struct sctk_rail_pin_ctx_list  lcr_memp_t;
typedef struct lcr_tag_context         lcr_tag_context_t;
typedef struct lcr_completion          lcr_completion_t;
typedef struct lcr_rail_attr           lcr_rail_attr_t;
typedef struct lcr_device              lcr_device_t;

typedef struct lcr_component          *lcr_component_h;

typedef union {
	struct {
		uint64_t comm:16;
		uint64_t tag:24;
		uint64_t src:24;
	} t_tag;
	uint64_t t;
} lcr_tag_t;

typedef enum {
        LCR_ATOMIC_OP_ADD,
        LCR_ATOMIC_OP_SWAP,
        LCR_ATOMIC_OP_CSWAP,
        LCR_ATOMIC_OP_AND,
        LCR_ATOMIC_OP_OR,
        LCR_ATOMIC_OP_XOR
} lcr_atomic_op_t;

typedef int (*lcr_am_callback_t)(void *arg, void *data, size_t length,
				 unsigned flags);

typedef ssize_t (*lcr_pack_callback_t)(void *dest, void *data);

typedef ssize_t (*lcr_unpack_callback_t)(void *arg, const void *data, size_t length);

typedef void (*lcr_completion_callback_t)(lcr_completion_t *self);

typedef int (*lcr_tag_completion_callback_t)(lcr_tag_context_t *ctx, unsigned flags);

#endif
