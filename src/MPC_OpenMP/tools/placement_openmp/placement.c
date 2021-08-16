#include "placement.h"

void init_struct(int size, int max_num_threads, struct worker_info_s *info){
    log_enum = _init_struct;
    int i;
    for (i=0; i<max_num_threads; i++) {
        info[i].local_id = -1;
        info[i].tid = -1;
        info[i].mpi_rank = -1;
        info[i].core = -1;
        info[i].pu = -1;
        info[i].node = -1;
        info[i].tile = -1;
        info[i].processus_id = -1;
    } 
}

/* init general MPI informations in a struct mpi_info_s about */
void init_gen_info(int size, int max_num_threads, struct worker_info_s *info, struct mpi_info_s *mpi_info, hwloc_topology_t topology){
    log_enum = _init_gen_info;
    int nb_core = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    int nb_pu = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    int *core_check = calloc(nb_core, sizeof(int));
    int *pu_check = calloc(nb_pu, sizeof(int));
    int i, j, k;
    for(i = 0; i < size; i++){
        mpi_info[i].first_pu = size * max_num_threads;
        mpi_info[i].last_pu = 0;
        mpi_info[i].nb_loc_th = 0;
        mpi_info[i].nb_core = 0;
        mpi_info[i].nb_pu = 0;
        for(j = 0; j < max_num_threads; j++){
            if(mpi_info[i].first_pu > info[j].pu){
                mpi_info[i].first_pu = info[j].pu;
            }
            if(mpi_info[i].last_pu < info[j].pu){
                mpi_info[i].last_pu = info[j].pu;
            }
            if(info[j].mpi_rank == i){
                mpi_info[i].nb_loc_th++;
            }
            if(core_check[info[j].core] == 0){
                core_check[info[j].core]++;
            }
            if(pu_check[info[j].pu] == 0){
                pu_check[info[j].core]++;
            }

        }
        for(k = 0; k < nb_core; k++){
            if(core_check[k]){
                mpi_info[i].nb_core++;
            }
        }
        for(k = 0; k < nb_pu; k++){
            if(pu_check[k]){
                mpi_info[i].nb_pu++;
            }
        }
    }
    //free(core_check);
    //free(pu_check);
}

/* Check the number and the repartition of pu among tasks in balanced smt mode */
void test_nb_pu_per_core(struct worker_info_s *info,struct worker_info_s *info_w, 
        struct mpi_info_s *mpi_info, 
        hwloc_topology_t topology, int size, int max_num_threads){

    log_enum = _test_nb_pu_per_core;

    int i, j;
    int nb_core_tot = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    int **nb_pu_per_core = (int **)malloc(size*sizeof(int*));
    int **nb_pu_per_core_w = (int **)malloc(size*sizeof(int*));
    for(i = 0; i < size; i++){
        nb_pu_per_core[i] = (int *)calloc(nb_core_tot ,sizeof(int));
    }

    for(i = 0; i < size; i++){
        nb_pu_per_core_w[i] = (int *)calloc(nb_core_tot ,sizeof(int));
    }

    for(i = 0; i < size; i++){
        for(j = 0; j < size * max_num_threads; j++){
            if(info[j].mpi_rank == i){
                maj_nb_core_per_task(i, info[j].core, nb_pu_per_core);
            }
            if(info_w[j].mpi_rank == i){
                maj_nb_core_per_task(i, info_w[j].core, nb_pu_per_core_w);
            }
        }
    }

    /* check number and repartition of pu per core of awaken threads for each tasks */
    for(i = 0; i < size; i++){
        int first = 0;
        while(1){
            if(nb_pu_per_core[i][first] != 0){	
                break;
            }
            first++;
        }
        int end = first;
        while(1){
            if(nb_pu_per_core[i][end] == 0){
                break;
            }
            end++;
            if( end == nb_core_tot){
                break;
            }
        }
        int first_w = 0;
        while(1){
            if(nb_pu_per_core_w[i][first_w] != 0){	
                break;
            }
            first_w++;
        }
        int end_w = first_w;
        while(1){
            if(nb_pu_per_core_w[i][end_w] == 0){
                break;
            }
            end_w++;
            if( end_w == nb_core_tot){
                break;
            }
        }


        /* Only one core booked */
        if(end - first == 1){
            if(nb_pu_per_core_w[i][first_w] > nb_pu_per_core[i][first_w]){
                abort_log();
            }
        }

        /* Two cores */
        if(end - first == 2){
            if(nb_pu_per_core_w[i][first_w] > nb_pu_per_core[i][first_w]){
                abort_log();
            }
            if(nb_pu_per_core_w[i][first_w+1] > nb_pu_per_core[i][first_w+1]){
                abort_log();
            }
            if(nb_pu_per_core_w[i][first_w] < nb_pu_per_core_w[i][first_w+ 1]){
                if(nb_pu_per_core_w[i][first_w] != nb_pu_per_core[i][first_w]){
                    abort_log();
                }
            }
            else if(nb_pu_per_core_w[i][first_w] > nb_pu_per_core_w[i][first_w+ 1]){
                if(nb_pu_per_core_w[i][first_w] - nb_pu_per_core_w[i][first_w+1] > 1){
                    abort_log();
                }
            }

        }

        /* More than thwo cores */
        if(end - first > 2){
            int altern = 2;
            /* first core */
            if(nb_pu_per_core_w[i][first_w] > nb_pu_per_core_w[i][first_w + 1] + 1){
                abort_log();
            }
            else if(nb_pu_per_core_w[i][first_w] == nb_pu_per_core_w[i][first_w+ 1] + 1){
                altern--;
            }
            /* midlle cores */
            for(j = first_w + 2; j < end_w - 1; j++){
                if(nb_pu_per_core_w[i][j] > nb_pu_per_core_w[i][j-1]){
                    abort_log();
                }
                else if( nb_pu_per_core_w[i][j] == nb_pu_per_core_w[i][j-1]){
                    if(altern == 0){ 
                        /* Already a -1 has been done */
                        abort_log(); 
                    }
                }
                else{
                    if(nb_pu_per_core_w[i][j] != nb_pu_per_core_w[i][first_w + 1] - 1){
                        abort_log();
                    }
                    altern--;
                }
            }
            /* last core */
            if(end_w - first_w > 1){
                if(nb_pu_per_core_w[i][end_w - 1] > nb_pu_per_core_w[i][end_w - 2]){
                    abort_log();
                }
                else{
                    if(nb_pu_per_core_w[i][end_w - 2] - nb_pu_per_core_w[i][end_w - 1] > 1){
                        if(nb_pu_per_core_w[i][end_w - 1] != nb_pu_per_core[i][end_w - 1]){
                            abort_log();
                        }
                    }	
                }
            }
            /* if threads booked are wrongly unused */
            if(end > end_w){
                if(nb_pu_per_core[i][end_w] > 0 && nb_pu_per_core_w[i][first_w] > 1){
                    abort_log();
                }
            }
        }
    }
    printf("\n  repartition of pus in balanced smt mode OK\n");
    for(i = 0; i < size; i++){
        free(nb_pu_per_core[i]);
    }

    for(i = 0; i < size; i++){
        free(nb_pu_per_core_w[i]);
    }
    free(nb_pu_per_core);
    free(nb_pu_per_core_w);


}

