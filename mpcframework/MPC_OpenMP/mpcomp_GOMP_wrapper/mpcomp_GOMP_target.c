#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

void mpcomp_GOMP_offload_register_ver(__UNUSED__ unsigned version, __UNUSED__ const void *host_table,
                                      __UNUSED__ int target_type,
                                      __UNUSED__ const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_register(__UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                  __UNUSED__ const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_unregister_ver(__UNUSED__ unsigned version,
                                        __UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                        __UNUSED__ const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_unregister(__UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                    __UNUSED__ const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target(__UNUSED__ int device, __UNUSED__ void (*fn)(void *), __UNUSED__ const void *unused,
                        __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                        __UNUSED__ unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

#include "sctk_thread.h"
void mpcomp_GOMP_target_ext(__UNUSED__ int device, __UNUSED__  void (*fn)(void *), __UNUSED__ size_t mapnum,
                            __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                            __UNUSED__ unsigned short *kinds,__UNUSED__  unsigned int flags,
                            __UNUSED__ void **depend, __UNUSED__ void **args) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_data(__UNUSED__ int device, __UNUSED__ const void *unused,__UNUSED__  size_t mapnum,
                             __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                             __UNUSED__ unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_data_ext(__UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                 __UNUSED__ size_t *sizes, __UNUSED__ unsigned short *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_end_data(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_update(__UNUSED__ int device, __UNUSED__ const void *unused, __UNUSED__ size_t mapnum,
                               __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                               __UNUSED__ unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_update_ext(__UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                  __UNUSED__  size_t *sizes,__UNUSED__  unsigned short *kinds,
                                   __UNUSED__ unsigned int flags, __UNUSED__ void **depend) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_enter_exit_data(__UNUSED__ int device, __UNUSED__ size_t mapnum,
                                        __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                                        __UNUSED__ unsigned short *kinds,
                                        __UNUSED__ unsigned int flags,__UNUSED__  void **depend) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_teams(__UNUSED__ unsigned int num_teams, __UNUSED__ unsigned int thread_limit) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}
