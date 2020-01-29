#ifndef MPC_COMMON_FLAGS_H_
#define MPC_COMMON_FLAGS_H_


struct mpc_common_flags
{
        /* Topology Management */
        short enable_smt_capabilities;          /*< Should MPC run on hyperthreads */

        /* Topology Display */
        short enable_topology_graphic_placement;/*< Should current pinning be dumped graphically */
        short enable_topology_text_placement;   /*< Should current pinning be dumped in text */

        unsigned int processor_number;          /*< Number of cores passed on the command-line */

};

struct mpc_common_flags * mpc_common_get_flags();

#endif /* MPC_COMMON_FLAGS_H_ */