void maj_nb_core_per_task(int i, int core, int **nb_pu_per_core){
    nb_pu_per_core[i][core]++; 
}
void maj_nb_obj_per_task(int i, int obj,int id_depth_2, int **nb_obj_per_type, int *tab_already_check){
    if(!tab_already_check[id_depth_2]){
        nb_obj_per_type[i][obj]++; 
        tab_already_check[id_depth_2] = 1;
    }
}

void maj_nb_obj_per_task_refine(int i, int obj, int **nb_obj_per_type){
    nb_obj_per_type[i][obj]++; 
}

/* Compute the higher Cache level id for a given logical core */
//int compute_tile_idx(hwloc_topology_t topology, hwloc_obj_t pu){
//    hwloc_obj_t next;
//    if(pu->parent != NULL){
//        next = pu->parent;
//    }
//    else{
//        return -1;
//    }
//    int id;
//    while(next->type != HWLOC_OBJ_CACHE){
//        if(next->parent != NULL){
//            next = next->parent;
//        }
//        else{
//            return -1;
//        }
//    }
//
//    hwloc_obj_t prev = next;
//    if(next->parent != NULL){
//        next = next->parent;
//    }
//    else{
//        return -1;
//    }
//    while(next->type == HWLOC_OBJ_CACHE){
//        prev = next;
//        if(next->parent != NULL){ next = next->parent;
//        }
//        else{
//            return -1;
//        }
//    }
//    id = prev->logical_index;
//    if(prev != NULL){
//        return id;
//    }
//    else{
//        return -1;
//    }
//}

char * get_info_by_name(hwloc_obj_t obj, const char *name)
{
    int i;
    for(i=0; i<obj->infos_count; i++)
        if (!strcmp(obj->infos[i].name, name))
            return obj->infos[i].value;
    return NULL;
}

/* Threads register their informations in struct worker_info_s */
void fill_info(struct worker_info_s *info, int rank, hwloc_topology_t topology, int size){
    log_enum = _fill_info;

    int ret;
    int thread_num = omp_get_thread_num();
    info[thread_num].local_id = thread_num;
    info[thread_num].tid = syscall(SYS_gettid);
    info[thread_num].processus_id = syscall(SYS_getpid);
    info[thread_num].mpi_rank = rank;

    hwloc_cpuset_t newset;
    newset = hwloc_bitmap_alloc();
    ret = hwloc_get_last_cpu_location(topology, newset, HWLOC_CPUBIND_THREAD);
    assert(ret == 0);
    hwloc_obj_t obj;
    obj = hwloc_get_obj_inside_cpuset_by_type(topology, newset,HWLOC_OBJ_PU, 0);
    info[thread_num].pu = obj->logical_index;
    info[thread_num].pu_os = obj->os_index;
    hwloc_obj_t core = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, obj);
    if(core == NULL){
        abort_log();
    }
    //TODO assert core == NULL
    info[thread_num].core = core->logical_index;
    //hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, obj);
    //hwloc_obj_t node = find_node_by_pu(topology,info[thread_num].pu);
    //if(node == NULL){
    //    abort_log();
    //}
    //info[thread_num].node = node->logical_index;
    //int ind_tile = compute_tile_idx(topology, obj);
    //if(ind_tile >= 0){
    //    info[thread_num].tile = ind_tile;
    //}
    hwloc_obj_t cluster = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_MACHINE, obj);
    if(cluster == NULL){
        abort_log();
    }
    strcpy(info[thread_num].cluster, "");
    strcpy(info[thread_num].cluster , hwloc_obj_get_info_by_name(cluster, "HostName"));

}

/* Print info about knl via hwloc */
/*void print_info_knl(hwloc_topology_t topo){
    log_enum = _print_info_knl;
    hwloc_obj_t root;
    const char *cluster_mode;
    const char *memory_mode;

    char info_root[4098];
    hwloc_topology_init(&topo);
    hwloc_topology_load(topo);

    root = hwloc_get_root_obj(topo);

    cluster_mode = hwloc_obj_get_info_by_name(root, "ClusterMode");
    hwloc_obj_attr_snprintf(info_root, 4098, root, " -- ",1);
    memory_mode = hwloc_obj_get_info_by_name(root, "MemoryMode");

    printf ("ClusterMode is '%s' MemoryMode is '%s'\n",
            cluster_mode ? cluster_mode : "NULL",
            memory_mode ? memory_mode : "NULL");
    printf("Info root : %s\n", info_root);
}*/

