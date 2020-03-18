#ifndef MPC_COMMON_FLAGS_H_
#define MPC_COMMON_FLAGS_H_

#include <mpc_config.h>

#include "opa_primitives.h"

/******************
 * FLAG STRUCTURE *
 ******************/

struct mpc_common_flags
{
        /* Verbosity level */
        int verbosity;

        /* Launch command */
        char * exename;

	/* Profiler output to be generated (requires MPC_Profiler) */
	char * profiler_outputs;

        /* Topology Management */
        short enable_smt_capabilities;          /*< Should MPC run on hyperthreads */

	/* Accelerator support */
	short enable_accelerators;

	/* Checkpoint mode enabled */
	short checkpoint_enabled;
	char *checkpoint_model;

        /* Topology Display */
        short enable_topology_graphic_placement;/*< Should current pinning be dumped graphically */
        short enable_topology_text_placement;   /*< Should current pinning be dumped in text */

        /* Thread-library */
        char * thread_library_kind;             /*< Type of threading library requested */
        void ( *thread_library_init ) ( void ); /*< Function called to initialize the thread library */
        short new_scheduler_engine_enabled;     /*< If the new thread engine has been enabled */

        /* Launch configuration */
        unsigned int processor_number;          /*< Number of cores passed on the command-line */
        unsigned int task_number;               /*< Number of MPC tasks passed on the CLI */

	/* Network conf */
	char * sctk_network_description_string;
        char * network_driver_name;             /*< Network configuration to be used by MPC */

};

extern struct mpc_common_flags ___mpc_flags;

static inline struct mpc_common_flags * mpc_common_get_flags()
{
        return &___mpc_flags;
}

/* Disguisement Fast Path Checker */

extern OPA_int_t __mpc_p_disguise_flag;

static inline int __MPC_Maybe_disguised()
{
	return OPA_load_int( &__mpc_p_disguise_flag );
}

/***********************
 * INTIALIZATION LISTS *
 ***********************/

#define MPC_INIT_CALL_ONLY_ONCE         static int __already_done = 0;\
                                        if(__already_done)\
                                                return;\
                                        __already_done = 1;

void mpc_common_init_init();
void mpc_common_init_trigger(char * level_name);
void mpc_common_init_list_register(char * list_name);
void mpc_common_init_callback_register(char * list_name, char * callback_name, void (*callback)(), int priority );
void mpc_common_init_print();

#endif /* MPC_COMMON_FLAGS_H_ */