
#ifdef MPC_USE_VIRTUAL_MACHINE
/*static char* sctk_qemu_shm_process_filename = NULL;*/
/*static size_t sctk_qemu_shm_process_size = 0;*/
static void* sctk_qemu_shm_shmem_base = NULL;

// FROM Henry S. Warren, Jr.'s "Hacker's Delight."
static long mpc_common_roundup_powerof2(unsigned long n)
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

/*char *sctk_get_qemu_shm_process_filename( void )*/
/*{*/
    /*return sctk_qemu_shm_process_filename;*/
/*}*/

/*void sctk_set_qemu_shm_process_filename( char *filename )*/
/*{*/
    /*sctk_qemu_shm_process_filename = filename;*/
/*}*/

/*size_t sctk_get_qemu_shm_process_size( void )*/
/*{*/
    /*return sctk_qemu_shm_process_size;*/
/*}*/

/*void sctk_set_qemu_shm_process_size( size_t size )*/
/*{*/
    /*sctk_qemu_shm_process_size = size;*/
/*}*/

void *sctk_get_shm_host_pmi_infos(void)
{
    return sctk_qemu_shm_shmem_base;
}

void sctk_shm_host_pmi_init( void )
{
	int sctk_shm_vmhost_node_rank;
	int sctk_shm_vmhost_node_number;
	int sctk_vmhost_shm_local_process_rank;
	int sctk_vmhost_shm_local_process_number;
    	sctk_shm_guest_pmi_infos_t *shm_host_pmi_infos;

    	shm_host_pmi_infos = (sctk_shm_guest_pmi_infos_t*) sctk_get_shm_host_pmi_infos();
    
 mpc_launch_pmi_get_node_rank ( &sctk_shm_vmhost_node_rank );
 mpc_launch_pmi_get_node_number ( &sctk_shm_vmhost_node_number );
 mpc_launch_pmi_get_process_on_node_rank ( &sctk_vmhost_shm_local_process_rank );
 mpc_launch_pmi_get_process_on_node_number ( &sctk_vmhost_shm_local_process_number );
    
    /* Set pmi infos for SHM infos */ 
    
    //shm_host_pmi_infos->sctk_pmi_procid = ;
    //shm_host_pmi_infos->sctk_pmi_nprocs = ; 
}
#endif /* MPC_USE_VIRTUAL_MACHINE */