/* To determine where abort() happened */
void string_abort(char * info){
    switch(log_enum){
        case _test_nb_pu_per_core:
            strcpy(info,  "test_nb_pu_per_core    ");
            break;

        case _test_nb_pu_per_node:
            strcpy(info,  "test_nb_pu_per_node");
            break;


        case _check_compact_policy:
            strcpy(info, "check_compact_policy    ");
            break;

        case _check_binding_obj:
            strcpy(info, "check_binding_obj");
            break;

        case _check_binding_obj_smt:
            strcpy(info, "check_binding_obj_smt");
            break;

        case _check_nb_obj_by_obj_smt:
            strcpy(info, "check_nb_obj_by_obj_smt");
            break;

        case _check_nb_obj_by_obj:
            strcpy(info, "check_nb_obj_by_obj");
            break;

        case _test_balanced_placement:
            strcpy(info, "test_balanced_placement    ");
            break;

        case _maj_nb_core_per_task:
            strcpy(info, "maj_nb_core_per_task    ");
            break;

        case _print_info_knl:
            strcpy(info, "print_info_knl    ");
            break;

        case _write_info_in_log:
            strcpy(info, "write_info_in_log    ");
            break;

        case _fill_all_core:
            strcpy(info, "fill_all_core    ");
            break;

        case _print_info_placement:
            strcpy(info, "print_info_placement    ");
            break;

        case _print_info_placement_wake_up:
            strcpy(info, "print_info_placement_wake_up    ");
            break;

        case _fill_info:
            strcpy(info, "fill_info    ");
            break;

        case _print_header:
            strcpy(info, "print_header    ");
            break;

        case _tid_unicity:
            strcpy(info, "tid_unicity    ");
            break;

        case _consecutive_binding_per_mpi_task_pu:
            strcpy(info, "consecutive_binding_per_mpi_task_pu    ");
            break;

        case _consecutive_binding_per_mpi_task_core:
            strcpy(info, "consecutive_binding_per_mpi_task_core    ");
            break;

        case _local_unicity:
            strcpy(info, "local_unicity    ");
            break;

        case _pu_unicity:
            strcpy(info, "pu_unicity    ");
            break;

        case _fill_all_pu:
            strcpy(info, "fill_all_pu    ");
            break;

        case _init_struct:
            strcpy(info, "init_struct    ");
            break;
    }
}

void remove_char(char *str,int c){
    int i;
    for(i = 0; i < c; i++){
        int x = 0;
        while(str[x] != '\0'){
            str[x] = str[x+1];
            x++;
        }
    }
}

/* To determine where abort() happened and output in a log file */
void abort_log(void){
    char test [128];
    strcpy(test, arg);
    FILE *f = NULL;
    remove_char(test, 2);
    strcat(test, "-log");
    f = fopen(test, "a");
    int done = 0;
    char info[128];
    string_abort(info);
    if(f != NULL){
        strcat(info, " FAILED\n");
        fprintf(f, info);
        fclose(f);
    }
    //Tell the other rank to abort() to
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    abort_or_not = 1; // send in main to other ranks to abort()
    int k;
    for(k = 1; k < size; k++){
        MPI_Send(&abort_or_not, 1, MPI_INT, k, 1000, MPI_COMM_WORLD);
    }
    abort();
}

void print_header(int rank) {
    log_enum = _print_header;
    if (rank == 0) {
        printf("\n");
        //printf("TESTING OPENMP WORKERS POS\n");
        printf("--------------------------\n\n");
    }
}

/* Check each thread Open MP has an unique thread id */
void tid_unicity(int size, int max_num_threads, struct worker_info_s *rbuf){
    log_enum = _tid_unicity;
    /* Check tid unicity */
    int i, j;
    printf("* Checking tid unicity... \n");
    for (i = 0; i<size*max_num_threads; i++) {
        if (rbuf[i].mpi_rank != -1) {
            int j;
            /* looking if tid already existed */
            for (j=0; j<i; j++) {
                if (rbuf[i].tid == rbuf[j].tid && rbuf[i].cluster != rbuf[j].cluster) { /* TID are unique among different processes on the same node */
                    printf("  unicity: FAILED\n");
                    abort_log();
                }
            }
        }
    }
    printf("pid unicity: OK\n\n");
}

/* Print threads placement */
void print_info_placement(struct worker_info_s *rbuf,struct worker_info_s *rbuf_wake_up , int size, int max_num_threads){
    log_enum = _print_info_placement;
    int i, j;
    for (i=0; i<size; i++) {
        int mpirank = i;
        printf("\n In MPI task:%d \n", i);

        for (j=0; j<size*max_num_threads; j++) {
            if (rbuf[j].mpi_rank == mpirank) {
                int k;
                for(k = 0; k<size*max_num_threads; k++){
                //if(j==4){
                //}
                    if(rbuf_wake_up[k].tid == rbuf[j].tid /*&& (strcmp(rbuf[j].cluster ,  rbuf_wake_up[k].cluster) == 0)*/) { //if the thread is awake
                        printf("  WORKER %d * awake \t[MPI rank:%d,  cluster node %s,  NUMA node id:%d,  higher cache id:%d,  core id:%d,  pu id:%d,  pu os id:%d,  local id:%d,  tid %d,  processus id %d]\n",
                                j, rbuf[j].mpi_rank,rbuf[j].cluster, rbuf[j].node,rbuf[j].tile, rbuf[j].core, rbuf[j].pu,rbuf[j].pu_os, rbuf[j].local_id, rbuf[j].tid, rbuf[j].processus_id);
                        fflush(stdout);
                        goto next;
                    }
                }
                printf("  WORKER %d \t\t[MPI rank:%d,  cluster node %s,  NUMA node id:%d,  higher cache id:%d,  core id:%d,  pu id:%d,  pu os id:%d,  local id:%d,  tid %d,  processus id %d]\n",
                        j, rbuf[j].mpi_rank,rbuf[j].cluster, rbuf[j].node,rbuf[j].tile, rbuf[j].core, rbuf[j].pu,rbuf[j].pu_os, rbuf[j].local_id, rbuf[j].tid, rbuf[j].processus_id);
                fflush(stdout);
            }
next:;

        }
    }
}

/* Check if logical cores indices are consecutive among MPI tasks */
void consecutive_binding_per_mpi_task_pu(int size, int max_num_threads, struct worker_info_s *rbuf, hwloc_topology_t topology){
    log_enum = _consecutive_binding_per_mpi_task_pu;
    /* Check consecutive binding per mpi task on pu */
    printf("\n* Checking binding on pu... \n");
    int i, j;
    int nb_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    for (i=0; i<size; i++) {
        int mpirank = i;
        printf("\n  In MPI task:%d \n", i);
        int first_id = nb_pus;
        int last_id = 0;
        int count = 0;

        /* for each mpi rank, count the thread open mp and take the first id and last id */
        for (j=0; j<size*max_num_threads; j++) {
            if (rbuf[j].mpi_rank == mpirank) {
                if (first_id > rbuf[j].pu) first_id = rbuf[j].pu;
                if (last_id < rbuf[j].pu) last_id = rbuf[j].pu;
                count++;
            }
        }

        /* Consecutive ? */
        if (last_id - first_id == count-1) { 
            printf("consecutive pu ids: OK\n");
        } else {
            printf("last - first = %d, count - 1 %d\n", last_id - first_id, count - 1);
            printf("  consecutive pu ids: FAILED\n");
            abort_log();
        }

    }
}

