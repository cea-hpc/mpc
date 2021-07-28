#include "mpcompt_cancel.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_cancel ( __UNUSED__ int flags ) {
    not_implemented();
}

#endif /* OMPT_SUPPORT */
