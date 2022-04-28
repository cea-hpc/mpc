#ifndef LOWCOMM_MQD_H_
#define LOWCOMM_MQD_H_

#include <mpc_lowcomm_mqd.h>

#include <mpc_lowcomm_msg.h>


struct mpc_lowcomm_mqs_image
{
};

struct mpc_lowcomm_mqs_process
{
    int local_rank;
    int global_rank;

    mpc_lowcomm_ptp_message_lists_t ** ptp;
    unsigned int list_count;

    mqs_communicator *comms;
    unsigned int comm_count;
    unsigned int comm_current_offset;

    mqs_pending_operation *ops;
    unsigned int ops_count;
    unsigned int ops_current_offset;
};







#endif /* LOWCOMM_MQD_H_ */