/* Check if physical cores indices are conscutive among MPI tasks */
void consecutive_binding_per_mpi_task_core(int size, int max_num_threads, struct worker_info_s *rbuf, hwloc_topology_t topology){
    log_enum = _consecutive_binding_per_mpi_task_core;
    /* Check consecutive binding per mpi task on core*/
    printf("\n* Checking binding on core... \n");
    int i, j;
    int nb_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    for (i=0; i<size; i++) {
        int * already_check = calloc(nb_cores,sizeof(int));
        int mpirank = i;
        printf("\n  In MPI task:%d \n", i);
        int first_id = nb_cores;
        int last_id = 0;
        int count = 0;

        /* for each mpi rank, count the thread open mp and take the first id and last id */
        for (j=0; j<size*max_num_threads; j++) {
            if (rbuf[j].mpi_rank == mpirank) {
                if (first_id > rbuf[j].core) first_id = rbuf[j].core;
                if (last_id < rbuf[j].core) last_id = rbuf[j].core;
                if(already_check[rbuf[j].core] == 0){
                    already_check[rbuf[j].core] = 1;
                    count++;
                }
            }
        }

        /* Consecutive ? */
        if (last_id - first_id == count-1) { 
            printf("  consecutive core ids: OK\n");
        } else {
            printf("  consecutive core ids: FAILED\n");
            abort_log();
        }
        free(already_check);
    }
}

/* Check the distribution of obj among their parents for scattered policy */
void check_nb_obj_by_obj(struct worker_info_s *infos,struct worker_info_s *infos_w, int size, int max_num_threads, hwloc_topology_t topology, int smt){
    log_enum = _check_nb_obj_by_obj_smt;
    printf("\n* Checking repartition of ressources in scattered mode ...\n");
    fflush(stdout);
    int depth_pu;
    if(smt){
        depth_pu= hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
    }
    else{
        depth_pu= hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
    }
    int i, t;
    int depth;
    for(t = 0; t < size; t++){
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE) - 1;
        int depth_ret = depth + 2;
        while(depth_ret != (depth_pu)){
            depth_ret = algo_check_nb_obj_by_obj(t, &depth, infos, infos_w, size, max_num_threads, topology);
            depth++;
        }
    }
    printf("  Repartition of ressources in scattered mode OK\n");
}

/* Check binding of obj of different types for scattered smt policy */
void check_binding_obj_smt(int task, struct worker_info_s *infos, int size, int max_num_threads, hwloc_topology_t topology){
    log_enum = _check_binding_obj_smt;
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE);
    int depth_pu = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
    int i;
    while(depth != depth_pu){
        algo_check_binding(task, &depth, infos, size, max_num_threads, topology);
        depth++;
    }       
}

/* Check binding of obj of different types for scattered policy */
void check_binding_obj(int task, struct worker_info_s *infos, int size, int max_num_threads, hwloc_topology_t topology){
    log_enum = _check_binding_obj;
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE);
    int depth_pu = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
    int i;
    while(depth != depth_pu){
        algo_check_binding(task, &depth, infos, size, max_num_threads, topology);
        depth++;
    }       
}


/* Algo check binding of obj of different types for scattered policy */
void algo_check_binding(int mpirank, int *depth, struct worker_info_s *rbuf, int size, int max_num_threads, hwloc_topology_t topology){
    int nb_obj = hwloc_get_nbobjs_by_depth(topology, *depth);
    int k,i,j;
    int * already_check = calloc(nb_obj,sizeof(int));
    printf("\n  In MPI task:%d \n", mpirank);
    int first_id = nb_obj;
    int last_id = 0;
    int count = 0;

    /* for each mpi rank, count the thread open mp and take the first id and last id */
    for (j=0; j<size*max_num_threads; j++) {
        if (rbuf[j].mpi_rank == mpirank) {
            hwloc_obj_t obj = hwloc_get_ancestor_obj_by_depth(topology , *depth, hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf[j].pu));
            int id = obj->logical_index;
            if (first_id > id) first_id = id;
            if (last_id < id) last_id = id;
            if(already_check[id] == 0){
                already_check[id] = 1;
                count++;
            }
        }
    }


    /* Consecutive ? */
    if (last_id - first_id == count-1) { 
        printf("consecutive obj ids: OK\n");
        fflush(stdout);
    } else {
        printf("consecutive obj ids: FAILED\n");
        fflush(stdout);
        abort_log();
    }
    free(already_check);

}

