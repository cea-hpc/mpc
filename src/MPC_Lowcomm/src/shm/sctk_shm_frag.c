#include "sctk_shm_frag.h"
#include "sctk_net_tools.h"

#include <mpc_common_rank.h>

#ifdef SCTK_USE_CHECKSUM
#include <zlib.h>
#define hash_payload(a,b) adler32(0UL, (void*)a, (size_t)b)
#endif

#include <sctk_alloc.h>

static volatile int sctk_shm_idle_frag_msg = 0;
static volatile int sctk_shm_process_msg_id = 0;

static mpc_common_spinlock_t sctk_shm_sending_frag_hastable_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static mpc_common_spinlock_t sctk_shm_recving_frag_hastable_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static struct mpc_common_hashtable *sctk_shm_sending_frag_hastable_ptr = NULL;
static struct mpc_common_hashtable *sctk_shm_recving_frag_hastable_ptr = NULL;
  
static sctk_shm_proc_frag_info_t*
sctk_shm_frag_get_elt_from_hash(int key, int table_id, sctk_shm_table_t table_type)
{
   void * tmp;
   switch(table_type)
   {
      case SCTK_SHM_SENDER_HT:
         tmp = mpc_common_hashtable_get(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key);
	 break;
      case SCTK_SHM_RECVER_HT:
         tmp = mpc_common_hashtable_get(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key);
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
         mpc_common_hashtable_set(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key, (void*) elt);
         break;
      case SCTK_SHM_RECVER_HT:
	 mpc_common_hashtable_set(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key, (void*) elt);
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
         mpc_common_hashtable_delete(&(sctk_shm_sending_frag_hastable_ptr[table_id]), key);
         break;
      case SCTK_SHM_RECVER_HT:
         mpc_common_hashtable_delete(&(sctk_shm_recving_frag_hastable_ptr[table_id]), key);
         break;
      default:
         assume_m(0, "Unknown shm hastable type");
   }
}

static void
sctk_shm_rdv_free(void* ptr)
{
    sctk_free(ptr);
}

static void
sctk_shm_rdv_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t* tmp)
{
   sctk_net_message_copy(tmp);
}

static sctk_shm_proc_frag_info_t*
sctk_shm_send_register_new_frag_msg(int dest)
{
   int try = 0;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   /* Prevent polling thread activity */ 
   frag_infos = sctk_malloc(sizeof(sctk_shm_proc_frag_info_t));
   mpc_common_spinlock_init(&frag_infos->is_ready, 0);

   mpc_common_spinlock_lock(&(frag_infos->is_ready));
 
   /* Try to get an empty key in hastable */  
   mpc_common_spinlock_lock(&sctk_shm_sending_frag_hastable_lock); 
   do
   {
      sctk_shm_process_msg_id = (sctk_shm_process_msg_id+1) % SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS;
      frag_infos->msg_frag_key = sctk_shm_process_msg_id;

      if(try > SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS)
         break;

   } while(sctk_shm_frag_get_elt_from_hash(frag_infos->msg_frag_key, dest, SCTK_SHM_SENDER_HT)); 
   
   /* Abort frag sending message */ 
   if( try > SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS)
   {
      sctk_free(frag_infos);
      frag_infos = NULL;
      mpc_common_nodebug("FAILED TO SEND MSG");
   }   
   else
   {
      sctk_shm_frag_add_elt_to_hash(frag_infos->msg_frag_key, dest, frag_infos, SCTK_SHM_SENDER_HT);
      sctk_shm_idle_frag_msg++;
   }   

   mpc_common_spinlock_unlock(&sctk_shm_sending_frag_hastable_lock); 
   return frag_infos; 
}

