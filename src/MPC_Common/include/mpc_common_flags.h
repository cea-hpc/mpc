#ifndef MPC_COMMON_FLAGS_H_
#define MPC_COMMON_FLAGS_H_

#include <opa_primitives.h>
#include <mpc_conf.h>

/******************
 * FLAG STRUCTURE *
 ******************/

struct mpc_common_flags
{
        /* Verbosity level */
        int verbosity;
        /** Print callbacks debug information */
	int debug_callbacks;

        /* Launch command */
        char * exename;

	/* Launcher used for MPC */
	char * launcher;

        /* MPC was launched in Fortran */
        short is_fortran;

	/* Profiler output to be generated (requires MPC_Profiler) */
	char * profiler_outputs;

        /* Topology Management */
        short enable_smt_capabilities;          /*< Should MPC run on hyperthreads */

	/* Accelerator support */
	short enable_accelerators;

	/* Checkpoint mode enabled */
	short checkpoint_enabled;
	char *checkpoint_model;

        /* Are we running in a TTY ?*/
        short isatty;

        /* Topology Display */
        short enable_topology_graphic_placement;/*< Should current pinning be dumped graphically */
        short enable_topology_text_placement;   /*< Should current pinning be dumped in text */

        /* Thread-library */
        char thread_library_kind[MPC_CONF_STRING_SIZE];             /*< Type of threading library requested */
        void ( *thread_library_init ) ( void ); /*< Function called to initialize the thread library */

        /* Launch configuration */
	unsigned int node_number;		/*< Number of MPC nodes passed on the CLI */
	unsigned int process_number;		/*< Number of MPC processes passed on the CLI */
        unsigned int processor_number;          /*< Number of cores passed on the command-line */
        unsigned int task_number;               /*< Number of MPC tasks passed on the CLI */

	/* Network conf */
	char * sctk_network_description_string;
        char * network_driver_name;             /*< Network configuration to be used by MPC */

	/* Shell colors */
        #ifdef MPC_ENABLE_SHELL_COLORS
        short colors;                           /*< Enable shell colors */
        #endif

};

extern struct mpc_common_flags ___mpc_flags;

static inline struct mpc_common_flags * mpc_common_get_flags()
{
        return &___mpc_flags;
}

/* Disguisement Fast Path Checker */

extern OPA_int_t __mpc_p_disguise_flag;

static inline int mpc_common_flags_disguised_get()
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