/*Check for algo check nb obj by obj */
void refine_check(int mpirank, int first_w, int first, int i, int size, int max_num_threads, struct worker_info_s *rbuf_w,struct worker_info_s *rbuf, int depth,int depth_1, hwloc_topology_t topology, 
        hwloc_obj_type_t type_up, hwloc_obj_type_t type_down ){
    int j;
    int nb_obj_tot, nb_obj_tot_d, nb_obj_tot_d_d;

    if(type_up == HWLOC_OBJ_NODE){
        nb_obj_tot = get_nb_node(topology);
    }
    else{
        nb_obj_tot = hwloc_get_nbobjs_by_depth(topology, depth);
    }

    if(type_down == HWLOC_OBJ_NODE){
        nb_obj_tot_d = get_nb_node(topology);

    }
    else{
        nb_obj_tot_d = hwloc_get_nbobjs_by_depth(topology, depth_1);
    }
    int depth_node = hwloc_get_depth_type(topology, HWLOC_OBJ_NODE);
    int **nb_obj_per_type_w = (int **)malloc(nb_obj_tot*sizeof(int*));
    int **nb_obj_per_type = (int **)malloc(nb_obj_tot*sizeof(int*));

    for(j = 0; j < nb_obj_tot; j++){
        nb_obj_per_type_w[j] = (int *)calloc(nb_obj_tot_d + 1,sizeof(int));
    }
    for(j = 0; j < nb_obj_tot; j++){
        nb_obj_per_type[j] = (int *)calloc(nb_obj_tot_d + 1,sizeof(int));
    }
    for(j = 0; j < size * max_num_threads; j++){
        if(rbuf_w[j].mpi_rank == mpirank){
            hwloc_obj_t temp = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf_w[j].pu);
            hwloc_obj_t obj;
            hwloc_obj_t par;
            int id;
            if(depth == depth_node - 1){
                id = rbuf_w[j].node;
                obj = hwloc_get_obj_by_depth(topology , depth_1, id);
            }
            else{
                obj = hwloc_get_ancestor_obj_by_depth(topology , depth_1, temp);
                id = obj->logical_index;
            }
            par = hwloc_get_ancestor_obj_by_depth(topology, depth, obj);
            if((par->logical_index == i) /*&& (depth != depth_node - 1)*/){
                maj_nb_obj_per_task_refine(i, id, nb_obj_per_type_w);
            }
            //printf("tab w at %d nb_pu_per_core_w[i][info_w[j].core] %d\n",info_w[j].core,nb_pu_per_core_w[i][info_w[j].core] );
        }
    }
    for(j = 0; j < size * max_num_threads; j++){
        if(rbuf[j].mpi_rank == mpirank){
            hwloc_obj_t temp = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf[j].pu);
            hwloc_obj_t obj;
            hwloc_obj_t par;
            int id;
            if(depth == depth_node - 1){
                id = rbuf[j].node;
                obj = hwloc_get_obj_by_depth(topology , depth_1, id);
            }
            else{
                obj = hwloc_get_ancestor_obj_by_depth(topology , depth_1, temp);
                id = obj->logical_index;
            }
            par = hwloc_get_ancestor_obj_by_depth(topology, depth, obj);
            if((par->logical_index == i) /*&& (depth != depth_node - 1)*/){
                maj_nb_obj_per_task_refine(i, id, nb_obj_per_type);
            }
        }
    }
    if(nb_obj_per_type_w[i][first_w] - nb_obj_per_type_w[i][first_w+1] > 1 && nb_obj_per_type[i][first + 1] !=  nb_obj_per_type_w[i][first_w + 1]){
        abort_log();
    }
    for(j = 0; j < nb_obj_tot; j++){
        free(nb_obj_per_type_w[j]);
    }
    free(nb_obj_per_type_w);
}

