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

#include "sctk_fast_cpy.h"

static int sctk_shm_proc_local_rank_on_node = -1;
static volatile int sctk_shm_driver_initialized = 0;

#ifdef MPC_USE_VIRTUAL_MACHINE
static char* sctk_qemu_shm_process_filename = NULL;
static size_t sctk_qemu_shm_process_size = 0;
static void* sctk_qemu_shm_shmem_base = NULL;
#endif /* MPC_USE_VIRTUAL_MACHINE */

#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

// FROM Henry S. Warren, Jr.'s "Hacker's Delight."
static long sctk_shm_roundup_powerof2(unsigned long n)
{
    assume( n < ( 1 << 31));
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n+1;
}

#ifdef MPC_USE_VIRTUAL_MACHINE
char *sctk_get_qemu_shm_process_filename( void )
{
    return sctk_qemu_shm_process_filename;
}

void sctk_set_qemu_shm_process_filename( char *filename )
{
    sctk_qemu_shm_process_filename = filename;
}

size_t sctk_get_qemu_shm_process_size( void )
{
    return sctk_qemu_shm_process_size;
}

void sctk_set_qemu_shm_process_size( size_t size )
{
    sctk_qemu_shm_process_size = size;
}

void *sctk_get_shm_host_pmi_infos(void)
{
    return sctk_qemu_shm_shmem_base;
}
#endif /* MPC_USE_VIRTUAL_MACHINE */

static void 
sctk_network_send_message_endpoint_shm ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
    vcli_cell_t * __cell = NULL;

    while(!__cell)
        __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, endpoint->data.shm.dest);
	
    /* get free cells from dest queue */
    //if( SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t ) < VCLI_CELLS_SIZE ) 
    //if( SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_t ) < 2*1024 ) 
    if( SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_t ) < 8*1024 ) 
    {
    	__cell->size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );
	__cell->msg_type = SCTK_SHM_EAGER;
    	FAST_MEMCPY( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       

    	if(SCTK_MSG_SIZE ( msg ) > 0)
        	sctk_net_copy_in_buffer( msg, (char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t) );          
        
    	vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
    	sctk_complete_and_free_message( msg ); 
    }
    else
    {
	//fprintf(stdout, "Send CMA SHM msg\n");
	struct iovec * tmp;
	sctk_shm_iovec_info_t * shm_iov;
    	__cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	
    	shm_iov = (sctk_shm_iovec_info_t *) sctk_malloc( 1 * sizeof( sctk_shm_iovec_info_t ));
    	assume(shm_iov != NULL);

	__cell->msg_type = SCTK_SHM_RDMA;
	
    	FAST_MEMCPY( __cell->data, (char*) msg, sizeof ( sctk_thread_ptp_message_body_t ));       
	shm_iov->msg = msg;
	shm_iov->pid = getpid();
	shm_iov->mpi_src = sctk_get_local_process_rank();
        //fprintf(stdout, "value : %d\n", shm_iov->mpi_src);
	
  	sctk_get_iovec_in_buffer( msg, &tmp, &(shm_iov->len));	
	shm_iov->ptr = (struct iovec*)((char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t) + sizeof(sctk_shm_iovec_info_t));
	
	assume( (char*) shm_iov->ptr + sizeof(struct iovec) < ( char*) __cell + VCLI_CELLS_SIZE );
	assume( sizeof(sctk_thread_ptp_message_body_t) + sizeof(sctk_shm_iovec_info_t) + sizeof(struct iovec) < VCLI_CELLS_SIZE );
	
	FAST_MEMCPY(( char*) __cell->data + sizeof(sctk_thread_ptp_message_body_t), shm_iov, sizeof(sctk_shm_iovec_info_t));

	FAST_MEMCPY(shm_iov->ptr, tmp, shm_iov->len * sizeof(struct iovec));
	//fprintf(stdout, "push to dest queue\n");
    	vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
	//fprintf(stdout, "pushed to dest queue\n");
    }
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
sctk_network_notify_recv_message_shm ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
}

