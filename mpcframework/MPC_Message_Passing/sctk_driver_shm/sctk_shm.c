
#include <sctk_debug.h>
#include "sctk_route.h"
#include "mpc_launch_pmi.h"
#include "sctk_shm_mapper.h"
#include "sctk_handler_pmi.h"
#include "sctk_alloc.h"

#include <sctk_net_tools.h>
#include <sctk_route.h>
#include "sctk_shm_raw_queues.h"
#include "sctk_shm.h"
#include "utlist.h"
#include "sctk_shm_raw_queues_internals.h"

static int sctk_shm_proc_local_rank_on_node = -1;
static volatile int sctk_shm_driver_initialized = 0;
static unsigned int sctk_shm_send_max_try = 2;
static int sctk_cma_enabled = 1;

static unsigned int sctk_shm_pending_ptp_msg_num = 0;
static sctk_spinlock_t sctk_shm_polling_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_spinlock_t sctk_shm_pending_ptp_msg_lock = SCTK_SPINLOCK_INITIALIZER;

static sctk_shm_msg_list_t *sctk_shm_pending_ptp_msg_list = NULL;

// FROM Henry S. Warren, Jr.'s "Hacker's Delight."
static long sctk_shm_roundup_powerof2(unsigned long n)
{
    assume( n < (unsigned long)( 1 << 31));
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n+1;
}

#if 0
void sctk_shm_network_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
                         size_t size )
{


}

void sctk_shm_network_rdma_read(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
                         size_t size )
{

}
#endif

static void
sctk_network_add_message_to_pending_shm_list( sctk_thread_ptp_message_t *msg, int sctk_shm_dest, int with_lock)
{
   sctk_shm_msg_list_t *tmp = sctk_malloc(sizeof(sctk_shm_msg_list_t));
   tmp->msg = msg;
   tmp->sctk_shm_dest = sctk_shm_dest; 
   if(with_lock)
      sctk_spinlock_lock(&sctk_shm_pending_ptp_msg_lock);
   DL_APPEND(sctk_shm_pending_ptp_msg_list, tmp);
   sctk_shm_pending_ptp_msg_num++;
   if(with_lock)
      sctk_spinlock_unlock(&sctk_shm_pending_ptp_msg_lock);
}

static int sctk_network_send_message_dest_shm(sctk_thread_ptp_message_t *msg,
                                              int sctk_shm_dest,
                                              int with_lock) {
  sctk_shm_cell_t *cell = NULL;
  unsigned int sctk_shm_send_cur_try;
  int is_message_control = 0;

  if (sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg))) {
    is_message_control = 1;
  }

  sctk_shm_send_cur_try = 0;
  while (!cell && sctk_shm_send_cur_try++ < sctk_shm_send_max_try)
    cell = sctk_shm_get_cell(sctk_shm_dest, is_message_control);

  if (!cell) {
    sctk_network_add_message_to_pending_shm_list(msg, sctk_shm_dest, with_lock);
    return 1;
  }

  cell->dest = sctk_shm_dest;
  cell->src = SCTK_MSG_SRC_PROCESS(msg);

  if (sctk_network_eager_msg_shm_send(msg, cell))
    return 0;
#ifdef MPC_USE_CMA
   if(sctk_network_cma_msg_shm_send(msg,cell) && sctk_cma_enabled)
     return 0;
#endif /* MPC_USE_CMA */
   if(sctk_network_frag_msg_shm_send(msg,cell))
     return 0;

   sctk_shm_release_cell(cell); 
   sctk_network_add_message_to_pending_shm_list( msg, sctk_shm_dest, with_lock );

   return 1;
}


