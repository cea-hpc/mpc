#include "sctk_shm_eager.h"
#include "msg_cpy.h"

#include <sctk_alloc.h>

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
sctk_shm_eager_message_copy_nocopy ( mpc_lowcomm_ptp_message_content_to_copy_t * tmp )
{
    _mpc_lowcomm_msg_cpy(tmp);
}

static mpc_lowcomm_ptp_message_t *
sctk_network_preform_eager_msg_shm_nocopy( sctk_shm_cell_t * cell )
{
    return (mpc_lowcomm_ptp_message_t*) cell->data;
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
sctk_shm_eager_message_copy_withcopy ( mpc_lowcomm_ptp_message_content_to_copy_t * tmp )
{
    _mpc_lowcomm_msg_cpy(tmp);
}

static mpc_lowcomm_ptp_message_t *
sctk_network_preform_eager_msg_shm_withcopy(sctk_shm_cell_t * cell)
{
    size_t size;
    mpc_lowcomm_ptp_message_t *msg, *tmp; 
    tmp = (mpc_lowcomm_ptp_message_t *) cell->data;
    size = SCTK_MSG_SIZE (tmp) + sizeof(mpc_lowcomm_ptp_message_t); 
    msg = sctk_malloc (size);
    memcpy( (char *) msg, cell->data, size);       
    return msg;	
}

mpc_lowcomm_ptp_message_t *
sctk_network_eager_msg_shm_recv(sctk_shm_cell_t *cell, int copy_enabled) {

  mpc_lowcomm_ptp_message_t *msg, *tmp;
  void (*shm_free_funct)(void *) = NULL;
  void (*shm_copy_funct)(mpc_lowcomm_ptp_message_content_to_copy_t *) = NULL;

  tmp = (mpc_lowcomm_ptp_message_t *)cell->data;

  

  // if ctrl message disable no copy
  if (_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(tmp))) {
    copy_enabled = 1;
  }

  if (copy_enabled) {
    msg = sctk_network_preform_eager_msg_shm_withcopy(cell);
    shm_free_funct = sctk_shm_eager_message_free_withcopy;
    shm_copy_funct = sctk_shm_eager_message_copy_withcopy;
    sctk_shm_release_cell(cell);
  } else {
    msg = sctk_network_preform_eager_msg_shm_nocopy(cell);
    shm_free_funct = sctk_shm_eager_message_free_nocopy;
    shm_copy_funct = sctk_shm_eager_message_copy_nocopy;
  }

  SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
  msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
  if (SCTK_MSG_COMMUNICATOR_ID(msg) == MPC_LOWCOMM_COMM_NULL_ID)
    return NULL;

  _mpc_comm_ptp_message_clear_request(msg);
  _mpc_comm_ptp_message_set_copy_and_free(msg, shm_free_funct, shm_copy_funct);
  return msg;
}

int
sctk_network_eager_msg_shm_send(mpc_lowcomm_ptp_message_t *msg, sctk_shm_cell_t *cell)
{
    if(SCTK_MSG_SIZE(msg)+sizeof(mpc_lowcomm_ptp_message_t) > SCTK_SHM_CELL_SIZE)
        return 0;

    cell->msg_type = SCTK_SHM_EAGER;
    memcpy(cell->data,(char*)msg,sizeof(mpc_lowcomm_ptp_message_body_t));       

    if(SCTK_MSG_SIZE(msg) > 0)
        _mpc_lowcomm_msg_cpy_in_buffer(msg,(char*)cell->data+sizeof(mpc_lowcomm_ptp_message_t)); 
        
    sctk_shm_send_cell(cell);
    mpc_lowcomm_ptp_message_complete_and_free( msg ); 

    return 1;
}

int sctk_network_eager_msg_shm_interface_init( void )
{
	return 1;
}