static sctk_thread_ptp_message_t *
sctk_network_perform_eager_msg_shm( vcli_cell_t * __cell )
{
    size_t size;
    sctk_thread_ptp_message_t *msg; 
    void *inb, *body;

    size = __cell->size - sizeof ( sctk_thread_ptp_message_body_t ) + sizeof ( sctk_thread_ptp_message_t );
    msg = sctk_malloc ( size );
    assume(msg != NULL);

    /* copy header */ 
    inb = __cell->data;
    FAST_MEMCPY( (char *) msg, __cell->data, sizeof ( sctk_thread_ptp_message_body_t ));       
    if( size > sizeof ( sctk_thread_ptp_message_t ))
    {
        /* copy msg */
        body = ( char * ) msg + sizeof ( sctk_thread_ptp_message_t );
        inb += sizeof ( sctk_thread_ptp_message_body_t );
        size = size -  sizeof ( sctk_thread_ptp_message_t );
        FAST_MEMCPY(body, inb, size);       
    }
    
    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    return msg;	
}
 
static sctk_thread_ptp_message_t *
sctk_network_perform_rdma_msg_shm( vcli_cell_t * __cell )
{
    size_t size;
    sctk_shm_iovec_info_t * shm_iov;
    sctk_thread_ptp_message_t * msg; 
    struct iov * tmp;
    void *inb;
	
    msg = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t )  + sizeof(sctk_shm_iovec_info_t) );
    assume(msg != NULL);

    shm_iov = (sctk_shm_iovec_info_t *) ((char*) msg + sizeof ( sctk_thread_ptp_message_t ));
    FAST_MEMCPY( msg, __cell->data, sizeof ( sctk_thread_ptp_message_body_t ));       
    FAST_MEMCPY( shm_iov, __cell->data + sizeof ( sctk_thread_ptp_message_body_t ), sizeof(sctk_shm_iovec_info_t));
   	
    tmp = sctk_malloc( shm_iov->len * sizeof(struct iovec));
    assume( tmp != NULL);
    
    FAST_MEMCPY(tmp, (char *) __cell->data + sizeof ( sctk_thread_ptp_message_body_t ) + sizeof(sctk_shm_iovec_info_t), shm_iov->len * sizeof(struct iovec));
    shm_iov->ptr = tmp;
        
    SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
    msg->tail.message_type = SCTK_MESSAGE_NETWORK;
    return msg;	
}

void sctk_shm_rdma_message_copy ( sctk_message_to_copy_t * tmp )
{
    	vcli_cell_t * __cell = NULL;
	sctk_thread_ptp_message_t *send;
        sctk_thread_ptp_message_t *recv;
        char *body;
	struct iovec * recv_iov, * send_iov;
	sctk_shm_iovec_info_t * shm_send_iov;
	int nread;

        send = tmp->msg_send;
        recv = tmp->msg_recv;

        body = ( char * ) send + sizeof ( sctk_thread_ptp_message_t );
        SCTK_MSG_COMPLETION_FLAG_SET ( send , NULL );

        sctk_nodebug ( "MSG |%s|", ( char * ) body );
        switch ( recv->tail.message_type )
        {
                case SCTK_MESSAGE_CONTIGUOUS:
                {
                        size_t size;
                        size = SCTK_MSG_SIZE ( send );
                        size = sctk_min ( SCTK_MSG_SIZE ( send ), recv->tail.message.contiguous.size );
			
			recv_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
			recv_iov->iov_base = recv->tail.message.contiguous.addr;
			recv_iov->iov_len = size;	
			
			send_iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ));
			shm_send_iov = (sctk_shm_iovec_info_t *)(( char * ) send + sizeof ( sctk_thread_ptp_message_t ));
			send_iov->iov_base = shm_send_iov->ptr->iov_base;	
			send_iov->iov_len = size;	
			
			nread = process_vm_readv(shm_send_iov->pid, recv_iov, 1, send_iov, 1, 0);
        		if(nread < 0)
           	 		perror("process_vm_readv:");
			
                        sctk_message_completion_and_free ( send, recv );
			sctk_free(recv_iov);
			sctk_free(send_iov);
                        break;
                }
		default:
			abort();
	}

	//fprintf(stdout, "[COPY] resend to %d\n", shm_send_iov->mpi_src);
    	while(!__cell)
        	__cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, shm_send_iov->mpi_src);

    	/* get free cells from dest queue */
	__cell->msg_type = SCTK_SHM_CMPL;
   	__cell->size = sizeof ( sctk_thread_ptp_message_body_t );
	FAST_MEMCPY( __cell->data, shm_send_iov, sizeof(sctk_shm_iovec_info_t) );
	//fprintf(stdout, "[COPY] push to src queue\n");
    	vcli_raw_push_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID, __cell);       
	//fprintf(stdout, "[COPY] pushed to src queue\n");
}

