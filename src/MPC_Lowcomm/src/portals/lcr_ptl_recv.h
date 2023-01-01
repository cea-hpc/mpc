#ifndef LCR_PTL_RECV_H
#define LCR_PTL_RECV_H

#include "sctk_ptl_types.h"

typedef struct lcr_ptl_recv_block {
        void *start;
        size_t size;
        
        sctk_ptl_rail_info_t *rail;
        sctk_ptl_meh_t meh;
} lcr_ptl_recv_block_t;


int lcr_ptl_recv_block_init(sctk_ptl_rail_info_t *srail, lcr_ptl_recv_block_t **block_p);
int lcr_ptl_recv_block_activate(lcr_ptl_recv_block_t *block);
int lcr_ptl_recv_block_enable(sctk_ptl_rail_info_t *srail);
int lcr_ptl_recv_block_disable(sctk_ptl_rail_info_t *srail);

#endif
