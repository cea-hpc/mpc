#include "sctk_shm_frag.h"
#include "sctk_net_tools.h"

static volatile int sctk_shm_idle_frag_msg = 0;
static volatile int sctk_shm_process_msg_id = 0;
static int sctk_shm_max_frag_msg_per_process = SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS;

static sctk_spinlock_t sctk_shm_sending_frag_hastable_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_spinlock_t sctk_shm_recving_frag_hastable_lock = SCTK_SPINLOCK_INITIALIZER;
static struct MPCHT *sctk_shm_sending_frag_hastable_ptr = NULL;
static struct MPCHT *sctk_shm_recving_frag_hastable_ptr = NULL;
  
static sctk_shm_proc_frag_info_t*
sctk_shm_frag_get_elt_from_hash(int key, int table_id, sctk_shm_table_t table_type)
{
   void * tmp;
   switch(table_type)
   {
      case SCTK_SHM_SENDER_HT:
         tmp = MPCHT_get(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key);
	 break;
      case SCTK_SHM_RECVER_HT:
         tmp = MPCHT_get(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key);
         break;
      default:
        assume_m(0, "Unknown shm hastable type");
   }
   return (sctk_shm_proc_frag_info_t*) tmp; 
}

static void
sctk_shm_frag_add_elt_to_hash(int key, int table_id, sctk_shm_proc_frag_info_t* elt, sctk_shm_table_t table_type)
{
   switch(table_type)
   {
      case SCTK_SHM_SENDER_HT:
         MPCHT_set(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key, (void*) elt);
         break;
      case SCTK_SHM_RECVER_HT:
	 MPCHT_set(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key, (void*) elt);
	 break;
      default:
	 assume_m(0, "Unknown shm hastable type");
   }
}

static void
sctk_shm_frag_del_elt_from_hash(int key, int table_id, sctk_shm_table_t table_type)
{
   switch(table_type)
   {
      case SCTK_SHM_SENDER_HT:
         MPCHT_delete(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key);
         break;
      case SCTK_SHM_RECVER_HT:
         MPCHT_delete(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key);
         break;
      default:
         assume_m(0, "Unknown shm hastable type");
   }
}

static void
sctk_shm_rdv_free(void* ptr)
{
    sctk_shm_proc_frag_info_t* info = (sctk_shm_proc_frag_info_t*)(ptr - sizeof(sctk_shm_proc_frag_info_t));
    sctk_free(info);
}

static void
sctk_shm_rdv_message_copy(sctk_message_to_copy_t* tmp)
{
   sctk_shm_proc_frag_info_t* info = NULL;
   info = (sctk_shm_proc_frag_info_t*)((void*)tmp->msg_send - sizeof(sctk_shm_proc_frag_info_t));
   sctk_net_message_copy_from_buffer(info->msg,tmp,1);
}

static sctk_shm_proc_frag_info_t*
sctk_shm_send_register_new_frag_msg(int dest)
{
   int try = 0;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   /* Prevent polling thread activity */ 
   frag_infos = sctk_malloc(sizeof(sctk_shm_proc_frag_info_t));
   frag_infos->is_ready = SCTK_SPINLOCK_INITIALIZER;
   sctk_spinlock_lock(&(frag_infos->is_ready));
 
   /* Try to get an empty key in hastable */  
   sctk_spinlock_lock(&sctk_shm_sending_frag_hastable_lock); 
   do
   {
      sctk_shm_process_msg_id = (sctk_shm_process_msg_id+1) % SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS;
      frag_infos->msg_frag_key = sctk_shm_process_msg_id;

      if(try++ > sctk_shm_max_frag_msg_per_process)
         break;

   } while(sctk_shm_frag_get_elt_from_hash(frag_infos->msg_frag_key, dest, SCTK_SHM_SENDER_HT)); 
   
   /* Abort frag sending message */ 
   if( try > sctk_shm_max_frag_msg_per_process)
   {
      sctk_free(frag_infos);
      frag_infos = NULL;
      sctk_error("FAILED TO SEND MSG");
   }   
   else
   {
      sctk_shm_frag_add_elt_to_hash(frag_infos->msg_frag_key, dest, frag_infos, SCTK_SHM_SENDER_HT);
      sctk_shm_idle_frag_msg++;
   }   

   sctk_spinlock_unlock(&sctk_shm_sending_frag_hastable_lock); 
   return frag_infos; 
}

static sctk_shm_proc_frag_info_t* 
sctk_shm_init_send_frag_msg(int *key, int remote, sctk_thread_ptp_message_t* msg)
{
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   frag_infos = sctk_shm_send_register_new_frag_msg(remote);
   
   if(!frag_infos)
      return NULL;
 
   frag_infos->size_total = SCTK_MSG_SIZE(msg);
   frag_infos->size_copied = 0;
   frag_infos->remote_mpi_rank = remote; 
   frag_infos->local_mpi_rank = sctk_get_local_process_rank(); 
   frag_infos->header = msg;
   frag_infos->msg = (void*) msg->tail.message.contiguous.addr;
    
   if( msg->tail.message_type != SCTK_MESSAGE_CONTIGUOUS)
   {
      frag_infos->msg = sctk_malloc(SCTK_MSG_SIZE(msg));
      sctk_net_copy_in_buffer( msg, frag_infos->msg);	
   }

   return frag_infos;
}

