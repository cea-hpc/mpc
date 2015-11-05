#include "sctk_shm_frag.h"

#define SCTK_SHM_MAX_SEQ_NUMBER 16777208 /* ( 2 << 24 - 8) */
#define SCTK_SHM_MAX_PROC_ID 64 /* ( 2 << 8 - 1) */

 
static struct MPCHT sctk_shm_recving_frag_hastable_ptr;
static struct MPCHT sctk_shm_sending_frag_hastable_ptr;
static struct MPCHT sctk_shm_pending_frag_hastable_ptr;

static sctk_spinlock_t sckt_shm_frag_hashtable_lock = SCTK_SPINLOCK_INITIALIZER;
static volatile int sctk_shm_frag_send_msg_num = 0;

static int sctk_shm_process_hash_id = 0;
static int sctk_shm_process_msg_id = 0;
static int sctk_shm_frag_max_try = 10;

typedef enum {SCTK_SHM_SENDER_HT, SCTK_SHM_RECVER_HT} sctk_shm_table_t;
  
static sctk_shm_rdv_info_t*
sctk_shm_frag_get_elt_from_hash(int key, sctk_shm_table_t table_type)
{
	void * tmp;
	switch(table_type)
	{
		case SCTK_SHM_SENDER_HT:
			tmp = MPCHT_get(&sctk_shm_sending_frag_hastable_ptr, key);
			break;
		case SCTK_SHM_RECVER_HT:
			tmp = MPCHT_get(&sctk_shm_recving_frag_hastable_ptr, key);
			break;
		default:
			assume_m(0, "Unknown shm hastable type");
	}
    return (sctk_shm_rdv_info_t*) tmp; 
}

static void
sctk_shm_frag_add_elt_to_hash(int key, sctk_shm_rdv_info_t* elt, sctk_shm_table_t table_type)
{
	switch(table_type)
	{
		case SCTK_SHM_SENDER_HT:
			MPCHT_set(&sctk_shm_sending_frag_hastable_ptr, key, (void*) elt);
			break;
		case SCTK_SHM_RECVER_HT:
			MPCHT_set(&sctk_shm_recving_frag_hastable_ptr, key, (void*) elt);
			break;
		default:
			assume_m(0, "Unknown shm hastable type");
	}
}

static void
sctk_shm_frag_del_elt_from_hash(int key, sctk_shm_table_t table_type)
{
	switch(table_type)
	{
		case SCTK_SHM_SENDER_HT:
			MPCHT_delete(&sctk_shm_sending_frag_hastable_ptr, key);
			break;
		case SCTK_SHM_RECVER_HT:
			MPCHT_delete(&sctk_shm_recving_frag_hastable_ptr, key);
			break;
		default:
			assume_m(0, "Unknown shm hastable type");
	}
}

static sctk_shm_rdv_info_t* 
sctk_shm_init_send_frag_msg(size_t size, int *key)
{
    sctk_shm_rdv_info_t* info = NULL;

    //*key = ( sctk_local_process_rank << 8 ) + sctk_shm_process_msg_id;
    *key = sctk_shm_process_msg_id;
    sctk_shm_process_msg_id = (sctk_shm_process_msg_id+1) % SCTK_SHM_MAX_PROC_ID;

    sctk_spinlock_lock(&sckt_shm_frag_hashtable_lock); 
    while(sctk_shm_frag_get_elt_from_hash(*key, SCTK_SHM_SENDER_HT))
    {
    	sctk_shm_process_msg_id = (sctk_shm_process_msg_id+1) % SCTK_SHM_MAX_PROC_ID;
        //*key = ( sctk_local_process_rank << 8 ) + sctk_shm_process_msg_id;
        *key = sctk_shm_process_msg_id;
    }
    
    info = sctk_malloc(sizeof(sctk_shm_rdv_info_t));     
    sctk_shm_frag_add_elt_to_hash(*key, info, SCTK_SHM_SENDER_HT);

    sctk_spinlock_unlock(&sckt_shm_frag_hashtable_lock); 
    
    info->key = *key;
    info->addr = NULL;
    info->size_total = size;
    info->size_compute = 0;

    return info;
}

