#ifndef MPC_OMP_INTEROP_CUDA_H
# define MPC_OMP_INTEROP_CUDA_H

# include <mpc_omp.h>

typedef struct  cuda_stream_progress_info_s
{
    /* the omp event */
    mpc_omp_event_handle_block_t * handle;

    /* the stream */
    CUstream hStream;

    /* query result */
    CUresult r;
}               cuda_stream_progress_info_t;

/**
 *  CUresult cuStreamQuery ( CUstream hStream )
 *      Determine status of a compute stream.
 *
 * Returns CUDA_SUCCESS if all operations in the stream specified by hStream have completed,
 * or CUDA_ERROR_NOT_READY if not. For the purposes of Unified Memory, a return value
 * of CUDA_SUCCESS is equivalent to having called cuStreamSynchronize().
 *
 * Note:
 *  This function uses standard default stream semantics.
 *  Note that this function may also return error codes from previous, asynchronous launches.
 */

static int
__cuda_stream_progress(cuda_stream_progress_info_t * infos)
{
    /* check if the request completed */
    infos->r = cuStreamQuery(infos->hStream);
    if (infos->r != CUDA_ERROR_NOT_READY)
    {
        mpc_omp_fulfill_event((mpc_omp_event_handle_t *) infos->handle);
        return 0;
    }
    return 1;
}

/* task blocking */

static inline CUresult
__task_block(CUstream hStream)
{
    /* test before blocking */
    CUresult r = cuStreamQuery(hStream);
    if (r != CUDA_ERROR_NOT_READY) return r;

    /* stream is blocking, suspend the task */

    /* progress function parameters */
    cuda_stream_progress_info_t infos;
    infos.hStream = hStream;

    /* create a task-blocking handler */
    mpc_omp_event_handle_init(
        (mpc_omp_event_handle_t**) &(infos.handle),
        MPC_OMP_EVENT_TASK_BLOCK);

    /* register the progression callback */
    mpc_omp_callback(
        (int (*)(void *)) __cuda_stream_progress,
        &infos,
        MPC_OMP_CALLBACK_SCOPE_INSTANCE,
        MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE,
        MPC_OMP_CALLBACK_REPEAT_RETURN
    );

    /* block current task until the associated event is fulfilled */
    mpc_omp_task_block(infos.handle);

    /* deinit the handle */
    mpc_omp_event_handle_deinit((mpc_omp_event_handle_t *) infos.handle);

    return infos.r;
}

/**
 * Synchronization point which preempt current task
 * and perform task switches until the stream is completed
 */
CUresult
cuxStreamSynchronize(CUstream hStream)
{
    if (mpc_omp_in_explicit_task())
    {
        return __task_block(hStream);
    }
    return cuStreamSynchronize(hStream);
}

#endif /* MPC_OMP_INTEROP_CUDA_H */
