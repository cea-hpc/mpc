#include "mpi_conf.h"

#include <dlfcn.h>

/*********************
 * GLOBAL MPI CONFIG *
 *********************/

struct _mpc_mpi_config __mpc_mpi_config = { 0 };

struct _mpc_mpi_config * _mpc_mpi_config(void)
{
    return &__mpc_mpi_config;
}

/******************
 * NBC PARAMETERS *
 ******************/

static inline __nbc_defaults(void)
{
    struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

    nbc->progress_thread = 0;
    snprintf(nbc->thread_bind_function_name, MPC_CONF_STRING_SIZE, "sctk_get_progress_thread_binding_bind");
    nbc->use_egreq_barrier = 0;
    nbc->use_egreq_bcast = 0;
    nbc->use_egreq_gather = 0;
    nbc->use_egreq_reduce = 0;
    nbc->use_egreq_scatter = 0;
}


mpc_conf_config_type_t * __init_nbc_config(void)
{
    __nbc_defaults();

    struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

    mpc_conf_config_type_t *egreq = mpc_conf_config_type_init("egreq",
                                                                PARAM("barrier", &nbc->use_egreq_barrier, MPC_CONF_BOOL, 
                                                                      "Enable EGREQ Barrier"),
                                                                PARAM("bcast", &nbc->use_egreq_bcast, MPC_CONF_BOOL, 
                                                                      "Enable EGREQ Barrier"),
                                                                    PARAM("scatter", &nbc->use_egreq_scatter, MPC_CONF_BOOL, 
                                                                    "Enable EGREQ Barrier"),
                                                                    PARAM("gather", &nbc->use_egreq_gather, MPC_CONF_BOOL, 
                                                                    "Enable EGREQ Barrier"),
                                                                    PARAM("reduce", &nbc->use_egreq_reduce, MPC_CONF_BOOL, 
                                                                    "Enable EGREQ Barrier"),
                                                                    NULL);

    mpc_conf_config_type_t *progress = mpc_conf_config_type_init("progress",
                                                                 PARAM("progress", &nbc->progress_thread, MPC_CONF_BOOL,
                                                                 "Enable progress thread for NBC"),
                                                                 PARAM("bindfunc",nbc->thread_bind_function_name, MPC_CONF_STRING,
                                                                "Function called to compute progress thread binding"),
                                                                PARAM("priority",&nbc->thread_basic_prio, MPC_CONF_INT,
                                                                "Basic priority for progress threads"),
                                                                NULL);
                                                            

    mpc_conf_config_type_t *ret = mpc_conf_config_type_init("nbc",
                                                            PARAM("egreq",egreq, MPC_CONF_TYPE,
                                                                "Extended Generic Request (EGREQ) based Collectives"),
                                                            PARAM("progress",progress, MPC_CONF_TYPE,
                                                                "NBC Progress thread configuration"),
                                                            NULL);
  

  
    return ret;
}



/*********************
 * GLOBAL PARAMETERS *
 *********************/

void _mpc_mpi_config_check()
{
    /* Ensure NBC binding function is resolved */
    struct _mpc_mpi_config_nbc *nbc = &__mpc_mpi_config.nbc;

    nbc->thread_bind_function = dlsym(nbc->thread_bind_function_name);

    if(!nbc->thread_bind_function)
    {
        bad_parameter("When loading mpcframework.mpi.nbc.progress.bindfunc = %s the function could not be resolved with dlsym", nbc->thread_bind_function_name);
    }

}

static inline void __config_defaults(void)
{
    __nbc_defaults();
    __mpc_mpi_config.message_buffering = 1;


    /* Run a check to resolve the default functions */
    _mpc_mpi_config_check();
} 


void _mpc_mpi_config_init(void)
{
    __config_defaults();


    mpc_conf_config_type_t *mpi = mpc_conf_config_type_init("mpi",
	                                                        PARAM("buffering", &__mpc_mpi_config.message_buffering, MPC_CONF_BOOL, 
                                                                  "Enable small messages buffering"),
                                                            PARAM("nbc", __init_nbc_config(), MPC_CONF_TYPE, 
                                                                  "Configuration for Non-Blocking Collectives (NBC)"),
	                                                        NULL);

	mpc_conf_root_config_append("mpcframework", mpi, "MPC MPI Configuration");
}