static void
sctk_network_send_message_from_pending_shm_list( void )
{
	sctk_shm_msg_list_t *elt = NULL;
	sctk_shm_msg_list_t *tmp = NULL;

   if(!sctk_shm_pending_ptp_msg_num)
      return;

	if(sctk_spinlock_trylock(&sctk_shm_pending_ptp_msg_lock))
		return;

	DL_FOREACH_SAFE(sctk_shm_pending_ptp_msg_list,elt,tmp) {
          DL_DELETE(sctk_shm_pending_ptp_msg_list, elt);
          if (elt) {
            if (sctk_network_send_message_dest_shm(elt->msg, elt->sctk_shm_dest,
                                                   0)) {
              /* If we are here the element
               * was back pushed to the pending list */

              /* sctk_thread_ptp_message_t *msg = elt->msg;
              sctk_debug(
                  "!%d!  [ %d -> %d ] [ %d -> %d ] (CLASS "
                  "%s(%d) SPE %d SIZE %d TAG %d)",
                  mpc_common_get_process_rank(), SCTK_MSG_SRC_PROCESS(msg),
                  SCTK_MSG_DEST_PROCESS(msg), SCTK_MSG_SRC_TASK(msg),
                  SCTK_MSG_DEST_TASK(msg),
                  sctk_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
                  SCTK_MSG_SPECIFIC_CLASS(msg),
                  sctk_message_class_is_process_specific(
                      SCTK_MSG_SPECIFIC_CLASS(msg)),
                  SCTK_MSG_SIZE(msg), SCTK_MSG_TAG(msg)); */

              sctk_free((void*)elt);
              sctk_shm_pending_ptp_msg_num--;
            }
          }
        }

        sctk_spinlock_unlock(&sctk_shm_pending_ptp_msg_lock);
}

static void 
sctk_network_send_message_endpoint_shm ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
	sctk_network_send_message_dest_shm( msg, endpoint->data.shm.dest, 1);
}

static void 
sctk_network_notify_matching_message_shm ( __UNUSED__ sctk_thread_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail )
{
}


static void 
sctk_network_notify_perform_message_shm ( __UNUSED__ int remote, __UNUSED__ int remote_task_id, __UNUSED__ int polling_task_id, __UNUSED__ int blocking, __UNUSED__ sctk_rail_info_t *rail )
{
}

static int 
sctk_send_message_from_network_shm ( sctk_thread_ptp_message_t *msg )
{
	if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
	{
		/* No reordering */
		sctk_send_message_try_check ( msg, 1 );
	}
	return 1;
}


#define MAX_REDUCE_POLLING 1024

static void 
sctk_network_notify_idle_message_shm ( __UNUSED__ sctk_rail_info_t *rail )
{
    sctk_shm_cell_t * cell;
    sctk_thread_ptp_message_t *msg;

    if(!sctk_shm_driver_initialized)
        return;

    if(sctk_spinlock_trylock(&sctk_shm_polling_lock))
      return; 


    while(1)
    {
      cell = sctk_shm_recv_cell();
      if (!cell) {
        break;
      }

      switch (cell->msg_type) {
      case SCTK_SHM_EAGER:
        msg = sctk_network_eager_msg_shm_recv(cell, 0);
        if (msg)
          sctk_send_message_from_network_shm(msg);
        break;
#ifdef MPC_USE_CMA
	    case SCTK_SHM_RDMA: 
		msg = sctk_network_cma_msg_shm_recv(cell,1);
               	if(msg) sctk_send_message_from_network_shm(msg);
		break;
	    case SCTK_SHM_CMPL:
            	msg = sctk_network_cma_cmpl_msg_shm_recv(cell);
        	sctk_complete_and_free_message(msg); 
		break;
#endif /* MPC_USE_CMA */
	    case SCTK_SHM_FIRST_FRAG:
	    case SCTK_SHM_NEXT_FRAG:
            	msg = sctk_network_frag_msg_shm_recv(cell);
    	    	if(msg) sctk_send_message_from_network_shm(msg);
		break;
	    default:
		abort();
        }
    }
    
    if(!cell)
    {
    	sctk_network_frag_msg_shm_idle(1);
    	sctk_network_send_message_from_pending_shm_list();
    }

    sctk_spinlock_unlock(&sctk_shm_polling_lock);

}

static void sctk_network_notify_recv_message_shm ( __UNUSED__ sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
    sctk_network_notify_idle_message_shm ( rail );
}

static void 
sctk_network_notify_any_source_message_shm ( __UNUSED__ int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail )
{
    sctk_network_notify_idle_message_shm ( rail );
}

/********************************************************************/
/* SHM Init                                                         */
/********************************************************************/

