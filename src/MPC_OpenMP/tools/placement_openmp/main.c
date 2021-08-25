#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <mpi.h>
#include <omp.h>
#include <assert.h>
#include <pthread.h>
#include "placement.h"

int abort_or_not = 0;

const char * arg;
enum func log_enum;

/* global variables to a process to load and init 
   one time the topo for each process */
pthread_spinlock_t my_lock;
int ret;
int lock_value = 1;
hwloc_topology_t topology;

//static void print_children(hwloc_topology_t topology, hwloc_obj_t obj, 
//                           int depth)
//{
//    char string[128];
//    unsigned i;
//    hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
//    printf("depth %d %*s%s\n",depth, 2*depth, "", string);
//    for (i = 0; i < obj->arity; i++) {
//        print_children(topology, obj->children[i], depth + 1);
//    fflush(stdout);
//    }
//}


int main(int argc, char **argv) {


	pthread_spin_init(&my_lock, PTHREAD_PROCESS_PRIVATE);
	int size_info = sizeof(struct worker_info_s)/sizeof(int);
	int rank, size, i;
	struct worker_info_s *info;
	struct worker_info_s *info_wake_up;
	int sinfo = sizeof(struct worker_info_s);
	struct worker_info_s *rbuf; 
	struct worker_info_s *rbuf_wake_up; 
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	print_header(rank);
    int num_th = 0; //provisoire car num_threads() n est plus prioritaire sur OMP_NUM_THREADS

    int limit = omp_get_thread_limit();

    for(i = 1; i < argc; i++){	
        if(!strcmp(argv[i], "--limit")){
            limit = atoi(argv[i+1]); 
        }
        if(!strcmp(argv[i], "--num-th")){
            num_th = atoi(argv[i+1]); 
        }
    }

	int max_num_threads, local_num_threads, total_num_threads;

	int tid = syscall(SYS_gettid);
	int pid = syscall(SYS_getpid);

	/* First thread per process init and load topo */
	if(!pthread_spin_lock(&my_lock)){
		if(lock_value){
			lock_value = 0;

			ret = hwloc_topology_init(&topology);
			assert(ret == 0);
			ret = hwloc_topology_load(topology);
			assert(ret == 0);
		}
        /* print node topo */
        //print_children(topology, hwloc_get_root_obj(topology), 0);
		pthread_spin_unlock(&my_lock);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	/* get the maximum threads that can be awaken
	   by rank by MPC */
	int num = omp_get_max_threads();
#pragma omp parallel num_threads(limit)
	{
			/* threads open mp id local */
			local_num_threads = omp_get_num_threads();
	}

	/* Share the Open MP instance size max among all rank */
	MPI_Allreduce(&local_num_threads, &max_num_threads, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

	/* Struct with all the worker create by MPC */
	info = (struct worker_info_s*)malloc(max_num_threads*sizeof(struct worker_info_s));

	/* Struct with all the worker awake by MPC */
	info_wake_up = (struct worker_info_s*)malloc(max_num_threads*sizeof(struct worker_info_s));

	init_struct(size, max_num_threads, info);
	init_struct(size, max_num_threads, info_wake_up);

	/* Fill info from all workers created by MPC */
#pragma omp parallel num_threads(limit)
	{
		fill_info(info, rank, topology, size);
	}

	/* Fill info from all workers awaken by MPC for the instance below */
    if(num_th){
#pragma omp parallel num_threads(num_th)
        {
            fill_info(info_wake_up, rank, topology, size);
        }
    }
    else{
#pragma omp parallel
        {
            fill_info(info_wake_up, rank, topology, size);
        }
    }

	if (rank == 0) {
		/* Buffer for MPI_Gather */
		rbuf = (struct worker_info_s*)malloc(size*max_num_threads*sizeof(struct worker_info_s));
		rbuf_wake_up = (struct worker_info_s*)malloc(size*max_num_threads*sizeof(struct worker_info_s));
	}

	/* Gather info from all workers created by MPC */
	MPI_Gather(info, sinfo*max_num_threads, MPI_BYTE, rbuf, sinfo*max_num_threads, MPI_BYTE, 0, MPI_COMM_WORLD);

	/* Gather info from all workers awaken by MPC */
	MPI_Gather(info_wake_up, sinfo*max_num_threads, MPI_BYTE, rbuf_wake_up, sinfo*max_num_threads, MPI_BYTE, 0, MPI_COMM_WORLD);




	/* One rank uses the informations gathered */
	if (rank == 0) {
		/* Struct with informations about each rank */
		struct mpi_info_s *mpi_info = (struct mpi_info_s*)malloc(size*sizeof(struct mpi_info_s));

		/* Fill the MPI informations struct above */
		//init_gen_info(size, max_num_threads, info, mpi_info, topology);

		/* Print on stdout placement informations */

		printf("\n/* Placement MPC threads, * means thread awake */\n");
        //sort_buf(rbuf, rbuf_wake_up, size, max_num_threads);
		print_info_placement(rbuf, rbuf_wake_up, size, max_num_threads);

        fflush(stdout);
		/* Flags execution */
		for(i = 1; i < argc; i++){	
			/*if(!strcmp(argv[i], "-g")){
				write_info_in_log(rbuf, size, max_num_threads);
			}*/
            //TODO check argument mode (simple mixed ..etc appel les bonnes fonctions en fonction
			if(!strcmp(argv[i], "--simple-mixed-smt")){
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf, topology);//consecutive allocation
            }
			if(!strcmp(argv[i], "--simple-mixed")){
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf, topology);//consecutive allocation
            }
			if(!strcmp(argv[i], "--tid-unicity")){
				/* Check thread id unicity */
				tid_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
			}

			if(!strcmp(argv[i], "--consecutive-binding-pu")){
				/* Check if workers are consecutive on logical cores among MPC tasks */
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf, topology);
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf_wake_up, topology);
			}
			if(!strcmp(argv[i], "--consecutive-binding-core")){
				/* Check if wokers are consecutive on physical cores among MPC tasks */ 
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf, topology);
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf_wake_up, topology);
			}
			if(!strcmp(argv[i], "--local-unicity")){
				/* Check the unicity of openMP threads among an instance among MPC tasks */
				local_unicity(size, max_num_threads, rbuf);
				local_unicity(size, max_num_threads, rbuf_wake_up);
			}
			if(!strcmp(argv[i], "--pu-unicity")){
				/* Check if each processing units is used by only one worker */
				pu_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
			}
			if(!strcmp(argv[i], "--fill-pu")){
				/* Check if all the logical cores are used if 
				 * all the topology is access in enable-smt mode */
				fill_all_pu(size, max_num_threads, rbuf);
				fill_all_pu(size, max_num_threads, rbuf_wake_up);
			}
			if(!strcmp(argv[i], "--fill-core")){
				/* Check if all the physical cores are used if 
				 * all the topology is access without enable-smt mode */
				fill_all_core(size, max_num_threads, rbuf);
				fill_all_core(size, max_num_threads, rbuf_wake_up);
			}
			if(!strcmp(argv[i], "--scattered")){
				/* Check scattered placement */
				printf("\n/* Checking affinity scattered without smt mode */\n");
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* conscecutive core needed for allocated workers with all policy without enable-smt */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf, topology);//consecutive allocation
				//consecutive_binding_per_mpi_task_node(size, max_num_threads, rbuf, topology, 0);
				//consecutive_binding_per_mpi_task_node(size, max_num_threads, rbuf_wake_up, topology, 0);
				check_nb_obj_by_obj(rbuf,rbuf_wake_up, size, max_num_threads, topology, 0);

			}
			if(!strcmp(argv[i], "--scattered-smt")){
				printf("\n/* Checking affinity scattered with smt mode */\n");
				/* Check scattered placement in enable-smt mode */
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* consecutive binding pu needed for allocated workers with all policy with enable-smt */
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf, topology);//consecutive allocation
				//consecutive_binding_per_mpi_task_node(size, max_num_threads, rbuf, topology, 1);
				//consecutive_binding_per_mpi_task_node(size, max_num_threads, rbuf_wake_up, topology, 1);
				check_nb_obj_by_obj(rbuf,rbuf_wake_up, size, max_num_threads, topology, 1);
			}
			if(!strcmp(argv[i], "--balanced")){
				printf("\n/* Checking affinity balanced without smt mode */\n");
				/* Check balanced placement */
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				/* conscecutive core needed for allocated workers with all policy without enable-smt */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf, topology);//consecutive allocation
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* are physical core awake conscutively ? */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf_wake_up, topology);
			}
			if(!strcmp(argv[i], "--balanced-smt")){
				printf("\n/* Checking affinity balanced in smt mode */\n");
				/* Check balanced placement in enable-smt mode */
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				/* consecutive binding pu needed for allocated workers with all policy with enable-smt */
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf, topology);//consecutive allocation
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* are physical core awake in a balanced way? */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf_wake_up, topology);
				/* are workers awaken repartition is in a balanced way with enable-smt? */
				test_nb_pu_per_core(rbuf, rbuf_wake_up, mpi_info, topology, size, max_num_threads);
			}
			if(!strcmp(argv[i], "--compact-smt")){
				printf("\n/* Checking affinity compact in smt mode */\n");
				/* Check compact placement in enable-smt mode */
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				/* consecutive binding pu needed for allocated workers with all policy with enable-smt */
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf, topology);//consecutive allocation
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* are workers awake in a compact way with enable-smt? */
				consecutive_binding_per_mpi_task_pu(size, max_num_threads, rbuf_wake_up, topology);
			}
			if(!strcmp(argv[i], "--compact")){
				printf("\n/* Checking affinity compact without smt mode */\n");
				/* Check compact placement */
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				/* needed for allocated workers with all policy without enable-smt */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf, topology);//consecutive allocation
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);
				/* are workers awake in a compact way? */
				consecutive_binding_per_mpi_task_core(size, max_num_threads, rbuf_wake_up, topology);
			}
			if(!strcmp(argv[i], "--compare")){
                int j;
                const char * output;
                int find = 0;
                for(j = 1; j < argc; j++){	
                    if(!strcmp(argv[j], "-o")){
                        output = argv[j+1];
                        find = 1;
                    }
                }
                if(find == 0){
                    output = "bitmap.txt";
                }
				printf("\n/* Checking mpc and GNU OpenMP placement*/\n");
				local_unicity(size, max_num_threads, rbuf);
				tid_unicity(size, max_num_threads, rbuf);
				pu_unicity(size, max_num_threads, rbuf);
				/* needed for allocated workers with all policy without enable-smt */
				local_unicity(size, max_num_threads, rbuf_wake_up);
				tid_unicity(size, max_num_threads, rbuf_wake_up);
				pu_unicity(size, max_num_threads, rbuf_wake_up);

                hwloc_bitmap_t bitmap= hwloc_bitmap_alloc();
                compute_bitmap(bitmap, topology, rbuf_wake_up, size, max_num_threads);
                int bsize = 512;
                char temp[bsize];
                char * ptr;
                hwloc_bitmap_snprintf(temp, bsize, bitmap);
                /*temp[1] = '0';
                int k = 0;
                while(temp[k] != '\0'){
                    if(temp[k] == ',' || temp[k] == 'x'){
                        temp[k] = '0';
                    }
                    switch(temp[k]){
                    case 'f':
                        temp[k] = '1';
                    }
                    k++;
                }*/
                FILE *f = fopen(output, "a");
                int k=0;
                if(f != NULL){
                    while(temp[k] != '\0'){
                        k++;
                    }
                    temp[k] = '\n';
                    temp[k+1] = '\0';
                    fprintf(f, temp);
                    fclose(f);
                }
                printf("bitmap %s\n", temp);
                //return bitmap;
			}
		}
		/* Free memory */
		free(rbuf_wake_up);
		free(rbuf);
		free(mpi_info);
        //Tell the other rank to not abort() to because tests OK
        int k;
        for(k = 1; k < size; k++){
            MPI_Send(&abort_or_not, 1, MPI_INT, k, 1000, MPI_COMM_WORLD);
        }
	}
    else{
        MPI_Status sta;
        MPI_Recv(&abort_or_not, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, &sta);
        if(abort_or_not){
            abort();
        }
    }

	MPI_Finalize();

	/* Free memory */
	free(info);
	free(info_wake_up);

	return 0;
}