static sctk_shm_proc_frag_info_t* 
sctk_shm_init_send_frag_msg( int remote, mpc_lowcomm_ptp_message_t* msg)
{
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   frag_infos = sctk_shm_send_register_new_frag_msg(remote);
   
   if(!frag_infos)
      return NULL;
 
   frag_infos->size_total = SCTK_MSG_SIZE(msg);
   frag_infos->size_copied = 0;
   frag_infos->remote_mpi_rank = remote; 
   frag_infos->local_mpi_rank = mpc_common_get_local_process_rank(); 
   frag_infos->header = msg;
   frag_infos->msg = (void*) msg->tail.message.contiguous.addr;
    
   if( msg->tail.message_type != MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
   {
      frag_infos->msg = sctk_malloc(SCTK_MSG_SIZE(msg));
      sctk_net_copy_in_buffer( msg, frag_infos->msg);	
   }

   return frag_infos;
}

static sctk_shm_proc_frag_info_t* 
sctk_shm_init_recv_frag_msg(int key, int remote, mpc_lowcomm_ptp_message_t* msg)
{
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   const size_t msg_size = SCTK_MSG_SIZE(msg);

   frag_infos = (sctk_shm_proc_frag_info_t*) sctk_malloc(sizeof(sctk_shm_proc_frag_info_t));
   mpc_common_spinlock_init(&frag_infos->is_ready, 0);
   mpc_common_spinlock_lock(&(frag_infos->is_ready));

   frag_infos->size_total = msg_size;
   frag_infos->size_copied = 0;
   frag_infos->remote_mpi_rank = remote;
   frag_infos->local_mpi_rank = mpc_common_get_local_process_rank();
   frag_infos->header = (mpc_lowcomm_ptp_message_t*) sctk_malloc(msg_size+sizeof(mpc_lowcomm_ptp_message_t));
   frag_infos->msg = (char*) frag_infos->header + sizeof(mpc_lowcomm_ptp_message_t);

   if(msg_size)
   {
      mpc_common_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
      sctk_shm_frag_add_elt_to_hash(key, remote, frag_infos, SCTK_SHM_RECVER_HT);
      mpc_common_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
   }

   return frag_infos;
}

static sctk_shm_proc_frag_info_t*
sctk_network_frag_msg_first_send(mpc_lowcomm_ptp_message_t* msg, sctk_shm_cell_t *cell)
{
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   
   cell->size_to_copy = (SCTK_MSG_SIZE(msg) > 0) ? SCTK_MSG_SIZE(msg) : 0;
 
   if( SCTK_MSG_SIZE(msg) > 0 ) 
   {
      frag_infos = sctk_shm_init_send_frag_msg( cell->dest, msg);
   }


   if(SCTK_MSG_SIZE(msg) > 0 && !frag_infos)
      return NULL;

   memcpy(cell->data, msg, sizeof(mpc_lowcomm_ptp_message_body_t));

   cell->frag_hkey = (frag_infos) ? frag_infos->msg_frag_key : -1; 
   cell->msg_type = SCTK_SHM_FIRST_FRAG;
   sctk_shm_send_cell(cell);       

   if(frag_infos) 
   {
   	    mpc_common_spinlock_unlock(&(frag_infos->is_ready));
   }
   else
   {
        mpc_lowcomm_ptp_message_complete_and_free(msg) ;
   }
   return frag_infos;
}

static sctk_shm_proc_frag_info_t*
sctk_network_frag_msg_first_recv(mpc_lowcomm_ptp_message_t* msg, sctk_shm_cell_t *cell)
{
   int msg_key, msg_src;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;

   msg_src = cell->src; 
   msg_key = cell->frag_hkey; 

   frag_infos = sctk_shm_init_recv_frag_msg(msg_key, msg_src, msg);
   memcpy(frag_infos->header, cell->data, sizeof(mpc_lowcomm_ptp_message_body_t));
   //mpc_common_nodebug("[KEY:%d-%ld]\t\tRECV FIRST PART MSG (HEADER:%lu)", msg_key, SCTK_MSG_SIZE(msg), hash_payload(frag_infos->header, sizeof(mpc_lowcomm_ptp_message_body_t)));
   
   /* reset tail */
   msg = frag_infos->header;
   SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
   msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
   _mpc_comm_ptp_message_clear_request(msg);
   _mpc_comm_ptp_message_set_copy_and_free(msg, sctk_shm_rdv_free, sctk_shm_rdv_message_copy);
   return frag_infos; 
}

static int
sctk_network_frag_msg_next_send(sctk_shm_proc_frag_info_t* frag_infos)
{
  int msg_key, msg_dest, is_control_msg;
  size_t size = 0;
  sctk_shm_cell_t *cell = NULL;
  mpc_lowcomm_ptp_message_t *msg = NULL;

  if (mpc_common_spinlock_trylock(&(frag_infos->is_ready)))
    return 0;

  is_control_msg = 0;
  msg_key = frag_infos->msg_frag_key;
  msg_dest = frag_infos->remote_mpi_rank;

  mpc_common_nodebug("[KEY:%d] TRY SEND NEXT PART OF FRAGMENTED MSG", msg_key);
  if (_mpc_comm_ptp_message_is_for_control(
          SCTK_MSG_SPECIFIC_CLASS(frag_infos->header))) {
    is_control_msg = 1;
  }

  cell = sctk_shm_get_cell(msg_dest, is_control_msg);
  if (!cell) {
    mpc_common_spinlock_unlock(&(frag_infos->is_ready));
    return 0;
   }

   size = frag_infos->size_total - frag_infos->size_copied;
   size = (size < SCTK_SHM_CELL_SIZE) ? size : SCTK_SHM_CELL_SIZE;

   cell->msg_type = SCTK_SHM_NEXT_FRAG;
   cell->src = mpc_common_get_local_process_rank();
   cell->dest = msg_dest;
   cell->size_to_copy = size;
   cell->frag_hkey = msg_key;

   memcpy(cell->data, frag_infos->msg+frag_infos->size_copied, cell->size_to_copy); 
   frag_infos->size_copied += cell->size_to_copy;

   mpc_common_nodebug("[KEY:%d] SEND NEXT PART MSG", msg_key);
   sctk_shm_send_cell(cell); 

   if(frag_infos->size_total == frag_infos->size_copied)
   {
      sctk_shm_frag_del_elt_from_hash(msg_key, msg_dest, SCTK_SHM_SENDER_HT);
      sctk_shm_idle_frag_msg++;
   }
   
   if(frag_infos->size_total == frag_infos->size_copied)
   {
        mpc_common_nodebug("[KEY:%d] SEND END PART MSG", msg_key);
        msg = frag_infos->header;
        sctk_free(frag_infos);
        mpc_lowcomm_ptp_message_complete_and_free(msg) ;
        frag_infos = NULL;
   }

   if(frag_infos)
      mpc_common_spinlock_unlock(&(frag_infos->is_ready));

   return 1;
}

void
sctk_network_frag_msg_shm_idle(int max_try)
{
    sctk_shm_proc_frag_info_t* infos = NULL;
    int i, cur_try;
    
    if(!sctk_shm_idle_frag_msg)
       return;

    if(mpc_common_spinlock_trylock(&sctk_shm_sending_frag_hastable_lock))
  	    return;
    
    cur_try = 0;
    for (i=0; i < mpc_common_get_local_process_count(); i++)
    {
    	MPC_HT_ITER( &(sctk_shm_sending_frag_hastable_ptr[i]), infos )
    	   if (!sctk_network_frag_msg_next_send(infos) && cur_try++ < max_try)
              break;
    	MPC_HT_ITER_END(&(sctk_shm_sending_frag_hastable_ptr[i]))
    }

    mpc_common_spinlock_unlock(&sctk_shm_sending_frag_hastable_lock);
}

int 
sctk_network_frag_msg_shm_send(mpc_lowcomm_ptp_message_t* msg, sctk_shm_cell_t* cell)
{
   sctk_shm_proc_frag_info_t* infos = sctk_network_frag_msg_first_send(msg, cell);
   
   if(!infos)
      return (SCTK_MSG_SIZE(msg) > 0) ? 0 : 1;
    
   return 1;
}

mpc_lowcomm_ptp_message_t *
sctk_network_frag_msg_shm_recv(sctk_shm_cell_t* cell)
{
   int msg_key, msg_src;
   sctk_shm_proc_frag_info_t* frag_infos = NULL;
   mpc_lowcomm_ptp_message_t* msg_hdr = NULL;
   
   msg_src = cell->src; 
   msg_key = cell->frag_hkey; 

   if( cell->msg_type == SCTK_SHM_FIRST_FRAG )
   {
     frag_infos = sctk_network_frag_msg_first_recv((mpc_lowcomm_ptp_message_t *)cell->data, cell);
   }
   else
   {
      mpc_common_nodebug("[KEY:%d]\t\tRECV NEXT PART MSG", msg_key);
      mpc_common_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
      frag_infos = sctk_shm_frag_get_elt_from_hash(msg_key, msg_src, SCTK_SHM_RECVER_HT); 
      assume_m((frag_infos->size_copied < frag_infos->size_total), "WRONG SIZE FOR FRAGMENT");
      mpc_common_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
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
    		mpc_common_spinlock_lock(&sctk_shm_recving_frag_hastable_lock);
   		    sctk_shm_frag_del_elt_from_hash(msg_key, msg_src, SCTK_SHM_RECVER_HT);
    		mpc_common_spinlock_unlock(&sctk_shm_recving_frag_hastable_lock);
    }
   }

   return msg_hdr;
}