static sctk_shm_proc_frag_info_t* 
sctk_shm_init_recv_frag_msg(int key, int remote, sctk_thread_ptp_message_t* msg)
{
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   const size_t msg_size = SCTK_MSG_SIZE(msg);

   frag_infos = sctk_malloc(sizeof(sctk_shm_proc_frag_info_t)+msg_size+sizeof(sctk_thread_ptp_message_t));
   frag_infos->is_ready = SCTK_SPINLOCK_INITIALIZER;
   sctk_spinlock_lock(&(frag_infos->is_ready));

   frag_infos->size_total = msg_size;
   frag_infos->size_copied = 0;
   frag_infos->remote_mpi_rank = remote;
   frag_infos->local_mpi_rank = sctk_get_local_process_rank();
   frag_infos->header = (sctk_thread_ptp_message_t*)((char*)frag_infos+sizeof(sctk_shm_proc_frag_info_t));
   frag_infos->msg = (char*) frag_infos->header + sizeof(sctk_thread_ptp_message_t);

   if(msg_size)
   {
      sctk_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
      sctk_shm_frag_add_elt_to_hash(key, remote, frag_infos, SCTK_SHM_RECVER_HT);
      sctk_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
   }

   return frag_infos;
}

static sctk_shm_proc_frag_info_t*
sctk_network_frag_msg_first_send(sctk_thread_ptp_message_t* msg, sctk_shm_cell_t *cell)
{
   int msg_key = -1;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   
   cell->size_to_copy = (SCTK_MSG_SIZE(msg) > 0) ? SCTK_MSG_SIZE(msg) : 0;
 
   if( SCTK_MSG_SIZE(msg) > 0 ) 
   {
      frag_infos = sctk_shm_init_send_frag_msg(&msg_key, cell->dest, msg);
   }


   if(SCTK_MSG_SIZE(msg) > 0 && !frag_infos)
      return NULL;

   memcpy(cell->data, msg, sizeof(sctk_thread_ptp_message_body_t));
  
   cell->frag_hkey = (frag_infos) ? frag_infos->msg_frag_key : -1; 
   cell->msg_type = SCTK_SHM_FIRST_FRAG;
   sctk_shm_send_cell(cell);       

   if(frag_infos) 
   {
   	sctk_spinlock_unlock(&(frag_infos->is_ready));
   }
   else
   {
        sctk_complete_and_free_message(msg) ;
   }
   return frag_infos;
}

static sctk_shm_proc_frag_info_t*
sctk_network_frag_msg_first_recv(sctk_thread_ptp_message_t* msg, sctk_shm_cell_t *cell)
{
   int msg_key, msg_src;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;

   msg_src = cell->src; 
   msg_key = cell->frag_hkey; 

   sctk_nodebug("[KEY:%d-%ld]\t\tRECV FIRST PART MSG", msg_key, SCTK_MSG_SIZE(msg));
   frag_infos = sctk_shm_init_recv_frag_msg(msg_key, msg_src, msg);
   memcpy(frag_infos->header, cell->data, sizeof(sctk_thread_ptp_message_body_t));

   /* reset tail */
   msg = frag_infos->header;
   msg->body.completion_flag = NULL;
   msg->tail.message_type = SCTK_MESSAGE_NETWORK;
   sctk_rebuild_header(msg);
   sctk_reinit_header(msg, sctk_shm_rdv_free, sctk_shm_rdv_message_copy);

   return frag_infos; 
}

