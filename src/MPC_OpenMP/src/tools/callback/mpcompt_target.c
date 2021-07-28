#include "mpcompt_target.h"

#if OMPT_SUPPORT
#include "mpcompt_internal_callback_interface.h"
#include "mpc_common_debug.h"

void
_mpc_omp_ompt_callback_target_data_op ( __UNUSED__ ompt_id_t target_id,
                                    __UNUSED__ ompt_id_t host_op_id,
                                    __UNUSED__ ompt_target_data_op_t optype,
                                    __UNUSED__ void *src_addr,
                                    __UNUSED__ int src_device_num,
                                    __UNUSED__ void *dest_addr,
                                    __UNUSED__ int dest_device_num,
                                    __UNUSED__ size_t bytes ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_target ( __UNUSED__ ompt_target_t kind,
                            __UNUSED__ ompt_scope_endpoint_t endpoint,
                            __UNUSED__ int device_num,
                            __UNUSED__ ompt_data_t *task_data,
                            __UNUSED__ ompt_id_t target_id ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_target_map ( __UNUSED__ ompt_id_t target_id,
                                __UNUSED__ unsigned int nitems,
                                __UNUSED__ void **host_addr,
                                __UNUSED__ void **device_addr,
                                __UNUSED__ size_t *bytes,
                                __UNUSED__ unsigned int *mapping_flags ) {
    not_implemented();
}

void
_mpc_omp_ompt_callback_target_submit ( __UNUSED__ ompt_id_t target_id,
                                   __UNUSED__ ompt_id_t host_op_id,
                                   __UNUSED__ unsigned int requested_num_teams ) {
    not_implemented();
}

#endif /* OMPT_SUPPORT */