static sctk_shm_rdv_info_t* 
sctk_shm_init_recv_frag_msg(int key, size_t size)
{
   sctk_shm_rdv_info_t* info = NULL;


   if(sctk_shm_frag_get_elt_from_hash(key,SCTK_SHM_RECVER_HT))
        assume_m(0, "msg already commit to hash table ...");

   info = sctk_malloc(sizeof(sctk_shm_rdv_info_t)+size);

   while(sctk_shm_frag_get_elt_from_hash(key, SCTK_SHM_RECVER_HT))
	;

   sctk_spinlock_lock(&sckt_shm_frag_hashtable_lock);
   sctk_shm_frag_add_elt_to_hash(key,info, SCTK_SHM_RECVER_HT);
   sctk_spinlock_unlock(&sckt_shm_frag_hashtable_lock);
    
   info->size_total = size - sizeof(sctk_thread_ptp_message_t);
   info->size_compute = 0;
   info->header = (sctk_thread_ptp_message_t*)((char*) info + sizeof(sctk_shm_rdv_info_t)) ;
   info->msg = (char*) info->header + sizeof(sctk_thread_ptp_message_t);
   return info;
}

static void
sctk_shm_rdv_free(void* ptr)
{
    sctk_shm_rdv_info_t* info = (sctk_shm_rdv_info_t*)(ptr - sizeof(sctk_shm_rdv_info_t));
    sctk_free(info);
}

static void
sctk_shm_rdv_message_copy(sctk_message_to_copy_t* tmp)
{
   sctk_net_message_copy(tmp);
  // sctk_shm_rdv_info_t* info = NULL;
 //  info = (sctk_shm_rdv_info_t*)((void*)tmp->msg_send - sizeof(sctk_shm_rdv_info_t));
  // fprintf(stderr, "COPY msg addr : %p\n", info->msg);
  // sctk_net_message_copy_from_buffer (info->,tmp,1);
}

static void 
sctk_shm_write_fragmented_msg( sctk_shm_cell_t* cell, sctk_shm_rdv_info_t* infos)
{
   size_t size_to_copy;
   void *buffer_in, *buffer_out;

   size_to_copy = infos->size_total - infos->size_compute;
   size_to_copy = (size_to_copy < SCTK_SHM_CELL_SIZE) ? size_to_copy : SCTK_SHM_CELL_SIZE;

   buffer_out = cell->data;
   buffer_in = infos->msg + infos->size_compute;
   memcpy(buffer_out, buffer_in, size_to_copy);
   infos->size_compute += size_to_copy;
}

/************************************************
 *   Send function per class                    *
 * *********************************************/

static int sctk_try_send_first_msg = 0;

static sctk_shm_rdv_info_t*
sctk_network_frag_msg_first_send(sctk_thread_ptp_message_t* msg, int dest)
{
   int key, try;
   sctk_shm_cell_t * cell = NULL;
   sctk_shm_rdv_info_t* sctk_shm_send_frag_info;
   int sctk_try_send_first_msg = 0;
   try = 0;

   while(!cell && try < sctk_shm_frag_max_try )
   	cell = sctk_shm_get_cell(dest);

   sctk_shm_send_frag_info = sctk_shm_init_send_frag_msg(SCTK_MSG_SIZE(msg), &key);
   sctk_shm_send_frag_info->header = msg;
   sctk_shm_send_frag_info->dest = dest;
   sctk_shm_send_frag_info->msg = (void*) msg->tail.message.contiguous.addr;

   memcpy(cell->data, msg, sizeof(sctk_thread_ptp_message_body_t));

   cell->msg_type = SCTK_SHM_FIRST_FRAG;
   cell->frag_hkey = ( sctk_local_process_rank << 8 ) + key;
   //fprintf(stderr, "(%d-%d) [FIRST - SEND]\tKey : %d\n", sctk_local_process_rank, dest, cell->frag_hkey);
   sctk_shm_frag_send_msg_num++;
   sctk_shm_send_cell(cell);       
   return sctk_shm_send_frag_info;
}

static int 
sctk_network_frag_msg_try_send(sctk_shm_rdv_info_t* infos, int retry)
{
   int old_value = retry;
   sctk_shm_cell_t * cell = NULL;

 //  while(infos->size_total != infos->size_compute )
   {
        do
        {       
            cell = sctk_shm_get_cell(infos->dest);
        }while(!cell && retry);
        
        if(!cell)
		return;
//		break;

        retry--;
   	cell->msg_type = SCTK_SHM_NEXT_FRAG;
        cell->frag_hkey = ( sctk_local_process_rank << 8 ) + infos->key;
        sctk_shm_write_fragmented_msg(cell, infos);
      	//fprintf(stderr, "(%d-%d) [NEXT - SEND]\tKey:%d\t%lu/%lu\n", sctk_local_process_rank, infos->dest, cell->frag_hkey,infos->size_compute,infos->size_total);
        sctk_shm_send_cell(cell); 
   }

   if(infos->size_total == infos->size_compute)
   {
        sctk_thread_ptp_message_t *msg = infos->header;
        sctk_shm_frag_del_elt_from_hash(infos->key, SCTK_SHM_SENDER_HT);
	sctk_shm_frag_send_msg_num--;
        sctk_free(infos);
        sctk_complete_and_free_message(msg) ;
   }

   return (!retry) ? retry : old_value-retry;
}

