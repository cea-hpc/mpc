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


#ifndef LCR_PTL_RECV_H
#define LCR_PTL_RECV_H

#include "ptl.h"

typedef struct lcr_ptl_recv_block {
        void                *start;
        size_t               size;

        lcr_ptl_rail_info_t *rail;
        ptl_handle_me_t      meh;
        lcr_ptl_op_t         op;
        mpc_list_elem_t      elem;
} lcr_ptl_recv_block_t;

int lcr_ptl_recv_block_activate(lcr_ptl_recv_block_t *block,
                                ptl_pt_index_t pte,
                                ptl_list_t list);
int lcr_ptl_recv_block_enable(lcr_ptl_rail_info_t *srail,
                              mpc_mempool_t *block_mp,
                              mpc_list_elem_t *block_head,
                              ptl_pt_index_t pte,
                              ptl_list_t list);
int lcr_ptl_recv_block_disable(mpc_list_elem_t *head);

#endif