static void *
sctk_shm_add_region_slave(sctk_size_t size,sctk_alloc_mapper_handler_t * handler) 
{
	//vars
	char *filename;
	int fd;
	void *ptr;

	//check args
	assert(handler != NULL);
	assert(handler->recv_handler != NULL);
	assert(handler->send_handler != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//get filename
        filename = handler->recv_handler(handler->option, handler->option1);
        assume_m(filename != NULL, "Failed to get the SHM filename.");

        // firt try to map
        fd = sctk_shm_mapper_slave_open(filename);
        ptr = sctk_shm_mapper_mmap(NULL, fd, size);

        mpc_launch_pmi_barrier();

	close(fd);
        sctk_free(filename);
        return ptr;
}

static void *
sctk_shm_add_region_master(sctk_size_t size,sctk_alloc_mapper_handler_t * handler)
{
	//vars
	char *filename;
	int fd;
	void *ptr;
        bool status;
	
	//check args
	assert(handler != NULL);
	assert(handler->recv_handler != NULL);
	assert(handler->send_handler != NULL);
	assert(size > 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE == 0);

	//get filename
        filename = handler->gen_filename(handler->option, handler->option1);

        // create file and map it
        fd = sctk_shm_mapper_create_shm_file(filename, size);
        ptr = sctk_shm_mapper_mmap(NULL, fd, size);

        // sync filename
        status =
            handler->send_handler(filename, handler->option, handler->option1);
        assume_m(status,
                 "Fail to send the SHM filename to other participants.");

        mpc_launch_pmi_barrier();

	close(fd);
	sctk_shm_mapper_unlink(filename);

	//free temp memory and return
        sctk_free(filename);
        return ptr;
}

static void* sctk_shm_add_region(sctk_size_t size,sctk_shm_mapper_role_t role,
            sctk_alloc_mapper_handler_t * handler)
{
	//errors
	assume_m(handler != NULL,"You must provide a valid handler to exchange the SHM filename.");
	if(size == 0 && size % SCTK_SHM_MAPPER_PAGE_SIZE != 0){
        printf("Size must be non NULL and multiple of page size (%lu bytes), get %lu\n",SCTK_SHM_MAPPER_PAGE_SIZE,size);
        abort();
    }
	//select the method
	switch(role)
	{
		case SCTK_SHM_MAPPER_ROLE_MASTER:
		    return sctk_shm_add_region_master(size,handler);
		case SCTK_SHM_MAPPER_ROLE_SLAVE:
			return sctk_shm_add_region_slave(size,handler);
		default:
			assume_m(0,"Invalid role on sctk_shm_mapper.");
	}

    return NULL;
}

static void sctk_shm_add_route(int dest,int shm_dest,sctk_rail_info_t *rail )
{
    sctk_endpoint_t *new_route;

    /* Allocate a new route */
    new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
    assume ( new_route != NULL );

    sctk_endpoint_init ( new_route, dest, rail, ROUTE_ORIGIN_STATIC );

    new_route->data.shm.dest = shm_dest;

    /* Add the new route */
    sctk_rail_add_static_route ( rail, new_route );

    /* set the route as connected */
    sctk_endpoint_set_state ( new_route, STATE_CONNECTED );

    //fprintf(stdout, "Add route to %d with endpoint %d\n", dest, shm_dest);     
    return;
}

static void sctk_shm_init_raw_queue(size_t size, int cells_num, int rank)
{
    char buffer[16];
    void *shm_base = NULL;
    sctk_shm_mapper_role_t shm_role;
    struct sctk_alloc_mapper_handler_s *pmi_handler = NULL;

    sprintf(buffer, "%d", rank);
    pmi_handler = sctk_shm_pmi_handler_init(buffer);

    shm_role = SCTK_SHM_MAPPER_ROLE_SLAVE;
    shm_role = ( mpc_common_get_local_process_rank() == rank)?SCTK_SHM_MAPPER_ROLE_MASTER:shm_role; 
    shm_base = sctk_shm_add_region( size, shm_role, pmi_handler);
        
    sctk_shm_add_region_infos(shm_base,size,cells_num,rank );

    if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER )
        sctk_shm_reset_process_queues( rank );

    sctk_shm_pmi_handler_free(pmi_handler);
}

