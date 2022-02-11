#include "mqd.h"

#include <mpc_config.h>
#include <stdio.h>

#include "sctk_alloc.h"
#include "mpc_common_rank.h"
#include "communicator.h"
#include "mpc_common_spinlock.h"

char MPIR_dll_name[] = "libmpclowcomm.so";

/**************************************
* EXECUTABLE IMAGE RELATED FUNCTIONS *
**************************************/

int mqs_setup_image(mqs_image *image, mqs_image_callbacks *cb)
{
    return mqs_ok;
}

int mqs_image_has_queues(mqs_image *image, char **message)
{
    *message = "MPC %s";
    return mqs_ok;
}

int mqs_destroy_image_info(mqs_image_info *info)
{
    return mqs_ok;
}


/*****************************
* PROCESS RELATED FUNCTIONS *
*****************************/

int mqs_setup_process(mqs_process *process, const mqs_process_callbacks *cb)
{
    struct mpc_lowcomm_mqs_process * ret = sctk_malloc(sizeof( struct mpc_lowcomm_mqs_process ));

    *process = NULL;

    if(!ret)
    {
        return mqs_no_information;
    }

    static mpc_common_spinlock_t proc_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

    mpc_common_spinlock_lock(&proc_lock);

    static int current_exported_process_offset = 0;

    if(mpc_common_get_local_task_count() <= current_exported_process_offset)
    {
        mpc_common_spinlock_unlock(&proc_lock);
        return mqs_end_of_list;
    }

    memset(ret, 0, sizeof(struct mpc_lowcomm_mqs_process));

    int fist_w_rank = _mpc_lowcomm_communicator_world_first_local_task();

    ret->local_rank = current_exported_process_offset;
    ret->global_rank = ret->local_rank + fist_w_rank;

    ret->ptp = sctk_malloc(128 * sizeof(struct mpc_lowcomm_ptp_message_lists_t*));

    if(!ret->ptp)
    {
        sctk_free(ret);
        mpc_common_spinlock_unlock(&proc_lock);

        return mqs_no_information;
    }

    int list_count = 128;

    if( _mpc_lowcomm_comm_get_lists(ret->global_rank, ret->ptp, &list_count ) != MPC_LOWCOMM_SUCCESS)
    {
        sctk_free(ret);
        mpc_common_spinlock_unlock(&proc_lock);

        return mqs_no_information;
    }

    if(list_count == 0)
    {
        sctk_free(ret->ptp);
        sctk_free(ret);
        mpc_common_spinlock_unlock(&proc_lock);

        return mqs_no_information;
    }

    ret->list_count = list_count;
    mpc_common_debug_error("MQD: proc %d has %d msg queues cnt (@%p)", ret->global_rank, ret->list_count, ret);

    current_exported_process_offset++;

    mpc_common_spinlock_unlock(&proc_lock);

    *process = ret;

    return mqs_ok;
}


int mqs_process_has_queues(mqs_process *process, char **message)
{
    *message = "MPC %s";
    return mqs_ok;
}

int mqs_destroy_process_info(mqs_process_info *processinfo)
{
    return mqs_ok;
}

/*******************
* QUERY FUNCTIONS *
*******************/
/* Communicator */

static void __push_comm_in_process(mpc_lowcomm_communicator_t comm, void * arg)
{
    struct mpc_lowcomm_mqs_process * proc = (struct mpc_lowcomm_mqs_process *)arg;

    int local_rank = mpc_lowcomm_communicator_rank_of(comm, proc->global_rank);

    /* Check if current proc belongs */
    if( local_rank == MPC_PROC_NULL )
    {
        return;
    }

    proc->comms = sctk_realloc(proc->comms, sizeof(mqs_communicator) * (proc->comm_count + 1));

    if(!proc->comms)
    {
        return;
    }

    mqs_communicator * new = &proc->comms[proc->comm_count];


    new->local_rank = local_rank;
    new->unique_id = (mqs_taddr_t)mpc_lowcomm_communicator_id(comm);
    new->size = mpc_lowcomm_communicator_size(comm);
    new->name[0] = '\0';

    proc->comm_count++;

}