void sctk_shm_rdma_message_free ( void * tmp )
{
	//sctk_shm_iovec_info_t * iov = (sctk_shm_iovec_info_t *) ((char*) tmp + sizeof ( sctk_thread_ptp_message_t ));
	//sctk_free( iov->ptr );
	//sctk_free( tmp );
}

static void 
sctk_network_notify_idle_message_shm ( sctk_rail_info_t *rail )
{
    vcli_cell_t * __cell;
    sctk_thread_ptp_message_t *msg; 

    if(!sctk_shm_driver_initialized)
        return;

    __cell = vcli_raw_pop_cell(SCTK_QEMU_SHM_RECV_QUEUE_ID,sctk_shm_proc_local_rank_on_node); 

    if( __cell == NULL)
        return;

    assume_m( __cell->size >= sizeof ( sctk_thread_ptp_message_body_t ), "Incorrect Msg\n");
    
    sctk_shm_iovec_info_t* shm_iov;

    switch(__cell->msg_type)
    {
	case SCTK_SHM_EAGER: 
		msg = sctk_network_perform_eager_msg_shm( __cell );
    		vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    		if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
	    		return;
    		sctk_rebuild_header ( msg ); 
    		sctk_reinit_header ( msg, sctk_free, sctk_net_message_copy );
    		sctk_send_message_from_network_shm ( msg );
		break;
	case SCTK_SHM_RDMA: 
		msg = sctk_network_perform_rdma_msg_shm( __cell );
    		vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
    		if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
	    		return;
    		sctk_rebuild_header ( msg ); 
    		sctk_reinit_header ( msg, sctk_shm_rdma_message_free, sctk_shm_rdma_message_copy );
    		sctk_send_message_from_network_shm ( msg );
		break;
	case SCTK_SHM_CMPL:
		shm_iov = __cell->data;
		msg = shm_iov->msg;
    		sctk_complete_and_free_message( msg ); 
    		vcli_raw_push_cell(SCTK_QEMU_SHM_FREE_QUEUE_ID, __cell);       
		break;
	default:
		abort();
    }

}

static void 
sctk_network_notify_any_source_message_shm ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}

/********************************************************************/
/* SHM Init                                                         */
/********************************************************************/

#ifdef MPC_USE_VIRTUAL_MACHINE
void sctk_shm_host_pmi_init( void )
{
	int sctk_shm_vmhost_node_rank;
	int sctk_shm_vmhost_node_number;
	int sctk_vmhost_shm_local_process_rank;
	int sctk_vmhost_shm_local_process_number;
    	sctk_shm_guest_pmi_infos_t *shm_host_pmi_infos;

    	shm_host_pmi_infos = (sctk_shm_guest_pmi_infos_t*) sctk_get_shm_host_pmi_infos();
    
	sctk_pmi_get_node_rank ( &sctk_shm_vmhost_node_rank );
	sctk_pmi_get_node_number ( &sctk_shm_vmhost_node_number );
	sctk_pmi_get_process_on_node_rank ( &sctk_vmhost_shm_local_process_rank );
	sctk_pmi_get_process_on_node_number ( &sctk_vmhost_shm_local_process_number );
    
    /* Set pmi infos for SHM infos */ 
    
    //shm_host_pmi_infos->sctk_pmi_procid = ;
    //shm_host_pmi_infos->sctk_pmi_nprocs = ; 
}
#endif /* MPC_USE_VIRTUAL_MACHINE */

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

