#include "lowcomm_config.h"

#include <string.h>
#include <mpc_common_debug.h>
#include <mpc_conf.h>

#include "coll.h"

/*******************
 * COLLECTIVE CONF *
 *******************/

static struct _mpc_lowcomm_coll_conf __coll_conf;

struct _mpc_lowcomm_coll_conf * _mpc_lowcomm_coll_conf_get(void)
{
    return &__coll_conf;
}

static void __mpc_lowcomm_coll_conf_set_default(void)
{
    __coll_conf.algorithm = "noalloc";
    __coll_conf.mpc_lowcomm_coll_init_hook = NULL;

    /* Barrier */
    __coll_conf.barrier_arity = 8;

    /* Bcast */
    __coll_conf.bcast_max_size = 1024;
    __coll_conf.bcast_max_arity = 32;
    __coll_conf.bcast_check_threshold = 512;

     /* Allreduce */
    __coll_conf.allreduce_max_size = 4096;
    __coll_conf.allreduce_max_arity = 8;
    __coll_conf.allreduce_check_threshold = 8192;
    __coll_conf.allreduce_max_slots = 65536;
}


static mpc_conf_config_type_t * __mpc_lowcomm_coll_conf_init(void)
{
    __mpc_lowcomm_coll_conf_set_default();

	mpc_conf_config_type_t *barrier = mpc_conf_config_type_init("barrier",
														     PARAM("arity", &__coll_conf.barrier_arity, MPC_CONF_INT, "Arity of the lowcomm barrier"),
															   NULL);

	mpc_conf_config_type_t *bcast = mpc_conf_config_type_init("bcast",
														      PARAM("maxsize", &__coll_conf.bcast_max_size, MPC_CONF_LONG_INT, "Maximum outoing size per rank for a bcast defines arity (max_size / size)"),
                                                              PARAM("arity", &__coll_conf.bcast_max_arity, MPC_CONF_INT, "Maximum arity  for a bcast"),
                                                              PARAM("check", &__coll_conf.bcast_check_threshold, MPC_CONF_LONG_INT, "Maximum size for checking messages immediately for copy"),
															  NULL);

	mpc_conf_config_type_t *allreduce = mpc_conf_config_type_init("allreduce",
														      PARAM("maxsize", &__coll_conf.allreduce_max_size, MPC_CONF_LONG_INT, "Maximum outoing size per rank for allreduce defines arity (max_size / size)"),
                                                              PARAM("arity", &__coll_conf.allreduce_max_arity, MPC_CONF_INT, "Maximum arity  for allreduce"),
                                                              PARAM("check", &__coll_conf.allreduce_check_threshold, MPC_CONF_LONG_INT, "Maximum size for checking messages immediately for copy"),
                                                              PARAM("check", &__coll_conf.allreduce_max_slots, MPC_CONF_INT, "Maximum number of of slots for allreduce"),
															  NULL);

	mpc_conf_config_type_t *coll = mpc_conf_config_type_init("coll",
														       PARAM("algo", &__coll_conf.algorithm, MPC_CONF_STRING, "Name of the collective module to use (simple, opt, hetero, noalloc)"),
                                                               PARAM("barrier", barrier, MPC_CONF_TYPE, "Options for Barrier"),
                                                               PARAM("bcast", bcast, MPC_CONF_TYPE, "Options for Bcast"),
                                                               PARAM("allreduce", allreduce, MPC_CONF_TYPE, "Options for Bcast"),
															   NULL);



    return coll;
}


void __mpc_lowcomm_coll_conf_validate(void)
{
    if(!strcmp(__coll_conf.algorithm, "noalloc"))
    {
        __coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_noalloc;
    }else if(!strcmp(__coll_conf.algorithm, "simple"))
    {
        __coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_simple;
    }else if(!strcmp(__coll_conf.algorithm, "opt"))
    {
        __coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_opt;
    }else if(!strcmp(__coll_conf.algorithm, "hetero"))
    {
        __coll_conf.mpc_lowcomm_coll_init_hook = mpc_lowcomm_coll_init_hetero;
    }
    else
    {
        bad_parameter("mpcframework.lowcomm.coll.algo must be one of simple, opt, hetero, noalloc, had '%s'", __coll_conf.algorithm);
    }
}


void mpc_lowcomm_coll_init_hook( mpc_lowcomm_communicator_t id )
{
    assume( __coll_conf.mpc_lowcomm_coll_init_hook != NULL);
    (__coll_conf.mpc_lowcomm_coll_init_hook)(id);
}





/************************************
 * GLOBAL CONFIGURATION FOR LOWCOMM *
 ************************************/


void _mpc_lowcomm_config_register(void)
{
    mpc_conf_config_type_t *coll = __mpc_lowcomm_coll_conf_init();

	mpc_conf_config_type_t *lowcomm = mpc_conf_config_type_init("lowcomm",
														       PARAM("coll", coll, MPC_CONF_TYPE, "Lowcomm collective configuration"),
															   NULL);

	mpc_conf_root_config_append("mpcframework", lowcomm, "MPC Lowcomm Configuration");
}

void _mpc_lowcomm_config_validate(void)
{
    __mpc_lowcomm_coll_conf_validate();
}