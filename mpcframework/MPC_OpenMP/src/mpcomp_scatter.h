
#ifndef __MPCOMP_SCATTER_H__
#define __MPCOMP_SCATTER_H__

#include "mpcomp_types.h"

mpcomp_mvp_t* __mpcomp_scatter_wakeup( mpcomp_node_t* );
mpcomp_instance_t* __mpcomp_scatter_instance_pre_init( mpcomp_thread_t*, const int );
void __mpcomp_scatter_instance_post_init( mpcomp_thread_t* );

#endif /* __MPCOMP_SCATTER_H__ */