void 
sctk_network_frag_shm_interface_init(void)
{
    int i;
    const int sctk_shm_process_on_node = mpc_common_get_local_process_count();

    sctk_shm_sending_frag_hastable_ptr = (struct mpc_common_hashtable*) sctk_malloc(sctk_shm_process_on_node * sizeof(struct mpc_common_hashtable)); 
    sctk_shm_recving_frag_hastable_ptr = (struct mpc_common_hashtable*) sctk_malloc(sctk_shm_process_on_node * sizeof(struct mpc_common_hashtable)); 
    
    for(i=0; i < sctk_shm_process_on_node; i++)
    { 
        mpc_common_nodebug("%d %d %p", mpc_common_get_local_process_rank(), i, &(sctk_shm_recving_frag_hastable_ptr[i]));
    	mpc_common_hashtable_init(&(sctk_shm_recving_frag_hastable_ptr[i]), SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS);
    	mpc_common_hashtable_init(&(sctk_shm_sending_frag_hastable_ptr[i]), SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS);
    }
}

void sctk_network_frag_shm_interface_free(void)
{
	const int max = mpc_common_get_local_process_count();
	int i;

	for (i = 0; i < max; ++i) {
		mpc_common_hashtable_release(sctk_shm_recving_frag_hastable_ptr + i);
		mpc_common_hashtable_release(sctk_shm_sending_frag_hastable_ptr + i);
	}

	sctk_free(sctk_shm_recving_frag_hastable_ptr);
	sctk_free(sctk_shm_sending_frag_hastable_ptr);
}
