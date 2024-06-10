#include <mpc_common_debug.h>



#define sctk_set_net_migration_available(a) (void)(0);
//~ #define mpc_lowcomm_communicator_remote_size(a) 0
#define sctk_is_valid_comm(a) 1

//#define sctk_net_abort() abort();

#define sctk_register_thread_initial(a) (void)(0);
#define sctk_net_send_task_end(a,b) (void)(0);
#define sctk_net_migration_check(a) (void)(0);
#define sctk_net_hybrid_finalize() (void)(0);

#define sctk_register_restart_thread(a,b) not_implemented();
#define sctk_register_thread(a) not_implemented();
#define sctk_net_migration(a,b) not_implemented()
#define sctk_net_get_pages(a,b,c) not_implemented()
