#include "sctk_shm_eager.h"
#include "sctk_net_tools.h"
#include "fast_memcpy.h"

/**
 * NO COPY FUNCTION
 */
static void 
sctk_shm_eager_message_free_nocopy ( void *tmp )
{
    sctk_shm_cell_t * cell = NULL;
    cell = container_of(tmp, sctk_shm_cell_t, data);
    sctk_shm_push_cell_origin(SCTK_SHM_CELLS_QUEUE_FREE, cell);       
}

static void 
sctk_shm_eager_message_copy_nocopy ( sctk_message_to_copy_t * tmp )
{
    sctk_net_message_copy(tmp);
}

static sctk_thread_ptp_message_t *
sctk_network_preform_eager_msg_shm_nocopy( sctk_shm_cell_t * cell )
{
    return (sctk_thread_ptp_message_t*) cell->data;
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
sctk_network_preform_eager_msg_shm_withcopy(sctk_shm_cell_t * cell)
{
    sctk_thread_ptp_message_t *msg; 
    msg = sctk_malloc ( cell->size );
    fast_memcpy( (char *) msg, cell->data, cell->size);       
    return msg;	
}


sctk_thread_ptp_message_t *
sctk_network_eager_msg_shm_recv(sctk_shm_cell_t * cell,int copy_enabled)
{ 
    
    sctk_thread_ptp_message_t *msg; 
    void (*shm_free_funct)(void*) = NULL;
    void (*shm_copy_funct)(sctk_message_to_copy_t *) = NULL;

    if( copy_enabled )
    {
        msg = sctk_network_preform_eager_msg_shm_withcopy( cell );
        shm_free_funct = sctk_shm_eager_message_free_withcopy;
        shm_copy_funct = sctk_shm_eager_message_copy_withcopy;
    	sctk_shm_push_cell_origin(SCTK_SHM_CELLS_QUEUE_FREE, cell);       
    }
    else
    {
        msg = sctk_network_preform_eager_msg_shm_nocopy( cell );
        shm_free_funct = sctk_shm_eager_message_free_nocopy;
        shm_copy_funct = sctk_shm_eager_message_copy_nocopy;
    }

    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
        return NULL;
    sctk_rebuild_header( msg ); 
    sctk_reinit_header( msg, shm_free_funct, shm_copy_funct);
    return msg;
}

int
sctk_network_eager_msg_shm_send(sctk_thread_ptp_message_t *msg, int dest)
{
    
    sctk_shm_cell_t * cell = NULL;
    
    if( SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_t ) > SCTK_SHM_CELL_SIZE)
        return 0;

    while(!cell) 
        cell = sctk_shm_pop_cell_free(dest);

    cell->size = SCTK_MSG_SIZE (msg) + sizeof ( sctk_thread_ptp_message_t );
	cell->msg_type = SCTK_SHM_EAGER;
    fast_memcpy( cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       

    if(SCTK_MSG_SIZE ( msg ) > 0)
        sctk_net_copy_in_buffer(msg,(char*)cell->data+sizeof(sctk_thread_ptp_message_t)); 
        
    sctk_shm_push_cell_dest(SCTK_SHM_CELLS_QUEUE_RECV, cell, dest);       
    sctk_complete_and_free_message( msg ); 
    return 1;
}