/* Algo check repartition of obj of different types for scattered policy*/
int algo_check_nb_obj_by_obj(int mpirank, int *depth, struct worker_info_s *rbuf, struct worker_info_s *rbuf_w, int size, int max_num_threads, hwloc_topology_t topology){

    int i, j;
    /* To jump chache L1 which is equivalent to cores in the topology KNL*/
    static int type_prec = - 1;
    static int type_prec_1 = - 1;
    static int type_prec_2 = - 1;

    int depth_1 = *depth + 1;
    int depth_2 = *depth + 2;

    hwloc_obj_type_t type_up = hwloc_get_depth_type (topology, *depth);
    hwloc_obj_type_t type_down = hwloc_get_depth_type (topology, *depth + 1);
    hwloc_obj_type_t type_down_d = hwloc_get_depth_type (topology, *depth + 2);

    if(type_prec == type_up){
        (*depth)++;
        depth_1++;
        depth_2++;
        type_up = hwloc_get_depth_type (topology, *depth);
        type_down = hwloc_get_depth_type (topology, depth_1);
        type_down_d = hwloc_get_depth_type (topology, depth_2);
    }
    if(type_prec_1 == type_down){
        depth_1++;
        depth_2++;
        type_down = hwloc_get_depth_type (topology, depth_1);
        type_down_d = hwloc_get_depth_type (topology, depth_2);
    }
    if(type_prec_2 == type_down_d){
        depth_2++;
        type_down_d = hwloc_get_depth_type (topology, depth_2);
    }
    type_prec_2 = type_down_d;
    type_prec_1 = type_down;
    type_prec = type_up;

    int nb_obj_tot, nb_obj_tot_d, nb_obj_tot_d_d;

    if(type_up == HWLOC_OBJ_NODE){
        nb_obj_tot = get_nb_node(topology);
    }
    else{
        nb_obj_tot = hwloc_get_nbobjs_by_depth(topology, *depth);
    }

    if(type_down == HWLOC_OBJ_NODE){
        nb_obj_tot_d = get_nb_node(topology);

    }
    else{
        nb_obj_tot_d = hwloc_get_nbobjs_by_depth(topology, depth_1);
    }
    if(type_down_d == HWLOC_OBJ_NODE){
        nb_obj_tot_d_d = get_nb_node(topology);

    }
    else{
        nb_obj_tot_d_d = hwloc_get_nbobjs_by_depth(topology, depth_2);
    }

    int depth_node = hwloc_get_depth_type(topology, HWLOC_OBJ_NODE);
    for(i = 0; i < nb_obj_tot; i++){
        int find_first = 0;
        int find_first_w = 0;
        int empty_w = 0;

        if(*depth == depth_node && !is_numa_or_mcdram(topology, i)){
            continue;
        }
        int **nb_obj_per_type = (int **)malloc(nb_obj_tot*sizeof(int*));
        int **nb_obj_per_type_w = (int **)malloc(nb_obj_tot*sizeof(int*));
        int *tab_already_check = (int *)calloc(nb_obj_tot_d_d,sizeof(int));
        int *tab_already_check_w = (int *)calloc(nb_obj_tot_d_d,sizeof(int));
        for(j = 0; j < nb_obj_tot; j++){
            nb_obj_per_type[j] = (int *)calloc(nb_obj_tot_d + 1,sizeof(int));
        }

        for(j = 0; j < nb_obj_tot; j++){
            nb_obj_per_type_w[j] = (int *)calloc(nb_obj_tot_d + 1,sizeof(int));
        }
        for(j = 0; j < size * max_num_threads; j++){
            if(rbuf[j].mpi_rank == mpirank){
                hwloc_obj_t temp = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf[j].pu);
                if(temp == NULL){
                }
                hwloc_obj_t obj;
                hwloc_obj_t par;
                int id;
                if(*depth == depth_node - 1){
                    id = rbuf[j].node;
                    obj = hwloc_get_obj_by_depth(topology , depth_1, id);
                }
                else{
                    obj = hwloc_get_ancestor_obj_by_depth(topology , depth_1, temp);
                    id = obj->logical_index;
                }
                par = hwloc_get_ancestor_obj_by_depth(topology, *depth, obj);
                if((par->logical_index == i) /*&& (*depth != depth_node - 1)*/){
                    hwloc_obj_t obj_depth_2 = hwloc_get_ancestor_obj_by_depth(topology, depth_2, temp);
                    int id_depth_2 = obj_depth_2->logical_index;
                    maj_nb_obj_per_task(i, id, id_depth_2, nb_obj_per_type, tab_already_check);
                    //printf("MMaj i%d, id %d\n", i, id);
                }
                //printf("tab at %d nb_pu_per_core[i][info[j].core] %d\n",info[j].core, nb_pu_per_core[i][info[j].core] );
                int k;
                for(k = 0; k < nb_obj_tot_d; k++){
                    if(nb_obj_per_type[i][k] != 0){
                        //	printf("depth %d i %d VVVAL %d\n", *depth, i,nb_obj_per_type[i][k]);
                    }
                }
            }
            if(rbuf_w[j].mpi_rank == mpirank){
                hwloc_obj_t temp = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf_w[j].pu);
                if(temp == NULL){
                    abort_log();
                }
                //hwloc_obj_t temp1 = hwloc_get_ancestor_obj_by_type(topology, type_down, temp);
                /*if(temp1 == NULL){
                  printf("NULL est l'objet wake\n");
                  PContinue  }*/
                hwloc_obj_t obj;
                hwloc_obj_t par;
                int id;
                if(*depth == depth_node - 1){
                    id = rbuf_w[j].node;
                    obj = hwloc_get_obj_by_depth(topology , depth_1, id);
                }
                else{
                    obj = hwloc_get_ancestor_obj_by_depth(topology , depth_1, temp);
                    id = obj->logical_index;
                }
                par = hwloc_get_ancestor_obj_by_depth(topology, *depth, obj);
                if((par->logical_index == i) /*&& (*depth != depth_node - 1)*/){
                    hwloc_obj_t obj_depth_2 = hwloc_get_ancestor_obj_by_depth(topology, depth_2, temp);
                    int id_depth_2 = obj_depth_2->logical_index;
                    maj_nb_obj_per_task(i, id, id_depth_2, nb_obj_per_type_w, tab_already_check_w);
                }
            }
        }

        /* check number and repartition of pu per core of awaken threads for each tasks */

        int first = 0;
        while(1){
            if(nb_obj_per_type[i][first] != 0){	
                find_first = 1;
                break;
            }
            first++;
            if(first == nb_obj_tot_d){
                /* check nb_obj is 0 for the other obj in obj i */
                return depth_2;
            }
        }
        int end = first;
        while(1){
            end++;
            if(end == nb_obj_tot_d){
                break;
            }
            if(nb_obj_per_type[i][end] == 0){
                break;
            }
        }
        int first_w = 0;
        while(1){
            if(nb_obj_per_type_w[i][first_w] != 0){	
                find_first_w = 1;
                break;
            }
            first_w++;
            if(first_w == nb_obj_tot_d){
                /* check nb_obj is 0 for the other obj in obj i */
                if(empty_w == 0){
                    return depth_2;
                }
            }
        }
        int end_w = first_w;
        while(1){
            end_w++;
            if(end_w == nb_obj_tot_d){
                break;
            }
            if(nb_obj_per_type_w[i][end_w] == 0){
                break;
            }
        }

        /* Only one obj top booked */
        if(end - first == 1){
            if(nb_obj_per_type_w[i][first_w] > nb_obj_per_type[i][first_w]){
                abort_log();
                printf("erro:nb_obj_by_obj 1\n");
                fflush(stdout);
            }
        }

        /* Two obj top*/
        if(end - first == 2){
            if(nb_obj_per_type_w[i][first_w] > nb_obj_per_type[i][first_w]){
                printf("erro:nb_obj_by_obj 2\n");
                fflush(stdout);
                abort_log();
            }
            if(nb_obj_per_type_w[i][first_w+1] > nb_obj_per_type[i][first_w+1]){
                printf("erro:nb_obj_by_obj 3\n");
                fflush(stdout);
                abort_log();
            }
            if(nb_obj_per_type_w[i][first_w] < nb_obj_per_type_w[i][first_w+ 1]){
                if(nb_obj_per_type_w[i][first_w] != nb_obj_per_type[i][first_w]){
                    printf("erro:nb_obj_by_obj 4\n");
                    fflush(stdout);
                    abort_log();
                }
            }
            else if(nb_obj_per_type_w[i][first_w] > nb_obj_per_type_w[i][first_w+ 1]){
                if(nb_obj_per_type_w[i][first_w] - nb_obj_per_type_w[i][first_w+1] > 1){
                    if(nb_obj_per_type_w[i][first_w + 1] != nb_obj_per_type[i][first_w+1]){
                        printf("erro:nb_obj_by_obj 5\n");
                        fflush(stdout);
                        abort_log();
                    }
                }
                refine_check(mpirank, first_w, first , i, size, max_num_threads, rbuf_w, rbuf, *depth, depth_1, topology, type_up, type_down);
            }

        }

        /* More than two obj top*/
        if(*depth == 3){
        }
        if(end - first > 2){
            int altern = 1;
            /* first obj top*/
            if(nb_obj_per_type_w[i][first_w] > nb_obj_per_type_w[i][first_w + 1] + 1){
                printf("erro:nb_obj_by_obj 6\n");
                fflush(stdout);
                abort_log();
            }
            else if(nb_obj_per_type_w[i][first_w] == nb_obj_per_type_w[i][first_w+ 1] + 1){
                altern--;
            }
            else{
                if(nb_obj_per_type_w[i][first_w] < nb_obj_per_type_w[i][first_w+ 1]){
                    if(nb_obj_per_type_w[i][first_w] != nb_obj_per_type[i][first_w]){
                        printf("erro:nb_obj_by_obj 7\n");
                        fflush(stdout);
                        abort_log();
                    }
                }
            }
            /* midlle cores */
            for(j = first_w + 2; j < end_w - 1; j++){
                if(nb_obj_per_type_w[i][j] > nb_obj_per_type_w[i][j-1]){
                    if(!altern){ 
                        printf("erro:nb_obj_by_obj 8\n");
                        fflush(stdout);
                        abort_log();
                    }
                }
                else if( nb_obj_per_type_w[i][j] == nb_obj_per_type_w[i][j-1]){
                }
                else{
                    if(nb_obj_per_type_w[i][j] != nb_obj_per_type_w[i][first_w + 1] - 1){
                        printf("erro:nb_obj_by_obj 9\n");
                        fflush(stdout);
                        abort_log();
                    }
                    altern--;
                }
            }
            /* last core */
            if(end_w - first_w > 1){
                if(nb_obj_per_type_w[i][end_w - 1] > nb_obj_per_type_w[i][end_w - 2]){
                    printf("erro:nb_obj_by_obj 10\n");
                    fflush(stdout);
                    abort_log();
                }
                else{
                    if(nb_obj_per_type_w[i][end_w - 2] - nb_obj_per_type_w[i][end_w - 1] > 1){
                        if(nb_obj_per_type_w[i][end_w - 1] != nb_obj_per_type[i][end_w - 1]){
                            printf("erro:nb_obj_by_obj 11\n");
                            fflush(stdout);
                            abort_log();
                        }
                    }	
                }
            }

            /* if obj down booked are wrongly unused */
            if(end > end_w){
                if(nb_obj_per_type[i][end_w] > 0 && nb_obj_per_type_w[i][first_w] > 1){
                    printf("erro:nb_obj_by_obj 12\n");
                    fflush(stdout);
                    abort_log();
                }
            }
        }
        for(j = 0; j < nb_obj_tot; j++){
            free(nb_obj_per_type[j]);
        }
        for(j = 0; j < nb_obj_tot; j++){
            free(nb_obj_per_type_w[j]);
        }
        free(nb_obj_per_type_w);
        free(nb_obj_per_type);
        free(tab_already_check);
        free(tab_already_check_w);
    }
    return depth_2;
}

