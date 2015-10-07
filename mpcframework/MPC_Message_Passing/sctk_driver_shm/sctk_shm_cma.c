#define _GNU_SOURCE
#include <sys/uio.h>

#include "sctk_shm_cma.h"
#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

#ifdef MPC_USE_CMA
static void
sctk_network_rdma_msg_cmpl_shm_send(sctk_shm_iovec_info_t *shm_send_iov, vcli_cell_t * __cell)
{
    while(!__cell)
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, shm_send_iov->mpi_src);
	__cell->msg_type = SCTK_SHM_CMPL;
   	__cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	memcpy( __cell->data, shm_send_iov, sizeof(sctk_shm_iovec_info_t) );
    vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
}
#endif  /* MPC_USE_CMA */

sctk_thread_ptp_message_t *
sctk_network_rdma_cmpl_msg_shm_recv(vcli_cell_t * __cell)
{
    sctk_thread_ptp_message_t *msg = NULL; 
#ifdef MPC_USE_CMA
    sctk_shm_iovec_info_t *shm_iov;
	shm_iov = (sctk_shm_iovec_info_t*) __cell->data;
	msg = shm_iov->msg;
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    return msg;
#else   /* MPC_USE_CMA */
    assume_m(0, "Wrong msg type CMA | CPML without CMA SUPPORT");
#endif  /* MPC_USE_CMA */
    return msg;
}

#ifdef MPC_USE_CMA
static void 
sctk_shm_rdma_message_free_nocopy(void *tmp)
{
    return;
}
#endif  /* MPC_USE_CMA */

#ifdef MPC_USE_CMA
static void 
sctk_shm_rdma_message_copy_nocopy(sctk_message_to_copy_t * tmp)
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
			shm_send_iov = (sctk_shm_iovec_info_t *)(( char * ) send + sizeof ( sctk_thread_ptp_message_t ));
			send_iov->iov_base = shm_send_iov->ptr->iov_base;	
			send_iov->iov_len = size;	
			
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
    sctk_network_rdma_msg_cmpl_shm_send(shm_send_iov, NULL);
}
#endif  /* MPC_USE_CMA */


sctk_thread_ptp_message_t *
sctk_network_rdma_msg_shm_recv(vcli_cell_t * __cell,int copy_enabled)
{
    sctk_thread_ptp_message_t *msg = NULL; 
#ifdef MPC_USE_CMA
    size_t size;
    sctk_shm_iovec_info_t *shm_iov = NULL;
    struct iov * tmp;
    void *inb;
	
    msg = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t )  + sizeof(sctk_shm_iovec_info_t) );
    assume(msg != NULL);

    shm_iov = (sctk_shm_iovec_info_t *) ((char*) msg + sizeof ( sctk_thread_ptp_message_t ));
    memcpy( msg, __cell->data, sizeof ( sctk_thread_ptp_message_body_t ));       
    memcpy( shm_iov, __cell->data + sizeof ( sctk_thread_ptp_message_body_t ), sizeof(sctk_shm_iovec_info_t));
   	
    tmp = sctk_malloc( shm_iov->len * sizeof(struct iovec));
    assume( tmp != NULL);
    
    memcpy(tmp, (char *) __cell->data + sizeof ( sctk_thread_ptp_message_body_t ) + sizeof(sctk_shm_iovec_info_t), shm_iov->len * sizeof(struct iovec));
    shm_iov->ptr = (void*) tmp;
        
    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
        return NULL;

    sctk_rebuild_header ( msg );    
    sctk_reinit_header ( msg, sctk_shm_rdma_message_free_nocopy, sctk_shm_rdma_message_copy_nocopy);
    return msg;	
#else   /* MPC_USE_CMA */
    assume_m(0, "Wrong msg type CMA without CMA SUPPORT");
#endif  /* MPC_USE_CMA */
    return msg;
}

int
sctk_network_rdma_msg_shm_send(sctk_thread_ptp_message_t *msg,vcli_cell_t * __cell, int copy_enabled)
{
#ifdef MPC_USE_CMA
    struct iovec * tmp;
    sctk_shm_iovec_info_t * shm_iov;

    __cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	shm_iov = (sctk_shm_iovec_info_t *) sctk_malloc( 1 * sizeof( sctk_shm_iovec_info_t ));
    assume(shm_iov != NULL);
    
    __cell->msg_type = SCTK_SHM_RDMA;
	
    memcpy( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       
    shm_iov->msg = msg;
    shm_iov->pid = getpid();
    shm_iov->mpi_src = sctk_get_local_process_rank();
	
  	sctk_get_iovec_in_buffer( msg, &tmp, &(shm_iov->len));	
	shm_iov->ptr = (struct iovec*)((char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t) + sizeof(sctk_shm_iovec_info_t));
	
	assume( (char*) shm_iov->ptr + sizeof(struct iovec) < ( char*) __cell + VCLI_CELLS_SIZE );
	assume( sizeof(sctk_thread_ptp_message_body_t) + sizeof(sctk_shm_iovec_info_t) + sizeof(struct iovec) < VCLI_CELLS_SIZE );
	
	memcpy(( char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t), shm_iov, sizeof(sctk_shm_iovec_info_t));

	memcpy(shm_iov->ptr, tmp, shm_iov->len * sizeof(struct iovec));
    vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
    return 1;
#else   /* MPC_USE_CMA */
    return 0;
#endif  /* MPC_USE_CMA */
}