int mqs_update_communicator_list(mqs_process *pprocess)
{
    struct mpc_lowcomm_mqs_process * process = (struct mpc_lowcomm_mqs_process *)*pprocess;
    if(process->comms)
    {
        /* Clear previous comms */
        sctk_free(process->comms);

        process->comms = NULL;
        process->comm_count = 0;
        process->comm_current_offset = 0;
    }

    /* Rebuild the list */
    mpc_lowcomm_communicator_scan(__push_comm_in_process, (void*)process);
    return mqs_ok;
}

int mqs_setup_communicator_iterator(mqs_process *process)
{
    (*process)->comm_current_offset = 0;
    return mqs_ok;
}

int mqs_get_communicator(mqs_process *pprocess, mqs_communicator *mqs_comm)
{
    struct mpc_lowcomm_mqs_process * process = (struct mpc_lowcomm_mqs_process *)*pprocess;

    if(!process->comms)
    {
        return mqs_no_information;
    }

    *mqs_comm = process->comms[process->comm_current_offset];
}

int mqs_get_comm_group(mqs_process *process, int *ranks)
{

}

int mqs_next_communicator(mqs_process *pprocess)
{
    struct mpc_lowcomm_mqs_process * process = (struct mpc_lowcomm_mqs_process *)*pprocess;

    if(process->comm_current_offset < (process->comm_count - 1))
    {
        process->comm_current_offset++;
        return mqs_ok;
    }

    return mqs_end_of_list;
}

/* Operations */



static void __unfold_msg_to_process(mpc_lowcomm_ptp_message_t *msg,
                                    struct mpc_lowcomm_mqs_process * process,
                                    mqs_op_class top,
                                    int same_proc,
                                    int is_recv)
{

    

    int msg_rank = MPC_PROC_NULL;

    if(is_recv)
    {
        msg_rank = SCTK_MSG_DEST_TASK(msg);
    }
    else
    {
        msg_rank = SCTK_MSG_SRC_TASK(msg);
    }


    if(same_proc)
    {
        if(msg_rank != process->global_rank)
        {
            return;
        }
    }
    else
    {
        if(msg_rank == process->global_rank)
        {
            return;
        }
    }

    process->ops = sctk_realloc(process->ops, sizeof(mqs_pending_operation) * (process->ops_count + 1));

    mqs_pending_operation *op = &process->ops[process->ops_count];

#if 0
typedef struct
{
	/* Field for all messages */
	int         status;
	mqs_tword_t desired_local_rank;
	mqs_tword_t desired_global_rank;
	int         tag_wild;
	mqs_tword_t desired_tag;
	mqs_taddr_t desired_length;
	int         system_buffer;
	mqs_taddr_t buffer;

	/* Fields valid if status >= matched */
	mqs_tword_t actual_local_rank;
	mqs_tword_t actual_global_rank;
	mqs_tword_t actual_tag;
	mqs_tword_t actual_length;

	char        extra_text[5][64];
}mqs_pending_operation;
#endif

    op->status = mqs_st_pending;

    if(is_recv)
    {
        op->desired_local_rank = SCTK_MSG_SRC_TASK(msg);
    }
    else
    {
        op->desired_local_rank = SCTK_MSG_DEST_TASK(msg);
    }

    mpc_lowcomm_communicator_print(msg->tail.communicator, 0);

    op->desired_global_rank = mpc_lowcomm_communicator_rank_of(msg->tail.communicator, op->desired_local_rank);

    if(SCTK_MSG_TAG(msg) == MPC_ANY_TAG)
    {
        op->tag_wild = 1;
    }
    else
    {
        op->tag_wild = 0;
    }

    op->desired_tag = SCTK_MSG_TAG(msg);
    op->desired_length = SCTK_MSG_SIZE(msg);


    mpc_common_debug_error("TAG %d LEN %d", op->desired_tag, op->desired_length);


    op->system_buffer = 0;

    op->buffer = NULL;

    if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
    {
        op->buffer = msg->tail.message.contiguous.addr;
        op->desired_length = msg->tail.message.contiguous.size;
    }

    process->ops_count = process->ops_count + 1;


    /* TODO handle completed ? */
}

