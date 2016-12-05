#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

void mpcomp_GOMP_offload_register_ver(unsigned version, const void *host_table,
                                      int target_type,
                                      const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_register(const void *host_table, int target_type,
                                  const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_unregister_ver(unsigned version,
                                        const void *host_table, int target_type,
                                        const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_offload_unregister(const void *host_table, int target_type,
                                    const void *target_data) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target(int device, void (*fn)(void *), const void *unused,
                        size_t mapnum, void **hostaddrs, size_t *sizes,
                        unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_ext(int device, void (*fn)(void *), size_t mapnum,
                            void **hostaddrs, size_t *sizes,
                            unsigned short *kinds, unsigned int flags,
                            void **depend, void **args) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_data(int device, const void *unused, size_t mapnum,
                             void **hostaddrs, size_t *sizes,
                             unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_data_ext(int device, size_t mapnum, void **hostaddrs,
                                 size_t *sizes, unsigned short *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_end_data(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_update(int device, const void *unused, size_t mapnum,
                               void **hostaddrs, size_t *sizes,
                               unsigned char *kinds) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_update_ext(int device, size_t mapnum, void **hostaddrs,
                                   size_t *sizes, unsigned short *kinds,
                                   unsigned int flags, void **depend) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_target_enter_exit_data(int device, size_t mapnum,
                                        void **hostaddrs, size_t *sizes,
                                        unsigned short *kinds,
                                        unsigned int flags, void **depend) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_teams(unsigned int num_teams, unsigned int thread_limit) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  not_implemented();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}
