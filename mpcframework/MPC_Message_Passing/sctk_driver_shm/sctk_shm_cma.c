#define _GNU_SOURCE
#include <sys/uio.h>

#include "sctk_net_tools.h"
#include "sctk_shm_cma.h"
#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

static pid_t sctk_shm_process_sys_id = -1;
static unsigned long sctk_shm_process_mpi_local_id = -1;
static int sctk_shm_cma_max_try = 10;


static void 
sctk_shm_cma_driver_iovec(struct iovec* liovec, int liovlen, sctk_thread_ptp_message_t* send)
{
   int nread, riovlen, pid;
   struct iovec *riovec;
   sctk_shm_iovec_info_t *shm_send_iov;

   shm_send_iov = (sctk_shm_iovec_info_t *) ((char*) send + sizeof(sctk_thread_ptp_message_t));
   riovlen = shm_send_iov->iovec_len;
   pid = shm_send_iov->pid;

   riovec = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
   memcpy( riovec, (char*)shm_send_iov+sizeof(sctk_shm_iovec_info_t),riovlen*sizeof(struct iovec));

   nread = process_vm_readv(pid, liovec, liovlen, riovec, riovlen, 0); 
   assume_m( nread > 0, "Failed process_vm_readv call");
}

static void
sctk_shm_cma_copy_iovec(sctk_message_to_copy_t * tmp)
{
	sctk_net_copy_msg_from_iovec(tmp, sctk_shm_cma_driver_iovec);
}


static void 
sctk_shm_cma_message_copy_generic(sctk_message_to_copy_t * tmp)
{
    sctk_shm_cell_t * cell = NULL;
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
			

            fast_memcpy( send_iov, (char*)shm_send_iov+sizeof(sctk_shm_iovec_info_t),shm_send_iov->iovec_len*sizeof(struct iovec));  
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
sctk_network_cma_msg_cmpl_shm_send(sctk_shm_iovec_info_t *shm_send_iov,sctk_shm_cell_t * cell, int dest)
{
	cell->msg_type = SCTK_SHM_CMPL;
	fast_memcpy(cell->data, shm_send_iov, sizeof(sctk_shm_iovec_info_t));
    	sctk_shm_send_cell(cell);       
}

sctk_thread_ptp_message_t *
sctk_network_cma_cmpl_msg_shm_recv(sctk_shm_cell_t * cell)
{
    sctk_thread_ptp_message_t *msg = NULL; 
    sctk_shm_iovec_info_t *shm_iov;
    shm_iov = (sctk_shm_iovec_info_t*) cell->data;
    msg = shm_iov->msg;
    sctk_shm_release_cell(cell);       
    assume_m(msg != NULL, "Error in cma recv msg");
    return msg;
}

static void 
sctk_shm_cma_message_free_nocopy(void *tmp)
{
    int dest;
    sctk_shm_cell_t * cell = NULL;
    sctk_shm_iovec_info_t *shm_iov = NULL; 
    sctk_thread_ptp_message_t *msg;

    dest = SCTK_MSG_SRC_PROCESS(msg); 
    shm_iov = (sctk_shm_iovec_info_t *) ((char*) tmp + sizeof(sctk_thread_ptp_message_t));
    cell = container_of(tmp, sctk_shm_cell_t, data);
    sctk_network_cma_msg_cmpl_shm_send(shm_iov,cell,dest);
}

static void 
sctk_shm_cma_message_copy_nocopy(sctk_message_to_copy_t * tmp)
{
	
    //sctk_shm_cma_message_copy_generic( tmp );  
}

static sctk_thread_ptp_message_t *
sctk_network_preform_cma_msg_shm_nocopy( sctk_shm_cell_t * cell)
{
    return (sctk_thread_ptp_message_t*) cell->data;
}

static void 
sctk_shm_cma_message_free_withcopy(void *tmp)
{
    int dest;
    sctk_thread_ptp_message_t* msg  = (sctk_thread_ptp_message_t*) tmp;
    sctk_shm_cell_t * cell = NULL;
    sctk_shm_iovec_info_t * shm_iov = NULL;
    shm_iov = (sctk_shm_iovec_info_t *)(( char * ) tmp + sizeof ( sctk_thread_ptp_message_t ));
    
    dest = SCTK_MSG_SRC_PROCESS(msg); 
    while(!cell)
        cell = sctk_shm_get_cell(dest);
    sctk_network_cma_msg_cmpl_shm_send(shm_iov,cell,dest);
    sctk_free(tmp);
}

static void 
sctk_shm_cma_message_copy_withcopy(sctk_message_to_copy_t * tmp)
{
    sctk_shm_cma_copy_iovec(tmp);
    //sctk_shm_cma_message_copy_generic(tmp);
}

static sctk_thread_ptp_message_t *
sctk_network_preform_cma_msg_shm_withcopy( sctk_shm_cell_t * cell)
{
    sctk_shm_iovec_info_t *shm_iov = NULL;
    sctk_thread_ptp_message_t * msg = NULL;
    struct iovec *tmp = NULL;
    char *buffer_in, *buffer_out;

    msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t)+sizeof(sctk_shm_iovec_info_t)  + sizeof(struct iovec));
    assume(msg != NULL);

    buffer_in = cell->data;
    buffer_out = (char*) msg;
    fast_memcpy( buffer_out, buffer_in, sizeof ( sctk_thread_ptp_message_body_t ));       
       
    buffer_out += sizeof ( sctk_thread_ptp_message_t );
    buffer_in += sizeof ( sctk_thread_ptp_message_t );
    fast_memcpy( buffer_out, buffer_in, sizeof ( sctk_shm_iovec_info_t));       
    shm_iov = (sctk_shm_iovec_info_t *) buffer_out;
 
    buffer_in += sizeof(sctk_shm_iovec_info_t);
    buffer_out += sizeof(sctk_shm_iovec_info_t);
    fast_memcpy( buffer_out, buffer_in, shm_iov->iovec_len * sizeof(struct iovec));

    return msg;
}


