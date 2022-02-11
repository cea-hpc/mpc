#include "mqd.h"

#include <mpc_config.h>
#include <stdio.h>

#include "sctk_alloc.h"
#include "mpc_common_rank.h"
#include "communicator.h"

char MPIR_dll_name[] = "libmpclowcomm.so";

/**************************************
* EXECUTABLE IMAGE RELATED FUNCTIONS *
**************************************/

int mqs_setup_image(mqs_image *image, mqs_image_callbacks *cb)
{
    return mqs_ok;
}

int mqs_image_has_queues(mqs_image *image, char **message)
{
    *message = "MPC %s";
    return mqs_ok;
}

int mqs_destroy_image_info(mqs_image_info *info)
{
    return mqs_ok;
}


/*****************************
* PROCESS RELATED FUNCTIONS *
*****************************/

int mqs_setup_process(mqs_process *process, const mqs_process_callbacks *cb)
{
    struct mpc_lowcomm_mqs_process * ret = sctk_malloc(sizeof( struct mpc_lowcomm_mqs_process ));

    if(!ret)
    {
        return mqs_no_information;
    }

    static int current_exported_process_offset = 0;

    if(mpc_common_get_local_process_count() <= current_exported_process_offset)
    {
        return mqs_no_information;
    }

    memset(ret, 0, sizeof(struct mpc_lowcomm_mqs_process));

    int fist_w_rank = _mpc_lowcomm_communicator_world_first_local_task();

    ret->local_rank = current_exported_process_offset;
    ret->global_rank = ret->local_rank + fist_w_rank;

    current_exported_process_offset++;
    return mqs_ok;
}


int mqs_process_has_queues(mqs_process *process, char **message);
int mqs_destroy_process_info(mqs_process_info *processinfo);


/***************************
 * SETUP RELATED FUNCTIONS *
 ***************************/

void mqs_setup_basic_callbacks(const mqs_basic_callbacks *cb)
{
    /* NO OP */
}

char * mqs_version_string()
{
    static char buff[32];
    snprintf(buff, 32, "mqs@%s", MPC_VERSION_STRING);
    return buff;
}

int mqs_version_compatibility()
{
    return MQS_INTERFACE_COMPATIBILITY;
}

int mqs_dll_taddr_width()
{
    return sizeof(mqs_taddr_t);
}

char * mqs_dll_error_string(int error_code)
{
    mqs_err code = error_code;

    switch (code)
    {
        case mqs_ok:
            return "mqs_ok";
        case mqs_no_information:
            return "v";
        case mqs_end_of_list:
            return "mqs_end_of_list";
        case mqs_first_user_code:
            return "mqs_first_user_code";
    }

    return "No such error code";
}