static void __walk_msg_list_to_process(mpc_lowcomm_msg_list_t *list,
                                       struct mpc_lowcomm_mqs_process * process,
                                       mqs_op_class op,
                                       int same_proc,
                                       int is_recv)
{
    struct mpc_lowcomm_msg_list_s * cur = (struct mpc_lowcomm_msg_list_s *)list;

    while(cur)
    {
        __unfold_msg_to_process(cur->msg, process, op, same_proc, is_recv);
        cur = (struct mpc_lowcomm_msg_list_s *)cur->next;
    }
}




int mqs_setup_operation_iterator(mqs_process *pprocess, int opclass)
{
    struct mpc_lowcomm_mqs_process * process = (struct mpc_lowcomm_mqs_process *)*pprocess;
    if(process->ops)
    {
        /* Clear previous comms */
        sctk_free(process->ops);

        process->ops = NULL;
        process->ops_count = 0;
        process->ops_current_offset = 0;
    }


    mqs_op_class op = opclass;

    int do_send_queue = 0;
    int do_recv_queue = 0;
    int same_proc = 0;

    switch (op)
    {
    case mqs_pending_sends:
        do_send_queue = 1;
        do_recv_queue = 0;
        same_proc = 1;
        break;
    case mqs_pending_receives:
        do_send_queue = 0;
        do_recv_queue = 1;
        same_proc = 1;
        break;
    case mqs_unexpected_messages:
        do_send_queue = 1;
        do_recv_queue = 1;
        same_proc = 0;
        break;
    default:
        return mqs_no_information;
        break;
    }

    int i;

    for(i = 0 ; i < process->list_count; i++)
    {
        mpc_lowcomm_ptp_message_lists_t * ptp = process->ptp[i];

        if(!ptp)
        {
            continue;
        }

        /* Take the pending lock */
        mpc_common_spinlock_lock(&ptp->pending_lock);

        if(do_recv_queue)
        {
            __walk_msg_list_to_process(ptp->pending_recv.list, process, op, same_proc, 1);
        }

        if(do_send_queue)
        {
            __walk_msg_list_to_process(ptp->pending_send.list, process, op, same_proc, 0);
        }
    
        mpc_common_spinlock_unlock(&ptp->pending_lock);

        if(do_recv_queue)
        {
            mpc_common_spinlock_lock(&ptp->incomming_recv.lock);
            __walk_msg_list_to_process(ptp->incomming_recv.list, process, op, same_proc, 1);
            mpc_common_spinlock_unlock(&ptp->incomming_recv.lock);
        }

        if(do_send_queue)
        {
            mpc_common_spinlock_lock(&ptp->incomming_send.lock);
            __walk_msg_list_to_process(ptp->incomming_send.list, process, op, same_proc, 0);
            mpc_common_spinlock_unlock(&ptp->incomming_send.lock);
        }

    }

    return mqs_ok;
}

int mqs_next_operation(mqs_process *pprocess, mqs_pending_operation *op)
{
    struct mpc_lowcomm_mqs_process * process = (struct mpc_lowcomm_mqs_process *)*pprocess;

    if(process->ops_count <= process->ops_current_offset)
    {
        return mqs_end_of_list;
    }

    *op = process->ops[process->ops_current_offset];

    process->ops_current_offset++;

    return mqs_ok;    
}


/***************************
 * SETUP RELATED FUNCTIONS *
 ***************************/

void mqs_setup_basic_callbacks(const mqs_basic_callbacks *cb)
{
    /* NO OP */
}

char * mqs_version_string()
{
    static char buff[32];
    snprintf(buff, 32, "mqs@%s", MPC_VERSION_STRING);
    return buff;
}

int mqs_version_compatibility()
{
    return MQS_INTERFACE_COMPATIBILITY;
}

int mqs_dll_taddr_width()
{
    return sizeof(mqs_taddr_t);
}

char * mqs_dll_error_string(int error_code)
{
    mqs_err code = error_code;

    switch (code)
    {
        case mqs_ok:
            return "mqs_ok";
        case mqs_no_information:
            return "v";
        case mqs_end_of_list:
            return "mqs_end_of_list";
        case mqs_first_user_code:
            return "mqs_first_user_code";
    }

    return "No such error code";
}