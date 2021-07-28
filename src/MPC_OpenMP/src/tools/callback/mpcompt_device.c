#include "mpcompt_device.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_device_initialize ( __UNUSED__ int device_num,
                                       __UNUSED__ const char *type,
                                       __UNUSED__ ompt_device_t *device,
                                       __UNUSED__ ompt_function_lookup_t lookup,
                                       __UNUSED__ const char *documentation ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_device_finalize ( __UNUSED__ int device_num ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_device_load ( __UNUSED__ int device_num,
                                 __UNUSED__ const char *filename,
                                 __UNUSED__ int64_t offset_in_file,
                                 __UNUSED__ void *vma_in_file,
                                 __UNUSED__ size_t bytes,
                                 __UNUSED__ void *host_addr,
                                 __UNUSED__ void *device_addr,
                                 __UNUSED__ uint64_t module_id ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_device_unload ( __UNUSED__ int device_num,
                                   __UNUSED__ uint64_t module_id ) {
    not_implemented();
}

#endif /* OMPT_SUPPORT */
