#include <hwloc.h>
#include <mpi.h>
#include <stdio.h>
#include <omp.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

/* Test given in argument of the programm */
extern const char * arg;
extern int abort_or_not;

struct worker_info_s {
	int mpi_rank;
	int tid;
	int local_id;
	int node;
	int tile;
	int core;
	int pu;
	int pu_os;
	int processus_id;
    char cluster[512];
};

struct mpi_info_s{
	int tile;
	int node;
	int nb_core;
	int nb_pu;
	int first_pu;
	int last_pu;
	int nb_loc_th;
};

/* to know where check abort() haapended */
enum func{
_test_nb_pu_per_core,

_test_nb_pu_per_node,

_init_gen_info,

_check_compact_policy,

_test_balanced_placement,

_maj_nb_core_per_task,

_print_info_knl,

_write_info_in_log,

_fill_all_core,

_print_info_placement,

_print_info_placement_wake_up,

_fill_info,

_print_header,

_tid_unicity,

_get_node_by_pu,

_consecutive_binding_per_mpi_task_pu,

_consecutive_binding_per_mpi_task_core,

_consecutive_binding_per_mpi_task_node,

_local_unicity,

_pu_unicity,

_fill_all_pu,

_init_struct,

_check_nb_obj_by_obj_smt,

_check_nb_obj_by_obj,

_check_binding_obj_smt,

_check_binding_obj

};

extern enum func log_enum;

void abort_log(void);

void string_abort(char * info);

void test_nb_pu_per_core(struct worker_info_s *info,struct worker_info_s *info_w, 
		struct mpi_info_s *mpi_info, hwloc_topology_t topology, 
		int size, int max_num_threads);

void init_gen_info(int size, int max_num_threads, 
		struct worker_info_s *info, struct mpi_info_s *mpi_info, 
		hwloc_topology_t topology);

void check_compact_policy(int size, struct worker_info_s* info, 
		struct mpi_info_s *mpi_info,  int max_num_threads, 
		hwloc_topology_t topology);

void test_balanced_placement(struct worker_info_s *info, 
		hwloc_topology_t topology, 
		int size, int max_num_threads);

void maj_nb_core_per_task(int i, int core, 
		int **nb_pu_per_core);

void maj_nb_obj_per_task(int i, int obj,int id_depth_2, int **nb_obj_per_type, int *tab_already_check);

void print_info_knl(hwloc_topology_t topology);

void write_info_in_log(struct worker_info_s *rbuf, 
		int size, int max_num_threads);

void fill_all_core(int size, int max_num_threads, 
		struct worker_info_s *rbuf);

void print_info_placement(struct worker_info_s *info, struct worker_info_s *info_wake_up,
		int size, int max_num_threads);

void fill_info(struct worker_info_s *info, int rank, 
        hwloc_topology_t topology, int size);

void print_header(int rank) ;

void tid_unicity(int size, int max_num_threads, 
		struct worker_info_s *rbuf);

hwloc_obj_t get_node_by_pu(int pu, hwloc_topology_t topology);

void consecutive_binding_per_mpi_task_pu(int size, int max_num_threads, 
		struct worker_info_s *rbuf, hwloc_topology_t topology);

void consecutive_binding_per_mpi_task_core(int size, int max_num_threads, 
		struct worker_info_s *rbuf, hwloc_topology_t topology);

void consecutive_binding_per_mpi_task_node(int size, int max_num_threads, 
		struct worker_info_s *rbuf, hwloc_topology_t topology, int smt);

void local_unicity(int size, int max_num_threads, struct worker_info_s *rbuf);

void pu_unicity(int size, int max_num_threads, struct worker_info_s *rbuf);

void fill_all_pu(int size, int max_num_threads, struct worker_info_s *rbuf);

void init_struct(int size, int max_num_threads, struct worker_info_s *info);

void check_binding_obj(int task, struct worker_info_s *infos, int size, int max_num_threads, hwloc_topology_t topology);

void check_nb_obj_by_obj(struct worker_info_s *infos,struct worker_info_s *infos_w, int size, int max_num_threads, hwloc_topology_t topology, int smt);

void algo_check_binding(int task, int *depth, struct worker_info_s *rbuf, int size, int max_num_threads, hwloc_topology_t topology);

int algo_check_nb_obj_by_obj(int mpirank, int *depth, struct worker_info_s *rbuf_w, struct worker_info_s *rbuf, int size, int max_num_threads, hwloc_topology_t topology);

void check_binding_obj_smt(int task, struct worker_info_s *infos, int size, int max_num_threads, hwloc_topology_t topology);

hwloc_obj_t find_node_by_pu( hwloc_topology_t topology, int pu);

void compute_indices_numa(int * tab, hwloc_topology_t topology);

int is_numa_or_mcdram( hwloc_topology_t topology, int node);

int get_nb_node(hwloc_topology_t topology);

void refine_check(int mpirank, int first_w, int first, int i, int size, int max_num_threads, struct worker_info_s *rbuf_w,struct worker_info_s *rbuf, int depth,int depth_1, hwloc_topology_t topology, 
hwloc_obj_type_t type_up, hwloc_obj_type_t type_down );

void maj_nb_obj_per_task_refine(int i, int obj, int **nb_obj_per_type);

void sort_buf(struct worker_info_s * rbuf, struct worker_info_s * rbuf_wake_up, int size, int max_num_threads);

hwloc_bitmap_t compute_bitmap(hwloc_bitmap_t bitmap, hwloc_topology_t topology, struct worker_info_s *rbuf_wake_up, int size, int max_num_threads);
