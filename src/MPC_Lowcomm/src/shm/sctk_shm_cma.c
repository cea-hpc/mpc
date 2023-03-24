#define _GNU_SOURCE
#include <sys/uio.h>

#include <mpc_config.h>

#ifdef MPC_USE_CMA

#include <sctk_alloc.h>
#include "msg_cpy.h"
#include "sctk_shm_cma.h"


static pid_t sctk_shm_process_sys_id = -1;

#if 0
static void  sctk_shm_cma_driver_iovec(struct iovec* liovec, int liovlen, mpc_lowcomm_ptp_message_t* send, int type)
{
   int nread, riovlen, pid;
   struct iovec *riovec;
   sctk_shm_iovec_info_t *shm_send_iov;

   shm_send_iov = (sctk_shm_iovec_info_t *) ((char*) send + sizeof(mpc_lowcomm_ptp_message_t));
   riovlen = shm_send_iov->iovec_len;
   pid = shm_send_iov->pid;
    
   riovec = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
   memcpy( riovec, (char*) shm_send_iov + sizeof(sctk_shm_iovec_info_t), riovlen*sizeof(struct iovec));

   nread = process_vm_readv(pid, liovec, liovlen, riovec, riovlen, 0); 
   assume_m( nread > 0, "Failed process_vm_readv call use MPC_Config - cma_enable = false");
}
#endif

static void 
sctk_shm_cma_message_copy_generic(mpc_lowcomm_ptp_message_content_to_copy_t * tmp)
{

   mpc_lowcomm_ptp_message_t *send;
   mpc_lowcomm_ptp_message_t *recv;
   struct iovec * recv_iov, * send_iov;
   sctk_shm_iovec_info_t * shm_send_iov;
   int nread;

   send = tmp->msg_send;
   recv = tmp->msg_recv;


    SCTK_MSG_COMPLETION_FLAG_SET ( send , NULL );

    switch ( recv->tail.message_type )
    {
        case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
        {
            size_t size;
            size = SCTK_MSG_SIZE ( send );
            size = mpc_common_min ( SCTK_MSG_SIZE ( send ), recv->tail.message.contiguous.size );
			
   	    shm_send_iov = (sctk_shm_iovec_info_t *) ((char*) send + sizeof(mpc_lowcomm_ptp_message_t));
	    recv_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
	    recv_iov->iov_base = recv->tail.message.contiguous.addr;
	    recv_iov->iov_len = size;	

	    send_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
            memcpy( send_iov, (char*)shm_send_iov+sizeof(sctk_shm_iovec_info_t),shm_send_iov->iovec_len*sizeof(struct iovec));  
	    nread = process_vm_readv(shm_send_iov->pid, recv_iov, 1, send_iov, 1, 0);
            assume_m( nread > 0, "Failed process_vm_readv call");
			
            _mpc_comm_ptp_message_commit_request ( send, recv );
	    sctk_free(recv_iov);
	    sctk_free(send_iov);
            break;
         }
	 default:
	    abort();
	}
}

static void
sctk_network_cma_msg_cmpl_shm_send(sctk_shm_iovec_info_t *shm_send_iov,sctk_shm_cell_t * cell, __UNUSED__ int dest)
{
	cell->msg_type = SCTK_SHM_CMPL;
	memcpy(cell->data, shm_send_iov, sizeof(sctk_shm_iovec_info_t));
    	sctk_shm_send_cell(cell);       
}

mpc_lowcomm_ptp_message_t *
sctk_network_cma_cmpl_msg_shm_recv(sctk_shm_cell_t * cell)
{
    mpc_lowcomm_ptp_message_t *msg = NULL; 
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
    mpc_lowcomm_ptp_message_t *msg = (mpc_lowcomm_ptp_message_t *)tmp;

    dest = SCTK_MSG_SRC_PROCESS(msg); 
    shm_iov = (sctk_shm_iovec_info_t *) ((char*) tmp + sizeof(mpc_lowcomm_ptp_message_t));
    cell = container_of(tmp, sctk_shm_cell_t, data);
    sctk_network_cma_msg_cmpl_shm_send(shm_iov,cell,dest);
}

static void 
sctk_shm_cma_message_copy_nocopy(mpc_lowcomm_ptp_message_content_to_copy_t * tmp)
{
    sctk_shm_cma_message_copy_generic( tmp );  
}

static mpc_lowcomm_ptp_message_t *
sctk_network_preform_cma_msg_shm_nocopy( sctk_shm_cell_t * cell)
{
    return (mpc_lowcomm_ptp_message_t*) cell->data;
}

