#define _GNU_SOURCE
#include <sys/uio.h>

#include "sctk_shm_cma.h"
#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

static pid_t sctk_shm_process_sys_id = -1;
static unsigned long sctk_shm_process_mpi_local_id = -1;

static void 
sctk_shm_rdma_message_copy_generic(sctk_message_to_copy_t * tmp)
{
    vcli_cell_t * __cell = NULL;
	sctk_thread_ptp_message_t *send;
    sctk_thread_ptp_message_t *recv;
    char *body;
	struct iovec * recv_iov, * send_iov;
	sctk_shm_iovec_info_t * shm_send_iov;
	int nread;

    send = tmp->msg_send;
    recv = tmp->msg_recv;

    body = ( char * ) send + sizeof ( sctk_thread_ptp_message_t );
    SCTK_MSG_COMPLETION_FLAG_SET ( send , NULL );

    switch ( recv->tail.message_type )
    {
        case SCTK_MESSAGE_CONTIGUOUS:
        {
            size_t size;
            size = SCTK_MSG_SIZE ( send );
            size = sctk_min ( SCTK_MSG_SIZE ( send ), recv->tail.message.contiguous.size );
			
			recv_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
			recv_iov->iov_base = recv->tail.message.contiguous.addr;
			recv_iov->iov_len = size;	
			
			send_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
			shm_send_iov = (sctk_shm_iovec_info_t *) body;
            memcpy( send_iov, (char*)shm_send_iov+sizeof(sctk_shm_iovec_info_t),shm_send_iov->len*sizeof(struct iovec));  
			nread = process_vm_readv(shm_send_iov->pid, recv_iov, 1, send_iov, 1, 0);
            assume_m( nread > 0, "Failed process_vm_readv call");
			
            sctk_message_completion_and_free ( send, recv );
			sctk_free(recv_iov);
			sctk_free(send_iov);
            break;
        }
		default:
			abort();
	}
}

static void
sctk_network_rdma_msg_cmpl_shm_send(sctk_shm_iovec_info_t *shm_send_iov, vcli_cell_t * __cell)
{
	__cell->msg_type = SCTK_SHM_CMPL;
   	__cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	memcpy( __cell->data, shm_send_iov, sizeof(sctk_shm_iovec_info_t) );
    //fprintf(stderr, "[%d]resend cmpl to src : %d -> %d\n", nb_seq_rdma_send_cmpl++,sctk_shm_process_mpi_local_id, shm_send_iov->mpi_src);
    vcli_raw_push_cell_dest(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, shm_send_iov->mpi_src);       
}

sctk_thread_ptp_message_t *
sctk_network_rdma_cmpl_msg_shm_recv(vcli_cell_t * __cell)
{
    sctk_thread_ptp_message_t *msg = NULL; 
    sctk_shm_iovec_info_t *shm_iov;
	shm_iov = (sctk_shm_iovec_info_t*) __cell->data;
	msg = shm_iov->msg;
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    assume_m(msg != NULL, "Error in cma recv msg");
    return msg;
}


static void 
sctk_shm_rdma_message_free_nocopy(void *tmp)
{
    vcli_cell_t * __cell = NULL;
    sctk_shm_iovec_info_t *shm_iov = NULL; 
    shm_iov = (sctk_shm_iovec_info_t *) ((char*) tmp + sizeof(sctk_thread_ptp_message_t));
    __cell = container_of(tmp, vcli_cell_t, data);
    assume_m( __cell != NULL, "Error: sctk_shm_rdma_message_free_nocopy");
    sctk_network_rdma_msg_cmpl_shm_send( shm_iov, __cell );
}

static void 
sctk_shm_rdma_message_copy_nocopy(sctk_message_to_copy_t * tmp)
{
    sctk_shm_rdma_message_copy_generic( tmp );  
}

static sctk_thread_ptp_message_t *
sctk_network_preform_rdma_msg_shm_nocopy( vcli_cell_t * __cell)
{
    return (sctk_thread_ptp_message_t*) __cell->data;
}

static void 
sctk_shm_rdma_message_free_withcopy(void *tmp)
{
    vcli_cell_t * __cell = NULL;
    sctk_shm_iovec_info_t * shm_iov = NULL;
    shm_iov = (sctk_shm_iovec_info_t *)(( char * ) tmp + sizeof ( sctk_thread_ptp_message_t ));
    while(!__cell)
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, sctk_shm_process_mpi_local_id);
    sctk_network_rdma_msg_cmpl_shm_send(shm_iov, __cell);
    sctk_free(tmp);
}