static int
sctk_network_frag_msg_next_send(sctk_shm_proc_frag_info_t* frag_infos)
{
   int msg_key, msg_dest;
   size_t size = 0;
   sctk_shm_cell_t* cell = NULL;
   sctk_thread_ptp_message_t* msg = NULL;
  
   if(sctk_spinlock_trylock(&(frag_infos->is_ready)))
	return 0;

   msg_key = frag_infos->msg_frag_key;
   msg_dest = frag_infos->remote_mpi_rank;

   sctk_nodebug("[KEY:%d] TRY SEND NEXT PART OF FRAGMENTED MSG", msg_key);
   cell = sctk_shm_get_cell(msg_dest);
   if(!cell)
   {
      sctk_spinlock_unlock(&(frag_infos->is_ready));
      return 0;
   }

   size = frag_infos->size_total - frag_infos->size_copied;
   size = (size < SCTK_SHM_CELL_SIZE) ? size : SCTK_SHM_CELL_SIZE;

   cell->msg_type = SCTK_SHM_NEXT_FRAG;
   cell->src = sctk_get_local_process_rank();
   cell->dest = msg_dest;
   cell->size_to_copy = size;
   cell->frag_hkey = msg_key;

   memcpy(cell->data, frag_infos->msg+frag_infos->size_copied, cell->size_to_copy); 
   frag_infos->size_copied += cell->size_to_copy;

   sctk_nodebug("[KEY:%d] SEND NEXT PART MSG", msg_key);
   sctk_shm_send_cell(cell); 

   if(frag_infos->size_total == frag_infos->size_copied)
   {
      sctk_shm_frag_del_elt_from_hash(msg_key, msg_dest, SCTK_SHM_SENDER_HT);
      sctk_shm_idle_frag_msg++;
   }
   
   if(frag_infos->size_total == frag_infos->size_copied)
   {
        sctk_nodebug("[KEY:%d] SEND END PART MSG", msg_key);
	msg = frag_infos->header;
   	sctk_free(frag_infos);
        sctk_complete_and_free_message(msg) ;
   }

   sctk_spinlock_unlock(&(frag_infos->is_ready));

   return 1;
}

void
sctk_network_frag_msg_shm_idle(int max_try)
{
    sctk_shm_proc_frag_info_t* infos = NULL;
    int i;
    
    if(!sctk_shm_idle_frag_msg)
       return;

    if(sctk_spinlock_trylock(&sctk_shm_sending_frag_hastable_lock))
  	    return;

    for (i=0; i < sctk_get_local_process_number(); i++)
    {
    	MPC_HT_ITER( &(sctk_shm_sending_frag_hastable_ptr[i]), infos )
    	   if(!sctk_network_frag_msg_next_send(infos))
              break;
    	MPC_HT_ITER_END
    }

    sctk_spinlock_unlock(&sctk_shm_sending_frag_hastable_lock);
}

int 
sctk_network_frag_msg_shm_send(sctk_thread_ptp_message_t* msg, sctk_shm_cell_t* cell)
{
   sctk_shm_proc_frag_info_t* infos = sctk_network_frag_msg_first_send(msg, cell);
   
   if(!infos)
   	return (SCTK_MSG_SIZE(msg) > 0) ? 0 : 1;
    
   return 1;
}

sctk_thread_ptp_message_t *
sctk_network_frag_msg_shm_recv(sctk_shm_cell_t* cell, int enabled_copy)
{
   int msg_key, msg_src;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   sctk_thread_ptp_message_t* msg_hdr = NULL;
   
   msg_src = cell->src; 
   msg_key = cell->frag_hkey; 

   if( cell->msg_type == SCTK_SHM_FIRST_FRAG )
   {
     frag_infos = sctk_network_frag_msg_first_recv((sctk_thread_ptp_message_t *)cell->data, cell);
   }
   else
   {
      sctk_nodebug("[KEY:%d]\t\tRECV NEXT PART MSG", msg_key);
      sctk_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
      frag_infos = sctk_shm_frag_get_elt_from_hash(msg_key, msg_src, SCTK_SHM_RECVER_HT); 
      sctk_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
      assume_m( frag_infos != NULL, "Recv an oprhelin SHM fragment\n");
      memcpy(frag_infos->msg + frag_infos->size_copied, cell->data, cell->size_to_copy);
      frag_infos->size_copied += cell->size_to_copy;
   }

   sctk_shm_release_cell(cell); 
   msg_hdr = NULL;

   if(frag_infos->size_total == frag_infos->size_copied)
   {
	msg_hdr = frag_infos->header;
	if(frag_infos->size_total)
        {	
    		sctk_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
   		    sctk_shm_frag_del_elt_from_hash(msg_key, msg_src, SCTK_SHM_RECVER_HT);
    		sctk_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
        }
        sctk_nodebug("[KEY:%d]\t\tRECV END PART MSG",msg_key);
   }

   return msg_hdr;
}

void 
sctk_network_frag_shm_interface_init(void)
{
    int i;
    const int sctk_shm_process_on_node = sctk_get_local_process_number();

    sctk_shm_sending_frag_hastable_ptr = (struct MPCHT*) sctk_malloc(sctk_shm_process_on_node * sizeof(struct MPCHT)); 
    sctk_shm_recving_frag_hastable_ptr = (struct MPCHT*) sctk_malloc(sctk_shm_process_on_node * sizeof(struct MPCHT)); 
    
    for(i=0; i < sctk_shm_process_on_node; i++)
    { 
        sctk_nodebug("%d %d %p", sctk_get_local_process_rank(), i, &(sctk_shm_recving_frag_hastable_ptr[i]));
    	MPCHT_init(&(sctk_shm_recving_frag_hastable_ptr[i]), sctk_shm_max_frag_msg_per_process);
    	MPCHT_init(&(sctk_shm_sending_frag_hastable_ptr[i]), sctk_shm_max_frag_msg_per_process);
    }
}