static void sctk_shm_free_raw_queue()
{
}

void sctk_shm_check_raw_queue(int local_process_number)
{
    int i;
    for( i=0; i < local_process_number; i++)
    {
        assume_m( sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_SEND,i),"Queue must be empty")
        assume_m( sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_RECV,i),"Queue must be empty")
        assume_m( sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_CMPL,i),"Queue must be empty")
        assume_m(!sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_FREE,i),"Queue must be full")
    }
    mpc_launch_pmi_barrier();
}

void sctk_network_finalize_shm(__UNUSED__ sctk_rail_info_t *rail)
{
	if(!sctk_shm_driver_initialized)
		return;

	/** complementary condition to init func: @see sctk_network_inif_shm */
	sctk_network_frag_shm_interface_free();
#ifdef MPC_USE_CMA
	sctk_shm_network_cma_shm_interface_free();
#endif
	int nb_process = -1, i = -1;
	for (i = 0; i < nb_process; ++i) {
		sctk_shm_free_raw_queue(i);
	}
	sctk_shm_free_regions_infos();
	/*TODO: move every global data into sctk_shm_rail_info_t struct */
}

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

void sctk_network_init_shm ( sctk_rail_info_t *rail )
{
    int i, local_process_rank, local_process_number;
    size_t sctk_shmem_size;
    int sctk_shmem_cells_num;

   /* Register Hooks in rail */
   rail->send_message_endpoint = sctk_network_send_message_endpoint_shm;
   rail->notify_recv_message = sctk_network_notify_recv_message_shm;
   rail->notify_matching_message = sctk_network_notify_matching_message_shm;
   rail->notify_any_source_message = sctk_network_notify_any_source_message_shm;
   rail->notify_perform_message = sctk_network_notify_perform_message_shm;
   rail->notify_idle_message = sctk_network_notify_idle_message_shm;
   rail->send_message_from_network = sctk_send_message_from_network_shm;

   rail->network_name = "SHM";
   rail->driver_finalize = sctk_network_finalize_shm;
//   if( strcmp(rail->runtime_config_rail->topology, "none"))
//	sctk_nodebug("SHM topology must be 'none'");

   sctk_rail_init_route ( rail, "none", NULL );

   sctk_shmem_cells_num =
       2048; // rail->runtime_config_driver_config->driver.value.shm.cells_num;
   sctk_shmem_size = sctk_shm_get_region_size(sctk_shmem_cells_num);
   sctk_shmem_size = sctk_shm_roundup_powerof2(sctk_shmem_size);

   local_process_rank = mpc_common_get_local_process_rank();
   local_process_number = mpc_common_get_local_process_count();

   sctk_shm_proc_local_rank_on_node = local_process_rank;

   if (local_process_number == 1 || mpc_common_get_node_count() > 1)
   {
     sctk_shm_driver_initialized = 0;
     return;
   }

   struct mpc_launch_pmi_process_layout *nodes_infos = NULL;
   mpc_launch_pmi_get_process_layout(&nodes_infos);

   int node_rank = mpc_common_get_node_rank();

   struct mpc_launch_pmi_process_layout *tmp;
   HASH_FIND_INT( nodes_infos, &node_rank, tmp);
   assert(tmp != NULL);

   sctk_shm_init_regions_infos(local_process_number);
   for(i=0; i < tmp->nb_process; i++)
   {
      sctk_shm_init_raw_queue(sctk_shmem_size,sctk_shmem_cells_num,i);
      if( i != local_process_rank)
         sctk_shm_add_route(tmp->process_list[i], i,rail);
      mpc_launch_pmi_barrier();
   }
    
    sctk_cma_enabled = rail->runtime_config_driver_config->driver.value.shm.cma_enable;
#ifdef MPC_USE_CMA
    (void) sctk_network_cma_shm_interface_init(0);
#endif /* MPC_USE_CMA */
    sctk_network_frag_shm_interface_init();
    //sctk_shm_check_raw_queue(local_process_number);
    sctk_shm_driver_initialized = 1;
}