/* Find a node which is not MCDRAM and which contain pu */
hwloc_obj_t find_node_by_pu( hwloc_topology_t topology, int pu){
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE);
    hwloc_const_cpuset_t topo_cpuset = hwloc_topology_get_complete_cpuset(topology);
    //int nb_obj = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, topo_cpuset, depth);
    //int nb_obj = hwloc_get_nbobjs_by_depth(topology, depth);
    int nb_obj = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    int k;
    hwloc_obj_t pu_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, pu);
    for(k = 0; k < nb_obj; k++){
        hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, k);
        hwloc_const_cpuset_t cpuset = obj->cpuset;
        if(pu_obj == NULL){
            int depth_pu = hwloc_get_depth_type(topology, HWLOC_OBJ_PU);
            pu_obj = hwloc_get_obj_by_depth(topology, depth_pu, pu);
            if(pu_obj == NULL){
            fprintf(stderr, "!!!Node not find\n");
                return NULL; /* No node find */
            }
        }
        if(!hwloc_bitmap_iszero(cpuset)){
            if(hwloc_bitmap_isincluded(pu_obj->cpuset, cpuset)){
                return obj;
            }
        }
    }
    fprintf(stderr, "!!!Node not find nb_obj %d\n", nb_obj);
    return NULL;
}

int get_nb_node(hwloc_topology_t topology){
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE);
    hwloc_const_cpuset_t topo_cpuset = hwloc_topology_get_complete_cpuset(topology);
    int nb_obj = hwloc_get_nbobjs_by_depth(topology, depth);
    int k;
    int count_node = 0;
    for(k = 0; k < nb_obj; k++){
        hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, k);
        hwloc_const_cpuset_t cpuset = obj->cpuset;
        if(!hwloc_bitmap_iszero(cpuset)){
            count_node++;
        }
    }
    return count_node;
}

/* Check if hwloc node is a NUMA node or MCDRAM */
int is_numa_or_mcdram( hwloc_topology_t topology, int node){
    int depth = hwloc_get_depth_type(topology, HWLOC_OBJ_NODE);
    hwloc_const_cpuset_t topo_cpuset = hwloc_topology_get_complete_cpuset(topology);
    hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, node);
    hwloc_const_cpuset_t cpuset = obj->cpuset;
    if(!hwloc_bitmap_iszero(cpuset)){
        return 1;
    }
    else{
        return 0;
    }
}

/* compute the numa node indice without MCDRAM */
void compute_indices_numa(int * tab, hwloc_topology_t topology){
    int i;
    int nb_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    for(i = 0; i < nb_node; i++){
        hwloc_obj_t obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, i);
        hwloc_const_cpuset_t cpuset = obj->cpuset;
        if(!hwloc_bitmap_iszero(cpuset)){
            int k;
            tab[i] = i;
            for(k = 0; k < i; k++){
                hwloc_obj_t node = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, k);
                hwloc_const_cpuset_t set = node->cpuset;
                if(hwloc_bitmap_iszero(set)){
                    tab[i]--;

                }

            }
        }
    }
}

/* Check binding of node for scattered policy */
void consecutive_binding_per_mpi_task_node(int size, int max_num_threads, struct worker_info_s *rbuf, hwloc_topology_t topology, int smt){
    log_enum = _consecutive_binding_per_mpi_task_node;
    /* Check consecutive binding per mpi task on core */
    int i, j;
    /*TODO NODE COULD BE MCDRAM O KNL */
    int nb_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    printf("\n* Checking binding on node... \n");
    int * tab_indices_numa; 
    tab_indices_numa = (int *)calloc(nb_nodes, sizeof(int));
    /* compute the numa node indices without MCDRAM */
    compute_indices_numa(tab_indices_numa, topology);
    for (i=0; i<size; i++) {
        int * already_check = calloc(nb_nodes,sizeof(int));
        int mpirank = i;
        printf("\n  In MPI task:%d \n", i);
        int first_id = nb_nodes;
        int last_id = 0;
        int count = 0;

        /* for each mpi rank, count the thread open mp and take the first id and last id */
        for (j=0; j<size*max_num_threads; j++) {
            if (rbuf[j].mpi_rank == mpirank) {
                //hwloc_obj_t node = hwloc_get_ancestor_obj_by_depth(topology , depth_node, hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, rbuf[j].pu));
                hwloc_obj_t node = find_node_by_pu(topology, rbuf[j].pu);
                int id = tab_indices_numa[node->logical_index];
                if (first_id > id) first_id = id;
                if (last_id < id) last_id = id;

                if(already_check[id] == 0){
                    already_check[id] = 1;
                    count++;
                }
            }
        }

        /* Consecutive ? */
        if (last_id - first_id == (count-1)) { 
            printf("  consecutive node ids: OK\n");
        } else {
            printf("  consecutive node ids: FAILED\n");
            abort_log();
        }

        if(nb_nodes == count){ /* There is enough node used to check the underlining binding */ 
            if(smt){
                check_binding_obj_smt(i, rbuf, size, max_num_threads, topology);
            }
            else{
                check_binding_obj(i, rbuf, size, max_num_threads, topology);
            }
        }
        free(already_check);
    }
    free(tab_indices_numa);
}

