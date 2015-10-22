
#include <sctk_debug.h>
#include "sctk_route.h"
#include "sctk_pmi.h"
#include "sctk_shm_mapper.h"
#include "sctk_handler_pmi.h"
#include "sctk_alloc.h"

#include <sctk_net_tools.h>
#include <sctk_route.h>
#include "sctk_shm_raw_queues.h"
#include "sctk_shm.h"

static int sctk_shm_proc_local_rank_on_node = -1;
static volatile int sctk_shm_driver_initialized = 0;

static void 
sctk_network_send_message_endpoint_shm ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
    if(sctk_network_eager_msg_shm_send(msg,endpoint->data.shm.dest))
        return;
    
    if(sctk_network_frag_msg_shm_send(msg,endpoint->data.shm.dest))
        return;
        
    if(sctk_network_rdma_msg_shm_send(msg,endpoint->data.shm.dest))
        return;

    assume_m(0, "message unsupported by mpc shm"); 
}

static void 
sctk_network_notify_matching_message_shm ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}


static void 
sctk_network_notify_perform_message_shm ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
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

static void 
sctk_network_notify_idle_message_shm ( sctk_rail_info_t *rail )
{
    sctk_shm_cell_t * cell;
    sctk_thread_ptp_message_t *msg; 
    
    if(!sctk_shm_driver_initialized)
        return;

    while(1)
    {
        cell = sctk_shm_recv_cell(); 
        if( cell == NULL )
            return;
    
        switch(cell->msg_type)
        {
	    case SCTK_SHM_EAGER: 
            msg = sctk_network_eager_msg_shm_recv(cell,0);
            if(msg) sctk_send_message_from_network_shm(msg);
		    break;
	    case SCTK_SHM_RDMA: 
		    msg = sctk_network_rdma_msg_shm_recv(cell,1);
                    if(msg) sctk_send_message_from_network_shm(msg);
		    break;
	    case SCTK_SHM_CMPL:
            msg = sctk_network_rdma_cmpl_msg_shm_recv(cell);
        	sctk_complete_and_free_message( msg ); 
		    break;
	    case SCTK_SHM_FRAG:
            	msg = sctk_network_frag_msg_shm_recv(cell,1);
    	    	if(msg) sctk_send_message_from_network_shm(msg);
		break;
	    default:
		    abort();
        }
    }
}

static void sctk_network_notify_recv_message_shm ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
    sctk_network_notify_idle_message_shm ( rail );
}

static void 
sctk_network_notify_any_source_message_shm ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
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
	filename = handler->recv_handler(handler->option);
	assume_m(filename != NULL,"Failed to get the SHM filename.");

	//firt try to map
	fd = sctk_shm_mapper_slave_open(filename);
	ptr = sctk_shm_mapper_mmap(NULL,fd,size);

    sctk_pmi_barrier();

	close(fd);
	free(filename);
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
	filename = handler->gen_filename(handler->option);

	//create file and map it
	fd = sctk_shm_mapper_create_shm_file(filename,size);
	ptr = sctk_shm_mapper_mmap(NULL,fd,size);

	//sync filename
	status = handler->send_handler(filename,handler->option);
	assume_m(status,"Fail to send the SHM filename to other participants.");

        sctk_pmi_barrier();

	close(fd);
	sctk_shm_mapper_unlink(filename);

	//free temp memory and return
	free(filename);
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
    sctk_rail_add_static_route ( rail, dest, new_route );

    /* set the route as connected */
    sctk_endpoint_set_state ( new_route, STATE_CONNECTED );

    //fprintf(stdout, "Add route to %d with endpoint %d\n", dest, shm_dest);     
    return;
}

static void sctk_shm_init_raw_queue(size_t size, int cells_num, int rank, int participants)
{
    char buffer[16];
    void *shm_base = NULL;
    sctk_shm_mapper_role_t shm_role;
    struct sctk_alloc_mapper_handler_s *pmi_handler = NULL;

    sprintf(buffer, "%d", rank);
    pmi_handler = sctk_shm_pmi_handler_init(buffer);

    shm_role = SCTK_SHM_MAPPER_ROLE_SLAVE;
    shm_role = (sctk_local_process_rank==rank)?SCTK_SHM_MAPPER_ROLE_MASTER:shm_role; 
    shm_base = sctk_shm_add_region( size, shm_role, pmi_handler);
        
    sctk_shm_add_region_infos(shm_base,size,cells_num,rank,participants );

    if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER )
        sctk_shm_reset_process_queues( rank );

    sctk_shm_pmi_handler_free(pmi_handler);
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
    sctk_pmi_barrier();
}

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

void sctk_network_init_shm ( sctk_rail_info_t *rail )
{
    int i, local_process_rank, local_process_number, first_proc_on_node;
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
	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, NULL );

	sctk_shmem_cells_num = 128;
//rail->runtime_config_driver_config->driver.value.shm.cells_num;
    sctk_shmem_size = sctk_shm_get_region_size(sctk_shmem_cells_num);
    fprintf(stderr, "size shm : %ld\n", sctk_shmem_size);

        sctk_shmem_size = sctk_shm_roundup_powerof2(sctk_shmem_size);

    sctk_pmi_get_process_on_node_rank(&local_process_rank);
    sctk_pmi_get_process_on_node_number(&local_process_number);
    sctk_shm_proc_local_rank_on_node = local_process_rank;
    first_proc_on_node = sctk_get_process_rank() - local_process_rank;

    sctk_shm_init_regions_infos(local_process_number);
    for( i=0; i<local_process_number; i++)
    {
        sctk_shm_init_raw_queue(sctk_shmem_size,sctk_shmem_cells_num,i,local_process_number);
		if( i != local_process_rank)
            sctk_shm_add_route(first_proc_on_node+i,i,rail);
        sctk_pmi_barrier();
    }

    sctk_shm_driver_initialized = 1;
    sctk_network_rdma_shm_interface_init();
    sctk_network_frag_shm_interface_init();
    sctk_shm_check_raw_queue(local_process_number);
}
