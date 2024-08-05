#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>

//NOTE: completion flag:
//      * 0: pending
//      * 1: done
//      * 2: cancelled

int lowcomm_request_complete(mpc_lowcomm_request_t *req){
	req->completion_flag = 1;
	return 0;
}

int main(int argc, char** argv) {
	int rc;
	lcp_context_h ctx;
        lcp_context_param_t ctx_params;
        lcp_manager_h mngr;
        lcp_manager_param_t mngr_params;
        lcp_task_h task;
	lcp_ep_h ep;
	mpc_lowcomm_set_uid_t suid;
        int my_tid, src_tid, dest_tid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;

	char data;

	/* load default config */
	mpc_conf_root_config_init("mpcframework");
	_mpc_lowcomm_config_register();
	_mpc_lowcomm_config_validate();
	mpc_conf_root_config_load_env_all();

	/* init pmi */
	rc = mpc_launch_pmi_init();
	if (rc != 0) {
		printf("ERROR: init pmi\n");
		goto err;
	}

	/* setup monitor: necessary to exchange set uid */
	rc = _mpc_lowcomm_monitor_setup();
	if (rc != 0) {
		printf("ERROR: monitor setup\n");
		goto pmi_fini;
	}

        ctx_params = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = mpc_lowcomm_monitor_get_uid()
        };
	rc = lcp_context_create(&ctx, &ctx_params);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto monitor_fini;
	}

        mngr_params = (lcp_manager_param_t) {
                .field_mask = LCP_MANAGER_ESTIMATED_EPS |
                        LCP_MANAGER_NUM_TASKS,
                .estimated_eps = 2,
                .num_tasks = 1,
        };
        rc = lcp_manager_create(ctx, &mngr, &mngr_params);
        if (rc != 0) {
                printf("ERROR: create manager\n");
                goto monitor_fini;
        }

	/* pmi barrier needed to commit pmi rail registration */
	mpc_launch_pmi_barrier();

	/* init world and self comm */
	_mpc_lowcomm_communicator_init();

	/* get uids */
	suid = mpc_lowcomm_monitor_get_gid();
        my_tid = mpc_lowcomm_get_rank();

        if (my_tid == 0) {
                src_tid  = mpc_lowcomm_get_rank();
                src_uid  = mpc_lowcomm_monitor_get_uid();
                dest_tid = 1;
                dest_uid = mpc_lowcomm_monitor_uid_of(suid, 1);
        } else {
                src_tid = 0;
                src_uid = mpc_lowcomm_monitor_uid_of(suid, 0);
                dest_uid = mpc_lowcomm_monitor_get_uid();
                dest_tid  = mpc_lowcomm_get_rank();
        }

        rc = lcp_task_create(mngr, my_tid, &task);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                printf("ERROR: create task");
                goto monitor_fini;
        }

	/* sender creates endpoint */
	if (my_tid == 0) {
		rc = lcp_ep_create(mngr, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	/* init mpc request */
	mpc_lowcomm_request_t req = {
		.header = {
			.communicator_id = mpc_lowcomm_get_comm_world_id(),
			.destination = dest_uid,
			.destination_task = (int)dest_uid,
			.msg_size = sizeof(data),
			.source = src_uid,
			.source_task = (int)src_uid,
			.message_tag = 1111,
		},
		.completion_flag = 0,
		.request_completion_fn = lowcomm_request_complete
	};

	mpc_launch_pmi_barrier();

	/* send/recv */
	if (my_tid == 0) {
		data = 'c';
                lcp_request_param_t param = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req.recv_info,
                };
		rc = lcp_tag_send_nb(ep, task, &data, sizeof(data), &req, &param);
		if (rc != 0) {
			printf("ERROR: send\n");
		}
	} else {
		/* wait to force early send */
                usleep(1000);
<<<<<<< HEAD:src/MPC_Tests/lcp/ep_earlysend.c
		lcp_progress(ctx);
                lcp_request_param_t param = {
=======
                /* progress so that message can be put in unexpected list. */
		lcp_progress(mngr);
                lcp_request_param_t param = {
>>>>>>> e285140f5 (WIP. LCP: communication manager for software communication context.):tests/lcp/ep_earlysend.c
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req.recv_info
                };
		rc = lcp_tag_recv_nb(task, &data, sizeof(data), &req, &param);
		if (rc != 0) {
			printf("ERROR: recv\n");
		}
	}

	/* progress communication */
	while (req.completion_flag == 0) {
		usleep(10);
		lcp_progress(mngr);
	}

        int check = 0;
        if (my_tid == 1) {
                if (data == 'c')
                        check = 1;
        } else {
                check = 1;
        }

	_mpc_lowcomm_communicator_release();

        rc = lcp_manager_fini(mngr);
        if (rc != 0) {
                printf("ERROR: fini manager\n");
        }

	rc = lcp_context_fini(ctx);
	if (rc != 0) {
		printf("ERROR: fini context\n");
	}

monitor_fini:
	rc = _mpc_lowcomm_monitor_teardown();
	if (rc != 0) {
		printf("ERROR: fini monitor\n");
	}
pmi_fini:
	rc = mpc_launch_pmi_finalize();
	if (rc != 0) {
		printf("ERROR: fini pmi\n");
	}
err:
	return check ? 0 : 1;
}
