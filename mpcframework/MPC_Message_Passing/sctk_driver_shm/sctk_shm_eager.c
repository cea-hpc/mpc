#include "sctk_shm_eager.h"
#include "sctk_net_tools.h"


/**
 * NO COPY FUNCTION
 */
static void 
sctk_shm_eager_message_free_nocopy ( void *tmp )
{
    vcli_cell_t * __cell = NULL;
    __cell = container_of(tmp, vcli_cell_t, data);
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
}

static void 
sctk_shm_eager_message_copy_nocopy ( sctk_message_to_copy_t * tmp )
{
    sctk_net_message_copy(tmp);
}

static sctk_thread_ptp_message_t *
sctk_network_preform_eager_msg_shm_nocopy( vcli_cell_t * __cell )
{
    return (sctk_thread_ptp_message_t*) __cell->data;
}

/**
 * COPY FUNCTION
 */
static void 
sctk_shm_eager_message_free_withcopy ( void *tmp )
{
    sctk_free(tmp);
}

static void 
sctk_shm_eager_message_copy_withcopy ( sctk_message_to_copy_t * tmp )
{
    sctk_net_message_copy(tmp);
}

static sctk_thread_ptp_message_t *
sctk_network_preform_eager_msg_shm_withcopy(vcli_cell_t * __cell)
{
    sctk_thread_ptp_message_t *msg; 
    msg = sctk_malloc ( __cell->size );
    memcpy( (char *) msg, __cell->data, __cell->size);       
    return msg;	
}


sctk_thread_ptp_message_t *
sctk_network_eager_msg_shm_recv(vcli_cell_t * __cell,int copy_enabled)
{ 
    
    sctk_thread_ptp_message_t *msg; 
    void (*shm_free_funct)(void*) = NULL;
    void (*shm_copy_funct)(sctk_message_to_copy_t *) = NULL;

    if( copy_enabled )
    {
        msg = sctk_network_preform_eager_msg_shm_withcopy( __cell );
        shm_free_funct = sctk_shm_eager_message_free_withcopy;
        shm_copy_funct = sctk_shm_eager_message_copy_withcopy;
    	vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    }
    else
    {
        msg = sctk_network_preform_eager_msg_shm_nocopy( __cell );
        shm_free_funct = sctk_shm_eager_message_free_nocopy;
        shm_copy_funct = sctk_shm_eager_message_copy_nocopy;
    }

    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
        return NULL;
    sctk_rebuild_header ( msg ); 
    sctk_reinit_header ( msg, shm_free_funct, shm_copy_funct);
    return msg;
}

static int sctk_shm_eager_number_try = 0;
int
sctk_network_eager_msg_shm_send(sctk_thread_ptp_message_t *msg, int dest, int copy_enabled)
{
    
    vcli_cell_t * __cell = NULL;
    
    if( SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_t ) > 4*1024)
        return 0;

    while(!__cell && sctk_shm_eager_number_try < 1000)
    {
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, dest);
    }

    __cell->size = SCTK_MSG_SIZE (msg) + sizeof ( sctk_thread_ptp_message_t );
	__cell->msg_type = SCTK_SHM_EAGER;
    memcpy( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_t ));       

    if(SCTK_MSG_SIZE ( msg ) > 0)
        sctk_net_copy_in_buffer(msg,(char*)__cell->data+sizeof(sctk_thread_ptp_message_t)); 
        
    vcli_raw_push_cell_dest(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, dest);       
    sctk_complete_and_free_message( msg ); 
    return 1;
}