static void 
sctk_shm_rdma_message_copy_withcopy(sctk_message_to_copy_t * tmp)
{
    sctk_shm_rdma_message_copy_generic(tmp);
}

static sctk_thread_ptp_message_t *
sctk_network_preform_rdma_msg_shm_withcopy( vcli_cell_t * __cell)
{
    sctk_shm_iovec_info_t *shm_iov = NULL;
    sctk_thread_ptp_message_t * msg = NULL;
    struct iovec *tmp = NULL;
    char *buffer_in, *buffer_out;
    
    msg = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t )  + sizeof(sctk_shm_iovec_info_t) );
    assume(msg != NULL);

    buffer_in = __cell->data;
    buffer_out = msg;
    memcpy( buffer_out, buffer_in, sizeof ( sctk_thread_ptp_message_body_t ));       
       
    buffer_out += sizeof ( sctk_thread_ptp_message_t );
    buffer_in += sizeof ( sctk_thread_ptp_message_t );
    memcpy( buffer_out, buffer_in, sizeof ( sctk_shm_iovec_info_t));       
    shm_iov = (sctk_shm_iovec_info_t *) buffer_out;

    buffer_out = sctk_malloc( shm_iov->len * sizeof(struct iovec));
    assume( buffer_out != NULL);
 
    buffer_in += sizeof(sctk_shm_iovec_info_t);
    memcpy( buffer_out, buffer_in, shm_iov->len * sizeof(struct iovec));
    shm_iov->ptr = (void*) buffer_out;

    return msg;
}


sctk_thread_ptp_message_t *
sctk_network_rdma_msg_shm_recv(vcli_cell_t * __cell,int copy_enabled)
{
    sctk_thread_ptp_message_t *msg; 
    void (*shm_free_funct)(void*) = NULL;
    void (*shm_copy_funct)(sctk_message_to_copy_t *) = NULL;

    if( copy_enabled )
    {
        msg = sctk_network_preform_rdma_msg_shm_withcopy( __cell );
        shm_free_funct = sctk_shm_rdma_message_free_withcopy;
        shm_copy_funct = sctk_shm_rdma_message_copy_withcopy;
    	vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    }
    else
    {
        msg = sctk_network_preform_rdma_msg_shm_nocopy( __cell );
        shm_free_funct = sctk_shm_rdma_message_free_nocopy;
        shm_copy_funct = sctk_shm_rdma_message_copy_nocopy;
    }

    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
        return NULL;
    sctk_rebuild_header ( msg );    
    sctk_reinit_header ( msg, shm_free_funct, shm_copy_funct); 
    return msg;
}

int
sctk_network_rdma_msg_shm_send(sctk_thread_ptp_message_t *msg, int dest, int copy_enabled)
{
    
    struct iovec * tmp;
    sctk_shm_iovec_info_t * shm_iov;
    vcli_cell_t * __cell = NULL;

    while(!__cell)
    {
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, sctk_shm_process_mpi_local_id);
        sched_yield();
    }
    
    __cell->msg_type = SCTK_SHM_RDMA;
    __cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	assume( sizeof(sctk_thread_ptp_message_t) + sizeof(sctk_shm_iovec_info_t) + sizeof(struct iovec) < VCLI_CELLS_SIZE );
    
    shm_iov = (sctk_shm_iovec_info_t *)((char*) __cell->data + sizeof(sctk_thread_ptp_message_t));
    shm_iov->msg = msg;
    shm_iov->pid = sctk_shm_process_sys_id;
    shm_iov->mpi_src = sctk_shm_process_mpi_local_id;
    shm_iov->ptr = (struct iovec*)((char*) shm_iov + sizeof(sctk_shm_iovec_info_t));
	assume( (char*) shm_iov->ptr + sizeof(struct iovec) < ( char*) __cell + VCLI_CELLS_SIZE );
	
    memcpy( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       
  	sctk_get_iovec_in_buffer( msg, &tmp, &(shm_iov->len));	
	
	memcpy(shm_iov->ptr, tmp, shm_iov->len * sizeof(struct iovec));
    vcli_raw_push_cell_dest( SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, dest);       
    return 1;
}

void 
sctk_network_rdma_shm_interface_init(void)
{
    sctk_shm_process_sys_id = getpid();
    sctk_shm_process_mpi_local_id = sctk_get_local_process_rank();
}