static void 
sctk_shm_cma_message_free_withcopy(void *tmp)
{
    int dest;
    mpc_lowcomm_ptp_message_t* msg  = (mpc_lowcomm_ptp_message_t*) tmp;
    sctk_shm_cell_t * cell = NULL;
    sctk_shm_iovec_info_t * shm_iov = NULL;
    shm_iov = (sctk_shm_iovec_info_t *)(( char * ) tmp + sizeof ( mpc_lowcomm_ptp_message_t ));

    int is_message_control = 0;

    if (_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg))) {
        is_message_control = 1;
    }


    dest = SCTK_MSG_SRC_PROCESS(msg); 
    while(!cell)
        cell = sctk_shm_get_cell(dest, is_message_control);
    sctk_network_cma_msg_cmpl_shm_send(shm_iov,cell,dest);
//    sctk_free(tmp);
}

static void 
sctk_shm_cma_message_copy_withcopy(mpc_lowcomm_ptp_message_content_to_copy_t * tmp)
{
	sctk_shm_cma_message_copy_generic(tmp);
}

static mpc_lowcomm_ptp_message_t *
sctk_network_preform_cma_msg_shm_withcopy( sctk_shm_cell_t * cell)
{
    sctk_shm_iovec_info_t *shm_iov = NULL;
    mpc_lowcomm_ptp_message_t * msg = NULL;

    char *buffer_in, *buffer_out;

    msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t)+sizeof(sctk_shm_iovec_info_t)  + sizeof(struct iovec));
    assume(msg != NULL);

    buffer_in = cell->data;
    buffer_out = (char*) msg;
    memcpy( buffer_out, buffer_in, sizeof ( mpc_lowcomm_ptp_message_body_t ));       
       
    buffer_out += sizeof ( mpc_lowcomm_ptp_message_t );
    buffer_in += sizeof ( mpc_lowcomm_ptp_message_t );
    memcpy( buffer_out, buffer_in, sizeof ( sctk_shm_iovec_info_t));       
    shm_iov = (sctk_shm_iovec_info_t *) buffer_out;
 
    buffer_in += sizeof(sctk_shm_iovec_info_t);
    buffer_out += sizeof(sctk_shm_iovec_info_t);
    memcpy( buffer_out, buffer_in, shm_iov->iovec_len * sizeof(struct iovec));

    return msg;
}


mpc_lowcomm_ptp_message_t *
sctk_network_cma_msg_shm_recv(sctk_shm_cell_t * cell,int copy_enabled)
{
    mpc_lowcomm_ptp_message_t *msg; 
    void (*shm_free_funct)(void*) = NULL;
    void (*shm_copy_funct)(mpc_lowcomm_ptp_message_content_to_copy_t *) = NULL;
    copy_enabled = 1;
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
    msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
        return NULL;
    _mpc_comm_ptp_message_clear_request ( msg );    
    _mpc_comm_ptp_message_set_copy_and_free ( msg, shm_free_funct, shm_copy_funct); 
    return msg;
}

int
sctk_network_cma_msg_shm_send(mpc_lowcomm_ptp_message_t *msg, sctk_shm_cell_t * cell)
{
    size_t size;  
    struct iovec * tmp;
    char * ptr;
    sctk_shm_iovec_info_t * shm_iov;


    if( msg->tail.message_type != MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	    return 0;

    size = sizeof(mpc_lowcomm_ptp_message_t) + sizeof(sctk_shm_iovec_info_t);
    size += sizeof(struct iovec);
    assume_m(size < 8*1024, "Too big message for one cell");

    cell->msg_type = SCTK_SHM_RDMA;
    memcpy( cell->data, (char*) msg, sizeof ( mpc_lowcomm_ptp_message_body_t ));       

    shm_iov = (sctk_shm_iovec_info_t *)((char*) cell->data + sizeof(mpc_lowcomm_ptp_message_t));
    shm_iov->msg = msg;
    shm_iov->pid = sctk_shm_process_sys_id;
    
    tmp = _mpc_lowcomm_msg_cpy_to_iovec(msg, &(shm_iov->iovec_len), SCTK_MSG_SIZE(msg));

    ptr = (char*) shm_iov + sizeof(sctk_shm_iovec_info_t);
    memcpy(ptr, tmp, shm_iov->iovec_len * sizeof(struct iovec));
    sctk_shm_send_cell(cell);       

    return 1;
}

int  
sctk_network_cma_shm_interface_init(__UNUSED__ void *options)
{
    sctk_shm_process_sys_id = getpid();
    //sctk_shm_cma_zerocopy_enabled = rail->runtime_config_driver_config->driver.value.shm.cells_num 
    return 1;
}

void sctk_shm_network_cma_shm_interface_free()
{
	sctk_shm_process_sys_id = -1;
}
#endif /* MPC_USE_CMA */
