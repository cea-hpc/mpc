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

#ifndef LCP_DEF_H
#define LCP_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <mpc_common_bit.h>

/* Handles */
typedef uint64_t                     lcp_datatype_t;
typedef struct lcp_ep               *lcp_ep_h;
typedef struct lcp_context          *lcp_context_h;
typedef struct lcp_manager          *lcp_manager_h;
typedef struct lcp_task             *lcp_task_h;
typedef struct lcp_request           lcp_request_t;
typedef struct lcp_unexp_ctnr        lcp_unexp_ctnr_t;
typedef struct lcp_mem              *lcp_mem_h;
typedef struct lcp_tag_recv_info     lcp_tag_recv_info_t;
typedef struct lcp_request_param     lcp_request_param_t;
typedef struct lcp_am_recv_param     lcp_am_recv_param_t;
typedef struct lcp_task_completion   lcp_task_completion_t;
typedef struct lcp_pending_table     lcp_pending_table_t;

typedef struct mpc_lowcomm_request_s                        mpc_lowcomm_request_t;
typedef struct _mpc_lowcomm_config_struct_net_driver_config lcr_driver_config_t;
typedef struct _mpc_lowcomm_config_struct_net_rail          lcr_rail_config_t;

typedef int (*lcp_complete_callback_func_t)(mpc_lowcomm_request_t *req);

typedef int (*lcp_send_callback_func_t)(int status, void *user_data);

typedef int (*lcp_am_recv_callback_func_t)(size_t sent, void *user_data);

typedef int (*lcp_am_callback_t)(void *arg, const void *user_hdr, const size_t hdr_size,
                                 void *data, size_t length,
				 lcp_am_recv_param_t *param);

/**
 * @ingroup LCP_MEM
 * @brief LCP memory attributes. 
 *
 * Memory attributes.
 *
 */

enum {
        LCP_MEM_SIZE_FIELD = MPC_BIT(0),
        LCP_MEM_ADDR_FIELD = MPC_BIT(1),
};

/**
 * @ingroup LCP_MEM
 * @brief LCP memory attributes. 
 *
 * Specifies a set of attributes associated to a memory region. They can be
 * retrieved by a call to \ref lcp_mem_query.
 * 
 */
typedef struct lcp_mem_attr {
        uint32_t field_mask; 
        void    *address;
        size_t   size;
} lcp_mem_attr_t;


#endif
