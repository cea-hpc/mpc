#ifdef MPC_USE_PORTALS

#ifndef __SCTK_PTL_ASYNC_H_
#define __SCTK_PTL_ASYNC_H_

#include "sctk_ptl_types.h"
#include "sctk_rail.h"

void sctk_ptl_async_start(sctk_rail_info_t* rail);
void sctk_ptl_async_stop(sctk_rail_info_t* rail);

#endif /* ifndef __SCTK_PTL_ASYNC_H_ */
#endif
