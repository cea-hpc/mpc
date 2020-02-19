/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#if defined(MPC_Accelerators) && defined(MPC_USE_CUDA)
#include <sctk_accelerators.h>
#include <sctk_alloc.h>
#include <sctk_debug.h>
#include <mpc_topology_device.h>
#include <mpc_common_spinlock.h>
#include <mpc_topology.h>

extern __thread void *sctk_cuda_ctx;

/**
 * Retrieve the best device to select for the thread cpu_id.
 *
 * This function mainly uses two sctk_device_topology calls:
 *   - <b>sctk_device_matric_get_list_closest_from_pu(...)</b>: Returns a list
 * of devices, each device being
 *     at the same distance from the PU than others
 *   - <b>sctk_device_attach_freest_device_from</b>: From the previously
 * generated list, selects the
 *     GPU with the smallest number of attached resources to it (ie. PU). This
 * call must be thread-safe.
 *
 * @param[in] cpu_id the CPU for whom we pick up a device
 * @return the device_id for the elected device
 */
static int sctk_accl_cuda_get_closest_device(int cpu_id) {
  int num_devices = sctk_accl_get_nb_devices(), nb_check;

  /* if CUDA support is loaded but the current configuration does not provide a
   * GPU: stop */
  if (num_devices <= 0)
    return 0;

  safe_cudart(cudaGetDeviceCount(&nb_check));
  assert(num_devices == nb_check);

  /* else, we try to find the closest device for the current thread */
  mpc_topology_device_t **closest_devices = NULL;
  int nearest_id = -1;

  /* to recycle, nb_check contains the number of minimum distance device */
  closest_devices = mpc_topology_device_matrix_get_list_closest_from_pu(
      cpu_id, "cuda-enabled-card*", &nb_check);

  /* once the list is filtered with the nearest ones, we need to elected the one
   * with the minimum
   * number of attached resources */
  mpc_topology_device_t *elected =
      mpc_topology_device_attach_freest_device_from(closest_devices, nb_check);
  assert(elected != NULL);

  nearest_id = elected->device_id;

  sctk_nodebug("CUDA: (DETECTION) elected device %d", nearest_id);

  sctk_free(closest_devices);
  return nearest_id;
}

/**
 * Initialize a CUDA context for the calling thread.
 * @return
 * 	- 0 if the init succeeded
 * 	- 1 otherwise
 */
int sctk_accl_cuda_init_context() {
  int num_devices = sctk_accl_get_nb_devices(), check_nb;

  /* if CUDA support is loaded but the current configuration does not provide a
   * GPU: stop */
  if (num_devices <= 0) {
    sctk_nodebug("CUDA support enabled but no GPU found !");
    return 1;
  }

  /* optional: sanity check */
  safe_cudart(cudaGetDeviceCount(&check_nb));
  assert(check_nb == num_devices);

  /* we init CUDA TLS context */
  cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;
  cuda = (cuda_ctx_t *)sctk_malloc(sizeof(cuda_ctx_t));
  cuda->pushed = 0;
  cuda->cpu_id = mpc_topology_get_current_cpu();

  sctk_nodebug("CUDA: (MALLOC) pushed?%d, cpu_id=%d, address=%p", cuda->pushed,
               cuda->cpu_id, cuda);

  /* cast from int -> CUdevice (which is a typedef to a int) */
  CUdevice nearest_device =
      (CUdevice)sctk_accl_cuda_get_closest_device(cuda->cpu_id);

  safe_cudadv(
      sctk_cuCtxCreate(&cuda->context, CU_CTX_SCHED_YIELD, nearest_device));

  /*sctk_cuCtxCreate() automatically attaches the ctx to the GPU */
  cuda->pushed = 1;

  mpc_common_debug("CUDA: (INIT) PU %d bound to device %d", cuda->cpu_id,
            nearest_device);

  /* Set the current pointer as default one for the current thread */
  sctk_cuda_ctx = cuda;

  return 0;
}

/**
 * Popping the CUDA context for the calling thread from the associated GPU.
 *
 * Popping a context means we request CUDA to detach the current ctx from the
 * calling thread. This is done when saving the thread, before yielding it with
 * another one (save())
 *
 * @return
 *   - 0 if the popping succeeded
 *   - 1 otherwise
 */
int sctk_accl_cuda_pop_context() {
  int num_devices = sctk_accl_get_nb_devices(), check_nb;

  /* if CUDA support is loaded but the current configuration does not provide a
   * GPU: stop */
  if (num_devices <= 0)
    return 1;

  /* sanity check */
  safe_cudart(cudaGetDeviceCount(&check_nb));
  assert(num_devices == check_nb);

  /* get The CUDA context for the current thread */
  cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;

  /* if the calling thread does not provide a CUDA ctx, this thread is
   * supposed to not execute GPU code */
  if (cuda == NULL) {
    /* we returns 1, to notify the user that nothing happened */
    return 1;
  }

  /* if the associated CUDA context is pushed on the GPU
   * This allow us to maintain save()/restore() operations independent from each
   * other
   */
  if (cuda->pushed) {
    safe_cudadv(sctk_cuCtxPopCurrent(&cuda->context));
    cuda->pushed = 0;

    ///* DEBUG
    int cuda_id;
    safe_cudart(cudaGetDevice(&cuda_id));
    sctk_nodebug("CUDA: (POP) PU %d detached from device %d", cuda->cpu_id,
                 cuda_id);
    //*/
  }

  return 0;
}

/**
 * Pushing the CUDA context for the calling thread to the associated GPU.
 *
 * Pushing a context means attaching the calling thread context to CUDA.
 * This is done when a thread is restored, its context is pushed to the GPU
 *
 * @return
 *  - 0 if succeded
 *  - 1 otherwise
 */
int sctk_accl_cuda_push_context() {
  int num_devices = sctk_accl_get_nb_devices(), nb_check;

  /* if CUDA support is loaded but the current configuration does not provide a
   * GPU: stop */
  if (num_devices <= 0)
    return 1;

  /* sanity checks */
  safe_cudart(cudaGetDeviceCount(&nb_check));
  assert(nb_check == num_devices);

  /* else, we try to push a ctx in GPU queue */
  cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;

  /* it the current thread does not have a CUDA ctx, skip... */
  if (cuda == NULL) {
    /* we returns 1 to notify the user nothing happened */
    return 1;
  }

  /* if the context is not already on the GPU */
  if (!cuda->pushed) {
    safe_cudadv(sctk_cuCtxPushCurrent(cuda->context));
    cuda->pushed = 1;

    ///* DEBUG
    int cuda_id;
    safe_cudart(cudaGetDevice(&cuda_id));
    sctk_nodebug("CUDA (PUSH) PU %d pushed to device %d", cuda->cpu_id,
                 cuda_id);
    //*/
  }
  return 0;
}

extern bool sctk_accl_support;
/**
 * Initialize the CUDA interface with MPC
 *
 * @return 0 if succeeded, 1 otherwise
 */
int sctk_accl_cuda_init() {
  if (sctk_accl_support) /* && sctk_cuda_support) */
  {
    safe_cudadv(sctk_cuInit(0));
    return 0;
  }
  return 1;
}
#endif // MPC_Accelerators && USE_CUDA
