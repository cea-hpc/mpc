#ifndef LCR_PTL_RECV_H
#define LCR_PTL_RECV_H

#include "sctk_ptl_types.h"

typedef struct lcr_ptl_recv_block {
        void *start;
        size_t size;
        
        sctk_ptl_rail_info_t *rail;
        sctk_ptl_meh_t meh;
        lcr_ptl_send_comp_t comp;
} lcr_ptl_recv_block_t;


int lcr_ptl_recv_block_init(sctk_ptl_rail_info_t *srail, lcr_ptl_recv_block_t **block_p);
int lcr_ptl_recv_block_activate(lcr_ptl_recv_block_t *block, sctk_ptl_pte_id_t pte, sctk_ptl_list_t list);
int lcr_ptl_recv_block_enable(sctk_ptl_rail_info_t *srail, sctk_ptl_pte_id_t pte, sctk_ptl_list_t list);
int lcr_ptl_recv_block_disable(lcr_ptl_block_list_t *list);

#endif
