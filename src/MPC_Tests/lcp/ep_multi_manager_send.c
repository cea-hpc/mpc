#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>

int main(int argc, char** argv) {
	int rc;
	lcp_context_h ctx;
        lcp_context_param_t ctx_params;
        lcp_manager_h mngr1, mngr2;
        lcp_task_h task1, task2;
	lcp_ep_h ep1, ep2;
        lcp_manager_param_t mngr_params;
	mpc_lowcomm_set_uid_t suid;
        int my_tid, src_tid, dest_tid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;

	char data1, data2;

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
		goto err;
	}


	/* create communication context */
        ctx_params = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = 0
        };
	rc = lcp_context_create(&ctx, &ctx_params);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto err;
	}

        mngr_params = (lcp_manager_param_t) {
                .field_mask = LCP_MANAGER_ESTIMATED_EPS |
                        LCP_MANAGER_NUM_TASKS,
                .estimated_eps = 2,
                .num_tasks = 1,
        };
        rc = lcp_manager_create(ctx, &mngr1, &mngr_params);
        if (rc != 0) {
                printf("ERROR: create manager\n");
                goto err;
        }

        rc = lcp_manager_create(ctx, &mngr2, &mngr_params);
        if (rc != 0) {
                printf("ERROR: create manager\n");
                goto err;
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

        rc = lcp_task_create(mngr1, my_tid, &task1);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                printf("ERROR: create task");
                goto err;
        }

	/* sender creates endpoint */
	if (my_tid == 0) {
		rc = lcp_ep_create(mngr1, &ep1, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

        rc = lcp_task_create(mngr2, my_tid, &task2);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                printf("ERROR: create task");
                goto err;
        }

	/* sender creates endpoint */
	if (my_tid == 0) {
		rc = lcp_ep_create(mngr2, &ep2, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	/* init mpc request */
	mpc_lowcomm_request_t req1 = {
		.header = {
			.communicator_id = mpc_lowcomm_get_comm_world_id(),
			.destination = dest_uid,
			.destination_task = (int)dest_uid,
			.msg_size = sizeof(data1),
			.source = src_uid,
			.source_task = (int)src_uid,
			.message_tag = 1111,
		},
		.completion_flag = 0,
	};

	mpc_lowcomm_request_t req2 = {
		.header = {
			.communicator_id = mpc_lowcomm_get_comm_world_id(),
			.destination = dest_uid,
			.destination_task = (int)dest_uid,
			.msg_size = sizeof(data1),
			.source = src_uid,
			.source_task = (int)src_uid,
			.message_tag = 1111,
		},
		.completion_flag = 0,
	};

	mpc_launch_pmi_barrier();

	/* send/recv */
	if (my_tid == 0) {
		data1 = 'c', data2='d';
                lcp_request_param_t param1 = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req1.recv_info,
                };
		rc = lcp_tag_send_nb(ep1, task1, &data1, sizeof(data1), &req1, &param1);
		if (rc != 0) {
			printf("ERROR: send\n");
		}

                lcp_request_param_t param2 = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req2.recv_info,
                };
		rc = lcp_tag_send_nb(ep2, task2, &data2, sizeof(data2), &req2, &param2);
		if (rc != 0) {
			printf("ERROR: send\n");
		}
	} else {
                lcp_request_param_t param1 = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req1.recv_info
                };
		rc = lcp_tag_recv_nb(task1, &data1, sizeof(data1), &req1, &param1);
		if (rc != 0) {
			printf("ERROR: recv\n");
		}

                lcp_request_param_t param2 = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req2.recv_info
                };
		rc = lcp_tag_recv_nb(task2, &data2, sizeof(data2), &req2, &param2);
		if (rc != 0) {
			printf("ERROR: recv\n");
		}
	}


        int check = 0;
	/* progress communication */
	while (req1.completion_flag == 0) {
		usleep(10);
		lcp_progress(mngr1);
	}

        /* Make sure that progress made only on mngr2. */
        if (my_tid == 1) {
                if(req2.completion_flag == 1) {
                        printf("ERROR: request 2 completed while it sould not.\n");
                        goto err;
                }
        }

	/* progress communication */
	while (req2.completion_flag == 0) {
		usleep(10);
		lcp_progress(mngr2);
	}

        if (my_tid == 1) {
                if (data1 == 'c' && data2 == 'd')
                        check = 1;
        } else {
                check = 1;
        }

	_mpc_lowcomm_communicator_release();

        rc = lcp_manager_fini(mngr1);
        if (rc != 0) {
                printf("ERROR: fini manager\n");
                goto err;
        }

        rc = lcp_manager_fini(mngr2);
        if (rc != 0) {
                printf("ERROR: fini manager\n");
                goto err;
        }

	rc = lcp_context_fini(ctx);
	if (rc != 0) {
		printf("ERROR: fini context\n");
                goto err;
	}

	rc = _mpc_lowcomm_monitor_teardown();
	if (rc != 0) {
		printf("ERROR: fini monitor\n");
                goto err;
	}

	rc = mpc_launch_pmi_finalize();
	if (rc != 0) {
		printf("ERROR: fini pmi\n");
                goto err;
	}


        printf("SUCCESS");
err:
	return check ? 0 : 1;
}
