#ifndef MPC_COMMON_FLAGS_H_
#define MPC_COMMON_FLAGS_H_


struct mpc_common_flags
{
        /* Topology Management */
        short enable_smt_capabilities;          /*< Should MPC run on hyperthreads */

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

};

extern struct mpc_common_flags ___mpc_flags;

static inline struct mpc_common_flags * mpc_common_get_flags()
{
        return &___mpc_flags;
}

#endif /* MPC_COMMON_FLAGS_H_ */