/* Check if an instance OpenMP identify each threads with an unique local id*/
void local_unicity(int size, int max_num_threads, struct worker_info_s *rbuf){	
    log_enum = _local_unicity;
    /* Check local id unicity */
    int i, j;
    printf("\n* Checking local id unicity... \n");
    for (i=0; i<size*max_num_threads; i++) {
        if (rbuf[i].mpi_rank != -1) {
            /* looking if this id already existed in the same mpi task */
            for (j=0; j<i; j++) {
                if (rbuf[i].local_id==rbuf[j].local_id && rbuf[i].mpi_rank==rbuf[j].mpi_rank) { 
                    printf("  local unicity: FAILED\n");
                    abort_log();
                }
            }
        }
    }
    printf("  local unicity: OK\n");
}

/* Check that each logical core is used only one time among threads */
void pu_unicity(int size, int max_num_threads, struct worker_info_s *rbuf){
    log_enum = _pu_unicity;
    /* Check pu unicity */
    printf("* Checking pu unicity... \n");
    int i;
    for (i=0; i<size*max_num_threads; i++) {
        if (rbuf[i].mpi_rank != -1) {
            int j;
            /* looking if pu id already existed */
            for (j=0; j<i; j++) {
                if (rbuf[i].pu==rbuf[j].pu && (strcpy(rbuf[i].cluster, rbuf[j].cluster) == 0)) { 
                    printf("  unicity: FAILED\n");
                    abort_log();
                }
            }
        }
    }
    printf("  pu unicity: OK\n");
}

/* Check if all logical cores are used (smt mode), if all topo is used */
void fill_all_pu(int size, int max_num_threads, struct worker_info_s *rbuf){
    log_enum = _fill_all_pu;
    /* need pid unicity and pu unicity OK */
    /* fill all pu */
    printf("\n* checking pus are all used... \n");
    hwloc_topology_t topology = NULL;

    int ret;
    int nb_pus;
    int nb_threads = 0;
    int j;
    for(j = 0; j < size*max_num_threads; j++){
        if(rbuf[j].mpi_rank != -1){
            nb_threads++;
        }
    }
    ret = hwloc_topology_init(&topology);
    assert(ret == 0);
    ret = hwloc_topology_load(topology);
    assert(ret == 0);
    nb_pus= hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    assert(nb_pus != -1);

    /* same number of threads than pu available? */
    if(nb_pus != nb_threads){
        printf("  pus all used: FAILED\n");
        abort_log();
    }


    /* because pid unicity and pu id unicity, test OK */
    printf("  pus all used: OK\n");
}

/* Check if all physical cores are used (smt mode), if all topo is used */
void fill_all_core(int size, int max_num_threads, struct worker_info_s *rbuf){
    log_enum = _fill_all_core;
    /* fill all cores*/
    printf("\n* checking cores are all used... \n");
    hwloc_topology_t topology = NULL;

    int ret;
    int nb_cores;
    int nb_threads = 0;
    int j;
    ret = hwloc_topology_init(&topology);
    assert(ret == 0);
    ret = hwloc_topology_load(topology);
    assert(ret == 0);
    nb_cores= hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    assert(nb_cores!= -1);
    int *check_core = calloc(nb_cores, sizeof(int));
    for(j = 0; j < size*max_num_threads; j++){
        if(rbuf[j].mpi_rank != -1){
            check_core[rbuf[j].core] = 1;
        }
    }
    int nb_core_used = 0;
    for(j = 0; j < nb_cores; j++){
        if(check_core[j]){
            nb_core_used++;
        }
    }

    free(check_core);

    /* same number of threads than core available? */
    if(nb_cores != nb_core_used){
        printf("  cores all used: FAILED\n");
        abort_log();
    }


    /* because pid unicity and core id unicity, test OK */
    printf("  cores all used: OK\n");
}

void sort_buf(struct worker_info_s * rbuf, struct worker_info_s * rbuf_wake_up, int size, int max_num_threads){
    int i;
    int k;
    for(k = 0; k < size*max_num_threads - 1; k++){
        for(i = 0; i < size*max_num_threads - 1; i++){
            if(rbuf[i].pu != -1 && rbuf[i].pu > rbuf[i + 1].pu){
                struct worker_info_s temp = rbuf[i];
                rbuf[i] = rbuf[i + 1];
                rbuf[i + 1] = temp;

            }
            if(rbuf_wake_up[i].pu != -1 && rbuf_wake_up[i].pu > rbuf_wake_up[i + 1].pu){
                struct worker_info_s temp = rbuf_wake_up[i];
                rbuf_wake_up[i] = rbuf_wake_up[i + 1];
                rbuf_wake_up[i + 1] = temp;

            }
        }
    }
}

hwloc_bitmap_t compute_bitmap(hwloc_bitmap_t newset, hwloc_topology_t topology, struct worker_info_s *rbuf_wake_up, int size, int max_num_threads){
    hwloc_bitmap_zero(newset);
    int i;
    for(i = 0; i < size * max_num_threads; i++){
        int pu = rbuf_wake_up[i].pu;
        if(pu >= 0){
            hwloc_bitmap_or(newset, newset, hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, pu)->cpuset);
        }
    }
}


