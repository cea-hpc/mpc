#include "sctk_shm_eager.h"
#include "sctk_net_tools.h"

static int sctk_shm_eager_max_try = 10;

/**
 * NO COPY FUNCTION
 */
static void 
sctk_shm_eager_message_free_nocopy ( void *tmp )
{
    sctk_shm_cell_t * cell = NULL;
    cell = container_of(tmp, sctk_shm_cell_t, data);
    sctk_shm_release_cell(cell);
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
    size_t size;
    sctk_thread_ptp_message_t *msg, *tmp; 
    tmp = (sctk_thread_ptp_message_t *) cell->data;
    size = SCTK_MSG_SIZE (tmp) + sizeof(sctk_thread_ptp_message_t); 
    msg = sctk_malloc (size);
    memcpy( (char *) msg, cell->data, size);       
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
        msg = sctk_network_preform_eager_msg_shm_withcopy(cell);
        shm_free_funct = sctk_shm_eager_message_free_withcopy;
        shm_copy_funct = sctk_shm_eager_message_copy_withcopy;
        sctk_shm_release_cell(cell);
    }
    else
    {
        msg = sctk_network_preform_eager_msg_shm_nocopy(cell);
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
    int try = 0;
    sctk_shm_cell_t * cell = NULL;
    struct iovec *to_send = NULL; 
    
    if(SCTK_MSG_SIZE(msg)+sizeof(sctk_thread_ptp_message_t) > SCTK_SHM_CELL_SIZE)
        return 0;

    while(!cell && try < sctk_shm_eager_max_try ) 
    {
        cell = sctk_shm_get_cell(dest);
	try++;
    }

    if( !cell )
    {
	return 0;
    }

    cell->msg_type = SCTK_SHM_EAGER;
    to_send = (struct iovec *) malloc(sizeof(struct iovec)); 
    memcpy(cell->data,(char*)msg,sizeof(sctk_thread_ptp_message_body_t));       

    if(SCTK_MSG_SIZE(msg) > 0)
        sctk_net_copy_in_buffer(msg,(char*)cell->data+sizeof(sctk_thread_ptp_message_t)); 
        
    sctk_shm_send_cell(cell);
    sctk_complete_and_free_message( msg ); 

    return 1;
}

int sctk_network_eager_msg_shm_interface_init( void )
{
	return 1;
}