static void sctk_shm_init_raw_queue(size_t sctk_shmem_size, int sctk_shmem_cells_num, int rank)
{
    char buffer[16];
    void *shm_base = NULL;
    sctk_shm_mapper_role_t shm_role;
    struct sctk_alloc_mapper_handler_s *pmi_handler = NULL;

#ifdef MPC_USE_VIRTUAL_MACHINE
    if( sctk_mpc_is_vmguest() )
    {
        printf("MPC is vmguest process\n");
        shm_base = sctk_mpc_get_vmguest_shmem_base();
        sctk_qemu_shm_shmem_base = shm_base;
        shm_base += sizeof( sctk_shm_guest_pmi_infos_t );
        sctk_vcli_raw_infos_add( shm_base, sctk_shmem_size, sctk_shmem_cells_num, rank );
    }
    else
    {
#endif /* MPC_USE_VIRTUAL_MACHINE */
        sprintf(buffer, "%d", rank);
        pmi_handler = sctk_shm_pmi_handler_init(buffer);
        shm_role = (sctk_shm_proc_local_rank_on_node == rank) ? SCTK_SHM_MAPPER_ROLE_MASTER : SCTK_SHM_MAPPER_ROLE_SLAVE; 
        shm_base = sctk_shm_add_region(sctk_shmem_size,shm_role,pmi_handler);

#ifdef MPC_USE_VIRTUAL_MACHINE
        if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER && sctk_mpc_is_vmhost() )
        {
            printf("MPC is vmhost process\n");
            //vcli_raw_reset_infos_reset( rank );
            sctk_qemu_shm_shmem_base = shm_base;
            shm_base += sizeof( sctk_shm_guest_pmi_infos_t );
        }
#endif /* MPC_USE_VIRTUAL_MACHINE */

        sctk_vcli_raw_infos_add( shm_base, sctk_shmem_size, sctk_shmem_cells_num, rank );

        if( shm_role == SCTK_SHM_MAPPER_ROLE_MASTER )
        {
            vcli_raw_reset_infos_reset( rank );
#ifdef MPC_USE_VIRTUAL_MACHINE
            sctk_set_qemu_shm_process_filename(pmi_handler->gen_filename(pmi_handler->option));
            sctk_set_qemu_shm_process_size(sctk_shmem_size);
#endif /* MPC_USE_VIRTUAL_MACHINE */
        }
        sctk_shm_pmi_handler_free(pmi_handler);

#ifdef MPC_USE_VIRTUAL_MACHINE
    }
#endif /* MPC_USE_VIRTUAL_MACHINE */

}

void sctk_shm_check_raw_queue(int local_process_number)
{
    int i;
#ifdef SCTK_SHM_CHECK_RAW_QUEUE
    for( i=0; i < local_process_number; i++)
    {
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_SEND_QUEUE_ID,i),"Queue must be empty")
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_RECV_QUEUE_ID,i),"Queue must be empty")
        assume_m( vcli_raw_empty_queue(SCTK_QEMU_SHM_CMPL_QUEUE_ID,i),"Queue must be empty")
        assume_m(!vcli_raw_empty_queue(SCTK_QEMU_SHM_FREE_QUEUE_ID,i),"Queue must be full")
    }
    sctk_pmi_barrier();
#endif /* SCTK_SHM_CHECK_RAW_QUEUE */
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

	sctk_shmem_cells_num = rail->runtime_config_driver_config->driver.value.shm.cells_num;
    	sctk_shmem_size = sctk_shm_get_raw_queues_size(sctk_shmem_cells_num);

#ifdef MPC_USE_VIRTUAL_MACHINE
    if( sctk_mpc_is_vmhost() || sctk_mpc_is_vmguest() )
        sctk_shmem_size = sctk_shm_roundup_powerof2(sctk_shmem_size);
#endif /* MPC_USE_VIRTUAL_MACHINE */

    sctk_pmi_get_process_on_node_rank(&local_process_rank);
    sctk_pmi_get_process_on_node_number(&local_process_number);
    sctk_shm_proc_local_rank_on_node = local_process_rank;
    first_proc_on_node = sctk_get_process_rank() - local_process_rank;

#ifdef MPC_USE_VIRTUAL_MACHINE
    if( sctk_mpc_is_vmguest())
    {
        sctk_vcli_raw_infos_init( 1 );
        sctk_shm_init_raw_queue(sctk_shmem_size, sctk_shmem_cells_num,0);
        int process_number = sctk_get_process_rank();
        for( i=0; i<process_number; i++)
            sctk_shm_add_route(i,0,rail);
    }
    else
    {
#endif /* MPC_USE_VIRTUAL_MACHINE */    
        sctk_vcli_raw_infos_init(local_process_number);
        for( i=0; i<local_process_number; i++)
        {
            	sctk_shm_init_raw_queue(sctk_shmem_size, sctk_shmem_cells_num, i);
		if( i != local_process_rank)
            		sctk_shm_add_route(first_proc_on_node+i,i,rail);
        }
#ifdef MPC_USE_VIRTUAL_MACHINE
    }
#endif /* MPC_USE_VIRTUAL_MACHINE */    

    sctk_shm_driver_initialized = 1;
    fprintf(stdout, "vcli cells size : %d\n", VCLI_CELLS_SIZE);
}
