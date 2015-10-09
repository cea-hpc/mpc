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
sctk_shm_write_fragmented_msg( vcli_cell_t* __cell)
{
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;
   size_t empty_space, size_to_copy;
   void *buffer_in, *buffer_out;

   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) __cell->opaque_send;
   empty_space = VCLI_CELLS_SIZE / 2 ;
   size_to_copy = sctk_shm_send_frag_info->size_total - sctk_shm_send_frag_info->size_compute;
   size_to_copy = (size_to_copy < empty_space) ? size_to_copy : empty_space;

   buffer_out = __cell->data ;
   buffer_in = sctk_shm_send_frag_info->msg + sctk_shm_send_frag_info->size_compute;
   memcpy(buffer_out, buffer_in, size_to_copy);
   sctk_shm_send_frag_info->size_compute += size_to_copy;
   __cell->size = size_to_copy;
}

/************************************************
 *   Send function per class                    *
 * *********************************************/

sctk_thread_ptp_message_t * 
sctk_network_frag_msg_shm_resend(vcli_cell_t* __cell, int dest, int enabled_copy)
{
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;
   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) __cell->opaque_send;
   __cell->msg_type = SCTK_SHM_FRAG;

   sctk_shm_write_fragmented_msg( __cell );
   vcli_raw_push_cell_dest(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, dest);       

   //fprintf(stderr, "send %d / %d\n", sctk_shm_send_frag_info->size_compute, sctk_shm_send_frag_info->size_total); 
   if( sctk_shm_send_frag_info->size_total != sctk_shm_send_frag_info->size_compute )
        return NULL;

    sctk_thread_ptp_message_t * msg = sctk_shm_send_frag_info->header;
    sctk_free(sctk_shm_send_frag_info);
    return msg;   
}

int 
sctk_network_frag_msg_shm_send(sctk_thread_ptp_message_t* msg,int dest,int enabled_copy)
{
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;

   vcli_cell_t * __cell = NULL;
   while(!__cell)
    __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID,dest);

   sctk_shm_send_frag_info = (sctk_shm_rdv_info_t*) __cell->opaque_send;

   __cell->size = SCTK_MSG_SIZE (msg) + sizeof ( sctk_thread_ptp_message_t ); 
   __cell->src = sctk_get_local_process_rank();
   __cell->dest = dest;
   __cell->msg_type = SCTK_SHM_FRAG;

   sctk_shm_send_frag_info = sctk_shm_init_send_frag_msg(SCTK_MSG_SIZE (msg));
   sctk_shm_send_frag_info->header = msg;
   sctk_shm_send_frag_info->msg = (void*) msg->tail.message.contiguous.addr;

   __cell->opaque_send = (void*) sctk_shm_send_frag_info;
   memcpy(__cell->data, msg, sizeof(sctk_thread_ptp_message_body_t));
   vcli_raw_push_cell_dest(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, dest);       
   return 1;
}

sctk_thread_ptp_message_t * 
sctk_network_frag_msg_shm_recv(vcli_cell_t* __cell, int enabled_copy)
{
   void *buffer_in, *buffer_out;
   sctk_thread_ptp_message_t *msg = NULL;
   sctk_shm_rdv_info_t* sctk_shm_recv_frag_info;
   sctk_shm_recv_frag_info = (sctk_shm_rdv_info_t*) __cell->opaque_recv;

   if( sctk_shm_recv_frag_info == NULL)
   {
      sctk_shm_recv_frag_info = sctk_shm_init_recv_frag_msg( __cell->size);
      sctk_shm_recv_frag_info->addr = __cell->opaque_send;
      __cell->opaque_recv = (void*) sctk_shm_recv_frag_info;
      /* copy and reinit ptp thread header */
      msg = sctk_shm_recv_frag_info->header;
      memcpy( msg, __cell->data, sizeof(sctk_thread_ptp_message_body_t));
      msg->body.completion_flag = NULL;
      msg->tail.message_type = SCTK_MESSAGE_NETWORK;
      sctk_rebuild_header(msg);
      sctk_reinit_header(msg, sctk_shm_rdv_free, sctk_shm_rdv_message_copy);
   }
   else
   {
      buffer_in = sctk_shm_recv_frag_info->msg + sctk_shm_recv_frag_info->size_compute;
      buffer_out = __cell->data;
      memcpy(buffer_in, buffer_out, __cell->size);
      sctk_shm_recv_frag_info->size_compute += __cell->size;
   }
    //fprintf(stderr, "Recv %d / %d\n", sctk_shm_recv_frag_info->size_compute, sctk_shm_recv_frag_info->size_total); 
    if(sctk_shm_recv_frag_info->size_total != sctk_shm_recv_frag_info->size_compute)
    {
        __cell->msg_type = SCTK_SHM_ACK;
        vcli_raw_push_cell_dest(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell, __cell->src);
        return NULL;
    }

    msg = sctk_shm_recv_frag_info->header;
    sctk_shm_recv_frag_info->is_ready_to_send = 1;
    __cell->opaque_send = NULL;
    __cell->opaque_recv = NULL;
    vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);
    return msg;
}