sctk_thread_ptp_message_t *
sctk_network_cma_msg_shm_recv(sctk_shm_cell_t * cell,int copy_enabled)
{
    sctk_thread_ptp_message_t *msg; 
    void (*shm_free_funct)(void*) = NULL;
    void (*shm_copy_funct)(sctk_message_to_copy_t *) = NULL;

    if( copy_enabled )
    {
        msg = sctk_network_preform_cma_msg_shm_withcopy(cell);
        shm_free_funct = sctk_shm_cma_message_free_withcopy;
        shm_copy_funct = sctk_shm_cma_message_copy_withcopy;
    	sctk_shm_release_cell(cell);       
    }
    else
    {
        msg = sctk_network_preform_cma_msg_shm_nocopy(cell);
        shm_free_funct = sctk_shm_cma_message_free_nocopy;
        shm_copy_funct = sctk_shm_cma_message_copy_nocopy;
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
sctk_network_cma_msg_shm_send(sctk_thread_ptp_message_t *msg, int dest)
{
    int try = 0;
    size_t size;  
    struct iovec * tmp;
    char * ptr;
    sctk_shm_iovec_info_t * shm_iov;
    sctk_shm_cell_t * cell = NULL;

    while(!cell && try < sctk_shm_cma_max_try)
    {
        cell = sctk_shm_get_cell(dest);
//	try++;
    }
    
    if( !cell ) 
        return 0;
 
    size = sizeof(sctk_thread_ptp_message_t) + sizeof(sctk_shm_iovec_info_t);
    size += sizeof(struct iovec);
    assume_m(size < 8*1024, "Too big message for one cell");

    cell->msg_type = SCTK_SHM_RDMA;
    fast_memcpy( cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       
    
    shm_iov = (sctk_shm_iovec_info_t *)((char*) cell->data + sizeof(sctk_thread_ptp_message_t));
    shm_iov->msg = msg;
    shm_iov->pid = sctk_shm_process_sys_id;
    
    tmp = sctk_net_convert_msg_to_iovec(msg, &(shm_iov->iovec_len), SCTK_MSG_SIZE(msg));
    ptr = (char*) shm_iov + sizeof(sctk_shm_iovec_info_t);
    memcpy(ptr, tmp, shm_iov->iovec_len * sizeof(struct iovec));
    
    sctk_shm_send_cell(cell);       
    return 1;
}

int  
sctk_network_cma_shm_interface_init(void *options)
{
    sctk_shm_process_sys_id = getpid();
    sctk_shm_process_mpi_local_id = sctk_get_local_process_rank();
    //sctk_shm_cma_zerocopy_enabled = rail->runtime_config_driver_config->driver.value.shm.cells_num 
    return 1;
}