void
sctk_network_frag_msg_shm_idle(int max_try)
{
    sctk_shm_rdv_info_t* infos = NULL;
	
    if(!sctk_shm_frag_send_msg_num)
	return;

    MPC_HT_ITER( &sctk_shm_sending_frag_hastable_ptr, infos )
    sctk_network_frag_msg_try_send(infos, 1);
    MPC_HT_ITER_END
}

int 
sctk_network_frag_msg_shm_send(sctk_thread_ptp_message_t* msg, int dest)
{
   sctk_shm_rdv_info_t* infos;
   return 0; 
   //if( SCTK_MSG_SIZE(msg) > 3 * SCTK_SHM_CELL_SIZE)
//   if( SCTK_MSG_SIZE(msg) > 524288)
 //       return 0;

   infos = sctk_network_frag_msg_first_send(msg, dest);
   sctk_network_frag_msg_try_send(infos, 1);
   return 1;
}

sctk_thread_ptp_message_t *
sctk_network_frag_msg_shm_recv(sctk_shm_cell_t* cell, int enabled_copy)
{
   int src, hkey;
   size_t size;
   void *buffer_in, *buffer_out;
   sctk_thread_ptp_message_t *msg = NULL;
   sctk_shm_rdv_info_t* sctk_shm_recv_frag_info;
    
   hkey = cell->frag_hkey; 

   if( cell->msg_type == SCTK_SHM_FIRST_FRAG )
   {
      msg = (sctk_thread_ptp_message_t *) cell->data; 
      //fprintf(stderr, "(%p-%d-%d) [FIRST - RECV]\tKey : %d\n", cell, sctk_local_process_rank, SCTK_MSG_SRC_PROCESS(msg), cell->frag_hkey);
      size = SCTK_MSG_SIZE (msg) + sizeof(sctk_thread_ptp_message_t);
      sctk_shm_recv_frag_info = sctk_shm_init_recv_frag_msg(hkey, size);
      sctk_shm_recv_frag_info->addr = cell->opaque_send;
      cell->opaque_recv = (void*) sctk_shm_recv_frag_info;
      /* copy and reinit ptp thread header */
      msg = sctk_shm_recv_frag_info->header;
      memcpy( msg, cell->data, sizeof(sctk_thread_ptp_message_body_t));
   }
   else
   {
      sctk_shm_recv_frag_info = sctk_shm_frag_get_elt_from_hash(hkey,SCTK_SHM_RECVER_HT); 
      assume_m( sctk_shm_recv_frag_info != NULL, "WHAT ?!" );
      //fprintf(stderr, "(%p-%d-%d) [NEXT - RECV]\tKey : %d\n", cell,sctk_local_process_rank, sctk_shm_recv_frag_info->dest, cell->frag_hkey);
      size = sctk_shm_recv_frag_info->size_total - sctk_shm_recv_frag_info->size_compute;
      size = (size < SCTK_SHM_CELL_SIZE) ? size : SCTK_SHM_CELL_SIZE;
      buffer_in = sctk_shm_recv_frag_info->msg + sctk_shm_recv_frag_info->size_compute;
      buffer_out = cell->data;
      memcpy(buffer_in, buffer_out, size);
      sctk_shm_recv_frag_info->size_compute += size;
   }

   sctk_shm_release_cell(cell); 

   if(sctk_shm_recv_frag_info->size_total != sctk_shm_recv_frag_info->size_compute)
	return NULL;

        msg = sctk_shm_recv_frag_info->header;
      	msg->body.completion_flag = NULL;
     	msg->tail.message_type = SCTK_MESSAGE_NETWORK;
      	sctk_rebuild_header(msg);
      	sctk_reinit_header(msg, sctk_shm_rdv_free, sctk_shm_rdv_message_copy);
        sctk_shm_frag_del_elt_from_hash(hkey,SCTK_SHM_RECVER_HT);
	return msg;
}

void 
sctk_network_frag_shm_interface_init(void)
{
    sctk_shm_process_hash_id = (sctk_get_local_process_rank() << 8);
    MPCHT_init(&sctk_shm_recving_frag_hastable_ptr, 2 << 16);
    MPCHT_init(&sctk_shm_sending_frag_hastable_ptr, SCTK_SHM_MAX_PROC_ID );
}
