#include "sctk_shm_frag.h"

static void
sctk_shm_rdv_free(void* ptr)
{
    sctk_shm_rdv_info_t* info = (sctk_shm_rdv_info_t*)(ptr - sizeof(sctk_shm_rdv_info_t));
    sctk_free(info);
}

static void
sctk_shm_rdv_message_copy(sctk_message_to_copy_t* tmp)
{
   sctk_shm_rdv_info_t* info = NULL;
   info = (sctk_shm_rdv_info_t*)((void*)tmp->msg_send - sizeof(sctk_shm_rdv_info_t));
   sctk_net_message_copy_from_buffer (info->msg,tmp,1);
}

static sctk_shm_rdv_info_t*
sctk_shm_init_send_frag_msg(size_t size)
{
   sctk_shm_rdv_info_t* info = sctk_malloc(sizeof(sctk_shm_rdv_info_t));
   info->addr = NULL;
   info->size_total = size;
   info->size_compute = 0;
   return info;
}

static sctk_shm_rdv_info_t*
sctk_shm_init_recv_frag_msg(size_t size)
{
   sctk_shm_rdv_info_t* info = sctk_malloc(sizeof(sctk_shm_rdv_info_t)+size);
   info->size_total = size - sizeof(sctk_thread_ptp_message_t);
   info->size_compute = 0;
   info->header = (sctk_thread_ptp_message_t*)((char*) info + sizeof(sctk_shm_rdv_info_t)) ;
   info->msg = (char*) info->header + sizeof(sctk_thread_ptp_message_t);
   return info;
}

static void 
sctk_shm_write_fragmented_msg( sctk_shm_cell_t* cell)
{
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;
   size_t size_to_copy;
   void *buffer_in, *buffer_out;

   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) cell->opaque_send;
   size_to_copy = sctk_shm_send_frag_info->size_total - sctk_shm_send_frag_info->size_compute;
   size_to_copy = (size_to_copy < SCTK_SHM_CELL_SIZE) ? size_to_copy : SCTK_SHM_CELL_SIZE;

   buffer_out = cell->data ;
   buffer_in = sctk_shm_send_frag_info->msg + sctk_shm_send_frag_info->size_compute;
   memcpy(buffer_out, buffer_in, size_to_copy);
   sctk_shm_send_frag_info->size_compute += size_to_copy;
   //cell->size = size_to_copy;
}

/************************************************
 *   Send function per class                    *
 * *********************************************/

sctk_thread_ptp_message_t * 
sctk_network_frag_msg_shm_resend(sctk_shm_cell_t* cell, int enabled_copy)
{
   int dest;
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;
   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) cell->opaque_send;
   cell->msg_type = SCTK_SHM_FRAG;

   sctk_shm_write_fragmented_msg( cell );
   dest = SCTK_MSG_SRC_PROCESS(sctk_shm_send_frag_info->header);
   sctk_shm_send_cell(cell);       

   if( sctk_shm_send_frag_info->size_total != sctk_shm_send_frag_info->size_compute )
        return NULL;

    sctk_thread_ptp_message_t * msg = sctk_shm_send_frag_info->header;
    sctk_free(sctk_shm_send_frag_info);
    return msg;   
}

int 
sctk_network_frag_msg_shm_send(sctk_thread_ptp_message_t* msg,int dest)
{
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;

   sctk_shm_cell_t * cell = NULL;
   while(!cell)
    cell = sctk_shm_get_cell(dest);

   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) cell->opaque_send;

   //cell->size = SCTK_MSG_SIZE (msg) + sizeof (sctk_thread_ptp_message_t); 
   cell->msg_type = SCTK_SHM_FRAG;

   sctk_shm_send_frag_info = sctk_shm_init_send_frag_msg(SCTK_MSG_SIZE(msg));
   sctk_shm_send_frag_info->header = msg;
   sctk_shm_send_frag_info->msg = (void*) msg->tail.message.contiguous.addr;

   cell->opaque_send = (void*) sctk_shm_send_frag_info;
   memcpy(cell->data, msg, sizeof(sctk_thread_ptp_message_body_t));
   sctk_shm_send_cell(cell);       
   return 1;
}

sctk_thread_ptp_message_t * 
sctk_network_frag_msg_shm_recv(sctk_shm_cell_t* cell, int enabled_copy)
{
   int src;
   size_t size;
   void *buffer_in, *buffer_out;
   sctk_thread_ptp_message_t *msg = NULL;
   sctk_shm_rdv_info_t* sctk_shm_recv_frag_info;
   sctk_shm_recv_frag_info = (sctk_shm_rdv_info_t*) cell->opaque_recv;

   if( sctk_shm_recv_frag_info == NULL)
   {
      msg = (sctk_thread_ptp_message_t *) cell->data; 
      size = SCTK_MSG_SIZE (msg) + sizeof(sctk_thread_ptp_message_t);
      sctk_shm_recv_frag_info = sctk_shm_init_recv_frag_msg(size);
      sctk_shm_recv_frag_info->addr = cell->opaque_send;
      cell->opaque_recv = (void*) sctk_shm_recv_frag_info;
      /* copy and reinit ptp thread header */
      msg = sctk_shm_recv_frag_info->header;
      memcpy( msg, cell->data, sizeof(sctk_thread_ptp_message_body_t));
      msg->body.completion_flag = NULL;
      msg->tail.message_type = SCTK_MESSAGE_NETWORK;
      sctk_rebuild_header(msg);
      sctk_reinit_header(msg, sctk_shm_rdv_free, sctk_shm_rdv_message_copy);
   }
   else
   {
      size = sctk_shm_recv_frag_info->size_total - sctk_shm_recv_frag_info->size_compute;
      size = (size < SCTK_SHM_CELL_SIZE) ? size : SCTK_SHM_CELL_SIZE;
      buffer_in = sctk_shm_recv_frag_info->msg + sctk_shm_recv_frag_info->size_compute;
      buffer_out = cell->data;
      memcpy(buffer_in, buffer_out, size);
      sctk_shm_recv_frag_info->size_compute += size;
   }

    if(sctk_shm_recv_frag_info->size_total != sctk_shm_recv_frag_info->size_compute)
    {
        src = SCTK_MSG_SRC_PROCESS(sctk_shm_recv_frag_info->header);
        cell->msg_type = SCTK_SHM_ACK;
        sctk_shm_send_cell(cell);
        return NULL;
    }

    msg = sctk_shm_recv_frag_info->header;
    sctk_shm_recv_frag_info->is_ready_to_send = 1;
    cell->opaque_send = NULL;
    cell->opaque_recv = NULL;
    sctk_shm_release_cell(cell);
    return msg;
}


